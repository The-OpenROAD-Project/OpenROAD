// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once
#include <string>
#include <vector>

#include "boost/algorithm/string.hpp"

namespace odb {

// Utility function to split semicolon-delimited strings and process each rule.
// This template function provides a common pattern for parsing
// semicolon-separated rule strings that are found in LEF file parsing. It
// handles the standard preprocessing steps and then delegates the actual rule
// processing to a user-provided handler function.
template <typename RuleHandler>
void processRules(const std::string& s, RuleHandler handler)
{
  std::vector<std::string> rules;
  boost::split(rules, s, boost::is_any_of(";"));

  for (auto& rule : rules) {
    boost::algorithm::trim(rule);

    // Skip empty rules
    if (rule.empty()) {
      continue;
    }

    rule += " ; ";

    // Delegate rule processing to the provided handler
    handler(rule);
  }
}

template <typename RuleParser>
bool parseAllRules(const std::string& s, RuleParser parser)
{
  const auto last_non_ws = s.find_last_not_of(" \t\n\r");
  if (last_non_ws == std::string::npos || s[last_non_ws] != ';') {
    return false;
  }
  bool all_valid = true;
  bool any_rule = false;
  processRules(s, [&](const std::string& rule) {
    any_rule = true;
    if (!parser(rule)) {
      all_valid = false;
    }
  });
  return any_rule && all_valid;
}
}  // namespace odb
