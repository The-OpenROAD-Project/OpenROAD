// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include <cstdint>
#include <map>
#include <utility>
#include <vector>

#include "odb/db.h"
#include "odb/dbMatrix.h"
#include "odb/dbObject.h"
#include "odb/dbTypes.h"
#include "odb/geom.h"

namespace utl {
class Logger;
}

namespace rcx {

//
// This class creates a new net along with a wire.
//
class dbCreateNetUtil
{
 public:
  dbCreateNetUtil(utl::Logger* logger);
  ~dbCreateNetUtil();

  void setBlock(odb::dbBlock* block, bool skipInit = false);
  odb::dbBlock* getBlock() const { return _block; }
  odb::dbNet* createNetSingleWire(const char* netName,
                                  int x1,
                                  int y1,
                                  int x2,
                                  int y2,
                                  int routingLayer,
                                  bool skipBterms = false,
                                  bool skipExistsNet = false,
                                  uint8_t color = 0);
  odb::dbNet* createNetSingleWire(const char* name,
                                  int x1,
                                  int y1,
                                  int x2,
                                  int y2,
                                  int routingLayer,
                                  odb::dbTechLayerDir dir,
                                  bool skipBterms = false);

  odb::dbNet* createNetSingleWire(odb::Rect& r,
                                  uint32_t level,
                                  uint32_t netId,
                                  uint32_t shapeId);
  odb::dbSBox* createSpecialWire(odb::dbNet* mainNet,
                                 odb::Rect& r,
                                 odb::dbTechLayer* layer,
                                 uint32_t sboxId);
  void setCurrentNet(odb::dbNet* net);
  odb::dbInst* createInst(odb::dbInst* inst0);
  std::vector<odb::dbTechLayer*> getRoutingLayer() { return _routingLayers; };

 private:
  uint32_t getFirstShape(odb::dbNet* net, odb::dbShape& s);
  bool setFirstShapeProperty(odb::dbNet* net, uint32_t prop);
  odb::dbTechLayerRule* getRule(int routingLayer, int width);
  odb::dbTechVia* getVia(int l1, int l2, odb::Rect& bbox);
  std::pair<odb::dbBTerm*, odb::dbBTerm*> createTerms4SingleNet(
      odb::dbNet* net,
      int x1,
      int y1,
      int x2,
      int y2,
      odb::dbTechLayer* inly);

  using RuleMap = std::map<int, odb::dbTechLayerRule*>;
  odb::dbTech* _tech;
  odb::dbBlock* _block;
  std::vector<RuleMap> _rules;
  std::vector<odb::dbTechLayer*> _routingLayers;
  int _ruleNameHint;
  odb::dbMatrix<std::vector<odb::dbTechVia*>> _vias;
  bool _milosFormat;
  odb::dbNet* _currentNet;
  odb::dbNet** _mapArray;
  uint32_t _mapCnt;
  uint32_t _ecoCnt;
  utl::Logger* logger_;
  bool _skipPowerNets;
  bool _useLocation;
  bool _verbose;
};

}  // namespace rcx
