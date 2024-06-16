///////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// Copyright (c) 2019, Nefelus Inc
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

#include "definBase.h"
#include "odb/odb.h"

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

  definBlockage();
  virtual ~definBlockage();
  void init();
};

}  // namespace odb
