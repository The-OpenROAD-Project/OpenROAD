// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include <list>
#include <map>
#include <string>

#include "odb/db.h"
#include "odb/dbMap.h"
#include "odb/defout.h"
#include "odb/odb.h"
namespace utl {
class Logger;
}

namespace odb {

class dbBlock;
class dbBTerm;
class dbInst;
class dbTechNonDefaultRule;
class dbTechLayerRule;

class defout_impl
{
  enum ObjType
  {
    COMPONENT,
    COMPONENTPIN,
    DESIGN,
    GROUP,
    NET,
    NONDEFAULTRULE,
    REGION,
    ROW,
    SPECIALNET
  };

  double _dist_factor;
  FILE* _out;
  bool _use_net_inst_ids;
  bool _use_master_ids;
  bool _use_alias;
  std::list<dbNet*> _select_net_list;
  std::list<dbInst*> _select_inst_list;
  dbMap<dbNet, char>* _select_net_map;
  dbMap<dbInst, char>* _select_inst_map;
  dbTechNonDefaultRule* _non_default_rule;
  int _version;
  std::map<std::string, bool> _prop_defs[9];
  utl::Logger* _logger;

  int defdist(int value) { return (int) (((double) value) * _dist_factor); }

  int defdist(uint value) { return (uint) (((double) value) * _dist_factor); }

  void writePropertyDefinitions(dbBlock* block);
  void writeRows(dbBlock* block);
  void writeTracks(dbBlock* block);
  void writeGCells(dbBlock* block);
  void writeVias(dbBlock* block);
  void writeVia(dbVia* via);
  void writeComponentMaskShift(dbBlock* block);
  void writeInsts(dbBlock* block);
  void writeNonDefaultRules(dbBlock* block);
  void writeNonDefaultRule(dbTechNonDefaultRule* rule);
  void writeLayerRule(dbTechLayerRule* rule);
  void writeInst(dbInst* inst);
  void writeBTerms(dbBlock* block);
  void writeBTerm(dbBTerm* bterm);
  void writeBPin(dbBPin* bpin, int n);
  void writeRegions(dbBlock* block);
  void writeGroups(dbBlock* block);
  void writeScanChains(dbBlock* block);
  void writeBlockages(dbBlock* block);
  void writeFills(dbBlock* block);
  void writeNets(dbBlock* block);
  void writeNet(dbNet* net);
  void writeSNet(dbNet* net);
  void writeWire(dbWire* wire);
  void writeSWire(dbSWire* wire);
  void writeSpecialPath(dbSBox* box);
  void writePropValue(dbProperty* prop);
  void writeProperties(dbObject* object);
  void writePinProperties(dbBlock* block);
  bool hasProperties(dbObject* object, ObjType type);

 public:
  defout_impl(utl::Logger* logger)
  {
    _dist_factor = 0;
    _out = nullptr;
    _use_net_inst_ids = false;
    _use_master_ids = false;
    _use_alias = false;
    _select_net_map = nullptr;
    _select_inst_map = nullptr;
    _non_default_rule = nullptr;
    _version = defout::DEF_5_8;
    _logger = logger;
  }

  ~defout_impl() = default;

  void setUseLayerAlias(bool value) { _use_alias = value; }

  void setUseNetInstIds(bool value) { _use_net_inst_ids = value; }

  void setUseMasterIds(bool value) { _use_master_ids = value; }

  void selectNet(dbNet* net);

  void selectInst(dbInst* inst);
  void setVersion(int v) { _version = v; }

  bool writeBlock(dbBlock* block, const char* def_file);
};

}  // namespace odb
