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

#include "dbTech.h"

#include "db.h"
#include "dbBox.h"
#include "dbBoxItr.h"
#include "dbDatabase.h"
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
#include "utility/Logger.h"

namespace odb {

template class dbTable<_dbTech>;

bool _dbTech::operator==(const _dbTech& rhs) const
{
  if (_flags._namecase != rhs._flags._namecase)
    return false;

  if (_flags._haswireext != rhs._flags._haswireext)
    return false;

  if (_flags._nowireext != rhs._flags._nowireext)
    return false;

  if (_flags._hasclmeas != rhs._flags._hasclmeas)
    return false;

  if (_flags._clmeas != rhs._flags._clmeas)
    return false;

  if (_flags._hasminspobs != rhs._flags._hasminspobs)
    return false;

  if (_flags._minspobs != rhs._flags._minspobs)
    return false;

  if (_flags._hasminsppin != rhs._flags._hasminsppin)
    return false;

  if (_flags._minsppin != rhs._flags._minsppin)
    return false;

  if (_version != rhs._version)
    return false;

  if (strcmp(_version_buf, rhs._version_buf) != 0)
    return false;

  if (_via_cnt != rhs._via_cnt)
    return false;

  if (_layer_cnt != rhs._layer_cnt)
    return false;

  if (_rlayer_cnt != rhs._rlayer_cnt)
    return false;

  if (_lef_units != rhs._lef_units)
    return false;

  if (_dbu_per_micron != rhs._dbu_per_micron)
    return false;

  if (_mfgrid != rhs._mfgrid)
    return false;

  if (_bottom != rhs._bottom)
    return false;

  if (_top != rhs._top)
    return false;

  if (_non_default_rules != rhs._non_default_rules)
    return false;

  if (_samenet_rules != rhs._samenet_rules)
    return false;

  if (_samenet_matrix != rhs._samenet_matrix)
    return false;

  if (*_layer_tbl != *rhs._layer_tbl)
    return false;

  if (*_via_tbl != *rhs._via_tbl)
    return false;

  if (*_non_default_rule_tbl != *rhs._non_default_rule_tbl)
    return false;

  if (*_layer_rule_tbl != *rhs._layer_rule_tbl)
    return false;

  if (*_box_tbl != *rhs._box_tbl)
    return false;

  if (*_samenet_rule_tbl != *rhs._samenet_rule_tbl)
    return false;

  if (*_antenna_rule_tbl != *rhs._antenna_rule_tbl)
    return false;

  if (*_via_rule_tbl != *rhs._via_rule_tbl)
    return false;

  if (*_via_layer_rule_tbl != *rhs._via_layer_rule_tbl)
    return false;

  if (*_via_generate_rule_tbl != *rhs._via_generate_rule_tbl)
    return false;

  if (*_prop_tbl != *rhs._prop_tbl)
    return false;

  if (*_name_cache != *rhs._name_cache)
    return false;

  if (_via_hash != rhs._via_hash)
    return false;

  return true;
}

void _dbTech::differences(dbDiff& diff,
                          const char* field,
                          const _dbTech& rhs) const
{
  DIFF_BEGIN
  DIFF_FIELD(_flags._namecase);
  DIFF_FIELD(_flags._haswireext);
  DIFF_FIELD(_flags._nowireext);
  DIFF_FIELD(_flags._hasclmeas);
  DIFF_FIELD(_flags._clmeas);
  DIFF_FIELD(_flags._hasminspobs);
  DIFF_FIELD(_flags._minspobs);
  DIFF_FIELD(_flags._hasminsppin);
  DIFF_FIELD(_flags._minsppin);
  DIFF_FIELD(_version);
  DIFF_FIELD(_version_buf);
  DIFF_FIELD(_via_cnt);
  DIFF_FIELD(_layer_cnt);
  DIFF_FIELD(_rlayer_cnt);
  DIFF_FIELD(_lef_units);
  DIFF_FIELD(_dbu_per_micron);
  DIFF_FIELD(_mfgrid);
  DIFF_FIELD(_bottom);
  DIFF_FIELD(_top);
  DIFF_FIELD(_non_default_rules);
  DIFF_VECTOR(_samenet_rules);
  DIFF_MATRIX(_samenet_matrix);
  if (!diff.deepDiff()) {
    DIFF_HASH_TABLE(_via_hash);
  }
  DIFF_TABLE_NO_DEEP(_layer_tbl);
  DIFF_TABLE_NO_DEEP(_via_tbl);
  DIFF_TABLE_NO_DEEP(_non_default_rule_tbl);
  DIFF_TABLE_NO_DEEP(_layer_rule_tbl);
  DIFF_TABLE_NO_DEEP(_box_tbl);
  DIFF_TABLE_NO_DEEP(_samenet_rule_tbl);
  DIFF_TABLE_NO_DEEP(_antenna_rule_tbl);
  DIFF_TABLE_NO_DEEP(_via_rule_tbl);
  DIFF_TABLE_NO_DEEP(_via_layer_rule_tbl);
  DIFF_TABLE_NO_DEEP(_via_generate_rule_tbl);
  DIFF_TABLE_NO_DEEP(_prop_tbl);
  DIFF_NAME_CACHE(_name_cache);
  DIFF_END
}

void _dbTech::out(dbDiff& diff, char side, const char* field) const
{
  DIFF_OUT_BEGIN
  DIFF_OUT_FIELD(_flags._namecase);
  DIFF_OUT_FIELD(_flags._haswireext);
  DIFF_OUT_FIELD(_flags._nowireext);
  DIFF_OUT_FIELD(_flags._hasclmeas);
  DIFF_OUT_FIELD(_flags._clmeas);
  DIFF_OUT_FIELD(_flags._hasminspobs);
  DIFF_OUT_FIELD(_flags._minspobs);
  DIFF_OUT_FIELD(_flags._hasminsppin);
  DIFF_OUT_FIELD(_flags._minsppin);
  DIFF_OUT_FIELD(_version);
  DIFF_OUT_FIELD(_version_buf);
  DIFF_OUT_FIELD(_via_cnt);
  DIFF_OUT_FIELD(_layer_cnt);
  DIFF_OUT_FIELD(_rlayer_cnt);
  DIFF_OUT_FIELD(_lef_units);
  DIFF_OUT_FIELD(_dbu_per_micron);
  DIFF_OUT_FIELD(_mfgrid);
  DIFF_OUT_FIELD(_bottom);
  DIFF_OUT_FIELD(_top);
  DIFF_OUT_FIELD(_non_default_rules);
  DIFF_OUT_VECTOR(_samenet_rules);
  DIFF_OUT_MATRIX(_samenet_matrix);
  if (!diff.deepDiff()) {
    DIFF_OUT_HASH_TABLE(_via_hash);
  }
  DIFF_OUT_TABLE_NO_DEEP(_layer_tbl);
  DIFF_OUT_TABLE_NO_DEEP(_via_tbl);
  DIFF_OUT_TABLE_NO_DEEP(_non_default_rule_tbl);
  DIFF_OUT_TABLE_NO_DEEP(_layer_rule_tbl);
  DIFF_OUT_TABLE_NO_DEEP(_box_tbl);
  DIFF_OUT_TABLE_NO_DEEP(_samenet_rule_tbl);
  DIFF_OUT_TABLE_NO_DEEP(_antenna_rule_tbl);
  DIFF_OUT_TABLE_NO_DEEP(_via_rule_tbl);
  DIFF_OUT_TABLE_NO_DEEP(_via_layer_rule_tbl);
  DIFF_OUT_TABLE_NO_DEEP(_via_generate_rule_tbl);
  DIFF_OUT_TABLE_NO_DEEP(_prop_tbl);
  DIFF_OUT_NAME_CACHE(_name_cache);
  DIFF_END
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
  _dbu_per_micron = 1000;
  _mfgrid = 0;

  _flags._namecase = dbOnOffType::ON;
  _flags._haswireext = dbOnOffType::OFF;
  _flags._nowireext = dbOnOffType::OFF;
  _flags._hasclmeas = dbOnOffType::OFF;
  _flags._clmeas = dbClMeasureType::EUCLIDEAN;
  _flags._hasminspobs = dbOnOffType::OFF;
  _flags._minspobs = dbOnOffType::ON;
  _flags._hasminsppin = dbOnOffType::OFF;
  _flags._minsppin = dbOnOffType::OFF;
  _flags._spare_bits = 0;
  _version = 5.4;
  strncpy(_version_buf, "5.4", 9);

  _layer_tbl = new dbTable<_dbTechLayer>(
      db, this, (GetObjTbl_t) &_dbTech::getObjectTable, dbTechLayerObj);
  ZALLOCATED(_layer_tbl);

  _via_tbl = new dbTable<_dbTechVia>(
      db, this, (GetObjTbl_t) &_dbTech::getObjectTable, dbTechViaObj);
  ZALLOCATED(_via_tbl);

  _non_default_rule_tbl = new dbTable<_dbTechNonDefaultRule>(
      db,
      this,
      (GetObjTbl_t) &_dbTech::getObjectTable,
      dbTechNonDefaultRuleObj,
      4,
      2);
  ZALLOCATED(_non_default_rule_tbl);

  _layer_rule_tbl
      = new dbTable<_dbTechLayerRule>(db,
                                      this,
                                      (GetObjTbl_t) &_dbTech::getObjectTable,
                                      dbTechLayerRuleObj,
                                      4,
                                      2);
  ZALLOCATED(_layer_rule_tbl);

  _box_tbl = new dbTable<_dbBox>(
      db, this, (GetObjTbl_t) &_dbTech::getObjectTable, dbBoxObj);
  ZALLOCATED(_box_tbl);

  _samenet_rule_tbl
      = new dbTable<_dbTechSameNetRule>(db,
                                        this,
                                        (GetObjTbl_t) &_dbTech::getObjectTable,
                                        dbTechSameNetRuleObj,
                                        16,
                                        4);
  ZALLOCATED(_samenet_rule_tbl);

  _antenna_rule_tbl = new dbTable<_dbTechLayerAntennaRule>(
      db,
      this,
      (GetObjTbl_t) &_dbTech::getObjectTable,
      dbTechLayerAntennaRuleObj,
      16,
      4);
  ZALLOCATED(_antenna_rule_tbl);

  _via_rule_tbl
      = new dbTable<_dbTechViaRule>(db,
                                    this,
                                    (GetObjTbl_t) &_dbTech::getObjectTable,
                                    dbTechViaRuleObj,
                                    16,
                                    4);
  ZALLOCATED(_via_rule_tbl);

  _via_layer_rule_tbl
      = new dbTable<_dbTechViaLayerRule>(db,
                                         this,
                                         (GetObjTbl_t) &_dbTech::getObjectTable,
                                         dbTechViaLayerRuleObj,
                                         16,
                                         4);
  ZALLOCATED(_via_layer_rule_tbl);

  _via_generate_rule_tbl = new dbTable<_dbTechViaGenerateRule>(
      db,
      this,
      (GetObjTbl_t) &_dbTech::getObjectTable,
      dbTechViaGenerateRuleObj,
      16,
      4);
  ZALLOCATED(_via_generate_rule_tbl);

  _prop_tbl = new dbTable<_dbProperty>(
      db, this, (GetObjTbl_t) &_dbTech::getObjectTable, dbPropertyObj);
  ZALLOCATED(_prop_tbl);

  _via_hash.setTable(_via_tbl);

  _name_cache
      = new _dbNameCache(db, this, (GetObjTbl_t) &_dbTech::getObjectTable);
  ZALLOCATED(_name_cache);

  _layer_itr = new dbTechLayerItr(_layer_tbl);
  ZALLOCATED(_layer_itr);

  _box_itr = new dbBoxItr(_box_tbl);
  ZALLOCATED(_box_itr);

  _prop_itr = new dbPropertyItr(_prop_tbl);
  ZALLOCATED(_prop_itr);
}

_dbTech::_dbTech(_dbDatabase* db, const _dbTech& t)
    : _version(t._version),
      _via_cnt(t._via_cnt),
      _layer_cnt(t._layer_cnt),
      _rlayer_cnt(t._rlayer_cnt),
      _lef_units(t._lef_units),
      _dbu_per_micron(t._dbu_per_micron),
      _mfgrid(t._mfgrid),
      _flags(t._flags),
      _bottom(t._bottom),
      _top(t._top),
      _non_default_rules(t._non_default_rules),
      _samenet_rules(t._samenet_rules),
      _samenet_matrix(t._samenet_matrix),
      _via_hash(t._via_hash)
{
  strncpy(_version_buf, t._version_buf, sizeof(_version_buf));

  _layer_tbl = new dbTable<_dbTechLayer>(db, this, *t._layer_tbl);
  ZALLOCATED(_layer_tbl);

  _via_tbl = new dbTable<_dbTechVia>(db, this, *t._via_tbl);
  ZALLOCATED(_via_tbl);

  _non_default_rule_tbl
      = new dbTable<_dbTechNonDefaultRule>(db, this, *t._non_default_rule_tbl);
  ZALLOCATED(_non_default_rule_tbl);

  _layer_rule_tbl = new dbTable<_dbTechLayerRule>(db, this, *t._layer_rule_tbl);
  ZALLOCATED(_layer_rule_tbl);

  _box_tbl = new dbTable<_dbBox>(db, this, *t._box_tbl);
  ZALLOCATED(_box_tbl);

  _samenet_rule_tbl
      = new dbTable<_dbTechSameNetRule>(db, this, *t._samenet_rule_tbl);
  ZALLOCATED(_samenet_rule_tbl);

  _antenna_rule_tbl
      = new dbTable<_dbTechLayerAntennaRule>(db, this, *t._antenna_rule_tbl);
  ZALLOCATED(_antenna_rule_tbl);

  _via_rule_tbl = new dbTable<_dbTechViaRule>(db, this, *t._via_rule_tbl);
  ZALLOCATED(_via_rule_tbl);

  _via_layer_rule_tbl
      = new dbTable<_dbTechViaLayerRule>(db, this, *t._via_layer_rule_tbl);
  ZALLOCATED(_via_layer_rule_tbl);

  _via_generate_rule_tbl = new dbTable<_dbTechViaGenerateRule>(
      db, this, *t._via_generate_rule_tbl);
  ZALLOCATED(_via_generate_rule_tbl);

  _prop_tbl = new dbTable<_dbProperty>(db, this, *t._prop_tbl);
  ZALLOCATED(_prop_tbl);

  _via_hash.setTable(_via_tbl);

  _name_cache = new _dbNameCache(db, this, *t._name_cache);
  ZALLOCATED(_name_cache);

  _layer_itr = new dbTechLayerItr(_layer_tbl);
  ZALLOCATED(_layer_itr);

  _box_itr = new dbBoxItr(_box_tbl);
  ZALLOCATED(_box_itr);

  _prop_itr = new dbPropertyItr(_prop_tbl);
  ZALLOCATED(_prop_itr);
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
  delete _name_cache;
  /******************************************* dimitri_fix
  dbTech.cpp:363:12: warning: deleting object of polymorphic class type
  ‘dbTechLayerItr’ which has non-virtual destructor might cause undefined
  behavior [-Wdelete-non-virtual-dtor] delete _layer_itr;

      delete _layer_itr;
      delete _box_itr;
      delete _prop_itr;
  */
}

dbOStream& operator<<(dbOStream& stream, const _dbTech& tech)
{
  stream << tech._via_cnt;
  stream << tech._layer_cnt;
  stream << tech._rlayer_cnt;
  stream << tech._lef_units;
  stream << tech._dbu_per_micron;
  stream << tech._mfgrid;

  uint* bit_field = (uint*) &tech._flags;
  stream << *bit_field;

  stream << tech._getLefVersion();
  stream << tech._bottom;
  stream << tech._top;
  stream << tech._non_default_rules;
  stream << tech._samenet_rules;
  stream << tech._samenet_matrix;
  stream << *tech._layer_tbl;
  stream << *tech._via_tbl;
  stream << *tech._non_default_rule_tbl;
  stream << *tech._layer_rule_tbl;
  stream << *tech._box_tbl;
  stream << *tech._samenet_rule_tbl;
  stream << *tech._antenna_rule_tbl;
  stream << *tech._via_rule_tbl;
  stream << *tech._via_layer_rule_tbl;
  stream << *tech._via_generate_rule_tbl;
  stream << *tech._prop_tbl;
  stream << *tech._name_cache;
  stream << tech._via_hash;
  return stream;
}

dbIStream& operator>>(dbIStream& stream, _dbTech& tech)
{
  stream >> tech._via_cnt;
  stream >> tech._layer_cnt;
  stream >> tech._rlayer_cnt;
  stream >> tech._lef_units;
  stream >> tech._dbu_per_micron;
  stream >> tech._mfgrid;

  uint* bit_field = (uint*) &tech._flags;
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
  stream >> *tech._name_cache;
  stream >> tech._via_hash;

  return stream;
}

double _dbTech::_getLefVersion() const
{
  return _version;
}

const char* _dbTech::_getLefVersionStr() const
{
  return (const char*) _version_buf;
}

//
// Version stored as double in %d.%1d%2d format;
// the last two digits are optional
//
void _dbTech::_setLefVersion(double inver)
{
  _version = inver;
  int major_version = (int) floor(inver);
  int minor_version = ((int) floor(inver * 10.0)) - (major_version * 10);
  int opt_minor_version = ((int) floor(inver * 1000.0)) - (major_version * 1000)
                          - (minor_version * 100);
  if (opt_minor_version > 0)
    snprintf(_version_buf,
             sizeof(_version_buf),
             "%d.%d.%d",
             major_version,
             minor_version,
             opt_minor_version);
  else
    snprintf(_version_buf, 10, "%d.%d", major_version, minor_version);
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
  dbSet<dbTechLayer> layers = getLayers();
  dbSet<dbTechLayer>::iterator itr;

  for (itr = layers.begin(); itr != layers.end(); ++itr) {
    _dbTechLayer* layer = (_dbTechLayer*) *itr;

    if (strcmp(layer->_name, name) == 0)
      return (dbTechLayer*) layer;
  }

  return NULL;
}

dbTechLayer* dbTech::findLayer(int layer_number)
{
  dbSet<dbTechLayer> layers = getLayers();
  dbSet<dbTechLayer>::iterator itr;

  for (itr = layers.begin(); itr != layers.end(); ++itr) {
    _dbTechLayer* layer = (_dbTechLayer*) *itr;

    if (layer->_number == (uint) layer_number)
      return (dbTechLayer*) layer;
  }

  return NULL;
}

dbTechLayer* dbTech::findRoutingLayer(int level_number)
{
  dbSet<dbTechLayer> layers = getLayers();
  dbSet<dbTechLayer>::iterator itr;

  for (itr = layers.begin(); itr != layers.end(); ++itr) {
    _dbTechLayer* layer = (_dbTechLayer*) *itr;

    if (layer->_rlevel == (uint) level_number)
      return (dbTechLayer*) layer;
  }

  return NULL;
}

void dbTech::setDbUnitsPerMicron(int value)
{
  _dbTech* tech = (_dbTech*) this;
  tech->_dbu_per_micron = value;
}

int dbTech::getDbUnitsPerMicron()
{
  _dbTech* tech = (_dbTech*) this;
  return tech->_dbu_per_micron;
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

const char* dbTech::getLefVersionStr() const
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
  return (tech->_flags._haswireext == dbOnOffType::ON);
}

dbOnOffType dbTech::getNoWireExtAtPin() const
{
  _dbTech* tech = (_dbTech*) this;
  return (hasNoWireExtAtPin() ? dbOnOffType(tech->_flags._nowireext)
                              : dbOnOffType(dbOnOffType::OFF));
}

void dbTech::setNoWireExtAtPin(dbOnOffType intyp)
{
  _dbTech* tech = (_dbTech*) this;
  tech->_flags._haswireext = dbOnOffType::ON;
  tech->_flags._nowireext = intyp.getValue();
}

dbOnOffType dbTech::getNamesCaseSensitive() const
{
  _dbTech* tech = (_dbTech*) this;

  return dbOnOffType((bool) tech->_flags._namecase);
  // return dbOnOffType(tech->_flags._namecase);
}

void dbTech::setNamesCaseSensitive(dbOnOffType intyp)
{
  _dbTech* tech = (_dbTech*) this;
  tech->_flags._namecase = intyp.getValue();
}

bool dbTech::hasClearanceMeasure() const
{
  _dbTech* tech = (_dbTech*) this;
  return (tech->_flags._hasclmeas == dbOnOffType::ON);
}

dbClMeasureType dbTech::getClearanceMeasure() const
{
  _dbTech* tech = (_dbTech*) this;
  return (hasClearanceMeasure() ? dbClMeasureType(tech->_flags._clmeas)
                                : dbClMeasureType(dbClMeasureType::EUCLIDEAN));
}

void dbTech::setClearanceMeasure(dbClMeasureType inmeas)
{
  _dbTech* tech = (_dbTech*) this;
  tech->_flags._hasclmeas = dbOnOffType::ON;
  tech->_flags._clmeas = inmeas.getValue();
}

bool dbTech::hasUseMinSpacingObs() const
{
  _dbTech* tech = (_dbTech*) this;
  return (tech->_flags._hasminspobs == dbOnOffType::ON);
}

dbOnOffType dbTech::getUseMinSpacingObs() const
{
  _dbTech* tech = (_dbTech*) this;
  return (hasUseMinSpacingObs() ? dbOnOffType(tech->_flags._minspobs)
                                : dbOnOffType(dbOnOffType::ON));
}

void dbTech::setUseMinSpacingObs(dbOnOffType intyp)
{
  _dbTech* tech = (_dbTech*) this;
  tech->_flags._hasminspobs = dbOnOffType::ON;
  tech->_flags._minspobs = intyp.getValue();
}

bool dbTech::hasUseMinSpacingPin() const
{
  _dbTech* tech = (_dbTech*) this;
  return (tech->_flags._hasminsppin == dbOnOffType::ON);
}

dbOnOffType dbTech::getUseMinSpacingPin() const
{
  _dbTech* tech = (_dbTech*) this;
  return (hasUseMinSpacingPin() ? dbOnOffType(tech->_flags._minsppin)
                                : dbOnOffType(dbOnOffType::ON));
}

void dbTech::setUseMinSpacingPin(dbOnOffType intyp)
{
  _dbTech* tech = (_dbTech*) this;
  tech->_flags._hasminsppin = dbOnOffType::ON;
  tech->_flags._minsppin = intyp.getValue();
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
  dbSet<dbTechNonDefaultRule> rules = getNonDefaultRules();
  dbSet<dbTechNonDefaultRule>::iterator itr;

  for (itr = rules.begin(); itr != rules.end(); ++itr) {
    _dbTechNonDefaultRule* r = (_dbTechNonDefaultRule*) *itr;

    if (strcmp(r->_name, name) == 0)
      return (dbTechNonDefaultRule*) r;
  }

  return NULL;
}

dbTechSameNetRule* dbTech::findSameNetRule(dbTechLayer* l1_, dbTechLayer* l2_)
{
  _dbTech* tech = (_dbTech*) this;
  _dbTechLayer* l1 = (_dbTechLayer*) l1_;
  _dbTechLayer* l2 = (_dbTechLayer*) l2_;
  dbId<_dbTechSameNetRule> rule
      = tech->_samenet_matrix(l1->_number, l2->_number);

  if (rule == 0)
    return NULL;

  return (dbTechSameNetRule*) tech->_samenet_rule_tbl->getPtr(rule);
}

void dbTech::getSameNetRules(std::vector<dbTechSameNetRule*>& rules)
{
  _dbTech* tech = (_dbTech*) this;
  rules.clear();
  dbVector<dbId<_dbTechSameNetRule>>::iterator itr;

  for (itr = tech->_samenet_rules.begin(); itr != tech->_samenet_rules.end();
       ++itr) {
    dbId<_dbTechSameNetRule> r = *itr;
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

dbTechViaRule* dbTech::findViaRule(const char* name)
{
  dbSet<dbTechViaRule> rules = getViaRules();
  dbSet<dbTechViaRule>::iterator itr;

  for (itr = rules.begin(); itr != rules.end(); ++itr) {
    _dbTechViaRule* rule = (_dbTechViaRule*) *itr;

    if (strcmp(name, rule->_name) == 0)
      return (dbTechViaRule*) rule;
  }

  return NULL;
}

dbTechViaGenerateRule* dbTech::findViaGenerateRule(const char* name)
{
  dbSet<dbTechViaGenerateRule> rules = getViaGenerateRules();
  dbSet<dbTechViaGenerateRule>::iterator itr;

  for (itr = rules.begin(); itr != rules.end(); ++itr) {
    _dbTechViaGenerateRule* rule = (_dbTechViaGenerateRule*) *itr;

    if (strcmp(name, rule->_name) == 0)
      return (dbTechViaGenerateRule*) rule;
  }

  return NULL;
}

void dbTech::checkLayer(bool typeChk,
                        bool widthChk,
                        bool pitchChk,
                        bool spacingChk)
{
  dbSet<dbTechLayer> layers = getLayers();
  dbSet<dbTechLayer>::iterator itr;
  if (!typeChk && !widthChk && !pitchChk && !spacingChk) {
    typeChk = true;
    widthChk = true;
    pitchChk = true;
    spacingChk = true;
  }

  dbTechLayer* layer;
  dbTechLayerType type;
  int pitch, spacing, level;
  uint width;
  for (itr = layers.begin(); itr != layers.end(); ++itr) {
    layer = (dbTechLayer*) *itr;
    type = layer->getType();
    if (type.getValue() == dbTechLayerType::CUT)
      continue;
    if (typeChk && type.getValue() != dbTechLayerType::ROUTING) {
      // TODO: remove this line
      warning(0, "Layer %s is not a routing layer!\n", layer->getConstName());
      getImpl()->getLogger()->warn(utl::ODB,
                                   58,
                                   "Layer {} is not a routing layer!",
                                   layer->getConstName());
      continue;
    }
    level = layer->getRoutingLevel();
    pitch = layer->getPitch();
    if (pitchChk && pitch <= 0)
      getImpl()->getLogger()->error(
          utl::ODB,
          59,
          "Layer {}, routing level {}, has {} pitch !!",
          layer->getConstName(),
          level,
          pitch);
    width = layer->getWidth();
    if (widthChk && width == 0)
      getImpl()->getLogger()->error(
          utl::ODB,
          60,
          "Layer {}, routing level {}, has {} width !!",
          layer->getConstName(),
          level,
          width);
    spacing = layer->getSpacing();
    if (spacingChk && spacing <= 0)
      getImpl()->getLogger()->error(
          utl::ODB,
          61,
          "Layer {}, routing level {}, has {} spacing !!",
          layer->getConstName(),
          level,
          spacing);
  }

  return;
}
dbTech* dbTech::create(dbDatabase* db_, int dbu_per_micron)
{
  _dbDatabase* db = (_dbDatabase*) db_;

  if (db->_tech != 0)
    return NULL;

  _dbTech* tech = db->_tech_tbl->create();
  db->_tech = tech->getOID();
  tech->_dbu_per_micron = dbu_per_micron;
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
  db->_tech = 0;
}

}  // namespace odb
