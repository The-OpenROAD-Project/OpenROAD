// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022-2025, The OpenROAD Authors

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
