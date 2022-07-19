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

#pragma once

#include <list>
#include <regex>
#include <string>

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

  MetricsPolicyType policy_;
  std::string pattern_;
  bool repeating_use_regex_;

 private:
  std::regex pattern_regex_;
  bool matching(std::string key);
};

}  // namespace utl
