// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors
//
// Thin shim over GlyphCache so tile_generator.cpp callers stay simple.

#pragma once

#include <string_view>

#include "glyph_cache.h"

namespace web {

inline GlyphCache::FontSize fontAtlasGetFont(int height_px)
{
  return glyphCache().getFont(height_px);
}

inline int fontAtlasTextWidth(std::string_view text, int height_px)
{
  return glyphCache().textWidth(text, height_px);
}

inline int fontAtlasTextHeight(int height_px)
{
  return glyphCache().textHeight(height_px);
}

inline int fontAtlasCellHeight(int height_px)
{
  return glyphCache().cellHeight(height_px);
}

inline GlyphCache::GlyphInfo fontAtlasGlyph(int height_px, char ch)
{
  return glyphCache().glyph(height_px, ch);
}

inline int fontAtlasKern(int height_px, char ch1, char ch2)
{
  return glyphCache().kern(height_px, ch1, ch2);
}

}  // namespace web
