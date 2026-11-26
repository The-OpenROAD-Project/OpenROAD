// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021-2025, The OpenROAD Authors

#include <algorithm>
#include <cmath>
#include <cstdint>

#include "MplObserver.h"
#include "SACore.h"
#include "mpl-util.h"
#include "object.h"
#include "odb/db.h"
#include "odb/geom.h"
#include "utl/Logger.h"

namespace mpl {

template <class T>
void SimulatedAnnealingCore<T>::calPenalty()
{
  calAreaPenalty();
  calOutlinePenalty();
  calWirelength();
  calGuidancePenalty();
  calFencePenalty();
  calExtraPenalties();
  if (graphics_) {
    graphics_->penaltyCalculated(calNormCost());
  }
}

// Private functions
template <class T>
void SimulatedAnnealingCore<T>::calAreaPenalty()
{
  Penalty& area_penalty = penalties_[kArea];
  area_penalty.value = block_->dbuAreaToMicrons(getArea())
                       / block_->dbuAreaToMicrons(outline_.area());
  if (graphics_) {
    graphics_->setPenalty(area_penalty);
  }
}

template <class T>
void SimulatedAnnealingCore<T>::calOutlinePenalty()
{
  const int max_width = std::max(outline_.dx(), width_);
  const int max_height = std::max(outline_.dy(), height_);
  Penalty& outline_penalty = penalties_[kOutline];
  outline_penalty.value
      = (max_width * static_cast<int64_t>(max_height)) - outline_.area();
  // normalization
  outline_penalty.value = outline_penalty.value / outline_.area();
  if (graphics_) {
    graphics_->setPenalty(outline_penalty);
  }
}

template <class T>
void SimulatedAnnealingCore<T>::calWirelength()
{
  if (penalties_[kWirelength].weight <= 0.0) {
    return;
  }

  Penalty& wirelength_penalty = penalties_[kWirelength];
  wirelength_penalty.value = computeNetsWireLength(nets_);

  if (graphics_) {
    graphics_->setPenalty(wirelength_penalty);
  }
}

template <class T>
float SimulatedAnnealingCore<T>::computeNetsWireLength(
    const BundledNetList& nets) const
{
  float nets_wire_length = 0.0;
  float nets_weight_sum = 0.0;

  for (const auto& net : nets_) {
    nets_weight_sum += net.weight;
  }

  if (nets_weight_sum != 0.0) {
    for (const auto& net : nets) {
      const T& source = macros_[net.terminals.first];
      const T& target = macros_[net.terminals.second];

      if (target.isClusterOfUnplacedIOPins()) {
        nets_wire_length
            += computeWLForClusterOfUnplacedIOPins(source, target, net.weight);
      } else {
        const int x1 = source.getPinX();
        const int y1 = source.getPinY();
        const int x2 = target.getPinX();
        const int y2 = target.getPinY();

        nets_wire_length
            += net.weight * (std::abs(x2 - x1) + std::abs(y2 - y1));
      }
    }

    nets_wire_length
        = nets_wire_length / nets_weight_sum / (outline_.dx() + outline_.dy());
  }

  return nets_wire_length;
}

template <class T>
int64_t SimulatedAnnealingCore<T>::computeWLForClusterOfUnplacedIOPins(
    const T& macro,
    const T& unplaced_ios,
    const float net_weight) const
{
  // To generate maximum cost.
  const int max_dist = die_area_.margin() / 2;

  if (isOutsideTheOutline(macro)) {
    return net_weight * max_dist;
  }

  const odb::Point macro_location(macro.getPinX(), macro.getPinY());
  int64_t smallest_distance;
  if (unplaced_ios.getCluster()->isClusterOfUnconstrainedIOPins()) {
    if (available_regions_for_unconstrained_pins_.empty()) {
      logger_->critical(
          utl::MPL,
          47,
          "There's no available region for the unconstrained pins!");
    }

    smallest_distance = computeDistToNearestRegion(
        macro_location, available_regions_for_unconstrained_pins_, nullptr);
  } else {
    Cluster* cluster = unplaced_ios.getCluster();
    const BoundaryRegion& constraint = io_cluster_to_constraint_.at(cluster);
    smallest_distance
        = computeDistToNearestRegion(macro_location, {constraint}, nullptr);
  }

  return net_weight * smallest_distance;
}

// We consider the macro outside the outline based on the location of
// the pin to avoid too many checks.
template <class T>
bool SimulatedAnnealingCore<T>::isOutsideTheOutline(const T& macro) const
{
  return macro.getPinX() > outline_.dx() || macro.getPinY() > outline_.dy();
}

template <class T>
void SimulatedAnnealingCore<T>::calFencePenalty()
{
  // Initialization
  Penalty& fence_penalty = penalties_[kFence];
  fence_penalty.value = 0.0;
  if (penalties_[kFence].weight <= 0.0 || fences_.empty()) {
    return;
  }

  for (const auto& [id, bbox] : fences_) {
    const int lx = macros_[id].getX();
    const int ly = macros_[id].getY();
    const int ux = lx + macros_[id].getWidth();
    const int uy = ly + macros_[id].getHeight();
    // check if the macro is valid
    if (macros_[id].getWidth() * macros_[id].getHeight() == 0) {
      continue;
    }
    // check if the fence is valid
    if (macros_[id].getWidth() > (bbox.xMax() - bbox.xMin())
        || macros_[id].getHeight() > (bbox.yMax() - bbox.yMin())) {
      continue;
    }
    // check how much the macro is far from no fence violation
    const int max_x_dist = ((bbox.xMax() - bbox.xMin()) - (ux - lx)) / 2;
    const int max_y_dist = ((bbox.yMax() - bbox.yMin()) - (uy - ly)) / 2;
    const int x_dist = std::abs(bbox.xCenter() - ((lx + ux) / 2));
    const int y_dist = std::abs(bbox.yCenter() - ((ly + uy) / 2));
    // calculate x and y direction independently
    const int width = x_dist <= max_x_dist ? 0 : (x_dist - max_x_dist);
    const int height = y_dist <= max_y_dist ? 0 : (y_dist - max_y_dist);
    const float width_ratio = width / static_cast<float>(outline_.dx());
    const float height_ratio = height / static_cast<float>(outline_.dy());
    fence_penalty.value
        += (width_ratio * width_ratio) + (height_ratio * height_ratio);
  }
  // normalization
  fence_penalty.value = fence_penalty.value / fences_.size();
  if (graphics_) {
    graphics_->setPenalty(fence_penalty);
  }
}

template <class T>
void SimulatedAnnealingCore<T>::calGuidancePenalty()
{
  // Initialization
  Penalty& guidance_penalty = penalties_[kGuidance];
  guidance_penalty.value = 0.0;
  if (penalties_[kGuidance].weight <= 0.0 || guides_.empty()) {
    return;
  }

  for (const auto& [id, guide] : guides_) {
    odb::Rect overlap;
    macros_[id].getBBox().intersection(guide, overlap);

    // maximum overlap area
    int64_t penalty
        = std::min(macros_[id].getWidth(), guide.dx())
          * static_cast<int64_t>(std::min(macros_[id].getHeight(), guide.dy()));

    // subtract overlap
    if (overlap.dx() > 0 && overlap.dy() > 0) {
      penalty -= overlap.area();
    }

    guidance_penalty.value += block_->dbuAreaToMicrons(penalty);
  }

  guidance_penalty.value = guidance_penalty.value / guides_.size();

  if (graphics_) {
    graphics_->setPenalty(guidance_penalty);
  }
}

template <class T>
float SimulatedAnnealingCore<T>::calNormCost() const
{
  float cost = 0.0f;
  for (const Penalty& penalty : penalties_) {
    cost += penalty.weight * penalty.value / penalty.normalization_factor;
  }
  return cost;
}

template <class T>
void SimulatedAnnealingCore<T>::savePenalties()
{
  for (Penalty& penalty : penalties_) {
    penalty.pre_value = penalty.value;
  }
}

template <class T>
void SimulatedAnnealingCore<T>::restorePenalties()
{
  for (Penalty& penalty : penalties_) {
    penalty.value = penalty.pre_value;
  }
}

template class SimulatedAnnealingCore<SoftMacro>;
template class SimulatedAnnealingCore<HardMacro>;

}  // namespace mpl
