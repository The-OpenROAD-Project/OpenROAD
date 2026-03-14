// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025, The OpenROAD Authors
//
// Regression test for MYR90/MXR90 orientation mapping (PR review fix).
// Verifies that all 8 dbOrientType values produce the correct GDS STRANS.

#include <string>

#include "gtest/gtest.h"
#include "odb/db.h"
#include "odb/def2gds.h"
#include "odb/geom.h"
#include "tst/fixture.h"

namespace odb::gds {
namespace {

using tst::Fixture;

// Helper: create a minimal design with a single instance at a given orientation
// and verify the GDS SREF transform matches expected values.
class OrientationTest : public Fixture
{
 protected:
  void checkOrientation(dbOrientType orient,
                        bool expected_flip,
                        double expected_angle)
  {
    // Create a minimal tech and design
    dbTech* tech = dbTech::create(db_.get(), "test_tech");
    dbTechLayer* layer
        = dbTechLayer::create(tech, "M1", dbTechLayerType::ROUTING);
    layer->setWidth(100);

    dbLib* lib = dbLib::create(db_.get(), "test_lib", tech);
    dbMaster* master = dbMaster::create(lib, "test_cell");
    master->setType(dbMasterType::CORE);
    master->setWidth(1000);
    master->setHeight(1000);
    master->setFrozen();

    dbChip* chip = dbChip::create(db_.get(), tech);
    dbBlock* block = dbBlock::create(chip, "test_block");
    block->setDieArea(Rect(0, 0, 10000, 10000));
    block->setDefUnits(1000);

    dbInst* inst = dbInst::create(block, master, "inst0");
    inst->setOrient(orient);
    inst->setLocation(500, 500);
    inst->setPlacementStatus(dbPlacementStatus::PLACED);

    // Set up a trivial layer map
    LayerMap lmap;
    DefToGds converter(&logger_);
    converter.setLayerMap(lmap);

    // Convert to GDS
    dbGDSLib* gds_lib = converter.convert(block, db_.get());
    ASSERT_NE(gds_lib, nullptr);

    // Find the top structure
    dbGDSStructure* top = gds_lib->findGDSStructure("test_block");
    ASSERT_NE(top, nullptr);

    // Should have exactly one SREF
    auto srefs = top->getGDSSRefs();
    ASSERT_EQ(srefs.size(), 1);

    dbGDSSRef* sref = *srefs.begin();
    dbGDSSTrans strans = sref->getTransform();

    EXPECT_EQ(strans.flipX_, expected_flip)
        << "Orientation " << orient.getString() << " flipX mismatch";
    EXPECT_DOUBLE_EQ(strans.angle_, expected_angle)
        << "Orientation " << orient.getString() << " angle mismatch";
  }
};

TEST_F(OrientationTest, R0)
{
  checkOrientation(dbOrientType::R0, false, 0.0);
}

TEST_F(OrientationTest, R90)
{
  checkOrientation(dbOrientType::R90, false, 90.0);
}

TEST_F(OrientationTest, R180)
{
  checkOrientation(dbOrientType::R180, false, 180.0);
}

TEST_F(OrientationTest, R270)
{
  checkOrientation(dbOrientType::R270, false, 270.0);
}

// Regression: MY was previously mapping to angle=0 (incorrect, that's MX).
// GDS flipX negates Y (= MX), then 180° rotation gives (x,y)->(-x,y) = MY.
TEST_F(OrientationTest, MY)
{
  checkOrientation(dbOrientType::MY, true, 180.0);
}

// Regression: MYR90 was previously mapping to angle=90 (incorrect, that's
// MXR90)
TEST_F(OrientationTest, MYR90)
{
  checkOrientation(dbOrientType::MYR90, true, 270.0);
}

// Regression: MX was previously mapping to angle=180 (incorrect, that's MY).
// GDS flipX alone: (x,y)->(x,-y) = MX.
TEST_F(OrientationTest, MX)
{
  checkOrientation(dbOrientType::MX, true, 0.0);
}

// Regression: MXR90 was previously mapping to angle=270 (incorrect, that's
// MYR90)
TEST_F(OrientationTest, MXR90)
{
  checkOrientation(dbOrientType::MXR90, true, 90.0);
}

}  // namespace
}  // namespace odb::gds
