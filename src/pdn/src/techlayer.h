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

#include <string>
#include <vector>

#include "odb/db.h"

namespace utl {
class Logger;
}  // namespace utl

namespace pdn {

class TechLayer
{
 public:
  explicit TechLayer(odb::dbTechLayer* layer);

  std::string getName() const { return layer_->getName(); }

  odb::dbTechLayer* getLayer() const { return layer_; }

  int getLefUnits() const { return layer_->getTech()->getLefUnits(); }

  int getMinWidth() const { return layer_->getMinWidth(); }
  int getMaxWidth() const { return layer_->getMaxWidth(); }
  odb::Rect adjustToMinArea(const odb::Rect& rect) const;
  // get the spacing by also checking for spacing constraints not normally
  // checked for
  int getSpacing(int width, int length = 0) const;

  void populateGrid(odb::dbBlock* block,
                    odb::dbTechLayerDir dir = odb::dbTechLayerDir::NONE);
  int snapToGrid(int pos, int greater_than = 0) const;
  int snapToGridInterval(odb::dbBlock* block, int dist) const;
  bool hasGrid() const { return !grid_.empty(); }
  const std::vector<int>& getGrid() const { return grid_; }
  int snapToManufacturingGrid(int pos,
                              bool round_up = false,
                              int grid_multiplier = 1) const;
  static int snapToManufacturingGrid(odb::dbTech* tech,
                                     int pos,
                                     bool round_up = false,
                                     int grid_multiplier = 1);
  static bool checkIfManufacturingGrid(odb::dbTech* tech, int value);
  bool checkIfManufacturingGrid(int value,
                                utl::Logger* logger,
                                const std::string& type) const;
  int getMinIncrementStep() const;

  double dbuToMicron(int value) const;

  struct MinCutRule
  {
    odb::dbTechLayerCutClassRule* cut_class;
    bool above;
    bool below;
    int width;
    int cuts;
  };
  std::vector<MinCutRule> getMinCutRules() const;

 private:
  odb::dbTechLayer* layer_;
  std::vector<int> grid_;
};

}  // namespace pdn
