///////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// Copyright (c) 2022, The Regents of the University of California
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

#pragma once

#include <map>
#include <set>
#include <vector>

#include "odb/db.h"
#include "shape.h"
#include "via.h"

namespace pdn {

class Connect
{
 public:
  Connect(Grid* grid, odb::dbTechLayer* layer0, odb::dbTechLayer* layer1);

  void addFixedVia(odb::dbTechViaGenerateRule* via);
  void addFixedVia(odb::dbTechVia* via);

  void setCutPitch(int x, int y);
  int getCutPitchX() const { return cut_pitch_x_; }
  int getCutPitchY() const { return cut_pitch_y_; }

  void setMaxRows(int rows) { max_rows_ = rows; }
  int getMaxRows() const { return max_rows_; }

  void setMaxColumns(int cols) { max_columns_ = cols; }
  int getMaxColumns() const { return max_columns_; }

  void setOnGrid(const std::vector<odb::dbTechLayer*>& layers);

  void setSplitCuts(const std::map<odb::dbTechLayer*, int>& splits);
  int getSplitCutPitch(odb::dbTechLayer* layer) const;

  void report() const;

  odb::dbTechLayer* getLowerLayer() const { return layer0_; }
  odb::dbTechLayer* getUpperLayer() const { return layer1_; }

  bool isSingleLayerVia() const;
  bool isMultiLayerVia() const { return !isSingleLayerVia(); }

  bool hasCutPitch() const { return cut_pitch_x_ != 0 || cut_pitch_y_ != 0; }

  const std::vector<odb::dbTechLayer*>& getIntermediteLayers() const
  {
    return intermediate_layers_;
  }
  const std::vector<odb::dbTechLayer*>& getIntermediteRoutingLayers() const
  {
    return intermediate_routing_layers_;
  }
  std::vector<odb::dbTechLayer*> getAllLayers() const;
  std::vector<odb::dbTechLayer*> getAllRoutingLayers() const;
  bool containsIntermediateLayer(odb::dbTechLayer* layer) const;
  bool overlaps(const Connect* other) const;
  bool startsBelow(const Connect* other) const;

  bool appliesToVia(const ViaPtr& via) const;

  void makeVia(odb::dbSWire* wire,
               const ShapePtr& lower,
               const ShapePtr& upper,
               const odb::dbWireShapeType& type,
               DbVia::ViaLayerShape& via_shapes);

  void setGrid(Grid* grid) { grid_ = grid; }
  Grid* getGrid() const { return grid_; }

  void clearShapes();

  void filterVias(const std::string& filter);

  void printViaReport() const;

  void addFailedVia(failedViaReason reason,
                    const odb::Rect& rect,
                    odb::dbNet* net);
  void recordFailedVias() const;

 private:
  Grid* grid_;
  odb::dbTechLayer* layer0_;
  odb::dbTechLayer* layer1_;
  std::vector<odb::dbTechViaGenerateRule*> fixed_generate_vias_;
  std::vector<odb::dbTechVia*> fixed_tech_vias_;
  int cut_pitch_x_ = 0;
  int cut_pitch_y_ = 0;

  int max_rows_ = 0;
  int max_columns_ = 0;

  std::set<odb::dbTechLayer*> ongrid_;
  std::map<odb::dbTechLayer*, int> split_cuts_;

  // map of built vias, where the key is the width and height of the via
  // intersection, and the value points of the associated via stack.
  using ViaIndex = std::pair<int, int>;
  std::map<ViaIndex, std::unique_ptr<DbGenerateStackedVia>> vias_;
  std::vector<odb::dbTechViaGenerateRule*> generate_via_rules_;
  std::vector<odb::dbTechVia*> tech_vias_;

  std::vector<odb::dbTechLayer*> intermediate_layers_;
  std::vector<odb::dbTechLayer*> intermediate_routing_layers_;

  std::map<failedViaReason, std::set<std::pair<odb::dbNet*, odb::Rect>>>
      failed_vias_;

  DbVia* makeSingleLayerVia(
      odb::dbBlock* block,
      odb::dbTechLayer* lower,
      const std::set<odb::Rect>& lower_rects,
      const ViaGenerator::Constraint& lower_constraint,
      odb::dbTechLayer* upper,
      const std::set<odb::Rect>& upper_rects,
      const ViaGenerator::Constraint& upper_constraint) const;

  void populateDBVias();
  void populateGenerateRules();
  void populateTechVias();

  bool generateRuleContains(odb::dbTechViaGenerateRule* rule,
                            odb::dbTechLayer* lower,
                            odb::dbTechLayer* upper) const;
  bool techViaContains(odb::dbTechVia* via,
                       odb::dbTechLayer* lower,
                       odb::dbTechLayer* upper) const;

  int getSplitCut(odb::dbTechLayer* layer) const;

  DbVia* generateDbVia(
      const std::vector<std::shared_ptr<ViaGenerator>>& generators,
      odb::dbBlock* block) const;

  using ViaLayerRects = std::set<odb::Rect>;
  bool isComplexStackedVia(const odb::Rect& lower,
                           const odb::Rect& upper) const;
  std::vector<ViaLayerRects> generateViaRects(const odb::Rect& lower,
                                              const odb::Rect& upper) const;
  std::vector<ViaLayerRects> generateComplexStackedViaRects(
      const odb::Rect& lower,
      const odb::Rect& upper) const;
  void generateMinEnclosureViaRects(std::vector<ViaLayerRects>& rects) const;

  int getMinWidth(odb::dbTechLayer* layer) const;
  int getMaxEnclosureFromCutLayer(odb::dbTechLayer* layer, int min_width) const;
};

}  // namespace pdn
