// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors
//
// Runtime glyph cache using stb_truetype.  Rasterizes anti-aliased
// glyphs from an embedded TTF font on demand and caches the results
// per pixel height.  Supports both monospaced and proportional fonts
// with kerning.

#pragma once

#include <map>
#include <memory>
#include <mutex>
#include <string_view>
#include <vector>

struct stbtt_fontinfo;

namespace web {

struct Color;

class GlyphCache
{
  // Per-glyph rendering info.
 public:
  struct GlyphInfo
  {
    const unsigned char* alpha;  // bitmap, bmp_width * bmp_height bytes
    int bmp_width;               // bitmap width
    int bmp_height;              // bitmap height
    int x_offset;                // x offset from cursor to bitmap left edge
    int y_offset;                // y offset from line top to bitmap top
    int advance;                 // horizontal advance in pixels
  };

 private:
  struct CachedGlyph
  {
    int alpha_offset = 0;  // byte offset into SizeSlot::alpha
    int bmp_width = 0;     // bitmap width
    int bmp_height = 0;    // bitmap height
    int x_offset = 0;      // from stbtt xoff
    int y_offset = 0;      // from line top (ascent + stbtt yoff)
    int advance = 0;       // horizontal advance
  };

  struct SizeSlot
  {
    int cell_height = 0;
    int ascent = 0;
    float scale = 0;
    CachedGlyph glyphs[95];            // ASCII 32-126
    std::vector<unsigned char> alpha;  // packed glyph bitmaps
  };

 public:
  // Lock-free handle to a resolved font size.  Retrieve via getFont()
  // and use for batch glyph/text operations without per-call locking.
  class FontSize
  {
   public:
    GlyphInfo glyph(char ch) const;
    int kern(char ch1, char ch2) const;
    int textWidth(std::string_view text) const;
    int cellHeight() const;

   private:
    friend class GlyphCache;
    FontSize(const SizeSlot& slot, const stbtt_fontinfo* font_info);
    const SizeSlot& slot_;
    const stbtt_fontinfo* font_info_;
  };

  // Initialize from raw TTF data (must remain valid for the cache lifetime).
  GlyphCache(const unsigned char* ttf_data, unsigned int ttf_size);
  ~GlyphCache();

  // Acquire a FontSize handle (locks once, then all lookups are lock-free).
  FontSize getFont(int font_height) const;

  // Convenience methods — each locks internally per call.
  int textWidth(std::string_view text, int font_height) const;
  int textHeight(int font_height) const;
  int cellHeight(int font_height) const;
  GlyphInfo glyph(int font_height, char ch) const;
  int kern(int font_height, char ch1, char ch2) const;

 private:
  const SizeSlot& getSlot(int font_height) const;

  std::unique_ptr<stbtt_fontinfo> font_info_;
  mutable std::mutex mutex_;
  mutable std::map<int, SizeSlot> cache_;
};

// Singleton accessor — initialised from the embedded font on first call.
const GlyphCache& glyphCache();

}  // namespace web
