// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include "Wirelength.h"

#include <algorithm>
#include <cstdint>
#include <map>
#include <tuple>
#include <utility>
#include <vector>

#include "odb/db.h"
#include "odb/dbWireCodec.h"

namespace wmk {

using odb::dbNet;
using odb::dbTechLayer;
using odb::dbTechLayerDir;
using odb::dbWire;
using odb::dbWireDecoder;

namespace {

// Total length of the union of a set of intervals.  Touching endpoints merge,
// so two records that meet end to end count once, not twice.
std::int64_t mergedLength(std::vector<std::pair<int, int>>& intervals)
{
  if (intervals.empty()) {
    return 0;
  }
  std::ranges::sort(intervals);
  std::int64_t total = 0;
  int lo = intervals[0].first;
  int hi = intervals[0].second;
  for (size_t i = 1; i < intervals.size(); ++i) {
    if (intervals[i].first > hi) {
      total += hi - lo;
      lo = intervals[i].first;
      hi = intervals[i].second;
    } else if (intervals[i].second > hi) {
      hi = intervals[i].second;
    }
  }
  total += hi - lo;
  return total;
}

}  // namespace

bool isRoutableSignalNet(dbNet* net)
{
  if (net->isSpecial()) {
    return false;
  }
  const odb::dbSigType sig = net->getSigType();
  return !sig.isSupply() && sig != odb::dbSigType::CLOCK;
}

void canonicalWirelength(dbNet* net,
                         std::int64_t& l_ww,
                         std::int64_t& l_tot,
                         int& diagonal_segments)
{
  l_ww = 0;
  l_tot = 0;
  dbWire* wire = net->getWire();
  if (wire == nullptr) {
    return;
  }

  // (layer, horizontal?, the fixed coordinate) -> intervals along the free axis
  std::map<std::tuple<dbTechLayer*, bool, int>,
           std::vector<std::pair<int, int>>>
      buckets;

  dbWireDecoder dec;
  dec.begin(wire);
  dbTechLayer* layer = nullptr;
  int prev_x = 0;
  int prev_y = 0;
  bool have_prev = false;

  for (;;) {
    const dbWireDecoder::OpCode op = dec.next();
    if (op == dbWireDecoder::END_DECODE) {
      break;
    }
    switch (op) {
      case dbWireDecoder::PATH:
      case dbWireDecoder::JUNCTION:
      case dbWireDecoder::SHORT:
      case dbWireDecoder::VWIRE:
        layer = dec.getLayer();
        have_prev = false;  // fresh path: the next point is the anchor
        break;
      case dbWireDecoder::VIA:
      case dbWireDecoder::TECH_VIA:
        // The decoder moves to the via's far layer, choosing top or bottom by
        // the direction of travel, and that is the layer the following run
        // sits on.  Reading it is what keeps the run's wrong-way test against
        // the right preferred direction.  The via itself is one point, so the
        // anchor does not move and no length is added.
        layer = dec.getLayer();
        break;
      case dbWireDecoder::POINT:
      case dbWireDecoder::POINT_EXT: {
        int x = 0;
        int y = 0;
        if (op == dbWireDecoder::POINT) {
          dec.getPoint(x, y);
        } else {
          int ext = 0;
          dec.getPoint(x, y, ext);
        }
        if (have_prev && layer != nullptr) {
          const int dx = x - prev_x;
          const int dy = y - prev_y;
          if (dx != 0 && dy == 0) {
            buckets[{layer, true, prev_y}].emplace_back(std::min(x, prev_x),
                                                        std::max(x, prev_x));
          } else if (dy != 0 && dx == 0) {
            buckets[{layer, false, prev_x}].emplace_back(std::min(y, prev_y),
                                                         std::max(y, prev_y));
          } else if (dx != 0 && dy != 0) {
            // A Manhattan router should never emit these.
            ++diagonal_segments;
          }
        }
        prev_x = x;
        prev_y = y;
        have_prev = true;
        break;
      }
      case dbWireDecoder::RECT:
      case dbWireDecoder::ITERM:
      case dbWireDecoder::BTERM:
      case dbWireDecoder::RULE:
      case dbWireDecoder::END_DECODE:
        // No planar wirelength, and no effect on the layer or the anchor.
        break;
    }
  }

  for (auto& [bkey, intervals] : buckets) {
    const std::int64_t len = mergedLength(intervals);
    l_tot += len;
    dbTechLayer* bucket_layer = std::get<0>(bkey);
    const bool horizontal_segment = std::get<1>(bkey);
    const dbTechLayerDir dir = bucket_layer->getDirection();
    // A horizontal run is wrong-way on a vertical-preferred layer, and the
    // other way round.  A layer with no stated direction constrains nothing.
    if ((dir == dbTechLayerDir::VERTICAL && horizontal_segment)
        || (dir == dbTechLayerDir::HORIZONTAL && !horizontal_segment)) {
      l_ww += len;
    }
  }
}

}  // namespace wmk
