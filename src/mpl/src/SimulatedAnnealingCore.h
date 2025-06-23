// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021-2025, The OpenROAD Authors

#pragma once

#include <map>
#include <random>
#include <set>
#include <string>
#include <vector>

#include "MplObserver.h"
#include "clusterEngine.h"
#include "util.h"

namespace utl {
class Logger;
}

namespace mpl {
struct BundledNet;
struct Rect;
class Graphics;

// Base class used for all annealing work within MPL.
// There are two types of SA Cores w.r.t. type of object that it receives.
// Each type has two purposes, one related to shaping, the other related
// to placement.
//
// SoftMacro:
//   - Generate tilings of mixed clusters;
//   - Cluster Placement.
//
// HardMacro:
//   - Generate tilings of macro clusters;
//   - Macro Placement.
template <class T>
class SimulatedAnnealingCore
{
 public:
  SimulatedAnnealingCore(PhysicalHierarchy* tree,
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
                         utl::Logger* logger,
                         odb::dbBlock* block);

  virtual ~SimulatedAnnealingCore() = default;

  void setNumberOfMacrosToPlace(int macros_to_place)
  {
    macros_to_place_ = macros_to_place;
  };
  void setNets(const std::vector<BundledNet>& nets);
  void setFences(const std::map<int, Rect>& fences);
  void setGuides(const std::map<int, Rect>& guides);
  void setInitialSequencePair(const SequencePair& sequence_pair);

  bool isValid() const;
  bool isValid(const Rect& outline) const;
  void writeCostFile(const std::string& file_name) const;
  float getNormCost() const;
  float getWidth() const;
  float getHeight() const;
  float getAreaPenalty() const;
  float getOutlinePenalty() const;
  float getNormOutlinePenalty() const;
  float getWirelength() const;
  float getNormWirelength() const;
  float getGuidancePenalty() const;
  float getNormGuidancePenalty() const;
  float getFencePenalty() const;
  float getNormFencePenalty() const;
  void getMacros(std::vector<T>& macros) const;

  virtual void initialize() = 0;
  virtual void run() = 0;
  virtual void fillDeadSpace() = 0;

 protected:
  struct Result
  {
    float cost{std::numeric_limits<float>::max()};
    SequencePair sequence_pair;
    // [Only for SoftMacro] The same sequence pair can represent different
    // floorplan arrangements depending on the macros' shapes.
    std::map<int, float> macro_id_to_width;
  };

  void fastSA();

  void setAvailableRegionsForUnconstrainedPins(
      const BoundaryRegionList& regions);
  void initSequencePair();
  void setDieArea(const Rect& die_area);
  void updateBestValidResult(float cost);
  void useBestValidResult();

  virtual float calNormCost() const = 0;
  virtual void calPenalty() = 0;
  void calOutlinePenalty();
  void calWirelength();
  void computeWLForClusterOfUnplacedIOPins(const T& macro,
                                           const T& unplaced_ios,
                                           float net_weight);
  bool isOutsideTheOutline(const T& macro) const;
  void calGuidancePenalty();
  void calFencePenalty();

  // operations
  void packFloorplan();
  virtual void perturb() = 0;
  virtual void restore() = 0;
  // actions used
  void singleSeqSwap(bool pos);
  void doubleSeqSwap();
  void exchangeMacros();
  void generateRandomIndices(int& index1, int& index2);

  // utilities
  static float calAverage(std::vector<float>& value_list);

  // For debugging
  void reportCoreWeights() const;
  void reportTotalCost() const;
  void reportLocations() const;
  void report(const PenaltyData& penalty) const;

  Rect outline_;
  Rect die_area_;  // Offset to the current outline.

  BoundaryRegionList available_regions_for_unconstrained_pins_;
  ClusterToBoundaryRegionMap io_cluster_to_constraint_;

  // Number of macros that will actually be part of the sequence pair
  int macros_to_place_ = 0;

  std::vector<BundledNet> nets_;
  std::map<int, Rect> fences_;  // Macro Id -> Fence
  std::map<int, Rect> guides_;  // Macro Id -> Guide

  SACoreWeights core_weights_;

  float original_notch_weight_ = 0.0;
  float notch_weight_ = 0.0;

  // Fast SA hyperparameter
  float init_prob_ = 0.0;
  float init_temperature_ = 1.0;
  int max_num_step_ = 0;
  int num_perturb_per_step_ = 0;

  // seed for reproduciabilty
  std::mt19937 generator_;
  std::uniform_real_distribution<float> distribution_;

  // current solution
  std::vector<int> pos_seq_;
  std::vector<int> neg_seq_;
  std::vector<T> macros_;  // here the macros can be HardMacro or SoftMacro

  // previous solution
  std::vector<int> pre_pos_seq_;
  std::vector<int> pre_neg_seq_;
  std::vector<T> pre_macros_;  // here the macros can be HardMacro or SoftMacro
  int macro_id_ = -1;          // the macro changed in the perturb
  int action_id_ = -1;         // the action_id of current step

  // metrics
  float width_ = 0.0;
  float height_ = 0.0;
  float pre_width_ = 0.0;
  float pre_height_ = 0.0;

  float outline_penalty_ = 0.0;
  float wirelength_ = 0.0;
  float guidance_penalty_ = 0.0;
  float fence_penalty_ = 0.0;

  float pre_outline_penalty_ = 0.0;
  float pre_wirelength_ = 0.0;
  float pre_guidance_penalty_ = 0.0;
  float pre_fence_penalty_ = 0.0;

  float norm_outline_penalty_ = 0.0;
  float norm_wirelength_ = 0.0;
  float norm_guidance_penalty_ = 0.0;
  float norm_fence_penalty_ = 0.0;
  float norm_area_penalty_ = 0.0;

  // probability of each action
  float pos_swap_prob_ = 0.0;
  float neg_swap_prob_ = 0.0;
  float double_swap_prob_ = 0.0;
  float exchange_prob_ = 0.0;

  utl::Logger* logger_ = nullptr;
  MplObserver* graphics_ = nullptr;
  odb::dbBlock* block_;

  Result best_valid_result_;

  std::vector<float> cost_list_;  // store the cost in the list
  std::vector<float> T_list_;     // store the temperature
  // we define accuracy to determine whether the floorplan is valid
  // because the error introduced by the type conversion
  static constexpr float acc_tolerance_ = 0.001;

  bool has_initial_sequence_pair_ = false;
};

// SACore wrapper function
// T can be SACoreHardMacro or SACoreSoftMacro
template <class T>
void runSA(T* sa_core)
{
  sa_core->initialize();
  sa_core->run();
}

}  // namespace mpl
