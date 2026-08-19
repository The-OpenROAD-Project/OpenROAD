// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#pragma once

#include <cstddef>
#include <string>
#include <string_view>

#include "utl/Logger.h"
#include "web_assets.h"

namespace web {

// Remembers anything that would make the report wrong, which is what lets it be
// refused: one that ships with a src="" opens to a blank page, and one that
// ships an unresolved url() reaches for a file next to wherever it was saved.
class ReportAssets
{
 public:
  explicit ReportAssets(utl::Logger* logger) : logger_(logger) {}

  const EmbeddedAsset* find(std::string_view path);

  // Record a failure that is not a missing asset.
  void fail(std::string_view reason);

  bool failed() const { return failed_; }

 private:
  utl::Logger* logger_;
  bool failed_ = false;
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
