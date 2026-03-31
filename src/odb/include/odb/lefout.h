// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include <cstdint>
#include <map>
#include <ostream>
#include <set>
#include <string>
#include <unordered_map>

#include "boost/polygon/polygon.hpp"
#include "odb/db.h"
#include "odb/dbObject.h"
#include "odb/dbSet.h"
#include "odb/dbTypes.h"
#include "odb/geom.h"
#include "utl/Logger.h"

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
 public:
  double lefdist(int value) { return value * dist_factor_; }
  double lefarea(int value) { return value * area_factor_; }

  lefout(utl::Logger* logger, std::ostream& out) : _out(out)
  {
    write_marked_masters_ = use_alias_ = use_master_ids_ = false;
    dist_factor_ = 0.001;
    area_factor_ = 0.000001;
    logger_ = logger;
    bloat_factor_ = 10;
    bloat_occupied_layers_ = false;
  }

  void setWriteMarkedMasters(bool value) { write_marked_masters_ = value; }
  void setUseLayerAlias(bool value) { use_alias_ = value; }
  void setUseMasterIds(bool value) { use_master_ids_ = value; }
  void setBloatFactor(int value) { bloat_factor_ = value; }
  void setBloatOccupiedLayers(bool value) { bloat_occupied_layers_ = value; }

  void writeTech(dbTech* tech);
  void writeLib(dbLib* lib);
  void writeTechAndLib(dbLib* lib);
  void writeAbstractLef(dbBlock* db_block);

  std::ostream& out() { return _out; }

 private:
  using ObstructionMap
      = std::map<dbTechLayer*, boost::polygon::polygon_90_set_data<int>>;

  template <typename GenericBox>
  std::set<dbVia*> writeBoxes(std::ostream& out,
                              dbBlock* block,
                              dbSet<GenericBox>& boxes,
                              const char* indent);

  void writeTechBody(std::ostream& out, dbTech* tech);
  void writeLayer(std::ostream& out, dbTechLayer* layer);
  void writeVia(std::ostream& out, dbTechVia* via);
  void writeBlockVia(std::ostream& out, dbBlock* db_block, dbVia* via);
  void writeHeader(std::ostream& out, dbLib* lib);
  void writeHeader(std::ostream& out, dbBlock* db_block);
  void writeLibBody(std::ostream& out, dbLib* lib);
  void writeMaster(std::ostream& out, dbMaster* master);
  void writeMTerm(std::ostream& out, dbMTerm* mterm);
  void writeSite(std::ostream& out, dbSite* site);
  void writeViaMap(std::ostream& out, dbTech* tech, bool use_via_cut_class);
  void writeNonDefaultRule(std::ostream& out,
                           dbTech* tech,
                           dbTechNonDefaultRule* rule);
  void writeLayerRule(std::ostream& out, dbTechLayerRule* rule);
  void writeSameNetRule(std::ostream& out, dbTechSameNetRule* rule);
  void writeTechViaRule(std::ostream& out, dbTechViaRule* rule);
  void writeTechViaGenerateRule(std::ostream& out, dbTechViaGenerateRule* rule);
  void writePropertyDefinition(std::ostream& out, dbProperty* prop);
  void writePropertyDefinitions(std::ostream& out, dbLib* lib);
  void writeVersion(std::ostream& out, const std::string& version);
  void writeNameCaseSensitive(std::ostream& out, dbOnOffType on_off_type);
  void writeBusBitChars(std::ostream& out,
                        char left_bus_delimiter,
                        char right_bus_delimiter);
  void writeUnits(std::ostream& out, int database_units);
  void writeDividerChar(std::ostream& out, char hier_delimiter);
  void writeObstructions(std::ostream& out, dbBlock* db_block);
  void getObstructions(dbBlock* db_block, ObstructionMap& obstructions) const;
  void writeBox(std::ostream& out, const std::string& indent, dbBox* box);
  void writePolygon(std::ostream& out,
                    const std::string& indent,
                    dbPolygon* polygon);
  void writeRect(std::ostream& out,
                 const std::string& indent,
                 const Rect& rect);
  void findInstsObstructions(ObstructionMap& obstructions,
                             dbBlock* db_block) const;
  void findWireLayerObstructions(ObstructionMap& obstructions,
                                 dbNet* net) const;
  void findSWireLayerObstructions(ObstructionMap& obstructions,
                                  dbNet* net) const;
  void findLayerViaObstructions(ObstructionMap& obstructions,
                                dbSBox* box) const;
  void writeBlock(std::ostream& out, dbBlock* db_block);
  std::set<dbVia*> writePins(std::ostream& out, dbBlock* db_block);
  std::set<dbVia*> writePowerPins(std::ostream& out, dbBlock* db_block);
  std::set<dbVia*> writeBlockTerms(std::ostream& out, dbBlock* db_block);

  inline void writeObjectPropertyDefinitions(
      std::ostream& out,
      dbObject* obj,
      std::unordered_map<std::string, int16_t>& propertiesMap);

  int determineBloat(dbTechLayer* layer) const;
  void insertObstruction(dbTechLayer* layer,
                         const Rect& rect,
                         ObstructionMap& obstructions) const;
  void insertObstruction(dbBox* box, ObstructionMap& obstructions) const;

  std::ostream& _out;
  bool use_master_ids_;
  bool use_alias_;
  bool write_marked_masters_;
  double dist_factor_;
  double area_factor_;
  utl::Logger* logger_;
  int bloat_factor_;
  bool bloat_occupied_layers_;
};
}  // namespace odb
