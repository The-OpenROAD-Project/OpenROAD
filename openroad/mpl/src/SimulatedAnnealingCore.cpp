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
SimulatedAnnealingCore<T>::SimulatedAnnealingCore(
    PhysicalHierarchy* tree,
    const odb::Rect& outline,
    const std::vector<T>& macros,
    const SACoreWeights& weights,
    const SACoreActionProbs& action_probs,
    const SAHyperparams& hyperparams,
    unsigned seed,
    MplObserver* graphics,
    utl::Logger* logger,
    odb::dbBlock* block)
    : outline_(outline), generator_(seed), graphics_(graphics), block_(block)
{
  penalties_ = {
      {.name = "Area", .weight = weights.area},
      {.name = "Outline", .weight = weights.outline},
      {.name = "Wire Length", .weight = weights.wirelength},
      {.name = "Guidance", .weight = weights.guidance},
      {.name = "Fence", .weight = weights.fence},
  };

  actions_ = {
      // Swaps two macros' positions within the positive sequence only.
      {
          .probability = action_probs.pos_swap,
          .perturb = [this] { singleSeqSwap(true); },
      },
      // Swaps two macros' positions within the negative sequence only.
      {
          .probability = action_probs.neg_swap,
          .perturb = [this] { singleSeqSwap(false); },
      },
      // Swaps two macros' positions in both sequences.
      {
          .probability = action_probs.double_swap,
          .perturb = [this] { doubleSeqSwap(); },
      },
      // Swaps which macros occupy two positions, in both sequences.
      {
          .probability = action_probs.exchange,
          .perturb = [this] { exchangeMacros(); },
      },
  };

  hyperparams_ = hyperparams;

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
std::vector<T> SimulatedAnnealingCore<T>::getMacros() const
{
  return macros_;
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
  for (int macro_id : pos_seq_) {
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

template <class T>
void SimulatedAnnealingCore<T>::perturb()
{
  if (macros_.empty()) {
    return;
  }

  const float op = distribution_(generator_);
  const Action* selected = &actions_.back();
  float cumulative_probability = 0.0f;
  for (const Action& action : actions_) {
    cumulative_probability += action.probability;
    if (op <= cumulative_probability) {
      selected = &action;
      break;
    }
  }

  selected->perturb();

  packFloorplan();
  calPenalty();
}

template <class T>
void SimulatedAnnealingCore<T>::saveState()
{
  if (macros_.empty()) {
    return;
  }

  pre_macros_ = macros_;
  pre_pos_seq_ = pos_seq_;
  pre_neg_seq_ = neg_seq_;

  pre_width_ = width_;
  pre_height_ = height_;

  savePenalties();
}

template <class T>
void SimulatedAnnealingCore<T>::restoreState()
{
  if (macros_.empty()) {
    return;
  }

  // To reduce the runtime, here we do not call PackFloorplan
  // again. So when we need to generate the final floorplan out,
  // we need to call PackFloorplan again at the end of SA process.
  std::swap(pos_seq_, pre_pos_seq_);
  std::swap(neg_seq_, pre_neg_seq_);
  std::swap(macros_, pre_macros_);

  width_ = pre_width_;
  height_ = pre_height_;

  restorePenalties();
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
void SimulatedAnnealingCore<T>::initialize()
{
  initSequencePair();
  findFixedMacros();

  std::vector<int> width_list;
  std::vector<int> height_list;
  std::vector<std::vector<float>> value_lists(penalties_.size());

  // We don't want to stop in the normalization factor setup
  MplObserver* save_graphics = graphics_;
  graphics_ = nullptr;

  for (int i = 0; i < hyperparams_.num_perturb_per_step; i++) {
    saveState();
    perturb();
    if (!invalid_states_allowed_ && !isValid()) {
      restoreState();
      continue;
    }

    // store current penalties
    width_list.push_back(width_);
    height_list.push_back(height_);
    for (int j = 0; j < penalties_.size(); j++) {
      value_lists[j].push_back(penalties_[j].value);
    }
  }
  graphics_ = save_graphics;

  // Floor near-zero normalization factors to 1.0.
  for (int j = 0; j < penalties_.size(); j++) {
    penalties_[j].normalization_factor = calAverage(value_lists[j]);
    if (penalties_[j].normalization_factor <= 1e-4) {
      penalties_[j].normalization_factor = 1.0;
    }
  }

  // Calculate initial temperature
  std::vector<float> cost_list;
  for (int i = 0; i < width_list.size(); i++) {
    width_ = width_list[i];
    height_ = height_list[i];
    for (int j = 0; j < penalties_.size(); j++) {
      penalties_[j].value = value_lists[j][i];
    }
    cost_list.push_back(calNormCost());
  }

  float delta_cost = 0.0;
  for (int i = 1; i < cost_list.size(); i++) {
    delta_cost += std::abs(cost_list[i] - cost_list[i - 1]);
  }

  if (cost_list.size() > 1 && delta_cost > 0.0) {
    init_temperature_ = (-1.0) * (delta_cost / (cost_list.size() - 1))
                        / std::log(hyperparams_.init_prob);
  } else {
    init_temperature_ = 1.0;
  }
}

template <class T>
void SimulatedAnnealingCore<T>::fastSA()
{
  float cost = calNormCost();
  float pre_cost = cost;
  int step = 1;
  float temperature = init_temperature_;
  const float min_t = 1e-10;
  const float t_factor = std::exp(std::log(min_t / init_temperature_)
                                  / hyperparams_.max_num_step);

  updateBestResult(cost);

  while (step <= hyperparams_.max_num_step) {
    for (int i = 0; i < hyperparams_.num_perturb_per_step; i++) {
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

template class SimulatedAnnealingCore<SoftMacro>;
template class SimulatedAnnealingCore<HardMacro>;

}  // namespace mpl
