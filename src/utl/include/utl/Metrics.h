// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022-2025, The OpenROAD Authors

#pragma once

#include <list>
#include <regex>
#include <string>
#include <vector>

namespace utl {

struct MetricsEntry
{
  std::string key;
  std::string value;

  static std::string assembleJSON(const std::list<MetricsEntry>& entries);
};

enum class MetricsPolicyType
{
  KeepFirst,
  KeepLast,
  Remove
};

class MetricsPolicy
{
 public:
  MetricsPolicy(const std::string& key_pattern,
                MetricsPolicyType policy,
                bool repeating_use_exact_match);

  void applyPolicy(std::list<MetricsEntry>& entries);

  static std::vector<MetricsPolicy> makeDefaultPolicies();

 private:
  MetricsPolicyType policy_;
  std::string pattern_;
  bool repeating_use_regex_;
  std::regex pattern_regex_;
  bool matching(const std::string& key);
};

}  // namespace utl
