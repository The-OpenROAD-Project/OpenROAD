// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include <algorithm>
#include <fstream>
#include <numeric>
#include <regex>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include "fsv/Itf.h"
#include "fsv/Stack.h"
#include "gtest/gtest.h"
#include "odb/db.h"
#include "odb/dbTypes.h"
#include "odb/dbWireCodec.h"
#include "odb/geom.h"
#include "pex/Network.h"
#include "pex/Resistance.h"
#include "tst/db_fixture.h"

namespace pex {
namespace {

// sky130.itf: met1 and met2 at 0.125 ohm/sq with a resistive etch of
// -0.0195 µm a side, so a 0.14 µm wire is 0.179 µm wide in silicon; a via
// cut of 0.0225 µm² is 4.5 ohm.
constexpr double kMet1PerUm = 0.125 / 0.179;
constexpr double kViaCut = 4.5;

class NetworkTest : public tst::DbFixture
{
 protected:
  void SetUp() override
  {
    tech_ = loadTechLef("sky130", "_main/test/sky130hd/sky130hd.tlef");
    loadLibLef(tech_, "cells", "_main/test/sky130hd/sky130hd_std_cell.lef");
    odb::dbChip* chip = odb::dbChip::create(getDb(), tech_);
    block_ = odb::dbBlock::create(chip, "top");
    stack_ = fsv::readItfFile(getFilePath("_main/src/fsv/test/sky130.itf"));
    met1_ = tech_->findLayer("met1");
    met2_ = tech_->findLayer("met2");
  }

  // A net "n" with a block pin and the A pins of `count` inverters.
  void makeNet(const int count)
  {
    bterm_ = makeBTerm(block_, "n");
    for (int i = 0; i < count; i++) {
      const std::string name = "u" + std::to_string(i);
      iterms_.push_back(
          makeInst(block_,
                   getDb()->findMaster("sky130_fd_sc_hd__inv_1"),
                   name.c_str(),
                   {
                       .location = {0, 0},
                       .status = odb::dbPlacementStatus::PLACED,
                       .iterms = {{.net_name = "n", .term_name = "A"}},
                   })
              ->findITerm("A"));
    }
    net_ = block_->findNet("n");
    wire_ = odb::dbWire::create(net_);
  }

  odb::dbTech* tech_ = nullptr;
  odb::dbBlock* block_ = nullptr;
  fsv::Stack stack_;
  odb::dbTechLayer* met1_ = nullptr;
  odb::dbTechLayer* met2_ = nullptr;
  odb::dbNet* net_ = nullptr;
  odb::dbWire* wire_ = nullptr;
  odb::dbBTerm* bterm_ = nullptr;
  std::vector<odb::dbITerm*> iterms_;
};

TEST_F(NetworkTest, WireAndCut)
{
  const Resistance r(stack_, tech_);
  EXPECT_NEAR(r.wire(met1_, 140, 10000), 10 * kMet1PerUm, 1e-9);
  EXPECT_NEAR(r.cut(tech_->findLayer("via"), {0, 0, 150, 150}), kViaCut, 1e-9);
  // A cut twice the area is half the resistance.
  EXPECT_NEAR(
      r.cut(tech_->findLayer("via"), {0, 0, 300, 150}), kViaCut / 2, 1e-9);

  stack_.setGlobalTemperature(125);
  const Resistance hot(stack_, tech_);
  EXPECT_NEAR(hot.wire(met1_, 140, 10000),
              10 * kMet1PerUm * (1 + (0.003179 * 100) + (3.094e-7 * 1e4)),
              1e-9);
}

TEST_F(NetworkTest, MissingConductor)
{
  std::ifstream in(getFilePath("_main/src/fsv/test/sky130.itf"));
  std::stringstream text;
  text << in.rdbuf();
  std::istringstream renamed(
      std::regex_replace(text.str(), std::regex("met5"), "top"));
  const fsv::Stack stack = fsv::readItf(renamed);
  // Only a wire on the layer the stack lacks is an error.
  const Resistance r(stack, tech_);
  EXPECT_NEAR(r.wire(met1_, 140, 10000), 10 * kMet1PerUm, 1e-9);
  try {
    r.wire(tech_->findLayer("met5"), 1600, 10000);
    FAIL() << "no error";
  } catch (const std::invalid_argument& e) {
    EXPECT_NE(std::string(e.what()).find("met5"), std::string::npos);
  }
}

TEST_F(NetworkTest, NoWire)
{
  makeNet(1);
  odb::dbWire::destroy(wire_);
  const Network n = buildNetwork(net_, Resistance(stack_, tech_), 50);
  EXPECT_TRUE(n.nodes.empty());
  EXPECT_TRUE(n.resistors.empty());
}

// A jog from the block pin to an inverter: a node at the corner only when
// every shape is its own resistor.
TEST_F(NetworkTest, Jog)
{
  makeNet(1);
  odb::dbWireEncoder e;
  e.begin(wire_);
  e.newPath(met1_, odb::dbWireType::ROUTED);
  e.addPoint(0, 0);
  e.addBTerm(bterm_);
  const int across = e.addPoint(10000, 0);
  const int up = e.addPoint(10000, 5000);
  e.addITerm(iterms_[0]);
  e.end();
  const Resistance r(stack_, tech_);

  const Network each = buildNetwork(net_, r, 0);
  ASSERT_EQ(each.nodes.size(), 3);
  EXPECT_EQ(each.nodes[0].kind, NetNode::Kind::kBTerm);
  EXPECT_EQ(each.nodes[1].kind, NetNode::Kind::kJunction);
  EXPECT_EQ(each.nodes[2].kind, NetNode::Kind::kITerm);
  ASSERT_EQ(each.resistors.size(), 2);
  EXPECT_EQ(each.resistors[0].source, 0);
  EXPECT_EQ(each.resistors[0].target, 1);
  EXPECT_NEAR(each.resistors[0].resistance, 10 * kMet1PerUm, 1e-9);
  EXPECT_EQ(each.resistors[1].source, 1);
  EXPECT_EQ(each.resistors[1].target, 2);
  EXPECT_NEAR(each.resistors[1].resistance, 5 * kMet1PerUm, 1e-9);

  const Network merged = buildNetwork(net_, r, 50);
  ASSERT_EQ(merged.nodes.size(), 2);
  ASSERT_EQ(merged.resistors.size(), 1);
  EXPECT_NEAR(merged.resistors[0].resistance, 15 * kMet1PerUm, 1e-9);
  EXPECT_EQ(merged.iterms.at(iterms_[0]->getId()), 1);
  EXPECT_EQ(merged.bterms.at(bterm_->getId()), 0);
  // Halfway up the second shape: 12.5 of 15 µm along the resistor; past
  // the shape's start, its start.
  EXPECT_NEAR(merged.place(up, {10000, 2500}).t, 12.5 / 15, 1e-12);
  EXPECT_NEAR(merged.place(across, {-500, 0}).t, 0, 1e-12);
  EXPECT_EQ(merged.place(up, {10000, 2500}).resistor, 0);
  EXPECT_THROW(merged.place(12345, {0, 0}), std::out_of_range);

  // Below 15 µm of resistance a node comes at the corner.
  const Network split = buildNetwork(net_, r, 12 * kMet1PerUm);
  EXPECT_EQ(split.resistors.size(), 2);
}

// A branch leaves from the middle of a path: its junction is a node.
TEST_F(NetworkTest, Branch)
{
  makeNet(2);
  odb::dbWireEncoder e;
  e.begin(wire_);
  e.newPath(met1_, odb::dbWireType::ROUTED);
  e.addPoint(0, 0);
  e.addBTerm(bterm_);
  const int middle = e.addPoint(10000, 0);
  e.addPoint(20000, 0);
  e.addITerm(iterms_[0]);
  e.newPath(middle, odb::dbWireType::ROUTED);
  e.addPoint(10000, 5000);
  e.addITerm(iterms_[1]);
  e.end();

  const Network n = buildNetwork(net_, Resistance(stack_, tech_), 50);
  ASSERT_EQ(n.nodes.size(), 4);
  ASSERT_EQ(n.resistors.size(), 3);
  const int junction = n.resistors[0].target;
  EXPECT_EQ(n.nodes[junction].kind, NetNode::Kind::kJunction);
  EXPECT_EQ(n.resistors[1].source, junction);
  EXPECT_EQ(n.resistors[2].source, junction);
  EXPECT_EQ(n.resistors[2].target, n.iterms.at(iterms_[1]->getId()));
  EXPECT_NEAR(n.resistors[2].resistance, 5 * kMet1PerUm, 1e-9);
}

// Whether every node of a network is joined to node 0 through resistors.
bool connected(const Network& n)
{
  std::vector<int> group(n.nodes.size());
  std::iota(group.begin(), group.end(), 0);
  auto find = [&](int i) {
    while (group[i] != i) {
      i = group[i] = group[group[i]];
    }
    return i;
  };
  for (const NetResistor& r : n.resistors) {
    group[find(r.source)] = find(r.target);
  }
  for (size_t i = 0; i < n.nodes.size(); i++) {
    if (find(static_cast<int>(i)) != find(0)) {
      return false;
    }
  }
  return true;
}

// A branch whose start carries a pin stays joined to the path it leaves.
TEST_F(NetworkTest, PinOnBranchStart)
{
  makeNet(2);
  odb::dbWireEncoder e;
  e.begin(wire_);
  e.newPath(met1_, odb::dbWireType::ROUTED);
  e.addPoint(0, 0);
  e.addBTerm(bterm_);
  const int middle = e.addPoint(10000, 0);
  e.addPoint(20000, 0);
  e.addITerm(iterms_[0]);
  e.newPath(middle, odb::dbWireType::ROUTED);
  e.addPoint(10000, 0);
  e.addITerm(iterms_[1]);
  e.addPoint(10000, 5000);
  e.end();

  const Network n = buildNetwork(net_, Resistance(stack_, tech_), 50);
  EXPECT_TRUE(connected(n));
  ASSERT_TRUE(n.iterms.contains(iterms_[1]->getId()));
  const NetNode& pin = n.nodes[n.iterms.at(iterms_[1]->getId())];
  EXPECT_EQ(pin.kind, NetNode::Kind::kITerm);
  EXPECT_EQ(pin.point, odb::Point(10000, 0));
}

// Two pins on one point each get a node, joined by 0 ohms.
TEST_F(NetworkTest, TwoPinsOnePoint)
{
  makeNet(2);
  odb::dbWireEncoder e;
  e.begin(wire_);
  e.newPath(met1_, odb::dbWireType::ROUTED);
  e.addPoint(0, 0);
  e.addBTerm(bterm_);
  e.addITerm(iterms_[0]);
  e.addPoint(10000, 0);
  e.addITerm(iterms_[1]);
  e.end();

  const Network n = buildNetwork(net_, Resistance(stack_, tech_), 50);
  EXPECT_TRUE(connected(n));
  ASSERT_TRUE(n.bterms.contains(bterm_->getId()));
  ASSERT_TRUE(n.iterms.contains(iterms_[0]->getId()));
  const int b = n.bterms.at(bterm_->getId());
  const int i = n.iterms.at(iterms_[0]->getId());
  EXPECT_NE(b, i);
  EXPECT_TRUE(std::ranges::any_of(n.resistors, [&](const NetResistor& r) {
    return r.resistance == 0
           && ((r.source == b && r.target == i)
               || (r.source == i && r.target == b));
  }));
}

// A via joins the wires on each side in series.
TEST_F(NetworkTest, Via)
{
  makeNet(1);
  odb::dbWireEncoder e;
  e.begin(wire_);
  e.newPath(met1_, odb::dbWireType::ROUTED);
  e.addPoint(0, 0);
  e.addBTerm(bterm_);
  e.addPoint(5000, 0);
  const int via = e.addTechVia(tech_->findVia("M1M2_PR"));
  e.addPoint(5000, 5000);
  e.addITerm(iterms_[0]);
  e.end();

  const Network n = buildNetwork(net_, Resistance(stack_, tech_), 50);
  ASSERT_EQ(n.resistors.size(), 1);
  EXPECT_NEAR(n.resistors[0].resistance, (10 * kMet1PerUm) + kViaCut, 1e-9);
  const double before = 5 * kMet1PerUm / n.resistors[0].resistance;
  const double after = (5 * kMet1PerUm + kViaCut) / n.resistors[0].resistance;
  EXPECT_NEAR(n.place(via, {5000, 0}).t, (before + after) / 2, 1e-12);
}

// A short starts a path at an earlier junction's node.
TEST_F(NetworkTest, Short)
{
  makeNet(2);
  odb::dbWireEncoder e;
  e.begin(wire_);
  e.newPath(met1_, odb::dbWireType::ROUTED);
  e.addPoint(0, 0);
  e.addBTerm(bterm_);
  const int middle = e.addPoint(10000, 0);
  e.addPoint(20000, 0);
  e.addITerm(iterms_[0]);
  e.newPathShort(middle, met2_, odb::dbWireType::ROUTED);
  e.addPoint(10000, 0);
  e.addPoint(10000, 5000);
  e.addITerm(iterms_[1]);
  e.end();

  const Network n = buildNetwork(net_, Resistance(stack_, tech_), 50);
  ASSERT_EQ(n.resistors.size(), 3);
  EXPECT_EQ(n.resistors[2].source, n.resistors[0].target);
  EXPECT_EQ(n.resistors[2].target, n.iterms.at(iterms_[1]->getId()));
}

}  // namespace
}  // namespace pex
