// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

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

class PinAccessServiceTest : public tst::DbFixture
{
 protected:
  void SetUp() override
  {
    auto tech
        = loadTechLef("test", "_main/src/dpl/test/repair_pin_access_tech.lef");
    ASSERT_NE(tech, nullptr);
    auto lib = loadLibLef(
        tech, "test", "_main/src/dpl/test/repair_pin_access_failure.lef");
    ASSERT_NE(lib, nullptr);
    ASSERT_NE(
        readDef("_main/src/dpl/test/repair_pin_access_failure.def", {lib}),
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

  odb::dbMTerm* makeSignalPin(const odb::Rect& shape)
  {
    auto block = db_->getChip()->getBlock();
    auto model = block->findInst("u")->getMaster();
    auto master = odb::dbMaster::create(model->getLib(), "minimum_cell");
    master->setType(model->getType());
    master->setWidth(model->getWidth());
    master->setHeight(model->getHeight());
    master->setSite(model->getSite());
    master->setSymmetryX();
    master->setSymmetryY();
    for (const auto name : {"A", "B"}) {
      auto term = odb::dbMTerm::create(
          master, name, odb::dbIoType::INPUT, odb::dbSigType::SIGNAL);
      auto pin = odb::dbMPin::create(term);
      const odb::Rect rect
          = name[0] == 'A' ? shape : odb::Rect(750, 200, 1050, 400);
      odb::dbBox::create(pin,
                         db_->getTech()->findLayer("M1"),
                         rect.xMin(),
                         rect.yMin(),
                         rect.xMax(),
                         rect.yMax());
    }
    master->setFrozen();
    for (auto inst : block->getInsts()) {
      EXPECT_TRUE(inst->swapMaster(master));
    }
    return master->findMTerm("A");
  }

  odb::dbInst* makeUnmovedClassMember()
  {
    auto block = db_->getChip()->getBlock();
    auto unmoved
        = odb::dbInst::create(block, block->findInst("u")->getMaster(), "w");
    unmoved->setLocation(6300, 0);
    unmoved->setPlacementStatus(odb::dbPlacementStatus::PLACED);
    auto net = odb::dbNet::create(block, "nB");
    auto term = unmoved->findITerm("B");
    term->connect(net);
    auto port = odb::dbBTerm::create(net, "OUTB");
    port->setIoType(odb::dbIoType::OUTPUT);
    auto pin = odb::dbBPin::create(port);
    pin->setPlacementStatus(odb::dbPlacementStatus::PLACED);
    odb::dbBox::create(
        pin, db_->getTech()->findLayer("M2"), 7150, 200, 7250, 400);
    return unmoved;
  }

  dst::Distributed dist_{&logger_};
  utl::ServiceRegistry services_{&logger_};
  TritonRoute router_{db_.get(), &logger_, &services_, &dist_, nullptr};
};

TEST_F(PinAccessServiceTest, PartialPinAccessRemainsUnready)
{
  auto inst = db_->getChip()->getBlock()->findInst("u");
  ASSERT_NE(inst, nullptr);
  router_.pinAccess({inst});
  EXPECT_FALSE(router_.hasPinAccess());
  EXPECT_NO_THROW(router_.updateDirtyPinAccess());
  EXPECT_FALSE(router_.hasPinAccess());
  router_.pinAccess();
  EXPECT_TRUE(router_.hasPinAccess());
}

TEST_F(PinAccessServiceTest, FailedRefreshReadiness)
{
  router_.pinAccess();
  ASSERT_TRUE(router_.hasPinAccess());
  auto inst = db_->getChip()->getBlock()->findInst("u");
  ASSERT_NE(inst, nullptr);

  inst->setLocation(700, 0);
  EXPECT_LOGGER_ERROR(router_.updateDirtyPinAccess(), "DRT-0073");
  EXPECT_FALSE(router_.hasPinAccess());
  inst->setLocation(900, 0);
  EXPECT_LOGGER_ERROR(router_.updateDirtyPinAccess(), "DRT-0073");
  EXPECT_FALSE(router_.hasPinAccess());

  router_.invalidatePinAccess();
  inst->setLocation(400, 0);
  EXPECT_NO_THROW(router_.updateDirtyPinAccess());
  EXPECT_TRUE(router_.hasPinAccess());
  const auto points = inst->findITerm("A")->getPrefAccessPoints();
  ASSERT_EQ(points.size(), 1);
  EXPECT_EQ(points.front()->getPoint(), odb::Point(400, 500));

  router_.invalidatePinAccess();
  EXPECT_NO_THROW(router_.updateDirtyPinAccess());
  EXPECT_FALSE(router_.hasPinAccess());
}

TEST_F(PinAccessServiceTest, UnresolvedRecoveryAndCancellationRemainUnready)
{
  auto block = db_->getChip()->getBlock();
  auto inst = block->findInst("u");
  inst->setLocation(700, 0);
  ASSERT_NO_THROW(router_.beginPinAccessRecovery());
  ASSERT_FALSE(router_.getMissingPinAccess().empty());
  EXPECT_LOGGER_ERROR(router_.finishPinAccessRecovery(), "DRT-0633");
  EXPECT_FALSE(router_.hasPinAccess());
  EXPECT_TRUE(block->getAccessPoints().empty());

  // Restoration is safe after cancel has discarded the provisional design.
  inst->setLocation(400, 0);
  ASSERT_NO_THROW(router_.beginPinAccessRecovery());
  EXPECT_TRUE(router_.getMissingPinAccess().empty());
  router_.cancelPinAccessRecovery();
  inst->setLocation(700, 0);
  EXPECT_NO_THROW(router_.updateDirtyPinAccess());
  EXPECT_FALSE(router_.hasPinAccess());
  EXPECT_TRUE(block->getAccessPoints().empty());
}

TEST_F(PinAccessServiceTest, InvalidRecoveryMinimumPreservesPublishedAccess)
{
  ASSERT_NO_THROW(router_.pinAccess());
  ASSERT_TRUE(router_.hasPinAccess());
  auto block = db_->getChip()->getBlock();
  const auto points
      = block->findInst("u")->findITerm("A")->getPrefAccessPoints();
  const auto count = block->getAccessPoints().size();
  for (const int min_points : {0, -1}) {
    EXPECT_LOGGER_ERROR(router_.beginPinAccessRecovery(min_points), "DRT-0635");
    EXPECT_TRUE(router_.hasPinAccess());
    EXPECT_EQ(block->getAccessPoints().size(), count);
    EXPECT_EQ(block->findInst("u")->findITerm("A")->getPrefAccessPoints(),
              points);
  }
}

TEST_F(PinAccessServiceTest, RecoveryMinimumIsSessionLocal)
{
  makeSignalPin({350, 450, 450, 550});
  auto block = db_->getChip()->getBlock();
  auto cfg = router_.getRouterConfiguration();
  cfg->MINNUMACCESSPOINT_MACROCELLPIN = 7;
  ASSERT_NO_THROW(router_.beginPinAccessRecovery(2));
  EXPECT_EQ(router_.getMissingPinAccess().size(), 2);
  EXPECT_FALSE(router_.hasPinAccess());
  EXPECT_TRUE(block->getAccessPoints().empty());
  EXPECT_EQ(cfg->MINNUMACCESSPOINT_STDCELLPIN, 1);
  EXPECT_EQ(cfg->MINNUMACCESSPOINT_MACROCELLPIN, 7);
  EXPECT_LOGGER_ERROR(router_.finishPinAccessRecovery(), "DRT-0633");
  EXPECT_FALSE(router_.hasPinAccess());
  EXPECT_TRUE(block->getAccessPoints().empty());

  ASSERT_NO_THROW(router_.beginPinAccessRecovery(2));
  router_.cancelPinAccessRecovery();
  EXPECT_FALSE(router_.hasPinAccess());
  EXPECT_TRUE(block->getAccessPoints().empty());
  ASSERT_NO_THROW(router_.beginPinAccessRecovery());
  EXPECT_TRUE(router_.getMissingPinAccess().empty());
  ASSERT_NO_THROW(router_.finishPinAccessRecovery());
  EXPECT_TRUE(router_.hasPinAccess());
  EXPECT_NO_THROW(router_.updateDirtyPinAccess());
  EXPECT_EQ(cfg->MINNUMACCESSPOINT_STDCELLPIN, 1);
  EXPECT_EQ(cfg->MINNUMACCESSPOINT_MACROCELLPIN, 7);
}

TEST_F(PinAccessServiceTest, RecoveryMinimumGeneratesLaterTierAccessPoints)
{
  auto block = db_->getChip()->getBlock();
  makeSignalPin({350, 400, 450, 700});
  auto inst = block->findInst("u");
  auto term = inst->findITerm("A");
  auto cfg = router_.getRouterConfiguration();
  ASSERT_NO_THROW(router_.beginPinAccessRecovery(1));
  ASSERT_NO_THROW(router_.finishPinAccessRecovery());
  auto points = term->getAccessPoints();
  ASSERT_EQ(points.size(), 1);
  EXPECT_EQ(points.begin()->second.size(), 1);

  ASSERT_NO_THROW(router_.beginPinAccessRecovery(2));
  EXPECT_TRUE(router_.getMissingPinAccess().empty());
  ASSERT_NO_THROW(router_.finishPinAccessRecovery());
  points = term->getAccessPoints();
  ASSERT_EQ(points.size(), 1);
  EXPECT_GE(points.begin()->second.size(), 2);
  EXPECT_EQ(cfg->MINNUMACCESSPOINT_STDCELLPIN, 1);

  // Requiring a previously skipped terminal regenerates its shared class.
  inst->findITerm("B")->connect(term->getNet());
  ASSERT_NO_THROW(router_.updateDirtyPinAccess());
  EXPECT_TRUE(router_.hasPinAccess());
  points = term->getAccessPoints();
  ASSERT_EQ(points.size(), 1);
  EXPECT_EQ(points.begin()->second.size(), 1);
  EXPECT_EQ(cfg->MINNUMACCESSPOINT_STDCELLPIN, 1);
}

TEST_F(PinAccessServiceTest, RecoveryMinimumSumsMPinCatalogues)
{
  auto term = makeSignalPin({350, 450, 450, 550});
  auto pin = odb::dbMPin::create(term);
  odb::dbBox::create(pin, db_->getTech()->findLayer("M1"), 350, 650, 450, 750);
  auto block = db_->getChip()->getBlock();
  auto cfg = router_.getRouterConfiguration();
  ASSERT_NO_THROW(router_.beginPinAccessRecovery(2));
  EXPECT_TRUE(router_.getMissingPinAccess().empty());
  EXPECT_FALSE(router_.hasPinAccess());
  EXPECT_TRUE(block->getAccessPoints().empty());
  ASSERT_NO_THROW(router_.finishPinAccessRecovery());
  EXPECT_TRUE(router_.hasPinAccess());
  for (const auto name : {"u", "v"}) {
    const auto points
        = block->findInst(name)->findITerm("A")->getAccessPoints();
    ASSERT_EQ(points.size(), 2);
    for (const auto& [mpin, catalogue] : points) {
      EXPECT_EQ(catalogue.size(), 1);
    }
  }
  EXPECT_EQ(cfg->MINNUMACCESSPOINT_STDCELLPIN, 1);
  EXPECT_EQ(cfg->MINNUMACCESSPOINT_MACROCELLPIN, 1);
}

TEST_F(PinAccessServiceTest, BlockClearCancelsProvisionalRecovery)
{
  auto block = db_->getChip()->getBlock();
  auto source = block->findInst("u");
  ASSERT_NE(source, nullptr);
  auto master = source->getMaster();
  ASSERT_NO_THROW(router_.beginPinAccessRecovery());
  ASSERT_TRUE(router_.getMissingPinAccess().empty());
  source->setLocation(800, 0);
  block->clear();
  ASSERT_EQ(router_.getDesign()->getTopBlock(), nullptr);
  EXPECT_NO_THROW(router_.updateDirtyPinAccess());
  EXPECT_FALSE(router_.hasPinAccess());
  EXPECT_LOGGER_ERROR(router_.getMissingPinAccess(), "DRT-0632");

  struct Cell
  {
    const char* instance;
    const char* net;
    const char* port;
    int x;
  };
  const Cell cells[]{{"u", "n", "OUT", 400}, {"v", "n2", "OUT2", 4400}};
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
      tst::getRunfilePath("_main/src/dpl/test/repair_pin_access_failure.def")
          .c_str(),
      db_->getChip()));
  ASSERT_EQ(db_->getChip()->getBlock(), block);
  ASSERT_EQ(router_.getDesign()->getTopBlock(), nullptr);
  EXPECT_NO_THROW(router_.updateDirtyPinAccess());
  EXPECT_FALSE(router_.hasPinAccess());
  EXPECT_LOGGER_ERROR(router_.updatePinAccessRecovery(), "DRT-0634");
  EXPECT_NO_THROW(router_.cancelPinAccessRecovery());
  ASSERT_NO_THROW(router_.beginPinAccessRecovery());
  EXPECT_TRUE(router_.getMissingPinAccess().empty());
  ASSERT_NO_THROW(router_.finishPinAccessRecovery());
  EXPECT_TRUE(router_.hasPinAccess());
  for (const auto& cell : cells) {
    const auto points
        = block->findInst(cell.instance)->findITerm("A")->getPrefAccessPoints();
    ASSERT_EQ(points.size(), 1);
    EXPECT_EQ(points.front()->getPoint(), odb::Point(400, 500));
  }
}

TEST_F(PinAccessServiceTest, ProvisionalTrialsPreservePublishedCatalogues)
{
  auto block = db_->getChip()->getBlock();
  auto inst = block->findInst("u");
  auto term = inst->findITerm("A");
  ASSERT_NO_THROW(router_.pinAccess());
  const auto published_points = term->getPrefAccessPoints();
  const auto published_count = block->getAccessPoints().size();
  ASSERT_EQ(published_points.size(), 1);
  const auto published_point = published_points.front()->getPoint();

  inst->setLocation(700, 0);
  ASSERT_NO_THROW(router_.beginPinAccessRecovery());
  ASSERT_EQ(router_.getMissingPinAccess().size(), 1);
  ASSERT_EQ(term->getPrefAccessPoints(), published_points);
  EXPECT_EQ(published_points.front()->getPoint(), published_point);
  EXPECT_EQ(block->getAccessPoints().size(), published_count);
  inst->setLocation(400, 0);
  ASSERT_NO_THROW(router_.updatePinAccessRecovery());
  EXPECT_TRUE(router_.getMissingPinAccess().empty());
  ASSERT_EQ(term->getPrefAccessPoints(), published_points);
  EXPECT_EQ(published_points.front()->getPoint(), published_point);
  EXPECT_EQ(block->getAccessPoints().size(), published_count);
  ASSERT_NO_THROW(router_.finishPinAccessRecovery());
  EXPECT_TRUE(router_.hasPinAccess());
  const auto points = term->getPrefAccessPoints();
  ASSERT_EQ(points.size(), 1);
  bool preferred_in_catalogue = false;
  for (const auto& [pin, catalogue] : term->getAccessPoints()) {
    for (auto point : catalogue) {
      preferred_in_catalogue |= point == points.front();
    }
  }
  EXPECT_TRUE(preferred_in_catalogue);
}

TEST_F(PinAccessServiceTest, MissingAccessIncludesSharedClassMembers)
{
  auto block = db_->getChip()->getBlock();
  auto first = block->findInst("u");
  auto second = block->findInst("v");
  first->setLocation(700, 0);
  ASSERT_TRUE(block->getAccessPoints().empty());
  ASSERT_NO_THROW(router_.beginPinAccessRecovery());
  EXPECT_FALSE(router_.hasPinAccess());
  ASSERT_EQ(router_.getMissingPinAccess().size(), 1);
  EXPECT_EQ(router_.getMissingPinAccess().front(), first->findITerm("A"));
  EXPECT_TRUE(block->getAccessPoints().empty());

  // Reuse an existing incomplete class, then repair its members separately.
  second->setLocation(4700, 0);
  ASSERT_NO_THROW(router_.updatePinAccessRecovery());
  const auto missing = router_.getMissingPinAccess();
  ASSERT_EQ(missing.size(), 2);
  EXPECT_EQ(missing[0], first->findITerm("A"));
  EXPECT_EQ(missing[1], second->findITerm("A"));
  first->setLocation(400, 0);
  ASSERT_NO_THROW(router_.updatePinAccessRecovery());
  ASSERT_EQ(router_.getMissingPinAccess().size(), 1);
  EXPECT_EQ(router_.getMissingPinAccess().front(), second->findITerm("A"));
  second->setLocation(4400, 0);
  ASSERT_NO_THROW(router_.updatePinAccessRecovery());
  EXPECT_TRUE(router_.getMissingPinAccess().empty());
  EXPECT_FALSE(router_.hasPinAccess());
  EXPECT_TRUE(block->getAccessPoints().empty());
  ASSERT_NO_THROW(router_.finishPinAccessRecovery());
  EXPECT_TRUE(router_.hasPinAccess());
  for (auto inst : {first, second}) {
    const auto points = inst->findITerm("A")->getPrefAccessPoints();
    ASSERT_EQ(points.size(), 1);
    EXPECT_EQ(points.front()->getPoint(), odb::Point(400, 500));
  }
}

TEST_F(PinAccessServiceTest, RecoveryRestoresUnmovedClassMember)
{
  auto block = db_->getChip()->getBlock();
  auto source = block->findInst("u");
  auto unmoved = makeUnmovedClassMember();
  auto term = unmoved->findITerm("B");
  router_.pinAccess();
  ASSERT_TRUE(router_.hasPinAccess());
  const auto original = term->getPrefAccessPoints();
  ASSERT_EQ(original.size(), 1);
  const auto original_point = original.front()->getPoint();
  EXPECT_EQ(original_point, odb::Point(900, 300));

  // Joining the occupied phase class makes its previously skipped A required.
  source->setLocation(700, 0);
  EXPECT_LOGGER_ERROR(router_.updateDirtyPinAccess(), "DRT-0073");
  EXPECT_FALSE(router_.hasPinAccess());
  source->setLocation(400, 0);
  EXPECT_NO_THROW(router_.updateDirtyPinAccess());
  EXPECT_TRUE(router_.hasPinAccess());
  EXPECT_EQ(unmoved->getLocation(), odb::Point(6300, 0));
  const auto restored = term->getPrefAccessPoints();
  ASSERT_EQ(restored.size(), 1);
  EXPECT_EQ(restored.front()->getPoint(), original_point);
  const auto catalogue = term->getAccessPoints();
  ASSERT_EQ(catalogue.size(), 1);
  EXPECT_FALSE(catalogue.begin()->second.empty());
  bool preferred_in_catalogue = false;
  for (auto point : catalogue.begin()->second) {
    preferred_in_catalogue |= point == restored.front();
    EXPECT_TRUE(odb::Rect(750, 200, 1050, 400).intersects(point->getPoint()));
  }
  EXPECT_TRUE(preferred_in_catalogue);
}

TEST_F(PinAccessServiceTest, ProvisionalRecoveryRefreshesBothClassRequirements)
{
  auto block = db_->getChip()->getBlock();
  auto source = block->findInst("u");
  auto unmoved = makeUnmovedClassMember();
  auto term = unmoved->findITerm("B");

  ASSERT_NO_THROW(router_.beginPinAccessRecovery());
  EXPECT_TRUE(router_.getMissingPinAccess().empty());
  source->setLocation(700, 0);
  ASSERT_NO_THROW(router_.updatePinAccessRecovery());
  const auto missing = router_.getMissingPinAccess();
  ASSERT_EQ(missing.size(), 1);
  EXPECT_EQ(missing.front(), source->findITerm("A"));
  source->setLocation(400, 0);
  ASSERT_NO_THROW(router_.updatePinAccessRecovery());
  EXPECT_TRUE(router_.getMissingPinAccess().empty());
  ASSERT_NO_THROW(router_.finishPinAccessRecovery());

  const auto points = term->getPrefAccessPoints();
  ASSERT_EQ(points.size(), 1);
  EXPECT_EQ(points.front()->getPoint(), odb::Point(900, 300));
  const auto catalogue = term->getAccessPoints();
  ASSERT_EQ(catalogue.size(), 1);
  bool preferred_in_catalogue = false;
  for (auto point : catalogue.begin()->second) {
    preferred_in_catalogue |= point == points.front();
    EXPECT_TRUE(odb::Rect(750, 200, 1050, 400).intersects(point->getPoint()));
  }
  EXPECT_TRUE(preferred_in_catalogue);
  EXPECT_FALSE(router_.isPinAccessRequired(unmoved->findITerm("A")));
}

}  // namespace
}  // namespace drt
