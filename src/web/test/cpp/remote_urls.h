// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// Scans for what would make the viewer fetch from the network, shared by the
// tests of the served assets and of the saved report.

#pragma once

#include <array>
#include <cctype>
#include <cstddef>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "gtest/gtest.h"

namespace web::test {

// Absolute URLs the bundles quote as names rather than fetch: XML namespaces,
// JSON Schema dialects, licence and homepage links in the kept comments.
inline constexpr auto kIdentifierUrls = std::to_array<std::string_view>({
    "http://www.w3.org/",
    "http://www.eclipse.org/",
    "http:///org/eclipse/",
    "http://json-schema.org/",
    "https://raw.githubusercontent.com/epoberezkin/ajv/",
    "http://github.com/garycourt/uri-js",
    "http://mths.be/",
    "http://underscorejs.org/",
    "https://lodash.com/",
    "https://js.foundation/",
    "https://feross.org",
    "https://npms.io/",
    "https://leafletjs.com",
    "https://discourse.threejs.org/",
    // The namespace netlistsvg stamps into the SVG it produces.
    "https://github.com/nturley/netlistsvg",
});

// An allowed prefix only matches up to a host or path boundary, so
// "https://feross.org.example.com/x.js" is not "https://feross.org".
inline bool isIdentifierUrl(const std::string_view url)
{
  for (const std::string_view allowed : kIdentifierUrls) {
    if (!url.starts_with(allowed)) {
      continue;
    }
    if (allowed.ends_with('/') || url.size() == allowed.size()) {
      return true;
    }
    const char next = url[allowed.size()];
    if (next == '/' || next == '?' || next == '#') {
      return true;
    }
  }
  return false;
}

// The URL starting at `begin`, up to the first character that would end it.
inline std::string urlAt(const std::string_view text, const size_t begin)
{
  const size_t end = text.find_first_of("\"'`) >,;\n", begin);
  return std::string(
      text.substr(begin, end == std::string_view::npos ? 60 : end - begin));
}

// Every absolute http(s) URL in `text` that is not an identifier.  Schemes and
// hosts are case-insensitive, so the scan is too.
inline std::vector<std::string> externalUrls(const std::string_view text)
{
  std::string lower(text);
  for (char& c : lower) {
    c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
  }
  const std::string_view view = lower;
  std::vector<std::string> found;
  for (size_t pos = view.find("http"); pos != std::string_view::npos;
       pos = view.find("http", pos + 4)) {
    if (view.compare(pos, 7, "http://") != 0
        && view.compare(pos, 8, "https://") != 0) {
      continue;
    }
    std::string url = urlAt(view, pos);
    if (!isIdentifierUrl(url)) {
      found.push_back(std::move(url));
    }
  }
  return found;
}

// Scheme-relative references ("//host/x.js") where a browser fetches them:
// HTML attributes, CSS url() and @import, and JS fetch(), import() and from.
inline std::vector<std::string> schemeRelativeUrls(const std::string_view text)
{
  static constexpr auto kOpeners = std::to_array<std::string_view>({
      "src=\"//",    "src='//",    "href=\"//",   "href='//",   "url(//",
      "url(\"//",    "url('//",    "@import\"//", "@import'//", "@import \"//",
      "@import '//", "fetch(\"//", "fetch('//",   "fetch(`//",  "import(\"//",
      "import('//",  "import(`//", "from\"//",    "from'//",    "from \"//",
      "from '//",
  });
  std::vector<std::string> found;
  for (const std::string_view opener : kOpeners) {
    for (size_t pos = text.find(opener); pos != std::string_view::npos;
         pos = text.find(opener, pos + 1)) {
      found.push_back(urlAt(text, pos + opener.size() - 2));
    }
  }
  return found;
}

// For EXPECT_TRUE: whether `text` would fetch anything from elsewhere.
inline ::testing::AssertionResult fetchesNothingRemote(
    const std::string_view text)
{
  std::vector<std::string> urls = externalUrls(text);
  for (std::string& url : schemeRelativeUrls(text)) {
    urls.push_back(std::move(url));
  }
  if (urls.empty()) {
    return ::testing::AssertionSuccess();
  }
  return ::testing::AssertionFailure() << "reaches out to " << urls.front()
                                       << " (" << urls.size() << " in total)";
}

}  // namespace web::test
