// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include "web_gif.h"

// Pre-include, at global scope, every system header gif.h pulls in, so their
// include guards are already set when gif.h is included inside the anonymous
// namespace below.  Otherwise those standard headers would be dragged into the
// anonymous namespace, breaking their declarations.  The <c*> headers include
// their <*.h> counterparts, so the C include guards (_STDINT_H, _STDIO_H, ...)
// gif.h checks are already defined.  (<stdbool.h> is a no-op in C++.)
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

// gif.h defines GifBegin/GifWriteFrame/GifEnd with external linkage and only a
// per-TU include guard.  gui's TU already defines those global symbols and the
// openroad binary links gui + web together, so a plain include here would
// cause a duplicate-symbol link error.  Including it inside an anonymous
// namespace gives these functions internal linkage in this TU, avoiding the
// collision.
namespace {
#include "third-party/gif-h/gif.h"
}  // namespace

namespace web {

struct GifEncoder::Impl
{
  GifWriter writer{};
};

GifEncoder::GifEncoder() : impl_(std::make_unique<Impl>())
{
}

GifEncoder::~GifEncoder()
{
  if (open_) {
    end();
  }
}

bool GifEncoder::begin(const std::string& filename,
                       int width,
                       int height,
                       int delay_centis)
{
  if (open_ || width <= 0 || height <= 0) {
    return false;
  }
  open_ = GifBegin(&impl_->writer,
                   filename.c_str(),
                   static_cast<uint32_t>(width),
                   static_cast<uint32_t>(height),
                   static_cast<uint32_t>(delay_centis));
  return open_;
}

bool GifEncoder::addFrame(const std::vector<unsigned char>& rgba,
                          int width,
                          int height,
                          int delay_centis)
{
  if (!open_ || width <= 0 || height <= 0) {
    return false;
  }
  if (rgba.size() < static_cast<size_t>(width) * height * 4) {
    return false;
  }
  return GifWriteFrame(&impl_->writer,
                       rgba.data(),
                       static_cast<uint32_t>(width),
                       static_cast<uint32_t>(height),
                       static_cast<uint32_t>(delay_centis));
}

bool GifEncoder::end()
{
  if (!open_) {
    return false;
  }
  const bool ok = GifEnd(&impl_->writer);
  open_ = false;
  return ok;
}

}  // namespace web
