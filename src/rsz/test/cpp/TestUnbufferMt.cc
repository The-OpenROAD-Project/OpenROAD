// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026-2026, The OpenROAD Authors

// UnbufferMt: the targets isApplicable() takes, the removal gain
// prepareInputs() estimates, the candidate generate() offers, and the buffer
// apply() removes.

#include "TestResizerMtFixture.hh"

namespace rsz {

// The fixture's path_pre0 -> path_pre1 chain: path_pre1 is a BUF_X16 that
// drives one NAND2 input, so its removal saves most of its own delay.
class TestUnbufferMt : public TestResizerMt
{
 protected:
  void SetUp() override
  {
    TestResizerMt::SetUp();

    // The removal estimate stitches the Steiner trees of the two nets, which
    // need placed pins.
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
    resizer_.updateParasiticsAndTiming();
  }

  UnbufferMtGenerator makeGenerator(MoveCommitter& committer)
  {
    return UnbufferMtGenerator(
        makeGeneratorContext(resizer_, committer, run_config_, policy_config_));
  }

  const OptimizerRunConfig run_config_;
  const OptimizationPolicyConfig policy_config_;
};

TEST_F(TestUnbufferMt, IsApplicableTakesABufferWithADrivingStage)
{
  MoveCommitter committer(resizer_);
  UnbufferMtGenerator generator = makeGenerator(committer);

  // path_pre1 is a buffer driven by path_pre0; the NAND2 is not a buffer.
  EXPECT_TRUE(generator.isApplicable(makeTarget("path_out", "path_pre1", "Z")));
  EXPECT_FALSE(
      generator.isApplicable(makeTarget("path_out", "path_target", "ZN")));
}

TEST_F(TestUnbufferMt, GenerateOffersARemovalThatGains)
{
  MoveCommitter committer(resizer_);
  UnbufferMtGenerator generator = makeGenerator(committer);
  Target target = makeTarget("path_out", "path_pre1", "Z");
  ASSERT_TRUE(generator.isApplicable(target));

  // The removal saves the buffer delay and costs path_pre0 little.
  generator.prepareInputs(target);
  ASSERT_TRUE(target.unbuffer_gain.has_value());
  EXPECT_GT(*target.unbuffer_gain, 0.0f);
  const std::vector<std::unique_ptr<MoveCandidate>> candidates
      = generator.generate(target);
  ASSERT_EQ(candidates.size(), 1u);
  EXPECT_FLOAT_EQ(candidates.front()->estimate().score, *target.unbuffer_gain);

  // A removal that estimates as no gain is not offered.
  target.unbuffer_gain = 0.0f;
  EXPECT_TRUE(generator.generate(target).empty());
}

// The impact reconnects the pins removeBuffer() moves: those of the output
// net, or those of the input net when only the output net has a port.
TEST_F(TestUnbufferMt, ImpactReconnectsThePinsOfTheDroppedNet)
{
  // path_pre1 keeps its input net, so the NAND2 input moves.
  const Target inner_target = makeTarget("path_out", "path_pre1", "Z");
  const UnbufferMtCandidate inner(resizer_, inner_target);
  EXPECT_EQ(
      inner.impact().reconnect_pins,
      std::vector<const sta::Pin*>{db_network_->findPin("path_target/A1")});

  // path_load drives the path_out port, so its output net stays and the NAND2
  // output moves onto it.
  const Target outer_target = makeTarget("path_out", "path_load", "Z");
  const UnbufferMtCandidate outer(resizer_, outer_target);
  EXPECT_EQ(
      outer.impact().reconnect_pins,
      std::vector<const sta::Pin*>{db_network_->findPin("path_target/ZN")});
}

TEST_F(TestUnbufferMt, ApplyRemovesTheBuffer)
{
  MoveCommitter committer(resizer_);
  UnbufferMtGenerator generator = makeGenerator(committer);
  Target target = makeTarget("path_out", "path_pre1", "Z");
  ASSERT_TRUE(generator.isApplicable(target));
  generator.prepareInputs(target);
  const std::vector<std::unique_ptr<MoveCandidate>> candidates
      = generator.generate(target);
  ASSERT_EQ(candidates.size(), 1u);

  // path_pre1 is gone and path_pre0 drives the NAND2 input directly.
  const MoveResult result = candidates.front()->apply();
  ASSERT_TRUE(result.accepted);
  EXPECT_EQ(result.type, MoveType::kUnbuffer);
  EXPECT_EQ(result.move_count, 1);
  EXPECT_EQ(result.touched_instances.size(), 1u);
  EXPECT_EQ(block_->findInst("path_pre1"), nullptr);
  sta::Network* network = sta_->network();
  EXPECT_EQ(network->net(db_network_->findPin("path_target/A1")),
            network->net(db_network_->findPin("path_pre0/Z")));
}

}  // namespace rsz
