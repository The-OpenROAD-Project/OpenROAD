// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// How global placement's Nesterov descent ends.
//
// It ends on Nesterov's iterate, not on the look-ahead point one momentum
// step past it.
//
// Each iteration takes a gradient step from the look-ahead v_k to the
// iterate u_(k+1), then extrapolates the look-ahead
// v_(k+1) = u_(k+1) + c_k (u_(k+1) - u_k), with c_k close to 1 late in a
// run. The gradient is evaluated at v; the iterate u is the sequence the
// method converges. A placement that ends on v carries one more step of the
// cells' motion than the descent has taken, and its wirelength with it.

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "gpl/Replace.h"
#include "gtest/gtest.h"
#include "odb/db.h"
#include "odb/dbTypes.h"
#include "odb/defin.h"
#include "odb/geom.h"
#include "odb/lefin.h"
#include "openroad/gpl/src/graphicsNone.h"
#include "openroad/gpl/src/nesterovBase.h"
#include "openroad/gpl/src/nesterovPlace.h"
#include "openroad/gpl/src/placerBase.h"
#include "openroad/gpl/src/point.h"
#include "tst/fixture.h"
#include "utl/deleter.h"

namespace gpl {

class EndOfDescentTest : public tst::Fixture
{
 protected:
  // gcd (294 cells, 65 % utilization) with every movable cell starting at
  // the core's center: a descent of a few hundred iterations, still moving
  // when the overflow reaches the target.
  struct Placer
  {
    // Declared first, so that it outlives the placer that points into it.
    utl::UniquePtrWithDeleter<odb::dbDatabase> db;
    std::shared_ptr<PlacerBaseCommon> pbc;
    std::shared_ptr<NesterovBaseCommon> nbc;
    std::vector<std::shared_ptr<PlacerBase>> pb_vec;
    std::vector<std::shared_ptr<NesterovBase>> nb_vec;
    std::unique_ptr<NesterovPlace> np;

    NesterovBase& nb() const { return *nb_vec.front(); }
  };

  // gcd in a database of its own and a placer for it. Two placers made with
  // the same options run the same descent.
  std::unique_ptr<Placer> makePlacer(const PlaceOptions& options)
  {
    auto placer = std::make_unique<Placer>();
    placer->db = utl::UniquePtrWithDeleter<odb::dbDatabase>(
        odb::dbDatabase::create(), odb::dbDatabase::destroy);
    placer->db->setLogger(&logger_);
    const std::string dir = "_main/openroad/gpl/test/";
    odb::lefin lef_reader(
        placer->db.get(), &logger_, /* ignore_non_routing_layers = */ false);
    odb::dbLib* lib = lef_reader.createTechAndLib(
        "tech", "lib", getFilePath(dir + "nangate45.lef").c_str());
    if (lib == nullptr) {
      ADD_FAILURE() << "nangate45.lef did not load";
      return nullptr;
    }
    odb::dbChip* chip = odb::dbChip::create(placer->db.get(), lib->getTech());
    std::vector<odb::dbLib*> libs{lib};
    odb::defin def_reader(placer->db.get(), &logger_);
    if (!def_reader.readChip(
            libs, getFilePath(dir + "gcd.def").c_str(), chip)) {
      ADD_FAILURE() << "gcd.def did not load";
      return nullptr;
    }
    odb::dbBlock* block = chip->getBlock();
    const odb::Point center = block->getCoreArea().center();
    for (odb::dbInst* inst : block->getInsts()) {
      if (!inst->isFixed()) {
        inst->setLocation(center.x(), center.y());
        inst->setPlacementStatus(odb::dbPlacementStatus::PLACED);
      }
    }

    const PlacerBaseVars pb_vars(options);
    placer->pbc = std::make_shared<PlacerBaseCommon>(
        placer->db.get(), pb_vars, &logger_);
    placer->pb_vec.push_back(std::make_shared<PlacerBase>(
        placer->db.get(), placer->pbc, &logger_, /* check_density = */ false));
    const NesterovBaseVars nb_vars(options);
    placer->nbc = std::make_shared<NesterovBaseCommon>(
        nb_vars, placer->pbc, &logger_, /* num_threads = */ 1, Clusters{});
    placer->nb_vec.push_back(std::make_shared<NesterovBase>(
        nb_vars, placer->pb_vec[0], placer->nbc, &logger_));
    const NesterovPlaceVars np_vars(options, placer->nbc->getHpwl());
    placer->np
        = std::make_unique<NesterovPlace>(np_vars,
                                          placer->pbc,
                                          placer->nbc,
                                          placer->pb_vec,
                                          placer->nb_vec,
                                          /* rb = */ nullptr,
                                          /* tb = */ nullptr,
                                          /* cb = */ nullptr,
                                          /* pwb = */ nullptr,
                                          std::make_unique<GraphicsNone>(),
                                          &logger_);
    return placer;
  }

  // A twin of a run, stopped by the iteration limit right after `iteration`:
  // at a target overflow of 0 nothing converges or settles, so its iterate
  // and look-ahead are the descent's own. It does not revert to the minimum
  // HPWL snapshot at the limit, which a run that converges never does either.
  std::unique_ptr<Placer> runTwin(const int iteration)
  {
    PlaceOptions options = options_;
    options.overflow = 0;
    options.nesterovPlaceMaxIter = iteration + 1;
    options.disableRevertIfDiverge = true;
    std::unique_ptr<Placer> twin = makePlacer(options);
    if (twin != nullptr) {
      twin_result_ = twin->np->doNesterovPlace();
    }
    return twin;
  }

  static const std::vector<FloatPoint>& iterate(const Placer& placer)
  {
    return placer.nb().curCoordi_;
  }
  static const std::vector<FloatPoint>& lookAhead(const Placer& placer)
  {
    return placer.nb().curSLPCoordi_;
  }

  // The HPWL of a placer's iterate, with the density put there.
  static int64_t iterateHpwl(const Placer& placer)
  {
    placer.nb().updateGCellDensityCenterLocation(placer.nb().curCoordi_);
    return placer.nbc->getHpwl();
  }

  // The largest distance, in DBU on either axis, between an instance's
  // density center in `placer` and `coordis`.
  static float worstInstanceOffset(const Placer& placer,
                                   const std::vector<FloatPoint>& coordis)
  {
    const NesterovBase& nb = placer.nb();
    float worst = 0;
    for (size_t i = 0; i < nb.nb_gcells_.size(); ++i) {
      const GCell* gcell = nb.nb_gcells_[i];
      if (!gcell->isInstance()) {
        continue;
      }
      worst = std::max({worst,
                        std::abs(gcell->dCx() - coordis[i].x),
                        std::abs(gcell->dCy() - coordis[i].y)});
    }
    return worst;
  }

  // How many gcells' look-ahead is not their iterate.
  static int lookAheadsApart(const Placer& placer)
  {
    const NesterovBase& nb = placer.nb();
    int apart = 0;
    for (size_t i = 0; i < nb.nb_gcells_.size(); ++i) {
      if (nb.curSLPCoordi_[i].x != nb.curCoordi_[i].x
          || nb.curSLPCoordi_[i].y != nb.curCoordi_[i].y) {
        ++apart;
      }
    }
    return apart;
  }

  PlaceOptions options_;
  PlaceResult twin_result_;
};

// The placement gpl hands back, the one it measured the overflow and the
// wirelength of, is the iterate the descent reached on its last iteration:
// the one a twin of the run, stopped there, has.
TEST_F(EndOfDescentTest, ConvergedPlacementIsTheIterate)
{
  const std::unique_ptr<Placer> run = makePlacer(options_);
  ASSERT_NE(run, nullptr);
  const PlaceResult result = run->np->doNesterovPlace();
  ASSERT_EQ(result.stop, PlaceStop::kConverged);
  EXPECT_LE(result.overflow, options_.overflow);
  ASSERT_GE(result.final_iteration, 50) << "a descent long enough for the "
                                           "momentum to have built up";

  const std::unique_ptr<Placer> twin = runTwin(result.final_iteration);
  ASSERT_NE(twin, nullptr);
  ASSERT_EQ(twin_result_.stop, PlaceStop::kIterationLimit);
  ASSERT_EQ(twin_result_.trace.size(), result.trace.size());
  for (size_t i = 0; i + 1 < result.trace.size(); ++i) {
    ASSERT_EQ(twin_result_.trace[i].hpwl, result.trace[i].hpwl)
        << "the twin must run the same descent, iteration "
        << result.trace[i].iteration;
  }
  ASSERT_GT(lookAheadsApart(*twin), 0)
      << "the descent's last look-ahead must differ from its iterate for "
         "this test to tell them apart";

  // Coordinates are rounded to DBU when the gcells take them.
  EXPECT_LE(worstInstanceOffset(*run, iterate(*twin)), 1.0f)
      << "every instance must sit on the descent's last iterate";
  EXPECT_GT(worstInstanceOffset(*run, lookAhead(*twin)), 1.0f)
      << "the placement is the look-ahead point, a momentum step past the "
         "iterate";
  // The look-ahead was brought back to the iterate.
  EXPECT_EQ(lookAheadsApart(*run), 0)
      << "gcells whose look-ahead is not their iterate";
}

// The final row of the trace, the one convergence was judged on, has the
// iterate's wirelength, and the overflow the run reports.
TEST_F(EndOfDescentTest, TraceEndsOnTheIterate)
{
  const std::unique_ptr<Placer> run = makePlacer(options_);
  ASSERT_NE(run, nullptr);
  const PlaceResult result = run->np->doNesterovPlace();
  ASSERT_EQ(result.stop, PlaceStop::kConverged);
  ASSERT_FALSE(result.trace.empty());
  EXPECT_EQ(result.trace.back().overflow, result.overflow);
  EXPECT_EQ(result.trace.back().hpwl, run->nbc->getHpwl());

  const std::unique_ptr<Placer> twin = runTwin(result.final_iteration);
  ASSERT_NE(twin, nullptr);
  ASSERT_EQ(twin_result_.stop, PlaceStop::kIterationLimit);
  const int64_t look_ahead_hpwl = twin_result_.trace.back().hpwl;
  const int64_t iterate_hpwl = iterateHpwl(*twin);
  ASSERT_NE(look_ahead_hpwl, iterate_hpwl);
  EXPECT_EQ(result.trace.back().hpwl, iterate_hpwl)
      << "the last row must measure the iterate, not the look-ahead ("
      << look_ahead_hpwl << ")";
}

}  // namespace gpl
