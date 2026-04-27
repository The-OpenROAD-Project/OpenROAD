// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors
//
// Runtime glyph cache using stb_truetype.  Rasterizes anti-aliased
// glyphs from an embedded TTF font on demand and caches the results
// per pixel height.  Supports both monospaced and proportional fonts
// with kerning.

#pragma once

#include <map>
#include <mutex>
#include <string_view>
#include <vector>

struct stbtt_fontinfo;

namespace web {

struct Color;

class GlyphCache
{
 public:
  // Initialize from raw TTF data (must remain valid for the cache lifetime).
  GlyphCache(const unsigned char* ttf_data, unsigned int ttf_size);
  ~GlyphCache();

  // Total pixel width of a rendered string (includes kerning).
  int textWidth(std::string_view text, int font_height) const;
  // Line height in pixels.
  int textHeight(int font_height) const;

  // Per-glyph rendering info.
  struct GlyphInfo
  {
    const unsigned char* alpha;  // bitmap, bmp_width * bmp_height bytes
    int bmp_width;               // bitmap width
    int bmp_height;              // bitmap height
    int x_offset;                // x offset from cursor to bitmap left edge
    int y_offset;                // y offset from line top to bitmap top
    int advance;                 // horizontal advance in pixels
  };

  // Cell height (line height, same for all glyphs at a given size).
  int cellHeight(int font_height) const;

  // Look up a glyph.  Returns {nullptr,...} for space / unknown.
  GlyphInfo glyph(int font_height, char ch) const;

  // Kerning adjustment between two characters (in pixels).
  int kern(int font_height, char ch1, char ch2) const;

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

  const SizeSlot& getSlot(int font_height) const;

  stbtt_fontinfo* font_info_;
  mutable std::mutex mutex_;
  mutable std::map<int, SizeSlot> cache_;
};

// Singleton accessor — initialised from the embedded font on first call.
const GlyphCache& glyphCache();

}  // namespace web
