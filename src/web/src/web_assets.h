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

// Iteration over the whole asset table, so a test can hold every asset to the
// rule that none of them loads anything remote.
size_t embeddedAssetCount();
const EmbeddedAssetEntry& embeddedAssetAt(size_t index);

// CSP source expressions for the inline <script> blocks in the embedded HTML,
// computed at build time by embed_web_assets.py -- today just the import map
// in index.html.  Generated rather than written down so the policy cannot
// drift from the markup it allows.
std::string_view inlineScriptHashes();

// Content-Security-Policy served with every response.  Every asset comes from
// this process, so the policy names no remote origin at all: a reintroduced CDN
// reference then fails loudly in the browser instead of quietly fetching remote
// code (issue #11065).  base-uri matters for the same reason -- the assets are
// loaded through relative paths, which an injected <base href> would redirect.
//
// Each relaxation is one the viewer is measurably broken without, and none of
// them lets a remote origin back in.  Nothing is listed for a use the viewer
// does not have yet: default-src 'none' then denies it, and a denial names the
// missing directive in the console.
//   script-src <hashes>       the import map in index.html
//   script-src 'unsafe-eval'  netlistsvg validates its input with ajv, which
//                             compiles JSON schemas through new Function()
//   style-src 'unsafe-inline' the widgets set element.style and the netlistsvg
//                             skin injects a <style> block
//   img-src data: blob:       tiles arrive as data: URIs from the cache and as
//                             blobs from the socket
//   connect-src 'self'        the WebSocket
inline const std::string& contentSecurityPolicy()
{
  static const std::string policy
      = std::string("default-src 'none'; script-src 'self' 'unsafe-eval' ")
        + std::string(inlineScriptHashes())
        + "; "
          "style-src 'self' 'unsafe-inline'; "
          "img-src 'self' data: blob:; "
          "connect-src 'self'; "
          "base-uri 'none'; "
          "object-src 'none'";
  return policy;
}

// The same policy for the saved report, which carries it in a <meta> element
// because a file:// document has no headers.  Everything there is inlined into
// the one file, so 'self' means nothing and data: has to be a script source;
// what the policy still buys is that the report cannot reach the network.
inline constexpr std::string_view kReportContentSecurityPolicy
    = "default-src 'none'; "
      "script-src 'unsafe-inline' 'unsafe-eval' data:; "
      "style-src 'unsafe-inline' data:; "
      "img-src data: blob:; "
      "connect-src 'none'; "
      "base-uri 'none'; "
      "object-src 'none'";

}  // namespace web
