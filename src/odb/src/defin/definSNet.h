// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include <map>
#include <vector>

#include "definBase.h"
#include "odb/db.h"
#include "odb/odb.h"

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
  virtual void rect(const char* layer,
                    int x1,
                    int y1,
                    int x2,
                    int y2,
                    const char* type,
                    uint mask);
  virtual void polygon(const char* layer, std::vector<defPoint>& points);
  virtual void wire(dbWireType type, const char* shield);
  virtual void path(const char* layer, int width);
  virtual void pathShape(const char* type);
  virtual void pathPoint(int x, int y, uint mask);
  virtual void pathPoint(int x, int y, int ext, uint mask);
  virtual void pathVia(const char* via,
                       uint bottom_mask,
                       uint cut_mask,
                       uint top_mask);
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
  ~definSNet() override;
  void init() override;

  void skipSpecialWires() { _skip_special_wires = true; }
  void skipShields() { _skip_shields = true; }
  void skipBlockWires() { _skip_block_wires = true; }
  void skipFillWires() { _skip_fill_wires = true; }
};

}  // namespace odb
