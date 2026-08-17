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

// Iteration over the whole asset table, for the test that holds every asset to
// the rule that none of them loads anything remote.
size_t embeddedAssetCount();
const EmbeddedAssetEntry& embeddedAssetAt(size_t index);

// CSP source expressions for the inline <script> blocks, computed from the
// markup at build time so the policy cannot drift from what it allows.
std::string_view inlineScriptHashes();

// Content-Security-Policy served with every response.  It names no remote
// origin, so a reintroduced CDN reference fails visibly instead of fetching
// (issue #11065); base-uri because the assets load through relative paths.
// Every relaxation below is one the viewer does not work without:
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

// The same policy for the saved report, in a <meta> because a file:// document
// has no headers.  Everything there is inlined, so 'self' means nothing and
// data: has to be a script source; what it still buys is connect-src 'none'.
inline constexpr std::string_view kReportContentSecurityPolicy
    = "default-src 'none'; "
      "script-src 'unsafe-inline' 'unsafe-eval' data:; "
      "style-src 'unsafe-inline' data:; "
      "img-src data: blob:; "
      "connect-src 'none'; "
      "base-uri 'none'; "
      "object-src 'none'";

}  // namespace web
