// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include "report_html.h"

#include <cstddef>
#include <string>
#include <string_view>

#include "boost/algorithm/string/replace.hpp"

namespace web {

std::string reportHead(const std::string_view index_html)
{
  constexpr std::string_view kBegin = "<!-- saved-report:begin -->";
  constexpr std::string_view kEnd = "<!-- saved-report:end -->";
  const size_t begin = index_html.find(kBegin);
  if (begin == std::string_view::npos) {
    return {};
  }
  const size_t start = begin + kBegin.size();
  const size_t end = index_html.find(kEnd, start);
  if (end == std::string_view::npos) {
    return {};
  }
  return std::string(index_html.substr(start, end - start));
}

std::string scriptSafeJson(const std::string_view json)
{
  return boost::algorithm::replace_all_copy(std::string(json), "<", "\\u003c");
}

std::string htmlCommentSafe(std::string text)
{
  boost::algorithm::replace_all(text, "<!--", "<! --");
  boost::algorithm::replace_all(text, "--!>", "--! >");
  boost::algorithm::replace_all(text, "-->", "-- >");
  return text;
}

}  // namespace web
