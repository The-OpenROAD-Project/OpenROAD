// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025, The OpenROAD Authors
//
// Test mergeSeal: verifies that seal GDS cells are merged as children
// of the top cell without reading the seal file twice (PR review fix).

#include <filesystem>
#include <string>

#include "gtest/gtest.h"
#include "odb/db.h"
#include "odb/def2gds.h"
#include "odb/gdsUtil.h"
#include "odb/gdsout.h"
#include "odb/geom.h"
#include "tst/fixture.h"
#include "utl/Logger.h"

namespace odb::gds {
namespace {

using tst::Fixture;

TEST_F(Fixture, merge_seal_adds_sref)
{
  // Create a seal GDS file with one cell
  dbGDSLib* seal_lib = createEmptyGDSLib(db_.get(), "seal_lib");
  dbGDSStructure* seal_top = dbGDSStructure::create(seal_lib, "seal_ring");
  dbGDSBoundary* bnd = dbGDSBoundary::create(seal_top);
  bnd->setLayer(1);
  bnd->setDatatype(0);
  std::vector<Point> xy = {
      Point(0, 0), Point(100, 0), Point(100, 100), Point(0, 100), Point(0, 0)};
  bnd->setXy(xy);

  std::filesystem::create_directory("results");
  const char* seal_file = "results/seal_test.gds";
  GDSWriter writer(&logger_);
  writer.write_gds(seal_lib, seal_file);

  // Create main library with a top cell
  dbGDSLib* main_lib = createEmptyGDSLib(db_.get(), "main_lib");
  dbGDSStructure* top = dbGDSStructure::create(main_lib, "design_top");

  // Add a regular cell to top
  dbGDSStructure* cell_a = dbGDSStructure::create(main_lib, "cell_a");
  dbGDSBoundary* cell_bnd = dbGDSBoundary::create(cell_a);
  cell_bnd->setLayer(2);
  cell_bnd->setDatatype(0);
  cell_bnd->setXy(xy);
  dbGDSSRef::create(top, cell_a);

  // Merge seal
  DefToGds converter(&logger_);
  converter.mergeSeal(main_lib, "design_top", seal_file);

  // Verify seal_ring was added as a child of design_top
  auto srefs = top->getGDSSRefs();
  EXPECT_EQ(srefs.size(), 2);  // cell_a + seal_ring

  bool found_seal = false;
  for (dbGDSSRef* sref : srefs) {
    if (std::string(sref->getStructure()->getName()) == "seal_ring") {
      found_seal = true;
      // Seal should be at origin with identity transform
      EXPECT_EQ(sref->getOrigin(), Point(0, 0));
      EXPECT_FALSE(sref->getTransform().flipX_);
      EXPECT_DOUBLE_EQ(sref->getTransform().angle_, 0.0);
    }
  }
  EXPECT_TRUE(found_seal);
}

TEST_F(Fixture, merge_seal_hierarchical)
{
  // Create a seal GDS file with hierarchical cells:
  // seal_top references seal_child
  dbGDSLib* seal_lib = createEmptyGDSLib(db_.get(), "seal_lib");

  dbGDSStructure* seal_child = dbGDSStructure::create(seal_lib, "seal_child");
  dbGDSBoundary* child_bnd = dbGDSBoundary::create(seal_child);
  child_bnd->setLayer(1);
  child_bnd->setDatatype(0);
  std::vector<Point> xy
      = {Point(0, 0), Point(50, 0), Point(50, 50), Point(0, 50), Point(0, 0)};
  child_bnd->setXy(xy);

  dbGDSStructure* seal_top = dbGDSStructure::create(seal_lib, "seal_ring");
  dbGDSSRef::create(seal_top, seal_child);

  std::filesystem::create_directory("results");
  const char* seal_file = "results/seal_hier_test.gds";
  GDSWriter writer(&logger_);
  writer.write_gds(seal_lib, seal_file);

  // Create main library
  dbGDSLib* main_lib = createEmptyGDSLib(db_.get(), "main_lib");
  dbGDSStructure* top = dbGDSStructure::create(main_lib, "design_top");

  DefToGds converter(&logger_);
  converter.mergeSeal(main_lib, "design_top", seal_file);

  // Only seal_ring (the top cell of the seal) should be added as child
  // seal_child should NOT be a direct child of design_top
  auto srefs = top->getGDSSRefs();
  EXPECT_EQ(srefs.size(), 1);

  dbGDSSRef* sref = *srefs.begin();
  EXPECT_STREQ(sref->getStructure()->getName(), "seal_ring");

  // Both seal cells should exist in the library
  EXPECT_NE(main_lib->findGDSStructure("seal_ring"), nullptr);
  EXPECT_NE(main_lib->findGDSStructure("seal_child"), nullptr);
}

TEST_F(Fixture, merge_seal_empty_file)
{
  // mergeSeal with empty string should be a no-op
  dbGDSLib* lib = createEmptyGDSLib(db_.get(), "test_lib");
  dbGDSStructure* top = dbGDSStructure::create(lib, "top");

  DefToGds converter(&logger_);
  converter.mergeSeal(lib, "top", "");

  // Top should have no children
  EXPECT_EQ(top->getGDSSRefs().size(), 0);
}

}  // namespace
}  // namespace odb::gds
