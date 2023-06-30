#!/usr/bin/python3

from subprocess import call
import argparse
import os
import shutil
import json
from parser import Parser
from jinja2 import Environment, FileSystemLoader
from helper import (
    add_once_to_dict,
    components,
    get_class_index,
    get_functional_name,
    get_hash_table_type,
    get_ref_type,
    get_struct,
    get_table_name,
    get_template_type,
    is_bit_fields,
    is_hash_table,
    is_pass_by_ref,
    is_ref,
    std,
)

parser = argparse.ArgumentParser(description="Code generator")
parser.add_argument("--json", action="store", required=True)
parser.add_argument("--src_dir", action="store", required=True)
parser.add_argument("--include_dir", action="store", required=True)
parser.add_argument("--impl", action="store", required=True)

args = parser.parse_args()

src = args.json
srcDir = args.src_dir
includeDir = args.include_dir
impl = args.impl

with open(src, encoding="ascii") as file:
    schema = json.load(file)
    file.close()

env = Environment(loader=FileSystemLoader(impl), trim_blocks=True)

# Creating Directory for generated files

if os.path.exists("generated"):
    shutil.rmtree("generated")
os.mkdir("generated")

toBeMerged = []


print("###################Code Generation Begin###################")
add_once_to_dict(["classes", "iterators", "relations"], schema)

for i, klass in enumerate(schema["classes"]):
    if "src" in klass:
        with open(klass["src"], encoding="ascii") as file:
            klass = json.load(file)
    add_once_to_dict(
        [
            "classes",
            "fields",
            "enums",
            "structs",
            "h_includes",
            "cpp_includes",
        ],
        klass,
    )
    schema["classes"][i] = klass

for relation in schema["relations"]:
    if relation["type"] == "n_1":
        relation["first"], relation["second"] = (
            relation["second"],
            relation["first"],
        )
        relation["type"] = "1_n"
    if relation["type"] != "1_n":
        raise KeyError(
            'relation type is not supported, " \
        "use either 1_n or n_1'
        )
    parent = get_class_index(schema, relation["first"])
    child = get_class_index(schema, relation["second"])
    if parent == -1:
        raise NameError(f"Class {relation['first']} in relations is not found")
    if child == -1:
        raise NameError(f"Class {relation['second']} in relations is not found")
    inParentField = {}
    if "tbl_name" in relation:
        inParentField["name"] = relation["tbl_name"]
    else:
        inParentField["name"] = relation["tbl_name"] = get_table_name(
            relation["second"]
        )
    inParentField["type"] = relation["second"]
    inParentField["table"] = True
    inParentField["dbSetGetter"] = True
    inParentField["components"] = [inParentField["name"]]
    inParentField["flags"] = ["cmp", "serial", "diff", "no-set", "get"]
    if "schema" in relation:
        inParentField["schema"] = relation["schema"]

    schema["classes"][parent]["fields"].append(inParentField)
    schema["classes"][parent]["cpp_includes"].extend(
        [f"{relation['second']}.h", "dbSet.h"]
    )

    child_type_name = f"_{relation['second']}"

    if child_type_name not in schema["classes"][parent]["classes"]:
        schema["classes"][parent]["classes"].append(child_type_name)

    if "dbTable" not in schema["classes"][parent]["classes"]:
        schema["classes"][parent]["classes"].append("dbTable")
    if relation.get("hash", False):
        inParentHashField = {}

        inParentHashField["name"] = inParentField["name"][:-4] + "hash_"
        inParentHashField["type"] = "dbHashTable<_" + relation["second"] + ">"
        inParentHashField["components"] = [inParentHashField["name"]]
        inParentHashField["table_name"] = inParentField["name"]
        inParentHashField["flags"] = ["cmp", "serial", "diff", "no-set", "get"]
        schema["classes"][parent]["fields"].append(inParentHashField)
        if "dbHashTable.h" not in schema["classes"][parent]["h_includes"]:
            schema["classes"][parent]["h_includes"].append("dbHashTable.h")
        inChildNextEntry = {"name": "_next_entry"}
        inChildNextEntry["type"] = "dbId<_" + relation["second"] + ">"
        inChildNextEntry["flags"] = ["cmp", "serial", "diff", "private", "no-deep"]
        schema["classes"][child]["fields"].append(inChildNextEntry)


for klass in schema["classes"]:

    # Adding functional name to fields and extracting field components
    struct = {"name": f"{klass['name']}Flags", "fields": []}
    klass["hasTables"] = False
    flag_num_bits = 0
    for field in klass["fields"]:
        if field["type"] == "bit":
            field["type"] = "bool"
            field["bits"] = 1
        if "bits" in field:
            struct["fields"].append(field)
            flag_num_bits += int(field["bits"])
        field["bitFields"] = is_bit_fields(field, klass["structs"])
        field["isStruct"] = get_struct(field["type"], klass["structs"]) is not None

        field["isRef"] = (
            is_ref(field["type"]) if field.get("parent") is not None else False
        )
        field["refType"] = get_ref_type(field["type"])
        # refTable is the table name from which the getter extracts the pointer to dbObject
        if field["isRef"]:
            field["refTable"] = get_table_name(field["refType"].replace("*", ""))
            # checking if there is a defined relation between parent and refType for extracting table name
            for relation in schema["relations"]:
                if relation["first"] == field["parent"] and relation["second"] == field[
                    "refType"
                ].replace("*", ""):
                    field["refTable"] = relation["tbl_name"]
        field["isHashTable"] = is_hash_table(field["type"])
        field["hashTableType"] = get_hash_table_type(field["type"])
        field["isPassByRef"] = is_pass_by_ref(field["type"])
        if "argument" not in field:
            field["argument"] = field["name"].strip("_")
        field.setdefault("flags", [])
        if "private" in field["flags"]:
            field["flags"].append("no-set")
            field["flags"].append("no-get")

        # Check if a class is being used inside a template definition to add
        # to the list of forward declared classes
        #
        # This needs documentation
        #
        template_class_name = None
        tmp = get_template_type(field["type"])
        while tmp is not None:
            template_class_name = tmp
            tmp = get_template_type(tmp)

        if template_class_name is not None:
            if (
                template_class_name not in klass["classes"]
                and template_class_name not in std
                and "no-template" not in field["flags"]
                and klass["name"] != template_class_name[1:]
            ):
                klass["classes"].append(template_class_name)
        ####
        ####
        ####
        if field.get("table", False):
            klass["hasTables"] = True
            if field["type"].startswith("db"):
                field["functional_name"] = f"{field['type'][2:]}s"
            else:
                field["functional_name"] = f"{field['type']}s"
            field["components"] = [field["name"]]
        elif field["isHashTable"]:
            field["functional_name"] = f"{field['type'][2:]}s"
        else:
            field["functional_name"] = get_functional_name(field["name"])
            field["components"] = components(
                klass["structs"], field["name"], field["type"]
            )
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
            field["setterArgumentType"] = field["getterReturnType"] = field[
                "type"
            ].replace("dbVector", "std::vector")
        elif field["type"] == "char *":
            field["setterArgumentType"] = field["type"]
            field["getterReturnType"] = "const char *"
        else:
            field["setterArgumentType"] = field["getterReturnType"] = field["type"]

    klass["fields"] = [field for field in klass["fields"] if "bits" not in field]

    klass["fields"] = [field for field in klass["fields"] if "bits" not in field]
    total_num_bits = flag_num_bits
    if flag_num_bits > 0 and flag_num_bits % 32 != 0:
        spare_bits_field = {
            "name": "spare_bits_",
            "type": "uint",
            "bits": 32 - (flag_num_bits % 32),
            "flags": ["no-cmp", "no-set", "no-get", "no-serial", "no-diff"],
        }
        total_num_bits += spare_bits_field["bits"]
        struct["fields"].append(spare_bits_field)

    if len(struct["fields"]) > 0:

        struct["in_class"] = True
        struct["in_class_name"] = "flags_"
        klass["structs"].insert(0, struct)
        klass["fields"].insert(
            0,
            {
                "name": "flags_",
                "type": struct["name"],
                "components": components(klass["structs"], "flags_", struct["name"]),
                "bitFields": True,
                "isStruct": True,
                "numBits": total_num_bits,
                "flags": ["no-cmp", "no-set", "no-get", "no-serial", "no-diff"],
            },
        )

    # Generating files
    for template_file in ["impl.h", "impl.cpp"]:
        template = env.get_template(template_file)
        text = template.render(klass=klass, schema=schema)
        fileType = template_file.split(".")
        # for field in klass['fields']:
        #     if field['isHashTable']:
        #         print(field)
        out_file = f"{klass['name']}.{template_file.split('.')[1]}"
        toBeMerged.append(out_file)
        out_file = os.path.join("generated", out_file)
        with open(out_file, "w", encoding="ascii") as file:
            file.write(text)

includes = ["db.h", "dbObject.h"]
for template_file in ["db.h", "dbObject.h", "CMakeLists.txt", "dbObject.cpp"]:
    template = env.get_template(template_file)
    text = template.render(schema=schema)
    out_file = os.path.join("generated", template_file)
    toBeMerged.append(template_file)
    with open(out_file, "w", encoding="ascii") as file:
        file.write(text)


# Generating all iterators
for itr in schema["iterators"]:
    for template_file in ["itr.h", "itr.cpp"]:
        template = env.get_template(template_file)
        text = template.render(itr=itr, schema=schema)
        out_file = f"{itr['name']}.{template_file.split('.')[1]}"
        toBeMerged.append(out_file)
        out_file = os.path.join("generated", out_file)
        with open(out_file, "w", encoding="ascii") as file:
            file.write(text)


# Merging with existing files
for item in toBeMerged:
    if item in includes:
        dr = includeDir
    else:
        dr = srcDir
    if os.path.exists(os.path.join(dr, item)):
        p = Parser(os.path.join(dr, item))
        if item == "CMakeLists.txt":
            p.set_comment_str("#")
        assert p.parse_user_code()
        assert p.clean_code()
        assert p.parse_source_code(os.path.join("generated", item))
        assert p.write_in_file(os.path.join(dr, item))
    else:
        with open(os.path.join("generated", item), "r", encoding="ascii") as read, open(
            os.path.join(dr, item), "w", encoding="ascii"
        ) as out:
            text = read.read()
            out.write(text)
    if item != "CMakeLists.txt":
        cf = ["clang-format", "-i", os.path.join(dr, item)]
        retcode = call(cf)
        if retcode != 0:
            print(f"Failed to format {os.path.join(dr, item)}")
    print("Generated: ", os.path.join(dr, item))
shutil.rmtree("generated")
print("###################Code Generation End###################")
