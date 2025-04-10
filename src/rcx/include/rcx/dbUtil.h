// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include <cstdint>
#include <map>
#include <utility>
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
