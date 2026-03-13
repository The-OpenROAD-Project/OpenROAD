// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025, The OpenROAD Authors

#include <string>

#include "gtest/gtest.h"
#include "odb/db.h"
#include "odb/def2gds.h"
#include "odb/gdsUtil.h"
#include "odb/geom.h"
#include "tst/fixture.h"
#include "utl/Logger.h"

namespace odb::gds {
namespace {

using tst::Fixture;

TEST_F(Fixture, validate_no_errors)
{
  dbGDSLib* lib = createEmptyGDSLib(db_.get(), "test_lib");

  dbGDSStructure* top = dbGDSStructure::create(lib, "top");
  dbGDSStructure* cell_a = dbGDSStructure::create(lib, "cell_a");

  // Add content to cell_a so it's not empty
  dbGDSBoundary* bnd = dbGDSBoundary::create(cell_a);
  bnd->setLayer(1);
  bnd->setDatatype(0);
  std::vector<Point> xy
      = {Point(0, 0), Point(10, 0), Point(10, 10), Point(0, 10), Point(0, 0)};
  bnd->setXy(xy);

  // Reference cell_a from top
  dbGDSSRef::create(top, cell_a);

  DefToGds converter(&logger_);
  int errors = converter.validate(lib, "top");
  EXPECT_EQ(errors, 0);
}

TEST_F(Fixture, validate_empty_cell)
{
  dbGDSLib* lib = createEmptyGDSLib(db_.get(), "test_lib");

  dbGDSStructure* top = dbGDSStructure::create(lib, "top");
  dbGDSStructure* empty_cell = dbGDSStructure::create(lib, "empty_cell");

  // Reference the empty cell from top
  dbGDSSRef::create(top, empty_cell);

  DefToGds converter(&logger_);
  int errors = converter.validate(lib, "top");
  EXPECT_EQ(errors, 1);  // empty_cell has no GDS content
}

TEST_F(Fixture, validate_empty_allowed)
{
  dbGDSLib* lib = createEmptyGDSLib(db_.get(), "test_lib");

  dbGDSStructure* top = dbGDSStructure::create(lib, "top");
  dbGDSStructure* empty_cell = dbGDSStructure::create(lib, "empty_cell");

  dbGDSSRef::create(top, empty_cell);

  DefToGds converter(&logger_);
  // empty_cell matches the allow_empty regex
  int errors = converter.validate(lib, "top", "empty_.*");
  EXPECT_EQ(errors, 0);
}

TEST_F(Fixture, validate_orphan_cell)
{
  dbGDSLib* lib = createEmptyGDSLib(db_.get(), "test_lib");

  dbGDSStructure::create(lib, "top");

  // Create a cell with content but not referenced from top
  dbGDSStructure* orphan = dbGDSStructure::create(lib, "orphan_cell");
  dbGDSBoundary* bnd = dbGDSBoundary::create(orphan);
  bnd->setLayer(1);
  bnd->setDatatype(0);
  std::vector<Point> xy
      = {Point(0, 0), Point(10, 0), Point(10, 10), Point(0, 10), Point(0, 0)};
  bnd->setXy(xy);

  DefToGds converter(&logger_);
  int errors = converter.validate(lib, "top");
  EXPECT_EQ(errors, 1);  // orphan_cell is not referenced
}

}  // namespace
}  // namespace odb::gds
