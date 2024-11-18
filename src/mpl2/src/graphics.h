///////////////////////////////////////////////////////////////////////////
//
// BSD 3-Clause License
//
// Copyright (c) 2020, The Regents of the University of California
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
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <optional>
#include <vector>

#include "Mpl2Observer.h"
#include "gui/gui.h"

namespace mpl2 {
class SoftMacro;
class HardMacro;

class Graphics : public gui::Renderer, public Mpl2Observer
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
  void finishedClustering(Cluster* root) override;

  void setMaxLevel(int max_level) override;
  void setAreaPenalty(const Penalty& penalty) override;
  void setBoundaryPenalty(const Penalty& penalty) override;
  void setFencePenalty(const Penalty& penalty) override;
  void setGuidancePenalty(const Penalty& penalty) override;
  void setMacroBlockagePenalty(const Penalty& penalty) override;
  void setNotchPenalty(const Penalty& penalty) override;
  void setOutlinePenalty(const Penalty& penalty) override;
  void setWirelengthPenalty(const Penalty& penalty) override;
  void penaltyCalculated(float norm_cost) override;

  void drawObjects(gui::Painter& painter) override;

  void setMacroBlockages(
      const std::vector<mpl2::Rect>& macro_blockages) override;
  void setPlacementBlockages(
      const std::vector<mpl2::Rect>& placement_blockages) override;
  void setBundledNets(const std::vector<BundledNet>& bundled_nets) override;
  void setShowBundledNets(bool show_bundled_nets) override;
  void setSkipSteps(bool skip_steps) override;
  void doNotSkip() override;
  void setOnlyFinalResult(bool only_final_result) override;
  void setTargetClusterId(int target_cluster_id) override;
  void setOutline(const odb::Rect& outline) override;
  void setCurrentCluster(Cluster* current_cluster) override;

  void eraseDrawing() override;

 private:
  void resetPenalties();
  void drawCluster(Cluster* cluster, gui::Painter& painter);
  void drawAllBlockages(gui::Painter& painter);
  void drawBlockage(const Rect& blockage, gui::Painter& painter);
  template <typename T>
  void drawBundledNets(gui::Painter& painter, const std::vector<T>& macros);
  void setSoftMacroBrush(gui::Painter& painter, const SoftMacro& soft_macro);
  void fetchSoftAndHard(Cluster* parent,
                        std::vector<HardMacro>& hard,
                        std::vector<SoftMacro>& soft,
                        std::vector<std::vector<odb::Rect>>& outlines,
                        int level);
  bool isTargetCluster();

  template <typename T>
  void report(const char* name, const std::optional<T>& value);
  void report(float norm_cost);

  std::vector<SoftMacro> soft_macros_;
  std::vector<HardMacro> hard_macros_;
  std::vector<mpl2::Rect> macro_blockages_;
  std::vector<mpl2::Rect> placement_blockages_;
  std::vector<BundledNet> bundled_nets_;
  odb::Rect outline_;
  int target_cluster_id_{-1};
  std::vector<std::vector<odb::Rect>> outlines_;
  // In Soft SA, we're shaping/placing the children of a certain parent,
  // so for this case, the current cluster is actually the current parent.
  Cluster* current_cluster_{nullptr};

  bool active_ = true;
  bool coarse_;
  bool fine_;
  bool show_bundled_nets_;
  bool skip_steps_;
  bool is_skipping_;
  bool only_final_result_;
  odb::dbBlock* block_;
  utl::Logger* logger_;

  std::optional<int> max_level_;
  std::optional<Penalty> outline_penalty_;
  std::optional<Penalty> fence_penalty_;
  std::optional<Penalty> wirelength_penalty_;
  std::optional<Penalty> guidance_penalty_;
  std::optional<Penalty> boundary_penalty_;
  std::optional<Penalty> macro_blockage_penalty_;
  std::optional<Penalty> notch_penalty_;
  std::optional<Penalty> area_penalty_;

  float best_norm_cost_ = 0;
  int skipped_ = 0;

  Cluster* root_ = nullptr;
};

}  // namespace mpl2
