// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include "syn/lir/ConstView.h"

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <string>

#include "absl/strings/str_cat.h"
#include "syn/lir/Checksum.h"

namespace syn::lir {

char logicValueChar(LogicValue value)
{
  switch (value) {
    case LogicValue::kZero:
      return '0';
    case LogicValue::kOne:
      return '1';
    case LogicValue::kX:
      return 'x';
  }
  return '?';
}

ConstView ConstView::fromInline(uint32_t width, uint32_t bits, uint32_t x_mask)
{
  assert(width <= kMaxInlineConstWidth);
  ConstView view;
  view.width_ = width;
  const uint64_t mask = width == 0 ? 0 : constLastWordMask(width);
  const uint64_t x = x_mask & mask;
  view.small_[0] = (bits & mask) & ~x;
  view.small_[1] = x;
  return view;
}

LogicValue ConstView::bit(uint32_t index) const
{
  assert(index < width_);
  const uint64_t* w = words();
  const uint32_t word = index / 64;
  const uint32_t shift = index % 64;
  const uint64_t v = (w[word] >> shift) & 1u;
  const uint64_t x = (w[numWords() + word] >> shift) & 1u;
  if (x != 0) {
    return LogicValue::kX;
  }
  return v != 0 ? LogicValue::kOne : LogicValue::kZero;
}

bool ConstView::isTwoState() const
{
  const std::span<const uint64_t> x = xMask();
  return std::ranges::all_of(x, [](uint64_t w) { return w == 0; });
}

bool ConstView::isFill(LogicValue value) const
{
  const uint64_t* w = words();
  const uint8_t v = static_cast<uint8_t>(value);
  const uint32_t n = numWords();
  for (uint32_t word = 0; word < n; ++word) {
    const uint64_t mask
        = word + 1 == n ? constLastWordMask(width_) : ~uint64_t{0};
    const uint64_t want_bits = (v & 1u) ? mask : 0;
    const uint64_t want_x = (v & 2u) ? mask : 0;
    if (w[word] != want_bits || w[n + word] != want_x) {
      return false;
    }
  }
  return true;
}

std::optional<uint64_t> ConstView::toUint64() const
{
  if (!isTwoState()) {
    return std::nullopt;
  }
  const std::span<const uint64_t> b = bits();
  for (size_t word = 1; word < b.size(); ++word) {
    if (b[word] != 0) {
      return std::nullopt;
    }
  }
  return b.empty() ? 0 : b[0];
}

std::string ConstView::toString() const
{
  std::string text = absl::StrCat(width_, "'b");
  text.reserve(text.size() + width_);
  for (uint32_t i = width_; i-- > 0;) {
    text.push_back(logicValueChar(bit(i)));
  }
  return text;
}

uint64_t ConstView::checksum() const
{
  uint64_t h = checksumCombine(kChecksumSeed, width_);
  for (uint64_t w : bits()) {
    h = checksumCombine(h, w);
  }
  for (uint64_t w : xMask()) {
    h = checksumCombine(h, w);
  }
  return h;
}

}  // namespace syn::lir
