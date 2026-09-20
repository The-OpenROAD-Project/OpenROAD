// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// Inflating the embedded assets.  They are stored gzipped (issue #11065 put
// them in the binary in the first place; gzip is what keeps that from costing
// ~3 MB), so everything that needs the text rather than the bytes -- the saved
// report, the tests, a client that does not accept gzip -- comes through here.
//
// This lives outside web_assets.cpp because that file is generated.

#include <cstddef>
#include <stdexcept>
#include <string>

#include "web_assets.h"
#include "zlib.h"

namespace web {

std::string assetText(const EmbeddedAsset& asset)
{
  if (!asset.gzipped) {
    return std::string(asset.content());
  }

  std::string out(asset.original_size, '\0');

  z_stream stream = {};
  // 16 + MAX_WBITS selects the gzip wrapper rather than raw zlib.
  if (inflateInit2(&stream, 16 + MAX_WBITS) != Z_OK) {
    throw std::runtime_error("could not start inflating an embedded asset");
  }
  // zlib's next_in is not const, but inflate() only reads through it.
  stream.next_in
      = const_cast<Bytef*>(reinterpret_cast<const Bytef*>(asset.data));
  stream.avail_in = static_cast<uInt>(asset.size);
  stream.next_out = reinterpret_cast<Bytef*>(out.data());
  stream.avail_out = static_cast<uInt>(out.size());

  const int result = inflate(&stream, Z_FINISH);
  const size_t produced = stream.total_out;
  inflateEnd(&stream);

  // The generator records the size the stream inflates to, so anything other
  // than "finished, and exactly that many bytes" means the table and the blob
  // disagree -- a build problem, not bad input.
  if (result != Z_STREAM_END || produced != asset.original_size) {
    throw std::runtime_error(
        "an embedded asset did not inflate to its recorded size");
  }
  return out;
}

}  // namespace web
