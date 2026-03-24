// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include <vector>

#include "definBase.h"
#include "odb/geom.h"

namespace odb {

class dbTechLayer;
class dbInst;

class definBlockage : public definBase
{
  dbTechLayer* _layer = nullptr;
  dbInst* _inst = nullptr;
  bool _slots = false;
  bool _fills = false;
  bool _except_pg_nets = false;
  bool _pushdown = false;
  bool _soft = false;
  bool _has_min_spacing = false;
  bool _has_effective_width = false;
  int _min_spacing = 0;
  int _effective_width = 0;
  float _max_density = 0;

 public:
  // Routing Blockage interface methods
  virtual void blockageRoutingBegin(const char* layer);
  virtual void blockageRoutingComponent(const char* comp);
  virtual void blockageRoutingSlots();
  virtual void blockageRoutingFills();
  virtual void blockageRoutingExceptPGNets();
  virtual void blockageRoutingPushdown();
  virtual void blockageRoutingMinSpacing(int spacing);
  virtual void blockageRoutingEffectiveWidth(int width);
  virtual void blockageRoutingRect(int x1, int y1, int x2, int y2);
  virtual void blockageRoutingPolygon(const std::vector<Point>& points);
  virtual void blockageRoutingEnd();

  // Placement Blockage interface methods
  virtual void blockagePlacementBegin();
  virtual void blockagePlacementComponent(const char* comp);
  virtual void blockagePlacementPushdown();
  virtual void blockagePlacementSoft();
  virtual void blockagePlacementMaxDensity(double max_density);
  virtual void blockagePlacementRect(int x1, int y1, int x2, int y2);
  virtual void blockagePlacementEnd();
};

}  // namespace odb
