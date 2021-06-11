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

#include <string>
#include <unordered_map>

#include "cdl.h"
#include "dbObject.h"
#include "odb.h"

namespace odb {

class dbTech;
class dbTechLayer;
class dbTechVia;
class dbLib;
class dbMaster;
class dbMTerm;
class dbDatabase;
class dbSite;
class dbTechSameNetRule;
class dbTechNonDefaultRule;
class dbTechLayerRule;
class dbTechViaRule;
class dbTechViaGenerateRule;
class dbProperty;

class lefout
{
  FILE* _out;
  bool _use_master_ids;
  bool _use_alias;
  bool _write_marked_masters;
  double _dist_factor;
  double _area_factor;

  void writeBoxes(void* boxes, const char* indent);
  void writeTech(dbTech* tech);
  void writeLayer(dbTechLayer* layer);
  void writeVia(dbTechVia* via);
  void writeHeader(dbLib* lib);
  void writeHeader(dbBlock* db_block);
  void writeLib(dbLib* lib);
  void writeMaster(dbMaster* master);
  void writeMTerm(dbMTerm* mterm);
  void writeSite(dbSite* site);
  void writeNonDefaultRule(dbTech* tech, dbTechNonDefaultRule* rule);
  void writeLayerRule(dbTechLayerRule* rule);
  void writeSameNetRule(dbTechSameNetRule* rule);
  void writeTechViaRule(dbTechViaRule* rule);
  void writeTechViaGenerateRule(dbTechViaGenerateRule* rule);
  void writePropertyDefinition(dbProperty* prop);
  void writePropertyDefinitions(dbLib* lib);
  void writeVersion(const char* version);
  void writeNameCaseSensitive(const dbOnOffType on_off_type);
  void writeBusBitChars(char left_bus_delimeter, char right_bus_delimeter);
  void writeUnits(int database_units);
  void writeDividerChar(char hier_delimeter);


  inline void writeObjectPropertyDefinitions(
      dbObject* obj,
      std::unordered_map<std::string, short>& propertiesMap);

 public:
  double lefdist(int value) { return ((double) value * _dist_factor); }

  double lefarea(int value) { return ((double) value * _area_factor); }

  lefout()
  {
    _out = nullptr;
    _write_marked_masters = _use_alias = _use_master_ids = false;
    _dist_factor = 0.001;
    _area_factor = 0.000001;
  }

  ~lefout() {}

  void setWriteMarkedMasters(bool value) { _write_marked_masters = value; }
  void setUseLayerAlias(bool value) { _use_alias = value; }
  void setUseMasterIds(bool value) { _use_master_ids = value; }

  bool writeTech(dbTech* tech, const char* lef_file);
  bool writeLib(dbLib* lib, const char* lef_file);
  bool writeTechAndLib(dbLib* lib, const char* lef_file);

  FILE* out() { return _out; }
  bool writeAbstractLef(odb::dbBlock* db_block, const char* lef_file);
};

}  // namespace odb
