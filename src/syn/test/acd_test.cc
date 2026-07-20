// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include "flow/acd.h"

#include <cstdint>
#include <limits>
#include <memory>
#include <optional>

#include "db_sta/dbSta.hh"
#include "flow/target_index.h"
#include "gtest/gtest.h"
#include "sta/Scene.hh"
#include "sta/Transition.hh"
#include "syn/synthesis.h"
#include "tst/fixture.h"

int main(int argc, char** argv)
{
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

namespace syn {

namespace {

constexpr uint32_t kAndOrChain5 = 0xEEEAEAEA;

}  // namespace

class AcdTest : public tst::Fixture
{
 protected:
  void SetUp() override
  {
    Fixture::SetUp();
    readLiberty(getFilePath("_main/src/syn/test/cm_test_cells.lib"));
    synthesis_ = std::make_unique<Synthesis>(getDb(),
                                             getSta(),
                                             /*resizer=*/nullptr,
                                             getLogger());

    corner_ = getSta()->findScene("default");
    // A scalar delay model in use by cm_test_cells.lib ignores the input slew
    fixed_slew_[0] = fixed_slew_[1] = 0;

    cm::buildIndex(getSta()->network(), index_, getLogger(), *synthesis_);
    match_cache_.emplace(getLogger(), index_, acd::kMaxBoundVars);

    dparams_.corner = corner_;
    dparams_.fixed_slews[0] = fixed_slew_[0];
    dparams_.fixed_slews[1] = fixed_slew_[1];
    dparams_.nand_delay
        = acd::findDelayLowerBound(corner_, match_cache_->nand2(), fixed_slew_);
  }

  // A single-output problem for the and-or chain function, unpacked from
  // `kAndOrChain5` into the layout synthesize/evaluateFunction use.
  acd::SynthesisProblem makeAndOrChainProblem()
  {
    acd::TruthTable f({0, 1, 2, 3, 4}, 1);
    const int nminterms = 1 << 5;
    for (int m = 0; m < nminterms; m++) {
      f.setValue(0, m, (kAndOrChain5 >> m) & 1);
    }

    acd::SynthesisProblem problem(f);
    problem.outputs[0].critical = true;
    problem.outputs[0].criticality = 1.0;
    return problem;
  }

  // Model input `var` as arriving `arrival` seconds relative to the
  // output required time
  static void setArrival(acd::SynthesisProblem& p, int var, float arrival)
  {
    for (const auto* in_rf : sta::RiseFall::range()) {
      for (const auto* out_rf : sta::RiseFall::range()) {
        p.inputs[var].arrivals.atTransition(in_rf).atExit(0, out_rf) = arrival;
      }
    }
  }

  // Synthesize with the timing objective turned on hard, so the structure is
  // chosen for slack rather than area.
  acd::GateNetwork synthesizeFor(const acd::SynthesisProblem& problem)
  {
    acd::GateNetwork net;
    long long explores = 0;
    const bool ok = acd::synthesize(problem,
                                    *match_cache_,
                                    dparams_,
                                    getLogger(),
                                    net,
                                    std::numeric_limits<double>::infinity(),
                                    /*allow_lateral=*/true,
                                    &explores,
                                    /*effort=*/1e11f);
    EXPECT_TRUE(ok) << "synthesize failed to find any network";
    return net;
  }

  float slackOf(const acd::SynthesisProblem& problem,
                const acd::GateNetwork& net)
  {
    return acd::networkSlack(getLogger(), problem, net, corner_, fixed_slew_);
  }

  void expectComputesAndOrChain(const acd::GateNetwork& net)
  {
    const acd::TruthTable tt = acd::evaluateFunction(net);
    uint32_t bits = 0;
    for (int m = 0; m < (1 << 5); m++) {
      if (tt.value(0, m)) {
        bits |= uint32_t{1} << m;
      }
    }
    EXPECT_EQ(bits, kAndOrChain5)
        << "synthesized network does not compute a*b*c";
  }

  std::unique_ptr<Synthesis> synthesis_;
  const sta::Scene* corner_ = nullptr;
  float fixed_slew_[2] = {0.0f, 0.0f};
  cm::TargetIndex index_;
  std::optional<acd::MatchCache> match_cache_;
  acd::DelayEstimationParameters dparams_;
};

TEST_F(AcdTest, StructureReassociatesWithTiming)
{
  constexpr float kEarly = -1e-7f;  // ~100 ns of slack
  constexpr float kLate = 0.0f;     // no slack

  // Context where group c arrives late: wants c near the output, i.e. (a*b)*c.
  acd::SynthesisProblem c_late = makeAndOrChainProblem();
  for (int v = 0; v < 5; v++) {
    setArrival(c_late, v, kEarly);
  }
  setArrival(c_late, /*gc=*/4, kLate);

  // Context where group a arrives late: wants a near the output, i.e. a*(b*c).
  acd::SynthesisProblem a_late = makeAndOrChainProblem();
  for (int v = 0; v < 5; v++) {
    setArrival(a_late, v, kEarly);
  }
  setArrival(a_late, /*ga=*/0, kLate);

  const acd::GateNetwork net_c_late = synthesizeFor(c_late);
  const acd::GateNetwork net_a_late = synthesizeFor(a_late);

  // Whatever it built, it must still compute a * b * c.
  expectComputesAndOrChain(net_c_late);
  expectComputesAndOrChain(net_a_late);

  // Cross-evaluate: each network's slack under both timing contexts.
  const float c_ctx_c_net = slackOf(c_late, net_c_late);
  const float c_ctx_a_net = slackOf(c_late, net_a_late);
  const float a_ctx_a_net = slackOf(a_late, net_a_late);
  const float a_ctx_c_net = slackOf(a_late, net_c_late);

  // Each structure is strictly better in the timing context it was built for.
  const float margin = 0.5f * dparams_.nand_delay;
  EXPECT_GT(c_ctx_c_net, c_ctx_a_net + margin)
      << "with group c late, the (a*b)*c structure (c near the output) should "
         "win";
  EXPECT_GT(a_ctx_a_net, a_ctx_c_net + margin)
      << "with group a late, the a*(b*c) structure (a near the output) should "
         "win";
}

}  // namespace syn
