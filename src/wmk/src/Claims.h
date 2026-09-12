// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors
//
// Reader for the plaintext claim files written by the placement and CTS
// embedders.  A claim file is the ownership commitment: it records which
// objects were marked and what value each was driven to, so a verifier can
// check them against a suspect layout.
//
// The format is a header row naming the columns, then one row per claim.
// Columns are looked up by name rather than position so that a schema that
// grows a column does not silently shift the values.
//
// The file is read and validated in full before anything is scored, so a
// hostile file must not be able to exhaust memory first: lines, columns and
// rows are all bounded.

#pragma once

#include <cstddef>
#include <istream>
#include <map>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace wmk {

// Bounds on a claim file.  A real file is a few thousand short rows; these
// are far above that, and far below what would matter to a verifier.
constexpr std::size_t kMaxClaimLineLength = 1 << 16;
constexpr std::size_t kMaxClaimColumns = 64;
constexpr std::size_t kMaxClaimRows = 1 << 20;

// Names must round-trip through the unquoted, whitespace-trimming format.
// Embedders check every eligible name before changing the design.
bool isClaimNameSupported(std::string_view name);

// One claim, as a column-name to value mapping.
using ClaimRow = std::map<std::string, std::string>;

enum class ClaimStage
{
  kPlacement,
  kCts
};

// Read and validate a complete claim file. On failure, rows is empty and error
// identifies the malformed row or schema. Extra named columns are allowed.
bool readClaims(const std::string& path,
                ClaimStage stage,
                std::vector<ClaimRow>& rows,
                std::string& error);

// Stream overload for callers that already have the claim data.
bool readClaims(std::istream& in,
                ClaimStage stage,
                std::vector<ClaimRow>& rows,
                std::string& error);

// Value of a column, or an empty string when absent.
std::string claimField(const ClaimRow& row, const std::string& key);

// A claim is checked unless the embedder recorded a reason for skipping it.
// "already_satisfied" means the object already carried the target value and
// needed no edit, which is still a claim the owner can verify.
bool claimIsCheckable(const ClaimRow& row);

// The two objects a claim names, in the row's own order: A_name and B_name
// for placement, target_lcb and other_lcb for the clock tree.
std::pair<std::string, std::string> claimNames(const ClaimRow& row,
                                               ClaimStage stage);

}  // namespace wmk
