// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include <cstdint>
#include <list>
#include <map>
#include <ostream>
#include <set>
#include <string>
#include <unordered_map>

#include "odb/db.h"
#include "odb/dbMap.h"
#include "odb/dbObject.h"
#include "odb/defout.h"

namespace utl {
class Logger;
}

namespace odb {

class dbBlock;
class dbBTerm;
class dbInst;
class dbTechNonDefaultRule;
class dbTechLayerRule;

class DefOut::Impl
{
 public:
  Impl(utl::Logger* logger) : _logger(logger) {}

  ~Impl() = default;

  void selectNet(dbNet* net);
  void selectInst(dbInst* inst);

  void setVersion(DefOut::Version v) { _version = v; }

  bool writeBlock(dbBlock* block, const char* def_file);
  bool writeBlock(dbBlock* block, std::ostream& stream);

 private:
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

  int defdist(int value) { return (int) (((double) value) * _dist_factor); }

  int defdist(uint32_t value)
  {
    return (uint32_t) (((double) value) * _dist_factor);
  }

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
  void writeSNet(
      dbNet* net,
      const std::unordered_map<std::string, std::set<dbNet*>>& snet_term_map);
  void writeWire(dbWire* wire);
  void writeSWire(dbSWire* wire);
  void writeSpecialPath(dbSBox* box);
  void writePropValue(dbProperty* prop);
  void writeProperties(dbObject* object);
  void writePinProperties(dbBlock* block);
  bool hasProperties(dbObject* object, ObjType type);

  double _dist_factor{0};
  std::ostream* _out{nullptr};
  std::list<dbNet*> _select_net_list;
  std::list<dbInst*> _select_inst_list;
  dbMap<dbNet, char>* _select_net_map{nullptr};
  dbMap<dbInst, char>* _select_inst_map{nullptr};
  dbTechNonDefaultRule* _non_default_rule{nullptr};
  DefOut::Version _version{DefOut::DEF_5_8};
  std::map<std::string, bool> _prop_defs[9];
  utl::Logger* _logger;
};

}  // namespace odb
