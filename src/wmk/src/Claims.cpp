// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include "Claims.h"

#include <cstddef>
#include <fstream>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace wmk {

namespace {

// Split one line on commas.  The claim writers emit instance and net names,
// which do not contain commas, so quoted fields are not supported.
std::vector<std::string> splitFields(const std::string& line)
{
  std::vector<std::string> out;
  std::string field;
  std::istringstream ss(line);
  while (std::getline(ss, field, ',')) {
    out.push_back(field);
  }
  // A trailing comma means a final empty field, which getline does not yield.
  if (!line.empty() && line.back() == ',') {
    out.emplace_back();
  }
  return out;
}

std::string trim(const std::string& s)
{
  const auto begin = s.find_first_not_of(" \t\r\n");
  if (begin == std::string::npos) {
    return "";
  }
  const auto end = s.find_last_not_of(" \t\r\n");
  return s.substr(begin, end - begin + 1);
}

}  // namespace

bool readClaims(const std::string& path,
                std::vector<ClaimRow>& rows,
                std::string& error)
{
  std::ifstream in(path);
  if (!in.is_open()) {
    error = "cannot open '" + path + "'";
    return false;
  }

  std::string line;
  if (!std::getline(in, line)) {
    error = "'" + path + "' is empty";
    return false;
  }
  const std::vector<std::string> header = splitFields(line);
  if (header.empty()) {
    error = "'" + path + "' has no header row";
    return false;
  }

  while (std::getline(in, line)) {
    if (trim(line).empty()) {
      continue;
    }
    const std::vector<std::string> fields = splitFields(line);
    if (fields.size() != header.size()) {
      continue;
    }
    ClaimRow row;
    for (size_t i = 0; i < header.size(); ++i) {
      row[trim(header[i])] = trim(fields[i]);
    }
    rows.push_back(std::move(row));
  }
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
