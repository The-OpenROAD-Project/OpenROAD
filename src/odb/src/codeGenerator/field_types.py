# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2025, The OpenROAD Authors

"""Typed model of a field's C++ storage kind.

`make_field_type()` classifies a field's C++ type into one of the kinds below,
and the resulting `FieldType` answers how to declare, serialize, compare, copy,
and accessorize the field. This keeps the type classification in one place rather
than re-sniffing the type string at each use site.

Kinds, in classification precedence:
  TableType      owned dbTable<_Child>*  (relation parent side; dbSet<> getter)
  RefType        dbId<_X> object reference (resolved via owner table getPtr)
  HashTableType  dbHashTable<_X>         (find(name) getter)
  BoolType       a width-1 bitfield      (accessor type is bool)
  VectorType     dbVector<>/std::vector<> (out-parameter getter)
  CharPtrType    char* / char *          (deep-copied; getter returns const char*)
  StringType     std::string             (set by const ref)
  PairType       std::pair<>             (set by const ref)
  ScalarType     everything else (PODs, enums, std:: tail)
"""

from __future__ import annotations

from helper import (
    get_hash_table_type,
    get_ref_type,
    get_table_name,
    is_get_by_ref,
    is_hash_table,
    is_pass_by_ref,
    is_ref,
    is_set_by_ref,
    mem_info_accountable,
)


class FieldType:
    """Base kind: a scalar/POD/enum/pass-through C++ type."""

    is_hash_table = False
    is_table = False
    db_set_getter = False

    ref_type: str | None = None
    ref_table: str | None = None
    hash_table_type: str | None = None
    table_base_type: str | None = None

    def __init__(self, raw: str):
        """Construct the kind from a raw C++ type string."""
        self.raw = raw
        # Whether the setter takes the value by const reference. Set by
        # make_field_type() since the rule needs the schema's enum names.
        self.type_set_by_ref = False

    def member_decl(self) -> str:
        """The C++ type as declared for the stored member."""
        return self.raw

    # ---- accessor signatures (setter argument type / getter return type) ----
    @property
    def setter_arg_type(self) -> str:
        """C++ type the setter accepts (the raw type by default)."""
        return self.raw

    @property
    def getter_return_type(self) -> str:
        """C++ type the getter returns: const ref for non-trivially-copyable
        aggregates, otherwise the raw type by value."""
        if is_get_by_ref(self.raw):
            return f"const {self.raw}&"
        return self.raw

    # ---- behavior ----
    def getter_kind(self) -> str:
        """One of: value | out_param | ref_lookup | hash_find | dbset."""
        return "value"

    def setter_assign_rhs(self, arg: str) -> str:
        """The right-hand side stored by the setter, given the argument name."""
        return arg

    def serialize_deref(self) -> bool:
        """Whether the stream operators dereference the member."""
        return False

    def compare_deref(self) -> bool:
        """Whether operator==/operator< dereference the member."""
        return False

    def needs_free(self, field_name: str) -> bool:
        """Whether the named member must be freed in the destructor."""
        return False

    def mem_info_account(self) -> bool:
        """Whether collectMemInfo() accounts this member via a single add()."""
        return mem_info_accountable(self.raw)


class ScalarType(FieldType):
    pass


class StringType(ScalarType):
    """std::string — same accessor types as a scalar, but set by const ref."""


class PairType(ScalarType):
    """std::pair<...> — same accessor types as a scalar, but set by const ref."""


class BoolType(FieldType):
    """A width-1 bitfield; its accessor type is bool."""

    @property
    def setter_arg_type(self) -> str:
        """Bitfields are set as bool."""
        return "bool"

    @property
    def getter_return_type(self) -> str:
        """Bitfields are read as bool."""
        return "bool"


class CharPtrType(FieldType):
    """A char pointer (char* / char *): deep-copied, getter returns const char*."""

    @property
    def setter_arg_type(self) -> str:
        """Setter takes a (non-const) char*."""
        return "char *"

    @property
    def getter_return_type(self) -> str:
        """Getter returns a const char*."""
        return "const char *"

    def needs_free(self, field_name: str) -> bool:
        """Only name_ owns its char* storage and is freed."""
        return field_name == "name_"


class VectorType(FieldType):
    """dbVector<...> / std::vector<...> — getter is an out-parameter."""

    @property
    def setter_arg_type(self) -> str:
        """Exposed as std::vector (dbVector is an internal alias)."""
        return self.raw.replace("dbVector", "std::vector")

    @property
    def getter_return_type(self) -> str:
        """Exposed as std::vector (dbVector is an internal alias)."""
        return self.raw.replace("dbVector", "std::vector")

    def getter_kind(self) -> str:
        """Vector getter fills a std::vector out-parameter."""
        return "out_param"


class RefType(FieldType):
    """dbId<_X> object reference; getter resolves via the owner table."""

    def __init__(self, raw: str, ref_table: str | None):
        """Capture the referenced public pointer type and owner table name."""
        super().__init__(raw)
        self.ref_type = get_ref_type(raw)
        self.ref_table = ref_table

    @property
    def setter_arg_type(self) -> str:
        """Setter takes the referenced object's pointer type."""
        return self.ref_type

    @property
    def getter_return_type(self) -> str:
        """Getter returns the referenced object's pointer type."""
        return self.ref_type

    def getter_kind(self) -> str:
        """Ref getter looks the id up in the owner table."""
        return "ref_lookup"

    def setter_assign_rhs(self, arg: str) -> str:
        """Store the referenced object's OID rather than the pointer."""
        return f"{arg}->getImpl()->getOID()"


class HashTableType(FieldType):
    """dbHashTable<_X>; getter is find(name)."""

    is_hash_table = True

    def __init__(self, raw: str):
        """Capture the hashed value's pointer type from the dbHashTable<...>."""
        super().__init__(raw)
        self.hash_table_type = get_hash_table_type(raw)

    def _value_type(self) -> str:
        """The hashed value's public pointer type (e.g. dbX*)."""
        return self.hash_table_type.replace("_", "") if self.hash_table_type else ""

    @property
    def setter_arg_type(self) -> str:
        """Value pointer type (hash tables have no generated setter)."""
        return self._value_type()

    @property
    def getter_return_type(self) -> str:
        """find() returns the value pointer type."""
        return self._value_type()

    def getter_kind(self) -> str:
        """Hash-table getter is find(name)."""
        return "hash_find"


class TableType(FieldType):
    """An owned dbTable<_Child>* (the parent side of a 1_n relation)."""

    is_table = True
    db_set_getter = True

    def __init__(self, base_type: str, page_size: int | None):
        """Owned table of `base_type` children with an optional page size."""
        # base_type is the child class name (e.g. "dbInst"); the stored member
        # type is the dbTable<_Child>* form returned by member_decl().
        super().__init__(base_type)
        self.table_base_type = base_type
        self.page_size = page_size

    def member_decl(self) -> str:
        """The stored C++ type: dbTable<_Child[, N]>*."""
        size = f", {self.page_size}" if self.page_size else ""
        return f"dbTable<_{self.table_base_type}{size}>*"

    def default_expr(self, klass_name: str) -> str:
        """The constructor initializer that allocates the owned table."""
        this_or_db = "this" if klass_name == "dbDatabase" else "db"
        # member_decl() without the trailing '*' is the dbTable<...> type.
        return (
            f"new {self.member_decl()[:-1]}({this_or_db}, this, "
            f"(GetObjTbl_t) &_{klass_name}::getObjectTable, {self.table_base_type}Obj)"
        )

    def getter_kind(self) -> str:
        """Owned tables expose a dbSet<> getter."""
        return "dbset"

    def serialize_deref(self) -> bool:
        """Serialized by dereferencing the owned table pointer."""
        return True

    def compare_deref(self) -> bool:
        """Compared by dereferencing the owned table pointer."""
        return True

    def mem_info_account(self) -> bool:
        """Owned tables recurse via collectMemInfo(), not add()."""
        return False


def _resolve_ref_table(ref_type: str, parent: str | None, schema) -> str | None:
    """Owner table the ref getter reads from, honoring any explicit relation."""
    child = ref_type.replace("*", "")
    table = get_table_name(child)
    for relation in schema.relations:
        if relation.parent == parent and relation.child == child:
            table = relation.tbl_name
    return table


def make_field_type(field, schema) -> FieldType:
    """Classify a field into its FieldType.

    Requires the 'bit' pseudo-type to be resolved first (see _normalize_bits), and
    expects field.type to hold the child class name for a table field.
    """
    raw = field.type
    if field.table:
        kind = TableType(raw, page_size=field.page_size)
    elif field.parent is not None and is_ref(raw):
        ref_type = get_ref_type(raw)
        ref_table = (
            _resolve_ref_table(ref_type, field.parent, schema) if ref_type else None
        )
        kind = RefType(raw, ref_table)
    elif is_hash_table(raw):
        kind = HashTableType(raw)
    elif field.bits == 1:
        kind = BoolType(raw)
    elif is_pass_by_ref(raw):
        kind = VectorType(raw)
    elif raw.replace(" ", "") == "char*":
        kind = CharPtrType(raw)
    elif raw == "std::string":
        kind = StringType(raw)
    elif raw.startswith("std::pair"):
        kind = PairType(raw)
    else:
        kind = ScalarType(raw)

    kind.type_set_by_ref = is_set_by_ref(raw, schema.enum_type_names)
    return kind
