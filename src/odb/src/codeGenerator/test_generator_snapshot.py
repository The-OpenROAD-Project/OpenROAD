# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2025, The OpenROAD Authors

"""Fine-grained regression tests for the odb code generator.

These render templates against the real committed schema and pin the rendered
output for one representative field of every kind (scalar, char*, std::string,
std::vector/dbVector pass-by-ref, dbId object ref, owned dbTable, dbHashTable).
The whole-tree guard is `python3 gen.py` producing no git diff (enforced by CI in
github-actions-are-odb-files-generated.yml).

Run with:  python3 -m unittest test_generator_snapshot
"""

import os
import unittest
from pathlib import Path

from gen import ODBGenerator, make_environment

HERE = Path(os.path.dirname(__file__))


def _processed_schema():
    env = make_environment(HERE / "templates")
    gen = ODBGenerator(env, ".", ".", False)
    schema = gen.load_schema(HERE / "schema.json")
    gen.process_schema(schema)
    return env, schema, {k.name: k for k in schema.classes}


class SnapshotTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.env, cls.schema, cls.by_name = _processed_schema()

    def render(self, class_name, template):
        return self.env.get_template(template).render(
            klass=self.by_name[class_name], schema=self.schema
        )

    def assertLine(self, class_name, template, line):
        """Assert an exact (stripped) line appears in the rendered template."""
        rendered = self.render(class_name, template)
        lines = {ln.strip() for ln in rendered.splitlines()}
        self.assertIn(
            line, lines, f"{line!r} not found in rendered {class_name}/{template}"
        )

    # --- char* name_ -> getter returns const char*, freed in destructor ---
    def test_charptr_getter_and_destructor(self):
        for klass in ("dbTechLayerCutClassRule", "dbGDSStructure"):
            self.assertLine(
                klass, "impl.cpp.jinja", f"const char * {klass}::getName() const"
            )
            self.assertLine(klass, "impl.cpp.jinja", "free((void*) name_);")

    # --- std::string -> setter takes const std::string& (set-by-ref) ---
    def test_string_setter_is_const_ref(self):
        self.assertLine(
            "dbGDSText",
            "impl.cpp.jinja",
            "void dbGDSText::setText( const std::string& text )",
        )

    # --- vector -> getter is an out-parameter by reference ---
    def test_vector_getter_is_out_param(self):
        self.assertLine(
            "dbGDSPath",
            "impl.cpp.jinja",
            "void dbGDSPath::getXy(std::vector<Point>& tbl) const",
        )

    # --- value-struct setter passes by const ref; enum setter passes by value ---
    def test_setter_by_ref_for_value_struct_by_value_for_enum(self):
        self.assertLine(
            "dbGDSBox",
            "impl.cpp.jinja",
            "void dbGDSBox::setBounds( const Rect& bounds )",
        )
        self.assertLine(
            "dbGDSSRef",
            "impl.cpp.jinja",
            "void dbGDSSRef::setTransform( dbGDSSTrans transform )",
        )

    # --- dbId object ref -> getter resolves via owner table getPtr ---
    def test_ref_getter_uses_getptr(self):
        self.assertLine(
            "dbGlobalConnect",
            "impl.cpp.jinja",
            "dbNet* dbGlobalConnect::getNet() const",
        )
        self.assertLine(
            "dbGlobalConnect",
            "impl.cpp.jinja",
            "return (dbRegion*) par->region_tbl_->getPtr(obj->region_);",
        )

    # --- owned table -> dbSet<> getter; hashed child -> .find(name) ---
    def test_table_dbset_and_hash_find(self):
        self.assertLine(
            "dbTechLayer",
            "impl.cpp.jinja",
            "dbSet<dbTechLayerCutClassRule> dbTechLayer::getTechLayerCutClassRules() const",
        )
        self.assertLine(
            "dbTechLayer",
            "impl.cpp.jinja",
            "return (dbTechLayerCutClassRule*) obj->cut_class_rules_hash_.find(name);",
        )

    # --- scoped enum ("Foo::Value") is a comparison leaf, not silently dropped ---
    def test_enum_field_is_compared(self):
        # cmpgt enums appear in both operator== and operator< (dbAccessType::Value).
        self.assertLine(
            "dbAccessPoint", "impl.cpp.jinja", "if (low_type_ != rhs.low_type_) {"
        )
        self.assertLine(
            "dbAccessPoint", "impl.cpp.jinja", "if (low_type_ >= rhs.low_type_) {"
        )
        # enum bitfields compare via the packed flags_ member.
        self.assertLine(
            "dbTechLayer", "impl.cpp.jinja", "if (flags_.type != rhs.flags_.type) {"
        )


if __name__ == "__main__":
    unittest.main()
