// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021-2025, The OpenROAD Authors

#include "SimulatedAnnealingCore.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <map>
#include <numeric>
#include <random>
#include <string>
#include <utility>
#include <vector>

#include "MplObserver.h"
#include "boost/random/uniform_int_distribution.hpp"
#include "clusterEngine.h"
#include "mpl-util.h"
#include "object.h"
#include "odb/db.h"
#include "odb/geom.h"
#include "utl/Logger.h"

namespace mpl {

using std::string;

template <class T>
SimulatedAnnealingCore<T>::SimulatedAnnealingCore(PhysicalHierarchy* tree,
                                                  const odb::Rect& outline,
                                                  const std::vector<T>& macros,
                                                  const SACoreWeights& weights,
                                                  float pos_swap_prob,
                                                  float neg_swap_prob,
                                                  float double_swap_prob,
                                                  float exchange_prob,
                                                  // Fast SA hyperparameter
                                                  float init_prob,
                                                  int max_num_step,
                                                  int num_perturb_per_step,
                                                  unsigned seed,
                                                  MplObserver* graphics,
                                                  utl::Logger* logger,
                                                  odb::dbBlock* block)
    : outline_(outline), graphics_(graphics), block_(block)
{
  core_weights_ = weights;

  pos_swap_prob_ = pos_swap_prob;
  neg_swap_prob_ = neg_swap_prob;
  double_swap_prob_ = double_swap_prob;
  exchange_prob_ = exchange_prob;

  init_prob_ = init_prob;
  max_num_step_ = max_num_step;
  num_perturb_per_step_ = num_perturb_per_step;

  // generate random
  std::mt19937 rand_gen(seed);
  generator_ = rand_gen;
  std::uniform_real_distribution<float> distribution(0.0, 1.0);
  distribution_ = distribution;

  logger_ = logger;
  macros_ = macros;

  setDieArea(tree->die_area);
  setAvailableRegionsForUnconstrainedPins(
      tree->available_regions_for_unconstrained_pins);

  io_cluster_to_constraint_ = tree->io_cluster_to_constraint;
  int max_cluster_id = -1;
  for (const auto& [cluster, constraint] : io_cluster_to_constraint_) {
    if (cluster != nullptr) {
      max_cluster_id = std::max(max_cluster_id, cluster->getId());
    }
  }
  if (max_cluster_id >= 0) {
    io_cluster_constraints_by_id_.resize(max_cluster_id + 1);
    has_io_cluster_constraint_.assign(max_cluster_id + 1, 0);
    for (const auto& [cluster, constraint] : io_cluster_to_constraint_) {
      if (cluster != nullptr && cluster->getId() >= 0) {
        io_cluster_constraints_by_id_[cluster->getId()] = constraint;
        has_io_cluster_constraint_[cluster->getId()] = 1;
      }
    }
  }
}

template <class T>
void SimulatedAnnealingCore<T>::setDieArea(const odb::Rect& die_area)
{
  die_area_ = die_area;
  die_area_.moveDelta(-outline_.xMin(), -outline_.yMin());
}

template <class T>
void SimulatedAnnealingCore<T>::setAvailableRegionsForUnconstrainedPins(
    const BoundaryRegionList& regions)
{
  available_regions_for_unconstrained_pins_ = regions;

  for (BoundaryRegion& region : available_regions_for_unconstrained_pins_) {
    region.line.addX(-outline_.xMin());
    region.line.addY(-outline_.yMin());
  }
}

template <class T>
void SimulatedAnnealingCore<T>::syncSequencePairBuffers()
{
  pre_pos_seq_ = pos_seq_;
  pre_neg_seq_ = neg_seq_;
  neg_seq_pos_.resize(macros_.size());
  fenwick_tree_.resize(pos_seq_.size() + 1, 0);
}

template <class T>
void SimulatedAnnealingCore<T>::saveSequencePairState()
{
  const size_t movable_count = pos_seq_.size();
  if (pre_pos_seq_.size() != movable_count) {
    pre_pos_seq_.resize(movable_count);
    pre_neg_seq_.resize(movable_count);
  }
  std::copy(pos_seq_.begin(), pos_seq_.end(), pre_pos_seq_.begin());
  std::copy(neg_seq_.begin(), neg_seq_.end(), pre_neg_seq_.begin());

  if (anneal_end_acceptance_ > 0.0f && core_weights_.wirelength > 0.0f
      && !nets_.empty()) {
    if (pre_pin_x_.size() < macros_.size()) {
      pre_pin_x_.resize(macros_.size(), 0);
      pre_pin_y_.resize(macros_.size(), 0);
    }
    for (const int macro_id : pos_seq_) {
      pre_pin_x_[macro_id] = macros_[macro_id].getPinX();
      pre_pin_y_[macro_id] = macros_[macro_id].getPinY();
    }
    has_pre_pin_snapshot_ = true;
    can_undo_net_wl_ = false;
    undo_net_wl_.clear();
  }
}

template <class T>
void SimulatedAnnealingCore<T>::restoreSequencePairState()
{
  if (action_id_ == 1) {
    std::copy(pre_pos_seq_.begin(), pre_pos_seq_.end(), pos_seq_.begin());
  } else if (action_id_ == 2) {
    std::copy(pre_neg_seq_.begin(), pre_neg_seq_.end(), neg_seq_.begin());
  } else if (action_id_ == 3 || action_id_ == 4) {
    std::copy(pre_pos_seq_.begin(), pre_pos_seq_.end(), pos_seq_.begin());
    std::copy(pre_neg_seq_.begin(), pre_neg_seq_.end(), neg_seq_.begin());
  }

  if (anneal_end_acceptance_ > 0.0f && wirelength_cache_valid_) {
    if (can_undo_net_wl_) {
      for (const auto& [net_idx, old_wl] : undo_net_wl_) {
        net_wl_[net_idx] = old_wl;
      }
      raw_wirelength_sum_ = pre_raw_wirelength_sum_;
      undo_net_wl_.clear();
      can_undo_net_wl_ = false;
    } else {
      wirelength_cache_valid_ = false;
    }
  }
  has_pre_pin_snapshot_ = false;
}

template <class T>
void SimulatedAnnealingCore<T>::initSequencePair()
{
  if (has_initial_sequence_pair_) {
    syncSequencePairBuffers();
    return;
  }

  const int sequence_pair_size = number_of_sequence_pair_macros_ != 0
                                     ? number_of_sequence_pair_macros_
                                     : macros_.size();

  int macro_id = 0;

  while (macro_id < sequence_pair_size) {
    pos_seq_.push_back(macro_id);
    neg_seq_.push_back(macro_id);
    pre_pos_seq_.push_back(macro_id);
    pre_neg_seq_.push_back(macro_id);

    ++macro_id;
  }

  neg_seq_pos_.resize(macros_.size());
  fenwick_tree_.resize(sequence_pair_size + 1, 0);
}

// access functions
template <class T>
void SimulatedAnnealingCore<T>::setNets(const BundledNetList& nets)
{
  nets_ = nets;
  nets_weight_sum_ = 0.0f;
  for (const auto& net : nets_) {
    nets_weight_sum_ += net.weight;
  }
  buildMacroNetIndex();
}

template <class T>
void SimulatedAnnealingCore<T>::buildMacroNetIndex()
{
  macro_to_nets_.assign(macros_.size(), {});
  for (int net_idx = 0; net_idx < static_cast<int>(nets_.size()); ++net_idx) {
    const int src = nets_[net_idx].terminals.first;
    const int dst = nets_[net_idx].terminals.second;
    if (src >= 0 && static_cast<size_t>(src) < macro_to_nets_.size()) {
      macro_to_nets_[src].push_back(net_idx);
    }
    if (dst != src && dst >= 0
        && static_cast<size_t>(dst) < macro_to_nets_.size()) {
      macro_to_nets_[dst].push_back(net_idx);
    }
  }
  net_wl_.assign(nets_.size(), 0.0f);
  net_touched_flags_.assign(nets_.size(), 0);
  raw_wirelength_sum_ = 0.0;
  pre_raw_wirelength_sum_ = 0.0;
  wirelength_cache_valid_ = false;
  has_pre_pin_snapshot_ = false;
  can_undo_net_wl_ = false;
  undo_net_wl_.clear();
}

template <class T>
void SimulatedAnnealingCore<T>::setFences(
    const std::map<int, odb::Rect>& fences)
{
  fences_ = fences;
}

template <class T>
void SimulatedAnnealingCore<T>::setGuides(
    const std::map<int, odb::Rect>& guides)
{
  guides_ = guides;
}

template <class T>
void SimulatedAnnealingCore<T>::setInitialSequencePair(
    const SequencePair& sequence_pair)
{
  if (sequence_pair.pos_sequence.empty()
      || sequence_pair.neg_sequence.empty()) {
    return;
  }

  pos_seq_ = sequence_pair.pos_sequence;
  neg_seq_ = sequence_pair.neg_sequence;
  syncSequencePairBuffers();

  has_initial_sequence_pair_ = true;
}

template <class T>
bool SimulatedAnnealingCore<T>::isValid() const
{
  return resultFitsInOutline();
}

template <class T>
bool SimulatedAnnealingCore<T>::fitsIn(const odb::Rect& outline) const
{
  return (width_ <= outline.dx()) && (height_ <= outline.dy());
}

template <class T>
float SimulatedAnnealingCore<T>::getNormCost() const
{
  return calNormCost();
}

template <class T>
int SimulatedAnnealingCore<T>::getWidth() const
{
  return width_;
}

template <class T>
int SimulatedAnnealingCore<T>::getHeight() const
{
  return height_;
}

template <class T>
int64_t SimulatedAnnealingCore<T>::getArea() const
{
  return width_ * static_cast<int64_t>(height_);
}

template <class T>
float SimulatedAnnealingCore<T>::getAreaPenalty() const
{
  return block_->dbuAreaToMicrons(getArea())
         / block_->dbuAreaToMicrons(outline_.area());
}

template <class T>
float SimulatedAnnealingCore<T>::getOutlinePenalty() const
{
  return outline_penalty_;
}

template <class T>
float SimulatedAnnealingCore<T>::getNormOutlinePenalty() const
{
  return norm_outline_penalty_;
}

template <class T>
float SimulatedAnnealingCore<T>::getWirelength() const
{
  return wirelength_;
}

template <class T>
float SimulatedAnnealingCore<T>::getNormWirelength() const
{
  return norm_wirelength_;
}

template <class T>
float SimulatedAnnealingCore<T>::getGuidancePenalty() const
{
  return guidance_penalty_;
}

template <class T>
float SimulatedAnnealingCore<T>::getNormGuidancePenalty() const
{
  return norm_guidance_penalty_;
}

template <class T>
float SimulatedAnnealingCore<T>::getFencePenalty() const
{
  return fence_penalty_;
}

template <class T>
float SimulatedAnnealingCore<T>::getNormFencePenalty() const
{
  return norm_fence_penalty_;
}

template <class T>
std::vector<T> SimulatedAnnealingCore<T>::getMacros() const
{
  return macros_;
}

// Private functions
template <class T>
void SimulatedAnnealingCore<T>::calOutlinePenalty()
{
  const int max_width = std::max(outline_.dx(), width_);
  const int max_height = std::max(outline_.dy(), height_);
  outline_penalty_
      = (max_width * static_cast<int64_t>(max_height)) - outline_.area();
  // normalization
  outline_penalty_ = outline_penalty_ / outline_.area();
  if (graphics_) {
    graphics_->setOutlinePenalty({"Outline",
                                  core_weights_.outline,
                                  outline_penalty_,
                                  norm_outline_penalty_});
  }
}

template <class T>
void SimulatedAnnealingCore<T>::calWirelength()
{
  if (core_weights_.wirelength <= 0.0) {
    return;
  }

  if (anneal_end_acceptance_ > 0.0f) {
    if (wirelength_cache_valid_ && has_pre_pin_snapshot_) {
      has_pre_pin_snapshot_ = false;
      wirelength_ = computeIncrementalNetsWireLength();
    } else {
      has_pre_pin_snapshot_ = false;
      can_undo_net_wl_ = false;
      undo_net_wl_.clear();
      wirelength_ = computeFullNetsWireLength();
    }
  } else {
    wirelength_ = computeNetsWireLength(nets_);
  }

  if (graphics_) {
    graphics_->setWirelengthPenalty({.name = "Wire Length",
                                     .weight = core_weights_.wirelength,
                                     .value = wirelength_,
                                     .normalization_factor = norm_wirelength_});
  }
}

template <class T>
float SimulatedAnnealingCore<T>::computeSingleNetWL(const BundledNet& net) const
{
  const T& source = macros_[net.terminals.first];
  const T& target = macros_[net.terminals.second];

  if (target.isClusterOfUnplacedIOPins()) {
    return static_cast<float>(
        computeWLForClusterOfUnplacedIOPins(source, target, net.weight));
  }
  const int x1 = source.getPinX();
  const int y1 = source.getPinY();
  const int x2 = target.getPinX();
  const int y2 = target.getPinY();

  return net.weight * (std::abs(x2 - x1) + std::abs(y2 - y1));
}

template <class T>
float SimulatedAnnealingCore<T>::computeFullNetsWireLength()
{
  raw_wirelength_sum_ = 0.0;
  if (nets_weight_sum_ != 0.0f) {
    if (net_wl_.size() != nets_.size()) {
      net_wl_.resize(nets_.size(), 0.0f);
    }
    for (size_t i = 0; i < nets_.size(); ++i) {
      const float wl = computeSingleNetWL(nets_[i]);
      net_wl_[i] = wl;
      raw_wirelength_sum_ += static_cast<double>(wl);
    }
  }
  wirelength_cache_valid_ = true;
  if (nets_weight_sum_ == 0.0f) {
    return 0.0f;
  }
  return static_cast<float>(raw_wirelength_sum_) / nets_weight_sum_
         / (outline_.dx() + outline_.dy());
}

template <class T>
float SimulatedAnnealingCore<T>::computeIncrementalNetsWireLength()
{
  if (nets_weight_sum_ == 0.0f || nets_.empty()) {
    pre_raw_wirelength_sum_ = raw_wirelength_sum_;
    can_undo_net_wl_ = true;
    undo_net_wl_.clear();
    return 0.0f;
  }

  moved_macros_.clear();
  for (const int macro_id : pos_seq_) {
    if (macros_[macro_id].getPinX() != pre_pin_x_[macro_id]
        || macros_[macro_id].getPinY() != pre_pin_y_[macro_id]) {
      moved_macros_.push_back(macro_id);
    }
  }

  if (moved_macros_.empty()) {
    pre_raw_wirelength_sum_ = raw_wirelength_sum_;
    can_undo_net_wl_ = true;
    undo_net_wl_.clear();
    return wirelength_;
  }

  if (moved_macros_.size() > pos_seq_.size() / 2) {
    can_undo_net_wl_ = false;
    undo_net_wl_.clear();
    return computeFullNetsWireLength();
  }

  touched_nets_.clear();
  for (const int macro_id : moved_macros_) {
    for (const int net_idx : macro_to_nets_[macro_id]) {
      if (!net_touched_flags_[net_idx]) {
        net_touched_flags_[net_idx] = 1;
        touched_nets_.push_back(net_idx);
      }
    }
  }

  pre_raw_wirelength_sum_ = raw_wirelength_sum_;
  undo_net_wl_.clear();
  undo_net_wl_.reserve(touched_nets_.size());
  for (const int net_idx : touched_nets_) {
    net_touched_flags_[net_idx] = 0;
    const float old_wl = net_wl_[net_idx];
    const float new_wl = computeSingleNetWL(nets_[net_idx]);
    undo_net_wl_.emplace_back(net_idx, old_wl);
    net_wl_[net_idx] = new_wl;
    raw_wirelength_sum_
        += static_cast<double>(new_wl) - static_cast<double>(old_wl);
  }
  can_undo_net_wl_ = true;

  return static_cast<float>(raw_wirelength_sum_) / nets_weight_sum_
         / (outline_.dx() + outline_.dy());
}

template <class T>
float SimulatedAnnealingCore<T>::computeNetsWireLength(
    const BundledNetList& nets) const
{
  float nets_wire_length = 0.0;

  if (nets_weight_sum_ != 0.0) {
    for (const auto& net : nets) {
      nets_wire_length += computeSingleNetWL(net);
    }

    nets_wire_length
        = nets_wire_length / nets_weight_sum_ / (outline_.dx() + outline_.dy());
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
    const int cluster_id = cluster->getId();
    const BoundaryRegion& constraint
        = (cluster_id >= 0
           && static_cast<size_t>(cluster_id)
                  < has_io_cluster_constraint_.size()
           && has_io_cluster_constraint_[cluster_id])
              ? io_cluster_constraints_by_id_[cluster_id]
              : io_cluster_to_constraint_.at(cluster);
    smallest_distance
        = computeDistToNearestRegion(macro_location, constraint, nullptr);
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
  fence_penalty_ = 0.0;
  if (core_weights_.fence <= 0.0 || fences_.empty()) {
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
    fence_penalty_
        += (width_ratio * width_ratio) + (height_ratio * height_ratio);
  }
  // normalization
  fence_penalty_ = fence_penalty_ / fences_.size();
  if (graphics_) {
    graphics_->setFencePenalty(
        {"Fence", core_weights_.fence, fence_penalty_, norm_fence_penalty_});
  }
}

template <class T>
void SimulatedAnnealingCore<T>::calGuidancePenalty()
{
  // Initialization
  guidance_penalty_ = 0.0;
  if (core_weights_.guidance <= 0.0 || guides_.empty()) {
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
      penalty -= (overlap.area());
    }

    guidance_penalty_ += block_->dbuAreaToMicrons(penalty);
  }

  guidance_penalty_ = guidance_penalty_ / guides_.size();

  if (graphics_) {
    graphics_->setGuidancePenalty({"Guidance",
                                   core_weights_.guidance,
                                   guidance_penalty_,
                                   norm_guidance_penalty_});
  }
}

template <class T>
int SimulatedAnnealingCore<T>::queryFenwickMax(const int pos) const
{
  int max_val = 0;
  for (int idx = pos + 1; idx > 0; idx -= idx & -idx) {
    max_val = std::max(max_val, fenwick_tree_[idx]);
  }
  return max_val;
}

template <class T>
void SimulatedAnnealingCore<T>::updateFenwickMax(const int pos,
                                                 const int n,
                                                 const int val)
{
  for (int idx = pos + 1; idx <= n; idx += idx & -idx) {
    fenwick_tree_[idx] = std::max(fenwick_tree_[idx], val);
  }
}

// Determine the positions of macros based on sequence pair
template <class T>
void SimulatedAnnealingCore<T>::packFloorplan()
{
  const int n = static_cast<int>(pos_seq_.size());
  if (n == 0) {
    width_ = 0;
    height_ = 0;
    return;
  }

  if (neg_seq_pos_.size() < macros_.size()) {
    neg_seq_pos_.resize(macros_.size());
  }
  if (static_cast<int>(fenwick_tree_.size()) < n + 1) {
    fenwick_tree_.resize(n + 1);
  }

  for (int i = 0; i < n; i++) {
    neg_seq_pos_[neg_seq_[i]] = i;
  }

  // Calculate X position using 1-indexed prefix-maximum Fenwick tree
  std::fill(fenwick_tree_.begin(), fenwick_tree_.begin() + n + 1, 0);
  for (int macro_id : pos_seq_) {
    const int neg_seq_pos = neg_seq_pos_[macro_id];
    T& macro = macros_[macro_id];

    if (!macro.isFixed()) {
      macro.setX(queryFenwickMax(neg_seq_pos));
    }

    const int current_length = macro.getX() + macro.getWidth();
    updateFenwickMax(neg_seq_pos, n, current_length);
  }

  width_ = queryFenwickMax(n - 1);

  // Calculate Y position by iterating pos_seq_ backward
  std::fill(fenwick_tree_.begin(), fenwick_tree_.begin() + n + 1, 0);
  for (int i = n - 1; i >= 0; --i) {
    const int macro_id = pos_seq_[i];
    const int neg_seq_pos = neg_seq_pos_[macro_id];
    T& macro = macros_[macro_id];

    if (!macro.isFixed()) {
      macro.setY(queryFenwickMax(neg_seq_pos));
    }

    const int current_height = macro.getY() + macro.getHeight();
    updateFenwickMax(neg_seq_pos, n, current_height);
  }

  height_ = queryFenwickMax(n - 1);

  if (graphics_) {
    graphics_->saStep(macros_);
  }
}

// SingleSeqSwap
template <class T>
void SimulatedAnnealingCore<T>::singleSeqSwap(bool pos)
{
  if (pos_seq_.size() <= 1) {
    return;
  }

  int index1 = 0, index2 = 0;
  generateRandomIndices(index1, index2);

  if (pos) {
    std::swap(pos_seq_[index1], pos_seq_[index2]);
  } else {
    std::swap(neg_seq_[index1], neg_seq_[index2]);
  }
}

// DoubleSeqSwap
template <class T>
void SimulatedAnnealingCore<T>::doubleSeqSwap()
{
  if (pos_seq_.size() <= 1) {
    return;
  }

  int index1 = 0, index2 = 0;
  generateRandomIndices(index1, index2);

  std::swap(pos_seq_[index1], pos_seq_[index2]);
  std::swap(neg_seq_[index1], neg_seq_[index2]);
}

// ExchaneMacros
template <class T>
void SimulatedAnnealingCore<T>::exchangeMacros()
{
  if (pos_seq_.size() <= 1) {
    return;
  }

  int index1 = 0, index2 = 0;
  generateRandomIndices(index1, index2);

  std::swap(pos_seq_[index1], pos_seq_[index2]);

  int neg_index1 = -1;
  int neg_index2 = -1;

  for (int i = 0; i < pos_seq_.size(); i++) {
    if (pos_seq_[index1] == neg_seq_[i]) {
      neg_index1 = i;
    }
    if (pos_seq_[index2] == neg_seq_[i]) {
      neg_index2 = i;
    }
  }

  if (neg_index1 < 0 || neg_index2 < 0) {
    logger_->error(utl::MPL,
                   18,
                   "Divergence in sequence pair: Macros ID {} or {} (or both) "
                   "exist only in positive sequence.",
                   index1,
                   index2);
  }
  std::swap(neg_seq_[neg_index1], neg_seq_[neg_index2]);
}

template <class T>
void SimulatedAnnealingCore<T>::generateRandomIndices(int& index1, int& index2)
{
  boost::random::uniform_int_distribution<> index_distribution(
      0, pos_seq_.size() - 1);

  index1 = index_distribution(generator_);
  index2 = index_distribution(generator_);

  while (index1 == index2) {
    index2 = index_distribution(generator_);
  }
}

/* static */
template <class T>
float SimulatedAnnealingCore<T>::calAverage(std::vector<float>& value_list)
{
  const auto size = value_list.size();
  if (size == 0) {
    return 0;
  }

  return std::accumulate(value_list.begin(), value_list.end(), 0.0f) / size;
}

template <class T>
void SimulatedAnnealingCore<T>::report(const PenaltyData& penalty) const
{
  logger_->report(
      "{:>15s} | {:>8.4f} | {:>7.4f} | {:>14.4f} | {:>7.4f} ",
      penalty.name,
      penalty.weight,
      penalty.value,
      penalty.normalization_factor,
      penalty.weight * penalty.value / penalty.normalization_factor);
}

template <class T>
void SimulatedAnnealingCore<T>::reportCoreWeights() const
{
  logger_->report(
      "\n  Penalty Type  |  Weight  |  Value  |  Norm. Factor  |  Cost");
  logger_->report(
      "---------------------------------------------------------------");
  report({"Area", core_weights_.area, getAreaPenalty(), 1.0f});
  report({"Outline",
          core_weights_.outline,
          outline_penalty_,
          norm_outline_penalty_});

  report(
      {"Wire Length", core_weights_.wirelength, wirelength_, norm_wirelength_});
  report({"Guidance",
          core_weights_.guidance,
          guidance_penalty_,
          norm_guidance_penalty_});
  report({"Fence", core_weights_.fence, fence_penalty_, norm_fence_penalty_});
}

template <class T>
void SimulatedAnnealingCore<T>::reportTotalCost() const
{
  logger_->report(
      "---------------------------------------------------------------");
  logger_->report("  Total Cost  {:>49.4f} \n", getNormCost());
}

template <class T>
void SimulatedAnnealingCore<T>::reportLocations() const
{
  if constexpr (std::is_same_v<T, HardMacro>) {
    logger_->report("     Id     |                                Location");
  } else {
    logger_->report(" Cluster Id |                                Location");
  }

  logger_->report("-----------------------------------------------------");

  // First the moveable macros. I.e., those from the sequence pair.
  for (const int macro_id : pos_seq_) {
    int display_id;
    if constexpr (std::is_same_v<T, SoftMacro>) {
      const SoftMacro& soft_macro = macros_[macro_id];
      Cluster* cluster = soft_macro.getCluster();
      display_id = cluster->getId();
    } else {
      display_id = macro_id;
    }

    const T& macro = macros_[macro_id];
    logger_->report("{:>11d} | ({:^8.2f} {:^8.2f}) ({:^8.2f} {:^8.2f})",
                    display_id,
                    block_->dbuToMicrons(macro.getX()),
                    block_->dbuToMicrons(macro.getY()),
                    block_->dbuToMicrons(macro.getWidth()),
                    block_->dbuToMicrons(macro.getHeight()));
  }

  // Then, the fixed terminals.
  const int number_of_moveable_macros = static_cast<int>(pos_seq_.size());
  for (int i = number_of_moveable_macros; i < macros_.size(); ++i) {
    const T& macro = macros_[i];
    logger_->report("{:>11s} | ({:^8.2f} {:^8.2f}) ({:^8.2f} {:^8.2f})",
                    "fixed",
                    block_->dbuToMicrons(macro.getX()),
                    block_->dbuToMicrons(macro.getY()),
                    block_->dbuToMicrons(macro.getWidth()),
                    block_->dbuToMicrons(macro.getHeight()));
  }
  logger_->report("");
}

template <class T>
bool SimulatedAnnealingCore<T>::shouldReplaceBestResult(
    const float cost,
    const bool is_valid) const
{
  if (useRelativeScheduleOrPatience()) {
    return (is_best_result_valid_ == is_valid && cost < best_result_.cost)
           || (!is_best_result_valid_ && is_valid);
  }
  return (!is_best_result_valid_ || is_valid) && cost < best_result_.cost;
}

template <class T>
void SimulatedAnnealingCore<T>::updateMinTemperature()
{
  min_temperature_
      = (anneal_end_acceptance_ > 0.0f)
            ? init_temperature_
                  * (std::log(0.9f) / std::log(anneal_end_acceptance_))
            : 1e-10f;
}

template <class T>
void SimulatedAnnealingCore<T>::fastSAStep(const float temperature,
                                           float& pre_cost,
                                           const int step,
                                           int& best_step)
{
  for (int i = 0; i < num_perturb_per_step_; i++) {
    saveState();
    perturb();
    const float cost = calNormCost();

    const bool is_valid = isValid();
    if (!invalid_states_allowed_ && !is_valid) {
      restoreState();
    } else {
      const bool should_update_best = shouldReplaceBestResult(cost, is_valid);
      if (should_update_best) {
        updateBestResult(cost);
        is_best_result_valid_ = is_valid;
      }

      const float delta_cost = cost - pre_cost;
      if (delta_cost <= 0) {
        // always accept improvements
        pre_cost = cost;
        if (should_update_best) {
          best_step = step;
        }
      } else {
        // probabilistically accept degradations for hill climbing
        const float num = distribution_(generator_);
        const float prob = std::exp(-delta_cost / temperature);
        if (num < prob) {
          pre_cost = cost;
        } else {
          restoreState();
        }
      }
    }
  }
}

template <class T>
void SimulatedAnnealingCore<T>::fastSA()
{
  float cost = calNormCost();
  float pre_cost = cost;
  int step = 1;
  float temperature = init_temperature_;
  updateMinTemperature();
  const float min_t = min_temperature_;
  const float t_factor
      = std::exp(std::log(min_t / init_temperature_) / max_num_step_);
  const float t_freeze = init_temperature_ * (std::log(0.9f) / std::log(0.01f));
  const int patience_limit
      = static_cast<int>(std::ceil(anneal_patience_ * max_num_step_));
  int best_step = 0;

  if (useRelativeScheduleOrPatience()) {
    is_best_result_valid_ = isValid();
  }
  updateBestResult(cost);

  while (step <= max_num_step_) {
    fastSAStep(temperature, pre_cost, step, best_step);

    temperature *= t_factor;
    step++;

    cost_list_.push_back(pre_cost);
    T_list_.push_back(temperature);

    if (anneal_patience_ > 0.0f && temperature < t_freeze
        && (step - best_step > patience_limit)) {
      break;
    }
  }

  packFloorplan();
  if (graphics_) {
    graphics_->doNotSkip();
  }
  calPenalty();
  cost = calNormCost();

  if (!shouldReplaceBestResult(cost, isValid())) {
    useBestResult();
  }
}

template <class T>
bool SimulatedAnnealingCore<T>::resultFitsInOutline() const
{
  return (width_ <= outline_.dx()) && (height_ <= outline_.dy());
}

template <class T>
void SimulatedAnnealingCore<T>::updateBestResult(const float cost)
{
  best_result_.sequence_pair.pos_sequence = pos_seq_;
  best_result_.sequence_pair.neg_sequence = neg_seq_;

  if constexpr (std::is_same_v<T, SoftMacro>) {
    const bool relative_or_patience = useRelativeScheduleOrPatience();
    if (relative_or_patience) {
      best_result_.widths.resize(macros_.size(), 0);
      best_result_.heights.resize(macros_.size(), 0);
    }
    for (const int macro_id : pos_seq_) {
      SoftMacro& macro = macros_[macro_id];
      best_result_.macro_id_to_width[macro_id] = macro.getWidth();
      if (relative_or_patience) {
        best_result_.widths[macro_id] = macro.getWidth();
        best_result_.heights[macro_id] = macro.getHeight();
      }
    }
  }

  best_result_.cost = cost;
}

template <class T>
void SimulatedAnnealingCore<T>::useBestResult()
{
  pos_seq_ = best_result_.sequence_pair.pos_sequence;
  neg_seq_ = best_result_.sequence_pair.neg_sequence;

  if constexpr (std::is_same_v<T, SoftMacro>) {
    const bool relative_or_patience = useRelativeScheduleOrPatience();
    for (const int macro_id : pos_seq_) {
      SoftMacro& macro = macros_[macro_id];
      if (relative_or_patience
          && static_cast<size_t>(macro_id) < best_result_.widths.size()) {
        macro.setShapeF(best_result_.widths[macro_id],
                        best_result_.heights[macro_id]);
      } else {
        const int valid_result_width
            = best_result_.macro_id_to_width.at(macro_id);

        if (macro.isMacroCluster()) {
          const int valid_result_height = macro.getArea() / valid_result_width;
          macro.setShapeF(valid_result_width, valid_result_height);
        } else {
          macro.setWidth(valid_result_width);
        }
      }
    }
  }

  packFloorplan();
  if (graphics_) {
    graphics_->doNotSkip();
  }
  calPenalty();
}

template <class T>
void SimulatedAnnealingCore<T>::writeCostFile(
    const std::string& file_name) const
{
  std::ofstream file(file_name);
  for (auto i = 0; i < cost_list_.size(); i++) {
    file << T_list_[i] << "  " << cost_list_[i] << '\n';
  }
}

template class SimulatedAnnealingCore<SoftMacro>;
template class SimulatedAnnealingCore<HardMacro>;

}  // namespace mpl
