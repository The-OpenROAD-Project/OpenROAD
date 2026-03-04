// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023-2026, The OpenROAD Authors

#include <string>

#include "Test3DBloxCheckerFixture.h"
#include "gtest/gtest.h"
#include "odb/3dblox.h"
#include "odb/db.h"
#include "odb/dbWireCodec.h"
#include "odb/geom.h"

namespace odb {
namespace {

class NetConnectivityFixture : public CheckerFixture
{
 protected:
  static constexpr const char* open_nets_category = "Open nets";

  NetConnectivityFixture()
  {
    dbTechLayer::create(tech_, "layer1", dbTechLayerType::ROUTING);

    // DIE chips need blocks (for bump masters/cells)
    dbBlock::create(chip1_, "block1");
    dbBlock::create(chip2_, "block2");

    // Lib + bump master
    lib_ = dbLib::create(db_.get(), "bump_lib", tech_, ',');
    bump_master_ = dbMaster::create(lib_, "bump_pad");
    bump_master_->setWidth(100);
    bump_master_->setHeight(100);
    bump_master_->setType(dbMasterType::CORE);
    dbMTerm::create(bump_master_, "pin", dbIoType::INOUT, dbSigType::SIGNAL);
    bump_master_->setFrozen();
  }

  // Create a bump on a chip region.
  // - Creates dbInst at (x,y) in the chip's block
  // - Creates dbChipBump on the region
  // - Creates dbNet, dbBTerm, dbBPin with a box at the bump location
  // - Sets the bump's net and bterm
  // Returns the dbChipBump for further use.
  dbChipBump* createBump(dbChip* chip,
                         dbChipRegion* region,
                         const char* bump_name,
                         int x,
                         int y)
  {
    dbBlock* block = chip->getBlock();
    dbTechLayer* layer = tech_->findLayer("layer1");

    // Physical cell in the block
    dbInst* inst = dbInst::create(block, bump_master_, bump_name);
    inst->setOrigin(x, y);
    inst->setPlacementStatus(dbPlacementStatus::PLACED);

    // Chip-level bump association
    dbChipBump* chip_bump = dbChipBump::create(region, inst);

    // Net + BTerm for wire-graph connectivity
    std::string net_name = std::string(bump_name) + "_net";
    dbNet* net = block->findNet(net_name.c_str());
    if (!net) {
      net = dbNet::create(block, net_name.c_str());
    }
    dbBTerm* bterm = dbBTerm::create(net, bump_name);
    bterm->setIoType(dbIoType::INOUT);

    // BPin with box at the bump location (needed by verifyChipConnectivity)
    dbBPin* bpin = dbBPin::create(bterm);
    bpin->setPlacementStatus(dbPlacementStatus::PLACED);
    dbBox::create(bpin, layer, x, y, x + 100, y + 100);

    chip_bump->setNet(net);
    chip_bump->setBTerm(bterm);

    return chip_bump;
  }

  // Helper: find the Nth bump instance on a chip instance's named region
  dbChipBumpInst* findBumpInst(dbChipInst* chip_inst,
                               const char* region_name,
                               int index)
  {
    auto* ri = chip_inst->findChipRegionInst(region_name);
    int i = 0;
    for (auto* bi : ri->getChipBumpInsts()) {
      if (i == index) {
        return bi;
      }
      i++;
    }
    return nullptr;
  }

  // Create two bumps on the same net and connect them via a wire.
  // Both bumps must be on the same chip / same block.
  void connectBumpsWithWire(dbChip* chip, dbChipBump* b1, dbChipBump* b2)
  {
    dbTechLayer* layer = tech_->findLayer("layer1");

    // Put both BTerms on the same net
    dbNet* net = b1->getNet();
    dbBTerm* bt2 = b2->getBTerm();
    bt2->disconnect();
    bt2->connect(net);

    // Orphan net logic removed as it's dangerous with dbNet::destroy over
    // connected graph nodes
    b2->setNet(net);

    // Encode a simple wire connecting the two BPin locations
    auto get_bpin_center = [](dbBTerm* bt) -> Point {
      dbBPin* bpin = *bt->getBPins().begin();
      dbBox* box = *bpin->getBoxes().begin();
      Rect r = box->getBox();
      return Point((r.xMin() + r.xMax()) / 2, (r.yMin() + r.yMax()) / 2);
    };

    Point p1 = get_bpin_center(b1->getBTerm());
    Point p2 = get_bpin_center(bt2);

    dbWire* wire = dbWire::create(net);
    dbWireEncoder encoder;
    encoder.begin(wire);
    encoder.newPath(layer, dbWireType::ROUTED);
    encoder.addPoint(p1.x(), p1.y());
    if (p1.x() != p2.x()) {
      encoder.addPoint(p2.x(), p1.y());
    }
    if (p1.y() != p2.y()) {
      encoder.addPoint(p2.x(), p2.y());
    }
    encoder.end();
  }

  dbLib* lib_;
  dbMaster* bump_master_;
};

// --- Test: No nets at all → no violations ---
TEST_F(NetConnectivityFixture, test_net_no_bumps_no_violation)
{
  auto inst1 = dbChipInst::create(top_chip_, chip1_, "inst1");
  inst1->setLoc(Point3D(0, 0, 0));
  inst1->setOrient(dbOrientType3D(dbOrientType::R0, false));

  auto inst2 = dbChipInst::create(top_chip_, chip2_, "inst2");
  inst2->setLoc(Point3D(0, 0, 500));
  inst2->setOrient(dbOrientType3D(dbOrientType::R0, false));

  auto* ri1 = inst1->findChipRegionInst("r1_fr");
  auto* ri2 = inst2->findChipRegionInst("r2_bk");
  auto* conn = dbChipConn::create("c1", top_chip_, {inst1}, ri1, {inst2}, ri2);
  conn->setThickness(0);

  check();
  EXPECT_TRUE(getMarkers(open_nets_category).empty());
}

// --- Test: Net with single bump → skipped (trivially connected) ---
TEST_F(NetConnectivityFixture, test_net_single_bump_skipped)
{
  createBump(chip1_, chip1_->findChipRegion("r1_fr"), "b1", 100, 100);

  auto inst1 = dbChipInst::create(top_chip_, chip1_, "inst1");
  inst1->setLoc(Point3D(0, 0, 0));
  inst1->setOrient(dbOrientType3D(dbOrientType::R0, false));

  auto inst2 = dbChipInst::create(top_chip_, chip2_, "inst2");
  inst2->setLoc(Point3D(0, 0, 500));
  inst2->setOrient(dbOrientType3D(dbOrientType::R0, false));

  auto* ri1 = inst1->findChipRegionInst("r1_fr");
  auto* ri2 = inst2->findChipRegionInst("r2_bk");
  auto* conn = dbChipConn::create("c1", top_chip_, {inst1}, ri1, {inst2}, ri2);
  conn->setThickness(0);

  // Create net with only 1 bump
  auto* chip_net = dbChipNet::create(top_chip_, "net1");
  auto* bi1 = findBumpInst(inst1, "r1_fr", 0);
  chip_net->addBumpInst(bi1, {inst1});

  check();
  EXPECT_TRUE(getMarkers(open_nets_category).empty());
}

// --- Test: Two bumps on different chips, same XY, valid connection → connected
// ---
TEST_F(NetConnectivityFixture, test_net_cross_chip_connected)
{
  // Bumps at same local position (100, 100)
  createBump(chip1_, chip1_->findChipRegion("r1_fr"), "b1", 100, 100);
  createBump(chip2_, chip2_->findChipRegion("r2_bk"), "b2", 100, 100);

  auto inst1 = dbChipInst::create(top_chip_, chip1_, "inst1");
  inst1->setLoc(Point3D(0, 0, 0));
  inst1->setOrient(dbOrientType3D(dbOrientType::R0, false));

  auto inst2 = dbChipInst::create(top_chip_, chip2_, "inst2");
  inst2->setLoc(Point3D(0, 0, 500));
  inst2->setOrient(dbOrientType3D(dbOrientType::R0, false));

  auto* ri1 = inst1->findChipRegionInst("r1_fr");
  auto* ri2 = inst2->findChipRegionInst("r2_bk");
  auto* conn = dbChipConn::create("c1", top_chip_, {inst1}, ri1, {inst2}, ri2);
  conn->setThickness(0);

  // Net with both bumps
  auto* chip_net = dbChipNet::create(top_chip_, "net1");
  chip_net->addBumpInst(findBumpInst(inst1, "r1_fr", 0), {inst1});
  chip_net->addBumpInst(findBumpInst(inst2, "r2_bk", 0), {inst2});

  check();
  EXPECT_TRUE(getMarkers(open_nets_category).empty());
}

// --- Test: Two bumps on different chips, different XY → disconnected ---
TEST_F(NetConnectivityFixture, test_net_cross_chip_xy_mismatch)
{
  createBump(chip1_, chip1_->findChipRegion("r1_fr"), "b1", 100, 100);
  createBump(chip2_, chip2_->findChipRegion("r2_bk"), "b2", 800, 800);

  auto inst1 = dbChipInst::create(top_chip_, chip1_, "inst1");
  inst1->setLoc(Point3D(0, 0, 0));
  inst1->setOrient(dbOrientType3D(dbOrientType::R0, false));

  auto inst2 = dbChipInst::create(top_chip_, chip2_, "inst2");
  inst2->setLoc(Point3D(0, 0, 500));
  inst2->setOrient(dbOrientType3D(dbOrientType::R0, false));

  auto* ri1 = inst1->findChipRegionInst("r1_fr");
  auto* ri2 = inst2->findChipRegionInst("r2_bk");
  auto* conn = dbChipConn::create("c1", top_chip_, {inst1}, ri1, {inst2}, ri2);
  conn->setThickness(0);

  auto* chip_net = dbChipNet::create(top_chip_, "net1");
  chip_net->addBumpInst(findBumpInst(inst1, "r1_fr", 0), {inst1});
  chip_net->addBumpInst(findBumpInst(inst2, "r2_bk", 0), {inst2});

  check();
  auto markers = getMarkers(open_nets_category);
  EXPECT_EQ(markers.size(), 1);
}

// --- Test: Two bumps on different chips, no connection → disconnected ---
TEST_F(NetConnectivityFixture, test_net_cross_chip_no_connection)
{
  createBump(chip1_, chip1_->findChipRegion("r1_fr"), "b1", 100, 100);
  createBump(chip2_, chip2_->findChipRegion("r2_bk"), "b2", 100, 100);

  auto inst1 = dbChipInst::create(top_chip_, chip1_, "inst1");
  inst1->setLoc(Point3D(0, 0, 0));
  inst1->setOrient(dbOrientType3D(dbOrientType::R0, false));

  auto inst2 = dbChipInst::create(top_chip_, chip2_, "inst2");
  inst2->setLoc(Point3D(0, 0, 500));
  inst2->setOrient(dbOrientType3D(dbOrientType::R0, false));

  // NO connection created between the chips

  auto* chip_net = dbChipNet::create(top_chip_, "net1");
  chip_net->addBumpInst(findBumpInst(inst1, "r1_fr", 0), {inst1});
  chip_net->addBumpInst(findBumpInst(inst2, "r2_bk", 0), {inst2});

  check();
  auto markers = getMarkers(open_nets_category);
  EXPECT_EQ(markers.size(), 1);
}

// --- Test: Two bumps on same chip, no wire → disconnected ---
TEST_F(NetConnectivityFixture, test_net_same_chip_no_wire)
{
  createBump(chip1_, chip1_->findChipRegion("r1_fr"), "b1", 100, 100);
  createBump(chip1_, chip1_->findChipRegion("r1_fr"), "b2", 500, 500);

  auto inst1 = dbChipInst::create(top_chip_, chip1_, "inst1");
  inst1->setLoc(Point3D(0, 0, 0));
  inst1->setOrient(dbOrientType3D(dbOrientType::R0, false));

  // Put both bumps on a single net, but NO wire
  auto* chip_net = dbChipNet::create(top_chip_, "net1");
  chip_net->addBumpInst(findBumpInst(inst1, "r1_fr", 0), {inst1});
  chip_net->addBumpInst(findBumpInst(inst1, "r1_fr", 1), {inst1});

  check();
  auto markers = getMarkers(open_nets_category);
  EXPECT_EQ(markers.size(), 1);
}

// --- Test: Two bumps on same chip, connected via wire → connected ---
TEST_F(NetConnectivityFixture, test_net_same_chip_with_wire)
{
  auto* b1
      = createBump(chip1_, chip1_->findChipRegion("r1_fr"), "b1", 100, 100);
  auto* b2
      = createBump(chip1_, chip1_->findChipRegion("r1_fr"), "b2", 500, 500);

  // Wire them on the same net
  connectBumpsWithWire(chip1_, b1, b2);

  auto inst1 = dbChipInst::create(top_chip_, chip1_, "inst1");
  inst1->setLoc(Point3D(0, 0, 0));
  inst1->setOrient(dbOrientType3D(dbOrientType::R0, false));

  auto* chip_net = dbChipNet::create(top_chip_, "net1");
  chip_net->addBumpInst(findBumpInst(inst1, "r1_fr", 0), {inst1});
  chip_net->addBumpInst(findBumpInst(inst1, "r1_fr", 1), {inst1});

  check();
  EXPECT_TRUE(getMarkers(open_nets_category).empty());
}

// --- Test: Wandering wire outside bump bounding box ---
TEST_F(NetConnectivityFixture, test_net_same_chip_wandering_wire)
{
  auto* b1
      = createBump(chip1_, chip1_->findChipRegion("r1_fr"), "b1", 100, 100);
  auto* b2
      = createBump(chip1_, chip1_->findChipRegion("r1_fr"), "b2", 500, 500);

  dbTechLayer* layer = tech_->findLayer("layer1");
  dbNet* net = b1->getNet();
  dbBTerm* bt2 = b2->getBTerm();
  bt2->disconnect();
  bt2->connect(net);
  b2->setNet(net);

  dbWire* wire = dbWire::create(net);
  dbWireEncoder encoder;
  encoder.begin(wire);
  encoder.newPath(layer, dbWireType::ROUTED);
  // Route goes far outside the 100-500 box (to 2000, 2000)
  encoder.addPoint(100, 100);
  encoder.addPoint(2000, 100);
  encoder.addPoint(2000, 2000);
  encoder.addPoint(500, 2000);
  encoder.addPoint(500, 500);
  encoder.end();

  auto inst1 = dbChipInst::create(top_chip_, chip1_, "inst1");
  inst1->setLoc(Point3D(0, 0, 0));
  inst1->setOrient(dbOrientType3D(dbOrientType::R0, false));

  auto* chip_net = dbChipNet::create(top_chip_, "net1");
  chip_net->addBumpInst(findBumpInst(inst1, "r1_fr", 0), {inst1});
  chip_net->addBumpInst(findBumpInst(inst1, "r1_fr", 1), {inst1});

  check();
  EXPECT_TRUE(getMarkers(open_nets_category).empty());
}

// --- Test: internal_ext connection ---
TEST_F(NetConnectivityFixture, test_net_internal_ext_connection)
{
  auto r1_int = dbChipRegion::create(
      chip1_, "r1_int", dbChipRegion::Side::INTERNAL_EXT, nullptr);
  r1_int->setBox(Rect(0, 0, 2000, 2000));

  auto r2_int = dbChipRegion::create(
      chip2_, "r2_int", dbChipRegion::Side::INTERNAL_EXT, nullptr);
  r2_int->setBox(Rect(0, 0, 1500, 1500));

  createBump(chip1_, r1_int, "b1", 100, 100);
  createBump(chip2_, r2_int, "b2", 100, 100);

  auto inst1 = dbChipInst::create(top_chip_, chip1_, "inst1");
  inst1->setLoc(Point3D(0, 0, 0));
  inst1->setOrient(dbOrientType3D(dbOrientType::R0, false));

  auto inst2 = dbChipInst::create(top_chip_, chip2_, "inst2");
  inst2->setLoc(Point3D(0, 0, 0));  // overlapping in Z
  inst2->setOrient(dbOrientType3D(dbOrientType::R0, false));

  auto* ri1 = inst1->findChipRegionInst("r1_int");
  auto* ri2 = inst2->findChipRegionInst("r2_int");
  auto* conn = dbChipConn::create("c1", top_chip_, {inst1}, ri1, {inst2}, ri2);
  conn->setThickness(0);

  auto* chip_net = dbChipNet::create(top_chip_, "net1");
  chip_net->addBumpInst(findBumpInst(inst1, "r1_int", 0), {inst1});
  chip_net->addBumpInst(findBumpInst(inst2, "r2_int", 0), {inst2});

  check();
  EXPECT_TRUE(getMarkers(open_nets_category).empty());
}

// --- Test: Coincident bumps tolerance zero ---
TEST_F(NetConnectivityFixture, test_net_coincident_bumps_tolerance_zero)
{
  createBump(chip1_, chip1_->findChipRegion("r1_fr"), "b1", 100, 100);
  createBump(chip1_, chip1_->findChipRegion("r1_fr"), "b2", 100, 100);
  createBump(chip2_, chip2_->findChipRegion("r2_bk"), "b3", 100, 100);

  auto inst1 = dbChipInst::create(top_chip_, chip1_, "inst1");
  inst1->setLoc(Point3D(0, 0, 0));
  inst1->setOrient(dbOrientType3D(dbOrientType::R0, false));

  auto inst2 = dbChipInst::create(top_chip_, chip2_, "inst2");
  inst2->setLoc(Point3D(0, 0, 500));
  inst2->setOrient(dbOrientType3D(dbOrientType::R0, false));

  auto* ri1 = inst1->findChipRegionInst("r1_fr");
  auto* ri2 = inst2->findChipRegionInst("r2_bk");
  auto* conn = dbChipConn::create("c1", top_chip_, {inst1}, ri1, {inst2}, ri2);
  conn->setThickness(0);

  auto* chip_net = dbChipNet::create(top_chip_, "net1");
  chip_net->addBumpInst(findBumpInst(inst1, "r1_fr", 0), {inst1});
  chip_net->addBumpInst(findBumpInst(inst1, "r1_fr", 1), {inst1});
  chip_net->addBumpInst(findBumpInst(inst2, "r2_bk", 0), {inst2});

  check();
  EXPECT_TRUE(getMarkers(open_nets_category).empty());
}

// --- Test: Tolerance – bumps within tolerance → connected ---
TEST_F(NetConnectivityFixture, test_net_tolerance_within)
{
  // Bumps offset by 50 in X
  createBump(chip1_, chip1_->findChipRegion("r1_fr"), "b1", 100, 100);
  createBump(chip2_, chip2_->findChipRegion("r2_bk"), "b2", 150, 100);

  auto inst1 = dbChipInst::create(top_chip_, chip1_, "inst1");
  inst1->setLoc(Point3D(0, 0, 0));
  inst1->setOrient(dbOrientType3D(dbOrientType::R0, false));

  auto inst2 = dbChipInst::create(top_chip_, chip2_, "inst2");
  inst2->setLoc(Point3D(0, 0, 500));
  inst2->setOrient(dbOrientType3D(dbOrientType::R0, false));

  auto* ri1 = inst1->findChipRegionInst("r1_fr");
  auto* ri2 = inst2->findChipRegionInst("r2_bk");
  auto* conn = dbChipConn::create("c1", top_chip_, {inst1}, ri1, {inst2}, ri2);
  conn->setThickness(0);

  auto* chip_net = dbChipNet::create(top_chip_, "net1");
  chip_net->addBumpInst(findBumpInst(inst1, "r1_fr", 0), {inst1});
  chip_net->addBumpInst(findBumpInst(inst2, "r2_bk", 0), {inst2});

  // Use check() with tolerance=100 via ThreeDBlox
  db_->setTopChip(top_chip_);
  ThreeDBlox three_dblox(&logger_, db_.get());
  three_dblox.check(100);  // tolerance = 100

  auto markers = getMarkers(open_nets_category);
  EXPECT_TRUE(markers.empty());
}

// --- Test: Tolerance – bumps outside tolerance → disconnected ---
TEST_F(NetConnectivityFixture, test_net_tolerance_outside)
{
  // Bumps offset by 200 in X
  createBump(chip1_, chip1_->findChipRegion("r1_fr"), "b1", 100, 100);
  createBump(chip2_, chip2_->findChipRegion("r2_bk"), "b2", 300, 100);

  auto inst1 = dbChipInst::create(top_chip_, chip1_, "inst1");
  inst1->setLoc(Point3D(0, 0, 0));
  inst1->setOrient(dbOrientType3D(dbOrientType::R0, false));

  auto inst2 = dbChipInst::create(top_chip_, chip2_, "inst2");
  inst2->setLoc(Point3D(0, 0, 500));
  inst2->setOrient(dbOrientType3D(dbOrientType::R0, false));

  auto* ri1 = inst1->findChipRegionInst("r1_fr");
  auto* ri2 = inst2->findChipRegionInst("r2_bk");
  auto* conn = dbChipConn::create("c1", top_chip_, {inst1}, ri1, {inst2}, ri2);
  conn->setThickness(0);

  auto* chip_net = dbChipNet::create(top_chip_, "net1");
  chip_net->addBumpInst(findBumpInst(inst1, "r1_fr", 0), {inst1});
  chip_net->addBumpInst(findBumpInst(inst2, "r2_bk", 0), {inst2});

  // tolerance = 100 but offset = 200 → still disconnected
  db_->setTopChip(top_chip_);
  ThreeDBlox three_dblox(&logger_, db_.get());
  three_dblox.check(100);

  auto markers = getMarkers(open_nets_category);
  EXPECT_EQ(markers.size(), 1);
}

// --- Test: Multiple disconnected groups on one net ---
TEST_F(NetConnectivityFixture, test_net_multiple_disconnected_groups)
{
  // 4 bumps: b1,b2 on chip1, b3,b4 on chip2, all at different XY
  createBump(chip1_, chip1_->findChipRegion("r1_fr"), "b1", 100, 100);
  createBump(chip1_, chip1_->findChipRegion("r1_fr"), "b2", 500, 500);
  createBump(chip2_, chip2_->findChipRegion("r2_bk"), "b3", 800, 100);
  createBump(chip2_, chip2_->findChipRegion("r2_bk"), "b4", 800, 800);

  auto inst1 = dbChipInst::create(top_chip_, chip1_, "inst1");
  inst1->setLoc(Point3D(0, 0, 0));
  inst1->setOrient(dbOrientType3D(dbOrientType::R0, false));

  auto inst2 = dbChipInst::create(top_chip_, chip2_, "inst2");
  inst2->setLoc(Point3D(0, 0, 500));
  inst2->setOrient(dbOrientType3D(dbOrientType::R0, false));

  auto* ri1 = inst1->findChipRegionInst("r1_fr");
  auto* ri2 = inst2->findChipRegionInst("r2_bk");
  auto* conn = dbChipConn::create("c1", top_chip_, {inst1}, ri1, {inst2}, ri2);
  conn->setThickness(0);

  auto* chip_net = dbChipNet::create(top_chip_, "net1");
  chip_net->addBumpInst(findBumpInst(inst1, "r1_fr", 0), {inst1});
  chip_net->addBumpInst(findBumpInst(inst1, "r1_fr", 1), {inst1});
  chip_net->addBumpInst(findBumpInst(inst2, "r2_bk", 0), {inst2});
  chip_net->addBumpInst(findBumpInst(inst2, "r2_bk", 1), {inst2});

  check();
  auto markers = getMarkers(open_nets_category);
  // All 4 bumps at different XY, no wire → at least one marker for the net
  EXPECT_EQ(markers.size(), 1);
  if (!markers.empty()) {
    // The largest group is kept as "main", rest reported as disconnected
    auto comment = markers[0]->getComment();
    EXPECT_TRUE(comment.find("net1") != std::string::npos);
    EXPECT_TRUE(comment.find("out of 4 total") != std::string::npos);
  }
}

// --- Test: Verify marker comment format ---
TEST_F(NetConnectivityFixture, test_net_marker_comment_format)
{
  createBump(chip1_, chip1_->findChipRegion("r1_fr"), "b1", 100, 100);
  createBump(chip2_, chip2_->findChipRegion("r2_bk"), "b2", 900, 900);

  auto inst1 = dbChipInst::create(top_chip_, chip1_, "inst1");
  inst1->setLoc(Point3D(0, 0, 0));
  inst1->setOrient(dbOrientType3D(dbOrientType::R0, false));

  auto inst2 = dbChipInst::create(top_chip_, chip2_, "inst2");
  inst2->setLoc(Point3D(0, 0, 500));
  inst2->setOrient(dbOrientType3D(dbOrientType::R0, false));

  auto* ri1 = inst1->findChipRegionInst("r1_fr");
  auto* ri2 = inst2->findChipRegionInst("r2_bk");
  auto* conn = dbChipConn::create("c1", top_chip_, {inst1}, ri1, {inst2}, ri2);
  conn->setThickness(0);

  auto* chip_net = dbChipNet::create(top_chip_, "mynet");
  chip_net->addBumpInst(findBumpInst(inst1, "r1_fr", 0), {inst1});
  chip_net->addBumpInst(findBumpInst(inst2, "r2_bk", 0), {inst2});

  check();
  auto markers = getMarkers(open_nets_category);
  ASSERT_EQ(markers.size(), 1);
  EXPECT_EQ(markers[0]->getComment(),
            "Net mynet has 1 disconnected bump(s) out of 2 total.");
}

// --- Test: Multiple nets – one connected, one disconnected ---
TEST_F(NetConnectivityFixture, test_net_multiple_nets_mixed)
{
  // Net1: connected (same XY across chips)
  createBump(chip1_, chip1_->findChipRegion("r1_fr"), "b1", 100, 100);
  createBump(chip2_, chip2_->findChipRegion("r2_bk"), "b2", 100, 100);

  // Net2: disconnected (different XY)
  createBump(chip1_, chip1_->findChipRegion("r1_fr"), "b3", 200, 200);
  createBump(chip2_, chip2_->findChipRegion("r2_bk"), "b4", 700, 700);

  auto inst1 = dbChipInst::create(top_chip_, chip1_, "inst1");
  inst1->setLoc(Point3D(0, 0, 0));
  inst1->setOrient(dbOrientType3D(dbOrientType::R0, false));

  auto inst2 = dbChipInst::create(top_chip_, chip2_, "inst2");
  inst2->setLoc(Point3D(0, 0, 500));
  inst2->setOrient(dbOrientType3D(dbOrientType::R0, false));

  auto* ri1 = inst1->findChipRegionInst("r1_fr");
  auto* ri2 = inst2->findChipRegionInst("r2_bk");
  auto* conn = dbChipConn::create("c1", top_chip_, {inst1}, ri1, {inst2}, ri2);
  conn->setThickness(0);

  // Net1: both bumps at (100,100) → connected
  auto* net1 = dbChipNet::create(top_chip_, "connected_net");
  net1->addBumpInst(findBumpInst(inst1, "r1_fr", 0), {inst1});
  net1->addBumpInst(findBumpInst(inst2, "r2_bk", 0), {inst2});

  // Net2: bumps at (200,200) and (700,700) → disconnected
  auto* net2 = dbChipNet::create(top_chip_, "open_net");
  net2->addBumpInst(findBumpInst(inst1, "r1_fr", 1), {inst1});
  net2->addBumpInst(findBumpInst(inst2, "r2_bk", 1), {inst2});

  check();
  auto markers = getMarkers(open_nets_category);
  EXPECT_EQ(markers.size(), 1);
  if (!markers.empty()) {
    EXPECT_TRUE(markers[0]->getComment().find("open_net") != std::string::npos);
  }
}

// --- Test: Two coincident bumps in set_b both get united (equal_range fix) ---
// Regression for bug where unordered_multimap::find() only united the first
// matching bump in set_b. With two bumps at the same XY on chip2's region,
// all three bumps (one on chip1, two on chip2) must be in the same group.
TEST_F(NetConnectivityFixture, test_net_coincident_bumps_in_set_b)
{
  // chip1: one bump at (100, 100)
  createBump(chip1_, chip1_->findChipRegion("r1_fr"), "b1", 100, 100);
  // chip2: TWO bumps at the exact same (100, 100) position
  createBump(chip2_, chip2_->findChipRegion("r2_bk"), "b2a", 100, 100);
  createBump(chip2_, chip2_->findChipRegion("r2_bk"), "b2b", 100, 100);

  auto inst1 = dbChipInst::create(top_chip_, chip1_, "inst1");
  inst1->setLoc(Point3D(0, 0, 0));
  inst1->setOrient(dbOrientType3D(dbOrientType::R0, false));

  auto inst2 = dbChipInst::create(top_chip_, chip2_, "inst2");
  inst2->setLoc(Point3D(0, 0, 500));
  inst2->setOrient(dbOrientType3D(dbOrientType::R0, false));

  auto* ri1 = inst1->findChipRegionInst("r1_fr");
  auto* ri2 = inst2->findChipRegionInst("r2_bk");
  auto* conn = dbChipConn::create("c1", top_chip_, {inst1}, ri1, {inst2}, ri2);
  conn->setThickness(0);

  // All three bumps on the same chip-level net
  auto* chip_net = dbChipNet::create(top_chip_, "net1");
  chip_net->addBumpInst(findBumpInst(inst1, "r1_fr", 0), {inst1});
  chip_net->addBumpInst(findBumpInst(inst2, "r2_bk", 0), {inst2});
  chip_net->addBumpInst(findBumpInst(inst2, "r2_bk", 1), {inst2});

  check();
  // All three bumps should be connected — no open-net marker
  EXPECT_TRUE(getMarkers(open_nets_category).empty());
}

// --- Test: Chain connectivity A→B→C, bumps flow through ---
TEST_F(NetConnectivityFixture, test_net_chain_connectivity)
{
  // Create a third chip
  auto* chip3
      = dbChip::create(db_.get(), tech_, "Chip3", dbChip::ChipType::DIE);
  chip3->setWidth(1500);
  chip3->setHeight(1500);
  chip3->setThickness(500);
  dbBlock::create(chip3, "block3");

  auto r3_bk
      = dbChipRegion::create(chip3, "r3_bk", dbChipRegion::Side::BACK, nullptr);
  r3_bk->setBox(Rect(0, 0, 1500, 1500));

  // Bump at same XY (100,100) on all three chips
  createBump(chip1_, chip1_->findChipRegion("r1_fr"), "b1", 100, 100);
  createBump(chip2_, chip2_->findChipRegion("r2_bk"), "b2_bk", 100, 100);
  createBump(chip2_, chip2_->findChipRegion("r2_fr"), "b2_fr", 100, 100);
  createBump(chip3, chip3->findChipRegion("r3_bk"), "b3", 100, 100);

  // Connect bumps on chip2 BACK and FRONT via wire
  auto* b2_bk_bump = *chip2_->findChipRegion("r2_bk")->getChipBumps().begin();
  auto* b2_fr_bump = *chip2_->findChipRegion("r2_fr")->getChipBumps().begin();
  connectBumpsWithWire(chip2_, b2_bk_bump, b2_fr_bump);

  auto inst1 = dbChipInst::create(top_chip_, chip1_, "inst1");
  inst1->setLoc(Point3D(0, 0, 0));
  inst1->setOrient(dbOrientType3D(dbOrientType::R0, false));

  auto inst2 = dbChipInst::create(top_chip_, chip2_, "inst2");
  inst2->setLoc(Point3D(0, 0, 500));
  inst2->setOrient(dbOrientType3D(dbOrientType::R0, false));

  auto inst3 = dbChipInst::create(top_chip_, chip3, "inst3");
  inst3->setLoc(Point3D(0, 0, 1000));
  inst3->setOrient(dbOrientType3D(dbOrientType::R0, false));

  // Connection chip1.FRONT ↔ chip2.BACK
  auto* ri1_fr = inst1->findChipRegionInst("r1_fr");
  auto* ri2_bk = inst2->findChipRegionInst("r2_bk");
  auto* conn1
      = dbChipConn::create("c1", top_chip_, {inst1}, ri1_fr, {inst2}, ri2_bk);
  conn1->setThickness(0);

  // Connection chip2.FRONT ↔ chip3.BACK
  auto* ri2_fr = inst2->findChipRegionInst("r2_fr");
  auto* ri3_bk = inst3->findChipRegionInst("r3_bk");
  auto* conn2
      = dbChipConn::create("c2", top_chip_, {inst2}, ri2_fr, {inst3}, ri3_bk);
  conn2->setThickness(0);

  // Create net: b1 → b2_bk ↔(wire)↔ b2_fr → b3
  auto* chip_net = dbChipNet::create(top_chip_, "chain_net");
  chip_net->addBumpInst(findBumpInst(inst1, "r1_fr", 0), {inst1});
  chip_net->addBumpInst(findBumpInst(inst2, "r2_bk", 0), {inst2});
  chip_net->addBumpInst(findBumpInst(inst2, "r2_fr", 0), {inst2});
  chip_net->addBumpInst(findBumpInst(inst3, "r3_bk", 0), {inst3});

  check();
  EXPECT_TRUE(getMarkers(open_nets_category).empty());
}

}  // namespace

}  // namespace odb
