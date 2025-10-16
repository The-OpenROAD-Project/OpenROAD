// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023-2025, The OpenROAD Authors

#include <unistd.h>

#include <memory>

#include "gtest/gtest.h"
#include "odb/db.h"
#include "odb/dbShape.h"
#include "odb/dbWireCodec.h"
#include "odb/lefin.h"
#include "tst/fixture.h"
#include "utl/Logger.h"

namespace odb {
class OdbMultiPatternedTest : public tst::Fixture
{
 protected:
  void SetUp() override
  {
    lib_ = loadTechAndLib(
        "multipatterned",
        "multipatterned",
        "_main/src/odb/test/data/sky130hd/sky130hd_multi_patterned.tlef");

    chip_ = odb::dbChip::create(db_.get(), db_->getTech());
    block_ = odb::dbBlock::create(chip_, "top");
    block_->setDefUnits(lib_->getTech()->getLefUnits());
    block_->setDieArea(odb::Rect(0, 0, 1000, 1000));
  }

  odb::dbLib* lib_;
  odb::dbChip* chip_;
  odb::dbBlock* block_;
};

TEST_F(OdbMultiPatternedTest, NdrWidth)
{
  dbTechLayer* met1 = lib_->getTech()->findLayer("met1");
  ASSERT_NE(met1, nullptr);

  // Setup an NDR
  dbTechNonDefaultRule* ndr = dbTechNonDefaultRule::create(block_, "ndr");
  dbTechLayerRule* rule = dbTechLayerRule::create(ndr, met1);
  const int width = 42;
  rule->setWidth(width);

  // Apply it to some routing
  dbNet* net = dbNet::create(block_, "net");
  dbWire* wire = dbWire::create(net);
  dbWireEncoder encoder;
  encoder.begin(wire);
  encoder.newPath(met1, dbWireType::ROUTED, rule);
  encoder.addPoint(0, 0);
  encoder.addPoint(0, 100000);
  encoder.addPoint(100000, 100000);
  encoder.end();

  // Check the width in both directions is correct
  int shape_cnt = 0;
  odb::dbWireShapeItr it;
  it.begin(wire);
  odb::dbShape shape;
  while (it.next(shape)) {
    EXPECT_EQ(shape.getBox().minDXDY(), width);
    ++shape_cnt;
  }
  EXPECT_EQ(shape_cnt, 2);
}

TEST_F(OdbMultiPatternedTest, CanColorColoredLayer)
{
  // Arrange
  dbNet* net = dbNet::create(block_, "net0");
  dbTech* tech = lib_->getTech();
  dbTechLayer* met1 = tech->findLayer("met1");
  dbWire* wire = dbWire::create(net);

  // Wire shape
  // (50,50) ----------(color 1)-----------(100,50)

  dbWireEncoder encoder;
  encoder.begin(wire);
  encoder.newPath(met1, dbWireType::ROUTED);
  encoder.addPoint(50, 50);
  encoder.setColor(/*mask_color=*/1);
  encoder.addPoint(100, 50);
  encoder.end();

  // Act & Assert
  dbWireDecoder decoder;
  decoder.begin(wire);
  EXPECT_EQ(decoder.next(), dbWireDecoder::OpCode::PATH);
  EXPECT_FALSE(decoder.getColor().has_value());
  EXPECT_EQ(decoder.next(), dbWireDecoder::OpCode::POINT);
  EXPECT_FALSE(decoder.getColor().has_value());
  EXPECT_EQ(decoder.next(), dbWireDecoder::OpCode::POINT);
  EXPECT_TRUE(decoder.getColor().has_value());
  EXPECT_EQ(decoder.getColor().value(), /*mask_color=*/1);
}

TEST_F(OdbMultiPatternedTest, WireColorIsClearedOnNewPath)
{
  // Arrange
  dbNet* net = dbNet::create(block_, "net0");
  dbTech* tech = lib_->getTech();
  dbTechLayer* met1 = tech->findLayer("met1");
  dbTechVia* met1_met2 = tech->findVia("M1M2_PR_MR");
  dbWire* wire = dbWire::create(net);

  // Wire shape
  //                                       M2
  //                                    (100, 50)--(color 2)---(130,50)
  //                                       |
  //                                       |
  //                                   VIA=(M1M2_PR_MR)
  //                                       |
  //                      M1               |
  // (50,50) ----------(color 1)--------(100,50)(junction_1)

  dbWireEncoder encoder;
  encoder.begin(wire);
  encoder.newPath(met1, dbWireType::ROUTED);
  encoder.addPoint(50, 50);
  encoder.setColor(/*mask_color=*/1);
  int junction_1 = encoder.addPoint(100, 50);
  encoder.addTechVia(met1_met2);
  encoder.newPath(junction_1);
  encoder.setColor(/*mask_color=*/2);
  encoder.addPoint(130, 50);
  encoder.end();

  // Act & Assert
  dbWireDecoder decoder;
  decoder.begin(wire);
  EXPECT_EQ(decoder.next(), dbWireDecoder::OpCode::PATH);
  EXPECT_FALSE(decoder.getColor().has_value());
  EXPECT_EQ(decoder.next(), dbWireDecoder::OpCode::POINT);
  EXPECT_FALSE(decoder.getColor().has_value());
  EXPECT_EQ(decoder.next(), dbWireDecoder::OpCode::POINT);
  EXPECT_TRUE(decoder.getColor().has_value());
  EXPECT_EQ(decoder.getColor().value(), /*mask_color=*/1);
  EXPECT_EQ(decoder.next(), dbWireDecoder::OpCode::TECH_VIA);
  EXPECT_EQ(decoder.next(), dbWireDecoder::OpCode::JUNCTION);
  EXPECT_EQ(decoder.next(), dbWireDecoder::OpCode::POINT);
  EXPECT_FALSE(decoder.getColor().has_value());
  EXPECT_EQ(decoder.next(), dbWireDecoder::OpCode::POINT);
  EXPECT_TRUE(decoder.getColor().has_value());
  EXPECT_EQ(decoder.getColor().value(), /*mask_color=*/2);
}

}  // namespace odb
