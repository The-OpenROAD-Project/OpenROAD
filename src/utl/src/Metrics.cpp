// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022-2025, The OpenROAD Authors

#include "utl/Metrics.h"

#include <list>
#include <regex>
#include <string>
#include <vector>

#include "spdlog/fmt/fmt.h"
#include "spdlog/fmt/ostr.h"
#include "spdlog/spdlog.h"

namespace utl {

std::string MetricsEntry::assembleJSON(const std::list<MetricsEntry>& entries)
{
  std::string json = "{";
  std::string separator;
  for (MetricsEntry entry : entries) {
    json += fmt::format("{}\n\t\"{}\": {}", separator, entry.key, entry.value);
    separator = ",";
  }

  return json + "\n}";
}

MetricsPolicy::MetricsPolicy(const std::string& key_pattern,
                             MetricsPolicyType policy,
                             bool repeating_use_regex)
    : policy_(policy),
      pattern_(key_pattern),
      repeating_use_regex_(repeating_use_regex)
{
  pattern_regex_ = std::regex(pattern_);
}

bool MetricsPolicy::matching(const std::string& key)
{
  if (repeating_use_regex_) {
    return std::regex_match(key, pattern_regex_);
  }

  return pattern_ == key;
}

void MetricsPolicy::applyPolicy(std::list<MetricsEntry>& entries)
{
  switch (policy_) {
    case MetricsPolicyType::KeepFirst: {
      bool matched = false;
      auto iter = entries.begin();
      while (iter != entries.end()) {
        auto copy_iter = iter;
        iter++;
        if (matching(copy_iter->key)) {
          if (matched) {
            entries.erase(copy_iter);
          }
          matched = true;
        }
      }
      break;
    }

    case MetricsPolicyType::KeepLast: {
      bool matched = false;
      std::list<MetricsEntry>::iterator last;
      for (auto iter = entries.begin(); iter != entries.end(); iter++) {
        if (matching(iter->key)) {
          if (matched) {
            entries.erase(last);
          }
          last = iter;
          matched = true;
        }
      }
      break;
    }

    case MetricsPolicyType::Remove: {
      auto iter = entries.begin();
      while (iter != entries.end()) {
        auto copy_iter = iter;
        iter++;
        if (matching(copy_iter->key)) {
          entries.erase(copy_iter);
        }
      }
      break;
    }
  }
}

std::vector<MetricsPolicy> MetricsPolicy::makeDefaultPolicies()
{
  return {
      // Examples of Metrics policy to appy before writing JSON file
      //
      // MetricsPolicy(".*::.*", MetricsPolicyType::Remove, true),
      // MetricsPolicy("placeopt_pre__.*", MetricsPolicyType::Remove, true),
      // MetricsPolicy("detailedroute__.*", MetricsPolicyType::KeepLast, true)
  };
}

}  // namespace utl
