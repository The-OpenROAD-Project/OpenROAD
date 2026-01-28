// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "dbTech.h"

#include <cmath>
#include <cstdint>
#include <cstring>
#include <vector>

#include "dbBox.h"
#include "dbBoxItr.h"
#include "dbCellEdgeSpacing.h"
#include "dbCore.h"
#include "dbDatabase.h"
#include "dbMetalWidthViaMap.h"
#include "dbNameCache.h"
#include "dbProperty.h"
#include "dbPropertyItr.h"
#include "dbTable.h"
#include "dbTechLayer.h"
#include "dbTechLayerAntennaRule.h"
#include "dbTechLayerItr.h"
#include "dbTechLayerRule.h"
#include "dbTechNonDefaultRule.h"
#include "dbTechSameNetRule.h"
#include "dbTechVia.h"
#include "dbTechViaGenerateRule.h"
#include "dbTechViaLayerRule.h"
#include "dbTechViaRule.h"
#include "odb/db.h"
#include "odb/dbObject.h"
#include "odb/dbSet.h"
#include "odb/dbStream.h"
#include "odb/dbTypes.h"
#include "utl/Logger.h"

namespace odb {

template class dbTable<_dbTech>;

bool _dbTech::operator==(const _dbTech& rhs) const
{
  if (flags_.namecase != rhs.flags_.namecase) {
    return false;
  }

  if (flags_.has_wire_ext != rhs.flags_.has_wire_ext) {
    return false;
  }

  if (flags_.no_wire_ext != rhs.flags_.no_wire_ext) {
    return false;
  }

  if (flags_.has_clearance_measure != rhs.flags_.has_clearance_measure) {
    return false;
  }

  if (flags_.clearance_measure != rhs.flags_.clearance_measure) {
    return false;
  }

  if (flags_.has_use_min_spacing_obs != rhs.flags_.has_use_min_spacing_obs) {
    return false;
  }

  if (flags_.use_min_spacing_obs != rhs.flags_.use_min_spacing_obs) {
    return false;
  }

  if (flags_.has_use_min_spacing_pin != rhs.flags_.has_use_min_spacing_pin) {
    return false;
  }

  if (flags_.use_min_spacing_pin != rhs.flags_.use_min_spacing_pin) {
    return false;
  }

  if (version_ != rhs.version_) {
    return false;
  }

  if (name_ != rhs.name_) {
    return false;
  }

  if (via_cnt_ != rhs.via_cnt_) {
    return false;
  }

  if (layer_cnt_ != rhs.layer_cnt_) {
    return false;
  }

  if (rlayer_cnt_ != rhs.rlayer_cnt_) {
    return false;
  }

  if (lef_units_ != rhs.lef_units_) {
    return false;
  }

  if (manufacturing_grid_ != rhs.manufacturing_grid_) {
    return false;
  }

  if (bottom_ != rhs.bottom_) {
    return false;
  }

  if (top_ != rhs.top_) {
    return false;
  }

  if (non_default_rules_ != rhs.non_default_rules_) {
    return false;
  }

  if (samenet_rules_ != rhs.samenet_rules_) {
    return false;
  }

  if (samenet_matrix_ != rhs.samenet_matrix_) {
    return false;
  }

  if (*layer_tbl_ != *rhs.layer_tbl_) {
    return false;
  }

  if (*via_tbl_ != *rhs.via_tbl_) {
    return false;
  }

  if (*non_default_rule_tbl_ != *rhs.non_default_rule_tbl_) {
    return false;
  }

  if (*layer_rule_tbl_ != *rhs.layer_rule_tbl_) {
    return false;
  }

  if (*box_tbl_ != *rhs.box_tbl_) {
    return false;
  }

  if (*samenet_rule_tbl_ != *rhs.samenet_rule_tbl_) {
    return false;
  }

  if (*antenna_rule_tbl_ != *rhs.antenna_rule_tbl_) {
    return false;
  }

  if (*via_rule_tbl_ != *rhs.via_rule_tbl_) {
    return false;
  }

  if (*via_layer_rule_tbl_ != *rhs.via_layer_rule_tbl_) {
    return false;
  }

  if (*via_generate_rule_tbl_ != *rhs.via_generate_rule_tbl_) {
    return false;
  }

  if (*prop_tbl_ != *rhs.prop_tbl_) {
    return false;
  }

  if (*metal_width_via_map_tbl_ != *rhs.metal_width_via_map_tbl_) {
    return false;
  }

  if (*cell_edge_spacing_tbl_ != *rhs.cell_edge_spacing_tbl_) {
    return false;
  }

  if (*name_cache_ != *rhs.name_cache_) {
    return false;
  }

  if (via_hash_ != rhs.via_hash_) {
    return false;
  }

  return true;
}

////////////////////////////////////////////////////////////////////
//
// _dbTech - Methods
//
////////////////////////////////////////////////////////////////////
_dbTech::_dbTech(_dbDatabase* db)
{
  via_cnt_ = 0;
  layer_cnt_ = 0;
  rlayer_cnt_ = 0;
  lef_units_ = 0;
  manufacturing_grid_ = 0;

  flags_.namecase = dbOnOffType::ON;
  flags_.has_wire_ext = dbOnOffType::OFF;
  flags_.no_wire_ext = dbOnOffType::OFF;
  flags_.has_clearance_measure = dbOnOffType::OFF;
  flags_.clearance_measure = dbClMeasureType::EUCLIDEAN;
  flags_.has_use_min_spacing_obs = dbOnOffType::OFF;
  flags_.use_min_spacing_obs = dbOnOffType::ON;
  flags_.has_use_min_spacing_pin = dbOnOffType::OFF;
  flags_.use_min_spacing_pin = dbOnOffType::OFF;
  flags_.spare_bits = 0;
  version_ = 5.4;

  layer_tbl_ = new dbTable<_dbTechLayer>(
      db, this, (GetObjTbl_t) &_dbTech::getObjectTable, dbTechLayerObj);

  via_tbl_ = new dbTable<_dbTechVia>(
      db, this, (GetObjTbl_t) &_dbTech::getObjectTable, dbTechViaObj);

  non_default_rule_tbl_ = new dbTable<_dbTechNonDefaultRule, 4>(
      db,
      this,
      (GetObjTbl_t) &_dbTech::getObjectTable,
      dbTechNonDefaultRuleObj);

  layer_rule_tbl_ = new dbTable<_dbTechLayerRule, 4>(
      db, this, (GetObjTbl_t) &_dbTech::getObjectTable, dbTechLayerRuleObj);

  box_tbl_ = new dbTable<_dbBox>(
      db, this, (GetObjTbl_t) &_dbTech::getObjectTable, dbBoxObj);

  samenet_rule_tbl_ = new dbTable<_dbTechSameNetRule, 16>(
      db, this, (GetObjTbl_t) &_dbTech::getObjectTable, dbTechSameNetRuleObj);

  antenna_rule_tbl_ = new dbTable<_dbTechLayerAntennaRule, 16>(
      db,
      this,
      (GetObjTbl_t) &_dbTech::getObjectTable,
      dbTechLayerAntennaRuleObj);

  via_rule_tbl_ = new dbTable<_dbTechViaRule, 16>(
      db, this, (GetObjTbl_t) &_dbTech::getObjectTable, dbTechViaRuleObj);

  via_layer_rule_tbl_ = new dbTable<_dbTechViaLayerRule, 16>(
      db, this, (GetObjTbl_t) &_dbTech::getObjectTable, dbTechViaLayerRuleObj);

  via_generate_rule_tbl_ = new dbTable<_dbTechViaGenerateRule, 16>(
      db,
      this,
      (GetObjTbl_t) &_dbTech::getObjectTable,
      dbTechViaGenerateRuleObj);

  prop_tbl_ = new dbTable<_dbProperty>(
      db, this, (GetObjTbl_t) &_dbTech::getObjectTable, dbPropertyObj);

  metal_width_via_map_tbl_ = new dbTable<_dbMetalWidthViaMap>(
      db, this, (GetObjTbl_t) &_dbTech::getObjectTable, dbMetalWidthViaMapObj);

  cell_edge_spacing_tbl_ = new dbTable<_dbCellEdgeSpacing>(
      db, this, (GetObjTbl_t) &_dbTech::getObjectTable, dbCellEdgeSpacingObj);

  via_hash_.setTable(via_tbl_);

  name_cache_
      = new _dbNameCache(db, this, (GetObjTbl_t) &_dbTech::getObjectTable);

  layer_itr_ = new dbTechLayerItr(layer_tbl_);

  box_itr_ = new dbBoxItr(box_tbl_, (dbTable<_dbPolygon>*) nullptr, false);

  prop_itr_ = new dbPropertyItr(prop_tbl_);
}

_dbTech::~_dbTech()
{
  delete layer_tbl_;
  delete via_tbl_;
  delete non_default_rule_tbl_;
  delete layer_rule_tbl_;
  delete box_tbl_;
  delete samenet_rule_tbl_;
  delete antenna_rule_tbl_;
  delete via_rule_tbl_;
  delete via_layer_rule_tbl_;
  delete via_generate_rule_tbl_;
  delete prop_tbl_;
  delete metal_width_via_map_tbl_;
  delete cell_edge_spacing_tbl_;
  delete name_cache_;
  delete layer_itr_;
  delete box_itr_;
  delete prop_itr_;
}

dbOStream& operator<<(dbOStream& stream, const _dbTech& tech)
{
  dbOStreamScope scope(stream, "dbTech");
  stream << tech.name_;
  stream << tech.via_cnt_;
  stream << tech.layer_cnt_;
  stream << tech.rlayer_cnt_;
  stream << tech.lef_units_;
  stream << tech.manufacturing_grid_;

  uint32_t* bit_field = (uint32_t*) &tech.flags_;
  stream << *bit_field;

  stream << tech.getLefVersion();
  stream << tech.bottom_;
  stream << tech.top_;
  stream << tech.non_default_rules_;
  stream << tech.samenet_rules_;
  stream << tech.samenet_matrix_;
  stream << NamedTable("layer_tbl", tech.layer_tbl_);
  stream << NamedTable("via_tbl", tech.via_tbl_);
  stream << NamedTable("non_default_rule_tbl", tech.non_default_rule_tbl_);
  stream << NamedTable("layer_rule_tbl", tech.layer_rule_tbl_);
  stream << NamedTable("box_tbl", tech.box_tbl_);
  stream << NamedTable("samenet_rule_tbl", tech.samenet_rule_tbl_);
  stream << NamedTable("antenna_rule_tbl", tech.antenna_rule_tbl_);
  stream << NamedTable("via_rule_tbl", tech.via_rule_tbl_);
  stream << NamedTable("via_layer_rule_tbl", tech.via_layer_rule_tbl_);
  stream << NamedTable("via_generate_rule_tbl", tech.via_generate_rule_tbl_);
  stream << NamedTable("prop_tbl", tech.prop_tbl_);
  stream << NamedTable("metal_width_via_map_tbl",
                       tech.metal_width_via_map_tbl_);
  stream << NamedTable("cell_edge_spacing_tbl_", tech.cell_edge_spacing_tbl_);
  stream << *tech.name_cache_;
  stream << tech.via_hash_;
  return stream;
}

dbIStream& operator>>(dbIStream& stream, _dbTech& tech)
{
  _dbDatabase* db = tech.getImpl()->getDatabase();
  if (db->isSchema(kSchemaBlockTech)) {
    stream >> tech.name_;
  } else {
    tech.name_ = "";
  }
  stream >> tech.via_cnt_;
  stream >> tech.layer_cnt_;
  stream >> tech.rlayer_cnt_;
  stream >> tech.lef_units_;
  if (db->isLessThanSchema(kSchemaRemoveDbuPerMicron)) {
    int dbu_per_micron;
    stream >> dbu_per_micron;
    db->dbu_per_micron_ = dbu_per_micron;
  }
  stream >> tech.manufacturing_grid_;

  uint32_t* bit_field = (uint32_t*) &tech.flags_;
  stream >> *bit_field;

  double lef_version;
  stream >> lef_version;
  tech.setLefVersion(lef_version);

  stream >> tech.bottom_;
  stream >> tech.top_;
  stream >> tech.non_default_rules_;
  stream >> tech.samenet_rules_;
  stream >> tech.samenet_matrix_;
  stream >> *tech.layer_tbl_;
  stream >> *tech.via_tbl_;
  stream >> *tech.non_default_rule_tbl_;
  stream >> *tech.layer_rule_tbl_;
  stream >> *tech.box_tbl_;
  stream >> *tech.samenet_rule_tbl_;
  stream >> *tech.antenna_rule_tbl_;
  stream >> *tech.via_rule_tbl_;
  stream >> *tech.via_layer_rule_tbl_;
  stream >> *tech.via_generate_rule_tbl_;
  stream >> *tech.prop_tbl_;
  stream >> *tech.metal_width_via_map_tbl_;
  if (tech.getDatabase()->isSchema(kSchemaCellEdgeSpcTbl)) {
    stream >> *tech.cell_edge_spacing_tbl_;
  }
  stream >> *tech.name_cache_;
  stream >> tech.via_hash_;

  return stream;
}

std::string dbTech::getName()
{
  auto tech = (_dbTech*) this;
  return tech->name_;
}

double _dbTech::getLefVersion() const
{
  return version_;
}

std::string _dbTech::getLefVersionStr() const
{
  int major_version = (int) floor(version_);
  int minor_version = ((int) floor(version_ * 10.0)) - (major_version * 10);
  int opt_minor_version = ((int) floor(version_ * 1000.0))
                          - (major_version * 1000) - (minor_version * 100);

  if (opt_minor_version > 0) {
    return fmt::format(
        "{}.{}.{}", major_version, minor_version, opt_minor_version);
  }
  return fmt::format("{}.{}", major_version, minor_version);
}

//
// Version stored as double in %d.%1d%2d format;
// the last two digits are optional
//
void _dbTech::setLefVersion(double inver)
{
  version_ = inver;
}

dbObjectTable* _dbTech::getObjectTable(dbObjectType type)
{
  switch (type) {
    case dbTechLayerObj:
      return layer_tbl_;
    case dbTechViaObj:
      return via_tbl_;
    case dbTechNonDefaultRuleObj:
      return non_default_rule_tbl_;
    case dbTechLayerRuleObj:
      return layer_rule_tbl_;
    case dbBoxObj:
      return box_tbl_;
    case dbTechSameNetRuleObj:
      return samenet_rule_tbl_;
    case dbTechLayerAntennaRuleObj:
      return antenna_rule_tbl_;
    case dbTechViaRuleObj:
      return via_rule_tbl_;
    case dbTechViaLayerRuleObj:
      return via_layer_rule_tbl_;
    case dbTechViaGenerateRuleObj:
      return via_generate_rule_tbl_;
    case dbPropertyObj:
      return prop_tbl_;
    case dbMetalWidthViaMapObj:
      return metal_width_via_map_tbl_;
    case dbCellEdgeSpacingObj:
      return cell_edge_spacing_tbl_;
    default:
      break;  // WAll
  }

  return getTable()->getObjectTable(type);
}

////////////////////////////////////////////////////////////////////
//
// dbTech - Methods
//
////////////////////////////////////////////////////////////////////

dbSet<dbTechLayer> dbTech::getLayers()
{
  _dbTech* tech = (_dbTech*) this;
  return dbSet<dbTechLayer>(tech, tech->layer_itr_);
}

dbTechLayer* dbTech::findLayer(const char* name)
{
  for (dbTechLayer* layer : getLayers()) {
    if (strcmp(layer->getConstName(), name) == 0) {
      return layer;
    }
  }

  return nullptr;
}

dbTechLayer* dbTech::findLayer(int layer_number)
{
  for (dbTechLayer* layer : getLayers()) {
    if (layer->getNumber() == layer_number) {
      return layer;
    }
  }

  return nullptr;
}

dbTechLayer* dbTech::findRoutingLayer(int level_number)
{
  for (dbTechLayer* layer : getLayers()) {
    if (layer->getRoutingLevel() == level_number) {
      return layer;
    }
  }

  return nullptr;
}

int dbTech::getDbUnitsPerMicron()
{
  return getDb()->getDbuPerMicron();
}

dbSet<dbTechVia> dbTech::getVias()
{
  _dbTech* tech = (_dbTech*) this;
  return dbSet<dbTechVia>(tech, tech->via_tbl_);
}

dbTechVia* dbTech::findVia(const char* name)
{
  _dbTech* tech = (_dbTech*) this;
  return (dbTechVia*) tech->via_hash_.find(name);
}

int dbTech::getLefUnits()
{
  _dbTech* tech = (_dbTech*) this;
  return tech->lef_units_;
}

void dbTech::setLefUnits(int units)
{
  _dbTech* tech = (_dbTech*) this;
  tech->lef_units_ = units;
}

double dbTech::getLefVersion() const
{
  _dbTech* tech = (_dbTech*) this;
  return tech->getLefVersion();
}

std::string dbTech::getLefVersionStr() const
{
  _dbTech* tech = (_dbTech*) this;
  return tech->getLefVersionStr();
}

void dbTech::setLefVersion(double inver)
{
  _dbTech* tech = (_dbTech*) this;
  tech->setLefVersion(inver);
}

bool dbTech::hasNoWireExtAtPin() const
{
  _dbTech* tech = (_dbTech*) this;
  return (tech->flags_.has_wire_ext == dbOnOffType::ON);
}

dbOnOffType dbTech::getNoWireExtAtPin() const
{
  _dbTech* tech = (_dbTech*) this;
  return (hasNoWireExtAtPin() ? dbOnOffType(tech->flags_.no_wire_ext)
                              : dbOnOffType(dbOnOffType::OFF));
}

void dbTech::setNoWireExtAtPin(dbOnOffType intyp)
{
  _dbTech* tech = (_dbTech*) this;
  tech->flags_.has_wire_ext = dbOnOffType::ON;
  tech->flags_.no_wire_ext = intyp.getValue();
}

dbOnOffType dbTech::getNamesCaseSensitive() const
{
  _dbTech* tech = (_dbTech*) this;

  return dbOnOffType((bool) tech->flags_.namecase);
  // return dbOnOffType(tech->flags_._namecase);
}

void dbTech::setNamesCaseSensitive(dbOnOffType intyp)
{
  _dbTech* tech = (_dbTech*) this;
  tech->flags_.namecase = intyp.getValue();
}

bool dbTech::hasClearanceMeasure() const
{
  _dbTech* tech = (_dbTech*) this;
  return (tech->flags_.has_clearance_measure == dbOnOffType::ON);
}

dbClMeasureType dbTech::getClearanceMeasure() const
{
  _dbTech* tech = (_dbTech*) this;
  return (hasClearanceMeasure()
              ? dbClMeasureType(tech->flags_.clearance_measure)
              : dbClMeasureType(dbClMeasureType::EUCLIDEAN));
}

void dbTech::setClearanceMeasure(dbClMeasureType inmeas)
{
  _dbTech* tech = (_dbTech*) this;
  tech->flags_.has_clearance_measure = dbOnOffType::ON;
  tech->flags_.clearance_measure = inmeas.getValue();
}

bool dbTech::hasUseMinSpacingObs() const
{
  _dbTech* tech = (_dbTech*) this;
  return (tech->flags_.has_use_min_spacing_obs == dbOnOffType::ON);
}

dbOnOffType dbTech::getUseMinSpacingObs() const
{
  _dbTech* tech = (_dbTech*) this;
  return (hasUseMinSpacingObs() ? dbOnOffType(tech->flags_.use_min_spacing_obs)
                                : dbOnOffType(dbOnOffType::ON));
}

void dbTech::setUseMinSpacingObs(dbOnOffType intyp)
{
  _dbTech* tech = (_dbTech*) this;
  tech->flags_.has_use_min_spacing_obs = dbOnOffType::ON;
  tech->flags_.use_min_spacing_obs = intyp.getValue();
}

bool dbTech::hasUseMinSpacingPin() const
{
  _dbTech* tech = (_dbTech*) this;
  return (tech->flags_.has_use_min_spacing_pin == dbOnOffType::ON);
}

dbOnOffType dbTech::getUseMinSpacingPin() const
{
  _dbTech* tech = (_dbTech*) this;
  return (hasUseMinSpacingPin() ? dbOnOffType(tech->flags_.use_min_spacing_pin)
                                : dbOnOffType(dbOnOffType::ON));
}

void dbTech::setUseMinSpacingPin(dbOnOffType intyp)
{
  _dbTech* tech = (_dbTech*) this;
  tech->flags_.has_use_min_spacing_pin = dbOnOffType::ON;
  tech->flags_.use_min_spacing_pin = intyp.getValue();
}

bool dbTech::hasManufacturingGrid() const
{
  _dbTech* tech = (_dbTech*) this;
  return (tech->manufacturing_grid_ > 0);
}

int dbTech::getManufacturingGrid() const
{
  _dbTech* tech = (_dbTech*) this;
  return tech->manufacturing_grid_;
}

void dbTech::setManufacturingGrid(int ingrd)
{
  _dbTech* tech = (_dbTech*) this;
  tech->manufacturing_grid_ = ingrd;
}

int dbTech::getLayerCount()
{
  _dbTech* tech = (_dbTech*) this;
  return tech->layer_cnt_;
}

int dbTech::getRoutingLayerCount()
{
  _dbTech* tech = (_dbTech*) this;
  return tech->rlayer_cnt_;
}

int dbTech::getViaCount()
{
  _dbTech* tech = (_dbTech*) this;
  return tech->via_cnt_;
}

dbSet<dbTechNonDefaultRule> dbTech::getNonDefaultRules()
{
  _dbTech* tech = (_dbTech*) this;
  dbSet<dbTechNonDefaultRule> ndr(tech, tech->non_default_rule_tbl_);
  return ndr;
}

dbTechNonDefaultRule* dbTech::findNonDefaultRule(const char* name)
{
  for (dbTechNonDefaultRule* r : getNonDefaultRules()) {
    if (strcmp(r->getConstName(), name) == 0) {
      return r;
    }
  }

  return nullptr;
}

dbTechSameNetRule* dbTech::findSameNetRule(dbTechLayer* l1_, dbTechLayer* l2_)
{
  _dbTech* tech = (_dbTech*) this;
  _dbTechLayer* l1 = (_dbTechLayer*) l1_;
  _dbTechLayer* l2 = (_dbTechLayer*) l2_;
  dbId<_dbTechSameNetRule> rule
      = tech->samenet_matrix_(l1->number_, l2->number_);

  if (rule == 0) {
    return nullptr;
  }

  return (dbTechSameNetRule*) tech->samenet_rule_tbl_->getPtr(rule);
}

void dbTech::getSameNetRules(std::vector<dbTechSameNetRule*>& rules)
{
  _dbTech* tech = (_dbTech*) this;
  rules.clear();

  for (const auto& r : tech->samenet_rules_) {
    rules.push_back((dbTechSameNetRule*) tech->samenet_rule_tbl_->getPtr(r));
  }
}

dbSet<dbTechViaRule> dbTech::getViaRules()
{
  _dbTech* tech = (_dbTech*) this;
  return dbSet<dbTechViaRule>(tech, tech->via_rule_tbl_);
}

dbSet<dbTechViaGenerateRule> dbTech::getViaGenerateRules()
{
  _dbTech* tech = (_dbTech*) this;
  return dbSet<dbTechViaGenerateRule>(tech, tech->via_generate_rule_tbl_);
}

dbSet<dbMetalWidthViaMap> dbTech::getMetalWidthViaMap()
{
  _dbTech* tech = (_dbTech*) this;
  return dbSet<dbMetalWidthViaMap>(tech, tech->metal_width_via_map_tbl_);
}

dbSet<dbCellEdgeSpacing> dbTech::getCellEdgeSpacingTable()
{
  _dbTech* tech = (_dbTech*) this;
  return dbSet<dbCellEdgeSpacing>(tech, tech->cell_edge_spacing_tbl_);
}

dbTechViaRule* dbTech::findViaRule(const char* name)
{
  for (dbTechViaRule* rule : getViaRules()) {
    if (rule->getName() == name) {
      return rule;
    }
  }

  return nullptr;
}

dbTechViaGenerateRule* dbTech::findViaGenerateRule(const char* name)
{
  for (dbTechViaGenerateRule* rule : getViaGenerateRules()) {
    if (rule->getName() == name) {
      return rule;
    }
  }

  return nullptr;
}

void dbTech::checkLayer(bool typeChk,
                        bool widthChk,
                        bool pitchChk,
                        bool spacingChk)
{
  if (!typeChk && !widthChk && !pitchChk && !spacingChk) {
    typeChk = true;
    widthChk = true;
    pitchChk = true;
    spacingChk = true;
  }

  for (dbTechLayer* layer : getLayers()) {
    dbTechLayerType type = layer->getType();
    if (type.getValue() == dbTechLayerType::CUT) {
      continue;
    }
    if (typeChk && type.getValue() != dbTechLayerType::ROUTING) {
      getImpl()->getLogger()->warn(utl::ODB,
                                   58,
                                   "Layer {} is not a routing layer!",
                                   layer->getConstName());
      continue;
    }
    const int level = layer->getRoutingLevel();
    const int pitch = layer->getPitch();
    if (pitchChk && pitch <= 0) {
      getImpl()->getLogger()->error(
          utl::ODB,
          59,
          "Layer {}, routing level {}, has {} pitch !!",
          layer->getConstName(),
          level,
          pitch);
    }
    const int width = layer->getWidth();
    if (widthChk && width == 0) {
      getImpl()->getLogger()->error(
          utl::ODB,
          60,
          "Layer {}, routing level {}, has {} width !!",
          layer->getConstName(),
          level,
          width);
    }
    const int spacing = layer->getSpacing();
    if (spacingChk && spacing <= 0) {
      getImpl()->getLogger()->error(
          utl::ODB,
          61,
          "Layer {}, routing level {}, has {} spacing !!",
          layer->getConstName(),
          level,
          spacing);
    }
  }
}
dbTech* dbTech::create(dbDatabase* db_, const char* name)
{
  _dbDatabase* db = (_dbDatabase*) db_;

  _dbTech* tech = db->tech_tbl_->create();
  tech->name_ = name;
  return (dbTech*) tech;
}

dbTech* dbTech::getTech(dbDatabase* db_, uint32_t dbid_)
{
  _dbDatabase* db = (_dbDatabase*) db_;
  return (dbTech*) db->tech_tbl_->getPtr(dbid_);
}

void dbTech::destroy(dbTech* tech_)
{
  _dbTech* tech = (_dbTech*) tech_;
  _dbDatabase* db = tech->getDatabase();
  dbProperty::destroyProperties(tech);
  db->tech_tbl_->destroy(tech);
}

void _dbTech::collectMemInfo(MemInfo& info)
{
  info.cnt++;
  info.size += sizeof(*this);

  info.children["name"].add(name_);
  info.children["samenet_rules"].add(samenet_rules_);
  info.children["samenet_matrix"].add(samenet_matrix_);
  info.children["via_hash"].add(via_hash_);

  layer_tbl_->collectMemInfo(info.children["layer"]);
  via_tbl_->collectMemInfo(info.children["via"]);
  non_default_rule_tbl_->collectMemInfo(info.children["non_default_rule"]);
  layer_rule_tbl_->collectMemInfo(info.children["layer_rule"]);
  box_tbl_->collectMemInfo(info.children["box"]);
  samenet_rule_tbl_->collectMemInfo(info.children["samenet_rule"]);
  antenna_rule_tbl_->collectMemInfo(info.children["antenna_rule"]);
  via_rule_tbl_->collectMemInfo(info.children["via_rule"]);
  via_layer_rule_tbl_->collectMemInfo(info.children["via_layer_rule"]);
  via_generate_rule_tbl_->collectMemInfo(info.children["via_generate_rule"]);
  prop_tbl_->collectMemInfo(info.children["prop"]);
  metal_width_via_map_tbl_->collectMemInfo(
      info.children["metal_width_via_map"]);
  cell_edge_spacing_tbl_->collectMemInfo(info.children["cell_edge_spacing"]);

  name_cache_->collectMemInfo(info.children["name_cache"]);
}

}  // namespace odb
