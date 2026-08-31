// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include <algorithm>
#include <string>
#include <vector>

#include "CtsOptions.h"
#include "TechChar.h"
#include "gtest/gtest.h"
#include "sta/Liberty.hh"
#include "tst/IntegratedFixture.h"

namespace cts {

namespace {

bool contains(const std::vector<std::string>& list, const std::string& name)
{
  return std::ranges::find(list, name) != list.end();
}

}  // namespace

// Exercises TechChar::createDelayBufList(), which builds the list of buffers
// available for latency balancing out of the CTS buffer list plus any
// dedicated delay cells found in the libraries.
class DelayBufListTest : public tst::IntegratedFixture
{
 protected:
  DelayBufListTest()
      : tst::IntegratedFixture(tst::Technology::kNangate45,
                               "_main/src/cts/test/")
  {
  }

  std::vector<std::string> delayBufList(const std::vector<std::string>& buffers,
                                        bool inferred = false,
                                        const char* library = nullptr)
  {
    CtsOptions options(&logger_, &stt_);
    options.setBufferList(buffers);
    options.setBufferListInferred(inferred);
    if (library != nullptr) {
      options.setCtsLibrary(library);
    }
    TechChar tech_char(&options,
                       db_.get(),
                       sta_.get(),
                       &resizer_,
                       &ep_,
                       db_network_,
                       &logger_);
    tech_char.createDelayBufList();
    return options.getDlyBufferList();
  }

  float driveResistance(const std::string& buf)
  {
    sta::LibertyCell* cell = db_network_->findLibertyCell(buf.c_str());
    EXPECT_NE(cell, nullptr) << buf << " has no liberty cell";
    sta::LibertyPort *in, *out;
    cell->bufferPorts(in, out);
    return out->driveResistance();
  }

  float intrinsicDelay(const std::string& buf)
  {
    sta::LibertyCell* cell = db_network_->findLibertyCell(buf.c_str());
    EXPECT_NE(cell, nullptr) << buf << " has no liberty cell";
    sta::LibertyPort *in, *out;
    cell->bufferPorts(in, out);
    return out->intrinsicDelay(sta_.get());
  }
};

TEST_F(DelayBufListTest, EmptyBufferListGivesEmptyDelayList)
{
  EXPECT_TRUE(delayBufList({}).empty());
}

TEST_F(DelayBufListTest, SingleBufferIsAlwaysKept)
{
  EXPECT_EQ(delayBufList({"BUF_X8"}), std::vector<std::string>{"BUF_X8"});
}

TEST_F(DelayBufListTest, ListIsSortedByAscendingDriveResistance)
{
  const std::vector<std::string> result = delayBufList(
      {"BUF_X32", "BUF_X1", "BUF_X8", "BUF_X2", "BUF_X16", "BUF_X4"});

  ASSERT_FALSE(result.empty());
  for (size_t i = 1; i < result.size(); ++i) {
    EXPECT_LT(driveResistance(result[i - 1]), driveResistance(result[i]))
        << result[i - 1] << " should be stronger than " << result[i];
  }
}

// Every kept buffer must be more than 10% weaker than the previous one;
// closer buffers are redundant for delay insertion.
TEST_F(DelayBufListTest, KeptBuffersAreMoreThan10PercentApart)
{
  const std::vector<std::string> all = {"BUF_X1",
                                        "BUF_X2",
                                        "BUF_X4",
                                        "BUF_X8",
                                        "BUF_X16",
                                        "BUF_X32",
                                        "CLKBUF_X1",
                                        "CLKBUF_X2",
                                        "CLKBUF_X3"};
  const std::vector<std::string> result = delayBufList(all);

  ASSERT_FALSE(result.empty());
  EXPECT_LT(result.size(), all.size());
  for (const std::string& buf : result) {
    EXPECT_TRUE(contains(all, buf)) << buf << " is not from the buffer list";
  }
  for (size_t i = 1; i < result.size(); ++i) {
    const float prev = driveResistance(result[i - 1]);
    const float cur = driveResistance(result[i]);
    EXPECT_GT((cur - prev) / cur, 0.1) << result[i - 1] << " vs " << result[i];
  }
}

// Within a group of buffers of similar strength, the one with the largest
// intrinsic delay wins -- it buys the most latency per instance.
TEST_F(DelayBufListTest, KeepsLargerIntrinsicDelayWithinResistanceGroup)
{
  const std::vector<std::string> group = {"BUF_X1", "CLKBUF_X1"};
  // Precondition: the two are within 10% of each other.
  ASSERT_LT(std::abs(driveResistance("BUF_X1") - driveResistance("CLKBUF_X1"))
                / driveResistance("CLKBUF_X1"),
            0.1);

  const std::vector<std::string> result = delayBufList(group);

  ASSERT_EQ(result.size(), 1);
  const std::string& other = result[0] == group[0] ? group[1] : group[0];
  EXPECT_GE(intrinsicDelay(result[0]), intrinsicDelay(other));
}

TEST_F(DelayBufListTest, HasNoDuplicates)
{
  const std::vector<std::string> result
      = delayBufList({"BUF_X1", "BUF_X1", "BUF_X8", "BUF_X8", "BUF_X32"});

  ASSERT_FALSE(result.empty());
  for (size_t i = 0; i < result.size(); ++i) {
    for (size_t j = i + 1; j < result.size(); ++j) {
      EXPECT_NE(result[i], result[j]);
    }
  }
}

// Nangate45 has no dedicated delay cells, so inference adds nothing.
TEST_F(DelayBufListTest, NoDedicatedDelayCellsInNangate45)
{
  EXPECT_EQ(delayBufList({"BUF_X8"}, /*inferred=*/true),
            delayBufList({"BUF_X8"}, /*inferred=*/false));
}

// The ihp-sg13g2 library provides sg13g2_dlygate4sd*_1 delay cells, tagged
// with a DLY* cell_footprint.
class DelayBufListDlyCellTest : public DelayBufListTest
{
 protected:
  DelayBufListDlyCellTest()
  {
    readLiberty(
        "_main/src/cts/test/ihp-sg13g2/"
        "sg13g2_stdcell_typ_1p20V_25C.lib");
  }
};

TEST_F(DelayBufListDlyCellTest, AppendsInferredDelayCells)
{
  const std::vector<std::string> result
      = delayBufList({"BUF_X8"}, /*inferred=*/true);

  EXPECT_EQ(result[0], "BUF_X8");
  EXPECT_TRUE(contains(result, "sg13g2_dlygate4sd1_1"));
  EXPECT_TRUE(contains(result, "sg13g2_dlygate4sd2_1"));
  EXPECT_TRUE(contains(result, "sg13g2_dlygate4sd3_1"));
  // Regular buffers of the same library are not delay cells.
  EXPECT_FALSE(contains(result, "sg13g2_buf_1"));
}

// A user supplied buffer list is taken as is: no delay cells are added.
TEST_F(DelayBufListDlyCellTest, NoDelayCellsWhenBufferListIsUserSupplied)
{
  EXPECT_EQ(delayBufList({"BUF_X8"}, /*inferred=*/false),
            std::vector<std::string>{"BUF_X8"});
}

// -library restricts inference to one library.
TEST_F(DelayBufListDlyCellTest, LibraryOptionRestrictsInference)
{
  EXPECT_EQ(delayBufList({"BUF_X8"},
                         /*inferred=*/true,
                         /*library=*/"NangateOpenCellLibrary"),
            std::vector<std::string>{"BUF_X8"});

  const std::vector<std::string> result
      = delayBufList({"BUF_X8"},
                     /*inferred=*/true,
                     /*library=*/"sg13g2_stdcell_typ_1p20V_25C");
  EXPECT_TRUE(contains(result, "sg13g2_dlygate4sd1_1"));
}

}  // namespace cts
