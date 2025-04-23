// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021-2025, The OpenROAD Authors

#include "SimulatedAnnealingCore.h"

#include <algorithm>
#include <boost/random/uniform_int_distribution.hpp>
#include <cmath>
#include <fstream>
#include <iostream>
#include <map>
#include <string>
#include <utility>
#include <vector>

#include "MplObserver.h"
#include "object.h"
#include "utl/Logger.h"

namespace mpl {

using std::string;

template <class T>
SimulatedAnnealingCore<T>::SimulatedAnnealingCore(PhysicalHierarchy* tree,
                                                  const Rect& outline,
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
                                                  utl::Logger* logger)
    : outline_(outline),
      blocked_boundaries_(tree->blocked_boundaries),
      graphics_(graphics)
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

  setBlockedBoundariesForIOs();
}

template <class T>
void SimulatedAnnealingCore<T>::setBlockedBoundariesForIOs()
{
  if (blocked_boundaries_.find(Boundary::L) != blocked_boundaries_.end()) {
    left_is_blocked_ = true;
  }

  if (blocked_boundaries_.find(Boundary::R) != blocked_boundaries_.end()) {
    right_is_blocked_ = true;
  }

  if (blocked_boundaries_.find(Boundary::B) != blocked_boundaries_.end()) {
    bottom_is_blocked_ = true;
  }

  if (blocked_boundaries_.find(Boundary::T) != blocked_boundaries_.end()) {
    top_is_blocked_ = true;
  }
}

template <class T>
void SimulatedAnnealingCore<T>::initSequencePair()
{
  if (has_initial_sequence_pair_) {
    return;
  }

  const int sequence_pair_size
      = macros_to_place_ != 0 ? macros_to_place_ : macros_.size();

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
void SimulatedAnnealingCore<T>::setNets(const std::vector<BundledNet>& nets)
{
  nets_ = nets;
}

template <class T>
void SimulatedAnnealingCore<T>::setFences(const std::map<int, Rect>& fences)
{
  fences_ = fences;
}

template <class T>
void SimulatedAnnealingCore<T>::setGuides(const std::map<int, Rect>& guides)
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
  return (width_ <= std::ceil(outline_.getWidth()))
         && (height_ <= std::ceil(outline_.getHeight()));
}

template <class T>
bool SimulatedAnnealingCore<T>::isValid(const Rect& outline) const
{
  return (width_ <= std::ceil(outline.getWidth()))
         && (height_ <= std::ceil(outline.getHeight()));
}

template <class T>
float SimulatedAnnealingCore<T>::getNormCost() const
{
  return calNormCost();
}

template <class T>
float SimulatedAnnealingCore<T>::getWidth() const
{
  return width_;
}

template <class T>
float SimulatedAnnealingCore<T>::getHeight() const
{
  return height_;
}

template <class T>
float SimulatedAnnealingCore<T>::getAreaPenalty() const
{
  const float outline_area = outline_.getWidth() * outline_.getHeight();
  return (width_ * height_) / outline_area;
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
void SimulatedAnnealingCore<T>::getMacros(std::vector<T>& macros) const
{
  macros = macros_;
}

// Private functions
template <class T>
void SimulatedAnnealingCore<T>::calOutlinePenalty()
{
  const float max_width = std::max(outline_.getWidth(), width_);
  const float max_height = std::max(outline_.getHeight(), height_);
  const float outline_area = outline_.getWidth() * outline_.getHeight();
  outline_penalty_ = max_width * max_height - outline_area;
  // normalization
  outline_penalty_ = outline_penalty_ / (outline_area);
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
  // Initialization
  wirelength_ = 0.0;
  if (core_weights_.wirelength <= 0.0) {
    return;
  }

  // calculate the total net weight
  float tot_net_weight = 0.0;
  for (const auto& net : nets_) {
    tot_net_weight += net.weight;
  }

  if (tot_net_weight <= 0.0) {
    return;
  }

  for (const auto& net : nets_) {
    T& source = macros_[net.terminals.first];
    T& target = macros_[net.terminals.second];

    if (target.isClusterOfUnplacedIOPins()) {
      addBoundaryDistToWirelength(source, target, net.weight);
      continue;
    }

    const float x1 = source.getPinX();
    const float y1 = source.getPinY();
    const float x2 = target.getPinX();
    const float y2 = target.getPinY();
    wirelength_ += net.weight * (std::abs(x2 - x1) + std::abs(y2 - y1));
  }

  // normalization
  wirelength_ = wirelength_ / tot_net_weight
                / (outline_.getHeight() + outline_.getWidth());

  if (graphics_) {
    graphics_->setWirelengthPenalty({"Wire Length",
                                     core_weights_.wirelength,
                                     wirelength_,
                                     norm_wirelength_});
  }
}

template <class T>
void SimulatedAnnealingCore<T>::addBoundaryDistToWirelength(
    const T& macro,
    const T& io,
    const float net_weight)
{
  Cluster* io_cluster = io.getCluster();
  const Rect die = io_cluster->getBBox();
  const float die_hpwl = die.getWidth() + die.getHeight();

  if (isOutsideTheOutline(macro)) {
    wirelength_ += net_weight * die_hpwl;
    return;
  }

  const float x1 = macro.getPinX();
  const float y1 = macro.getPinY();

  Boundary constraint_boundary = io_cluster->getConstraintBoundary();

  if (constraint_boundary == NONE) {
    float dist_to_left = die_hpwl;
    if (!left_is_blocked_) {
      dist_to_left = std::abs(x1 - die.xMin());
    }

    float dist_to_right = die_hpwl;
    if (!right_is_blocked_) {
      dist_to_right = std::abs(x1 - die.xMax());
    }

    float dist_to_bottom = die_hpwl;
    if (!bottom_is_blocked_) {
      dist_to_right = std::abs(y1 - die.yMin());
    }

    float dist_to_top = die_hpwl;
    if (!top_is_blocked_) {
      dist_to_top = std::abs(y1 - die.yMax());
    }

    wirelength_
        += net_weight
           * std::min(
               {dist_to_left, dist_to_right, dist_to_bottom, dist_to_top});
  } else if (constraint_boundary == Boundary::L
             || constraint_boundary == Boundary::R) {
    const float x2 = io.getPinX();
    wirelength_ += net_weight * std::abs(x2 - x1);
  } else if (constraint_boundary == Boundary::T
             || constraint_boundary == Boundary::B) {
    const float y2 = io.getPinY();
    wirelength_ += net_weight * std::abs(y2 - y1);
  }
}

// We consider the macro outside the outline based on the location of
// the pin to avoid too many checks.
template <class T>
bool SimulatedAnnealingCore<T>::isOutsideTheOutline(const T& macro) const
{
  return macro.getPinX() > outline_.getWidth()
         || macro.getPinY() > outline_.getHeight();
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
    const float lx = macros_[id].getX();
    const float ly = macros_[id].getY();
    const float ux = lx + macros_[id].getWidth();
    const float uy = ly + macros_[id].getHeight();
    // check if the macro is valid
    if (macros_[id].getWidth() * macros_[id].getHeight() <= 1e-4) {
      continue;
    }
    // check if the fence is valid
    if (macros_[id].getWidth() > (bbox.xMax() - bbox.xMin())
        || macros_[id].getHeight() > (bbox.yMax() - bbox.yMin())) {
      continue;
    }
    // check how much the macro is far from no fence violation
    const float max_x_dist = ((bbox.xMax() - bbox.xMin()) - (ux - lx)) / 2.0;
    const float max_y_dist = ((bbox.yMax() - bbox.yMin()) - (uy - ly)) / 2.0;
    const float x_dist
        = std::abs((bbox.xMin() + bbox.xMax()) / 2.0 - (lx + ux) / 2.0);
    const float y_dist
        = std::abs((bbox.yMin() + bbox.yMax()) / 2.0 - (ly + uy) / 2.0);
    // calculate x and y direction independently
    float width = x_dist <= max_x_dist ? 0.0 : (x_dist - max_x_dist);
    float height = y_dist <= max_y_dist ? 0.0 : (y_dist - max_y_dist);
    width = width / outline_.getWidth();
    height = height / outline_.getHeight();
    fence_penalty_ += width * width + height * height;
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
    const float macro_x_min = macros_[id].getX();
    const float macro_y_min = macros_[id].getY();
    const float macro_x_max = macro_x_min + macros_[id].getWidth();
    const float macro_y_max = macro_y_min + macros_[id].getHeight();

    const float overlap_width = std::min(guide.xMax(), macro_x_max)
                                - std::max(guide.xMin(), macro_x_min);
    const float overlap_height = std::min(guide.yMax(), macro_y_max)
                                 - std::max(guide.yMin(), macro_y_min);

    // maximum overlap area
    float penalty = std::min(macros_[id].getWidth(), guide.getWidth())
                    * std::min(macros_[id].getHeight(), guide.getHeight());

    // subtract overlap
    if (overlap_width > 0 && overlap_height > 0) {
      penalty -= (overlap_width * overlap_height);
    }

    guidance_penalty_ += penalty;
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
  for (auto& macro_id : pos_seq_) {
    macros_[macro_id].setX(0.0);
    macros_[macro_id].setY(0.0);
  }

  // Each index corresponds to a macro id whose pair is:
  // <Position in Positive Sequence , Position in Negative Sequence>
  std::vector<std::pair<int, int>> sequence_pair_pos(pos_seq_.size());

  // calculate X position
  for (int i = 0; i < pos_seq_.size(); i++) {
    sequence_pair_pos[pos_seq_[i]].first = i;
    sequence_pair_pos[neg_seq_[i]].second = i;
  }

  std::vector<float> accumulated_length(pos_seq_.size(), 0.0);
  for (int i = 0; i < pos_seq_.size(); i++) {
    const int macro_id = pos_seq_[i];
    const int neg_seq_pos = sequence_pair_pos[macro_id].second;

    macros_[macro_id].setX(accumulated_length[neg_seq_pos]);

    const float current_length
        = macros_[macro_id].getX() + macros_[macro_id].getWidth();

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
    accumulated_length[i] = 0.0;
  }

  for (int i = 0; i < pos_seq_.size(); i++) {
    const int macro_id = reversed_pos_seq[i];
    const int neg_seq_pos = sequence_pair_pos[macro_id].second;

    macros_[macro_id].setY(accumulated_length[neg_seq_pos]);

    const float current_height
        = macros_[macro_id].getY() + macros_[macro_id].getHeight();

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
  float delta_cost = 0.0;
  int step = 1;
  float temperature = init_temperature_;
  const float min_t = 1e-10;
  const float t_factor
      = std::exp(std::log(min_t / init_temperature_) / max_num_step_);

  // Used to ensure notch penalty is used only in the latter steps
  // as it is too expensive
  notch_weight_ = 0.0;

  int num_restart = 1;
  const int max_num_restart = 2;

  if (isValid()) {
    updateBestValidResult();
  }

  while (step <= max_num_step_) {
    for (int i = 0; i < num_perturb_per_step_; i++) {
      perturb();
      cost = calNormCost();

      const bool keep_result
          = cost < pre_cost
            || best_valid_result_.sequence_pair.pos_sequence.empty();
      if (isValid() && keep_result) {
        updateBestValidResult();
      }

      delta_cost = cost - pre_cost;
      const float num = distribution_(generator_);
      const float prob
          = (delta_cost > 0.0) ? std::exp((-1) * delta_cost / temperature) : 1;
      if (num < prob) {
        pre_cost = cost;
      } else {
        restore();
      }
    }

    temperature *= t_factor;
    step++;

    cost_list_.push_back(pre_cost);
    T_list_.push_back(temperature);

    if (best_valid_result_.macro_id_to_width.empty()
        && (num_restart <= max_num_restart)
        && (step == std::floor(max_num_step_ / max_num_restart)
            && (outline_penalty_ > 0.0))) {
      shrink();
      packFloorplan();
      calPenalty();
      pre_cost = calNormCost();
      num_restart++;
      step = 1;
      num_perturb_per_step_ *= 2;
      temperature = init_temperature_;
    }

    if (step == max_num_step_ - macros_.size() * 2) {
      notch_weight_ = original_notch_weight_;
      packFloorplan();
      calPenalty();
      pre_cost = calNormCost();
    }
  }

  packFloorplan();
  if (graphics_) {
    graphics_->doNotSkip();
  }
  calPenalty();

  if (!isValid() && !best_valid_result_.sequence_pair.pos_sequence.empty()) {
    useBestValidResult();
  }
}

template <class T>
void SimulatedAnnealingCore<T>::updateBestValidResult()
{
  best_valid_result_.sequence_pair.pos_sequence = pos_seq_;
  best_valid_result_.sequence_pair.neg_sequence = neg_seq_;

  if constexpr (std::is_same_v<T, SoftMacro>) {
    for (const int macro_id : pos_seq_) {
      SoftMacro& macro = macros_[macro_id];
      best_valid_result_.macro_id_to_width[macro_id] = macro.getWidth();
    }
  }
}

template <class T>
void SimulatedAnnealingCore<T>::useBestValidResult()
{
  pos_seq_ = best_valid_result_.sequence_pair.pos_sequence;
  neg_seq_ = best_valid_result_.sequence_pair.neg_sequence;

  if constexpr (std::is_same_v<T, SoftMacro>) {
    for (const int macro_id : pos_seq_) {
      SoftMacro& macro = macros_[macro_id];
      const float valid_result_width
          = best_valid_result_.macro_id_to_width.at(macro_id);

      if (macro.isMacroCluster()) {
        const float valid_result_height = macro.getArea() / valid_result_width;
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
    file << T_list_[i] << "  " << cost_list_[i] << std::endl;
  }
}

template class SimulatedAnnealingCore<SoftMacro>;
template class SimulatedAnnealingCore<HardMacro>;

}  // namespace mpl
