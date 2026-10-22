// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors
//
// The assets embedded in the binary, as the server hands them out.

#include <stdexcept>
#include <string>

#include "gtest/gtest.h"
#include "web_assets.h"

namespace web {
namespace {

TEST(WebAssets, StoresThePageGzipped)
{
  const EmbeddedAsset* asset = findEmbeddedAsset("/index.html");
  ASSERT_NE(asset, nullptr);
  EXPECT_TRUE(asset->gzipped);
  // The gzip magic number: what the server hands to a browser that asked for
  // gzip has to actually be a gzip stream.
  ASSERT_GE(asset->size, 2u);
  EXPECT_EQ(static_cast<unsigned char>(asset->data[0]), 0x1f);
  EXPECT_EQ(static_cast<unsigned char>(asset->data[1]), 0x8b);
  EXPECT_LT(asset->size, asset->original_size);
}

TEST(WebAssets, InflatesToTheRecordedSize)
{
  const EmbeddedAsset* asset = findEmbeddedAsset("/index.html");
  ASSERT_NE(asset, nullptr);
  const std::string html = assetText(*asset);
  EXPECT_EQ(html.size(), asset->original_size);
  EXPECT_NE(html.find("<html"), std::string::npos);
}

// The generator keeps a file as it is when gzip would not make it smaller.
TEST(WebAssets, KeepsAnUncompressedAssetAsItIs)
{
  const EmbeddedAsset stored{"abc", 3, "text/plain", false, 3};
  EXPECT_EQ(assetText(stored), "abc");
}

// A table entry whose size disagrees with its blob is a build bug, and must
// not come back as truncated or padded text.
TEST(WebAssets, RefusesABlobThatDoesNotMatchItsSize)
{
  const EmbeddedAsset* asset = findEmbeddedAsset("/index.html");
  ASSERT_NE(asset, nullptr);
  EmbeddedAsset wrong = *asset;
  wrong.original_size += 1;
  EXPECT_THROW(assetText(wrong), std::runtime_error);
}

}  // namespace
}  // namespace web
