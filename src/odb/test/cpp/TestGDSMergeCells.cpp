// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025, The OpenROAD Authors

#include <filesystem>
#include <string>

#include "gtest/gtest.h"
#include "odb/db.h"
#include "odb/def2gds.h"
#include "odb/gdsUtil.h"
#include "odb/gdsin.h"
#include "odb/gdsout.h"
#include "odb/geom.h"
#include "tst/fixture.h"
#include "utl/Logger.h"

namespace odb::gds {
namespace {

using tst::Fixture;

TEST_F(Fixture, merge_cells)
{
  // Create a "source" GDS file with a cell
  std::string libname = "source_lib";
  dbGDSLib* source_lib = createEmptyGDSLib(db_.get(), libname);

  dbGDSStructure* cell_a = dbGDSStructure::create(source_lib, "cell_a");
  dbGDSBoundary* bnd = dbGDSBoundary::create(cell_a);
  bnd->setLayer(10);
  bnd->setDatatype(0);
  std::vector<Point> xy = {
      Point(0, 0), Point(100, 0), Point(100, 100), Point(0, 100), Point(0, 0)};
  bnd->setXy(xy);

  // Write it to a temporary file
  std::filesystem::create_directory("results");
  const char* source_file = "results/merge_source.gds";
  GDSWriter writer(&logger_);
  writer.write_gds(source_lib, source_file);

  // Create a target library with a placeholder structure
  dbGDSLib* target_lib = createEmptyGDSLib(db_.get(), "target_lib");
  dbGDSStructure* top = dbGDSStructure::create(target_lib, "top");
  dbGDSStructure* placeholder = dbGDSStructure::create(target_lib, "cell_a");

  // The placeholder should be empty
  EXPECT_TRUE(placeholder->getGDSBoundaries().empty());

  // Create SREF from top to placeholder
  dbGDSSRef::create(top, placeholder);

  // Merge the source file
  DefToGds converter(&logger_);
  converter.mergeCells(target_lib, {source_file});

  // After merge, cell_a should have the boundary
  dbGDSStructure* merged = target_lib->findGDSStructure("cell_a");
  ASSERT_NE(merged, nullptr);
  EXPECT_EQ(merged->getGDSBoundaries().size(), 1);

  auto bnd_it = merged->getGDSBoundaries().begin();
  dbGDSBoundary* merged_bnd = *bnd_it;
  EXPECT_EQ(merged_bnd->getLayer(), 10);
  EXPECT_EQ(merged_bnd->getDatatype(), 0);
}

}  // namespace
}  // namespace odb::gds
