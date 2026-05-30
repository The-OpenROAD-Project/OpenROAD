# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2021-2025, The OpenROAD Authors

from dataclasses import dataclass, field as dataclass_field
from typing import Any, Dict, List, Optional


def _require_known_keys(cls, data: Dict[str, Any]) -> Dict[str, Any]:
    """Reject schema keys the dataclass does not declare (catches JSON typos).

    Must be called after any key remapping (e.g. Class 'type'->'base_class').
    """
    unknown = set(data) - set(cls.__dataclass_fields__)
    if unknown:
        raise ValueError(f"{cls.__name__}: unknown schema key(s): {sorted(unknown)}")
    return data


@dataclass
class Field:
    name: str = ""
    type: str = ""
    flags: List[str] = dataclass_field(default_factory=list)
    bits: Optional[int] = None
    default: Optional[Any] = None
    comment: Optional[str] = None
    public_comment: Optional[str] = None
    parent: Optional[str] = None
    argument: Optional[str] = None
    functional_name: Optional[str] = None
    setterFunctionName: Optional[str] = None
    getterFunctionName: Optional[str] = None
    bitFields: bool = False
    table: bool = False
    dbSetGetter: bool = False
    table_name: Optional[str] = None
    page_size: Optional[int] = None
    schema: Optional[str] = None
    components: List[str] = dataclass_field(default_factory=list)
    numBits: Optional[int] = None
    # Set during processing to a field_types.FieldType; not read from JSON. It is
    # the single source of truth for the storage-kind properties below.
    kind: Optional[Any] = None

    # The following are derived from `kind`, which is the only place that classifies
    # a C++ type. They are properties so the templates can read them by name.
    @property
    def refType(self) -> Optional[str]:
        return self.kind.ref_type if self.kind else None

    @property
    def refTable(self) -> Optional[str]:
        return self.kind.ref_table if self.kind else None

    @property
    def isHashTable(self) -> bool:
        return self.kind.is_hash_table if self.kind else False

    @property
    def hashTableType(self) -> Optional[str]:
        return self.kind.hash_table_type if self.kind else None

    @property
    def isSetByRef(self) -> bool:
        return self.kind.type_set_by_ref if self.kind else False

    @property
    def setterArgumentType(self) -> Optional[str]:
        return self.kind.setter_arg_type if self.kind else None

    @property
    def getterReturnType(self) -> Optional[str]:
        return self.kind.getter_return_type if self.kind else None

    @property
    def table_base_type(self) -> Optional[str]:
        return self.kind.table_base_type if self.kind else None

    @property
    def member_decl(self) -> str:
        """The C++ type for the stored member declaration."""
        return self.kind.member_decl() if self.kind else self.type

    @classmethod
    def from_dict(cls, data: Dict[str, Any]) -> "Field":
        _require_known_keys(cls, data)
        return cls(**data)


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
        _require_known_keys(cls, data)
        return cls(**data)


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
        _require_known_keys(cls, data)
        return cls(**data)


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
        _require_known_keys(cls, data)
        return cls(**data)


@dataclass
class Iterator:
    name: str
    parentObject: str
    tableName: str
    reversible: bool = False
    orderReversed: bool = False
    customEnd: bool = False
    sequential: int = 0
    tablePageSize: Optional[int] = None
    includes: List[str] = dataclass_field(default_factory=list)
    flags: List[str] = dataclass_field(default_factory=list)

    @classmethod
    def from_dict(cls, data: Dict[str, Any]) -> "Iterator":
        data = dict(data)
        # The schema spells these as the JSON strings "true"/"false".
        for key in ("reversible", "orderReversed", "customEnd"):
            if isinstance(data.get(key), str):
                data[key] = data[key].lower() == "true"
        _require_known_keys(cls, data)
        return cls(**data)


@dataclass
class Relation:
    parent: str
    child: str
    type: str
    tbl_name: str
    hash: bool = False
    page_size: Optional[int] = None
    schema: Optional[str] = None
    flags: List[str] = dataclass_field(default_factory=list)

    @classmethod
    def from_dict(cls, data: Dict[str, Any]) -> "Relation":
        _require_known_keys(cls, data)
        return cls(**data)


@dataclass
class Schema:
    classes_dir: str
    classes: List[Class] = dataclass_field(default_factory=list)
    iterators: List[Iterator] = dataclass_field(default_factory=list)
    relations: List[Relation] = dataclass_field(default_factory=list)
    # Names of enum types defined outside the generator (e.g. dbGDSSTrans). They
    # are passed by value like other enums; see is_set_by_ref().
    extern_enums: List[str] = dataclass_field(default_factory=list)
    # Derived during processing: all enum type names (declared + external).
    enum_type_names: set = dataclass_field(default_factory=set)

    @classmethod
    def from_dict(cls, data: Dict[str, Any]) -> "Schema":
        data = dict(data)
        data["iterators"] = [
            Iterator.from_dict(i) if isinstance(i, dict) else i
            for i in data.get("iterators", [])
        ]
        data["relations"] = [
            Relation.from_dict(r) if isinstance(r, dict) else r
            for r in data.get("relations", [])
        ]
        _require_known_keys(cls, data)
        return cls(**data)
