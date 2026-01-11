# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2021-2025, The OpenROAD Authors

from dataclasses import dataclass, field as dataclass_field
from typing import Any, Dict, List, Optional


@dataclass
class Field:
    name: str = ""
    type: str = ""
    flags: List[str] = dataclass_field(default_factory=list)
    bits: Optional[int] = None
    default: Optional[Any] = None
    comment: Optional[str] = None
    parent: Optional[str] = None
    argument: Optional[str] = None
    functional_name: Optional[str] = None
    setterFunctionName: Optional[str] = None
    getterFunctionName: Optional[str] = None
    setterArgumentType: Optional[str] = None
    getterReturnType: Optional[str] = None
    isRef: bool = False
    refType: Optional[str] = None
    refTable: Optional[str] = None
    isHashTable: bool = False
    hashTableType: Optional[str] = None
    isPassByRef: bool = False
    isSetByRef: bool = False
    bitFields: bool = False
    table: bool = False
    dbSetGetter: bool = False
    table_base_type: Optional[str] = None
    table_name: Optional[str] = None
    page_size: Optional[int] = None
    schema: Optional[str] = None
    components: List[str] = dataclass_field(default_factory=list)
    numBits: Optional[int] = None

    @classmethod
    def from_dict(cls, data: Dict[str, Any]) -> "Field":
        return cls(**{k: v for k, v in data.items() if k in cls.__dataclass_fields__})


@dataclass
class Enum:
    name: str
    values: List[str]
    public: bool = False
    cls: bool = dataclass_field(default=False)

    @classmethod
    def from_dict(cls, data: Dict[str, Any]) -> "Enum":
        if "class" in data:
            data["cls"] = data.pop("class")
        return cls(**{k: v for k, v in data.items() if k in cls.__dataclass_fields__})


@dataclass
class Struct:
    name: str
    fields: List[Field] = dataclass_field(default_factory=list)
    public: bool = False
    flags: List[str] = dataclass_field(default_factory=list)
    in_class_name: Optional[str] = None

    @classmethod
    def from_dict(cls, data: Dict[str, Any]) -> "Struct":
        fields = [
            Field.from_dict(f) if isinstance(f, dict) else f
            for f in data.get("fields", [])
        ]
        data["fields"] = fields
        return cls(**{k: v for k, v in data.items() if k in cls.__dataclass_fields__})


@dataclass
class Class:
    name: str
    base_class: str = "dbObject"
    fields: List[Field] = dataclass_field(default_factory=list)
    enums: List[Enum] = dataclass_field(default_factory=list)
    structs: List[Struct] = dataclass_field(default_factory=list)
    h_includes: List[str] = dataclass_field(default_factory=list)
    h_sys_includes: List[str] = dataclass_field(default_factory=list)
    cpp_includes: List[str] = dataclass_field(default_factory=list)
    cpp_sys_includes: List[str] = dataclass_field(default_factory=list)
    declared_classes: List[str] = dataclass_field(default_factory=list)
    description: List[str] = dataclass_field(default_factory=list)
    do_not_generate_compare: bool = False
    hash: Optional[str] = None
    hasTables: bool = False
    hasBitFields: bool = False
    needs_non_default_destructor: bool = False
    assert_alignment_is_multiple_of: Optional[int] = None
    ostream_scope: bool = False
    equal_fields: List[Dict[str, str]] = dataclass_field(default_factory=list)
    less_fields: List[Dict[str, str]] = dataclass_field(default_factory=list)

    @classmethod
    def from_dict(cls, data: Dict[str, Any]) -> "Class":
        if "do_not_generate_compare" in data and isinstance(
            data["do_not_generate_compare"], str
        ):
            data["do_not_generate_compare"] = (
                data["do_not_generate_compare"].lower() == "true"
            )
        if "type" in data:
            data["base_class"] = data.pop("type")
        data["fields"] = [
            Field.from_dict(f) if isinstance(f, dict) else f
            for f in data.get("fields", [])
        ]
        data["enums"] = [
            Enum.from_dict(e) if isinstance(e, dict) else e
            for e in data.get("enums", [])
        ]
        data["structs"] = [
            Struct.from_dict(s) if isinstance(s, dict) else s
            for s in data.get("structs", [])
        ]
        return cls(**{k: v for k, v in data.items() if k in cls.__dataclass_fields__})


@dataclass
class Schema:
    classes_dir: str
    classes: List[Class] = dataclass_field(default_factory=list)
    iterators: List[Dict[str, Any]] = dataclass_field(default_factory=list)
    relations: List[Dict[str, Any]] = dataclass_field(default_factory=list)

    @classmethod
    def from_dict(cls, data: Dict[str, Any]) -> "Schema":
        return cls(**{k: v for k, v in data.items() if k in cls.__dataclass_fields__})
