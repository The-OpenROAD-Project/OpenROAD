#!/usr/bin/python3

# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2021-2025, The OpenROAD Authors

from subprocess import call
import argparse
import os
import re
import shutil
import json
import logging
from parser import Parser
from jinja2 import Environment, FileSystemLoader
from helper import (
    add_once_to_dict,
    components,
    get_class,
    get_functional_name,
    get_plural_name,
    get_hash_table_type,
    get_ref_type,
    get_table_name,
    is_template_type,
    get_template_types,
    is_bit_fields,
    is_hash_table,
    is_pass_by_ref,
    is_set_by_ref,
    is_ref,
    std,
    fnv1a_32,
)

# map types to their header if it isn't equal to their name
std_type_hdr = {
    "int16_t": "cstdint",
    "uint32_t": "cstdint",
    "pair": "utility",
}


def get_json_files(directory):
    """Return all json files under directory recursively"""
    json_files = []
    for root, _, files in os.walk(directory):
        for file in files:
            if file.endswith(".json"):
                json_files.append(os.path.join(root, file))
    return json_files


def make_parent_field(parent, relation):
    """Adds a table field to the parent of a relationsip"""
    field = {}
    field["name"] = relation["tbl_name"]
    field["type"] = relation["child"]
    field["table"] = True
    field["dbSetGetter"] = True
    field["components"] = [field["name"]]
    field["flags"] = ["no-set"] + relation.get("flags", [])
    if "page_size" in relation:
        field["page_size"] = relation["page_size"]
    if "schema" in relation:
        field["schema"] = relation["schema"]

    parent["fields"].append(field)
    if relation["parent"] != relation["child"]:
        parent["cpp_includes"].extend([f"{relation['child']}.h", "odb/dbSet.h"])
    logging.debug(f"Add relation field {field['name']} to {relation['parent']}")
    return field


def make_parent_hash_field(parent, relation, parent_field):
    """Adds a hash table field to the parent of a hashed relationsip"""
    field = {}
    field["name"] = parent_field["name"][:-4] + "hash_"
    page_size_part = f", {relation['page_size']}" if "page_size" in relation else ""
    field["type"] = f"dbHashTable<_{relation['child']}{page_size_part}>"
    field["components"] = [field["name"]]
    field["table_name"] = parent_field["name"]
    field["flags"] = ["no-set"]
    field["flags"].extend(relation.get("flags", []))
    parent["fields"].append(field)
    if "dbHashTable.h" not in parent["h_includes"]:
        parent["h_includes"].append("dbHashTable.h")
    logging.debug(f"Add hash field {field['name']} to {relation['parent']}")
    return field


def make_child_next_field(child, relation):
    """Adds a next entry field to the child of a hashed relationsip"""
    inChildNextEntry = {"name": "next_entry_"}
    inChildNextEntry["type"] = "dbId<_" + relation["child"] + ">"
    inChildNextEntry["flags"] = ["private"]
    inChildNextEntry["flags"].extend(relation.get("flags", []))
    child["fields"].append(inChildNextEntry)
    logging.debug(f"Add hash field {inChildNextEntry['name']} to {relation['child']}")


def add_field_attributes(field, klass, flags_struct, schema):
    """Adds various derived attributes to a field"""
    flag_num_bits = 0
    if field["type"] == "bit":
        field["type"] = "bool"
        field["bits"] = 1
    if "bits" in field:
        flags_struct["fields"].append(field)
        flag_num_bits = int(field["bits"])
    field["bitFields"] = is_bit_fields(field, klass["structs"])

    # Handle types from <cstdint> that are also in the global namespace (eg uint32_t)
    hdr = std_type_hdr.get(field["type"])
    if hdr:
        klass["h_sys_includes"].append(hdr)
        klass["cpp_sys_includes"].append(hdr)

    if field["type"].startswith("std::"):
        for type in re.findall(r"std::(\w+)", field["type"]):
            hdr = std_type_hdr.get(type, type)
            klass["h_sys_includes"].append(hdr)
            klass["cpp_sys_includes"].append(hdr)

    field["isRef"] = is_ref(field["type"]) if field.get("parent") is not None else False
    field["refType"] = get_ref_type(field["type"])
    # refTable is the table name from which the getter extracts the pointer to dbObject
    if field["isRef"]:
        field["refTable"] = get_table_name(field["refType"].replace("*", ""))
        # checking if there is a defined relation between parent and refType for extracting table name
        for relation in schema["relations"]:
            if relation["parent"] == field["parent"] and relation["child"] == field[
                "refType"
            ].replace("*", ""):
                field["refTable"] = relation["tbl_name"]
    field["isHashTable"] = is_hash_table(field["type"])
    field["hashTableType"] = get_hash_table_type(field["type"])
    field["isPassByRef"] = is_pass_by_ref(field["type"])
    field.setdefault("flags", [])
    field["isSetByRef"] = "set-const-ref" in field["flags"] or is_set_by_ref(
        field["type"]
    )
    if "argument" not in field:
        field["argument"] = field["name"].strip("_")
    if "private" in field["flags"]:
        field["flags"].append("no-set")
        field["flags"].append("no-get")

    # Check if a class is being used inside a template definition to add
    # to the list of forward declared classes
    #
    # This needs documentation
    #
    if is_template_type(field["type"]):
        for template_class_name in get_template_types(field["type"]):
            if (
                template_class_name not in klass["declared_classes"]
                and template_class_name not in std
                and not template_class_name.isdigit()
                and template_class_name != "false"
                and template_class_name != "true"
                and "no-template" not in field["flags"]
                and klass["name"] != template_class_name[1:]
                and klass["name"] + "::"
                != template_class_name[0 : len(klass["name"]) + 2]
            ):
                klass["declared_classes"].append(template_class_name)
    ####
    ####
    ####
    if field.get("table", False):
        klass["hasTables"] = True
        if field["type"].startswith("db"):
            field["functional_name"] = get_plural_name(field["type"][2:])
        else:
            field["functional_name"] = get_plural_name(field["type"])
        field["components"] = [field["name"]]
    elif field["isHashTable"]:
        field["functional_name"] = get_plural_name(field["type"][2:])
    else:
        field["functional_name"] = get_functional_name(field["name"])
        field["components"] = components(klass["structs"], field["name"], field["type"])
    field.setdefault("setterFunctionName", "set" + field["functional_name"])
    field.setdefault(
        "getterFunctionName",
        ("is" if field["type"] == "bool" or field.get("bits") == 1 else "get")
        + field["functional_name"],
    )

    if field["isRef"]:
        field["setterArgumentType"] = field["getterReturnType"] = field["refType"]
    elif field["isHashTable"]:
        if "no-set" not in field["flags"]:
            field.append("no-set")
        field["setterArgumentType"] = field["getterReturnType"] = field[
            "hashTableType"
        ].replace("_", "")
        field["getterFunctionName"] = "find" + field["setterArgumentType"][2:-1]
    elif "bits" in field and field["bits"] == 1:
        field["setterArgumentType"] = field["getterReturnType"] = "bool"
    elif field["isPassByRef"]:
        field["setterArgumentType"] = field["getterReturnType"] = field["type"].replace(
            "dbVector", "std::vector"
        )
    elif field["type"] == "char *":
        field["setterArgumentType"] = field["type"]
        field["getterReturnType"] = "const char *"
    else:
        field["setterArgumentType"] = field["getterReturnType"] = field["type"]

    # For fields that we need to free/destroy in the destructor
    needs_free = (
        field["name"] == "name_"
        and field["type"] == "char *"
        and "no-destruct" not in field["flags"]
    )

    if needs_free or "table" in field:
        klass["needs_non_default_destructor"] = True
        if needs_free:
            klass["cpp_sys_includes"].extend(["cstdlib"])
    return flag_num_bits


def add_bitfield_flags(klass, flag_num_bits, flags_struct):
    """Create a flags field for all the bitfields in a class"""
    klass["fields"] = [field for field in klass["fields"] if "bits" not in field]

    klass["hasBitFields"] = False
    if flag_num_bits > 0:
        klass["hasBitFields"] = True

    total_num_bits = flag_num_bits
    if flag_num_bits > 0 and flag_num_bits % 32 != 0:
        spare_bits_field = {
            "name": "spare_bits",
            "type": "uint32_t",
            "bits": 32 - (flag_num_bits % 32),
            "flags": ["no-cmp", "no-set", "no-get", "no-serial"],
        }
        total_num_bits += spare_bits_field["bits"]
        flags_struct["fields"].append(spare_bits_field)

    if len(flags_struct["fields"]) > 0:
        flags_struct["in_class"] = True
        flags_struct["in_class_name"] = "flags_"
        klass["structs"].insert(0, flags_struct)
        klass["fields"].insert(
            0,
            {
                "name": "flags_",
                "type": flags_struct["name"],
                "components": components(
                    klass["structs"], "flags_", flags_struct["name"]
                ),
                "bitFields": True,
                "numBits": total_num_bits,
                "flags": ["no-cmp", "no-set", "no-get", "no-serial"],
            },
        )


def generate_relations(schema):
    """Generate the parent and child fields for all relationships"""
    for relation in schema["relations"]:
        if relation["type"] != "1_n":
            raise KeyError("relation type is not supported, use 1_n")
        parent = get_class(schema, relation["parent"])
        child = get_class(schema, relation["child"])

        parent_field = make_parent_field(parent, relation)

        child_type_name = f"_{relation['child']}"

        if child_type_name not in parent["declared_classes"]:
            parent["declared_classes"].append(child_type_name)

        if relation.get("hash", False):
            make_parent_hash_field(parent, relation, parent_field)
            make_child_next_field(child, relation)


def add_include(klass, key, include, cpp=False, sys=False):
    for field in klass["fields"]:
        if key in field["type"]:
            if sys:
                klass["h_sys_includes"].insert(0, include)
            else:
                klass["h_includes"].insert(0, include)

            if cpp:
                if sys:
                    klass["cpp_sys_includes"].insert(0, include)
                else:
                    klass["cpp_includes"].insert(0, include)
            break


def preprocess_klass(klass):
    klass["declared_classes"].insert(0, "dbIStream")
    klass["declared_classes"].insert(1, "dbOStream")
    if klass["name"] != "dbDatabase":
        klass["declared_classes"].insert(2, "_dbDatabase")
        klass["cpp_includes"].append("dbDatabase.h")
    klass["h_includes"].insert(0, "dbCore.h")
    klass["h_sys_includes"].insert(1, "cstdint")
    name = klass["name"]
    klass["cpp_includes"].extend(["dbTable.h", "dbTable.hpp", "odb/db.h", f"{name}.h"])
    if klass["hasBitFields"]:
        klass["cpp_sys_includes"].extend(["cstdint", "cstring"])
    add_include(klass, "dbObject", "dbCore.h", cpp=True)
    add_include(klass, "dbId<", "odb/dbId.h")
    add_include(klass, "dbVector", "dbVector.h")
    add_include(klass, "dbObjectType", "odb/dbObject.h", cpp=True)
    add_include(klass, "tuple", "tuple", cpp=True, sys=True)
    for field in klass["fields"]:
        if field.get("table", False):
            page_size_part = f", {field['page_size']}" if "page_size" in field else ""
            # setting default value for table fields
            this_or_db = "this" if klass["name"] == "dbDatabase" else "db"
            field["default"] = (
                f"new dbTable<_{field['type']}{page_size_part}>({this_or_db}, this, (GetObjTbl_t) &_{klass['name']}::getObjectTable, {field['type']}Obj)"
            )
            # setting table identifier for table fields
            field["table_base_type"] = field["type"]
            field["type"] = f"dbTable<_{field['type']}{page_size_part}>*"


def generate(schema, env, includeDir, srcDir, keep_empty):
    """Generate generate code based on the schema and templates"""
    print("###################Code Generation Begin###################")
    add_once_to_dict(["classes", "iterators", "relations"], schema)

    for file_path in get_json_files(schema["classes_dir"]):
        with open(file_path, encoding="ascii") as file:
            klass = json.load(file)
        schema["classes"].append(klass)

    for i, klass in enumerate(schema["classes"]):
        add_once_to_dict(
            [
                "declared_classes",
                "fields",
                "enums",
                "structs",
                "h_includes",
                "h_sys_includes",
                "cpp_includes",
                "cpp_sys_includes",
            ],
            klass,
        )
        klass.setdefault("type", "dbObject")
        schema["classes"][i] = klass

    generate_relations(schema)

    to_be_merged = []
    hash_dict = {}
    for klass in schema["classes"]:
        # Adding functional name to fields and extracting field components
        flags_struct = {
            "name": f"{klass['name']}Flags",
            "fields": [],
            "flags": ["no-serializer"],
        }
        klass["hasTables"] = False
        flag_num_bits = 0
        for field in klass["fields"]:
            flag_num_bits += add_field_attributes(field, klass, flags_struct, schema)

        add_bitfield_flags(klass, flag_num_bits, flags_struct)

        # Add required header files if they are not already expressed
        for struct in klass["structs"]:
            if "public" in struct and struct["public"]:
                if "odb/db.h" not in klass["h_includes"]:
                    klass["h_includes"].append("odb/db.h")
                break
        # Add hash to class
        if "hash" not in klass:
            hash_value = fnv1a_32(klass["name"])
        else:
            hash_value = int(klass["hash"], 16)

        if hash_value in hash_dict:
            # Collision detected, error out
            raise ValueError(
                f"Collision detected for {klass['name']} with {hash_dict[hash_value]}"
            )
        hash_dict[hash_value] = klass["name"]
        klass["hash"] = f"0x{hash_value:08X}"
        preprocess_klass(klass)
        # Generating files
        for template_file in ["impl.h", "impl.cpp"]:
            template = env.get_template(template_file)
            text = template.render(klass=klass, schema=schema)
            fileType = template_file.split(".")
            out_file = f"{klass['name']}.{template_file.split('.')[1]}"
            to_be_merged.append(out_file)
            out_file = os.path.join("generated", out_file)
            with open(out_file, "w", encoding="ascii") as file:
                file.write(text)

    includes = ["db.h", "dbObject.h", "dbCompare.inc"]
    for template_file in [
        "db.h",
        "dbObject.h",
        "CMakeLists.txt",
        "dbObject.cpp",
        "dbCompare.inc",
    ]:
        template = env.get_template(template_file)
        text = template.render(schema=schema)
        out_file = os.path.join("generated", template_file)
        to_be_merged.append(template_file)
        with open(out_file, "w", encoding="ascii") as file:
            file.write(text)

    # Generating all iterators
    for itr in schema["iterators"]:
        for template_file in ["itr.h", "itr.cpp"]:
            template = env.get_template(template_file)
            text = template.render(itr=itr, schema=schema)
            out_file = f"{itr['name']}.{template_file.split('.')[1]}"
            to_be_merged.append(out_file)
            out_file = os.path.join("generated", out_file)
            with open(out_file, "w", encoding="ascii") as file:
                file.write(text)

    # Merging with existing files
    for item in to_be_merged:
        if item in includes:
            dr = includeDir
        else:
            dr = srcDir
        if os.path.exists(os.path.join(dr, item)):
            p = Parser(os.path.join(dr, item))
            if item == "CMakeLists.txt":
                p.set_comment_str("#")
            p.parse_user_code()
            p.clean_code()
            p.parse_source_code(os.path.join("generated", item))
            p.write_in_file(os.path.join(dr, item), keep_empty)
        else:
            shutil.copy(os.path.join("generated", item), os.path.join(dr, item))
        if item != "CMakeLists.txt":
            cf = ["clang-format", "-i", os.path.join(dr, item)]
            retcode = call(cf)
            if retcode != 0:
                print(f"Failed to format {os.path.join(dr, item)}")
        print("Generated: ", os.path.join(dr, item))

    with open("generated/final.json", "w") as outfile:
        outfile.write(json.dumps(schema, indent=2))
    print("###################Code Generation End###################")


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
    parser.add_argument("--log", action="store", default="INFO")
    parser.add_argument("--keep_generated", action="store_true")
    parser.add_argument("--keep_empty", action="store_true")

    args = parser.parse_args()

    numeric_level = getattr(logging, args.log.upper(), None)
    if not isinstance(numeric_level, int):
        raise ValueError("Invalid log level: %s" % args.log)
    logging.basicConfig(level=numeric_level)

    with open(args.json, encoding="ascii") as file:
        schema = json.load(file)

    env = Environment(loader=FileSystemLoader(args.templates), trim_blocks=True)

    # Creating Directory for generated files
    if os.path.exists("generated"):
        shutil.rmtree("generated")
    os.mkdir("generated")

    generate(schema, env, args.include_dir, args.src_dir, args.keep_empty)

    if not args.keep_generated:
        shutil.rmtree("generated")


if __name__ == "__main__":
    main()
