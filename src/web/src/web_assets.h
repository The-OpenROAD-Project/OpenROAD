// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#pragma once

#include <cstddef>
#include <string_view>

namespace web {

struct EmbeddedAsset
{
  const char* data;
  size_t size;
  const char* content_type;

  std::string_view content() const { return {data, size}; }
};

struct EmbeddedAssetEntry
{
  std::string_view path;
  EmbeddedAsset asset;
};

// Returns the embedded asset for the given URL path (e.g. "/index.html"),
// or nullptr if not found.
const EmbeddedAsset* findEmbeddedAsset(std::string_view path);

// Iteration over the whole asset table, for the test that holds every asset to
// the rule that none of them loads anything remote.
size_t embeddedAssetCount();
const EmbeddedAssetEntry& embeddedAssetAt(size_t index);

}  // namespace web
