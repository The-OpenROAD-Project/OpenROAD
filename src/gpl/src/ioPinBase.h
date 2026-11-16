// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <optional>
#include <utility>
#include <vector>

#include "odb/geom.h"
#include "point.h"

namespace odb {
class dbBTerm;
class dbTechLayer;
}  // namespace odb

namespace ppl {
struct SlotPosition;
}

namespace utl {
class Logger;
}

namespace gpl {

class GPin;
class NesterovBaseCommon;
class PlacerBase;

// Concurrent IO pin placement (-place_ios). The pins take no Nesterov step:
// each iteration every pin jumps to its weighted-HPWL optimum on its locus,
// and the pins are then spaced one slot apart.
class IoPinBase
{
 public:
  enum class Iterate : uint8_t
  {
    kPrev,
    kCur,
    kNext
  };

  IoPinBase(const std::vector<ppl::SlotPosition>& slots,
            PlacerBase* pb,
            NesterovBaseCommon* nbc,
            utl::Logger* log);

  void computeNext();
  void commitNext(int iter);
  void placeGPins(Iterate iterate);
  void saveSnapshot();
  void revertToSnapshot();
  // GPin addresses change when NesterovBaseCommon::fixPointers() runs.
  void refreshGPins();
  void updateDb();

  // For debug (global_placement_debug -pin).
  size_t size() const { return bterms_.size(); }
  odb::dbBTerm* getBTerm(size_t io_index) const { return bterms_[io_index]; }
  GPin* getGPin(size_t io_index) const { return gpins_[io_index]; }
  std::optional<size_t> findPin(const odb::dbBTerm* bterm) const;
  odb::Rect pinRect(size_t io_index) const;
  // Where the pin jumps next, before the spacing.
  FloatPoint target(size_t io_index) const;
  void report(size_t io_index) const;

 private:
  enum class DieEdge : uint8_t
  {
    kLeft,
    kRight,
    kBottom,
    kTop
  };
  static bool isHorizontalEdge(DieEdge edge)
  {
    return edge == DieEdge::kBottom || edge == DieEdge::kTop;
  }

  struct PerimSegment
  {
    DieEdge edge;
    float lo;
    float hi;
  };

  struct RingSlot
  {
    DieEdge edge;
    float pos;
  };
  static constexpr std::array<DieEdge, 4> kRingOrder
      = {DieEdge::kBottom, DieEdge::kRight, DieEdge::kTop, DieEdge::kLeft};
  static bool ringAscends(DieEdge edge)
  {
    return edge == DieEdge::kBottom || edge == DieEdge::kRight;
  }

  struct NetBox
  {
    float lx, ly, ux, uy, w;
    bool mirrored;
  };

  static constexpr size_t kNoMirrorPartner = std::numeric_limits<size_t>::max();

  // Init
  void pickTopLayerGrid();
  void initMirrorPairs();
  void initSlots(const std::vector<ppl::SlotPosition>& slots);
  void initConstraints();
  FloatPoint seed(size_t io_index) const;

  // Locus: where each pin may go
  static std::vector<PerimSegment> mirrorSegments(
      const std::vector<PerimSegment>& segs);
  static std::vector<PerimSegment> intersectSegments(
      const std::vector<PerimSegment>& a,
      const std::vector<PerimSegment>& b);
  bool rectToPerimSegment(const odb::Rect& r, PerimSegment& seg) const;
  FloatPoint projectOntoSegment(const PerimSegment& seg,
                                float x,
                                float y) const;
  const PerimSegment* nearestSegment(const std::vector<PerimSegment>& segs,
                                     float x,
                                     float y,
                                     FloatPoint* projection) const;
  const std::vector<PerimSegment>& locus(size_t io_index) const;
  FloatPoint project(size_t io_index, float x, float y) const;
  DieEdge edgeOnLocus(size_t io_index, float x, float y) const;
  bool isBoxConstrained(size_t io_index) const
  {
    return box_constraints_[io_index].has_value();
  }

  // Optimum
  void netBoxes(size_t io_index, std::vector<NetBox>& boxes) const;
  double cost(size_t io_index,
              const std::vector<NetBox>& boxes,
              const FloatPoint& q) const;
  FloatPoint optimum(size_t io_index, const FloatPoint& cur) const;

  // Mirroring
  FloatPoint mirrorOf(size_t master_io, const FloatPoint& p) const;
  void applyMirrorConstraints(std::vector<FloatPoint>& coordi) const;
  bool isMirrorFollower(size_t io_index) const
  {
    return follower_to_master_[io_index] != kNoMirrorPartner;
  }
  bool isMirrorMaster(size_t io_index) const
  {
    return master_to_follower_[io_index] != kNoMirrorPartner;
  }

  // Spacing on the slots
  float ringIndexAt(DieEdge edge, float pos) const;
  FloatPoint ringPointAt(const std::vector<RingSlot>& ring, float r) const;
  void separate(std::vector<FloatPoint>& coordi) const;
  void separateMirrored(std::vector<FloatPoint>& coordi) const;

  // Db write
  odb::dbTechLayer* pinLayer(DieEdge edge, float pos, int nth) const;
  std::pair<int, int> halfSize(size_t io_index) const;

  // For debug
  void reportMoves(int iter) const;

  PlacerBase* pb_ = nullptr;
  NesterovBaseCommon* nbc_ = nullptr;
  utl::Logger* log_ = nullptr;

  // Pins
  std::vector<odb::dbBTerm*> bterms_;
  // Null for a port without a GPin.
  std::vector<GPin*> gpins_;
  std::vector<odb::Point> pin_size_;

  // Positions at the Nesterov iterates
  std::vector<FloatPoint> prev_;
  std::vector<FloatPoint> cur_;
  std::vector<FloatPoint> next_;
  std::vector<FloatPoint> snapshot_;

  // Locus
  std::vector<PerimSegment> free_segments_;
  std::vector<std::vector<PerimSegment>> constraint_segments_;
  // A 2D up: region is an area on the top-layer grid, not a perimeter
  // interval, so these pins are clamped to the box instead of projected.
  std::vector<std::optional<odb::Rect>> box_constraints_;

  // Mirroring
  std::vector<std::pair<uint32_t, uint32_t>> mirror_pairs_;
  std::vector<size_t> master_to_follower_;
  std::vector<size_t> follower_to_master_;

  // Slots
  std::array<std::vector<float>, 4> edge_slots_;
  std::array<std::vector<odb::dbTechLayer*>, 4> edge_slot_layers_;
  std::array<std::vector<float>, 2> axis_slots_;
  std::vector<RingSlot> slot_ring_;
  std::array<size_t, 4> ring_begin_{};

  // Db write
  std::vector<odb::Point> last_written_pos_;
  std::vector<odb::dbTechLayer*> last_written_layer_;
  // define_pin_shape_pattern grid, used by pins with a 2D up: region.
  odb::dbTechLayer* top_layer_ = nullptr;
  int top_pin_width_ = 0;
  int top_pin_height_ = 0;

  // For debug: the last optima before the spacing.
  std::vector<FloatPoint> optimum_;
};

// Slot index arithmetic. A pin is a (slot index, IO pin index).
float slotIndexAt(const std::vector<float>& slots, float q);
float slotCoordinateAt(const std::vector<float>& slots, float g);
float unrollRingAtWidestGap(std::vector<std::pair<float, size_t>>& pins, int n);
void poolAdjacentViolators(std::vector<float>& z, float lo, float hi);
std::vector<float> fitSpacing(const std::vector<std::pair<float, size_t>>& pins,
                              float lo,
                              int total);

}  // namespace gpl
