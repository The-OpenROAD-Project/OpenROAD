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
void SimulatedAnnealingCore<T>::initSequencePair()
{
  if (has_initial_sequence_pair_) {
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
}

// access functions
template <class T>
void SimulatedAnnealingCore<T>::setNets(const BundledNetList& nets)
{
  nets_ = nets;
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

  wirelength_ = computeNetsWireLength(nets_);

  if (graphics_) {
    graphics_->setWirelengthPenalty({.name = "Wire Length",
                                     .weight = core_weights_.wirelength,
                                     .value = wirelength_,
                                     .normalization_factor = norm_wirelength_});
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

// Determine the positions of macros based on sequence pair
template <class T>
void SimulatedAnnealingCore<T>::packFloorplan()
{
  // Each index corresponds to a macro id whose pair is:
  // <Position in Positive Sequence , Position in Negative Sequence>
  std::vector<std::pair<int, int>> sequence_pair_pos(pos_seq_.size());

  // calculate X position
  for (int i = 0; i < pos_seq_.size(); i++) {
    sequence_pair_pos[pos_seq_[i]].first = i;
    sequence_pair_pos[neg_seq_[i]].second = i;
  }

  std::vector<int> accumulated_length(pos_seq_.size(), 0);
  for (int i = 0; i < pos_seq_.size(); i++) {
    const int macro_id = pos_seq_[i];
    const int neg_seq_pos = sequence_pair_pos[macro_id].second;

    T& macro = macros_[macro_id];

    if (!macro.isFixed()) {
      macro.setX(accumulated_length[neg_seq_pos]);
    }

    const int current_length = macro.getX() + macro.getWidth();

    for (int j = neg_seq_pos; j < neg_seq_.size(); j++) {
      if (current_length > accumulated_length[j]) {
        accumulated_length[j] = current_length;
      } else {
        break;
      }
    }
  }

  width_ = accumulated_length[pos_seq_.size() - 1];

  // calulate Y position
  std::vector<int> reversed_pos_seq(pos_seq_.size());
  for (int i = 0; i < reversed_pos_seq.size(); i++) {
    reversed_pos_seq[i] = pos_seq_[reversed_pos_seq.size() - 1 - i];
  }

  for (int i = 0; i < pos_seq_.size(); i++) {
    sequence_pair_pos[reversed_pos_seq[i]].first = i;
    sequence_pair_pos[neg_seq_[i]].second = i;

    // This is actually the accumulated height, but we use the same vector
    // to avoid more allocation.
    accumulated_length[i] = 0;
  }

  for (int i = 0; i < pos_seq_.size(); i++) {
    const int macro_id = reversed_pos_seq[i];
    const int neg_seq_pos = sequence_pair_pos[macro_id].second;
    T& macro = macros_[macro_id];

    if (!macro.isFixed()) {
      macro.setY(accumulated_length[neg_seq_pos]);
    }

    const int current_height = macro.getY() + macro.getHeight();

    for (int j = neg_seq_pos; j < neg_seq_.size(); j++) {
      if (current_height > accumulated_length[j]) {
        accumulated_length[j] = current_height;
      } else {
        break;
      }
    }
  }

  height_ = accumulated_length[pos_seq_.size() - 1];

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
                    macro.getX(),
                    macro.getY(),
                    macro.getWidth(),
                    macro.getHeight());
  }

  // Then, the fixed terminals.
  const int number_of_moveable_macros = static_cast<int>(pos_seq_.size());
  for (int i = 0; i < macros_.size(); ++i) {
    if (i <= number_of_moveable_macros) {
      continue;
    }

    const T& macro = macros_[i];
    logger_->report("{:>11s} | ({:^8.2f} {:^8.2f}) ({:^8.2f} {:^8.2f})",
                    "fixed",
                    macro.getX(),
                    macro.getY(),
                    macro.getWidth(),
                    macro.getHeight());
  }
  logger_->report("");
}

template <class T>
void SimulatedAnnealingCore<T>::fastSA()
{
  float cost = calNormCost();
  float pre_cost = cost;
  int step = 1;
  float temperature = init_temperature_;
  const float min_t = 1e-10;
  const float t_factor
      = std::exp(std::log(min_t / init_temperature_) / max_num_step_);

  updateBestResult(cost);

  while (step <= max_num_step_) {
    for (int i = 0; i < num_perturb_per_step_; i++) {
      saveState();
      perturb();
      cost = calNormCost();

      const bool is_valid = isValid();
      if (!invalid_states_allowed_ && !is_valid) {
        restoreState();
        continue;
      }

      const bool found_new_best_result = cost < best_result_.cost;
      if ((!is_best_result_valid_ || is_valid) && found_new_best_result) {
        updateBestResult(cost);
        is_best_result_valid_ = is_valid;
      }

      const float delta_cost = cost - pre_cost;
      if (delta_cost <= 0) {
        // always accept improvements
        pre_cost = cost;
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

    temperature *= t_factor;
    step++;

    cost_list_.push_back(pre_cost);
    T_list_.push_back(temperature);
  }

  packFloorplan();
  if (graphics_) {
    graphics_->doNotSkip();
  }
  calPenalty();
  cost = calNormCost();

  const bool found_new_best_result = cost < best_result_.cost;
  if ((is_best_result_valid_ && !isValid()) || !found_new_best_result) {
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
    for (const int macro_id : pos_seq_) {
      SoftMacro& macro = macros_[macro_id];
      best_result_.macro_id_to_width[macro_id] = macro.getWidth();
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
    for (const int macro_id : pos_seq_) {
      SoftMacro& macro = macros_[macro_id];
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
