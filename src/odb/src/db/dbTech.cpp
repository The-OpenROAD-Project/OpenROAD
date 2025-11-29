// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "dbTech.h"

#include <cmath>
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
#include "dbTable.hpp"
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
#include "odb/odb.h"
#include "utl/Logger.h"

namespace odb {

template class dbTable<_dbTech>;

bool _dbTech::operator==(const _dbTech& rhs) const
{
  if (flags_._namecase != rhs.flags_._namecase) {
    return false;
  }

  if (flags_._haswireext != rhs.flags_._haswireext) {
    return false;
  }

  if (flags_._nowireext != rhs.flags_._nowireext) {
    return false;
  }

  if (flags_._hasclmeas != rhs.flags_._hasclmeas) {
    return false;
  }

  if (flags_._clmeas != rhs.flags_._clmeas) {
    return false;
  }

  if (flags_._hasminspobs != rhs.flags_._hasminspobs) {
    return false;
  }

  if (flags_._minspobs != rhs.flags_._minspobs) {
    return false;
  }

  if (flags_._hasminsppin != rhs.flags_._hasminsppin) {
    return false;
  }

  if (flags_._minsppin != rhs.flags_._minsppin) {
    return false;
  }

  if (_version != rhs._version) {
    return false;
  }

  if (name_ != rhs.name_) {
    return false;
  }

  if (_via_cnt != rhs._via_cnt) {
    return false;
  }

  if (_layer_cnt != rhs._layer_cnt) {
    return false;
  }

  if (_rlayer_cnt != rhs._rlayer_cnt) {
    return false;
  }

  if (_lef_units != rhs._lef_units) {
    return false;
  }

  if (_mfgrid != rhs._mfgrid) {
    return false;
  }

  if (_bottom != rhs._bottom) {
    return false;
  }

  if (_top != rhs._top) {
    return false;
  }

  if (_non_default_rules != rhs._non_default_rules) {
    return false;
  }

  if (_samenet_rules != rhs._samenet_rules) {
    return false;
  }

  if (_samenet_matrix != rhs._samenet_matrix) {
    return false;
  }

  if (*_layer_tbl != *rhs._layer_tbl) {
    return false;
  }

  if (*_via_tbl != *rhs._via_tbl) {
    return false;
  }

  if (*_non_default_rule_tbl != *rhs._non_default_rule_tbl) {
    return false;
  }

  if (*_layer_rule_tbl != *rhs._layer_rule_tbl) {
    return false;
  }

  if (*_box_tbl != *rhs._box_tbl) {
    return false;
  }

  if (*_samenet_rule_tbl != *rhs._samenet_rule_tbl) {
    return false;
  }

  if (*_antenna_rule_tbl != *rhs._antenna_rule_tbl) {
    return false;
  }

  if (*_via_rule_tbl != *rhs._via_rule_tbl) {
    return false;
  }

  if (*_via_layer_rule_tbl != *rhs._via_layer_rule_tbl) {
    return false;
  }

  if (*_via_generate_rule_tbl != *rhs._via_generate_rule_tbl) {
    return false;
  }

  if (*_prop_tbl != *rhs._prop_tbl) {
    return false;
  }

  if (*_metal_width_via_map_tbl != *rhs._metal_width_via_map_tbl) {
    return false;
  }

  if (*cell_edge_spacing_tbl_ != *rhs.cell_edge_spacing_tbl_) {
    return false;
  }

  if (*_name_cache != *rhs._name_cache) {
    return false;
  }

  if (_via_hash != rhs._via_hash) {
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
  _via_cnt = 0;
  _layer_cnt = 0;
  _rlayer_cnt = 0;
  _lef_units = 0;
  _mfgrid = 0;

  flags_._namecase = dbOnOffType::ON;
  flags_._haswireext = dbOnOffType::OFF;
  flags_._nowireext = dbOnOffType::OFF;
  flags_._hasclmeas = dbOnOffType::OFF;
  flags_._clmeas = dbClMeasureType::EUCLIDEAN;
  flags_._hasminspobs = dbOnOffType::OFF;
  flags_._minspobs = dbOnOffType::ON;
  flags_._hasminsppin = dbOnOffType::OFF;
  flags_._minsppin = dbOnOffType::OFF;
  flags_._spare_bits = 0;
  _version = 5.4;

  _layer_tbl = new dbTable<_dbTechLayer>(
      db, this, (GetObjTbl_t) &_dbTech::getObjectTable, dbTechLayerObj);

  _via_tbl = new dbTable<_dbTechVia>(
      db, this, (GetObjTbl_t) &_dbTech::getObjectTable, dbTechViaObj);

  _non_default_rule_tbl = new dbTable<_dbTechNonDefaultRule, 4>(
      db,
      this,
      (GetObjTbl_t) &_dbTech::getObjectTable,
      dbTechNonDefaultRuleObj);

  _layer_rule_tbl = new dbTable<_dbTechLayerRule, 4>(
      db, this, (GetObjTbl_t) &_dbTech::getObjectTable, dbTechLayerRuleObj);

  _box_tbl = new dbTable<_dbBox>(
      db, this, (GetObjTbl_t) &_dbTech::getObjectTable, dbBoxObj);

  _samenet_rule_tbl = new dbTable<_dbTechSameNetRule, 16>(
      db, this, (GetObjTbl_t) &_dbTech::getObjectTable, dbTechSameNetRuleObj);

  _antenna_rule_tbl = new dbTable<_dbTechLayerAntennaRule, 16>(
      db,
      this,
      (GetObjTbl_t) &_dbTech::getObjectTable,
      dbTechLayerAntennaRuleObj);

  _via_rule_tbl = new dbTable<_dbTechViaRule, 16>(
      db, this, (GetObjTbl_t) &_dbTech::getObjectTable, dbTechViaRuleObj);

  _via_layer_rule_tbl = new dbTable<_dbTechViaLayerRule, 16>(
      db, this, (GetObjTbl_t) &_dbTech::getObjectTable, dbTechViaLayerRuleObj);

  _via_generate_rule_tbl = new dbTable<_dbTechViaGenerateRule, 16>(
      db,
      this,
      (GetObjTbl_t) &_dbTech::getObjectTable,
      dbTechViaGenerateRuleObj);

  _prop_tbl = new dbTable<_dbProperty>(
      db, this, (GetObjTbl_t) &_dbTech::getObjectTable, dbPropertyObj);

  _metal_width_via_map_tbl = new dbTable<_dbMetalWidthViaMap>(
      db, this, (GetObjTbl_t) &_dbTech::getObjectTable, dbMetalWidthViaMapObj);

  cell_edge_spacing_tbl_ = new dbTable<_dbCellEdgeSpacing>(
      db, this, (GetObjTbl_t) &_dbTech::getObjectTable, dbCellEdgeSpacingObj);

  _via_hash.setTable(_via_tbl);

  _name_cache
      = new _dbNameCache(db, this, (GetObjTbl_t) &_dbTech::getObjectTable);

  _layer_itr = new dbTechLayerItr(_layer_tbl);

  _box_itr = new dbBoxItr(_box_tbl, (dbTable<_dbPolygon>*) nullptr, false);

  _prop_itr = new dbPropertyItr(_prop_tbl);
}

_dbTech::~_dbTech()
{
  delete _layer_tbl;
  delete _via_tbl;
  delete _non_default_rule_tbl;
  delete _layer_rule_tbl;
  delete _box_tbl;
  delete _samenet_rule_tbl;
  delete _antenna_rule_tbl;
  delete _via_rule_tbl;
  delete _via_layer_rule_tbl;
  delete _via_generate_rule_tbl;
  delete _prop_tbl;
  delete _metal_width_via_map_tbl;
  delete cell_edge_spacing_tbl_;
  delete _name_cache;
  delete _layer_itr;
  delete _box_itr;
  delete _prop_itr;
}

dbOStream& operator<<(dbOStream& stream, const _dbTech& tech)
{
  dbOStreamScope scope(stream, "dbTech");
  stream << tech.name_;
  stream << tech._via_cnt;
  stream << tech._layer_cnt;
  stream << tech._rlayer_cnt;
  stream << tech._lef_units;
  stream << tech._mfgrid;

  uint* bit_field = (uint*) &tech.flags_;
  stream << *bit_field;

  stream << tech._getLefVersion();
  stream << tech._bottom;
  stream << tech._top;
  stream << tech._non_default_rules;
  stream << tech._samenet_rules;
  stream << tech._samenet_matrix;
  stream << NamedTable("layer_tbl", tech._layer_tbl);
  stream << NamedTable("via_tbl", tech._via_tbl);
  stream << NamedTable("non_default_rule_tbl", tech._non_default_rule_tbl);
  stream << NamedTable("layer_rule_tbl", tech._layer_rule_tbl);
  stream << NamedTable("box_tbl", tech._box_tbl);
  stream << NamedTable("samenet_rule_tbl", tech._samenet_rule_tbl);
  stream << NamedTable("antenna_rule_tbl", tech._antenna_rule_tbl);
  stream << NamedTable("via_rule_tbl", tech._via_rule_tbl);
  stream << NamedTable("via_layer_rule_tbl", tech._via_layer_rule_tbl);
  stream << NamedTable("via_generate_rule_tbl", tech._via_generate_rule_tbl);
  stream << NamedTable("prop_tbl", tech._prop_tbl);
  stream << NamedTable("metal_width_via_map_tbl",
                       tech._metal_width_via_map_tbl);
  stream << NamedTable("cell_edge_spacing_tbl_", tech.cell_edge_spacing_tbl_);
  stream << *tech._name_cache;
  stream << tech._via_hash;
  return stream;
}

dbIStream& operator>>(dbIStream& stream, _dbTech& tech)
{
  _dbDatabase* db = tech.getImpl()->getDatabase();
  if (db->isSchema(db_schema_block_tech)) {
    stream >> tech.name_;
  } else {
    tech.name_ = "";
  }
  stream >> tech._via_cnt;
  stream >> tech._layer_cnt;
  stream >> tech._rlayer_cnt;
  stream >> tech._lef_units;
  if (db->isLessThanSchema(db_schema_remove_dbu_per_micron)) {
    int dbu_per_micron;
    stream >> dbu_per_micron;
    db->dbu_per_micron_ = dbu_per_micron;
  }
  stream >> tech._mfgrid;

  uint* bit_field = (uint*) &tech.flags_;
  stream >> *bit_field;

  double lef_version;
  stream >> lef_version;
  tech._setLefVersion(lef_version);

  stream >> tech._bottom;
  stream >> tech._top;
  stream >> tech._non_default_rules;
  stream >> tech._samenet_rules;
  stream >> tech._samenet_matrix;
  stream >> *tech._layer_tbl;
  stream >> *tech._via_tbl;
  stream >> *tech._non_default_rule_tbl;
  stream >> *tech._layer_rule_tbl;
  stream >> *tech._box_tbl;
  stream >> *tech._samenet_rule_tbl;
  stream >> *tech._antenna_rule_tbl;
  stream >> *tech._via_rule_tbl;
  stream >> *tech._via_layer_rule_tbl;
  stream >> *tech._via_generate_rule_tbl;
  stream >> *tech._prop_tbl;
  stream >> *tech._metal_width_via_map_tbl;
  if (tech.getDatabase()->isSchema(db_schema_cell_edge_spc_tbl)) {
    stream >> *tech.cell_edge_spacing_tbl_;
  }
  stream >> *tech._name_cache;
  stream >> tech._via_hash;

  return stream;
}

std::string dbTech::getName()
{
  auto tech = (_dbTech*) this;
  return tech->name_;
}

double _dbTech::_getLefVersion() const
{
  return _version;
}

std::string _dbTech::_getLefVersionStr() const
{
  int major_version = (int) floor(_version);
  int minor_version = ((int) floor(_version * 10.0)) - (major_version * 10);
  int opt_minor_version = ((int) floor(_version * 1000.0))
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
void _dbTech::_setLefVersion(double inver)
{
  _version = inver;
}

dbObjectTable* _dbTech::getObjectTable(dbObjectType type)
{
  switch (type) {
    case dbTechLayerObj:
      return _layer_tbl;
    case dbTechViaObj:
      return _via_tbl;
    case dbTechNonDefaultRuleObj:
      return _non_default_rule_tbl;
    case dbTechLayerRuleObj:
      return _layer_rule_tbl;
    case dbBoxObj:
      return _box_tbl;
    case dbTechSameNetRuleObj:
      return _samenet_rule_tbl;
    case dbTechLayerAntennaRuleObj:
      return _antenna_rule_tbl;
    case dbTechViaRuleObj:
      return _via_rule_tbl;
    case dbTechViaLayerRuleObj:
      return _via_layer_rule_tbl;
    case dbTechViaGenerateRuleObj:
      return _via_generate_rule_tbl;
    case dbPropertyObj:
      return _prop_tbl;
    case dbMetalWidthViaMapObj:
      return _metal_width_via_map_tbl;
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
  return dbSet<dbTechLayer>(tech, tech->_layer_itr);
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
  return dbSet<dbTechVia>(tech, tech->_via_tbl);
}

dbTechVia* dbTech::findVia(const char* name)
{
  _dbTech* tech = (_dbTech*) this;
  return (dbTechVia*) tech->_via_hash.find(name);
}

int dbTech::getLefUnits()
{
  _dbTech* tech = (_dbTech*) this;
  return tech->_lef_units;
}

void dbTech::setLefUnits(int units)
{
  _dbTech* tech = (_dbTech*) this;
  tech->_lef_units = units;
}

double dbTech::getLefVersion() const
{
  _dbTech* tech = (_dbTech*) this;
  return tech->_getLefVersion();
}

std::string dbTech::getLefVersionStr() const
{
  _dbTech* tech = (_dbTech*) this;
  return tech->_getLefVersionStr();
}

void dbTech::setLefVersion(double inver)
{
  _dbTech* tech = (_dbTech*) this;
  tech->_setLefVersion(inver);
}

bool dbTech::hasNoWireExtAtPin() const
{
  _dbTech* tech = (_dbTech*) this;
  return (tech->flags_._haswireext == dbOnOffType::ON);
}

dbOnOffType dbTech::getNoWireExtAtPin() const
{
  _dbTech* tech = (_dbTech*) this;
  return (hasNoWireExtAtPin() ? dbOnOffType(tech->flags_._nowireext)
                              : dbOnOffType(dbOnOffType::OFF));
}

void dbTech::setNoWireExtAtPin(dbOnOffType intyp)
{
  _dbTech* tech = (_dbTech*) this;
  tech->flags_._haswireext = dbOnOffType::ON;
  tech->flags_._nowireext = intyp.getValue();
}

dbOnOffType dbTech::getNamesCaseSensitive() const
{
  _dbTech* tech = (_dbTech*) this;

  return dbOnOffType((bool) tech->flags_._namecase);
  // return dbOnOffType(tech->flags_._namecase);
}

void dbTech::setNamesCaseSensitive(dbOnOffType intyp)
{
  _dbTech* tech = (_dbTech*) this;
  tech->flags_._namecase = intyp.getValue();
}

bool dbTech::hasClearanceMeasure() const
{
  _dbTech* tech = (_dbTech*) this;
  return (tech->flags_._hasclmeas == dbOnOffType::ON);
}

dbClMeasureType dbTech::getClearanceMeasure() const
{
  _dbTech* tech = (_dbTech*) this;
  return (hasClearanceMeasure() ? dbClMeasureType(tech->flags_._clmeas)
                                : dbClMeasureType(dbClMeasureType::EUCLIDEAN));
}

void dbTech::setClearanceMeasure(dbClMeasureType inmeas)
{
  _dbTech* tech = (_dbTech*) this;
  tech->flags_._hasclmeas = dbOnOffType::ON;
  tech->flags_._clmeas = inmeas.getValue();
}

bool dbTech::hasUseMinSpacingObs() const
{
  _dbTech* tech = (_dbTech*) this;
  return (tech->flags_._hasminspobs == dbOnOffType::ON);
}

dbOnOffType dbTech::getUseMinSpacingObs() const
{
  _dbTech* tech = (_dbTech*) this;
  return (hasUseMinSpacingObs() ? dbOnOffType(tech->flags_._minspobs)
                                : dbOnOffType(dbOnOffType::ON));
}

void dbTech::setUseMinSpacingObs(dbOnOffType intyp)
{
  _dbTech* tech = (_dbTech*) this;
  tech->flags_._hasminspobs = dbOnOffType::ON;
  tech->flags_._minspobs = intyp.getValue();
}

bool dbTech::hasUseMinSpacingPin() const
{
  _dbTech* tech = (_dbTech*) this;
  return (tech->flags_._hasminsppin == dbOnOffType::ON);
}

dbOnOffType dbTech::getUseMinSpacingPin() const
{
  _dbTech* tech = (_dbTech*) this;
  return (hasUseMinSpacingPin() ? dbOnOffType(tech->flags_._minsppin)
                                : dbOnOffType(dbOnOffType::ON));
}

void dbTech::setUseMinSpacingPin(dbOnOffType intyp)
{
  _dbTech* tech = (_dbTech*) this;
  tech->flags_._hasminsppin = dbOnOffType::ON;
  tech->flags_._minsppin = intyp.getValue();
}

bool dbTech::hasManufacturingGrid() const
{
  _dbTech* tech = (_dbTech*) this;
  return (tech->_mfgrid > 0);
}

int dbTech::getManufacturingGrid() const
{
  _dbTech* tech = (_dbTech*) this;
  return tech->_mfgrid;
}

void dbTech::setManufacturingGrid(int ingrd)
{
  _dbTech* tech = (_dbTech*) this;
  tech->_mfgrid = ingrd;
}

int dbTech::getLayerCount()
{
  _dbTech* tech = (_dbTech*) this;
  return tech->_layer_cnt;
}

int dbTech::getRoutingLayerCount()
{
  _dbTech* tech = (_dbTech*) this;
  return tech->_rlayer_cnt;
}

int dbTech::getViaCount()
{
  _dbTech* tech = (_dbTech*) this;
  return tech->_via_cnt;
}

dbSet<dbTechNonDefaultRule> dbTech::getNonDefaultRules()
{
  _dbTech* tech = (_dbTech*) this;
  dbSet<dbTechNonDefaultRule> ndr(tech, tech->_non_default_rule_tbl);
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
      = tech->_samenet_matrix(l1->_number, l2->_number);

  if (rule == 0) {
    return nullptr;
  }

  return (dbTechSameNetRule*) tech->_samenet_rule_tbl->getPtr(rule);
}

void dbTech::getSameNetRules(std::vector<dbTechSameNetRule*>& rules)
{
  _dbTech* tech = (_dbTech*) this;
  rules.clear();

  for (const auto& r : tech->_samenet_rules) {
    rules.push_back((dbTechSameNetRule*) tech->_samenet_rule_tbl->getPtr(r));
  }
}

dbSet<dbTechViaRule> dbTech::getViaRules()
{
  _dbTech* tech = (_dbTech*) this;
  return dbSet<dbTechViaRule>(tech, tech->_via_rule_tbl);
}

dbSet<dbTechViaGenerateRule> dbTech::getViaGenerateRules()
{
  _dbTech* tech = (_dbTech*) this;
  return dbSet<dbTechViaGenerateRule>(tech, tech->_via_generate_rule_tbl);
}

dbSet<dbMetalWidthViaMap> dbTech::getMetalWidthViaMap()
{
  _dbTech* tech = (_dbTech*) this;
  return dbSet<dbMetalWidthViaMap>(tech, tech->_metal_width_via_map_tbl);
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

  _dbTech* tech = db->_tech_tbl->create();
  tech->name_ = name;
  return (dbTech*) tech;
}

dbTech* dbTech::getTech(dbDatabase* db_, uint dbid_)
{
  _dbDatabase* db = (_dbDatabase*) db_;
  return (dbTech*) db->_tech_tbl->getPtr(dbid_);
}

void dbTech::destroy(dbTech* tech_)
{
  _dbTech* tech = (_dbTech*) tech_;
  _dbDatabase* db = tech->getDatabase();
  dbProperty::destroyProperties(tech);
  db->_tech_tbl->destroy(tech);
}

void _dbTech::collectMemInfo(MemInfo& info)
{
  info.cnt++;
  info.size += sizeof(*this);

  info.children_["name"].add(name_);
  info.children_["samenet_rules"].add(_samenet_rules);
  info.children_["samenet_matrix"].add(_samenet_matrix);
  info.children_["via_hash"].add(_via_hash);

  _layer_tbl->collectMemInfo(info.children_["layer"]);
  _via_tbl->collectMemInfo(info.children_["via"]);
  _non_default_rule_tbl->collectMemInfo(info.children_["non_default_rule"]);
  _layer_rule_tbl->collectMemInfo(info.children_["layer_rule"]);
  _box_tbl->collectMemInfo(info.children_["box"]);
  _samenet_rule_tbl->collectMemInfo(info.children_["samenet_rule"]);
  _antenna_rule_tbl->collectMemInfo(info.children_["antenna_rule"]);
  _via_rule_tbl->collectMemInfo(info.children_["via_rule"]);
  _via_layer_rule_tbl->collectMemInfo(info.children_["via_layer_rule"]);
  _via_generate_rule_tbl->collectMemInfo(info.children_["via_generate_rule"]);
  _prop_tbl->collectMemInfo(info.children_["prop"]);
  _metal_width_via_map_tbl->collectMemInfo(
      info.children_["metal_width_via_map"]);
  cell_edge_spacing_tbl_->collectMemInfo(info.children_["cell_edge_spacing"]);

  _name_cache->collectMemInfo(info.children_["name_cache"]);
}

}  // namespace odb
