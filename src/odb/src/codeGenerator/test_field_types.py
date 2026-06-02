# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2025, The OpenROAD Authors

"""Validates the Field / FieldType contract against the real committed schema.

Processes every field in the schema and checks that each carries a `kind`
(field_types.FieldType) and that the Field accessor properties
(setterArgumentType, refType, ...) resolve from that kind.

Run with:  python3 -m unittest test_field_types
"""

import os
import unittest
from pathlib import Path

from field_types import CharPtrType, make_field_type
from gen import ODBGenerator, make_environment
from helper import components, is_enum, is_set_by_ref, mem_info_accountable
from schema_models import Field, Schema

HERE = Path(os.path.dirname(__file__))


def _all_fields():
    env = make_environment(HERE / "templates")
    gen = ODBGenerator(env, ".", ".", False)
    schema = gen.load_schema(HERE / "schema.json")
    gen.process_schema(schema)
    for klass in schema.classes:
        for field in klass.fields:
            yield klass, field
        # The in-class (flags_) struct members get accessors generated too, so
        # they must carry a kind as well.
        for struct in klass.structs:
            if struct.in_class_name:
                for field in struct.fields:
                    yield klass, field


class FieldKindContractTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.fields = list(_all_fields())
        assert cls.fields, "no fields processed"

    def test_every_field_has_a_kind(self):
        for klass, field in self.fields:
            self.assertIsNotNone(field.kind, f"{klass.name}.{field.name} has no kind")

    def test_field_properties_resolve_from_kind(self):
        for klass, field in self.fields:
            k = field.kind
            ctx = f"{klass.name}.{field.name} (type={field.type!r})"

            # classification booleans
            self.assertEqual(k.is_hash_table, field.isHashTable, f"isHashTable {ctx}")
            self.assertEqual(k.is_table, bool(field.table), f"table {ctx}")
            self.assertEqual(
                k.db_set_getter, bool(field.dbSetGetter), f"dbSetGetter {ctx}"
            )

            # isSetByRef is determined solely by the kind.
            self.assertEqual(k.type_set_by_ref, field.isSetByRef, f"isSetByRef {ctx}")

            # accessor signature types. The synthetic flags_ aggregate has no
            # accessor types (no-get/no-set, handled by the bitFields branch).
            if field.setterArgumentType is not None:
                self.assertEqual(
                    k.setter_arg_type, field.setterArgumentType, f"setterArgType {ctx}"
                )
            if field.getterReturnType is not None:
                self.assertEqual(
                    k.getter_return_type, field.getterReturnType, f"getterRetType {ctx}"
                )

            # type-dependent details, checked where they apply
            if field.refType is not None:
                self.assertEqual(k.ref_type, field.refType, f"refType {ctx}")
                self.assertEqual(k.ref_table, field.refTable, f"refTable {ctx}")
            if field.isHashTable:
                self.assertEqual(
                    k.hash_table_type, field.hashTableType, f"hashTableType {ctx}"
                )
            if field.table:
                self.assertEqual(
                    k.table_base_type, field.table_base_type, f"tableBaseType {ctx}"
                )

    def test_table_member_decl_is_owned_table_pointer(self):
        """Owned-table members declare a dbTable<_Child[, N]>*; type is the child."""
        for klass, field in self.fields:
            if field.table:
                ctx = f"{klass.name}.{field.name}"
                decl = field.member_decl
                self.assertTrue(
                    decl.startswith(f"dbTable<_{field.table_base_type}")
                    and decl.endswith(">*"),
                    f"member_decl {ctx} = {decl!r}",
                )
                # field.type holds the child class name; member_decl adds the wrapper.
                self.assertEqual(field.type, field.table_base_type, f"type {ctx}")


class CharPtrSpellingTest(unittest.TestCase):
    """Both "char *" and "char*" classify as CharPtrType (spacing-insensitive)."""

    def _kind(self, type_str):
        return make_field_type(Field(name="name_", type=type_str), Schema("."))

    def test_both_spellings_are_charptr(self):
        for spelling in ("char *", "char*"):
            kind = self._kind(spelling)
            self.assertIsInstance(kind, CharPtrType, spelling)
            self.assertEqual(kind.getter_return_type, "const char *", spelling)
            self.assertEqual(kind.setter_arg_type, "char *", spelling)
            self.assertTrue(kind.needs_free("name_"), spelling)
            self.assertFalse(kind.needs_free("other_"), spelling)

    def test_member_decl_preserves_original_spelling(self):
        # The stored member keeps the authored spelling; clang-format normalizes it.
        self.assertEqual(self._kind("char *").member_decl(), "char *")
        self.assertEqual(self._kind("char*").member_decl(), "char*")

    def test_plain_char_is_not_a_pointer(self):
        self.assertNotIsInstance(self._kind("char"), CharPtrType)


class SetByRefRuleTest(unittest.TestCase):
    """is_set_by_ref passes everything by const ref except scalars/pointers/enums."""

    ENUMS = {"dbGDSSTrans", "dbOrientType3D"}

    def test_const_ref_for_aggregates(self):
        for t in (
            "std::string",
            "std::pair<int,int>",
            "std::vector<Point>",
            "std::map<int,int>",
            "dbVector<int>",
            "Rect",
            "Point",
            "Polygon",
        ):
            self.assertTrue(is_set_by_ref(t, self.ENUMS), t)

    def test_by_value_for_pod_and_pointers_and_enums(self):
        for t in (
            "int",
            "float",
            "bool",
            "uint32_t",
            "int16_t",
            "char *",
            "char*",
            "dbId<_dbNet>",
            "dbHashTable<_dbInst>",
            "dbGDSSTrans",  # registered external enum
            "dbAccessType::Value",  # scoped enum
        ):
            self.assertFalse(is_set_by_ref(t, self.ENUMS), t)

    def test_nested_struct_is_const_ref_not_by_value(self):
        # A scoped name that is not a "::Value" enum (e.g. a nested struct) must
        # be passed by const ref, not by value -- guards the is_enum() precision.
        self.assertTrue(is_set_by_ref("dbPowerSwitch::UPFControlPort", self.ENUMS))


class ComponentsRuleTest(unittest.TestCase):
    """components() decides which fields contribute to operator==/operator<."""

    def test_scoped_enum_is_a_comparison_leaf(self):
        self.assertTrue(is_enum("dbAccessType::Value"))
        self.assertEqual(
            components([], "low_type_", "dbAccessType::Value"), ["low_type_"]
        )

    def test_non_value_scoped_name_is_not_an_enum(self):
        # Nested structs share the "X::Y" shape but must not be treated as leaves.
        self.assertFalse(is_enum("dbPowerSwitch::UPFControlPort"))
        self.assertFalse(is_enum("std::string"))

    def test_scalar_and_ref_remain_leaves(self):
        self.assertEqual(components([], "x_", "int"), ["x_"])
        self.assertEqual(components([], "net_", "dbId<_dbNet>"), ["net_"])

    def test_uncompared_container_yields_nothing(self):
        self.assertEqual(components([], "tbl_", "dbVector<int>"), [])

    def test_cmpgt_field_with_empty_components_raises(self):
        # A field flagged for ordering must resolve to a component; an unhandled
        # type would otherwise drop its operator< term silently.
        env = make_environment(HERE / "templates")
        gen = ODBGenerator(env, ".", ".", False)
        schema = gen.load_schema(HERE / "schema.json")
        klass = schema.classes[0]
        klass.fields = [Field(name="orphan_", type="dbVector<int>", flags=["cmpgt"])]
        with self.assertRaises(ValueError):
            gen.process_schema(schema)


class FactoryGenerationTest(unittest.TestCase):
    """An opted-in relation wires the child's generated create()/destroy()."""

    @classmethod
    def setUpClass(cls):
        env = make_environment(HERE / "templates")
        gen = ODBGenerator(env, ".", ".", False)
        schema = gen.load_schema(HERE / "schema.json")
        gen.process_schema(schema)
        cls.by_name = {k.name: k for k in schema.classes}
        cls.relations = schema.relations

    def test_opted_in_relation_populates_child_factory(self):
        rel = next((r for r in self.relations if r.create or r.destroy), None)
        if rel is None:
            self.skipTest("no relation opts into factory generation")
        child = self.by_name[rel.child]
        self.assertEqual(child.factory_parent, rel.parent)
        self.assertEqual(child.factory_table, rel.tbl_name)
        self.assertEqual(child.gen_create, rel.create)
        self.assertEqual(child.gen_destroy, rel.destroy)
        # The parent's internal header is pulled in for the owner cast.
        self.assertIn(f"{rel.parent}.h", child.cpp_includes)

    def test_non_factory_class_has_no_factory(self):
        # dbCellEdgeSpacing has a hand-managed owner table (no relation), so it
        # never opts into factory generation.
        klass = self.by_name["dbCellEdgeSpacing"]
        self.assertFalse(klass.gen_create)
        self.assertFalse(klass.gen_destroy)
        self.assertIsNone(klass.factory_parent)


class MemInfoAccountableTest(unittest.TestCase):
    """mem_info_accountable matches the heap types MemInfo::add() overloads accept."""

    def test_accountable_heap_types(self):
        for t in (
            "std::string",
            "std::vector<Point>",
            "dbVector<int>",
            "dbHashTable<_dbInst>",
            "std::map<int,int>",
            "std::unordered_map<std::string, dbId<_dbInst>>",
            "std::set<int>",
        ):
            self.assertTrue(mem_info_accountable(t), t)

    def test_not_accountable_inline_types(self):
        # Scalars, refs, and small aggregates own no heap and have no overload.
        for t in (
            "int",
            "bool",
            "double",
            "uint32_t",
            "dbId<_dbNet>",
            "std::pair<int,int>",
            "std::array<bool, 6>",
            "std::tuple<Rect, bool, bool>",
            "char *",
            "Point",
            "Polygon",
        ):
            self.assertFalse(mem_info_accountable(t), t)


if __name__ == "__main__":
    unittest.main()
