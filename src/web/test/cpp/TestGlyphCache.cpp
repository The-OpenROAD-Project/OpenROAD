// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include <cstddef>
#include <string_view>

#include "glyph_cache.h"
#include "gtest/gtest.h"

namespace web {
namespace {

TEST(GlyphCacheTest, TextWidthPositive)
{
  EXPECT_GT(glyphCache().textWidth("hello", 14), 0);
}

// textWidth() is the sum of the cached advances and the cached kerning pairs,
// so it has to agree with what those two report on their own.
TEST(GlyphCacheTest, TextWidthIsAdvancesPlusKerning)
{
  constexpr std::string_view text = "AVATAR wo.Ty";
  const GlyphCache::FontSize font = glyphCache().getFont(16);

  int expected = 0;
  for (size_t i = 0; i < text.size(); ++i) {
    expected += font.glyph(text[i]).advance;
    if (i + 1 < text.size()) {
      expected += font.kern(text[i], text[i + 1]);
    }
  }
  EXPECT_EQ(font.textWidth(text), expected);
}

// The kerning table is built once per size and read for every pair a label
// measures.  A table that was allocated but never filled would leave every
// pair at zero and silently narrow every label, so check the font really does
// kern something.
TEST(GlyphCacheTest, KerningTableIsPopulated)
{
  const GlyphCache::FontSize font = glyphCache().getFont(48);

  int nonzero_pairs = 0;
  for (char first = '!'; first <= '~'; ++first) {
    for (char second = '!'; second <= '~'; ++second) {
      if (font.kern(first, second) != 0) {
        ++nonzero_pairs;
      }
    }
  }
  EXPECT_GT(nonzero_pairs, 0);
}

// Characters outside the cached range have no table entry; they must read as
// zero rather than indexing past it.
TEST(GlyphCacheTest, KerningOutsideTheCachedRangeIsZero)
{
  const GlyphCache::FontSize font = glyphCache().getFont(14);

  EXPECT_EQ(font.kern('\n', 'A'), 0);
  EXPECT_EQ(font.kern('A', '\x7f'), 0);
  EXPECT_EQ(font.kern('\x80', '\xff'), 0);
}

TEST(GlyphCacheTest, TextWidthEmpty)
{
  EXPECT_EQ(glyphCache().textWidth("", 14), 0);
}

TEST(GlyphCacheTest, TextHeightPositive)
{
  EXPECT_GT(glyphCache().textHeight(14), 0);
}

TEST(GlyphCacheTest, GlyphReturnsAlpha)
{
  // Every printable ASCII char should have a non-null bitmap
  // (except space which has no visible pixels).
  for (char ch = '!'; ch <= '~'; ++ch) {
    auto gi = glyphCache().glyph(14, ch);
    EXPECT_NE(gi.alpha, nullptr) << "char '" << ch << "'";
    EXPECT_GT(gi.bmp_width, 0) << "char '" << ch << "'";
    EXPECT_GT(gi.bmp_height, 0) << "char '" << ch << "'";
    EXPECT_GT(gi.advance, 0) << "char '" << ch << "'";
  }
}

TEST(GlyphCacheTest, SpaceHasNoPixels)
{
  auto gi = glyphCache().glyph(14, ' ');
  EXPECT_EQ(gi.alpha, nullptr);
  EXPECT_GT(gi.advance, 0);  // space still has an advance
}

TEST(GlyphCacheTest, GlyphOutOfRange)
{
  auto gi = glyphCache().glyph(14, '\n');
  EXPECT_EQ(gi.alpha, nullptr);

  auto gi2 = glyphCache().glyph(14, '\x7f');  // DEL
  EXPECT_EQ(gi2.alpha, nullptr);
}

TEST(GlyphCacheTest, AntiAliasedPixels)
{
  // A rendered glyph should contain alpha values between 1 and 254
  // (anti-aliased edges), not just binary 0/255.
  auto gi = glyphCache().glyph(20, 'O');
  ASSERT_NE(gi.alpha, nullptr);
  bool found_intermediate = false;
  for (int i = 0; i < gi.bmp_width * gi.bmp_height; ++i) {
    if (gi.alpha[i] > 0 && gi.alpha[i] < 255) {
      found_intermediate = true;
      break;
    }
  }
  EXPECT_TRUE(found_intermediate) << "No anti-aliased pixels found in 'O'";
}

TEST(GlyphCacheTest, ProportionalWidths)
{
  // DejaVu Sans is proportional — 'W' should be wider than 'i'.
  const int w_width = glyphCache().textWidth("W", 14);
  const int i_width = glyphCache().textWidth("i", 14);
  EXPECT_GT(w_width, i_width);
}

TEST(GlyphCacheTest, DifferentSizes)
{
  EXPECT_LT(glyphCache().cellHeight(10), glyphCache().cellHeight(20));
}

TEST(GlyphCacheTest, CasePreserved)
{
  // 'a' and 'A' should produce different bitmaps.
  auto lower = glyphCache().glyph(14, 'a');
  auto upper = glyphCache().glyph(14, 'A');
  ASSERT_NE(lower.alpha, nullptr);
  ASSERT_NE(upper.alpha, nullptr);
  // At minimum, the bitmap dimensions or content should differ.
  bool differ = (lower.bmp_width != upper.bmp_width)
                || (lower.bmp_height != upper.bmp_height);
  if (!differ) {
    // Same dimensions — compare content.
    for (int i = 0; i < lower.bmp_width * lower.bmp_height; ++i) {
      if (lower.alpha[i] != upper.alpha[i]) {
        differ = true;
        break;
      }
    }
  }
  EXPECT_TRUE(differ) << "'a' and 'A' glyphs should differ";
}

TEST(GlyphCacheTest, TextWidthIncludesAllChars)
{
  // Width of "ab" should be greater than width of "a".
  const int w1 = glyphCache().textWidth("a", 14);
  const int w2 = glyphCache().textWidth("ab", 14);
  EXPECT_GT(w2, w1);
}

}  // namespace
}  // namespace web
