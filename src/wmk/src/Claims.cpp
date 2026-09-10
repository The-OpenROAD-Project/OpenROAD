// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include "Claims.h"

#include <cstddef>
#include <fstream>
#include <istream>
#include <set>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "boost/algorithm/string.hpp"

namespace wmk {

namespace {

// Split one line on commas. Embedders reject names the unquoted format
// cannot represent before changing the design. A trailing
// comma yields a final empty field, which is how an empty skipped_reason is
// written.
std::vector<std::string> splitFields(const std::string& line)
{
  std::vector<std::string> out;
  boost::split(out, line, [](char ch) { return ch == ','; });
  return out;
}

std::string trim(const std::string& s)
{
  return boost::trim_copy_if(s, [](char ch) {
    return ch == ' ' || ch == '\t' || ch == '\r' || ch == '\n';
  });
}

bool validateClaimRow(const ClaimRow& row,
                      ClaimStage stage,
                      const std::vector<std::string>& required,
                      std::string& error)
{
  if (stage == ClaimStage::kPlacement && claimField(row, "kind").empty()) {
    error = "empty required field 'kind'";
    return false;
  }
  const bool checkable
      = claimIsCheckable(row)
        && (stage == ClaimStage::kCts || claimField(row, "kind") == "pair");
  if (checkable) {
    for (const std::string& column : required) {
      if (column != "skipped_reason" && claimField(row, column).empty()) {
        error = "empty required field '";
        error += column;
        error += "'";
        return false;
      }
    }
    const std::vector<std::string> names
        = stage == ClaimStage::kPlacement
              ? std::vector<std::string>{"A_name", "B_name"}
              : std::vector<std::string>{"target_lcb"};
    for (const std::string& name : names) {
      if (!isClaimNameSupported(claimField(row, name))) {
        error = "unrepresentable instance name in '" + name + "'";
        return false;
      }
    }
    const std::string bit = claimField(row, "target_bit");
    if (bit != "0" && bit != "1") {
      error = "target_bit must be 0 or 1, got '";
      error += bit;
      error += "'";
      return false;
    }
  }
  return true;
}

}  // namespace

bool isClaimNameSupported(std::string_view name)
{
  return !name.empty() && name.find_first_of(",\r\n") == std::string_view::npos
         && name.find('\0') == std::string_view::npos && name.front() != ' '
         && name.front() != '\t' && name.back() != ' ' && name.back() != '\t';
}

bool readClaims(const std::string& path,
                ClaimStage stage,
                std::vector<ClaimRow>& rows,
                std::string& error)
{
  std::ifstream in(path);
  if (!readClaims(in, stage, rows, error)) {
    error = "'" + path + "': " + error;
    return false;
  }
  return true;
}

bool readClaims(std::istream& in,
                ClaimStage stage,
                std::vector<ClaimRow>& rows,
                std::string& error)
{
  rows.clear();
  error.clear();
  std::string line;
  if (!std::getline(in, line)) {
    error = "cannot read header";
    return false;
  }
  if (line.find('\0') != std::string::npos) {
    error = "line 1: NUL byte in claim header";
    return false;
  }
  std::vector<std::string> header = splitFields(line);
  std::set<std::string> columns;
  for (std::string& column : header) {
    column = trim(column);
    if (column.empty() || !columns.insert(column).second) {
      error = "line 1: empty or duplicate column name '" + column + "'";
      return false;
    }
  }
  const std::vector<std::string> required
      = stage == ClaimStage::kPlacement
            ? std::vector<std::string>{"kind",
                                       "A_name",
                                       "B_name",
                                       "target_bit",
                                       "skipped_reason"}
            : std::vector<std::string>{
                  "target_lcb", "target_bit", "skipped_reason"};
  for (const std::string& column : required) {
    if (!columns.contains(column)) {
      error = "line 1: missing required column '" + column + "'";
      return false;
    }
  }

  std::vector<ClaimRow> parsed;
  size_t line_number = 1;
  while (std::getline(in, line)) {
    ++line_number;
    // OpenDB lookups accept C strings. A NUL must never truncate a claimed
    // name into a different instance, even in an otherwise well-formed row.
    if (line.find('\0') != std::string::npos) {
      error = "line " + std::to_string(line_number) + ": NUL byte in claim row";
      return false;
    }
    if (trim(line).empty()) {
      continue;
    }
    const std::string location = "line " + std::to_string(line_number) + ": ";
    const std::vector<std::string> fields = splitFields(line);
    if (fields.size() != header.size()) {
      error = location + "expected " + std::to_string(header.size())
              + " fields, got " + std::to_string(fields.size());
      return false;
    }
    ClaimRow row;
    for (size_t i = 0; i < header.size(); ++i) {
      row[header[i]] = trim(fields[i]);
    }
    if (!validateClaimRow(row, stage, required, error)) {
      error.insert(0, location);
      return false;
    }
    parsed.push_back(std::move(row));
  }
  if (in.bad() || !in.eof()) {
    error = "error reading claims after line " + std::to_string(line_number);
    return false;
  }
  rows.swap(parsed);
  return true;
}

std::string claimField(const ClaimRow& row, const std::string& key)
{
  const auto it = row.find(key);
  return it == row.end() ? std::string() : it->second;
}

bool claimIsCheckable(const ClaimRow& row)
{
  const std::string reason = claimField(row, "skipped_reason");
  return reason.empty() || reason == "already_satisfied";
}

}  // namespace wmk
