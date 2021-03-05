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
#include <string>

#include "dbWireCodec.h"
#include "definBase.h"
#include "odb.h"

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
  bool                          _skip_signal_connections;
  bool                          _skip_wires;
  bool                          _replace_wires;
  bool                          _names_are_ids;
  bool                          _assembly_mode;
  bool                          _found_new_routing;
  dbNet*                        _cur_net;
  dbTechLayer*                  _cur_layer;
  dbWireEncoder                 _wire_encoder;
  dbWire*                       _wire;
  dbWireType                    _wire_type;
  dbWireShapeType               _wire_shape_type;
  int                           _prev_x;
  int                           _prev_y;
  int                           _width;
  int                           _point_cnt;
  dbTechLayerRule*              _taper_rule;
  dbTechNonDefaultRule*         _non_default_rule;
  dbTechNonDefaultRule*         _rule_for_path;
  std::map<std::string, dbVia*> _rotated_vias;

  void   getUniqueViaName(std::string& viaName);
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
  virtual ~definNet();
  virtual void init() override;
  void         skipWires() { _skip_wires = true; }
  void         skipConnections() { _skip_signal_connections = true; }
  void         replaceWires() { _replace_wires = true; }
  void         setAssemblyMode() { _assembly_mode = true; }
  void         namesAreDBIDs() { _names_are_ids = true; }
};

}  // namespace odb
