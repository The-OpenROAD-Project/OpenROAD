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

#include "dbFlatten.h"
#include "dbTransform.h"
#include "dbTypes.h"
#include "dbVector.h"
#include "odb.h"

namespace odb {

class dbBlock;
class dbNet;
class dbInst;
class dbVia;
class dbWire;
class dbSWire;
class dbObstruction;
class dbBlockage;
class dbRegion;
class dbTechLayerRule;
class dbTechNonDefaultRule;
class dbProperty;
class dbCapNode;

class dbFlatten
{
  bool                                         _do_not_copy_power_wires;
  bool                                         _copy_shields;
  bool                                         _create_boundary_regions;
  bool                                         _create_bterm_map;
  bool                                         _copy_parasitics;
  std::map<dbNet*, dbNet*>                     _net_map;
  std::map<dbVia*, dbVia*>                     _via_map;
  std::map<dbInst*, dbInst*>                   _inst_map;
  std::map<int, int>                           _node_map;
  std::map<int, int>                           _shape_rc_map;
  std::map<dbTechLayerRule*, dbTechLayerRule*> _layer_rule_map;
  std::map<dbRegion*, dbRegion*>               _reg_map;
  dbTransform                                  _transform;
  char                                         _hier_d;
  int                                          _next_bterm_map_id;

  bool canCopyWire(dbWire* wire, dbSigType::Value sig_type);
  bool canCopySWire(dbSWire* wire, dbSigType::Value sig_type);
  void copySWire(dbNet* dst, dbNet* src, dbSWire* src_swire);
  void copyNetWires(dbNet* dst_, dbNet* src_, int level, dbProperty* bterm_map);
  void copyWires(dbNet*      dst_,
                 dbNet*      src_,
                 int         level,
                 dbProperty* bterm_map,
                 bool        copyParasitics);
  void copySWires(dbNet* dst_, dbNet* src_);
  void copyAttrs(dbNet* dst, dbNet* src);
  void copyAttrs(dbInst* dst, dbInst* src);
  dbNet* getParentNet(dbBlock* parent_block, dbNet* child_net);
  bool   copyInst(dbBlock* parent, dbInst* child, dbInst* grandchild);
  bool   flatten(dbBlock*    parent,
                 dbBlock*    child,
                 int         level,
                 dbProperty* bterm_map);
  dbNet* copyNet(dbBlock* parent_block, dbNet* child_net);
  dbTechNonDefaultRule* copyNonDefaultRule(dbBlock*              parent,
                                           dbInst*               child,
                                           dbTechNonDefaultRule* child_rule);
  void                  fixWire(dbVector<unsigned char>& opcodes,
                                dbVector<int>&           data,
                                dbBlock*                 src,
                                int                      level,
                                dbProperty*              bterm_map);
  void                  appendWire(dbVector<unsigned char>& opcodes,
                                   dbVector<int>&           data,
                                   dbWire*                  dst_);
  void                  copyObstruction(dbBlock* dst_block, dbObstruction* src);
  void                  copyBlockage(dbBlock* dst_block, dbBlockage* src);
  void                  copyRegion(dbBlock*  parent_block,
                                   dbInst*   child_inst,
                                   dbRegion* parent_region,
                                   dbRegion* src);
  void                  setShapeProperties(dbWire* wire);
  void                  setOldShapeIds(dbWire* wire);
  void                  mapOld2newIds(dbWire* wire, FILE* fp);
  bool                  createParentCapNode(dbCapNode* node, dbNet* dstNet);
  void                  createTop1stRseg(dbNet* src, dbNet* dst);
  dbCapNode*            checkNode(dbCapNode* src, uint srcTermId);
  uint                  adjustParentNode(dbNet* dstNet, uint srcTermId);
  uint                  adjustParentNode2(dbNet* dstNet, uint srcTermId);
  uint                  createCapNodes(dbNet* src, dbNet* dst, bool noDstWires);
  uint                  createRSegs(dbNet* src, dbNet* dst);
  uint                  printRSegs(FILE* fp, dbNet* net);
  uint                  setCorrectRsegIds(dbNet* dst);
  FILE* debugNetWires(FILE* fp, dbNet* dst, dbNet* src, const char* msg);

 public:
  dbFlatten();
  ~dbFlatten();

  void setCreateBoundaryRegions(bool value)
  {
    _create_boundary_regions = value;
  }
  void setCreateBTermMap(bool value) { _create_bterm_map = value; }
  void setCopyParasitics(bool value) { _copy_parasitics = value; }
  bool flatten(dbBlock* block, int level);
  void printShapes(FILE* fp, dbWire* wire, bool skipRCs = false);
};

}  // namespace odb
