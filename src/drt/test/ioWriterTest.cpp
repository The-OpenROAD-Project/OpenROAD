// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include <memory>
#include <tuple>
#include <vector>

#include "fixture.h"
#include "gtest/gtest.h"
#include "io/io.h"
#include "odb/db.h"
#include "odb/dbWireCodec.h"

namespace drt {

class IoWriterTest : public Fixture
{
 protected:
  std::unique_ptr<odb::dbDatabase, decltype(&odb::dbDatabase::destroy)> makeDb()
  {
    std::unique_ptr<odb::dbDatabase, decltype(&odb::dbDatabase::destroy)> db(
        odb::dbDatabase::create(), odb::dbDatabase::destroy);
    db->setLogger(logger.get());
    auto tech = odb::dbTech::create(db.get(), "tech");
    auto layer
        = odb::dbTechLayer::create(tech, "m1", odb::dbTechLayerType::ROUTING);
    layer->setWidth(100);
    odb::dbLib::create(db.get(), "lib", tech, ',');
    auto chip = odb::dbChip::create(db.get(), tech);
    odb::dbBlock::create(chip, "block");
    return db;
  }

  void addPath(frNet* net,
               const odb::Point& begin,
               const odb::Point& end,
               frEndStyleEnum begin_style,
               frCoord begin_ext,
               frEndStyleEnum end_style,
               frCoord end_ext)
  {
    auto path = std::make_unique<frPathSeg>();
    path->setPoints(begin, end);
    path->setLayerNum(2);
    frSegStyle style;
    style.setWidth(100);
    style.setBeginStyle(begin_style, begin_ext);
    style.setEndStyle(end_style, end_ext);
    path->setStyle(style);
    net->addShape(std::move(path));
  }

  std::vector<std::tuple<int, int, int>> points(odb::dbNet* net)
  {
    std::vector<std::tuple<int, int, int>> result;
    odb::dbWireDecoder decoder;
    decoder.begin(net->getWire());
    for (auto op = decoder.next(); op != odb::dbWireDecoder::END_DECODE;
         op = decoder.next()) {
      if (op == odb::dbWireDecoder::POINT
          || op == odb::dbWireDecoder::POINT_EXT) {
        int x = 0;
        int y = 0;
        int ext = 0;
        if (op == odb::dbWireDecoder::POINT) {
          decoder.getPoint(x, y);
        } else {
          decoder.getPoint(x, y, ext);
        }
        result.emplace_back(x, y, ext);
      }
    }
    return result;
  }
};

TEST_F(IoWriterTest, preservesAsymmetricVariableEndStyle)
{
  auto db = makeDb();
  auto block = db->getChip()->getBlock();
  auto variable_net = makeNet("variable");
  auto control_net = makeNet("control");
  auto variable_db_net = odb::dbNet::create(block, "variable");
  auto control_db_net = odb::dbNet::create(block, "control");

  addPath(variable_net,
          {10, 20},
          {110, 20},
          frcTruncateEndStyle,
          0,
          frcVariableEndStyle,
          37);
  addPath(control_net,
          {30, 40},
          {130, 40},
          frcTruncateEndStyle,
          0,
          frcExtendEndStyle,
          60);

  io::Writer writer(design.get(), logger.get());
  writer.updateDb(db.get(), router_cfg.get());

  EXPECT_EQ(
      points(variable_db_net),
      (std::vector<std::tuple<int, int, int>>{{10, 20, 0}, {110, 20, 37}}));
  EXPECT_EQ(
      points(control_db_net),
      (std::vector<std::tuple<int, int, int>>{{30, 40, 0}, {130, 40, 60}}));
}

}  // namespace drt
