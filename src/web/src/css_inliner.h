// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#pragma once

#include <cstddef>
#include <string>
#include <string_view>

#include "utl/Logger.h"
#include "web_assets.h"

namespace web {

// Looking assets up through this rather than through a bare Logger is what lets
// the saved report be refused: one that ships with a src="" opens to a blank
// page, and the icons a stylesheet reaches through url() are part of that.
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

// Offset of the next url( token at or after `from`, or npos.  The token is
// case-insensitive, and it has to be a token: matching the tail of an
// identifier -- "myurl(" -- would refuse a report the browser reads fine.
size_t findUrlToken(std::string_view css, size_t from);

// Resolve a reference found inside a stylesheet ("images/x.png",
// "../../img/y.png") against the directory the stylesheet is served from.  A
// reference that starts at the root replaces the directory.  The result is a
// lookup key, not a file name.
std::string resolveAssetPath(std::string_view base_dir,
                             std::string_view reference);

// Rewrite the url() references inside a stylesheet to data: URIs.  Without this
// the icons would resolve against the directory the report was saved in.
std::string inlineStylesheetUrls(std::string_view css,
                                 std::string_view base_dir,
                                 ReportAssets& assets);

}  // namespace web
