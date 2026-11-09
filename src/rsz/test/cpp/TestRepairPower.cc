// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026-2026, The OpenROAD Authors

// repair_power: VT swaps that save power within the timing contract.
// TestRepairPower.v chains LVT AND2 cells p0..p5 from input a to oend.  Each
// p<k> also has a side input s<k> and an output o<k> of its own, behind a
// typical-VT buffer that has no higher VT.  r0, r1 and f0 are LVT buffers;
// f_out has no output delay, so f0 has no required time.  The Nangate45 LVT
// LEF has no VT implant layer, so the fixture gives the LVT masters one.

#include <map>
#include <string>

#include "PowerModel.hh"
#include "TestResizerMtFixture.hh"
#include "sta/PowerClass.hh"

namespace rsz {

class TestRepairPower : public TestResizerMt
{
 protected:
  static constexpr int kChain = 6;
  static constexpr float kPeriod = 1e-9f;
  // Float noise in the slack sums.
  static constexpr float kNoise = 1e-15f;

  virtual bool tagLvtMasters() const { return true; }

  void SetUp() override
  {
    if (tagLvtMasters()) {
      odb::dbTechLayer* implant = odb::dbTechLayer::create(
          db_->getTech(), "LVT", odb::dbTechLayerType::IMPLANT);
      for (odb::dbMaster* master : lib_->getMasters()) {
        if (master->getName().ends_with("_L")) {
          odb::dbBox::create(
              master, implant, 0, 0, master->getWidth(), master->getHeight());
        }
      }
    }
    readVerilogAndSetup("TestRepairPower.v", /*init_default_sdc=*/false);
    sta::PinSet clk_pins(db_network_);
    clk_pins.insert(pin("clk"));
    sta_->makeClock("clk",
                    clk_pins,
                    false,
                    kPeriod,
                    {0.0f, kPeriod / 2},
                    "",
                    sta_->cmdMode());
    setInputDelay("a", 0.0f);
    setInputDelay("r_in", 0.0f);
    setInputDelay("f_in", 0.0f);
    setOutputDelay("r_out", 0.0f);
    setOutputDelay("oend", 0.0f);
    for (int k = 0; k < kChain; ++k) {
      setOutputDelay(name("o", k), 0.0f);
    }
  }

  static std::string name(const char* prefix, const int k)
  {
    return prefix + std::to_string(k);
  }

  sta::Pin* pin(const std::string& path) const
  {
    sta::Pin* found = db_network_->findPin(path.c_str());
    EXPECT_NE(found, nullptr) << path;
    return found;
  }

  sta::Clock* clock() const { return sta_->cmdMode()->sdc()->findClock("clk"); }

  void setInputDelay(const std::string& port, const float delay)
  {
    sta_->setInputDelay(pin(port),
                        sta::RiseFallBoth::riseFall(),
                        clock(),
                        sta::RiseFall::rise(),
                        nullptr,
                        false,
                        false,
                        sta::MinMaxAll::all(),
                        false,
                        delay,
                        sta_->cmdMode()->sdc());
  }

  void setOutputDelay(const std::string& port, const float delay)
  {
    sta_->setOutputDelay(pin(port),
                         sta::RiseFallBoth::riseFall(),
                         clock(),
                         sta::RiseFall::rise(),
                         nullptr,
                         false,
                         false,
                         sta::MinMaxAll::all(),
                         false,
                         delay,
                         sta_->cmdMode()->sdc());
  }

  float arrival(const std::string& path)
  {
    sta_->updateTiming(false);
    return sta::delayAsFloat(sta_->arrival(
        pin(path), sta::RiseFallBoth::riseFall(), sta::MinMax::max()));
  }

  float slack(const std::string& port)
  {
    sta_->updateTiming(false);
    return sta::delayAsFloat(sta_->slack(pin(port),
                                         sta::RiseFallBoth::riseFall(),
                                         sta_->scenes(),
                                         sta::MinMax::max()));
  }

  std::string master(const std::string& inst) const
  {
    return block_->findInst(inst.c_str())->getMaster()->getName();
  }

  void swap(const std::string& inst, const std::string& cell)
  {
    sta::Instance* sta_inst = db_network_->findInstance(inst.c_str());
    ASSERT_NE(sta_inst, nullptr) << inst;
    sta::LibertyCell* lib_cell = db_network_->findLibertyCell(cell.c_str());
    ASSERT_NE(lib_cell, nullptr) << cell;
    sta_->replaceCell(sta_inst, lib_cell);
  }

  std::map<std::string, std::string> masters() const
  {
    std::map<std::string, std::string> result;
    for (odb::dbInst* inst : block_->getInsts()) {
      result[inst->getName()] = inst->getMaster()->getName();
    }
    return result;
  }

  float totalPower()
  {
    sta::PowerResult total, sequential, combinational, clk, macro, pad;
    sta_->power(
        sta_->cmdScene(), total, sequential, combinational, clk, macro, pad);
    return total.total();
  }

  struct Contract
  {
    float setup_ws;
    float setup_tns;
    float hold_ws;
  };

  Contract contract()
  {
    sta_->updateTiming(false);
    return {.setup_ws = sta::delayAsFloat(sta_->worstSlack(sta::MinMax::max())),
            .setup_tns
            = sta::delayAsFloat(sta_->totalNegativeSlack(sta::MinMax::max())),
            .hold_ws = sta::delayAsFloat(sta_->worstSlack(sta::MinMax::min()))};
  }

  // The strict contract against `start`: setup and hold WS no worse below
  // their margins, setup TNS within 1%.
  static void expectKept(const Contract& start,
                         const Contract& now,
                         const float setup_margin,
                         const float hold_margin)
  {
    EXPECT_GE(std::min(now.setup_ws - setup_margin, 0.0f),
              std::min(start.setup_ws - setup_margin, 0.0f) - kNoise);
    EXPECT_GE(now.setup_tns, 1.01f * start.setup_tns - kNoise);
    EXPECT_GE(std::min(now.hold_ws - hold_margin, 0.0f),
              std::min(start.hold_ws - hold_margin, 0.0f) - kNoise);
  }

  std::map<std::string, float> endpointSlacks()
  {
    sta_->updateTiming(false);
    std::map<std::string, float> slacks;
    for (sta::Vertex* endpoint : sta_->endpoints()) {
      slacks[db_network_->pathName(endpoint->pin())]
          = sta::delayAsFloat(sta_->slack(endpoint, sta::MinMax::max()));
    }
    return slacks;
  }

  // Each p<k> gets `room` on its own path s<k> -> p<k> -> o<k>: s<k> arrives
  // after the chain at p<k>, so p<k>'s worst path holds no other p.  The
  // chain a -> p0 .. p5 -> oend gets `room` plus the lead the side inputs
  // built up plus `extra`, so oend itself is at `room` plus `extra`.
  void timeChain(const float room, const float extra)
  {
    constexpr float kGap = 3e-12f;
    constexpr float kLate = 500e-12f;
    const float chain_end = arrival("oend");
    for (int k = 0; k < kChain; ++k) {
      const std::string out = name("p", k) + "/ZN";
      const float through_a1 = arrival(out);
      setInputDelay(name("s", k), kLate);
      const float a2_delay = arrival(out) - kLate;
      setInputDelay(name("s", k), through_a1 + kGap - a2_delay);
    }
    for (int k = 0; k < kChain; ++k) {
      const std::string out = name("o", k);
      setOutputDelay(out, kPeriod - arrival(out) - room);
    }
    const float lead = arrival("oend") - chain_end;
    setOutputDelay("oend", kPeriod - chain_end - (room + lead + extra));
  }
};

// Every LVT cell has slack to spare, the one without a required time
// included: all of them go to the typical VT and the total power drops.
TEST_F(TestRepairPower, SwapsEveryCellWithRoom)
{
  for (int k = 0; k < kChain; ++k) {
    setInputDelay(name("s", k), 0.0f);
  }
  const Contract start = contract();
  const float power = totalPower();
  resizer_.repairPower(0.0, 0.0, 0.0, false);

  for (int k = 0; k < kChain; ++k) {
    EXPECT_EQ(master(name("p", k)), "AND2_X1");
  }
  EXPECT_EQ(master("r0"), "BUF_X1");
  EXPECT_EQ(master("r1"), "BUF_X1");
  EXPECT_EQ(master("f0"), "BUF_X1");
  EXPECT_LT(totalPower(), power);
  expectKept(start, contract(), 0.0f, 0.0f);
}

// The gain PowerModel predicts for a swap is the drop in the instance's STA
// internal and leakage power.  The test libraries differ in supply voltage,
// which STA also applies to the driven net's switching power; the model
// takes the VTs of one cell to share a supply.
TEST_F(TestRepairPower, PowerModelGainIsTheInstancePowerDrop)
{
  PowerModel model(resizer_);
  model.init();
  sta::Instance* r0 = db_network_->findInstance("r0");
  model.prepare(r0);
  const float gain = model.gain(r0, db_network_->findLibertyCell("BUF_X1"));
  const auto own_power = [&]() {
    totalPower();
    const sta::PowerResult power = sta_->power(r0, sta_->cmdScene());
    return power.internal() + power.leakage();
  };
  const float before = own_power();
  swap("r0", "BUF_X1");
  const float drop = before - own_power();
  ASSERT_GT(drop, 0.0f);
  EXPECT_NEAR(gain, drop, 0.01f * drop);
}

// A swap may spend at most half of its room: p0's swap costs more than half
// of the room on its own path, though less than all of it, and is not taken.
TEST_F(TestRepairPower, SpendsAtMostHalfTheRoom)
{
  for (int k = 0; k < kChain; ++k) {
    setInputDelay(name("s", k), 0.0f);
  }
  const float before = slack("o0");
  swap("p0", "AND2_X1");
  const float cost = before - slack("o0");
  swap("p0", "AND2_X1_L");
  ASSERT_GT(cost, 0.0f);
  setOutputDelay("o0", kPeriod - arrival("o0") - 1.5f * cost);

  resizer_.repairPower(0.0, 0.0, 0.0, false);
  EXPECT_EQ(master("p0"), "AND2_X1_L");
  EXPECT_EQ(master("p1"), "AND2_X1");
  EXPECT_GE(slack("o0"), 1.5f * cost - kNoise);
}

// Each p<k> fits its own budget, but together the swaps break the chain: the
// batch is salvaged.  The swaps beside the chain stay and the chain keeps
// its slack.
TEST_F(TestRepairPower, SalvagesABatchThatBreaksAPath)
{
  const float room = 40e-12f;
  timeChain(room, 2e-12f);
  for (int k = 0; k < kChain; ++k) {
    const std::string p = name("p", k);
    const std::string out = name("o", k);
    swap(p, "AND2_X1");
    EXPECT_LT(room - slack(out), room / 2) << p << " exceeds its budget";
    swap(p, "AND2_X1_L");
  }
  for (int k = 0; k < kChain; ++k) {
    swap(name("p", k), "AND2_X1");
  }
  ASSERT_LT(slack("oend"), 0.0f) << "the swaps together must break oend";
  for (int k = 0; k < kChain; ++k) {
    swap(name("p", k), "AND2_X1_L");
  }

  const Contract start = contract();
  ASSERT_GT(start.setup_ws, 0.0f);
  const float power = totalPower();
  resizer_.repairPower(0.0, 0.0, 0.0, false);

  EXPECT_EQ(master("r0"), "BUF_X1");
  EXPECT_EQ(master("r1"), "BUF_X1");
  EXPECT_EQ(master("f0"), "BUF_X1");
  EXPECT_GE(slack("oend"), -kNoise);
  EXPECT_LT(totalPower(), power);
  expectKept(start, contract(), 0.0f, 0.0f);
}

// With -setup_tolerance, the chain that the strict run left alone may spend
// its slack below zero, and no endpoint falls below its floor.
TEST_F(TestRepairPower, ToleranceKeepsEachEndpointAboveItsFloor)
{
  timeChain(40e-12f, 2e-12f);
  resizer_.repairPower(0.0, 0.0, 0.0, false);
  const std::map<std::string, float> start = endpointSlacks();

  const double percent = 5.0;
  resizer_.repairPower(0.0, 0.0, percent, false);
  const float tolerance = percent / 100.0 * kPeriod;
  for (const auto& [endpoint, slack] : endpointSlacks()) {
    EXPECT_GE(slack, std::min(start.at(endpoint), 0.0f) - tolerance - kNoise)
        << endpoint;
  }
  for (int k = 0; k < kChain; ++k) {
    EXPECT_EQ(master(name("p", k)), "AND2_X1");
  }
  EXPECT_LT(slack("oend"), 0.0f);
}

// A tolerance smaller than what the whole chain would spend: the batch that
// takes the chain breaks oend's floor and is salvaged, and every endpoint
// stays above its floor.
TEST_F(TestRepairPower, ToleranceSalvageKeepsTheFloors)
{
  timeChain(40e-12f, 2e-12f);
  const std::map<std::string, float> start = endpointSlacks();
  const double percent = 1.0;
  resizer_.repairPower(0.0, 0.0, percent, false);
  const float tolerance = percent / 100.0 * kPeriod;
  for (const auto& [endpoint, slack] : endpointSlacks()) {
    EXPECT_GE(slack, std::min(start.at(endpoint), 0.0f) - tolerance - kNoise)
        << endpoint;
  }
  EXPECT_EQ(master("r0"), "BUF_X1");
}

// Setup violations at the start: the negative paths are not touched, the
// rest still saves power.
TEST_F(TestRepairPower, StartsWithSetupViolations)
{
  for (int k = 0; k < kChain; ++k) {
    setInputDelay(name("s", k), 0.0f);
  }
  setOutputDelay("oend", kPeriod - arrival("oend") + 20e-12f);
  const Contract start = contract();
  ASSERT_LT(start.setup_ws, 0.0f);
  const float power = totalPower();
  const float margin = 5e-12f;
  resizer_.repairPower(margin, 0.0, 0.0, false);

  EXPECT_EQ(master("r0"), "BUF_X1");
  EXPECT_LT(totalPower(), power);
  expectKept(start, contract(), margin, 0.0f);
}

// A max transition limit and a hold margin above the hold slack: no slew
// violation gets new or larger, and the hold WS does not drop.
TEST_F(TestRepairPower, KeepsSlewLimitsAndHoldMargin)
{
  for (int k = 0; k < kChain; ++k) {
    setInputDelay(name("s", k), 0.0f);
  }
  sta_->updateTiming(false);
  const float limit
      = 1.05f
        * sta::delayAsFloat(sta_->graph()->slew(
            sta_->graph()->pinDrvrVertex(pin("p0/ZN")),
            sta::RiseFall::rise(),
            sta_->cmdScene()->dcalcAnalysisPtIndex(sta::MinMax::max())));
  sta_->setSlewLimit(db_network_->cell(db_network_->topInstance()),
                     sta::MinMax::max(),
                     limit,
                     sta_->cmdMode()->sdc());

  const auto slew_slacks = [&]() {
    sta_->updateTiming(false);
    sta_->checkSlewsPreamble();
    std::map<std::string, float> slacks;
    for (odb::dbInst* inst : block_->getInsts()) {
      for (odb::dbITerm* iterm : inst->getITerms()) {
        const sta::Pin* p = db_network_->dbToSta(iterm);
        sta::Slew slew;
        float pin_limit, pin_slack;
        const sta::RiseFall* rf;
        const sta::Scene* scene;
        sta_->checkSlew(p,
                        sta_->scenes(),
                        sta::MinMax::max(),
                        false,
                        slew,
                        pin_limit,
                        pin_slack,
                        rf,
                        scene);
        slacks[db_network_->pathName(p)] = pin_slack;
      }
    }
    return slacks;
  };
  const std::map<std::string, float> slews = slew_slacks();
  const Contract start = contract();
  const float hold_margin = kPeriod;
  resizer_.repairPower(0.0, hold_margin, 0.0, false);

  for (const auto& [p, slack] : slew_slacks()) {
    EXPECT_GE(slack, std::min(slews.at(p), 0.0f) - kNoise) << p;
  }
  EXPECT_GE(contract().hold_ws, start.hold_ws - kNoise);
  expectKept(start, contract(), 0.0f, hold_margin);
}

// Cells already at the highest VT have nothing to swap.
TEST_F(TestRepairPower, LeavesTheHighestVtAlone)
{
  for (int k = 0; k < kChain; ++k) {
    swap(name("p", k), "AND2_X1");
  }
  for (const char* buffer : {"r0", "r1", "f0"}) {
    swap(buffer, "BUF_X1");
  }
  const std::map<std::string, std::string> start = masters();
  resizer_.repairPower(0.0, 0.0, 0.0, false);
  EXPECT_EQ(masters(), start);
}

// Without a VT implant layer the library has a single VT: nothing to swap.
class TestRepairPowerSingleVt : public TestRepairPower
{
 protected:
  bool tagLvtMasters() const override { return false; }
};

TEST_F(TestRepairPowerSingleVt, LeavesEveryCell)
{
  const std::map<std::string, std::string> start = masters();
  resizer_.repairPower(0.0, 0.0, 0.0, false);
  EXPECT_EQ(masters(), start);
}

// The same netlist and constraints give the same swaps.
TEST_F(TestRepairPower, RepeatsItsSwaps)
{
  timeChain(40e-12f, 2e-12f);
  const std::map<std::string, std::string> start = masters();
  resizer_.repairPower(0.0, 0.0, 0.0, false);
  const std::map<std::string, std::string> first = masters();
  for (const auto& [inst, cell] : start) {
    swap(inst, cell);
  }
  resizer_.repairPower(0.0, 0.0, 0.0, false);
  EXPECT_EQ(masters(), first);
}

}  // namespace rsz
