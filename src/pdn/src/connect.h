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
#include <vector>

#include "shape.h"

namespace odb {
class dbVia;
class dbTechViaGenerateRule;
class dbTechLayer;
class dbTechVia;
class dbSWire;
class Rect;
}  // namespace odb

namespace pdn {

class Connect
{
 public:
  Connect(odb::dbTechLayer* layer0, odb::dbTechLayer* layer1);

  void addFixedVia(odb::dbTechViaGenerateRule* via);
  void addFixedVia(odb::dbTechVia* via);

  void setCutPitch(int x, int y);
  int getCutPitchX() const { return cut_pitch_x_; }
  int getCutPitchY() const { return cut_pitch_y_; }

  void report() const;

  odb::dbTechLayer* getLowerLayer() const { return layer0_; }
  odb::dbTechLayer* getUpperLayer() const { return layer1_; }

  bool isSingleLayerVia() const;
  bool isMultiLayerVia() const { return !isSingleLayerVia(); }
  bool isTaperedVia(const odb::Rect& lower, const odb::Rect& upper) const;

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
  bool overlaps(const Connect* connect) const;
  bool startsBelow(const Connect* connect) const;

  bool appliesToVia(const ViaPtr& via) const;

  void makeVia(odb::dbSWire* wire,
               const odb::Rect& lower_rect,
               const odb::Rect& upper_rect,
               odb::dbWireShapeType type);

  void setGrid(Grid* grid) { grid_ = grid; }
  Grid* getGrid() const { return grid_; }

 private:
  Grid* grid_;
  odb::dbTechLayer* layer0_;
  odb::dbTechLayer* layer1_;
  std::vector<odb::dbTechViaGenerateRule*> fixed_generate_vias_;
  std::vector<odb::dbTechVia*> fixed_tech_vias_;
  int cut_pitch_x_;
  int cut_pitch_y_;

  struct ViaDef
  {
    std::unique_ptr<DbVia> via;
    odb::dbTechLayer* bottom;
    odb::dbTechLayer* top;
  };
  std::map<std::pair<int, int>, std::vector<ViaDef>> vias_;
  std::vector<odb::dbTechViaGenerateRule*> generate_via_rules_;
  std::vector<odb::dbTechVia*> tech_vias_;

  std::vector<odb::dbTechLayer*> intermediate_layers_;
  std::vector<odb::dbTechLayer*> intermediate_routing_layers_;

  DbVia* makeSingleLayerVia(odb::dbBlock* block,
                            odb::dbTechLayer* lower,
                            const odb::Rect& lower_rect,
                            odb::dbTechLayer* upper,
                            const odb::Rect& upper_rect) const;
  odb::dbTechVia* findTechVia(odb::dbTechLayer* lower,
                              const odb::Rect& lower_rect,
                              odb::dbTechLayer* upper,
                              const odb::Rect& upper_rect) const;

  void populateGenerateRules();
  void populateTechVias();

  bool generateRuleContains(odb::dbTechViaGenerateRule* rule,
                            odb::dbTechLayer* lower,
                            odb::dbTechLayer* upper) const;
  bool techViaContains(odb::dbTechVia* rule,
                       odb::dbTechLayer* lower,
                       odb::dbTechLayer* upper) const;
};

}  // namespace pdn
