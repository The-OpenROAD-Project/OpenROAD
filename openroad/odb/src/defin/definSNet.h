// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include <cstdint>
#include <map>
#include <vector>

#include "definBase.h"
#include "definTypes.h"
#include "odb/db.h"
#include "odb/dbTypes.h"

namespace odb {

class dbSWire;
class dbNet;
class dbTechLayer;

class definSNet : public definBase
{
  bool _skip_special_wires{false};
  bool _skip_shields{false};
  bool _skip_block_wires{false};
  bool _skip_fill_wires{false};
  dbNet* _cur_net{nullptr};
  dbTechLayer* _cur_layer{nullptr};
  dbSWire* _swire{nullptr};
  dbWireType _wire_type{dbWireType::NONE};
  dbWireShapeType _wire_shape_type{dbWireShapeType::NONE};
  dbNet* _shield_net{nullptr};
  int _prev_x{0};
  int _prev_y{0};
  int _prev_ext{0};
  bool _has_prev_ext{false};
  int _width{0};
  int _point_cnt{0};

 public:
  int _snet_cnt{0};
  int _snet_iterm_cnt{0};

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
                    uint32_t mask);
  virtual void polygon(const char* layer, std::vector<defPoint>& points);
  virtual void wire(dbWireType type, const char* shield);
  virtual void path(const char* layer, int width);
  virtual void pathShape(const char* type);
  virtual void pathPoint(int x, int y, uint32_t mask);
  virtual void pathPoint(int x, int y, int ext, uint32_t mask);
  virtual void pathVia(const char* via,
                       uint32_t bottom_mask,
                       uint32_t cut_mask,
                       uint32_t top_mask);
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

  void skipSpecialWires() { _skip_special_wires = true; }
  void skipShields() { _skip_shields = true; }
  void skipBlockWires() { _skip_block_wires = true; }
  void skipFillWires() { _skip_fill_wires = true; }
};

}  // namespace odb
