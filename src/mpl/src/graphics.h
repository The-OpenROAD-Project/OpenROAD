// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021-2025, The OpenROAD Authors

#pragma once

#include <map>
#include <optional>
#include <string>
#include <vector>

#include "MplObserver.h"
#include "clusterEngine.h"
#include "gui/gui.h"
#include "object.h"
#include "odb/db.h"
#include "odb/geom.h"
#include "util.h"

namespace mpl {
class SoftMacro;
class HardMacro;

class Graphics : public gui::Renderer, public MplObserver
{
 public:
  Graphics(bool coarse, bool fine, odb::dbBlock* block, utl::Logger* logger);

  ~Graphics() override = default;

  void startCoarse() override;
  void startFine() override;

  void startSA() override;
  void saStep(const std::vector<SoftMacro>& macros) override;
  void saStep(const std::vector<HardMacro>& macros) override;
  void endSA(float norm_cost) override;
  void drawResult() override;
  void finishedClustering(PhysicalHierarchy* tree) override;

  void setMaxLevel(int max_level) override;
  void setAreaPenalty(const PenaltyData& penalty) override;
  void setBoundaryPenalty(const PenaltyData& penalty) override;
  void setFencePenalty(const PenaltyData& penalty) override;
  void setGuidancePenalty(const PenaltyData& penalty) override;
  void setMacroBlockagePenalty(const PenaltyData& penalty) override;
  void setNotchPenalty(const PenaltyData& penalty) override;
  void setOutlinePenalty(const PenaltyData& penalty) override;
  void setWirelengthPenalty(const PenaltyData& penalty) override;
  void penaltyCalculated(float norm_cost) override;

  void drawObjects(gui::Painter& painter) override;

  void setMacroBlockages(
      const std::vector<mpl::Rect>& macro_blockages) override;
  void setPlacementBlockages(
      const std::vector<mpl::Rect>& placement_blockages) override;
  void setBundledNets(const std::vector<BundledNet>& bundled_nets) override;
  void setShowBundledNets(bool show_bundled_nets) override;
  void setShowClustersIds(bool show_clusters_ids) override;
  void setSkipSteps(bool skip_steps) override;
  void doNotSkip() override;
  void setOnlyFinalResult(bool only_final_result) override;
  void setTargetClusterId(int target_cluster_id) override;
  void setOutline(const odb::Rect& outline) override;
  void setCurrentCluster(Cluster* current_cluster) override;
  void setGuides(const std::map<int, Rect>& guides) override;
  void setFences(const std::map<int, Rect>& fences) override;
  void setIOConstraintsMap(
      const ClusterToBoundaryRegionMap& io_cluster_to_constraint) override;
  void setBlockedRegionsForPins(
      const std::vector<odb::Rect>& blocked_regions_for_pins) override;
  void setAvailableRegionsForUnconstrainedPins(
      const BoundaryRegionList& regions) override;

  void eraseDrawing() override;

 private:
  void setXMarksSize();
  void resetPenalties();
  void drawCluster(Cluster* cluster, gui::Painter& painter);
  void drawBlockedRegionsIndication(gui::Painter& painter);
  void drawAllBlockages(gui::Painter& painter);
  void drawOffsetRect(const Rect& rect,
                      const std::string& center_text,
                      gui::Painter& painter);
  void drawFences(gui::Painter& painter);
  void drawGuides(gui::Painter& painter);
  template <typename T>
  void drawBundledNets(gui::Painter& painter, const std::vector<T>& macros);
  template <typename T>
  void drawDistToRegion(gui::Painter& painter, const T& macro, const T& io);
  template <typename T>
  bool isOutsideTheOutline(const T& macro) const;
  void addOutlineOffsetToLine(odb::Point& from, odb::Point& to);
  void setSoftMacroBrush(gui::Painter& painter, const SoftMacro& soft_macro);
  void fetchSoftAndHard(Cluster* parent,
                        std::vector<HardMacro>& hard,
                        std::vector<SoftMacro>& soft,
                        std::vector<std::vector<odb::Rect>>& outlines,
                        int level);
  bool isTargetCluster();
  template <typename T>
  bool isSkippable(const T& macro);

  template <typename T>
  void report(const std::optional<T>& value);
  void report(float norm_cost);

  std::vector<SoftMacro> soft_macros_;
  std::vector<HardMacro> hard_macros_;
  std::vector<mpl::Rect> macro_blockages_;
  std::vector<mpl::Rect> placement_blockages_;
  std::vector<BundledNet> bundled_nets_;
  odb::Rect outline_;
  int target_cluster_id_{-1};
  std::vector<std::vector<odb::Rect>> outlines_;
  std::vector<odb::Rect> blocked_regions_for_pins_;
  BoundaryRegionList available_regions_for_unconstrained_pins_;
  ClusterToBoundaryRegionMap io_cluster_to_constraint_;

  // In Soft SA, we're shaping/placing the children of a certain parent,
  // so for this case, the current cluster is actually the current parent.
  Cluster* current_cluster_{nullptr};
  std::map<int, Rect> guides_;  // Id -> Guidance Region
  std::map<int, Rect> fences_;  // Id -> Fence

  int x_mark_size_{0};  // For blocked regions.

  bool active_ = true;
  bool coarse_;
  bool fine_;
  bool show_bundled_nets_;
  bool show_clusters_ids_;
  bool skip_steps_;
  bool is_skipping_;
  bool only_final_result_;
  odb::dbBlock* block_;
  utl::Logger* logger_;

  std::optional<int> max_level_;
  std::optional<PenaltyData> outline_penalty_;
  std::optional<PenaltyData> fence_penalty_;
  std::optional<PenaltyData> wirelength_penalty_;
  std::optional<PenaltyData> guidance_penalty_;
  std::optional<PenaltyData> boundary_penalty_;
  std::optional<PenaltyData> macro_blockage_penalty_;
  std::optional<PenaltyData> notch_penalty_;
  std::optional<PenaltyData> area_penalty_;

  float best_norm_cost_ = 0;
  int skipped_ = 0;

  Cluster* root_ = nullptr;
};

}  // namespace mpl
