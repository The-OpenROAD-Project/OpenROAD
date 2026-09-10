// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#pragma once

#include <memory>
#include <string>
#include <vector>

namespace web {

// RAII wrapper around the vendored third-party/gif-h encoder for the web
// module's animated-GIF export (mirrors gui's save_animated_gif).
//
// gif.h defines GifBegin/GifWriteFrame/GifEnd with *external* linkage and only
// a per-TU include guard.  gui's translation unit already defines those global
// symbols, and the openroad binary links gui + web together, so including
// gif.h directly here would cause a "multiple definition" link error.  The
// implementation (web_gif.cpp) therefore includes gif.h inside an anonymous
// namespace (internal linkage) and hides the GifWriter type behind a PIMPL so
// this header stays free of gif.h.
class GifEncoder
{
 public:
  GifEncoder();
  ~GifEncoder();

  GifEncoder(const GifEncoder&) = delete;
  GifEncoder& operator=(const GifEncoder&) = delete;

  // Open `filename` for writing at width x height.  delay is in hundredths of
  // a second.  Returns false on failure.
  bool begin(const std::string& filename,
             int width,
             int height,
             int delay_centis);

  // Append one RGBA8 frame (width*height*4 bytes, top-down).  delay in
  // hundredths of a second.  Returns false on failure.
  bool addFrame(const std::vector<unsigned char>& rgba,
                int width,
                int height,
                int delay_centis);

  // Finalize and close the file.  Returns false on failure.  Safe to call
  // once; the destructor calls it if the caller did not.
  bool end();

  bool isOpen() const { return open_; }

 private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
  bool open_ = false;
};

}  // namespace web
