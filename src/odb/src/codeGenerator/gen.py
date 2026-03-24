#!/usr/bin/python3

# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2021-2025, The OpenROAD Authors

import argparse
import json
import logging
import re
import shutil
from pathlib import Path
from subprocess import call
from typing import Any, Dict, List, Optional, Set

from jinja2 import Environment, FileSystemLoader

from helper import (
    add_once_to_dict,
    components,
    fnv1a_32,
    get_class,
    get_functional_name,
    get_hash_table_type,
    get_plural_name,
    get_ref_type,
    get_table_name,
    is_bit_fields,
    is_hash_table,
    is_pass_by_ref,
    is_ref,
    is_set_by_ref,
    is_template_type,
    get_template_types,
    std,
)
from parser import Parser
from schema_models import Field, Enum, Struct, Class, Schema

# map types to their header if it isn't equal to their name
STD_TYPE_HDR = {
    "int16_t": "cstdint",
    "uint32_t": "cstdint",
    "pair": "utility",
}


def get_json_files(directory: Path) -> List[Path]:
    """Return all json files under directory recursively."""
    return list(directory.rglob("*.json"))


def make_parent_field(parent: Class, relation: Dict[str, Any]) -> Field:
    """Adds a table field to the parent of a relationship."""
    field = Field(
        name=relation["tbl_name"],
        type=relation["child"],
        table=True,
        dbSetGetter=True,
        components=[relation["tbl_name"]],
        flags=["no-set"] + relation.get("flags", []),
    )
    if "page_size" in relation:
        field.page_size = relation["page_size"]
    if "schema" in relation:
        field.schema = relation["schema"]

    parent.fields.append(field)
    if relation["parent"] != relation["child"]:
        parent.cpp_includes.extend([f"{relation['child']}.h", "odb/dbSet.h"])
    logging.debug(f"Add relation field {field.name} to {relation['parent']}")
    return field


def make_parent_hash_field(
    parent: Class, relation: Dict[str, Any], parent_field: Field
) -> Field:
    """Adds a hash table field to the parent of a hashed relationship."""
    page_size_part = f", {relation['page_size']}" if "page_size" in relation else ""
    field = Field(
        name=parent_field.name[:-4] + "hash_",
        type=f"dbHashTable<_{relation['child']}{page_size_part}>",
        components=[parent_field.name[:-4] + "hash_"],
        table_name=parent_field.name,
        flags=["no-set"] + relation.get("flags", []),
    )
    parent.fields.append(field)
    if "dbHashTable.h" not in parent.h_includes:
        parent.h_includes.append("dbHashTable.h")
    logging.debug(f"Add hash field {field.name} to {relation['parent']}")
    return field


def make_child_next_field(child: Class, relation: Dict[str, Any]) -> None:
    """Adds a next entry field to the child of a hashed relationship."""
    field = Field(
        name="next_entry_",
        type=f"dbId<_{relation['child']}>",
        flags=["private"] + relation.get("flags", []),
    )
    child.fields.append(field)
    logging.debug(f"Add hash field {field.name} to {relation['child']}")


def add_field_attributes(
    field: Field, klass: Class, flags_struct: Struct, schema: Schema
) -> int:
    """Adds various derived attributes to a field."""
    flag_num_bits = 0
    if field.type == "bit":
        field.type = "bool"
        field.bits = 1

    if field.bits:
        flags_struct.fields.append(field)
        flag_num_bits = int(field.bits)

    field.bitFields = is_bit_fields(field, klass.structs)

    # Handle types from <cstdint> that are also in the global namespace (eg uint32_t)
    hdr = STD_TYPE_HDR.get(field.type)
    if hdr:
        klass.h_sys_includes.append(hdr)
        klass.cpp_sys_includes.append(hdr)

    if field.type.startswith("std::"):
        for t in re.findall(r"std::(\w+)", field.type):
            hdr = STD_TYPE_HDR.get(t, t)
            klass.h_sys_includes.append(hdr)
            klass.cpp_sys_includes.append(hdr)

    field.isRef = is_ref(field.type) if field.parent is not None else False
    field.refType = get_ref_type(field.type)

    # refTable is the table name from which the getter extracts the pointer to dbObject
    if field.isRef and field.refType:
        field.refTable = get_table_name(field.refType.replace("*", ""))
        # checking if there is a defined relation between parent and refType for extracting table name
        for relation in schema.relations:
            if relation["parent"] == field.parent and relation[
                "child"
            ] == field.refType.replace("*", ""):
                field.refTable = relation["tbl_name"]

    field.isHashTable = is_hash_table(field.type)
    field.hashTableType = get_hash_table_type(field.type)
    field.isPassByRef = is_pass_by_ref(field.type)
    field.isSetByRef = "set-const-ref" in field.flags or is_set_by_ref(field.type)

    if not field.argument:
        field.argument = field.name.strip("_")

    if "private" in field.flags:
        if "no-set" not in field.flags:
            field.flags.append("no-set")
        if "no-get" not in field.flags:
            field.flags.append("no-get")

    # Check if a class is being used inside a template definition to add
    # to the list of forward declared classes
    if is_template_type(field.type):
        for template_class_name in get_template_types(field.type):
            name_check = (
                template_class_name not in klass.declared_classes
                and template_class_name not in std
                and not template_class_name.isdigit()
                and template_class_name not in {"true", "false"}
                and "no-template" not in field.flags
                and klass.name != template_class_name[1:]
                and klass.name + "::" != template_class_name[: len(klass.name) + 2]
            )
            if name_check:
                klass.declared_classes.append(template_class_name)

    if field.table:
        klass.hasTables = True
        field.functional_name = get_plural_name(
            field.type[2:] if field.type.startswith("db") else field.type
        )
        field.components = [field.name]
    elif field.isHashTable:
        field.functional_name = get_plural_name(field.type[2:])
    else:
        field.functional_name = get_functional_name(field.name)
        field.components = components(klass.structs, field.name, field.type)

    if not field.setterFunctionName:
        field.setterFunctionName = f"set{field.functional_name}"

    getter_prefix = "is" if (field.type == "bool" or field.bits == 1) else "get"
    if not field.getterFunctionName:
        field.getterFunctionName = f"{getter_prefix}{field.functional_name}"

    if field.isRef:
        field.setterArgumentType = field.getterReturnType = field.refType
    elif field.isHashTable:
        if "no-set" not in field.flags:
            field.flags.append("no-set")
        field.setterArgumentType = field.getterReturnType = (
            field.hashTableType.replace("_", "") if field.hashTableType else ""
        )
        field.getterFunctionName = (
            f"find{field.setterArgumentType[2:-1]}" if field.setterArgumentType else ""
        )
    elif field.bits == 1:
        field.setterArgumentType = field.getterReturnType = "bool"
    elif field.isPassByRef:
        field.setterArgumentType = field.getterReturnType = field.type.replace(
            "dbVector", "std::vector"
        )
    elif field.type == "char *":
        field.setterArgumentType = field.type
        field.getterReturnType = "const char *"
    else:
        field.setterArgumentType = field.getterReturnType = field.type

    # For fields that we need to free/destroy in the destructor
    needs_free = (
        field.name == "name_"
        and field.type == "char *"
        and "no-destruct" not in field.flags
    )

    if needs_free or field.table:
        klass.needs_non_default_destructor = True
        if needs_free:
            klass.cpp_sys_includes.append("cstdlib")

    return flag_num_bits


def add_bitfield_flags(klass: Class, flag_num_bits: int, flags_struct: Struct) -> None:
    """Create a flags field for all the bitfields in a class."""
    klass.fields = [field for field in klass.fields if not field.bits]
    klass.hasBitFields = flag_num_bits > 0

    if flag_num_bits == 0:
        return

    total_num_bits = flag_num_bits
    if flag_num_bits % 32 != 0:
        spare_bits = 32 - (flag_num_bits % 32)
        spare_bits_field = Field(
            name="spare_bits",
            type="uint32_t",
            bits=spare_bits,
            flags=["no-cmp", "no-set", "no-get", "no-serial"],
        )
        total_num_bits += spare_bits
        flags_struct.fields.append(spare_bits_field)

    if flags_struct.fields:
        flags_struct.in_class_name = "flags_"
        klass.structs.insert(0, flags_struct)
        klass.fields.insert(
            0,
            Field(
                name="flags_",
                type=flags_struct.name,
                components=components(klass.structs, "flags_", flags_struct.name),
                bitFields=True,
                numBits=total_num_bits,
                flags=["no-cmp", "no-set", "no-get", "no-serial"],
            ),
        )


def generate_relations(schema: Schema) -> None:
    """Generate the parent and child fields for all relationships."""
    for relation in schema.relations:
        if relation["type"] != "1_n":
            raise KeyError("relation type is not supported, use 1_n")

        parent = next(c for c in schema.classes if c.name == relation["parent"])
        child = next(c for c in schema.classes if c.name == relation["child"])

        parent_field = make_parent_field(parent, relation)
        child_type_name = f"_{relation['child']}"

        if child_type_name not in parent.declared_classes:
            parent.declared_classes.append(child_type_name)

        if relation.get("hash", False):
            make_parent_hash_field(parent, relation, parent_field)
            make_child_next_field(child, relation)


def add_include(
    klass: Class, key: str, include: str, cpp: bool = False, sys: bool = False
) -> None:
    """Conditionally adds an include if a type key is present in any field."""
    if any(key in field.type for field in klass.fields):
        includes = klass.h_sys_includes if sys else klass.h_includes
        if include not in includes:
            includes.insert(0, include)

        if cpp:
            cpp_includes = klass.cpp_sys_includes if sys else klass.cpp_includes
            if include not in cpp_includes:
                cpp_includes.insert(0, include)


def preprocess_klass(klass: Class) -> None:
    """Final adjustments to class data before rendering."""
    klass.declared_classes.insert(0, "dbIStream")
    klass.declared_classes.insert(1, "dbOStream")

    if klass.name != "dbDatabase":
        klass.declared_classes.insert(2, "_dbDatabase")
        klass.cpp_includes.append("dbDatabase.h")

    klass.h_includes.insert(0, "dbCore.h")
    klass.h_sys_includes.insert(1, "cstdint")

    name = klass.name
    klass.cpp_includes.extend(["dbTable.h", "dbCore.h", "odb/db.h", f"{name}.h"])

    if klass.hasBitFields:
        klass.cpp_sys_includes.extend(["cstdint", "cstring"])

    add_include(klass, "dbObject", "dbCore.h", cpp=True)
    add_include(klass, "dbId<", "odb/dbId.h")
    add_include(klass, "dbVector", "dbVector.h")
    add_include(klass, "dbObjectType", "odb/dbObject.h", cpp=True)
    add_include(klass, "tuple", "tuple", cpp=True, sys=True)

    for field in klass.fields:
        if field.table:
            page_size_part = f", {field.page_size}" if field.page_size else ""
            this_or_db = "this" if klass.name == "dbDatabase" else "db"
            field.default = (
                f"new dbTable<_{field.type}{page_size_part}>({this_or_db}, this, "
                f"(GetObjTbl_t) &_{klass.name}::getObjectTable, {field.type}Obj)"
            )
            field.table_base_type = field.type
            field.type = f"dbTable<_{field.type}{page_size_part}>*"


class ODBGenerator:
    def __init__(
        self, env: Environment, include_dir: Path, src_dir: Path, keep_empty: bool
    ):
        self.env = env
        self.include_dir = include_dir
        self.src_dir = src_dir
        self.keep_empty = keep_empty
        self.generated_dir = Path("generated")

    def load_schema(self, json_path: Path) -> Schema:
        with open(json_path, encoding="ascii") as f:
            data = json.load(f)

        schema = Schema.from_dict(data)

        classes_dir = Path(schema.classes_dir)
        for file_path in get_json_files(classes_dir):
            with open(file_path, encoding="ascii") as f:
                klass_data = json.load(f)
                klass = Class.from_dict(klass_data)
                schema.classes.append(klass)

        return schema

    def process_schema(self, schema: Schema) -> None:
        generate_relations(schema)

        hash_dict = {}
        for klass in schema.classes:
            flags_struct = Struct(
                name=f"{klass.name}Flags", fields=[], flags=["no-serializer"]
            )
            klass.hasTables = False
            flag_num_bits = sum(
                add_field_attributes(field, klass, flags_struct, schema)
                for field in klass.fields
            )

            add_bitfield_flags(klass, flag_num_bits, flags_struct)

            if any(s.public for s in klass.structs):
                if "odb/db.h" not in klass.h_includes:
                    klass.h_includes.append("odb/db.h")

            # Hash handling
            hash_value = int(klass.hash, 16) if klass.hash else fnv1a_32(klass.name)
            if hash_value in hash_dict:
                raise ValueError(
                    f"Collision detected for {klass.name} with {hash_dict[hash_value]}"
                )
            hash_dict[hash_value] = klass.name
            klass.hash = f"0x{hash_value:08X}"

            preprocess_klass(klass)
            self._prepare_comparisons(klass)

    def _prepare_comparisons(self, klass: Class) -> None:
        """Pre-calculate fields for operator== and operator<."""
        logging.debug(f"Preparing comparisons for {klass.name}")
        klass.equal_fields = []
        klass.less_fields = []

        for field in klass.fields:
            if field.bitFields:
                # Bitfields are in the first struct (flags_)
                flags_struct = klass.structs[0]
                for inner in flags_struct.fields:
                    if "no-cmp" not in inner.flags:
                        for comp in inner.components:
                            klass.equal_fields.append(
                                {
                                    "left": f"{field.name}.{comp}",
                                    "right": f"rhs.{field.name}.{comp}",
                                }
                            )
                    if "cmpgt" in inner.flags:
                        for comp in inner.components:
                            klass.less_fields.append(
                                {
                                    "left": f"{field.name}.{comp}",
                                    "right": f"rhs.{field.name}.{comp}",
                                }
                            )
            else:
                if "no-cmp" not in field.flags:
                    for comp in field.components:
                        deref = "*" if field.table else ""
                        klass.equal_fields.append(
                            {"left": f"{deref}{comp}", "right": f"{deref}rhs.{comp}"}
                        )
                if "cmpgt" in field.flags:
                    for comp in field.components:
                        klass.less_fields.append(
                            {"left": f"{comp}", "right": f"rhs.{comp}"}
                        )

    def generate(self, schema: Schema) -> None:
        print("###################Code Generation Begin###################")
        self.process_schema(schema)

        to_be_merged = []

        # Generate Class files
        for klass in schema.classes:
            for template_base in ["impl.h", "impl.cpp"]:
                template_file = f"{template_base}.jinja"
                out_name = f"{klass.name}.{template_base.split('.')[1]}"
                self._render_to_file(
                    template_file, out_name, klass=klass, schema=schema
                )
                to_be_merged.append(out_name)

        # Generate General files
        general_templates = [
            "db.h",
            "dbObject.h",
            "CMakeLists.txt",
            "dbObject.cpp",
            "dbCompare.inc",
        ]
        for template_base in general_templates:
            template_file = f"{template_base}.jinja"
            self._render_to_file(template_file, template_base, schema=schema)
            to_be_merged.append(template_base)

        # Generate Iterator files
        for itr in schema.iterators:
            for template_base in ["itr.h", "itr.cpp"]:
                template_file = f"{template_base}.jinja"
                out_name = f"{itr['name']}.{template_base.split('.')[1]}"
                self._render_to_file(template_file, out_name, itr=itr, schema=schema)
                to_be_merged.append(out_name)

        # Merge and Format
        self._merge_files(to_be_merged)

        with open(self.generated_dir / "final.json", "w") as f:
            json.dump(
                schema,
                f,
                indent=2,
                default=lambda o: o.__dict__ if hasattr(o, "__dict__") else str(o),
            )
        print("###################Code Generation End###################")

    def _render_to_file(self, template_name: str, out_name: str, **kwargs) -> None:
        template = self.env.get_template(template_name)
        text = template.render(**kwargs)
        with open(self.generated_dir / out_name, "w", encoding="ascii") as f:
            f.write(text)

    def _merge_files(self, file_names: List[str]) -> None:
        includes = {"db.h", "dbObject.h", "dbCompare.inc"}
        for item in file_names:
            target_dir = self.include_dir if item in includes else self.src_dir
            target_path = target_dir / item
            gen_path = self.generated_dir / item

            if target_path.exists():
                parser = Parser(str(target_path))
                if item == "CMakeLists.txt":
                    parser.set_comment_str("#")
                parser.parse_user_code()
                parser.clean_code()
                parser.parse_source_code(str(gen_path))
                parser.write_in_file(str(target_path), self.keep_empty)
            else:
                shutil.copy(gen_path, target_path)

            if item != "CMakeLists.txt":
                if call(["clang-format", "-i", str(target_path)]) != 0:
                    print(f"Failed to format {target_path}")
            print(f"Generated: {target_path}")


def main():
    parser = argparse.ArgumentParser(
        description="Code generator",
        formatter_class=argparse.ArgumentDefaultsHelpFormatter,
    )
    parser.add_argument("--json", default="schema.json", help="json schema filename")
    parser.add_argument("--src_dir", default="../db", help="odb src dir")
    parser.add_argument(
        "--include_dir", default="../../include/odb", help="odb include dir"
    )
    parser.add_argument("--templates", default="templates", help="jinja templates dir")
    parser.add_argument("--log", default="INFO")
    parser.add_argument("--keep_generated", action="store_true")
    parser.add_argument("--keep_empty", action="store_true")

    args = parser.parse_args()
    logging.basicConfig(level=getattr(logging, args.log.upper()))

    generated_dir = Path("generated")
    if generated_dir.exists():
        shutil.rmtree(generated_dir)
    generated_dir.mkdir()

    env = Environment(loader=FileSystemLoader(args.templates), trim_blocks=True)
    generator = ODBGenerator(
        env, Path(args.include_dir), Path(args.src_dir), args.keep_empty
    )

    schema = generator.load_schema(Path(args.json))
    generator.generate(schema)

    if not args.keep_generated:
        shutil.rmtree(generated_dir)


if __name__ == "__main__":
    main()
