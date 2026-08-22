// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#pragma once

#include <cstddef>
#include <string>
#include <string_view>

#include "utl/Logger.h"
#include "web_assets.h"

namespace web {

// Remembers a miss, which is what lets the saved report be refused: one that
// ships with a src="" opens to a blank page.
class ReportAssets
{
 public:
  explicit ReportAssets(utl::Logger* logger) : logger_(logger) {}

  const EmbeddedAsset* find(std::string_view path);

  bool missing() const { return missing_; }

 private:
  utl::Logger* logger_;
  bool missing_ = false;
};

// Offset of the next url( token at or after `from`, or npos.  Case-insensitive,
// and it has to be a token: "myurl(" is not one.
size_t findUrlToken(std::string_view css, size_t from);

// Resolve a stylesheet reference ("images/x.png", "../../img/y.png") against
// the directory the stylesheet is served from, into a lookup key.
std::string resolveAssetPath(std::string_view base_dir,
                             std::string_view reference);

// Rewrite the url() references to data: URIs; a relative one would resolve
// against wherever the report was saved.
std::string inlineStylesheetUrls(std::string_view css,
                                 std::string_view base_dir,
                                 ReportAssets& assets);

}  // namespace web
