/////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2022, The Regents of the University of California
// All rights reserved.
//
// BSD 3-Clause License
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// * Redistributions of source code must retain the above copyright notice, this
//   list of conditions and the following disclaimer.
//
// * Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
//
// * Neither the name of the copyright holder nor the names of its
//   contributors may be used to endorse or promote products derived from
//   this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//
///////////////////////////////////////////////////////////////////////////////

#include "utl/Metrics.h"

#include "spdlog/fmt/fmt.h"
#include "spdlog/fmt/ostr.h"
#include "spdlog/spdlog.h"

namespace utl {

std::string MetricsEntry::assembleJSON(const std::list<MetricsEntry>& entries)
{
  std::string json = "{";
  std::string separator = "";
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

bool MetricsPolicy::matching(std::string key)
{
  if (repeating_use_regex_)
    return std::regex_match(key, pattern_regex_);
  else
    return pattern_.compare(key) == 0;
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
          if (matched)
            entries.erase(copy_iter);
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
          if (matched)
            entries.erase(last);
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
