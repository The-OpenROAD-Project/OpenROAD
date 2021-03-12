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

#include <list>
#include <map>
#include <string>

#include "db.h"
#include "dbMap.h"
#include "defout.h"
#include "odb.h"
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
    _use_net_inst_ids = false;
    _use_master_ids = false;
    _use_alias = false;
    _select_net_map = NULL;
    _select_inst_map = NULL;
    _version = defout::DEF_5_8;
    _logger = logger;
  }

  ~defout_impl() {}

  void setUseLayerAlias(bool value) { _use_alias = value; }

  void setUseNetInstIds(bool value) { _use_net_inst_ids = value; }

  void setUseMasterIds(bool value) { _use_master_ids = value; }

  void selectNet(dbNet* net);

  void selectInst(dbInst* inst);
  void setVersion(int v) { _version = v; }

  bool writeBlock(dbBlock* block, const char* def_file);
};

}  // namespace odb
