// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include <cstdio>
#include <fstream>
#include <memory>
#include <vector>

#include "AbstractGraphicsFactory.h"
#include "drt-global.h"
#include "drt/TritonRoute.h"
#include "dst/Distributed.h"
#include "frDesign.h"
#include "gtest/gtest.h"
#include "odb/db.h"
#include "odb/defin.h"
#include "tst/db_fixture.h"
#include "tst/logger_checks.h"
#include "utl/ServiceRegistry.h"

namespace drt {
namespace {

class NoGraphics : public AbstractGraphicsFactory
{
 public:
  void reset(frDebugSettings*,
             frDesign*,
             odb::dbDatabase*,
             utl::Logger*,
             RouterConfiguration*) override
  {
  }
  bool guiActive() override { return false; }
  std::unique_ptr<AbstractDRGraphics> makeUniqueDRGraphics() override
  {
    return nullptr;
  }
  std::unique_ptr<AbstractTAGraphics> makeUniqueTAGraphics() override
  {
    return nullptr;
  }
  std::unique_ptr<AbstractPAGraphics> makeUniquePAGraphics() override
  {
    return nullptr;
  }
};

class IncrementalPinAccessTest : public tst::DbFixture
{
 protected:
  void SetUp() override
  {
    auto tech = loadTechLef(
        "test", "_main/src/drt/test/incremental_pin_access_tech.lef");
    ASSERT_NE(tech, nullptr);
    auto lib = loadLibLef(
        tech, "test", "_main/src/drt/test/incremental_pin_access.lef");
    ASSERT_NE(lib, nullptr);
    ASSERT_NE(readDef("_main/src/drt/test/incremental_pin_access.def", {lib}),
              nullptr);
    router_.initGraphics(std::make_unique<NoGraphics>());
    ParamStruct params;
    params.viaAccessLayer = "M1";
    params.viaInPinBottomLayer = "M1";
    params.viaInPinTopLayer = "M1";
    params.minAccessPoints = 1;
    params.verbose = 0;
    params.num_threads = 2;
    router_.setParams(params);
    router_.getRouterConfiguration()->BOTTOM_ROUTING_LAYER = 4;
    router_.getRouterConfiguration()->TOP_ROUTING_LAYER = 6;
  }

  odb::dbInst* makeNeighbor()
  {
    auto block = db_->getChip()->getBlock();
    auto power = block->findNet("VDD");
    while (!power->getSWires().empty()) {
      odb::dbSWire::destroy(*power->getSWires().begin());
    }
    auto master = db_->findLib("test")->findMaster("neighbor_cell");
    auto neighbor = odb::dbInst::create(block, master, "neighbor");
    neighbor->setLocation(400, 0);
    neighbor->setPlacementStatus(odb::dbPlacementStatus::PLACED);
    return neighbor;
  }

  dst::Distributed dist_{&logger_};
  utl::ServiceRegistry services_{&logger_};
  TritonRoute router_{db_.get(), &logger_, &services_, &dist_, nullptr};
};

TEST_F(IncrementalPinAccessTest, QueryPreservesPublishedAccess)
{
  auto inst = db_->getChip()->getBlock()->findInst("u");
  EXPECT_FALSE(router_.hasPinAccess());
  EXPECT_FALSE(router_.hasUsablePinAccess(inst, inst->getLocation()));
  ASSERT_NO_THROW(router_.pinAccess());
  ASSERT_TRUE(router_.hasPinAccess());
  auto term = inst->findITerm("A");
  const auto preferred = term->getPrefAccessPoints();
  const auto catalogue = term->getAccessPoints();
  const auto location = inst->getLocation();
  for (int repeat = 0; repeat < 2; ++repeat) {
    EXPECT_FALSE(router_.hasUsablePinAccess(inst, location));
    EXPECT_TRUE(router_.hasUsablePinAccess(inst, odb::Point(0, 0)));
    EXPECT_FALSE(router_.hasUsablePinAccess(inst, odb::Point(300, 0)));
    EXPECT_EQ(inst->getLocation(), location);
    EXPECT_EQ(term->getPrefAccessPoints(), preferred);
    EXPECT_EQ(term->getAccessPoints(), catalogue);
  }
  db_->getChip()->getBlock()->clear();
  EXPECT_FALSE(router_.hasPinAccess());
}

TEST_F(IncrementalPinAccessTest, BulkQueryMatchesScalarAndPreservesAccess)
{
  auto* block = db_->getChip()->getBlock();
  auto* healthy = block->findInst("u");
  auto* failing = block->findInst("v");
  healthy->setOrient(odb::dbOrientType::MY);
  healthy->setLocation(400, 0);
  ASSERT_NO_THROW(router_.pinAccess());
  ASSERT_TRUE(router_.hasUsablePinAccess(healthy, healthy->getLocation()));
  ASSERT_FALSE(router_.hasUsablePinAccess(failing, failing->getLocation()));
  auto* child = odb::dbBlock::create(block, "child");
  auto* foreign = odb::dbInst::create(child, healthy->getMaster(), "u");
  std::vector<odb::dbInst*> instances;
  for (int i = 0; i < 8; ++i) {
    instances.insert(instances.end(), {healthy, failing, nullptr, foreign});
  }
  std::vector<odb::dbInst*> expected;
  for (auto* inst : instances) {
    if (!router_.hasUsablePinAccess(
            inst, inst ? inst->getLocation() : odb::Point())) {
      expected.push_back(inst);
    }
  }
  auto* healthy_term = healthy->findITerm("A");
  auto* failing_term = failing->findITerm("A");
  const auto healthy_preferred = healthy_term->getPrefAccessPoints();
  const auto healthy_catalogue = healthy_term->getAccessPoints();
  const auto failing_preferred = failing_term->getPrefAccessPoints();
  const auto failing_catalogue = failing_term->getAccessPoints();
  for (const int threads : {1, 2, 16}) {
    router_.getRouterConfiguration()->MAX_THREADS = threads;
    for (int repeat = 0; repeat < 2; ++repeat) {
      EXPECT_EQ(router_.findPinAccessFailures(instances), expected);
      EXPECT_TRUE(router_.findPinAccessFailures({}).empty());
      EXPECT_TRUE(router_.hasPinAccess());
      EXPECT_EQ(healthy->getLocation(), odb::Point(400, 0));
      EXPECT_EQ(healthy->getOrient(), odb::dbOrientType::MY);
      EXPECT_EQ(failing->getLocation(), odb::Point(4400, 0));
      EXPECT_EQ(healthy_term->getPrefAccessPoints(), healthy_preferred);
      EXPECT_EQ(healthy_term->getAccessPoints(), healthy_catalogue);
      EXPECT_EQ(failing_term->getPrefAccessPoints(), failing_preferred);
      EXPECT_EQ(failing_term->getAccessPoints(), failing_catalogue);
      EXPECT_TRUE(router_.hasUsablePinAccess(healthy, healthy->getLocation()));
      EXPECT_FALSE(router_.hasUsablePinAccess(failing, failing->getLocation()));
    }
  }
}

TEST_F(IncrementalPinAccessTest, BulkQueryRejectsUnavailablePinAccess)
{
  auto* block = db_->getChip()->getBlock();
  auto* inst = block->findInst("u");
  const std::vector<odb::dbInst*> instances{inst, nullptr, inst};
  EXPECT_EQ(router_.findPinAccessFailures(instances), instances);
  EXPECT_TRUE(router_.findPinAccessFailures({}).empty());
  ASSERT_NO_THROW(router_.pinAccess());
  EXPECT_LOGGER_ERROR(router_.main(), "DRT-0047");
  EXPECT_EQ(router_.findPinAccessFailures(instances), instances);
}

TEST_F(IncrementalPinAccessTest, QueryRejectsSupplyOverlapOnAlternativeMPin)
{
  auto block = db_->getChip()->getBlock();
  auto inst = block->findInst("u");
  auto power = block->findNet("VDD");
  while (!power->getSWires().empty()) {
    odb::dbSWire::destroy(*power->getSWires().begin());
  }
  auto term = inst->findITerm("A");
  auto pin = odb::dbMPin::create(term->getMTerm());
  auto layer = block->getTech()->findLayer("M1");
  odb::dbBox::create(pin, layer, 750, 600, 850, 800);
  auto wire = odb::dbSWire::create(power, odb::dbWireType::ROUTED);
  ASSERT_NE(odb::dbSBox::create(
                wire, layer, 750, 680, 850, 720, odb::dbWireShapeType::STRIPE),
            nullptr);
  ASSERT_NO_THROW(router_.pinAccess());
  const auto preferred = term->getPrefAccessPoints();
  const auto catalogue = term->getAccessPoints();
  EXPECT_TRUE(router_.hasUsablePinAccess(inst, inst->getLocation()));
  EXPECT_FALSE(router_.hasUsablePinAccess(inst, odb::Point(0, 0)));
  EXPECT_EQ(inst->getLocation(), odb::Point(400, 0));
  EXPECT_EQ(term->getPrefAccessPoints(), preferred);
  EXPECT_EQ(term->getAccessPoints(), catalogue);
}

TEST_F(IncrementalPinAccessTest, QueryChecksPDNCuts)
{
  auto block = db_->getChip()->getBlock();
  auto power = block->findNet("VDD");
  while (!power->getSWires().empty()) {
    odb::dbSWire::destroy(*power->getSWires().begin());
  }
  auto wire = odb::dbSWire::create(power, odb::dbWireType::ROUTED);
  ASSERT_NE(odb::dbSBox::create(wire,
                                block->getTech()->findLayer("V1"),
                                780,
                                480,
                                820,
                                520,
                                odb::dbWireShapeType::STRIPE),
            nullptr);
  ASSERT_NO_THROW(router_.pinAccess());
  auto inst = block->findInst("u");
  EXPECT_FALSE(router_.hasUsablePinAccess(inst, inst->getLocation()));
  EXPECT_TRUE(router_.hasUsablePinAccess(inst, odb::Point(0, 0)));
}

TEST_F(IncrementalPinAccessTest, QueryChecksFirstHopSegments)
{
  auto block = db_->getChip()->getBlock();
  auto power = block->findNet("VDD");
  while (!power->getSWires().empty()) {
    odb::dbSWire::destroy(*power->getSWires().begin());
  }
  auto wire = odb::dbSWire::create(power, odb::dbWireType::ROUTED);
  const odb::Rect obstacles[]{{550, 450, 650, 550},
                              {950, 450, 1050, 550},
                              {750, 250, 850, 350},
                              {750, 650, 850, 750}};
  for (const auto& box : obstacles) {
    ASSERT_NE(odb::dbSBox::create(wire,
                                  block->getTech()->findLayer("M2"),
                                  box.xMin(),
                                  box.yMin(),
                                  box.xMax(),
                                  box.yMax(),
                                  odb::dbWireShapeType::STRIPE),
              nullptr);
  }
  ASSERT_NO_THROW(router_.pinAccess());
  auto inst = block->findInst("u");
  EXPECT_FALSE(router_.hasUsablePinAccess(inst, inst->getLocation()));
  EXPECT_TRUE(router_.hasUsablePinAccess(inst, odb::Point(0, 0)));
}

TEST_F(IncrementalPinAccessTest, QueryUsesMirroredLowerLeft)
{
  auto inst = db_->getChip()->getBlock()->findInst("u");
  inst->setOrient(odb::dbOrientType::MY);
  inst->setLocation(400, 0);
  ASSERT_NO_THROW(router_.pinAccess());
  EXPECT_TRUE(router_.hasUsablePinAccess(inst, odb::Point(400, 0)));
  EXPECT_FALSE(router_.hasUsablePinAccess(inst, odb::Point(0, 0)));
  EXPECT_EQ(inst->getLocation(), odb::Point(400, 0));
  EXPECT_EQ(inst->getOrient(), odb::dbOrientType::MY);
}

TEST_F(IncrementalPinAccessTest, QueryUsesAlternativeMPin)
{
  auto inst = db_->getChip()->getBlock()->findInst("u");
  auto pin = odb::dbMPin::create(inst->findITerm("A")->getMTerm());
  odb::dbBox::create(pin,
                     inst->getMaster()->getLib()->getTech()->findLayer("M1"),
                     750,
                     600,
                     850,
                     800);
  ASSERT_NO_THROW(router_.pinAccess());
  EXPECT_TRUE(router_.hasUsablePinAccess(inst, inst->getLocation()));
}

TEST_F(IncrementalPinAccessTest, QueryMovesOwnObstructions)
{
  auto inst = db_->getChip()->getBlock()->findInst("u");
  auto master = inst->getMaster();
  odb::dbBox::create(
      master, master->getLib()->getTech()->findLayer("M2"), -50, 400, 50, 600);
  ASSERT_NO_THROW(router_.pinAccess());
  EXPECT_TRUE(router_.hasUsablePinAccess(inst, odb::Point(0, 0)));
}

TEST_F(IncrementalPinAccessTest, QueryChecksNeighborPins)
{
  auto block = db_->getChip()->getBlock();
  auto inst = block->findInst("u");
  auto neighbor = makeNeighbor();
  ASSERT_NO_THROW(router_.pinAccess());
  neighbor->findITerm("A")->connect(block->findNet("n2"));
  EXPECT_FALSE(router_.hasUsablePinAccess(inst, inst->getLocation()));
  EXPECT_TRUE(router_.hasUsablePinAccess(inst, odb::Point(0, 0)));
}

TEST_F(IncrementalPinAccessTest, QueryAcceptsSameNetNeighbor)
{
  auto block = db_->getChip()->getBlock();
  auto inst = block->findInst("u");
  auto neighbor = makeNeighbor();
  ASSERT_NO_THROW(router_.pinAccess());
  neighbor->findITerm("A")->connect(block->findNet("n"));
  EXPECT_TRUE(router_.hasUsablePinAccess(inst, inst->getLocation()));
}

TEST_F(IncrementalPinAccessTest, ReinitializationInvalidatesReadiness)
{
  ASSERT_NO_THROW(router_.pinAccess());
  ASSERT_TRUE(router_.hasPinAccess());
  EXPECT_LOGGER_ERROR(router_.main(), "DRT-0047");
  EXPECT_FALSE(router_.hasPinAccess());
  auto inst = db_->getChip()->getBlock()->findInst("u");
  EXPECT_FALSE(router_.hasUsablePinAccess(inst, odb::Point(0, 0)));
}

TEST_F(IncrementalPinAccessTest, FailedRefreshRebuildsOnRetry)
{
  auto block = db_->getChip()->getBlock();
  auto inst = block->findInst("u");
  auto unmoved = block->findInst("v");
  auto term = unmoved->findITerm("B");
  // B precedes A in the LEF to exercise partial regeneration.
  unmoved->findITerm("A")->disconnect();
  term->connect(block->findNet("n2"));
  unmoved->setLocation(6300, 0);
  ASSERT_NO_THROW(router_.pinAccess());
  inst->setLocation(700, 0);
  EXPECT_LOGGER_ERROR(router_.updateDirtyPAData(), "DRT-0073");
  inst->setLocation(900, 0);
  EXPECT_LOGGER_ERROR(router_.updateDirtyPAData(), "DRT-0073");
  inst->setLocation(400, 0);
  ASSERT_NO_THROW(router_.updateDirtyPAData());
  const auto points = inst->findITerm("A")->getPrefAccessPoints();
  ASSERT_EQ(points.size(), 1);
  EXPECT_EQ(points.front()->getPoint(), odb::Point(400, 500));
  EXPECT_EQ(unmoved->getLocation(), odb::Point(6300, 0));
  const auto preferred = term->getPrefAccessPoints();
  ASSERT_EQ(preferred.size(), 1);
  EXPECT_EQ(preferred.front()->getPoint(), odb::Point(900, 300));
  const auto catalogue = term->getAccessPoints();
  ASSERT_EQ(catalogue.size(), 1);
  ASSERT_FALSE(catalogue.begin()->second.empty());
  bool preferred_in_catalogue = false;
  for (auto point : catalogue.begin()->second) {
    preferred_in_catalogue |= point == preferred.front();
    EXPECT_TRUE(odb::Rect(750, 200, 1050, 400).intersects(point->getPoint()));
  }
  EXPECT_TRUE(preferred_in_catalogue);
}

TEST_F(IncrementalPinAccessTest, RefreshPreservesUnmovedClassMember)
{
  auto block = db_->getChip()->getBlock();
  auto unmoved = block->findInst("v");
  auto term = unmoved->findITerm("A");
  ASSERT_NO_THROW(router_.pinAccess());
  block->findInst("u")->findITerm("B")->connect(block->findNet("n2"));
  ASSERT_NO_THROW(router_.updateDirtyPAData());
  EXPECT_EQ(unmoved->getLocation(), odb::Point(4400, 0));
  const auto preferred = term->getPrefAccessPoints();
  ASSERT_EQ(preferred.size(), 1);
  EXPECT_EQ(preferred.front()->getPoint(), odb::Point(400, 500));
  const auto catalogue = term->getAccessPoints();
  ASSERT_EQ(catalogue.size(), 1);
  ASSERT_FALSE(catalogue.begin()->second.empty());
  bool preferred_in_catalogue = false;
  for (auto point : catalogue.begin()->second) {
    preferred_in_catalogue |= point == preferred.front();
    EXPECT_TRUE(odb::Rect(350, 400, 450, 600).intersects(point->getPoint()));
  }
  EXPECT_TRUE(preferred_in_catalogue);
}

TEST_F(IncrementalPinAccessTest, ResetDbDiscardsPinAccess)
{
  ASSERT_TRUE(db_->getChip()->getBlock()->getAccessPoints().empty());
  const auto odb_path = testing::TempDir() + "incremental_pin_access.odb";
  {
    std::ofstream stream(odb_path, std::ios::binary);
    db_->write(stream);
  }
  ASSERT_NO_THROW(router_.pinAccess());
  odb::dbChip::destroy(db_->getChip());
  ASSERT_NO_THROW(router_.resetDb(odb_path.c_str()));
  std::remove(odb_path.c_str());
  auto block = db_->getChip()->getBlock();
  ASSERT_TRUE(block->getAccessPoints().empty());
  auto inst = block->findInst("u");
  inst->setLocation(800, 0);
  ASSERT_NO_THROW(router_.updateDirtyPinAccess());
  EXPECT_TRUE(block->getAccessPoints().empty());
  ASSERT_NO_THROW(router_.pinAccess());
  const auto points = inst->findITerm("A")->getPrefAccessPoints();
  ASSERT_EQ(points.size(), 1);
  EXPECT_EQ(points.front()->getPoint(), odb::Point(400, 500));
}

TEST_F(IncrementalPinAccessTest, BlockClearDiscardsDesign)
{
  auto block = db_->getChip()->getBlock();
  auto master = block->findInst("u")->getMaster();
  ASSERT_NO_THROW(router_.pinAccess());
  block->findInst("u")->setLocation(800, 0);
  block->clear();
  ASSERT_EQ(router_.getDesign()->getTopBlock(), nullptr);
  EXPECT_NO_THROW(router_.updateDirtyPinAccess());

  struct Cell
  {
    const char* instance;
    const char* net;
    const char* port;
  };
  const Cell cells[]{{"u", "n", "OUT"}, {"v", "n2", "OUT2"}};
  // FLOORPLAN updates existing objects; recreate the cleared connections first.
  for (const auto& cell : cells) {
    auto inst = odb::dbInst::create(block, master, cell.instance);
    auto net = odb::dbNet::create(block, cell.net);
    inst->findITerm("A")->connect(net);
    odb::dbBTerm::create(net, cell.port);
  }
  auto power = odb::dbNet::create(block, "VDD");
  power->setSigType(odb::dbSigType::POWER);
  power->setSpecial();
  std::vector<odb::dbLib*> libs{db_->findLib("test")};
  odb::defin def(db_.get(), &logger_, odb::defin::FLOORPLAN);
  ASSERT_TRUE(def.readChip(
      libs,
      getFilePath("_main/src/drt/test/incremental_pin_access.def").c_str(),
      db_->getChip()));
  ASSERT_EQ(router_.getDesign()->getTopBlock(), nullptr);
  ASSERT_NO_THROW(router_.pinAccess());
  for (const auto& cell : cells) {
    const auto points
        = block->findInst(cell.instance)->findITerm("A")->getPrefAccessPoints();
    ASSERT_EQ(points.size(), 1);
    EXPECT_EQ(points.front()->getPoint(), odb::Point(400, 500));
  }
  block->clear();
  EXPECT_EQ(router_.getDesign()->getTopBlock(), nullptr);
}

}  // namespace
}  // namespace drt
