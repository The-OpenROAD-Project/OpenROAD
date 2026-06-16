# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2021-2025, The OpenROAD Authors

from __future__ import annotations

from dataclasses import dataclass, field as dataclass_field
from typing import TYPE_CHECKING, Any

if TYPE_CHECKING:
    from field_types import FieldType


def _require_known_keys(cls, data: dict[str, Any]) -> dict[str, Any]:
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
    flags: list[str] = dataclass_field(default_factory=list)
    bits: int | None = None
    default: Any | None = None
    comment: str | None = None
    public_comment: str | None = None
    parent: str | None = None
    argument: str | None = None
    functional_name: str | None = None
    setterFunctionName: str | None = None
    getterFunctionName: str | None = None
    bitFields: bool = False
    table: bool = False
    dbSetGetter: bool = False
    table_name: str | None = None
    page_size: int | None = None
    schema: str | None = None
    components: list[str] = dataclass_field(default_factory=list)
    numBits: int | None = None
    # Set during processing to a FieldType; not read from JSON. It is the single
    # source of truth for the storage-kind properties below.
    kind: FieldType | None = None

    # The following are derived from `kind`, which is the only place that classifies
    # a C++ type. They are properties so the templates can read them by name.
    @property
    def refType(self) -> str | None:
        """Referenced object's pointer type, for a dbId<> field."""
        return self.kind.ref_type if self.kind else None

    @property
    def refTable(self) -> str | None:
        """Owner table a dbId<> getter reads from."""
        return self.kind.ref_table if self.kind else None

    @property
    def isHashTable(self) -> bool:
        """Whether the field is a dbHashTable<>."""
        return self.kind.is_hash_table if self.kind else False

    @property
    def hashTableType(self) -> str | None:
        """Value pointer type of a dbHashTable<> field."""
        return self.kind.hash_table_type if self.kind else None

    @property
    def isSetByRef(self) -> bool:
        """Whether the setter takes its argument by const reference."""
        return self.kind.type_set_by_ref if self.kind else False

    @property
    def setterArgumentType(self) -> str | None:
        """C++ type the generated setter accepts."""
        return self.kind.setter_arg_type if self.kind else None

    @property
    def getterReturnType(self) -> str | None:
        """C++ type the generated getter returns."""
        return self.kind.getter_return_type if self.kind else None

    @property
    def table_base_type(self) -> str | None:
        """Child class name of an owned-table field."""
        return self.kind.table_base_type if self.kind else None

    @property
    def member_decl(self) -> str:
        """The C++ type for the stored member declaration."""
        return self.kind.member_decl() if self.kind else self.type

    @classmethod
    def from_dict(cls, data: dict[str, Any]) -> Field:
        """Build a Field from its JSON object, rejecting unknown keys."""
        _require_known_keys(cls, data)
        return cls(**data)


@dataclass
class Enum:
    name: str
    values: list[str]
    public: bool = False
    cls: bool = dataclass_field(default=False)

    @classmethod
    def from_dict(cls, data: dict[str, Any]) -> Enum:
        """Build an Enum from JSON, mapping the reserved 'class' key to `cls`."""
        if "class" in data:
            data["cls"] = data.pop("class")
        _require_known_keys(cls, data)
        return cls(**data)


@dataclass
class Struct:
    name: str
    fields: list[Field] = dataclass_field(default_factory=list)
    public: bool = False
    flags: list[str] = dataclass_field(default_factory=list)
    in_class_name: str | None = None

    @classmethod
    def from_dict(cls, data: dict[str, Any]) -> Struct:
        """Build a Struct from JSON, parsing its nested field list."""
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
    fields: list[Field] = dataclass_field(default_factory=list)
    enums: list[Enum] = dataclass_field(default_factory=list)
    structs: list[Struct] = dataclass_field(default_factory=list)
    h_includes: list[str] = dataclass_field(default_factory=list)
    h_sys_includes: list[str] = dataclass_field(default_factory=list)
    cpp_includes: list[str] = dataclass_field(default_factory=list)
    cpp_sys_includes: list[str] = dataclass_field(default_factory=list)
    declared_classes: list[str] = dataclass_field(default_factory=list)
    description: list[str] = dataclass_field(default_factory=list)
    do_not_generate_compare: bool = False
    hash: str | None = None
    hasTables: bool = False
    hasBitFields: bool = False
    needs_non_default_destructor: bool = False
    assert_alignment_is_multiple_of: int | None = None
    ostream_scope: bool = False
    equal_fields: list[dict[str, str]] = dataclass_field(default_factory=list)
    less_fields: list[dict[str, str]] = dataclass_field(default_factory=list)
    # Generated factory methods, populated from an opted-in relation (see
    # generate_relations): the owning parent class and its table member, plus
    # whether to emit create()/destroy().
    factory_parent: str | None = None
    factory_table: str | None = None
    gen_create: bool = False
    gen_destroy: bool = False

    @classmethod
    def from_dict(cls, data: dict[str, Any]) -> Class:
        """Build a Class from JSON: the 'type' alias, do_not_generate_compare
        coercion, and the nested field/enum/struct lists."""
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
    tablePageSize: int | None = None
    includes: list[str] = dataclass_field(default_factory=list)
    flags: list[str] = dataclass_field(default_factory=list)

    @classmethod
    def from_dict(cls, data: dict[str, Any]) -> Iterator:
        """Build an Iterator from JSON, coercing the 'true'/'false' string
        flags to bool."""
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
    page_size: int | None = None
    schema: str | None = None
    flags: list[str] = dataclass_field(default_factory=list)
    # Opt in to generating the child's trivial factory methods. 'create' emits
    # Child::create(Parent*) and 'destroy' emits Child::destroy(Child*) using the
    # canonical owned-table pattern; set only when the hand-written versions do
    # nothing more than allocate/free the table entry (see generate_relations).
    create: bool = False
    destroy: bool = False

    @classmethod
    def from_dict(cls, data: dict[str, Any]) -> Relation:
        """Build a Relation from its JSON object, rejecting unknown keys."""
        _require_known_keys(cls, data)
        return cls(**data)


@dataclass
class Schema:
    classes_dir: str
    classes: list[Class] = dataclass_field(default_factory=list)
    iterators: list[Iterator] = dataclass_field(default_factory=list)
    relations: list[Relation] = dataclass_field(default_factory=list)
    # Names of enum types defined outside the generator (e.g. dbGDSSTrans). They
    # are passed by value like other enums; see is_set_by_ref().
    extern_enums: list[str] = dataclass_field(default_factory=list)
    # Derived during processing: all enum type names (declared + external).
    enum_type_names: set = dataclass_field(default_factory=set)

    @classmethod
    def from_dict(cls, data: dict[str, Any]) -> Schema:
        """Build a Schema from JSON, parsing the iterator and relation lists."""
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
