///////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// Copyright (c) 2021, The Regents of the University of California
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// * Redistributions of source code must retain the above copyright notice, this
//   list of conditions and the following disclaimer.
//
// * Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
//
// * Neither the name of the copyright holder nor the names of its
//   contributors may be used to endorse or promote products derived from
//   this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
// SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
// CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
// OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
///////////////////////////////////////////////////////////////////////////////

#include "SimulatedAnnealingCore.h"

#include <fstream>
#include <iostream>

#include "Mpl2Observer.h"
#include "object.h"
#include "utl/Logger.h"

namespace mpl2 {

using std::string;

//////////////////////////////////////////////////////////////////
// Class SimulatedAnnealingCore
template <class T>
SimulatedAnnealingCore<T>::SimulatedAnnealingCore(
    const Rect& outline,           // boundary constraints
    const std::vector<T>& macros,  // macros (T = HardMacro or T = SoftMacro)
    // weight for different penalty
    float area_weight,
    float outline_weight,
    float wirelength_weight,
    float guidance_weight,
    float fence_weight,  // each blockage will be modeled by a macro with fences
    // probability of each action
    float pos_swap_prob,
    float neg_swap_prob,
    float double_swap_prob,
    float exchange_prob,
    // Fast SA hyperparameter
    float init_prob,
    int max_num_step,
    int num_perturb_per_step,
    unsigned seed,
    Mpl2Observer* graphics,
    utl::Logger* logger)
    : outline_(outline), graphics_(graphics)
{
  area_weight_ = area_weight;
  outline_weight_ = outline_weight;
  wirelength_weight_ = wirelength_weight;
  guidance_weight_ = guidance_weight;
  fence_weight_ = fence_weight;

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
    graphics_->setOutlinePenalty(outline_penalty_);
  }
}

template <class T>
void SimulatedAnnealingCore<T>::calWirelength()
{
  // Initialization
  wirelength_ = 0.0;
  if (wirelength_weight_ <= 0.0) {
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
    const float x1 = macros_[net.terminals.first].getPinX();
    const float y1 = macros_[net.terminals.first].getPinY();
    const float x2 = macros_[net.terminals.second].getPinX();
    const float y2 = macros_[net.terminals.second].getPinY();
    wirelength_ += net.weight * (std::abs(x2 - x1) + std::abs(y2 - y1));
  }

  // normalization
  wirelength_ = wirelength_ / tot_net_weight
                / (outline_.getHeight() + outline_.getWidth());

  if (graphics_) {
    graphics_->setWirelength(wirelength_);
  }
}

template <class T>
void SimulatedAnnealingCore<T>::calFencePenalty()
{
  // Initialization
  fence_penalty_ = 0.0;
  if (fence_weight_ <= 0.0 || fences_.empty()) {
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
    graphics_->setFencePenalty(fence_penalty_);
  }
}

template <class T>
void SimulatedAnnealingCore<T>::calGuidancePenalty()
{
  // Initialization
  guidance_penalty_ = 0.0;
  if (guidance_weight_ <= 0.0 || guides_.empty()) {
    return;
  }

  for (const auto& [id, bbox] : guides_) {
    const float macro_lx = macros_[id].getX();
    const float macro_ly = macros_[id].getY();
    const float macro_ux = macro_lx + macros_[id].getWidth();
    const float macro_uy = macro_ly + macros_[id].getHeight();
    // center to center distance
    const float width
        = ((macro_ux - macro_lx) + (bbox.xMax() - bbox.xMin())) / 2.0;
    const float height
        = ((macro_uy - macro_ly) + (bbox.yMax() - bbox.yMin())) / 2.0;
    float x_dist = std::abs((macro_ux + macro_lx) / 2.0
                            - (bbox.xMax() + bbox.xMin()) / 2.0);
    float y_dist = std::abs((macro_uy + macro_ly) / 2.0
                            - (bbox.yMax() + bbox.yMin()) / 2.0);
    x_dist = std::max(x_dist - width, 0.0f) / width;
    y_dist = std::max(y_dist - height, 0.0f) / height;
    guidance_penalty_ += x_dist * x_dist + y_dist * y_dist;
  }
  guidance_penalty_ = guidance_penalty_ / guides_.size();
  if (graphics_) {
    graphics_->setGuidancePenalty(guidance_penalty_);
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

    // There may exist pin access macros with zero area in our sequence pair
    // when bus planning is on. This check is a temporary approach.
    if (macros_[macro_id].getWidth() <= 0
        || macros_[macro_id].getHeight() <= 0) {
      continue;
    }

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

    // There may exist pin access macros with zero area in our sequence pair
    // when bus planning is on. This check is a temporary approach.
    if (macros_[macro_id].getWidth() <= 0
        || macros_[macro_id].getHeight() <= 0) {
      continue;
    }

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
  index1 = (int) (std::floor(distribution_(generator_) * pos_seq_.size()));
  index2 = (int) (std::floor(distribution_(generator_) * pos_seq_.size()));

  while (index1 == index2) {
    index2 = (int) (std::floor(distribution_(generator_) * pos_seq_.size()));
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
void SimulatedAnnealingCore<T>::fastSA()
{
  if (graphics_) {
    graphics_->startSA();
  }

  // record the previous status
  float cost = calNormCost();
  float pre_cost = cost;
  float delta_cost = 0.0;
  int step = 1;
  float temperature = init_temperature_;
  const float min_t = 1e-10;
  const float t_factor
      = std::exp(std::log(min_t / init_temperature_) / max_num_step_);
  notch_weight_ = 0.0;  // notch pealty is too expensive, we try to avoid
                        // calculating notch penalty at very beginning
  // const for restart
  int num_restart = 1;
  const int max_num_restart = 2;
  // SA process
  while (step <= max_num_step_) {
    for (int i = 0; i < num_perturb_per_step_; i++) {
      perturb();
      cost = calNormCost();
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
    // temperature *= 0.985;
    temperature *= t_factor;
    cost_list_.push_back(pre_cost);
    T_list_.push_back(temperature);
    // increase step
    step++;
    // check if restart condition
    if ((num_restart <= max_num_restart)
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
    }  // end if
    // only consider the last step to optimize notch weight
    if (step == max_num_step_ - macros_.size() * 2) {
      notch_weight_ = original_notch_weight_;
      packFloorplan();
      calPenalty();
      pre_cost = calNormCost();
    }
  }  // end while
  // update the final results

  packFloorplan();
  if (graphics_) {
    graphics_->doNotSkip();
  }
  calPenalty();

  if (centralization_on_) {
    attemptCentralization(calNormCost());
  }

  if (graphics_) {
    graphics_->endSA();
  }
}

template <class T>
void SimulatedAnnealingCore<T>::attemptCentralization(const float pre_cost)
{
  if (outline_penalty_ > 0) {
    return;
  }

  // In order to revert the centralization, we cache the current location
  // of the clusters to avoid floating-point evilness when creating the
  // x,y grid to fill the dead space by expanding mixed clusters.
  std::map<int, std::pair<float, float>> clusters_locations;

  for (int& id : pos_seq_) {
    clusters_locations[id] = {macros_[id].getX(), macros_[id].getY()};
  }

  std::pair<float, float> offset((outline_.getWidth() - width_) / 2,
                                 (outline_.getHeight() - height_) / 2);
  moveFloorplan(offset);

  // revert centralization
  if (calNormCost() > pre_cost) {
    centralization_was_reverted_ = true;

    for (int& id : pos_seq_) {
      macros_[id].setX(clusters_locations[id].first);
      macros_[id].setY(clusters_locations[id].second);
    }

    if (graphics_) {
      graphics_->saStep(macros_);
    }

    calPenalty();
  }
}

template <class T>
void SimulatedAnnealingCore<T>::moveFloorplan(
    const std::pair<float, float>& offset)
{
  for (auto& id : pos_seq_) {
    macros_[id].setX(macros_[id].getX() + offset.first);
    macros_[id].setY(macros_[id].getY() + offset.second);
  }

  if (graphics_) {
    graphics_->saStep(macros_);
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

}  // namespace mpl2
