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

#include <map>
#include <string>
#include <vector>

namespace wmk {

// One claim, as a column-name to value mapping.
using ClaimRow = std::map<std::string, std::string>;

// Parse a claim file.  Returns false if the file cannot be opened or has no
// header.  Rows with a different field count than the header are skipped.
bool readClaims(const std::string& path,
                std::vector<ClaimRow>& rows,
                std::string& error);

// Value of a column, or an empty string when absent.
std::string claimField(const ClaimRow& row, const std::string& key);

// A claim is checked unless the embedder recorded a reason for skipping it.
// "already_satisfied" means the object already carried the target value and
// needed no edit, which is still a claim the owner can verify.
bool claimIsCheckable(const ClaimRow& row);

}  // namespace wmk
