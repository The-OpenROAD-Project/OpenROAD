// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026-2026, The OpenROAD Authors

// SplitLoadMt: the split prepareInputs() chooses, the candidate generate()
// wraps around it, and the loads apply() moves.

#include "TestResizerMtFixture.hh"

namespace rsz {

// One driver with twelve buffer loads and a top-level port on its net.
class TestSplitLoadMt : public TestResizerMt
{
 protected:
  void SetUp() override
  {
    readVerilogAndSetup("TestSplitLoadMt.v");

    // Placed pins give the split model its bounding boxes.
    int x = 1000;
    for (odb::dbInst* inst : block_->getInsts()) {
      inst->setLocation(x, 1000);
      inst->setPlacementStatus(odb::dbPlacementStatus::PLACED);
      x += 2000;
    }
    ep_.setHWireSignalRC(nullptr, sta_->cmdScene(), 1.0e3, 1.0e-10);
    ep_.setVWireSignalRC(nullptr, sta_->cmdScene(), 1.0e3, 1.0e-10);
    ep_.estimateWireParasitics();
    resizer_.runRepairSetupPreamble();

    // out<i> requires its data 300 + 20 x i ps before the clock edge, so
    // load0 is the least critical load and load11 the most.
    sta::Sdc* sdc = sta_->cmdMode()->sdc();
    for (int i = 0; i < kLoadCount; ++i) {
      const std::string port = "out" + std::to_string(i);
      sta_->setOutputDelay(db_network_->findPin(port.c_str()),
                           sta::RiseFallBoth::riseFall(),
                           sdc->findClock("clk"),
                           sta::RiseFall::rise(),
                           nullptr,
                           false,
                           false,
                           sta::MinMaxAll::all(),
                           false,
                           300e-12 + i * 20e-12,
                           sdc);
    }
    resizer_.updateParasiticsAndTiming();
  }

  // The split_drv target on the worst path, with its path window.
  Target makeSplitTarget()
  {
    Target target = makeTarget("out11", "split_drv", "Z");
    // The twelve loads and the port.
    target.fanout = kFanoutCount;
    FailReason fail_reason = FailReason::kNone;
    target.estimator_context
        = DelayEstimator::buildContext(resizer_, target, 0, &fail_reason);
    EXPECT_TRUE(target.estimator_context.has_value())
        << failReasonName(fail_reason);
    return target;
  }

  std::string pathName(const sta::Pin* pin) const
  {
    return sta_->network()->pathName(pin);
  }

  static constexpr int kLoadCount = 12;
  static constexpr int kFanoutCount = kLoadCount + 1;
};

TEST_F(TestSplitLoadMt, PrepareInputsSplitsOffTheLessCriticalHalf)
{
  Target target = makeSplitTarget();
  ASSERT_TRUE(target.estimator_context.has_value());
  MoveCommitter committer(resizer_);
  const OptimizerRunConfig run_config;
  const OptimizationPolicyConfig policy_config;
  SplitLoadMtGenerator generator(
      makeGeneratorContext(resizer_, committer, run_config, policy_config));
  ASSERT_TRUE(generator.isApplicable(target));
  generator.prepareInputs(target);
  ASSERT_NE(target.split_load_input, nullptr);

  // The port is the least critical of the 13 fanout pins and the cut takes
  // the first 6, but a top-level port never moves: load0 to load4 move.
  const SplitLoadMtInput& input = *target.split_load_input;
  ASSERT_EQ(input.load_pins.size(), 5u);
  for (size_t i = 0; i < input.load_pins.size(); ++i) {
    EXPECT_EQ(pathName(input.load_pins[i]), "load" + std::to_string(i) + "/A");
  }

  // Every moved load has a finite slack, so each pays the buffer delay.
  EXPECT_EQ(input.moved_loads.size(), input.load_pins.size());
  EXPECT_GT(input.moved_pin_cap, 0.0f);
  EXPECT_GT(input.moved_wire_cap, 0.0f);
  ASSERT_NE(input.resized_cell, nullptr);
  EXPECT_TRUE(input.resized_cell->isBuffer());
}

TEST_F(TestSplitLoadMt, IsApplicableNeedsALargeFanout)
{
  Target target = makeSplitTarget();
  ASSERT_TRUE(target.estimator_context.has_value());
  MoveCommitter committer(resizer_);
  const OptimizerRunConfig run_config;
  const OptimizationPolicyConfig policy_config;
  SplitLoadMtGenerator generator(
      makeGeneratorContext(resizer_, committer, run_config, policy_config));

  // A driver with 8 or fewer loads is not split.
  EXPECT_TRUE(generator.isApplicable(target));
  target.fanout = 8;
  EXPECT_FALSE(generator.isApplicable(target));
}

TEST_F(TestSplitLoadMt, GenerateWrapsThePreparedSplit)
{
  Target target = makeSplitTarget();
  ASSERT_TRUE(target.estimator_context.has_value());
  MoveCommitter committer(resizer_);
  const OptimizerRunConfig run_config;
  const OptimizationPolicyConfig policy_config;
  SplitLoadMtGenerator generator(
      makeGeneratorContext(resizer_, committer, run_config, policy_config));
  generator.prepareInputs(target);
  ASSERT_NE(target.split_load_input, nullptr);

  // One candidate, which reconnects exactly the prepared loads.
  const std::vector<std::unique_ptr<MoveCandidate>> candidates
      = generator.generate(target);
  ASSERT_EQ(candidates.size(), 1u);
  const MoveImpact impact = candidates.front()->impact();
  EXPECT_EQ(impact.reconnect_pins, target.split_load_input->load_pins);

  // generate() reads nothing but the prepared split, so it can run on a
  // worker: a target holding only the split gets the same candidate.
  Target bare;
  bare.split_load_input = target.split_load_input;
  const std::vector<std::unique_ptr<MoveCandidate>> bare_candidates
      = generator.generate(bare);
  ASSERT_EQ(bare_candidates.size(), 1u);
  EXPECT_EQ(bare_candidates.front()->guardedSidePins(),
            candidates.front()->guardedSidePins());
}

TEST_F(TestSplitLoadMt, ApplyMovesTheLoadsOfTheSplit)
{
  Target target = makeSplitTarget();
  ASSERT_TRUE(target.estimator_context.has_value());
  MoveCommitter committer(resizer_);
  const OptimizerRunConfig run_config;
  const OptimizationPolicyConfig policy_config;
  SplitLoadMtGenerator generator(
      makeGeneratorContext(resizer_, committer, run_config, policy_config));
  generator.prepareInputs(target);
  ASSERT_NE(target.split_load_input, nullptr);
  const std::vector<std::unique_ptr<MoveCandidate>> candidates
      = generator.generate(target);
  ASSERT_EQ(candidates.size(), 1u);

  // The new buffer drives exactly the loads of the split.
  const MoveResult result = candidates.front()->apply();
  ASSERT_TRUE(result.accepted);
  ASSERT_EQ(result.touched_instances.size(), 1u);
  sta::Network* network = sta_->network();
  sta::Instance* buffer = result.touched_instances.front();
  std::vector<std::string> buffer_loads;
  std::unique_ptr<sta::InstancePinIterator> buffer_pins(
      network->pinIterator(buffer));
  while (buffer_pins->hasNext()) {
    const sta::Pin* pin = buffer_pins->next();
    if (!network->isDriver(pin)) {
      continue;
    }
    std::unique_ptr<sta::PinConnectedPinIterator> loads(
        network->connectedPinIterator(pin));
    while (loads->hasNext()) {
      const sta::Pin* load = loads->next();
      if (network->isLoad(load) && !network->isHierarchical(load)) {
        buffer_loads.push_back(pathName(load));
      }
    }
  }
  std::vector<std::string> moved;
  moved.reserve(target.split_load_input->load_pins.size());
  for (const sta::Pin* pin : target.split_load_input->load_pins) {
    moved.push_back(pathName(pin));
  }
  std::ranges::sort(buffer_loads);
  std::ranges::sort(moved);
  EXPECT_EQ(buffer_loads, moved);
}

}  // namespace rsz
