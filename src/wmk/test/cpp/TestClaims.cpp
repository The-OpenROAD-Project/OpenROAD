// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors
#include <ios>
#include <sstream>
#include <string>
#include <vector>

#include "Claims.h"
#include "gtest/gtest.h"

namespace wmk {
namespace {

constexpr const char* kPlacementHeader
    = "kind,A_name,B_name,target_bit,skipped_reason\n";

TEST(Claims, KeepsEveryWellFormedClaim)
{
  std::istringstream in(std::string(kPlacementHeader)
                        + "pair,a,b,0,\npair,c,d,1,already_satisfied\n");
  std::vector<ClaimRow> rows;
  std::string error;
  ASSERT_TRUE(readClaims(in, ClaimStage::kPlacement, rows, error)) << error;
  ASSERT_EQ(rows.size(), 2);
  EXPECT_TRUE(claimIsCheckable(rows[0]));
  EXPECT_TRUE(claimIsCheckable(rows[1]));
}

TEST(Claims, MalformedRowCannotReduceTheDenominator)
{
  for (const std::string damaged : {"pair,c,d,1", "pair,c,d,1,,"}) {
    std::istringstream in(std::string(kPlacementHeader) + "pair,a,b,0,\n"
                          + damaged + "\n");
    std::vector<ClaimRow> rows{{{"previous", "result"}}};
    std::string error;
    EXPECT_FALSE(readClaims(in, ClaimStage::kPlacement, rows, error));
    EXPECT_TRUE(rows.empty());
    EXPECT_NE(error.find("line 3:"), std::string::npos);
  }
}

TEST(Claims, RejectsMissingRequiredColumns)
{
  for (const std::string column :
       {"kind", "A_name", "B_name", "target_bit", "skipped_reason"}) {
    std::string header = kPlacementHeader;
    header.replace(header.find(column), column.size(), "unexpected");
    std::istringstream in(header);
    std::vector<ClaimRow> rows;
    std::string error;
    EXPECT_FALSE(readClaims(in, ClaimStage::kPlacement, rows, error));
    EXPECT_NE(error.find("missing required column '" + column + "'"),
              std::string::npos);
  }
}

TEST(Claims, RejectsEmptyAndDuplicateHeaderNames)
{
  for (const std::string header : {"\n", "kind,,A_name\n", "kind, kind\n"}) {
    std::istringstream in(header);
    std::vector<ClaimRow> rows;
    std::string error;
    EXPECT_FALSE(readClaims(in, ClaimStage::kPlacement, rows, error));
    EXPECT_NE(error.find("empty or duplicate"), std::string::npos);
  }
}

TEST(Claims, RejectsEmptyNamesAndInvalidBits)
{
  for (const std::string row : {",a,b,0,",
                                "pair,,b,0,",
                                "pair,a,,0,",
                                "pair,a,b,,",
                                "pair,a,b,2,",
                                "pair,a,b,0junk,"}) {
    std::istringstream in(std::string(kPlacementHeader) + row + "\n");
    std::vector<ClaimRow> rows;
    std::string error;
    EXPECT_FALSE(readClaims(in, ClaimStage::kPlacement, rows, error));
    EXPECT_TRUE(rows.empty());
    EXPECT_NE(error.find("line 2:"), std::string::npos);
  }
}

TEST(Claims, AcceptsReorderedExtendedCrLfSchema)
{
  std::istringstream in(
      "extra, skipped_reason, B_name, target_bit, A_name, kind\r\n"
      "metadata,,b,1,a,pair\r\n\r\n");
  std::vector<ClaimRow> rows;
  std::string error = "previous error";
  ASSERT_TRUE(readClaims(in, ClaimStage::kPlacement, rows, error)) << error;
  EXPECT_TRUE(error.empty());
  ASSERT_EQ(rows.size(), 1);
  EXPECT_EQ(claimField(rows[0], "A_name"), "a");
  EXPECT_EQ(claimField(rows[0], "target_bit"), "1");
}

TEST(Claims, PreservesDocumentedSkippedRecords)
{
  std::istringstream in(std::string(kPlacementHeader)
                        + "pair,,,,hpwl_reject\nother,,,,\n");
  std::vector<ClaimRow> rows;
  std::string error;
  ASSERT_TRUE(readClaims(in, ClaimStage::kPlacement, rows, error)) << error;
  ASSERT_EQ(rows.size(), 2);
  EXPECT_FALSE(claimIsCheckable(rows[0]));
}

TEST(Claims, ValidatesCtsSchemaAndRows)
{
  for (const std::string text :
       {"target_bit,skipped_reason\n1,\n",
        "target_lcb,target_bit,skipped_reason\n,1,\n",
        "target_lcb,target_bit,skipped_reason\nleaf,2,\n",
        "target_lcb,target_bit,skipped_reason\nleaf,1\n"}) {
    std::istringstream in(text);
    std::vector<ClaimRow> rows;
    std::string error;
    EXPECT_FALSE(readClaims(in, ClaimStage::kCts, rows, error));
    EXPECT_TRUE(rows.empty());
  }
  std::istringstream in("target_lcb,target_bit,skipped_reason\nleaf,1,\n");
  std::vector<ClaimRow> rows;
  std::string error;
  EXPECT_TRUE(readClaims(in, ClaimStage::kCts, rows, error)) << error;
  ASSERT_EQ(rows.size(), 1);
}

TEST(Claims, RejectsUnreadableStream)
{
  std::istringstream in;
  in.setstate(std::ios::badbit);
  std::vector<ClaimRow> rows{{{"previous", "result"}}};
  std::string error;
  EXPECT_FALSE(readClaims(in, ClaimStage::kCts, rows, error));
  EXPECT_TRUE(rows.empty());
  EXPECT_FALSE(error.empty());
}

TEST(Claims, SupportedNamesRoundTripWithoutLoss)
{
  for (const std::string name :
       {"top/bank/ff[3]", "a+b", "a b", "a\tb", "a\"b"}) {
    EXPECT_TRUE(isClaimNameSupported(name));
    std::istringstream in(std::string(kPlacementHeader) + "pair," + name
                          + ",peer,1,\n");
    std::vector<ClaimRow> rows;
    std::string error;
    ASSERT_TRUE(readClaims(in, ClaimStage::kPlacement, rows, error)) << error;
    ASSERT_EQ(rows.size(), 1);
    EXPECT_EQ(claimField(rows.front(), "A_name"), name);
  }
}

TEST(Claims, RejectsNamesThatCannotRoundTrip)
{
  for (const std::string name :
       {"", "a,b", "a\nb", "a\rb", " a", "a ", "\ta", "a\t"}) {
    EXPECT_FALSE(isClaimNameSupported(name));
  }
  EXPECT_FALSE(isClaimNameSupported(std::string("a\0b", 3)));
}

}  // namespace
}  // namespace wmk
