#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <numeric>
#include <string>
#include <unordered_map>
#include <vector>

#include "ScanArchitect.hh"
#include "ScanReplace.hh"
#include "gtest/gtest.h"
#include "sta/Liberty.hh"
#include "tst/fixture.h"

namespace dft {
namespace {

bool operator!=(const ScanArchitect::HashDomainLimits& lhs,
                const ScanArchitect::HashDomainLimits& rhs)
{
  return lhs.chain_count != rhs.chain_count || lhs.max_length != rhs.max_length;
}

template <typename T>
bool CompareUnordered(const T& u1, const T& u2)
{
  for (const auto& [key, value] : u1) {
    auto it = u2.find(key);
    if (it == u2.end()) {
      return false;
    }
    if (it->second != value) {
      return false;
    }
  }
  return true;
}

ScanArchitect::HashDomainLimits CreateTestHashDomainLimits(uint64_t chain_count,
                                                           uint64_t max_length)
{
  ScanArchitect::HashDomainLimits limits;
  limits.chain_count = chain_count;
  limits.max_length = max_length;
  return limits;
}

// Check if we are correctly calculating the limits of the hash domains
TEST(TestScanArchitect, CalculatesChainCountAndMaxLength)
{
  // For 1 hash domain (1) with 10 bits and a max length of 5, we need to have a
  // chain count of 2 in that hash domain (1)
  EXPECT_TRUE(CompareUnordered(
      ScanArchitect::inferChainCountFromMaxLength({{1, 10}}, 5, {}),
      {{1, CreateTestHashDomainLimits(2, 5)}}));
  // Unbalance cases
  // We create 2 chains with max_length of 4
  EXPECT_TRUE(CompareUnordered(
      ScanArchitect::inferChainCountFromMaxLength({{1, 9}}, 5, {}),
      {{1, CreateTestHashDomainLimits(2, 5)}}));
  // We create 2 chains with max_length of 3
  EXPECT_TRUE(CompareUnordered(
      ScanArchitect::inferChainCountFromMaxLength({{1, 6}}, 5, {}),
      {{1, CreateTestHashDomainLimits(2, 3)}}));

  // Two hash domains, balance case
  EXPECT_TRUE(CompareUnordered(
      ScanArchitect::inferChainCountFromMaxLength({{1, 10}, {2, 10}}, 5, {}),
      {{1, CreateTestHashDomainLimits(2, 5)},
       {2, CreateTestHashDomainLimits(2, 5)}}));
  // Unbalance
  EXPECT_TRUE(CompareUnordered(
      ScanArchitect::inferChainCountFromMaxLength({{1, 10}, {2, 9}}, 5, {}),
      {{1, CreateTestHashDomainLimits(2, 5)},
       {2, CreateTestHashDomainLimits(2, 5)}}));

  //// Two hash domains with different number of cells
  EXPECT_TRUE(CompareUnordered(
      ScanArchitect::inferChainCountFromMaxLength({{1, 10}, {2, 15}}, 5, {}),
      {{1, CreateTestHashDomainLimits(2, 5)},
       {2, CreateTestHashDomainLimits(3, 5)}}));

  // Constrained max chain count
  EXPECT_TRUE(CompareUnordered(
      ScanArchitect::inferChainCountFromMaxLength({{1, 10}}, 5, {1}),
      {{1, CreateTestHashDomainLimits(1, 10)}}));
}

// Verify single-character 's'/'S' scan insertion matching across leading,
// library-prefixed (infix), and trailing naming conventions.
TEST(TestScanReplace, IsSingleCharScanInsertion)
{
  // Leading 'S' / 's' prefix convention.
  EXPECT_TRUE(IsSingleCharScanInsertion("DFF_X1", "SDFF_X1"));
  EXPECT_TRUE(IsSingleCharScanInsertion("dfxtp", "sdfxtp"));
  EXPECT_TRUE(IsSingleCharScanInsertion("dfxtp_1", "sdfxtp_1"));

  // Infix 's' after library prefix (e.g. Sky130 standard cells).
  EXPECT_TRUE(IsSingleCharScanInsertion("sky130_fd_sc_hd__dfxtp_1",
                                        "sky130_fd_sc_hd__sdfxtp_1"));

  // Trailing 'S' / 's' suffix convention.
  EXPECT_TRUE(IsSingleCharScanInsertion("DFF_X1", "DFF_X1S"));

  // Negative cases: multi-character insertions, non-'s' insertions, or equal
  // strings.
  EXPECT_FALSE(IsSingleCharScanInsertion("dfxtp", "sdfbbp"));
  EXPECT_FALSE(IsSingleCharScanInsertion("sky130_fd_sc_hd__dfxtp_1",
                                         "sky130_fd_sc_hd__sdfxtp_1_lp"));
  EXPECT_FALSE(IsSingleCharScanInsertion("DFF_X1", "XDFF_X1"));
  EXPECT_FALSE(IsSingleCharScanInsertion("DFF_X1", "DFF_X1"));
}

// Verify Liberty cell_footprint matching accepts exact equality and
// single-char 's'/'S' insertion while rejecting empty or unrelated footprints.
TEST(TestScanReplace, IsMatchingFootprint)
{
  EXPECT_TRUE(IsMatchingFootprint("dfxtp", "dfxtp"));
  EXPECT_TRUE(IsMatchingFootprint("dfxtp", "sdfxtp"));
  EXPECT_TRUE(IsMatchingFootprint("DFF_X1", "SDFF_X1"));

  EXPECT_FALSE(IsMatchingFootprint("", ""));
  EXPECT_FALSE(IsMatchingFootprint("dfxtp", ""));
  EXPECT_FALSE(IsMatchingFootprint("", "sdfxtp"));
  EXPECT_FALSE(IsMatchingFootprint("dfxtp", "sdfbbp"));
}

bool IsBetterCandidateRank(const CandidateRank& lhs, const CandidateRank& rhs)
{
  return SelectBestCandidateRankIndex({lhs, rhs}) == 0
         && SelectBestCandidateRankIndex({rhs, lhs}) == 1;
}

// Verify footprint match takes highest priority, followed by same
// LibertyLibrary (VT family) affinity.
TEST(TestScanReplace, IsBetterCandidateRankPrioritizesFootprintAndLibrary)
{
  const CandidateRank matching_footprint{
      .footprint_match = true,
      .same_library = true,
      .rel_drive_diff = 0.02,
      .area = 20.0f,
      .direct_name_match = true,
      .name = "sky130_fd_sc_hd__sdfxtp_1",
  };
  const CandidateRank mismatched_footprint{
      .footprint_match = false,
      .same_library = true,
      .rel_drive_diff = 0.001,
      .area = 19.0f,
      .direct_name_match = false,
      .name = "sky130_fd_sc_hd__sdfbbp_1",
  };
  EXPECT_TRUE(IsBetterCandidateRank(matching_footprint, mismatched_footprint));
  EXPECT_FALSE(IsBetterCandidateRank(mismatched_footprint, matching_footprint));

  const CandidateRank cross_library{
      .footprint_match = true,
      .same_library = false,
      .rel_drive_diff = 0.001,
      .area = 18.0f,
      .direct_name_match = true,
      .name = "sky130_fd_sc_hs__sdfxtp_1",
  };
  EXPECT_TRUE(IsBetterCandidateRank(matching_footprint, cross_library));
  EXPECT_FALSE(IsBetterCandidateRank(cross_library, matching_footprint));
}

// Verify drive resistance differences > 1% win, whereas sub-1% differences
// defer to smaller cell area.
TEST(TestScanReplace,
     IsBetterCandidateRankPrefersSmallerAreaWithinDriveTolerance)
{
  const CandidateRank compact_std_scan_flop{
      .footprint_match = true,
      .same_library = true,
      .rel_drive_diff = 0.004,
      .area = 20.0f,
      .direct_name_match = true,
      .name = "sky130_fd_sc_hd__sdfxtp_1",
  };
  const CandidateRank wider_variant_scan_flop{
      .footprint_match = true,
      .same_library = true,
      .rel_drive_diff = 0.002,
      .area = 21.25f,
      .direct_name_match = false,
      .name = "sky130_fd_sc_hd__sdfxtp_1_lp",
  };
  EXPECT_TRUE(
      IsBetterCandidateRank(compact_std_scan_flop, wider_variant_scan_flop));
  EXPECT_FALSE(
      IsBetterCandidateRank(wider_variant_scan_flop, compact_std_scan_flop));

  const CandidateRank wrong_drive_strength{
      .footprint_match = true,
      .same_library = true,
      .rel_drive_diff = 0.40,
      .area = 17.5f,
      .direct_name_match = false,
      .name = "sky130_fd_sc_hd__sdfxtp_2",
  };
  EXPECT_TRUE(
      IsBetterCandidateRank(compact_std_scan_flop, wrong_drive_strength));
  EXPECT_FALSE(
      IsBetterCandidateRank(wrong_drive_strength, compact_std_scan_flop));
}

// Verify tie-breaking by direct single-character 's'/'S' name insertion, then
// lexicographical cell name.
TEST(TestScanReplace,
     IsBetterCandidateRankTieBreaksByDirectNameThenLexicographic)
{
  const CandidateRank direct_name{
      .footprint_match = true,
      .same_library = true,
      .rel_drive_diff = 0.002,
      .area = 20.0f,
      .direct_name_match = true,
      .name = "sky130_fd_sc_hd__sdfxtp_1_b",
  };
  const CandidateRank non_direct_name{
      .footprint_match = true,
      .same_library = true,
      .rel_drive_diff = 0.002,
      .area = 20.0f,
      .direct_name_match = false,
      .name = "sky130_fd_sc_hd__sdfxtp_1_a",
  };
  EXPECT_TRUE(IsBetterCandidateRank(direct_name, non_direct_name));
  EXPECT_FALSE(IsBetterCandidateRank(non_direct_name, direct_name));

  const CandidateRank lex_first{
      .footprint_match = true,
      .same_library = true,
      .rel_drive_diff = 0.002,
      .area = 20.0f,
      .direct_name_match = false,
      .name = "sky130_fd_sc_hd__sdfxtp_1_a",
  };
  const CandidateRank lex_second{
      .footprint_match = true,
      .same_library = true,
      .rel_drive_diff = 0.002,
      .area = 20.0f,
      .direct_name_match = false,
      .name = "sky130_fd_sc_hd__sdfxtp_1_b",
  };
  EXPECT_TRUE(IsBetterCandidateRank(lex_first, lex_second));
  EXPECT_FALSE(IsBetterCandidateRank(lex_second, lex_first));
}

// Verify that `SelectBestCandidateRankIndex` is invariant to candidate
// iteration order.
TEST(TestScanReplace, SelectBestCandidateRankIndexIsOrderInvariant)
{
  const std::vector<CandidateRank> base_candidates = {
      CandidateRank{
          .footprint_match = true,
          .same_library = true,
          .rel_drive_diff = 0.001,
          .area = 21.25f,
          .direct_name_match = false,
          .name = "sky130_fd_sc_hd__sdfxtp_1_lp",
      },
      // Within 1% of minimum rel_drive_diff (0.008 - 0.001 <= 0.01) and smaller
      // area -> expected winner across all permutations.
      CandidateRank{
          .footprint_match = true,
          .same_library = true,
          .rel_drive_diff = 0.008,
          .area = 20.0f,
          .direct_name_match = true,
          .name = "sky130_fd_sc_hd__sdfxtp_1",
      },
      // Within 1% of sky130_fd_sc_hd__sdfxtp_1, but outside 1% of the minimum
      // rel_drive_diff (0.015 - 0.001 = 0.014 > 0.01).
      CandidateRank{
          .footprint_match = true,
          .same_library = true,
          .rel_drive_diff = 0.015,
          .area = 18.75f,
          .direct_name_match = false,
          .name = "sky130_fd_sc_hd__sdfxtp_2",
      },
      CandidateRank{
          .footprint_match = false,
          .same_library = true,
          .rel_drive_diff = 0.0,
          .area = 15.0f,
          .direct_name_match = false,
          .name = "sky130_fd_sc_hd__sdfbbp_1",
      },
  };

  std::vector<size_t> perm(base_candidates.size());
  std::iota(perm.begin(), perm.end(), 0);
  do {
    std::vector<CandidateRank> permuted;
    permuted.reserve(perm.size());
    for (size_t idx : perm) {
      permuted.push_back(base_candidates[idx]);
    }
    const size_t winner_idx = SelectBestCandidateRankIndex(permuted);
    EXPECT_EQ(permuted[winner_idx].name, "sky130_fd_sc_hd__sdfxtp_1");
  } while (std::next_permutation(perm.begin(), perm.end()));
}

class Sky130ScanReplaceTest : public tst::Fixture
{
 protected:
  void SetUp() override
  {
    library_ = readLiberty(
        "_main/src/dft/test/sky130hd/"
        "sky130_fd_sc_hd__tt_025C_1v80.lib");
    ASSERT_NE(library_, nullptr);
  }

  sta::LibertyLibrary* library_ = nullptr;
};

// Verify `SelectBestScanCell` selects the matching SKY130 scan flip-flop by
// footprint, drive resistance, and area across all candidate permutations.
TEST_F(Sky130ScanReplaceTest, SelectBestScanCellMatchesSky130Cells)
{
  sta::LibertyCell* dfxtp_1
      = library_->findLibertyCell("sky130_fd_sc_hd__dfxtp_1");
  sta::LibertyCell* dfxtp_2
      = library_->findLibertyCell("sky130_fd_sc_hd__dfxtp_2");
  sta::LibertyCell* sdfxtp_1
      = library_->findLibertyCell("sky130_fd_sc_hd__sdfxtp_1");
  sta::LibertyCell* sdfxtp_2
      = library_->findLibertyCell("sky130_fd_sc_hd__sdfxtp_2");
  sta::LibertyCell* sdfxtp_4
      = library_->findLibertyCell("sky130_fd_sc_hd__sdfxtp_4");
  sta::LibertyCell* sdfxbp_1
      = library_->findLibertyCell("sky130_fd_sc_hd__sdfxbp_1");
  ASSERT_NE(dfxtp_1, nullptr);
  ASSERT_NE(dfxtp_2, nullptr);
  ASSERT_NE(sdfxtp_1, nullptr);
  ASSERT_NE(sdfxtp_2, nullptr);
  ASSERT_NE(sdfxtp_4, nullptr);
  ASSERT_NE(sdfxbp_1, nullptr);

  sta::LibertyCell* dfrtp_1
      = library_->findLibertyCell("sky130_fd_sc_hd__dfrtp_1");
  sta::LibertyCell* sdfrtp_1
      = library_->findLibertyCell("sky130_fd_sc_hd__sdfrtp_1");
  sta::LibertyCell* sdfrtp_2
      = library_->findLibertyCell("sky130_fd_sc_hd__sdfrtp_2");
  sta::LibertyCell* sdfrbp_1
      = library_->findLibertyCell("sky130_fd_sc_hd__sdfrbp_1");
  ASSERT_NE(dfrtp_1, nullptr);
  ASSERT_NE(sdfrtp_1, nullptr);
  ASSERT_NE(sdfrtp_2, nullptr);
  ASSERT_NE(sdfrbp_1, nullptr);

  const std::unordered_map<std::string, std::string> empty_port_map;
  const std::vector<sta::LibertyCell*> candidate_cells
      = {sdfxbp_1, sdfxtp_4, sdfxtp_2, sdfxtp_1};
  std::vector<size_t> perm(candidate_cells.size());
  std::iota(perm.begin(), perm.end(), 0);
  do {
    std::vector<std::unique_ptr<ScanCandidate>> candidates_1;
    std::vector<std::unique_ptr<ScanCandidate>> candidates_2;
    for (size_t idx : perm) {
      candidates_1.push_back(std::make_unique<ScanCandidate>(
          candidate_cells[idx], empty_port_map));
      candidates_2.push_back(std::make_unique<ScanCandidate>(
          candidate_cells[idx], empty_port_map));
    }
    std::unique_ptr<ScanCandidate> best_for_1
        = SelectBestScanCell(dfxtp_1, candidates_1);
    ASSERT_NE(best_for_1, nullptr);
    EXPECT_EQ(best_for_1->getScanCell(), sdfxtp_1);

    // Even though `sdfxtp_1` has smaller area than `sdfxtp_2`, `sdfxtp_2`
    // matches `dfxtp_2` in drive resistance and must be selected.
    std::unique_ptr<ScanCandidate> best_for_2
        = SelectBestScanCell(dfxtp_2, candidates_2);
    ASSERT_NE(best_for_2, nullptr);
    EXPECT_EQ(best_for_2->getScanCell(), sdfxtp_2);
  } while (std::next_permutation(perm.begin(), perm.end()));

  // Although `sdfrbp_1` has slightly closer raw |dR_Q| to `dfrtp_1`,
  // `sdfrtp_1` matches the `dfrtp` footprint and saves 10.7% area.
  std::vector<std::unique_ptr<ScanCandidate>> reset_candidates;
  for (sta::LibertyCell* cell : {sdfrbp_1, sdfrtp_2, sdfrtp_1}) {
    reset_candidates.push_back(
        std::make_unique<ScanCandidate>(cell, empty_port_map));
  }
  std::unique_ptr<ScanCandidate> best_reset
      = SelectBestScanCell(dfrtp_1, reset_candidates);
  ASSERT_NE(best_reset, nullptr);
  EXPECT_EQ(best_reset->getScanCell(), sdfrtp_1);

  // Verify null and empty inputs return nullptr gracefully.
  std::vector<std::unique_ptr<ScanCandidate>> empty_candidates;
  EXPECT_EQ(SelectBestScanCell(dfrtp_1, empty_candidates), nullptr);
  std::vector<std::unique_ptr<ScanCandidate>> non_empty_candidates;
  non_empty_candidates.push_back(
      std::make_unique<ScanCandidate>(sdfrtp_1, empty_port_map));
  EXPECT_EQ(SelectBestScanCell(nullptr, non_empty_candidates), nullptr);
}

class Asap7ScanReplaceTest : public tst::Fixture
{
 protected:
  void SetUp() override
  {
    rvt_library_ = readLiberty(
        "_main/src/dft/test/asap7/asap7sc7p5t_SEQ_RVT_FF_nldm_220123.lib");
    lvt_library_ = readLiberty(
        "_main/src/dft/test/asap7/asap7sc7p5t_SEQ_LVT_FF_nldm_220123.lib");
    ASSERT_NE(rvt_library_, nullptr);
    ASSERT_NE(lvt_library_, nullptr);
  }

  sta::LibertyLibrary* rvt_library_ = nullptr;
  sta::LibertyLibrary* lvt_library_ = nullptr;
};

// Verify `SelectBestScanCell` resolves the inverted sequential output `QN` on
// ASAP7 `DFFHQNx*` cells (which have no `Q` port) and selects the drive- and
// VT-matched `SDFHx*` scan cell across all candidate permutations.
TEST_F(Asap7ScanReplaceTest, SelectBestScanCellMatchesAsap7QnOnlyCells)
{
  sta::LibertyCell* dffhqn_x1_rvt
      = rvt_library_->findLibertyCell("DFFHQNx1_ASAP7_75t_R");
  sta::LibertyCell* dffhqn_x2_rvt
      = rvt_library_->findLibertyCell("DFFHQNx2_ASAP7_75t_R");
  sta::LibertyCell* sdfh_x1_rvt
      = rvt_library_->findLibertyCell("SDFHx1_ASAP7_75t_R");
  sta::LibertyCell* sdfh_x2_rvt
      = rvt_library_->findLibertyCell("SDFHx2_ASAP7_75t_R");
  sta::LibertyCell* sdfh_x4_rvt
      = rvt_library_->findLibertyCell("SDFHx4_ASAP7_75t_R");
  sta::LibertyCell* sdfh_x1_lvt
      = lvt_library_->findLibertyCell("SDFHx1_ASAP7_75t_L");
  ASSERT_NE(dffhqn_x1_rvt, nullptr);
  ASSERT_NE(dffhqn_x2_rvt, nullptr);
  ASSERT_NE(sdfh_x1_rvt, nullptr);
  ASSERT_NE(sdfh_x2_rvt, nullptr);
  ASSERT_NE(sdfh_x4_rvt, nullptr);
  ASSERT_NE(sdfh_x1_lvt, nullptr);

  // Confirm that ASAP7 DFFHQNx* and SDFHx* cells only expose QN (no Q port).
  EXPECT_EQ(dffhqn_x1_rvt->findLibertyPort("Q"), nullptr);
  EXPECT_NE(dffhqn_x1_rvt->findLibertyPort("QN"), nullptr);
  EXPECT_EQ(dffhqn_x2_rvt->findLibertyPort("Q"), nullptr);
  EXPECT_NE(dffhqn_x2_rvt->findLibertyPort("QN"), nullptr);

  const std::unordered_map<std::string, std::string> qn_port_map
      = {{"CLK", "CLK"}, {"D", "D"}, {"QN", "QN"}};
  const std::vector<sta::LibertyCell*> candidate_cells
      = {sdfh_x1_lvt, sdfh_x4_rvt, sdfh_x2_rvt, sdfh_x1_rvt};
  std::vector<size_t> perm(candidate_cells.size());
  std::iota(perm.begin(), perm.end(), 0);
  do {
    std::vector<std::unique_ptr<ScanCandidate>> candidates_x1;
    std::vector<std::unique_ptr<ScanCandidate>> candidates_x2;
    for (size_t idx : perm) {
      candidates_x1.push_back(
          std::make_unique<ScanCandidate>(candidate_cells[idx], qn_port_map));
      candidates_x2.push_back(
          std::make_unique<ScanCandidate>(candidate_cells[idx], qn_port_map));
    }

    std::unique_ptr<ScanCandidate> best_for_x1
        = SelectBestScanCell(dffhqn_x1_rvt, candidates_x1);
    ASSERT_NE(best_for_x1, nullptr);
    EXPECT_EQ(best_for_x1->getScanCell(), sdfh_x1_rvt);

    std::unique_ptr<ScanCandidate> best_for_x2
        = SelectBestScanCell(dffhqn_x2_rvt, candidates_x2);
    ASSERT_NE(best_for_x2, nullptr);
    EXPECT_EQ(best_for_x2->getScanCell(), sdfh_x2_rvt);
  } while (std::next_permutation(perm.begin(), perm.end()));
}

}  // namespace
}  // namespace dft
