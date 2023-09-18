// Copyright 2023 Google LLC
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include <unistd.h>

#include <memory>

#include "gtest/gtest.h"
#include "odb/db.h"
#include "odb/dbWireCodec.h"
#include "odb/lefin.h"
#include "utl/Logger.h"

namespace odb {
class OdbMultiPatternedTest : public ::testing::Test
{
 protected:
  template <class T>
  using OdbUniquePtr = std::unique_ptr<T, void (*)(T*)>;

  void SetUp() override
  {
    db_ = OdbUniquePtr<odb::dbDatabase>(odb::dbDatabase::create(),
                                        &odb::dbDatabase::destroy);
    odb::lefin lef_reader(
        db_.get(), &logger_, /*ignore_non_routing_layers=*/false);
    lib_ = OdbUniquePtr<odb::dbLib>(
        lef_reader.createTechAndLib(
            "multipatterned",
            "multipatterned",
            "data/sky130hd/sky130hd_multi_patterned.tlef"),
        &odb::dbLib::destroy);

    chip_ = OdbUniquePtr<odb::dbChip>(odb::dbChip::create(db_.get()),
                                      &odb::dbChip::destroy);
    block_ = OdbUniquePtr<odb::dbBlock>(
        odb::dbBlock::create(chip_.get(), "top"), &odb::dbBlock::destroy);
    block_->setDefUnits(lib_->getTech()->getLefUnits());
    block_->setDieArea(odb::Rect(0, 0, 1000, 1000));
  }

  utl::Logger logger_;
  OdbUniquePtr<odb::dbDatabase> db_{nullptr, &odb::dbDatabase::destroy};
  OdbUniquePtr<odb::dbLib> lib_{nullptr, &odb::dbLib::destroy};
  OdbUniquePtr<odb::dbChip> chip_{nullptr, &odb::dbChip::destroy};
  OdbUniquePtr<odb::dbBlock> block_{nullptr, &odb::dbBlock::destroy};
};

TEST_F(OdbMultiPatternedTest, CanColorColoredLayer)
{
  // Arrange
  dbNet* net = dbNet::create(block_.get(), "net0");
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
  dbNet* net = dbNet::create(block_.get(), "net0");
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
