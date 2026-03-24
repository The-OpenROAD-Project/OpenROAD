// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021-2025, The OpenROAD Authors

#pragma once

#include <cstdint>
#include <limits>
#include <map>
#include <random>
#include <string>
#include <vector>

#include "MplObserver.h"
#include "clusterEngine.h"
#include "mpl-util.h"
#include "object.h"
#include "odb/db.h"
#include "odb/geom.h"

namespace utl {
class Logger;
}

namespace mpl {
struct BundledNet;
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
                         odb::dbBlock* block);

  virtual ~SimulatedAnnealingCore() = default;

  void setNumberOfSequencePairMacros(int number_of_sequence_pair_macros)
  {
    number_of_sequence_pair_macros_ = number_of_sequence_pair_macros;
  };
  void setNets(const BundledNetList& nets);
  void setFences(const std::map<int, odb::Rect>& fences);
  void setGuides(const std::map<int, odb::Rect>& guides);
  void setInitialSequencePair(const SequencePair& sequence_pair);
  void disallowInvalidStates() { invalid_states_allowed_ = false; }

  virtual bool isValid() const;
  bool fitsIn(const odb::Rect& outline) const;
  void writeCostFile(const std::string& file_name) const;
  float getNormCost() const;
  int getWidth() const;
  int getHeight() const;
  int64_t getArea() const;
  float getAreaPenalty() const;
  float getOutlinePenalty() const;
  float getNormOutlinePenalty() const;
  float getWirelength() const;
  float getNormWirelength() const;
  float getGuidancePenalty() const;
  float getNormGuidancePenalty() const;
  float getFencePenalty() const;
  float getNormFencePenalty() const;
  std::vector<T> getMacros() const;

  virtual void initialize() = 0;
  virtual void run() = 0;
  virtual void fillDeadSpace() = 0;

 protected:
  struct Result
  {
    bool empty() const { return sequence_pair.pos_sequence.empty(); }

    float cost{std::numeric_limits<float>::max()};
    SequencePair sequence_pair;
    // [Only for SoftMacro] The same sequence pair can represent different
    // floorplan arrangements depending on the macros' shapes.
    std::map<int, int> macro_id_to_width;
  };

  void fastSA();
  bool resultFitsInOutline() const;

  void setAvailableRegionsForUnconstrainedPins(
      const BoundaryRegionList& regions);
  void initSequencePair();
  void setDieArea(const odb::Rect& die_area);
  void updateBestResult(float cost);
  void useBestResult();

  virtual float calNormCost() const = 0;
  virtual void calPenalty() = 0;
  void calOutlinePenalty();
  void calWirelength();
  float computeNetsWireLength(const BundledNetList& nets) const;
  int64_t computeWLForClusterOfUnplacedIOPins(const T& macro,
                                              const T& unplaced_ios,
                                              float net_weight) const;
  bool isOutsideTheOutline(const T& macro) const;
  void calGuidancePenalty();
  void calFencePenalty();

  // operations
  void packFloorplan();
  virtual void perturb() = 0;
  virtual void saveState() = 0;
  virtual void restoreState() = 0;
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

  odb::Rect outline_;
  odb::Rect die_area_;  // Offset to the current outline.

  BoundaryRegionList available_regions_for_unconstrained_pins_;
  ClusterToBoundaryRegionMap io_cluster_to_constraint_;

  int number_of_sequence_pair_macros_ = 0;

  BundledNetList nets_;
  std::map<int, odb::Rect> fences_;  // Macro Id -> Fence
  std::map<int, odb::Rect> guides_;  // Macro Id -> Guide

  SACoreWeights core_weights_;

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
  int width_ = 0;
  int height_ = 0;
  int pre_width_ = 0;
  int pre_height_ = 0;

  float wirelength_ = 0.0;
  float outline_penalty_ = 0.0;
  float guidance_penalty_ = 0.0;
  float fence_penalty_ = 0.0;

  float pre_outline_penalty_ = 0.0;
  float pre_wirelength_ = 0.0;
  float pre_guidance_penalty_ = 0.0;
  float pre_fence_penalty_ = 0.0;

  float norm_wirelength_ = 0;
  float norm_outline_penalty_ = 0.0;
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

  Result best_result_;
  bool is_best_result_valid_ = false;

  std::vector<float> cost_list_;  // store the cost in the list
  std::vector<float> T_list_;     // store the temperature

  bool has_initial_sequence_pair_ = false;
  bool invalid_states_allowed_{true};
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
