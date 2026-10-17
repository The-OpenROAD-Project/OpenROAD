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

constexpr const char* kCtsHeader
    = "target_lcb,other_lcb,target_bit,skipped_reason\n";

TEST(Claims, ValidatesCtsSchemaAndRows)
{
  for (const std::string& text :
       {std::string("other_lcb,target_bit,skipped_reason\nleaf,1,\n"),
        std::string("target_lcb,target_bit,skipped_reason\nleaf,1,\n"),
        std::string(kCtsHeader) + ",peer,1,\n",
        std::string(kCtsHeader) + "leaf,,1,\n",
        std::string(kCtsHeader) + "leaf,peer,2,\n",
        std::string(kCtsHeader) + "leaf,peer,1\n",
        std::string(kCtsHeader) + "leaf,leaf,1,\n"}) {
    std::istringstream in(text);
    std::vector<ClaimRow> rows;
    std::string error;
    EXPECT_FALSE(readClaims(in, ClaimStage::kCts, rows, error)) << text;
    EXPECT_TRUE(rows.empty());
  }
  std::istringstream in(std::string(kCtsHeader) + "leaf,peer,1,\n");
  std::vector<ClaimRow> rows;
  std::string error;
  EXPECT_TRUE(readClaims(in, ClaimStage::kCts, rows, error)) << error;
  ASSERT_EQ(rows.size(), 1);
  EXPECT_EQ(claimNames(rows.front(), ClaimStage::kCts),
            (std::pair<std::string, std::string>{"leaf", "peer"}));
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

TEST(Claims, RejectsNulInEveryRequiredInstanceNameAtomically)
{
  const std::string nul(1, '\0');
  for (const std::string& row :
       {"pair,a" + nul + "_missing,b,0,", "pair,a,b" + nul + "_missing,0,"}) {
    std::istringstream in(std::string(kPlacementHeader) + "pair,a,b,0,\n" + row
                          + "\n");
    std::vector<ClaimRow> rows{{{"previous", "result"}}};
    std::string error;
    EXPECT_FALSE(readClaims(in, ClaimStage::kPlacement, rows, error));
    EXPECT_TRUE(rows.empty());
    EXPECT_EQ(error, "line 3: NUL byte in claim row");
  }
  std::istringstream in(std::string(kCtsHeader) + "leaf,peer,0,\nleaf" + nul
                        + "_missing,peer,0,\n");
  std::vector<ClaimRow> rows{{{"previous", "result"}}};
  std::string error;
  EXPECT_FALSE(readClaims(in, ClaimStage::kCts, rows, error));
  EXPECT_TRUE(rows.empty());
  EXPECT_EQ(error, "line 3: NUL byte in claim row");
}

TEST(Claims, RejectsNulInHeadersAndSkippedRows)
{
  const std::string nul(1, '\0');
  for (const std::string& text :
       {std::string(kPlacementHeader) + "pair,,,,hpwl" + nul + "_reject\n",
        "extra" + nul + "," + kPlacementHeader + "metadata,pair,a,b,0,\n"}) {
    std::istringstream in(text);
    std::vector<ClaimRow> rows;
    std::string error;
    EXPECT_FALSE(readClaims(in, ClaimStage::kPlacement, rows, error));
    EXPECT_TRUE(rows.empty());
    EXPECT_NE(error.find("NUL byte"), std::string::npos);
  }
}

TEST(Claims, RejectsEmbeddedCarriageReturnInInstanceNames)
{
  std::istringstream in(std::string(kPlacementHeader) + "pair,a\rb,c,0,\n");
  std::vector<ClaimRow> rows;
  std::string error;
  EXPECT_FALSE(readClaims(in, ClaimStage::kPlacement, rows, error));
  EXPECT_TRUE(rows.empty());
  EXPECT_EQ(error, "line 2: unrepresentable instance name in 'A_name'");
}

TEST(Claims, RejectsPlacementSelfPairs)
{
  for (const char* bit : {"0", "1"}) {
    std::istringstream in(std::string(kPlacementHeader) + "pair,a,a," + bit
                          + ",\n");
    std::vector<ClaimRow> rows;
    std::string error;
    EXPECT_FALSE(readClaims(in, ClaimStage::kPlacement, rows, error));
    EXPECT_TRUE(rows.empty());
    EXPECT_EQ(error,
              "line 2: placement pair must name two different instances: 'a'");
  }
}

TEST(Claims, RejectsIdenticalConflictingAndReversedPlacementDuplicates)
{
  for (const char* duplicate : {"pair,a,b,0,",
                                "pair,a,b,1,",
                                "pair,b,a,1,",
                                "pair,b,a,0,already_satisfied"}) {
    std::istringstream in(std::string(kPlacementHeader)
                          + "pair,a,b,0,\npair,c,d,1,\n" + duplicate + "\n");
    std::vector<ClaimRow> rows{{{"previous", "result"}}};
    std::string error;
    EXPECT_FALSE(readClaims(in, ClaimStage::kPlacement, rows, error));
    EXPECT_TRUE(rows.empty());
    EXPECT_EQ(error, "line 4: duplicate placement pair 'a' / 'b'");
  }
}

TEST(Claims, RejectsRepeatedCtsPairsRegardlessOfMetadata)
{
  for (const char* duplicate :
       {"a,b,0,new,", "a,b,1,new,", "b,a,1,new,already_satisfied"}) {
    std::istringstream in(
        std::string("target_lcb,other_lcb,target_bit,pair_key,skipped_reason\n")
        + "a,b,0,old,\nb,c,1,other,\n" + duplicate + "\n");
    std::vector<ClaimRow> rows{{{"previous", "result"}}};
    std::string error;
    EXPECT_FALSE(readClaims(in, ClaimStage::kCts, rows, error));
    EXPECT_TRUE(rows.empty());
    EXPECT_EQ(error, "line 4: duplicate CTS pair 'a' / 'b'");
  }
}

TEST(Claims, BoundsLinesColumnsAndRows)
{
  {
    std::istringstream in(std::string(kPlacementHeader) + "pair,"
                          + std::string(kMaxClaimLineLength, 'a') + ",b,0,\n");
    std::vector<ClaimRow> rows;
    std::string error;
    EXPECT_FALSE(readClaims(in, ClaimStage::kPlacement, rows, error));
    EXPECT_EQ(error, "line 2: longer than 65536 bytes");
  }
  {
    std::string header = kPlacementHeader;
    header.pop_back();
    for (size_t i = 0; i < kMaxClaimColumns; ++i) {
      header += ",extra" + std::to_string(i);
    }
    std::istringstream in(header + "\n");
    std::vector<ClaimRow> rows;
    std::string error;
    EXPECT_FALSE(readClaims(in, ClaimStage::kPlacement, rows, error));
    EXPECT_EQ(error, "line 1: more than 64 columns");
  }
  {
    std::string text = kPlacementHeader;
    for (size_t i = 0; i <= kMaxClaimRows; ++i) {
      text += "pair,a" + std::to_string(i) + ",b,0,\n";
    }
    std::istringstream in(text);
    std::vector<ClaimRow> rows;
    std::string error;
    EXPECT_FALSE(readClaims(in, ClaimStage::kPlacement, rows, error));
    EXPECT_EQ(error,
              "line " + std::to_string(kMaxClaimRows + 2) + ": more than "
                  + std::to_string(kMaxClaimRows) + " claims");
  }
  // A final line without a newline is still a claim.
  std::istringstream in(std::string(kPlacementHeader) + "pair,a,b,0,");
  std::vector<ClaimRow> rows;
  std::string error;
  EXPECT_TRUE(readClaims(in, ClaimStage::kPlacement, rows, error)) << error;
  EXPECT_EQ(rows.size(), 1);
}

TEST(Claims, SkippedCandidatesDoNotConsumeScoredCarriers)
{
  std::istringstream in(
      std::string(kPlacementHeader)
      + "pair,a,b,0,hpwl_reject\npair,a,b,0,\nother,a,b,0,\n");
  std::vector<ClaimRow> rows;
  std::string error;
  ASSERT_TRUE(readClaims(in, ClaimStage::kPlacement, rows, error)) << error;
  EXPECT_EQ(rows.size(), 3);
}

}  // namespace
}  // namespace wmk
