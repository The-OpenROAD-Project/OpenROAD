// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once
#include <boost/algorithm/string.hpp>

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
}  // namespace odb
