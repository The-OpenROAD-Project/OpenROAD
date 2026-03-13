// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025, The OpenROAD Authors
//
// Test pruneUnreferencedCells: verifies that cells not transitively
// referenced from the top cell are removed from the GDS library.

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

// Build a GDS library with:
//   top -> child_a -> grandchild
//   orphan  (unreferenced)
// After pruning, orphan should be gone, the other 3 should remain.
TEST_F(Fixture, prune_removes_orphan_cells)
{
  dbGDSLib* lib = createEmptyGDSLib(db_.get(), "test_lib");

  // Create structures
  dbGDSStructure* grandchild = dbGDSStructure::create(lib, "grandchild");
  dbGDSBoundary* bnd = dbGDSBoundary::create(grandchild);
  bnd->setLayer(1);
  bnd->setDatatype(0);
  bnd->setXy({Point(0, 0), Point(100, 0), Point(100, 100), Point(0, 100),
               Point(0, 0)});

  dbGDSStructure* child_a = dbGDSStructure::create(lib, "child_a");
  dbGDSSRef* sref_gc = dbGDSSRef::create(child_a, grandchild);
  sref_gc->setOrigin(Point(0, 0));

  dbGDSStructure* top = dbGDSStructure::create(lib, "top");
  dbGDSSRef* sref_a = dbGDSSRef::create(top, child_a);
  sref_a->setOrigin(Point(0, 0));

  // Unreferenced cell
  dbGDSStructure* orphan = dbGDSStructure::create(lib, "orphan");
  dbGDSBoundary* orphan_bnd = dbGDSBoundary::create(orphan);
  orphan_bnd->setLayer(2);
  orphan_bnd->setDatatype(0);
  orphan_bnd->setXy({Point(0, 0), Point(50, 0), Point(50, 50), Point(0, 50),
                      Point(0, 0)});

  EXPECT_EQ(lib->getGDSStructures().size(), 4);

  DefToGds converter(&logger_);
  int removed = converter.pruneUnreferencedCells(lib, "top");

  EXPECT_EQ(removed, 1);
  EXPECT_EQ(lib->getGDSStructures().size(), 3);
  EXPECT_NE(lib->findGDSStructure("top"), nullptr);
  EXPECT_NE(lib->findGDSStructure("child_a"), nullptr);
  EXPECT_NE(lib->findGDSStructure("grandchild"), nullptr);
  EXPECT_EQ(lib->findGDSStructure("orphan"), nullptr);
}

// When all cells are referenced, nothing should be pruned.
TEST_F(Fixture, prune_keeps_all_when_fully_referenced)
{
  dbGDSLib* lib = createEmptyGDSLib(db_.get(), "test_lib");

  dbGDSStructure* child = dbGDSStructure::create(lib, "child");
  dbGDSBoundary* bnd = dbGDSBoundary::create(child);
  bnd->setLayer(1);
  bnd->setDatatype(0);
  bnd->setXy({Point(0, 0), Point(100, 0), Point(100, 100), Point(0, 100),
               Point(0, 0)});

  dbGDSStructure* top = dbGDSStructure::create(lib, "top");
  dbGDSSRef* sref = dbGDSSRef::create(top, child);
  sref->setOrigin(Point(0, 0));

  EXPECT_EQ(lib->getGDSStructures().size(), 2);

  DefToGds converter(&logger_);
  int removed = converter.pruneUnreferencedCells(lib, "top");

  EXPECT_EQ(removed, 0);
  EXPECT_EQ(lib->getGDSStructures().size(), 2);
}

// Multiple orphans should all be removed.
TEST_F(Fixture, prune_removes_multiple_orphans)
{
  dbGDSLib* lib = createEmptyGDSLib(db_.get(), "test_lib");

  dbGDSStructure* top = dbGDSStructure::create(lib, "top");
  dbGDSBoundary* bnd = dbGDSBoundary::create(top);
  bnd->setLayer(1);
  bnd->setDatatype(0);
  bnd->setXy({Point(0, 0), Point(100, 0), Point(100, 100), Point(0, 100),
               Point(0, 0)});

  // Create 5 orphan cells
  for (int i = 0; i < 5; ++i) {
    std::string name = "orphan_" + std::to_string(i);
    dbGDSStructure* orphan = dbGDSStructure::create(lib, name.c_str());
    dbGDSBoundary* obnd = dbGDSBoundary::create(orphan);
    obnd->setLayer(2);
    obnd->setDatatype(0);
    obnd->setXy({Point(0, 0), Point(50, 0), Point(50, 50), Point(0, 50),
                  Point(0, 0)});
  }

  EXPECT_EQ(lib->getGDSStructures().size(), 6);

  DefToGds converter(&logger_);
  int removed = converter.pruneUnreferencedCells(lib, "top");

  EXPECT_EQ(removed, 5);
  EXPECT_EQ(lib->getGDSStructures().size(), 1);
  EXPECT_NE(lib->findGDSStructure("top"), nullptr);
}

}  // namespace
}  // namespace odb::gds
