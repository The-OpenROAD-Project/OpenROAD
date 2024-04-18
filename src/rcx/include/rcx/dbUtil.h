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

#include <cstdint>
#include <map>
#include <vector>

#include "odb/db.h"
#include "odb/dbMatrix.h"
#include "odb/dbTypes.h"
#include "odb/geom.h"

namespace utl {
class Logger;
}

namespace rcx {

using odb::dbBlock;
using odb::dbBox;
using odb::dbBTerm;
using odb::dbInst;
using odb::dbITerm;
using odb::dbMatrix;
using odb::dbNet;
using odb::dbObject;
using odb::dbSBox;
using odb::dbShape;
using odb::dbTech;
using odb::dbTechLayer;
using odb::dbTechLayerDir;
using odb::dbTechLayerRule;
using odb::dbTechNonDefaultRule;
using odb::dbTechVia;
using odb::Rect;

//
// This class creates a new net along with a wire.
//
class dbCreateNetUtil
{
 public:
  dbCreateNetUtil(utl::Logger* logger);
  ~dbCreateNetUtil();

  void setBlock(dbBlock* block, bool skipInit = false);
  dbBlock* getBlock() const { return _block; }
  dbNet* createNetSingleWire(const char* name,
                             int x1,
                             int y1,
                             int x2,
                             int y2,
                             int rlevel,
                             bool skipBterms = false,
                             bool skipNetExists = false,
                             uint8_t color = 0);
  dbNet* createNetSingleWire(const char* name,
                             int x1,
                             int y1,
                             int x2,
                             int y2,
                             int rlevel,
                             dbTechLayerDir dir,
                             bool skipBterms = false);

  dbNet* createNetSingleWire(Rect& r, uint level, uint netId, uint shapeId);
  dbSBox* createSpecialWire(dbNet* mainNet,
                            Rect& r,
                            dbTechLayer* layer,
                            uint sboxId);
  void setCurrentNet(dbNet* net);
  dbInst* createInst(dbInst* inst0);
  std::vector<dbTechLayer*> getRoutingLayer() { return _routingLayers; };

 private:
  uint getFirstShape(dbNet* net, dbShape& s);
  bool setFirstShapeProperty(dbNet* net, uint prop);
  dbTechLayerRule* getRule(int routingLayer, int width);
  dbTechVia* getVia(int l1, int l2, Rect& bbox);
  std::pair<dbBTerm*, dbBTerm*> createTerms4SingleNet(dbNet* net,
                                                      int x1,
                                                      int y1,
                                                      int x2,
                                                      int y2,
                                                      dbTechLayer* inly);

  using RuleMap = std::map<int, dbTechLayerRule*>;
  dbTech* _tech;
  dbBlock* _block;
  std::vector<RuleMap> _rules;
  std::vector<dbTechLayer*> _routingLayers;
  int _ruleNameHint;
  dbMatrix<std::vector<dbTechVia*>> _vias;
  bool _milosFormat;
  dbNet* _currentNet;
  dbNet** _mapArray;
  uint _mapCnt;
  uint _ecoCnt;
  utl::Logger* logger_;
  bool _skipPowerNets;
  bool _useLocation;
  bool _verbose;
};

}  // namespace rcx
