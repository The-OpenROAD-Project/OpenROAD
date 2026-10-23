// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "boost/json/array.hpp"
#include "boost/json/object.hpp"
#include "boost/json/parse.hpp"
#include "boost/json/serialize.hpp"
#include "boost/json/value.hpp"
#include "gtest/gtest.h"
#include "odb/db.h"
#include "odb/dbTypes.h"
#include "odb/geom.h"
#include "request_handler.h"
#include "tile_generator.h"
#include "tst/nangate45_fixture.h"

namespace web {
namespace {

class Interconnect3DTest : public tst::Nangate45Fixture
{
 protected:
  void SetUp() override
  {
    die_ = getDb()->getChip();
    die_->setWidth(100000);
    die_->setHeight(80000);
    die_->setThickness(10000);
    auto* master = odb::dbMaster::create(lib_, "BUMP");
    master->setType(odb::dbMasterType::COVER_BUMP);
    master->setWidth(400);
    master->setHeight(600);
    master->setFrozen();
    const std::vector<odb::dbChipRegion::Side> sides{
        odb::dbChipRegion::Side::FRONT,
        odb::dbChipRegion::Side::BACK,
        odb::dbChipRegion::Side::INTERNAL};
    for (int i = 0; i < 4; ++i) {
      auto* region = odb::dbChipRegion::create(
          die_, "r" + std::to_string(i), sides[i % 3], nullptr);
      auto* inst = odb::dbInst::create(
          block_, master, ("b" + std::to_string(i)).c_str());
      inst->setLocation(1000 + i * 4000, 2000 + i * 4000);
      if (i < 3) {
        inst->setPlacementStatus(odb::dbPlacementStatus::PLACED);
      }
      ASSERT_NE(odb::dbChipBump::create(region, inst), nullptr);
    }
    group_ = odb::dbChip::create(
        getDb(), nullptr, "group", odb::dbChip::ChipType::HIER);
    auto* left = odb::dbChipInst::create(group_, die_, "left");
    left->setOrient(odb::dbOrientType3D("R90"));
    left->setLoc(odb::Point3D(10000, 20000, 30000));
    auto* right = odb::dbChipInst::create(group_, die_, "right");
    right->setOrient(odb::dbOrientType3D("MZ_MX_R90"));
    right->setLoc(odb::Point3D(200000, 50000, 40000));
    auto* net = odb::dbChipNet::create(group_, "bus");
    auto bump = [](odb::dbChipInst* chip, const char* region) {
      return *chip->findChipRegionInst(region)->getChipBumpInsts().begin();
    };
    net->addBumpInst(bump(left, "r0"), {left});
    net->addBumpInst(bump(right, "r0"), {right});
    net->addBumpInst(bump(left, "r0"), {left});  // duplicate terminal
    net->addBumpInst(bump(left, "r3"), {left});  // unplaced terminal
    root_ = odb::dbChip::create(
        getDb(), nullptr, "stack", odb::dbChip::ChipType::HIER);
    first_ = odb::dbChipInst::create(root_, group_, "g0");
    first_->setLoc(odb::Point3D(400000, 600000, 100000));
    auto* second = odb::dbChipInst::create(root_, group_, "g1");
    second->setOrient(odb::dbOrientType3D("MZ_MY_R90"));
    second->setLoc(odb::Point3D(1000000, 200000, 200000));
    getDb()->setTopChip(root_);
    getDb()->constructUnfoldedModel();
    gen_ = std::make_shared<TileGenerator>(getDb(), nullptr, getLogger());
    evaluator_ = std::make_shared<TclEvaluator>(nullptr, getLogger());
    handler_ = std::make_unique<SelectHandler>(gen_, evaluator_);
  }

  boost::json::object data()
  {
    WebSocketRequest request;
    request.type = WebSocketRequest::kGet3DData;
    const auto response = handler_->handleGet3DData(request);
    EXPECT_EQ(response.type, WebSocketResponse::kJson);
    return boost::json::parse(
               std::string(response.payload.begin(), response.payload.end()))
        .as_object();
  }

  void writeFixture(boost::json::object result, const char* filename)
  {
    const char* directory = std::getenv("TEST_UNDECLARED_OUTPUTS_DIR");
    if (directory == nullptr) {
      return;
    }
    result["dbu_per_micron"] = getDb()->getDbuPerMicron();
    result["bootstrap_tech"] = serializeTechResponse(*gen_);
    ASSERT_FALSE(result.at("bootstrap_tech")
                     .as_object()
                     .at("layers")
                     .as_array()
                     .empty());
    std::ofstream file(std::filesystem::path(directory) / filename);
    ASSERT_TRUE(file.is_open());
    file << boost::json::serialize(result);
    file.close();
    ASSERT_TRUE(file.good());
  }

  void expectUnfoldedGeometry(const boost::json::object& result)
  {
    const auto& chips = result.at("chiplets").as_array();
    const auto& bumps = result.at("bumps").as_array();
    size_t checked = 0;
    for (auto* unfolded : getDb()->getUnfoldedChipBumpInsts()) {
      auto* inst = unfolded->getChipBumpInst()->getChipBump()->getInst();
      if (!inst->isPlaced()) {
        continue;
      }
      auto* parent = unfolded->getParentRegion()->getParentChip();
      std::string path = root_->getName();
      for (auto* chip : parent->getChipInstPath()) {
        path += "/" + chip->getName();
      }
      const auto chip
          = std::find_if(chips.begin(), chips.end(), [&](const auto& c) {
              return c.as_object().at("name").as_string() == path;
            });
      ASSERT_NE(chip, chips.end());
      const auto index = std::distance(chips.begin(), chip);
      auto box = inst->getBBox()->getBox();
      parent->getTransform().apply(box);
      const auto position = unfolded->getGlobalPosition();
      const boost::json::array expected{
          index, box.xMin(), box.yMin(), box.xMax(), box.yMax(), position.z()};
      EXPECT_NE(
          std::find(bumps.begin(), bumps.end(), boost::json::value(expected)),
          bumps.end());
      ++checked;
    }
    EXPECT_EQ(checked, bumps.size());
    EXPECT_GT(checked, 0);
  }

  odb::dbChip* die_ = nullptr;
  odb::dbChip* group_ = nullptr;
  odb::dbChip* root_ = nullptr;
  odb::dbChipInst* first_ = nullptr;
  std::shared_ptr<TileGenerator> gen_;
  std::shared_ptr<TclEvaluator> evaluator_;
  std::unique_ptr<SelectHandler> handler_;
};

TEST_F(Interconnect3DTest, MatchesODBGeometryAcrossRepeatedRotatedFlippedChips)
{
  const auto result = data();
  const auto& chips = result.at("chiplets").as_array();
  const auto& bumps = result.at("bumps").as_array();
  const auto& nets = result.at("nets").as_array();
  ASSERT_EQ(chips.size(), 4);
  ASSERT_EQ(bumps.size(), 12);  // three placed bumps per die occurrence
  ASSERT_EQ(nets.size(), 2);    // one net per group occurrence
  expectUnfoldedGeometry(result);
  writeFixture(result, "interconnect3d-fixture.json");
  for (const auto& value : nets) {
    const auto& net = value.as_object();
    const auto& endpoints = net.at("bumps").as_array();
    ASSERT_EQ(endpoints.size(), 2);  // deduplicated, no unplaced terminal
    const std::string name(net.at("name").as_string());
    const auto prefix = name.substr(0, name.rfind('/'));
    for (const auto& endpoint : endpoints) {
      const auto& bump = bumps[endpoint.to_number<size_t>()].as_array();
      const auto& chip = chips[bump[0].to_number<size_t>()].as_object();
      EXPECT_EQ(std::string(chip.at("name").as_string()).find(prefix + "/"), 0);
    }
  }
}

TEST_F(Interconnect3DTest, PlacementEditsDoNotUseStaleUnfoldedBumps)
{
  const auto before = data();
  const auto old_location = first_->getLoc();
  first_->setLoc(odb::Point3D(old_location.x() + 12345,
                              old_location.y() - 23456,
                              old_location.z() + 1234));
  const auto after = data();
  const auto& chips = before.at("chiplets").as_array();
  const auto& old_bumps = before.at("bumps").as_array();
  const auto& new_bumps = after.at("bumps").as_array();
  ASSERT_EQ(old_bumps.size(), 12);
  ASSERT_EQ(new_bumps.size(), old_bumps.size());
  int moved = 0;
  for (size_t i = 0; i < old_bumps.size(); ++i) {
    const auto& a = old_bumps[i].as_array();
    const auto& b = new_bumps[i].as_array();
    const auto& chip = chips[a[0].to_number<size_t>()].as_object();
    if (std::string(chip.at("name").as_string()).starts_with("stack/g0/")) {
      EXPECT_EQ(b[1].to_number<int>() - a[1].to_number<int>(), 12345);
      EXPECT_EQ(b[2].to_number<int>() - a[2].to_number<int>(), -23456);
      EXPECT_EQ(b[5].to_number<int>() - a[5].to_number<int>(), 1234);
      ++moved;
    } else {
      EXPECT_EQ(a, b);
    }
  }
  EXPECT_EQ(moved, 6);  // must exercise actual changed geometry
  EXPECT_EQ(after.at("nets"), before.at("nets"));
}

TEST_F(Interconnect3DTest, LargeRepeatedHierarchyHasDistinctGeometryAndLinks)
{
  ASSERT_EQ(data().at("chiplets").as_array().size(), 4);
  for (int i = 2; i < 128; ++i) {
    auto* instance
        = odb::dbChipInst::create(root_, group_, "g" + std::to_string(i));
    instance->setOrient(odb::dbOrientType3D(i % 2 ? "MZ_R90" : "MY"));
    instance->setLoc(odb::Point3D(i * 400000, i * 100000, i * 10000));
  }
  const auto result = data();
  EXPECT_EQ(result.at("chiplets").as_array().size(), 256);
  EXPECT_EQ(result.at("bumps").as_array().size(), 768);
  EXPECT_EQ(result.at("nets").as_array().size(), 128);
}

TEST_F(Interconnect3DTest, RootNetSpansRepeatedGroupOccurrences)
{
  ASSERT_EQ(data().at("nets").as_array().size(), 2);
  auto* left = group_->findChipInst("left");
  auto* right = group_->findChipInst("right");
  auto* second = root_->findChipInst("g1");
  auto* net = odb::dbChipNet::create(root_, "root_bus");
  for (auto* group : {first_, second}) {
    for (auto* leaf : {left, right}) {
      auto* bump = *leaf->findChipRegionInst("r0")->getChipBumpInsts().begin();
      net->addBumpInst(bump, {group, leaf});
      net->addBumpInst(bump, {group, leaf});
    }
  }
  const auto result = data();
  const auto& nets = result.at("nets").as_array();
  ASSERT_EQ(nets.size(), 3);
  const auto found = std::find_if(nets.begin(), nets.end(), [](const auto& n) {
    return n.as_object().at("name").as_string() == "stack/root_bus";
  });
  ASSERT_NE(found, nets.end());
  const auto& endpoints = found->as_object().at("bumps").as_array();
  ASSERT_EQ(endpoints.size(), 4);
  std::set<size_t> indices;
  std::set<std::string> paths;
  for (const auto& endpoint : endpoints) {
    const auto index = endpoint.to_number<size_t>();
    EXPECT_TRUE(indices.insert(index).second);
    const auto& row = result.at("bumps").as_array().at(index).as_array();
    const auto& chip = result.at("chiplets")
                           .as_array()
                           .at(row[0].to_number<size_t>())
                           .as_object();
    paths.emplace(chip.at("name").as_string());
  }
  EXPECT_EQ(paths,
            (std::set<std::string>{"stack/g0/left",
                                   "stack/g0/right",
                                   "stack/g1/left",
                                   "stack/g1/right"}));
}

TEST_F(Interconnect3DTest,
       OrientationEditsReplaceStaleGeometryAndKeepConnectivity)
{
  const auto before = data();
  first_->setOrient(odb::dbOrientType3D("MZ_R90"));
  first_->setLoc(odb::Point3D(650000, 450000, 100000));
  const auto after = data();
  ASSERT_EQ(after.at("bumps").as_array().size(), 12);
  auto footprints = [](const boost::json::object& result) {
    std::map<std::string, std::vector<boost::json::array>> by_chip;
    for (const auto& value : result.at("bumps").as_array()) {
      auto row = value.as_array();
      const auto& chip = result.at("chiplets")
                             .as_array()
                             .at(row[0].to_number<size_t>())
                             .as_object();
      row.erase(row.begin());
      by_chip[std::string(chip.at("name").as_string())].push_back(
          std::move(row));
    }
    return by_chip;
  };
  const auto old_footprints = footprints(before);
  const auto new_footprints = footprints(after);
  int changed = 0;
  for (const auto& [path, rows] : old_footprints) {
    const auto& updated = new_footprints.at(path);
    ASSERT_EQ(rows.size(), updated.size());
    for (size_t i = 0; i < rows.size(); ++i) {
      if (path.starts_with("stack/g0/")) {
        EXPECT_NE(rows[i], updated[i]);
        EXPECT_EQ(
            rows[i][2].to_number<int>() - rows[i][0].to_number<int>(),
            updated[i][3].to_number<int>() - updated[i][1].to_number<int>());
        EXPECT_EQ(
            rows[i][3].to_number<int>() - rows[i][1].to_number<int>(),
            updated[i][2].to_number<int>() - updated[i][0].to_number<int>());
        ++changed;
      } else {
        EXPECT_EQ(rows[i], updated[i]);
      }
    }
  }
  EXPECT_EQ(changed, 6);
  auto membership = [](const boost::json::object& result) {
    std::map<std::string, std::set<std::string>> nets;
    for (const auto& value : result.at("nets").as_array()) {
      const auto& net = value.as_object();
      auto& paths = nets[std::string(net.at("name").as_string())];
      for (const auto& endpoint : net.at("bumps").as_array()) {
        const auto& row = result.at("bumps")
                              .as_array()
                              .at(endpoint.to_number<size_t>())
                              .as_array();
        paths.emplace(result.at("chiplets")
                          .as_array()
                          .at(row[0].to_number<size_t>())
                          .as_object()
                          .at("name")
                          .as_string());
      }
    }
    return nets;
  };
  EXPECT_EQ(membership(before), membership(after));
  // Only refresh ODB after capturing the response: the read under test must
  // have ignored the stale unfolded model. Compare against ODB's new model.
  getDb()->constructUnfoldedModel();
  expectUnfoldedGeometry(after);
}

TEST_F(Interconnect3DTest,
       TwentyThousandBumpPayloadHasCompleteDistinctConnectivity)
{
  ASSERT_EQ(data().at("bumps").as_array().size(), 12);
  auto* region = odb::dbChipRegion::create(
      die_, "stress", odb::dbChipRegion::Side::FRONT, nullptr);
  auto* master = lib_->findMaster("BUMP");
  for (int i = 0; i < 4997; ++i) {
    auto* inst = odb::dbInst::create(
        block_, master, ("stress_" + std::to_string(i)).c_str());
    inst->setLocation(500 + (i % 100) * 800, 1000 + (i / 100) * 1200);
    inst->setPlacementStatus(odb::dbPlacementStatus::PLACED);
    ASSERT_NE(odb::dbChipBump::create(region, inst), nullptr);
  }
  auto* root = odb::dbChip::create(
      getDb(), nullptr, "stress", odb::dbChip::ChipType::HIER);
  const std::vector<std::string> orientations{"R0", "R90", "MY", "MZ_MX_R90"};
  std::vector<odb::dbChipInst*> chips;
  std::vector<std::vector<odb::dbChipBumpInst*>> terminals;
  for (int i = 0; i < 4; ++i) {
    auto* chip = odb::dbChipInst::create(root, die_, "d" + std::to_string(i));
    chip->setOrient(odb::dbOrientType3D(orientations[i]));
    chip->setLoc(odb::Point3D((i / 2) * 160000, 0, 0));
    chips.push_back(chip);
    std::vector<odb::dbChipBumpInst*> bumps;
    for (auto* placed_region : chip->getRegions()) {
      for (auto* bump : placed_region->getChipBumpInsts()) {
        if (bump->getChipBump()->getInst()->isPlaced()) {
          bumps.push_back(bump);
        }
      }
    }
    ASSERT_EQ(bumps.size(), 5000);
    terminals.push_back(std::move(bumps));
  }
  for (int i = 0; i < 5000; ++i) {
    auto* net = odb::dbChipNet::create(root, "n" + std::to_string(i));
    for (size_t c = 0; c < chips.size(); ++c) {
      net->addBumpInst(terminals[c][i], {chips[c]});
    }
  }
  getDb()->setTopChip(root);
  const auto result = data();
  const auto& bumps = result.at("bumps").as_array();
  const auto& nets = result.at("nets").as_array();
  ASSERT_EQ(result.at("chiplets").as_array().size(), 4);
  ASSERT_EQ(bumps.size(), 20000);
  ASSERT_EQ(nets.size(), 5000);
  std::set<size_t> referenced;
  for (const auto& value : nets) {
    const auto& net = value.as_object();
    const std::string name(net.at("name").as_string());
    const int terminal = std::stoi(name.substr(std::string("stress/n").size()));
    ASSERT_GE(terminal, 0);
    ASSERT_LT(terminal, 5000);
    const auto& endpoints = net.at("bumps").as_array();
    ASSERT_EQ(endpoints.size(), 4);
    std::set<std::string> connected_chips;
    for (const auto& endpoint : endpoints) {
      const auto index = endpoint.to_number<size_t>();
      EXPECT_TRUE(referenced.insert(index).second);
      const auto& row = bumps.at(index).as_array();
      const auto& chip = result.at("chiplets")
                             .as_array()
                             .at(row[0].to_number<size_t>())
                             .as_object();
      const std::string path(chip.at("name").as_string());
      const int c = std::stoi(path.substr(std::string("stress/d").size()));
      ASSERT_GE(c, 0);
      ASSERT_LT(c, 4);
      auto box = terminals[c][terminal]
                     ->getChipBump()
                     ->getInst()
                     ->getBBox()
                     ->getBox();
      chips[c]->getTransform().apply(box);
      EXPECT_EQ(row[1].to_number<int>(), box.xMin());
      EXPECT_EQ(row[2].to_number<int>(), box.yMin());
      EXPECT_EQ(row[3].to_number<int>(), box.xMax());
      EXPECT_EQ(row[4].to_number<int>(), box.yMax());
      connected_chips.insert(path);
    }
    EXPECT_EQ(connected_chips.size(), 4);
  }
  EXPECT_EQ(referenced.size(), 20000);
  const auto payload_bytes = boost::json::serialize(result).size();
  EXPECT_GT(payload_bytes, 500000);
  RecordProperty("payload_bytes", std::to_string(payload_bytes));
  writeFixture(result, "interconnect3d-stress.json");
}

}  // namespace
}  // namespace web
