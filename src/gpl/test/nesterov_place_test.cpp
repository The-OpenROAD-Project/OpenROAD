// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// Global placement on small designs, checked by what a good placement must
// satisfy instead of by diffing the placed DEF:
//  - QoR limits: final HPWL and iteration count within a tolerance of the
//    recorded values, better or worse, and overflow within the target.
//  - Invariants: movable cells placed with their center in the core, fixed
//    cells unmoved, and HPWL well below a random spread of the same cells.
//  - Convergence path: overflow mostly falls, with only small upticks, and
//    the density penalty ramps up.
//  - Determinism: the same input gives the same placement, whatever the
//    thread count.
//  - Options: each option the flow uses changes what it should.

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <iterator>
#include <optional>
#include <ostream>
#include <random>
#include <regex>
#include <string>
#include <utility>
#include <vector>

#include "gpl/Replace.h"
#include "gtest/gtest.h"
#include "odb/db.h"
#include "odb/dbTypes.h"
#include "odb/defin.h"
#include "odb/geom.h"
#include "odb/util.h"
#include "tst/fixture.h"

namespace gpl {
namespace {

struct Design
{
  const char* def;
  const char* tech_lef;
  // nullptr when tech_lef holds the cells too.
  const char* lib_lef = nullptr;
};

const Design kGcd{"gcd.def", "nangate45.lef"};
const Design kInsetCore{"inset_core.def", "nangate45.lef"};

// A placed cell, to compare placements and to restore the input.
struct CellState
{
  odb::dbInst* inst;
  odb::Point location;
  odb::dbPlacementStatus status;

  bool operator==(const CellState& other) const
  {
    return inst == other.inst && location == other.location
           && status == other.status;
  }
};

std::ostream& operator<<(std::ostream& os, const CellState& cell)
{
  return os << cell.inst->getName() << " at (" << cell.location.x() << ", "
            << cell.location.y() << ")";
}

class NesterovPlaceTest : public tst::Fixture
{
 protected:
  void load(const Design& design)
  {
    const std::string dir = "_main/src/gpl/test/";
    odb::dbLib* lib = nullptr;
    if (design.lib_lef) {
      odb::dbTech* tech = loadTechLef("tech", dir + design.tech_lef);
      lib = loadLibaryLef(tech, "lib", dir + design.lib_lef);
    } else {
      lib = loadTechAndLib("tech", "lib", dir + design.tech_lef);
    }
    ASSERT_NE(lib, nullptr);
    odb::dbChip* chip = odb::dbChip::create(db_.get(), lib->getTech());
    std::vector<odb::dbLib*> libs{lib};
    odb::defin reader(db_.get(), &logger_);
    const std::string def = getFilePath(dir + design.def);
    ASSERT_TRUE(reader.readChip(libs, def.c_str(), chip));
    block_ = chip->getBlock();
  }

  // A placer that has not seen the design, as a separate command would be.
  Replace placer()
  {
    return Replace{db_.get(), sta_.get(), nullptr, nullptr, nullptr, &logger_};
  }

  // Runs global placement and returns what it logged.
  std::string placeAndLog(const PlaceOptions& options,
                          PlaceResult* result = nullptr)
  {
    logger_.redirectStringBegin();
    const PlaceResult r = placer().doPlace(1, options);
    if (result) {
      *result = r;
    }
    return logger_.redirectStringEnd();
  }

  int64_t hpwl() const { return odb::WireLengthEvaluator(block_).hpwl(); }

  std::vector<CellState> cells() const
  {
    std::vector<CellState> result;
    for (odb::dbInst* inst : block_->getInsts()) {
      result.push_back({inst, inst->getLocation(), inst->getPlacementStatus()});
    }
    return result;
  }

  static void restore(const std::vector<CellState>& saved)
  {
    for (const CellState& cell : saved) {
      cell.inst->setLocation(cell.location.x(), cell.location.y());
      cell.inst->setPlacementStatus(cell.status);
    }
  }

  // HPWL of the movable cells dropped at uniformly random spots in the core,
  // which spreads them about as evenly as global placement does but ignores
  // the nets.
  int64_t randomSpreadHpwl()
  {
    const std::vector<CellState> saved = cells();
    const odb::Rect core = block_->getCoreArea();
    std::mt19937 rng(1);
    for (odb::dbInst* inst : block_->getInsts()) {
      if (inst->isFixed()) {
        continue;
      }
      const odb::Rect box = inst->getBBox()->getBox();
      std::uniform_int_distribution<int> x(core.xMin(), core.xMax() - box.dx());
      std::uniform_int_distribution<int> y(core.yMin(), core.yMax() - box.dy());
      inst->setLocation(x(rng), y(rng));
    }
    const int64_t result = hpwl();
    restore(saved);
    return result;
  }

  // Area of all cells inside area.
  int64_t coveredArea(const odb::Rect& area) const
  {
    int64_t covered = 0;
    for (odb::dbInst* inst : block_->getInsts()) {
      const odb::Rect box = inst->getBBox()->getBox();
      if (area.overlaps(box)) {
        covered += area.intersect(box).area();
      }
    }
    return covered;
  }

  int movableCount() const
  {
    int count = 0;
    for (odb::dbInst* inst : block_->getInsts()) {
      count += !inst->isFixed();
    }
    return count;
  }

  // The target density the run logged (GPL-0023).
  static std::optional<float> targetDensity(const std::string& log)
  {
    const std::regex re(R"(Placement target density:\s+([0-9.]+))");
    std::smatch m;
    if (!std::regex_search(log, m, re)) {
      return std::nullopt;
    }
    return std::stof(m[1]);
  }

  odb::dbBlock* block_ = nullptr;
};

// ---------------------------------------------------------------------------
// QoR limits, invariants and convergence path, on each design and option set
// the flow exercises.

struct QorCase
{
  const char* name;
  Design design;
  PlaceOptions options;
  // Recorded results. A run must stay within kHpwlTolerance /
  // kIterationTolerance of them in both directions, so a gain beyond the
  // tolerance is recorded and kept.
  int64_t hpwl;
  int iterations;
};

void PrintTo(const QorCase& c, std::ostream* os)
{
  *os << c.name;
}

// Ordinary gpl changes move these HPWLs by under 0.5%.
constexpr double kHpwlTolerance = 0.01;
constexpr double kIterationTolerance = 0.2;

// Designs with enough cells for spreading and the convergence path to mean
// something; the rest are 1-6 cell edge cases.
constexpr int kMinCellsForQualityChecks = 100;

// The recorded results use an initial density penalty of 0.01, above the
// PlaceOptions default of 0.00008; a case that wants another passes it.
PlaceOptions withoutInitialPlace(const float density_penalty = 0.01f)
{
  PlaceOptions options;
  options.initDensityPenaltyFactor = density_penalty;
  options.initialPlaceMaxIter = 0;
  return options;
}

PlaceOptions withDensity(PlaceOptions options, const float density)
{
  options.density = density;
  return options;
}

PlaceOptions coarseBins()
{
  PlaceOptions options = withDensity({}, 0.75f);
  options.binGridCntX = 64;
  options.binGridCntY = 64;
  options.overflow = 0.2f;
  return options;
}

PlaceOptions uniformDensity()
{
  PlaceOptions options = withoutInitialPlace();
  options.uniformTargetDensityMode = true;
  return options;
}

PlaceOptions referenceHpwl()
{
  PlaceOptions options = withoutInitialPlace();
  options.referenceHpwl = 384000;
  return options;
}

PlaceOptions skipIo()
{
  PlaceOptions options;
  options.skipIo();
  return options;
}

std::vector<QorCase> qorCases()
{
  const Design one_movable_cell{"one_movable_cell.def", "sky130hd.lef"};
  return {
      // 294-cell gcd, 65% utilization, IOs on all four sides.
      {"Basic", kGcd, withoutInitialPlace(), 5195638, 235},
      {"WithInitialPlace", kGcd, {}, 5200644, 357},
      {"MovedIos", {"moved_ios.def", "nangate45.lef"}, {}, 5299387, 363},
      // A row one site short adds a dummy cell; the target is raised to fit.
      {"DensityBelowUtilization",
       {"density_below_utilization.def", "nangate45.lef"},
       withDensity(withoutInitialPlace(), 0.6f),
       5361141,
       240},
      // The core is inset from the die.
      {"InsetCore",
       kInsetCore,
       withDensity(withoutInitialPlace(), 0.6f),
       5770600,
       236},
      {"TallCore",
       {"tall_core.def", "nangate45.lef"},
       withDensity(withoutInitialPlace(), 0.6f),
       5989080,
       236},
      {"WideCore",
       {"wide_core.def", "nangate45.lef"},
       withDensity(withoutInitialPlace(), 0.6f),
       5905140,
       232},
      // A hard and a 50% partial placement blockage.
      {"Blockages",
       {"blockages.def", "nangate45.lef"},
       withDensity(withoutInitialPlace(), 0.8f),
       5432148,
       239},
      {"UniformDensity", kGcd, uniformDensity(), 5327360, 240},
      {"ReferenceHpwl", kGcd, referenceHpwl(), 5199938, 242},
      {"SkipIo", kGcd, skipIo(), 6002435, 376},
      // One cell, two nets, in a 2x2 bin grid.
      {"SingleCellSmallCore",
       {"single_cell_small_core.def", "nangate45.lef"},
       withoutInitialPlace(0.00008f),
       28787,
       0},
      // One cell with a density penalty high enough to diverge a real
      // design.
      {"SingleCellHighPenalty",
       {"single_cell_high_penalty.def", "nangate45.lef"},
       withoutInitialPlace(1.0f),
       9989,
       0},
      // One movable inverter between two IO pins, among 31 fixed tap and
      // decap cells with no signal nets.
      {"OneMovableCell", one_movable_cell, withDensity({}, 0.75f), 84481, 0},
      {"OneMovableCellCoarseBins", one_movable_cell, coarseBins(), 84481, 0},
      // One movable cell in an 8 mm die: a 2048 x 2048 bin grid.
      {"OneCellHugeDie",
       {"one_cell_huge_die.def", "sky130hd.lef"},
       withDensity({}, 0.75f),
       2065311,
       0},
      // Six buffers, each driven by one IO pin and driving another. With
      // skip_io every net has a single pin, so wirelength gives no gradient
      // and the density penalty alone has to start the spreading.
      {"NetsWithoutGradient",
       {"nets_without_gradient.def",
        "asap7/asap7_tech_1x_201209.lef",
        "asap7/asap7sc7p5t_28_R_1x_220121a.lef"},
       skipIo(),
       4560638,
       2},
  };
}

class QorTest : public NesterovPlaceTest,
                public testing::WithParamInterface<QorCase>
{
 protected:
  void expectInvariants(const std::vector<CellState>& before)
  {
    const odb::Rect core = block_->getCoreArea();
    for (const CellState& cell : before) {
      odb::dbInst* inst = cell.inst;
      if (cell.status.isFixed()) {
        EXPECT_EQ(inst->getPlacementStatus(), cell.status) << inst->getName();
        EXPECT_EQ(inst->getLocation(), cell.location)
            << inst->getName() << " is fixed";
        continue;
      }
      EXPECT_TRUE(inst->isPlaced()) << inst->getName();
      // Global placement keeps a cell's density footprint in the core. For
      // wide cells that is a little narrower than the cell, so only the
      // center is sure to be inside.
      const odb::Point center = inst->getBBox()->getBox().center();
      EXPECT_TRUE(core.intersects(center))
          << inst->getName() << " center (" << center.x() << ", " << center.y()
          << ") outside the core";
    }
  }

  static void expectConvergencePath(const PlaceResult& result)
  {
    const std::vector<PlaceIteration>& trace = result.trace;
    ASSERT_EQ(std::ssize(trace), result.final_iteration + 1);
    int upticks = 0;
    for (int i = 1; i < std::ssize(trace); ++i) {
      const float rise = trace[i].overflow - trace[i - 1].overflow;
      if (rise > 0) {
        ++upticks;
        EXPECT_LT(rise, 0.005f) << "iteration " << trace[i].iteration;
      }
    }
    EXPECT_LE(upticks, std::ssize(trace) / 4);
    EXPECT_GT(trace.back().density_penalty, trace.front().density_penalty);
  }
};

TEST_P(QorTest, PlacesWithinLimits)
{
  const QorCase& c = GetParam();
  ASSERT_NO_FATAL_FAILURE(load(c.design));
  const std::vector<CellState> before = cells();

  Replace replace = placer();
  replace.doInitialPlace(1, c.options);
  const int64_t initial_hpwl = hpwl();
  const PlaceResult result = replace.doNesterovPlace(1, c.options);

  EXPECT_EQ(result.stop, PlaceStop::kConverged);
  EXPECT_LE(result.overflow, c.options.overflow);
  EXPECT_EQ(result.hpwl, hpwl());
  const std::string record = "Record " + std::to_string(result.hpwl) + ", "
                             + std::to_string(result.final_iteration)
                             + " as this case's HPWL and iterations.";
  EXPECT_LE(result.hpwl, c.hpwl * (1 + kHpwlTolerance)) << record;
  EXPECT_GE(result.hpwl, c.hpwl * (1 - kHpwlTolerance)) << record;
  EXPECT_LE(
      result.final_iteration,
      std::max(c.iterations * (1 + kIterationTolerance), c.iterations + 5.0))
      << record;
  EXPECT_GE(
      result.final_iteration,
      std::min(c.iterations * (1 - kIterationTolerance), c.iterations - 5.0))
      << record;
  expectInvariants(before);

  if (movableCount() < kMinCellsForQualityChecks) {
    return;
  }
  // Spreading costs wirelength, but not much more than the clumped initial
  // placement, and far less than ignoring the nets.
  EXPECT_LT(result.hpwl, 2 * initial_hpwl);
  EXPECT_LT(result.hpwl, 0.5 * randomSpreadHpwl());
  expectConvergencePath(result);
}

INSTANTIATE_TEST_SUITE_P(Designs,
                         QorTest,
                         testing::ValuesIn(qorCases()),
                         [](const testing::TestParamInfo<QorCase>& info) {
                           return info.param.name;
                         });

// ---------------------------------------------------------------------------
// Determinism.

TEST_F(NesterovPlaceTest, SameInputGivesTheSamePlacement)
{
  ASSERT_NO_FATAL_FAILURE(load(kInsetCore));
  const PlaceOptions options = withDensity(withoutInitialPlace(), 0.6f);
  const std::vector<CellState> input = cells();

  placer().doPlace(1, options);
  const std::vector<CellState> first = cells();
  restore(input);
  placer().doPlace(1, options);

  EXPECT_EQ(cells(), first);
}

TEST_F(NesterovPlaceTest, ThreadCountDoesNotChangeThePlacement)
{
  ASSERT_NO_FATAL_FAILURE(load(kInsetCore));
  const PlaceOptions options = withDensity(withoutInitialPlace(), 0.6f);
  const std::vector<CellState> input = cells();

  placer().doPlace(1, options);
  const std::vector<CellState> single = cells();
  restore(input);
  placer().doPlace(8, options);

  EXPECT_EQ(cells(), single);
}

// ---------------------------------------------------------------------------
// Options.

// A target below what the cells need is raised to fit them, with a warning.
TEST_F(NesterovPlaceTest, TargetDensityBelowUtilizationIsRaised)
{
  ASSERT_NO_FATAL_FAILURE(
      load({"density_below_utilization.def", "nangate45.lef"}));
  const std::string log = placeAndLog(withDensity(withoutInitialPlace(), 0.6f));

  EXPECT_NE(log.find("GPL-0302"), std::string::npos);
  const std::optional<float> target = targetDensity(log);
  ASSERT_TRUE(target);
  EXPECT_GT(*target, 0.6f);
}

// Uniform density targets the design's own utilization instead of the
// default 0.7.
TEST_F(NesterovPlaceTest, UniformDensityTargetsTheUtilization)
{
  ASSERT_NO_FATAL_FAILURE(load(kGcd));
  const float utilization
      = placer().getUniformTargetDensity(uniformDensity(), 1);
  const std::string log = placeAndLog(uniformDensity());

  const std::optional<float> target = targetDensity(log);
  ASSERT_TRUE(target);
  EXPECT_GE(*target, utilization);
  EXPECT_LT(*target, PlaceOptions{}.density);
}

// Padding widens every cell, so the uniform density counts it. On gcd
// each site of padding per side adds about 0.2.
TEST_F(NesterovPlaceTest, UniformDensityIncludesPadding)
{
  ASSERT_NO_FATAL_FAILURE(load(kGcd));
  std::vector<float> densities;
  for (const int pad : {0, 1, 2}) {
    PlaceOptions options = withoutInitialPlace();
    options.padLeft = pad;
    options.padRight = pad;
    densities.push_back(placer().getUniformTargetDensity(options, 1));
  }

  EXPECT_NEAR(densities[0], 0.650f, 0.001f);
  EXPECT_NEAR(densities[1], 0.850f, 0.001f);
  EXPECT_NEAR(densities[2], 1.030f, 0.001f);
}

TEST_F(NesterovPlaceTest, BinGridCountIsUsed)
{
  ASSERT_NO_FATAL_FAILURE(load({"one_movable_cell.def", "sky130hd.lef"}));
  PlaceResult result;
  const std::string log = placeAndLog(coarseBins(), &result);

  EXPECT_TRUE(std::regex_search(
      log, std::regex(R"(Bin count \(X, Y\):\s+64\s*,\s*64)")))
      << log;
  // The looser 0.2 overflow target lets the run stop above the default 0.1.
  EXPECT_GT(result.overflow, PlaceOptions{}.overflow);
  EXPECT_LE(result.overflow, 0.2f);
}

// Cells stay out of a hard placement blockage, apart from the slight overlap
// global placement leaves for detailed placement to remove.
TEST_F(NesterovPlaceTest, CellsStayOutOfHardBlockages)
{
  ASSERT_NO_FATAL_FAILURE(load({"blockages.def", "nangate45.lef"}));
  placer().doPlace(1, withDensity(withoutInitialPlace(), 0.8f));

  int hard = 0;
  for (odb::dbBlockage* blockage : block_->getBlockages()) {
    if (blockage->isSoft() || blockage->getMaxDensity() > 0) {
      continue;
    }
    ++hard;
    const odb::Rect area = blockage->getBBox()->getBox();
    EXPECT_LT(coveredArea(area), 0.02 * area.area());
  }
  EXPECT_EQ(hard, 1);
}

// A partial blockage caps the cell density under it near its max density.
// Ignoring the blockage here leaves it about 70% covered.
TEST_F(NesterovPlaceTest, PartialBlockageLimitsDensity)
{
  ASSERT_NO_FATAL_FAILURE(load({"blockages.def", "nangate45.lef"}));
  const PlaceOptions options = withDensity(withoutInitialPlace(), 0.8f);
  placer().doPlace(1, options);

  int partial = 0;
  for (odb::dbBlockage* blockage : block_->getBlockages()) {
    if (blockage->getMaxDensity() <= 0) {
      continue;
    }
    ++partial;
    const odb::Rect area = blockage->getBBox()->getBox();
    const double density = static_cast<double>(coveredArea(area)) / area.area();
    // Max density is a percentage.
    EXPECT_LT(density, blockage->getMaxDensity() / 100.0 + 0.05);
  }
  EXPECT_EQ(partial, 1);
}

// Initial placement alone places every cell and pulls connected cells
// together; on gcd it ends near 0.16 of a random spread's HPWL.
TEST_F(NesterovPlaceTest, InitialPlaceFollowsTheNets)
{
  ASSERT_NO_FATAL_FAILURE(load(kGcd));
  placer().doInitialPlace(1, PlaceOptions{});

  for (odb::dbInst* inst : block_->getInsts()) {
    EXPECT_TRUE(inst->isPlaced()) << inst->getName();
  }
  EXPECT_LT(hpwl(), 0.25 * randomSpreadHpwl());
}

// With -skip_io the nets to IO pins are ignored, so moving the pins does not
// move any cell.
TEST_F(NesterovPlaceTest, SkipIoIgnoresPinLocations)
{
  ASSERT_NO_FATAL_FAILURE(load(kGcd));
  const std::vector<CellState> input = cells();
  placer().doPlace(1, skipIo());
  const std::vector<CellState> placed = cells();

  restore(input);
  // Reflect every pin through the die center.
  const odb::Rect die = block_->getDieArea();
  for (odb::dbBTerm* bterm : block_->getBTerms()) {
    for (odb::dbBPin* pin : bterm->getBPins()) {
      std::vector<std::pair<odb::dbTechLayer*, odb::Rect>> shapes;
      for (odb::dbBox* box : pin->getBoxes()) {
        shapes.emplace_back(box->getTechLayer(), box->getBox());
      }
      for (auto it = pin->getBoxes().begin(); it != pin->getBoxes().end();) {
        odb::dbBox* box = *it;
        ++it;
        odb::dbBox::destroy(box);
      }
      for (const auto& [layer, r] : shapes) {
        odb::dbBox::create(pin,
                           layer,
                           die.xMax() + die.xMin() - r.xMax(),
                           die.yMax() + die.yMin() - r.yMax(),
                           die.xMax() + die.xMin() - r.xMin(),
                           die.yMax() + die.yMin() - r.yMin());
      }
    }
  }
  placer().doPlace(1, skipIo());

  EXPECT_EQ(cells(), placed);
}

// The density penalty ramp slows in proportion to the HPWL change over the
// reference HPWL. A reference far below the design's HPWL eases the ramp at
// some iterations; by default this design's penalty always grows at the
// full rate.
TEST_F(NesterovPlaceTest, ReferenceHpwlEasesThePenaltyRamp)
{
  auto slowestGrowth = [](const PlaceResult& result) {
    float slowest = INFINITY;
    for (int i = 1; i < std::ssize(result.trace); ++i) {
      slowest = std::min(slowest,
                         result.trace[i].density_penalty
                             / result.trace[i - 1].density_penalty);
    }
    return slowest;
  };
  ASSERT_NO_FATAL_FAILURE(load(kGcd));
  const std::vector<CellState> input = cells();
  const PlaceResult by_default = placer().doPlace(1, withoutInitialPlace());
  restore(input);
  const PlaceResult with_reference = placer().doPlace(1, referenceHpwl());

  EXPECT_LT(slowestGrowth(with_reference), 1.0f);
  EXPECT_GT(slowestGrowth(by_default), 1.0f);
}

}  // namespace
}  // namespace gpl
