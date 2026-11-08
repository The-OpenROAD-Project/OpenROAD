// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include "gtest/gtest.h"
#include "odb/db.h"
#include "odb/dbTypes.h"
#include "odb/geom.h"
#include "tst/db_fixture.h"
#include "tst/physical_builder.h"

namespace odb {
namespace {

class NetAbutmentTest : public tst::DbFixture
{
 protected:
  void SetUp() override
  {
    auto* tech = tst::makeTech(getDb());
    layers_ = tst::makeLayerStack(tech, 5);
    lib_ = tst::makeLib(getDb(), tech);
    block_ = tst::makeBlock(getDb(), tech);
    net_ = dbNet::create(block_, "net");
  }

  dbInst* makeBlockPin(const char* name,
                       dbTechLayer* layer,
                       const Rect& rect = {500, 500, 1500, 1500})
  {
    auto* master = tst::makeMaster(lib_,
                                   name,
                                   2000,
                                   2000,
                                   {{.name = "P",
                                     .io_type = dbIoType::INOUT,
                                     .layer = layer,
                                     .rect = rect}},
                                   dbMasterType::BLOCK);
    return makeInst(block_,
                    master,
                    name,
                    {.location = {10000, 10000},
                     .status = dbPlacementStatus::PLACED,
                     .iterms = {{"net", "P"}}});
  }

  tst::LayerStack layers_;
  dbLib* lib_ = nullptr;
  dbBlock* block_ = nullptr;
  dbNet* net_ = nullptr;
};

TEST_F(NetAbutmentTest, OverlapOnDifferentLayersIsNotConnected)
{
  makeBlockPin("a", layers_.metal(2));
  makeBlockPin("b", layers_.metal(5));

  EXPECT_FALSE(net_->isConnectedByAbutment());
  EXPECT_FALSE(block_->designIsRouted(false));
}

TEST_F(NetAbutmentTest, OverlapOnSameLayerIsConnected)
{
  makeBlockPin("a", layers_.metal(2));
  makeBlockPin("b", layers_.metal(2));

  EXPECT_TRUE(net_->isConnectedByAbutment());
  EXPECT_TRUE(block_->designIsRouted(false));
}

TEST_F(NetAbutmentTest, TouchOnSameLayerIsConnected)
{
  makeBlockPin("a", layers_.metal(2));
  auto* b = makeBlockPin("b", layers_.metal(2));
  b->setLocation(11000, 10000);

  EXPECT_TRUE(net_->isConnectedByAbutment());
}

TEST_F(NetAbutmentTest, SeparatedPinsOnSameLayerAreNotConnected)
{
  makeBlockPin("a", layers_.metal(2));
  auto* b = makeBlockPin("b", layers_.metal(2));
  b->setLocation(25000, 10000);

  EXPECT_FALSE(net_->isConnectedByAbutment());
}

TEST_F(NetAbutmentTest, LayersAreMatchedToTheirOwnShapes)
{
  makeBlockPin("a", layers_.metal(5));
  auto* b = makeBlockPin("b", layers_.metal(2));
  auto* other_pin = dbMPin::create(b->getMaster()->findMTerm("P"));
  dbBox::create(other_pin, layers_.metal(5), 0, 0, 250, 250);

  EXPECT_FALSE(net_->isConnectedByAbutment());
}

TEST_F(NetAbutmentTest, InstanceTransformsAreApplied)
{
  makeBlockPin("a", layers_.metal(2), {0, 0, 500, 500});
  auto* b = makeBlockPin("b", layers_.metal(2), {1500, 1500, 2000, 2000});
  EXPECT_FALSE(net_->isConnectedByAbutment());

  b->setOrient(dbOrientType::R180);
  b->setLocation(10000, 10000);

  EXPECT_TRUE(net_->isConnectedByAbutment());
}

TEST_F(NetAbutmentTest, AnyOverlappingShapeOnSameLayerConnects)
{
  auto* a = makeBlockPin("a", layers_.metal(2));
  makeBlockPin("b", layers_.metal(5));
  auto* other_pin = dbMPin::create(a->getMaster()->findMTerm("P"));
  dbBox::create(other_pin, layers_.metal(5), 750, 750, 1250, 1250);

  EXPECT_TRUE(net_->isConnectedByAbutment());
}

TEST_F(NetAbutmentTest, OverlappingCutShapesDoNotConnect)
{
  makeBlockPin("a", layers_.cut(2));
  makeBlockPin("b", layers_.cut(2));

  EXPECT_FALSE(net_->isConnectedByAbutment());
}

}  // namespace
}  // namespace odb
