// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// The packing problems mpl's annealer was asked on ORFS designs, and
// XiangShan's parent floorplan: wherever the annealer placed the macros,
// the exact check must decide the problem and find a legal placement too;
// wherever it proves that they do not fit, the proposed fix must fit. A
// problem the annealer could not place either may stay undecided.

#include <sstream>
#include <string>

#include "PackingTestUtil.h"
#include "gtest/gtest.h"
#include "packing.h"

namespace mpl {
namespace {

struct RealProblem
{
  const char* design;
  bool annealer_placed;
  const char* text;
};

const RealProblem kProblems[] = {
#include "RealDesigns.inc"
};

class RealDesigns : public ::testing::TestWithParam<RealProblem>
{
};

TEST_P(RealDesigns, DecidedAndConsistentWithTheAnnealer)
{
  std::istringstream in(GetParam().text);
  std::string tag;
  PackingProblem problem;
  ASSERT_TRUE(readPackingProblem(in, tag, problem));
  // A problem the annealer could not place asserts nothing when it stays
  // undecided, so it gets a small budget.
  PackingLimits limits;
  if (!GetParam().annealer_placed) {
    limits.deterministic_time = 1.0;
  }
  const PackingResult result = checkPacking(problem, limits);
  if (GetParam().annealer_placed) {
    EXPECT_TRUE(isLegal(problem, result)) << tag;
  }
  // The proposed fix's proven end fits; a small budget keeps the search
  // quick, the unit tests cover its tightness.
  if (result.status == PackingStatus::kDoesNotFit) {
    PackingLimits quick;
    quick.deterministic_time = 1.0;
    const DimensionFix fix = smallestHeight(problem, 1, quick);
    if (fix.found) {
      PackingProblem fixed = problem;
      fixed.height = fix.upper;
      EXPECT_EQ(checkPacking(fixed).status, PackingStatus::kFits) << tag;
    }
  }
}

INSTANTIATE_TEST_SUITE_P(Problems,
                         RealDesigns,
                         ::testing::ValuesIn(kProblems),
                         [](const ::testing::TestParamInfo<RealProblem>& info) {
                           std::string name = std::string(info.param.design)
                                              + "_"
                                              + std::to_string(info.index);
                           for (char& c : name) {
                             if (!std::isalnum(static_cast<unsigned char>(c))) {
                               c = '_';
                             }
                           }
                           return name;
                         });

}  // namespace
}  // namespace mpl
