// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include <map>
#include <string>

#include "definBase.h"
#include "odb/dbWireCodec.h"
#include "odb/odb.h"

namespace odb {

class dbWire;
class dbSWire;
class dbNet;
class dbVia;
class dbTechLayer;
class dbTechLayerRule;
class dbTechNonDefaultRule;

class definNet : public definBase
{
  bool _skip_signal_connections;
  bool _skip_wires;
  dbNet* _cur_net;
  dbTechLayer* _cur_layer;
  dbWireEncoder _wire_encoder;
  dbWire* _wire;
  dbWireType _wire_type;
  dbWireShapeType _wire_shape_type;
  int _prev_x;
  int _prev_y;
  int _width;
  int _point_cnt;
  dbTechLayerRule* _taper_rule;
  dbTechNonDefaultRule* _non_default_rule;
  dbTechNonDefaultRule* _rule_for_path;
  std::map<std::string, dbVia*> _rotated_vias;

  void getUniqueViaName(std::string& viaName);
  dbVia* getRotatedVia(const char* via_name, dbOrientType orient);
  dbTechNonDefaultRule* findNonDefaultRule(const char* name);

 public:
  int _net_cnt;
  uint _update_cnt;
  int _net_iterm_cnt;

  /// Net interface methods
  void begin(const char* name);
  void beginMustjoin(const char* iname, const char* pname);
  void connection(const char* iname, const char* pname);
  void nonDefaultRule(const char* rule);
  void use(dbSigType type);
  void wire(dbWireType type);
  void path(const char* layer);
  void pathTaper(const char* layer);
  void pathTaperRule(const char* layer, const char* rule);
  void pathPoint(int x, int y);
  void pathPoint(int x, int y, int ext);
  void pathVia(const char* via);
  void pathVia(const char* via, dbOrientType orient);
  void pathRect(int deltaX1, int deltaY1, int deltaX2, int deltaY2);
  void pathColor(int color);
  void pathViaColor(int bottom_color, int cut_color, int top_color);
  void pathEnd();
  void wireEnd();
  void source(dbSourceType source);
  void weight(int weight);
  void fixedbump();
  void xtalk(int value);
  void property(const char* name, const char* value);
  void property(const char* name, int value);
  void property(const char* name, double value);
  void end();

  void pathBegin(const char* layer);
  // void netBeginCreate( const char * name );
  // void netBeginReplace( const char * name );

  dbTechLayer* getLayer() const { return _cur_layer; }

  definNet();
  ~definNet() override;
  void init() override;
  void skipWires() { _skip_wires = true; }
  void skipConnections() { _skip_signal_connections = true; }
};

}  // namespace odb
