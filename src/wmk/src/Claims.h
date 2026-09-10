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

#pragma once

#include <istream>
#include <map>
#include <string>
#include <string_view>
#include <vector>

namespace wmk {

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

}  // namespace wmk
