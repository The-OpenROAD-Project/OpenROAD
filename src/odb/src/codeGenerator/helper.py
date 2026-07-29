# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2021-2025, The OpenROAD Authors

from __future__ import annotations

import re

from schema_models import Field, Struct

# Atomic leaf types: in components() these compare as a whole rather than being
# decomposed into struct members. Includes scalars plus std::string and char*.
_LEAF_TYPES = {
    "bool",
    "char *",
    "char",
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
    "uint32_t",
    "uint8_t",
    "int64_t",
}

# Value structs that are comparison leaves in components() but are not scalars.
_VALUE_TYPES = {"Point", "Point3D", "Rect", "Polygon", "Line"}

# Types treated as a single comparison leaf by components() (leaf types + value
# structs); used in helper.py:components().
_comparable = _LEAF_TYPES | _VALUE_TYPES

# C++ fundamental / cstdint scalar types that are cheap to pass by value; used by
# is_set_by_ref(). std::string is intentionally excluded (it is passed by ref).
_BY_VALUE_SCALARS = {
    "bool",
    "char",
    "signed char",
    "unsigned char",
    "short",
    "unsigned short",
    "int",
    "unsigned",
    "unsigned int",
    "long",
    "unsigned long",
    "long long",
    "unsigned long long",
    "float",
    "double",
    "long double",
    "int8_t",
    "int16_t",
    "int32_t",
    "int64_t",
    "uint8_t",
    "uint16_t",
    "uint32_t",
    "size_t",
    "uint64_t",
    "int64_t",
}

_removable = {"const", "static", "unsigned"}


def _stem(s: str) -> str:
    """Strip cv/`unsigned`/`static` qualifiers from a type string."""
    return " ".join(item for item in s.split() if item not in _removable)


def _get_struct(name: str, structs: list[Struct]) -> Struct | None:
    """Return the Struct named `name` from `structs`, or None if absent."""
    for struct in structs:
        if struct.name == name:
            return struct
    return None


def is_enum(type_name: str) -> bool:
    """Whether the type is an odb-style scoped enum ("Foo::Value").

    Such enums are comparison leaves: they support == and < directly and are
    decomposed no further. Other scoped names (e.g. nested structs like
    "dbPowerSwitch::UPFControlPort") are intentionally excluded -- only the
    "::Value" convention used for odb enums is treated as a leaf.
    """
    return type_name.endswith("::Value")


def components(structs: list[Struct], name: str, _type: str) -> list[str]:
    """Expand a field into its comparison sub-components.

    A leaf/ref/enum type yields itself; a struct type recurses into its members;
    anything else yields nothing (not compared).
    """
    if _stem(_type) in _comparable or is_ref(_type) or is_enum(_type):
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


def is_bit_fields(field: Field, structs: list[Struct]) -> bool:
    """Whether the field (or any nested struct member) is a bitfield."""
    if field.bits:
        return True
    struct = _get_struct(field.type, structs)
    if struct is None:
        return False
    return any(is_bit_fields(struct_field, structs) for struct_field in struct.fields)


def get_plural_name(name: str) -> str:
    """Pluralize a name (handles trailing 'y' and 's')."""
    if name.endswith("y"):
        return f"{name[:-1]}ies"
    if name.endswith("s"):
        return f"{name}es"
    return f"{name}s"


def get_functional_name(name: str) -> str:
    """CamelCase accessor stem from a snake_case field name (tbl -> table)."""
    if name.islower():
        return "".join(n.capitalize() for n in name.replace("tbl", "table").split("_"))
    return name


def get_table_name(name: str) -> str:
    """Table member name for a db class (e.g. dbInst -> inst_tbl_)."""
    if name.startswith("db"):
        name = name[2:]
    return f"{name.lower()}_tbl_"


# get_ref_type() turns "dbId<_dbModule>" into the public pointer type "dbModule*"
# by dropping this 6-char prefix (note the trailing "_": the private storage class
# is "_dbModule" but the public type is "dbModule") and the closing ">".
_DBID_PUBLIC_PREFIX = "dbId<_"

# get_hash_table_type() strips this prefix off "dbHashTable<_dbX[, N]>".
_DBHASHTABLE_PREFIX = "dbHashTable<"


def is_ref(type_name: str) -> bool:
    """Whether the type is a dbId<...> object reference."""
    return type_name.startswith("dbId<") and type_name.endswith(">")


def is_hash_table(type_name: str) -> bool:
    """Whether the type is a dbHashTable<...>."""
    return type_name.startswith("dbHashTable<") and type_name.endswith(">")


def get_hash_table_type(type_name: str) -> str | None:
    """Value pointer type of a dbHashTable<_dbX[, N]> (e.g. dbX*), or None."""
    if not is_hash_table(type_name) or len(type_name) < 13:
        return None
    types = get_template_types(type_name[len(_DBHASHTABLE_PREFIX) : -1])
    for t in types:
        if "db" in t:
            return f"{t}*"
    return None


def is_pass_by_ref(type_name: str) -> bool:
    """Whether the type is a dbVector<...> or std::vector<...>."""
    return type_name.startswith(("dbVector", "std::vector"))


def is_set_by_ref(type_name: str, enum_names) -> bool:
    """Whether a setter takes the value by const reference rather than by value.

    Trivially-copyable types are passed by value: fundamental scalars, pointers
    and handles (char*, dbId<>, dbHashTable<>), and enums (scoped "X::Value" or
    registered in enum_names). Everything else -- std::string, std:: containers,
    dbVector, geometry/value structs, any class type -- is passed by const ref.
    """
    if type_name in _BY_VALUE_SCALARS:
        return False
    if type_name.endswith("*") or type_name.startswith(("dbId<", "dbHashTable<")):
        return False
    if type_name in enum_names:
        return False
    if is_enum(type_name):
        return False
    return True


# Heap std:: container prefixes whose value getter returns a const reference.
# std::vector is excluded (it uses an out-parameter getter); std::array/pair are
# trivially copyable when their elements are, so they are returned by value.
_GET_BY_REF_PREFIXES = (
    "std::map",
    "std::multimap",
    "std::unordered_map",
    "std::set",
    "std::multiset",
    "std::unordered_set",
    "std::list",
    "std::deque",
)


def is_get_by_ref(type_name: str) -> bool:
    """Whether a value getter returns a const reference rather than by value.

    True for non-trivially-copyable value types: std::string, heap std::
    containers, and Polygon (an odb geometry type holding a std::vector). Small
    value structs (Point/Rect/Line/...) and trivially-copyable scalars/enums are
    returned by value.
    """
    if type_name in ("std::string", "Polygon"):
        return True
    return type_name.startswith(_GET_BY_REF_PREFIXES)


# Type prefixes for which MemInfo::add() has an overload that accounts the
# member's heap allocation in collectMemInfo(). Scalars, dbId, std::pair,
# std::array, std::tuple, and the value structs have no overload (no heap to
# account) and are skipped.
_MEM_INFO_PREFIXES = (
    "dbVector",
    "std::vector",
    "dbHashTable",
    "dbIntHashTable",
    "std::map",
    "std::unordered_map",
    "boost::container::flat_map",
    "std::set",
)


def mem_info_accountable(type_name: str) -> bool:
    """Whether collectMemInfo() accounts this member via a single add() call.

    True for the heap-backed types MemInfo::add() overloads accept (std::string,
    vectors, hash tables, maps, sets). Members whose elements themselves own heap
    storage (e.g. a vector of strings, or a map of matrices) need element-level
    accounting and opt out with the 'no-meminfo' flag, keeping that logic in the
    collectMemInfo User Code block.
    """
    if type_name == "std::string":
        return True
    return type_name.startswith(_MEM_INFO_PREFIXES)


def is_template_type(type_name: str) -> bool:
    """Whether the type has a <...> template argument list."""
    open_bracket = type_name.find("<")
    if open_bracket == -1:
        return False
    closed_bracket = type_name.find(">")
    return closed_bracket >= open_bracket


def _split_top_level_commas(type_name: str) -> list[str]:
    """Split by commas at angle-bracket depth 0."""
    parts = []
    depth = 0
    start = 0
    for i, ch in enumerate(type_name):
        if ch == "<":
            depth += 1
        elif ch == ">":
            depth -= 1
        elif ch == "," and depth == 0:
            parts.append(type_name[start:i].strip())
            start = i + 1
    parts.append(type_name[start:].strip())
    return [p for p in parts if p]


def get_template_types(type_name: str) -> list[str]:
    """Flatten a type into its constituent template argument type names."""
    # Check for top-level commas first (bracket-aware)
    parts = _split_top_level_commas(type_name)
    if len(parts) > 1:
        result = []
        for part in parts:
            if part.isdigit() or part in {"true", "false"}:
                continue
            result.extend(get_template_types(part))
        return result
    # Single type — if it is a template, peel the outer layer
    if is_template_type(type_name):
        return get_template_types(get_template_type(type_name))
    return [type_name]


def get_template_type(type_name: str) -> str | None:
    """The substring inside the outer <...> of a template type, or None."""
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


def get_ref_type(type_name: str) -> str | None:
    """Public pointer type for a dbId<_dbX> reference (e.g. dbX*), or None."""
    if not is_ref(type_name) or len(type_name) < 7:
        return None
    return f"{type_name[len(_DBID_PUBLIC_PREFIX) : -1]}*"


def fnv1a_32(string: str) -> int:
    """FNV-1a 32-bit hash of a string (used for stable dbObjectType hashes)."""
    FNV_prime = 0x01000193
    hash_value = 0x811C9DC5
    for c in string:
        hash_value ^= ord(c)
        hash_value = (hash_value * FNV_prime) % (1 << 32)
    return hash_value
