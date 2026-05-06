// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include "glyph_cache.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <string_view>
#include <vector>

#include "font_data.h"
#include "third-party/stb_truetype/stb_truetype.h"

namespace web {

namespace {
constexpr int kFirstChar = 32;  // space
constexpr int kLastChar = 126;  // ~
constexpr int kNumGlyphs = kLastChar - kFirstChar + 1;
}  // namespace

GlyphCache::GlyphCache(const unsigned char* ttf_data, unsigned int /*ttf_size*/)
    : font_info_(std::make_unique<stbtt_fontinfo>())
{
  if (!stbtt_InitFont(font_info_.get(), ttf_data, 0)) {
    throw std::runtime_error("Failed to initialize font from TTF data");
  }
}

GlyphCache::~GlyphCache() = default;

const GlyphCache::SizeSlot& GlyphCache::getSlot(int font_height) const
{
  std::lock_guard<std::mutex> lock(mutex_);
  auto it = cache_.find(font_height);
  if (it != cache_.end()) {
    return it->second;
  }

  SizeSlot& slot = cache_[font_height];
  slot.scale = stbtt_ScaleForPixelHeight(font_info_.get(), font_height);

  int ascent, descent, line_gap;
  stbtt_GetFontVMetrics(font_info_.get(), &ascent, &descent, &line_gap);
  slot.ascent = static_cast<int>(std::round(ascent * slot.scale));
  const int desc_px = static_cast<int>(std::round(descent * slot.scale));
  slot.cell_height = slot.ascent - desc_px;

  // First pass: rasterize each glyph with stb_truetype.
  struct TempGlyph
  {
    std::vector<unsigned char> bitmap;
    int gw = 0;
    int gh = 0;
    int xoff = 0;  // from stbtt: cursor-relative x offset to bitmap
    int yoff = 0;  // from stbtt: baseline-relative y offset to bitmap
    int advance = 0;
  };
  std::vector<TempGlyph> temps(kNumGlyphs);

  size_t total_bytes = 0;
  for (int i = 0; i < kNumGlyphs; ++i) {
    const int cp = kFirstChar + i;
    auto& t = temps[i];

    int advance_raw, lsb_raw;
    stbtt_GetCodepointHMetrics(font_info_.get(), cp, &advance_raw, &lsb_raw);
    t.advance = static_cast<int>(std::round(advance_raw * slot.scale));

    unsigned char* bmp = stbtt_GetCodepointBitmap(
        font_info_.get(), 0, slot.scale, cp, &t.gw, &t.gh, &t.xoff, &t.yoff);
    if (bmp && t.gw > 0 && t.gh > 0) {
      const size_t n = static_cast<size_t>(t.gw) * t.gh;
      t.bitmap.assign(bmp, bmp + n);
      total_bytes += n;
    }
    if (bmp) {
      stbtt_FreeBitmap(bmp, nullptr);
    }
  }

  // Second pass: pack bitmaps tightly into slot.alpha.
  slot.alpha.resize(total_bytes, 0);
  size_t offset = 0;

  for (int i = 0; i < kNumGlyphs; ++i) {
    const auto& t = temps[i];
    auto& cg = slot.glyphs[i];
    cg.advance = t.advance;
    cg.x_offset = t.xoff;
    cg.y_offset = slot.ascent + t.yoff;  // line-top relative

    if (t.bitmap.empty()) {
      cg.alpha_offset = 0;
      cg.bmp_width = 0;
      cg.bmp_height = 0;
      continue;
    }

    cg.bmp_width = t.gw;
    cg.bmp_height = t.gh;
    cg.alpha_offset = static_cast<int>(offset);

    const size_t n = static_cast<size_t>(t.gw) * t.gh;
    std::copy_n(t.bitmap.data(), n, slot.alpha.data() + offset);
    offset += n;
  }

  return slot;
}

// --- FontSize (lock-free handle) -------------------------------------------

GlyphCache::FontSize::FontSize(const SizeSlot& slot,
                               const stbtt_fontinfo* font_info)
    : slot_(slot), font_info_(font_info)
{
}

GlyphCache::GlyphInfo GlyphCache::FontSize::glyph(char ch) const
{
  const int idx = static_cast<unsigned char>(ch) - kFirstChar;
  if (idx < 0 || idx >= kNumGlyphs) {
    return {.alpha = nullptr,
            .bmp_width = 0,
            .bmp_height = 0,
            .x_offset = 0,
            .y_offset = 0,
            .advance = 0};
  }
  const auto& cg = slot_.glyphs[idx];
  const unsigned char* alpha_ptr
      = cg.bmp_width > 0 ? slot_.alpha.data() + cg.alpha_offset : nullptr;
  return {.alpha = alpha_ptr,
          .bmp_width = cg.bmp_width,
          .bmp_height = cg.bmp_height,
          .x_offset = cg.x_offset,
          .y_offset = cg.y_offset,
          .advance = cg.advance};
}

int GlyphCache::FontSize::kern(char ch1, char ch2) const
{
  const int raw = stbtt_GetCodepointKernAdvance(font_info_, ch1, ch2);
  return static_cast<int>(std::round(raw * slot_.scale));
}

int GlyphCache::FontSize::textWidth(std::string_view text) const
{
  if (text.empty()) {
    return 0;
  }
  int width = 0;
  for (size_t i = 0; i < text.size(); ++i) {
    const int idx = static_cast<unsigned char>(text[i]) - kFirstChar;
    if (idx >= 0 && idx < kNumGlyphs) {
      width += slot_.glyphs[idx].advance;
    }
    if (i + 1 < text.size()) {
      width += kern(text[i], text[i + 1]);
    }
  }
  return width;
}

int GlyphCache::FontSize::cellHeight() const
{
  return slot_.cell_height;
}

// --- GlyphCache public API (convenience, locks per call) -------------------

GlyphCache::FontSize GlyphCache::getFont(int font_height) const
{
  return FontSize(getSlot(font_height), font_info_.get());
}

int GlyphCache::textWidth(std::string_view text, int font_height) const
{
  return getFont(font_height).textWidth(text);
}

int GlyphCache::textHeight(int font_height) const
{
  return getFont(font_height).cellHeight();
}

int GlyphCache::cellHeight(int font_height) const
{
  return getFont(font_height).cellHeight();
}

GlyphCache::GlyphInfo GlyphCache::glyph(int font_height, char ch) const
{
  return getFont(font_height).glyph(ch);
}

int GlyphCache::kern(int font_height, char ch1, char ch2) const
{
  return getFont(font_height).kern(ch1, ch2);
}

const GlyphCache& glyphCache()
{
  static GlyphCache instance(kEmbeddedFontData, kEmbeddedFontSize);
  return instance;
}

}  // namespace web
