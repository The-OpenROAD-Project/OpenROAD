// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#pragma once

#include <cstddef>
#include <string>
#include <string_view>

namespace web {

struct EmbeddedAsset
{
  const char* data;
  size_t size;
  const char* content_type;
  // The assets are stored gzipped: the server hands the bytes straight to a
  // browser that accepts gzip, and only the report path pays to inflate them.
  bool gzipped;
  // Bytes after inflating; equal to size when the asset is not compressed.
  size_t original_size;

  // The stored bytes, compressed if gzipped is set.
  std::string_view content() const { return {data, size}; }
};

// Returns the embedded asset for the given URL path (e.g. "/index.html"),
// or nullptr if not found.
const EmbeddedAsset* findEmbeddedAsset(std::string_view path);

// The saved report's script (report_assets.cpp).  Not in the served table:
// saveReport() inflates it into the file it writes.
extern const EmbeddedAsset kReportJS;

// The asset's bytes, inflating it first if it is stored gzipped.  For a client
// that does not accept gzip, and for the tests; the server hands content() over
// as it is otherwise.
std::string assetText(const EmbeddedAsset& asset);

}  // namespace web
