// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include "Marks.h"

#include <array>
#include <cstdint>
#include <string>

#include "HmacSha256.h"

namespace wmk {

namespace {

// The tile as the PRF sees it: two big-endian int32, as bytes rather than
// text, because that is what a tile index is.
std::string tilePart(int tile_x, int tile_y)
{
  std::string out(8, '\0');
  for (int k = 0; k < 4; ++k) {
    out[k] = static_cast<char>((tile_x >> (24 - 8 * k)) & 0xff);
    out[4 + k] = static_cast<char>((tile_y >> (24 - 8 * k)) & 0xff);
  }
  return out;
}

}  // namespace

OrderedPair orderPair(const std::string& a, const std::string& b)
{
  if (b < a) {
    return {.first = b, .second = a};
  }
  return {.first = a, .second = b};
}

int placementTargetBit(const StageKey& key, const OrderedPair& pair)
{
  return hmac_digest(key, {"bit", pair.first, pair.second})[0] & 1;
}

std::array<std::uint8_t, 32> placementSortKey(const StageKey& key,
                                              int tile_x,
                                              int tile_y,
                                              const OrderedPair& pair)
{
  return hmac_digest(
      key, {"pair_sort", tilePart(tile_x, tile_y), pair.first, pair.second});
}

std::string ctsPairKey(const OrderedPair& pair)
{
  return pair.first + "+" + pair.second;
}

std::array<std::uint8_t, 32> ctsSortKey(const StageKey& key,
                                        const OrderedPair& pair)
{
  return hmac_digest(key, {"pair_sort", ctsPairKey(pair)});
}

CtsTarget ctsTarget(const StageKey& key, const OrderedPair& pair)
{
  // One digest decides both which buffer carries the mark and what parity it
  // must show, so neither is guessable from the netlist.
  const std::array<std::uint8_t, 32> d
      = hmac_digest(key, {"pair", ctsPairKey(pair), pair.first, pair.second});
  const bool target_is_first = ((d[0] >> 1) & 1) != 0;
  return {.target = target_is_first ? pair.first : pair.second,
          .other = target_is_first ? pair.second : pair.first,
          .bit = d[0] & 1};
}

}  // namespace wmk
