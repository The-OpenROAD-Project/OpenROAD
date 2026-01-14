# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2021-2025, The OpenROAD Authors

from typing import List, Optional, Dict, Any, Union
import re
from schema_models import Field, Struct, Class, Schema

_comparable = {
    "Point",
    "Point3D",
    "Rect",
    "Polygon",
    "Line",
    "bool",
    "char *",
    "char",
    "char*",
    "double",
    "float",
    "int",
    "int8_t",
    "long double",
    "long long",
    "long",
    "short",
    "int16_t",
    "std::string",
    "string",
    "uint32_t",
    "uint8_t",
}

std = {
    "bool",
    "char *",
    "char",
    "char*",
    "double",
    "float",
    "int",
    "int8_t",
    "long double",
    "long long",
    "long",
    "short",
    "int16_t",
    "std::string",
    "string",
    "uint32_t",
    "uint8_t",
}

_removable = {"const", "static", "unsigned"}


def _stem(s: str) -> str:
    return " ".join(item for item in s.split() if item not in _removable)


def _get_struct(name: str, structs: List[Struct]) -> Optional[Struct]:
    for struct in structs:
        if struct.name == name:
            return struct
    return None


def components(structs: List[Struct], name: str, _type: str) -> List[str]:
    if _stem(_type) in _comparable or is_ref(_type):
        return [name]
    struct = _get_struct(_type.rstrip(" *"), structs)
    if struct is not None:
        ret = []
        is_pointer = "*" in _type
        separator = "->" if is_pointer else "."
        for field in struct.fields:
            target = components(structs, field.name, field.type)
            ret.extend(f"{name}{separator}{elem}" for elem in target)
        return ret
    return []


def add_once_to_dict(src: List[str], target: Union[Dict[str, Any], Class]) -> None:
    for obj in src:
        if isinstance(target, dict):
            target.setdefault(obj, [])
        else:
            if not getattr(target, obj, None):
                setattr(target, obj, [])


def is_bit_fields(field: Field, structs: List[Struct]) -> bool:
    if field.bits:
        return True
    struct = _get_struct(field.type, structs)
    if struct is None:
        return False
    return any(is_bit_fields(struct_field, structs) for struct_field in struct.fields)


def get_plural_name(name: str) -> str:
    if name.endswith("y"):
        return f"{name[:-1]}ies"
    if name.endswith("s"):
        return f"{name}es"
    return f"{name}s"


def get_functional_name(name: str) -> str:
    if name.islower():
        return "".join(n.capitalize() for n in name.replace("tbl", "table").split("_"))
    return name


def get_class(schema: Schema, name: str) -> Class:
    for klass in schema.classes:
        if klass.name == name:
            return klass
    raise NameError(f"Class {name} in relations is not found")


def get_table_name(name: str) -> str:
    if name.startswith("db"):
        name = name[2:]
    return f"{name.lower()}_tbl_"


def is_ref(type_name: str) -> bool:
    return type_name.startswith("dbId<") and type_name.endswith(">")


def is_hash_table(type_name: str) -> bool:
    return type_name.startswith("dbHashTable<") and type_name.endswith(">")


def get_hash_table_type(type_name: str) -> Optional[str]:
    if not is_hash_table(type_name) or len(type_name) < 13:
        return None
    types = get_template_types(type_name[12:-1])
    for t in types:
        if "db" in t:
            return f"{t}*"
    return None


def is_pass_by_ref(type_name: str) -> bool:
    return type_name.startswith(("dbVector", "std::vector"))


def is_set_by_ref(type_name: str) -> bool:
    return (
        type_name == "std::string"
        or type_name.startswith("std::pair")
        or type_name.startswith("std::vector")
    )


def is_template_type(type_name: str) -> bool:
    open_bracket = type_name.find("<")
    if open_bracket == -1:
        return False
    closed_bracket = type_name.find(">")
    return closed_bracket >= open_bracket


def _is_comma_divided(type_name: str) -> bool:
    return "," in type_name


def get_template_types(type_name: str) -> List[str]:
    if is_template_type(type_name):
        return get_template_types(get_template_type(type_name))
    if _is_comma_divided(type_name):
        types = [t.strip() for t in type_name.split(",")]
        return [t for t in types if not (t.isdigit() or t in {"true", "false"})]
    return [type_name]


def get_template_type(type_name: str) -> Optional[str]:
    if not is_template_type(type_name):
        return None
    num_brackets = 0
    open_bracket = type_name.find("<")
    for i in range(open_bracket, len(type_name)):
        if type_name[i] == "<":
            num_brackets += 1
        elif type_name[i] == ">":
            num_brackets -= 1
            if num_brackets == 0:
                return type_name[open_bracket + 1 : i]
    return None


def get_ref_type(type_name: str) -> Optional[str]:
    if not is_ref(type_name) or len(type_name) < 7:
        return None
    return f"{type_name[6:-1]}*"


def fnv1a_32(string: str) -> int:
    FNV_prime = 0x01000193
    hash_value = 0x811C9DC5
    for c in string:
        hash_value ^= ord(c)
        hash_value = (hash_value * FNV_prime) % (1 << 32)
    return hash_value
