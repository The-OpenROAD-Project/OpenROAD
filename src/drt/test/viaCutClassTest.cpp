// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include <memory>

#include "db/tech/frLayer.h"
#include "db/tech/frTechObject.h"
#include "db/tech/frViaDef.h"
#include "drt-global.h"
#include "frDesign.h"
#include "gtest/gtest.h"
#include "io/io.h"
#include "odb/db.h"
#include "odb/dbTypes.h"
#include "odb/geom.h"
#include "utl/Logger.h"

namespace drt {
namespace {

class ViaCutClassTest : public testing::Test
{
 protected:
  ViaCutClassTest()
      : db_(odb::dbDatabase::create(), odb::dbDatabase::destroy),
        design_(&logger_, &router_cfg_)
  {
    tech_ = odb::dbTech::create(db_.get(), "tech");
    db_->setDbuPerMicron(1000);
    tech_->setManufacturingGrid(1);
    auto* lower
        = odb::dbTechLayer::create(tech_, "M1", odb::dbTechLayerType::ROUTING);
    auto* cut
        = odb::dbTechLayer::create(tech_, "V1", odb::dbTechLayerType::CUT);
    auto* upper
        = odb::dbTechLayer::create(tech_, "M2", odb::dbTechLayerType::ROUTING);
    for (auto* layer : {lower, upper}) {
      layer->setWidth(100);
      layer->setPitch(200);
    }
    lower->setDirection(odb::dbTechLayerDir::HORIZONTAL);
    upper->setDirection(odb::dbTechLayerDir::VERTICAL);
    auto* cut_class = odb::dbTechLayerCutClassRule::create(cut, "RECT");
    cut_class->setWidth(20);
    cut_class->setLength(40);
    cut_class->setLengthValid(true);
    auto* chip = odb::dbChip::create(db_.get(), tech_);
    block_ = odb::dbBlock::create(chip, "top");
    block_->setMinRoutingLayer(1);
  }

  template <class Via>
  void addGeometry(Via* via, int cut_width = 40)
  {
    odb::dbBox::create(via, tech_->findLayer("M1"), -50, -100, 50, 100);
    odb::dbBox::create(via, tech_->findLayer("V1"), 0, 0, cut_width, 20);
    odb::dbBox::create(via, tech_->findLayer("M2"), -100, -50, 100, 50);
  }

  void loadTech()
  {
    io::Parser parser(db_.get(), &design_, &logger_, &router_cfg_);
    parser.readTechAndLibs(db_.get());
  }

  std::unique_ptr<odb::dbDatabase, decltype(&odb::dbDatabase::destroy)> db_;
  utl::Logger logger_;
  RouterConfiguration router_cfg_;
  frDesign design_;
  odb::dbTech* tech_;
  odb::dbBlock* block_;
};

TEST_F(ViaCutClassTest, BoxViaUsesIndividualNormalizedCutDimensions)
{
  auto* single = odb::dbTechVia::create(tech_, "SINGLE12");
  addGeometry(single);
  auto* via = odb::dbVia::create(block_, "BOX12");
  addGeometry(via);
  odb::dbBox::create(via, tech_->findLayer("V1"), 80, 0, 120, 20);
  loadTech();

  const auto* imported = design_.getTech()->getVia("BOX12");
  ASSERT_NE(imported, nullptr);
  EXPECT_EQ(imported->getCutClassIdx(), 0);
  EXPECT_EQ(imported->getCutClass(),
            design_.getTech()->getLayer("V1")->getCutClass(0));
  ASSERT_EQ(imported->getCutFigs().size(), 2);
  EXPECT_EQ(imported->getCutShapeBox(), odb::Rect(0, 0, 120, 20));
  for (const auto& cut : imported->getCutFigs()) {
    EXPECT_EQ(cut->getBBox().dx(), 40);
    EXPECT_EQ(cut->getBBox().dy(), 20);
  }
}

TEST_F(ViaCutClassTest, UnknownBoxViaHasNoCutClass)
{
  auto* unknown = odb::dbVia::create(block_, "UNKNOWN12");
  addGeometry(unknown, 60);
  loadTech();

  const auto* unclassified = design_.getTech()->getVia("UNKNOWN12");
  ASSERT_NE(unclassified, nullptr);
  EXPECT_FALSE(unclassified->hasCutClass());
  EXPECT_EQ(unclassified->getCutClassIdx(), -1);
  EXPECT_EQ(unclassified->getCutShapeBox(), odb::Rect(0, 0, 60, 20));
}

TEST_F(ViaCutClassTest, GeneratedDefaultViaPreservesSourceCutClass)
{
  auto* via = odb::dbTechVia::create(tech_, "TECH12");
  addGeometry(via);
  loadTech();

  const auto* source = design_.getTech()->getVia("TECH12");
  ASSERT_NE(source, nullptr);
  ASSERT_TRUE(source->hasCutClass());
  const auto* generated = design_.getTech()->getLayer("V1")->getDefaultViaDef();
  ASSERT_NE(generated, nullptr);
  EXPECT_NE(generated, source);
  EXPECT_TRUE(generated->isAddedByRouter());
  EXPECT_EQ(generated->getCutClass(), source->getCutClass());
  EXPECT_EQ(generated->getCutClassIdx(), source->getCutClassIdx());
  EXPECT_EQ(generated->getCutShapeBox(), source->getCutShapeBox());
  EXPECT_EQ(generated->getLayer1ShapeBox(), odb::Rect(-100, -50, 100, 50));
  EXPECT_EQ(generated->getLayer2ShapeBox(), odb::Rect(-50, -100, 50, 100));
}

}  // namespace
}  // namespace drt
