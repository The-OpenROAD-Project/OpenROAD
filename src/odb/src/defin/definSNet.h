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

#include <map>
#include <vector>

#include "db.h"
#include "definBase.h"
#include "odb.h"

namespace odb {

class dbSWire;
class dbNet;
class dbTechLayer;

class definSNet : public definBase
{
  bool _skip_special_wires;
  bool _skip_shields;
  bool _skip_block_wires;
  bool _skip_fill_wires;
  bool _replace_wires;
  bool _names_are_ids;
  dbNet* _cur_net;
  dbTechLayer* _cur_layer;
  dbSWire* _swire;
  dbWireType _wire_type;
  dbWireShapeType _wire_shape_type;
  dbNet* _shield_net;
  int _prev_x;
  int _prev_y;
  int _prev_ext;
  bool _has_prev_ext;
  int _width;
  int _point_cnt;

 public:
  int _snet_cnt;
  int _snet_iterm_cnt;

  /// Special Net interface methods
  virtual void begin(const char* name);
  virtual void connection(const char* iname,
                          const char* pname,
                          bool synthesized);
  virtual void use(dbSigType type);
  virtual void rect(const char* layer, int x1, int y1, int x2, int y2);
  virtual void polygon(const char* layer, std::vector<defPoint>& points);
  virtual void wire(dbWireType type, const char* shield);
  virtual void path(const char* layer, int width);
  virtual void pathShape(const char* type);
  virtual void pathPoint(int x, int y);
  virtual void pathPoint(int x, int y, int ext);
  virtual void pathVia(const char* via);
  virtual void pathViaArray(const char* via,
                            int numX,
                            int numY,
                            int stepX,
                            int stepY);
  virtual void pathEnd();
  virtual void wireEnd();
  virtual void source(dbSourceType source);
  virtual void weight(int weight);
  virtual void fixedbump();
  virtual void property(const char* name, const char* value);
  virtual void property(const char* name, int value);
  virtual void property(const char* name, double value);
  virtual void end();

  void connect_all(dbNet*, const char* term);

  definSNet();
  virtual ~definSNet();
  void init();

  void skipSpecialWires() { _skip_special_wires = true; }
  void skipShields() { _skip_shields = true; }
  void skipBlockWires() { _skip_block_wires = true; }
  void skipFillWires() { _skip_fill_wires = true; }
  void replaceWires() { _replace_wires = true; }
  void namesAreDBIDs() { _names_are_ids = true; }
};

}  // namespace odb
