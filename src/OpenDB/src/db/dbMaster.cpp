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

#include "dbMaster.h"

#include "db.h"
#include "dbBox.h"
#include "dbBoxItr.h"
#include "dbDatabase.h"
#include "dbHashTable.hpp"
#include "dbLib.h"
#include "dbMPin.h"
#include "dbMPinItr.h"
#include "dbMTerm.h"
#include "dbSite.h"
#include "dbTable.h"
#include "dbTable.hpp"
#include "dbTarget.h"
#include "dbTargetItr.h"
#include "dbTechLayerAntennaRule.h"
#include "dbTransform.h"

namespace odb {

template class dbHashTable<_dbMTerm>;
template class dbTable<_dbMaster>;

bool _dbMaster::operator==(const _dbMaster& rhs) const
{
  if (_flags._frozen != rhs._flags._frozen)
    return false;

  if (_flags._x_symmetry != rhs._flags._x_symmetry)
    return false;

  if (_flags._y_symmetry != rhs._flags._y_symmetry)
    return false;

  if (_flags._R90_symmetry != rhs._flags._R90_symmetry)
    return false;

  if (_flags._type != rhs._flags._type)
    return false;

  if (_x != rhs._x)
    return false;

  if (_y != rhs._y)
    return false;

  if (_height != rhs._height)
    return false;

  if (_width != rhs._width)
    return false;

  if (_mterm_cnt != rhs._mterm_cnt)
    return false;

  if (_id != rhs._id)
    return false;

  if (_name && rhs._name) {
    if (strcmp(_name, rhs._name) != 0)
      return false;
  } else if (_name || rhs._name)
    return false;

  if (_next_entry != rhs._next_entry)
    return false;

  if (_leq != rhs._leq)
    return false;

  if (_eeq != rhs._eeq)
    return false;

  if (_obstructions != rhs._obstructions)
    return false;

  if (_site != rhs._site)
    return false;

  if (_mterm_hash != rhs._mterm_hash)
    return false;

  if (*_mterm_tbl != *rhs._mterm_tbl)
    return false;

  if (*_mpin_tbl != *rhs._mpin_tbl)
    return false;

  if (*_target_tbl != *rhs._target_tbl)
    return false;

  if (*_box_tbl != *rhs._box_tbl)
    return false;

  if (*_antenna_pin_model_tbl != *rhs._antenna_pin_model_tbl)
    return false;

  return true;
}

void _dbMaster::differences(dbDiff& diff,
                            const char* field,
                            const _dbMaster& rhs) const
{
  DIFF_BEGIN
  DIFF_FIELD(_flags._frozen);
  DIFF_FIELD(_flags._x_symmetry);
  DIFF_FIELD(_flags._y_symmetry);
  DIFF_FIELD(_flags._R90_symmetry);
  DIFF_FIELD(_flags._type);
  DIFF_FIELD(_x);
  DIFF_FIELD(_y);
  DIFF_FIELD(_height);
  DIFF_FIELD(_width);
  DIFF_FIELD(_mterm_cnt);
  DIFF_FIELD(_id);
  DIFF_FIELD(_name);
  DIFF_FIELD(_next_entry);
  DIFF_FIELD(_leq);
  DIFF_FIELD(_eeq);
  DIFF_FIELD(_obstructions);
  DIFF_FIELD(_site);
  DIFF_HASH_TABLE(_mterm_hash);
  DIFF_TABLE_NO_DEEP(_mterm_tbl);
  DIFF_TABLE_NO_DEEP(_mpin_tbl);
  DIFF_TABLE_NO_DEEP(_target_tbl);
  DIFF_TABLE_NO_DEEP(_box_tbl);
  DIFF_TABLE_NO_DEEP(_antenna_pin_model_tbl);
  DIFF_END
}

void _dbMaster::out(dbDiff& diff, char side, const char* field) const
{
  DIFF_OUT_BEGIN
  DIFF_OUT_FIELD(_flags._frozen);
  DIFF_OUT_FIELD(_flags._x_symmetry);
  DIFF_OUT_FIELD(_flags._y_symmetry);
  DIFF_OUT_FIELD(_flags._R90_symmetry);
  DIFF_OUT_FIELD(_flags._type);
  DIFF_OUT_FIELD(_x);
  DIFF_OUT_FIELD(_y);
  DIFF_OUT_FIELD(_height);
  DIFF_OUT_FIELD(_width);
  DIFF_OUT_FIELD(_mterm_cnt);
  DIFF_OUT_FIELD(_id);
  DIFF_OUT_FIELD(_name);
  DIFF_OUT_FIELD(_next_entry);
  DIFF_OUT_FIELD(_leq);
  DIFF_OUT_FIELD(_eeq);
  DIFF_OUT_FIELD(_obstructions);
  DIFF_OUT_FIELD(_site);
  DIFF_OUT_HASH_TABLE(_mterm_hash);
  DIFF_OUT_TABLE_NO_DEEP(_mterm_tbl);
  DIFF_OUT_TABLE_NO_DEEP(_mpin_tbl);
  DIFF_OUT_TABLE_NO_DEEP(_target_tbl);
  DIFF_OUT_TABLE_NO_DEEP(_box_tbl);
  DIFF_OUT_TABLE_NO_DEEP(_antenna_pin_model_tbl);
  DIFF_END
}

////////////////////////////////////////////////////////////////////
//
// _dbMaster - Methods
//
////////////////////////////////////////////////////////////////////
_dbMaster::_dbMaster(_dbDatabase* db)
{
  _flags._x_symmetry = 0;
  _flags._y_symmetry = 0;
  _flags._R90_symmetry = 0;
  _flags._type = dbMasterType::CORE;
  _flags._frozen = 0;
  _flags._mark = 0;
  _flags._special_power = 0;
  _flags._sequential = 0;
  _flags._spare_bits_19 = 0;

  _x = 0;
  _y = 0;
  _height = 0;
  _width = 0;
  _mterm_cnt = 0;
  _id = 0;
  _name = 0;

  _mterm_tbl = new dbTable<_dbMTerm>(
      db, this, (GetObjTbl_t) &_dbMaster::getObjectTable, dbMTermObj, 4, 2);
  ZALLOCATED(_mterm_tbl);

  _mpin_tbl = new dbTable<_dbMPin>(
      db, this, (GetObjTbl_t) &_dbMaster::getObjectTable, dbMPinObj, 4, 2);
  ZALLOCATED(_mpin_tbl);

  _target_tbl = new dbTable<_dbTarget>(
      db, this, (GetObjTbl_t) &_dbMaster::getObjectTable, dbTargetObj, 4, 2);
  ZALLOCATED(_target_tbl);

  _box_tbl = new dbTable<_dbBox>(
      db, this, (GetObjTbl_t) &_dbMaster::getObjectTable, dbBoxObj, 8, 3);
  ZALLOCATED(_box_tbl);

  _antenna_pin_model_tbl = new dbTable<_dbTechAntennaPinModel>(
      db,
      this,
      (GetObjTbl_t) &_dbMaster::getObjectTable,
      dbTechAntennaPinModelObj,
      8,
      3);
  ZALLOCATED(_antenna_pin_model_tbl);

  _box_itr = new dbBoxItr(_box_tbl);
  ZALLOCATED(_box_itr);

  _mpin_itr = new dbMPinItr(_mpin_tbl);
  ZALLOCATED(_mpin_itr);

  _target_itr = new dbTargetItr(_target_tbl);
  ZALLOCATED(_target_itr);

  _mterm_hash.setTable(_mterm_tbl);

  _sta_cell = nullptr;
  _clocked_mterm_index = 0;
  _output_mterm_index = 0;
}

_dbMaster::_dbMaster(_dbDatabase* db, const _dbMaster& m)
    : _flags(m._flags),
      _x(m._x),
      _y(m._y),
      _height(m._height),
      _width(m._width),
      _mterm_cnt(m._mterm_cnt),
      _id(m._id),
      _name(NULL),
      _next_entry(m._next_entry),
      _leq(m._leq),
      _eeq(m._eeq),
      _obstructions(m._obstructions),
      _site(m._site),
      _mterm_hash(m._mterm_hash),
      _sta_cell(m._sta_cell),
      _clocked_mterm_index(m._clocked_mterm_index),
      _output_mterm_index(m._output_mterm_index)
{
  if (m._name) {
    _name = strdup(m._name);
    ZALLOCATED(_name);
  }

  _mterm_tbl = new dbTable<_dbMTerm>(db, this, *m._mterm_tbl);
  ZALLOCATED(_mterm_tbl);

  _mpin_tbl = new dbTable<_dbMPin>(db, this, *m._mpin_tbl);
  ZALLOCATED(_mpin_tbl);

  _target_tbl = new dbTable<_dbTarget>(db, this, *m._target_tbl);
  ZALLOCATED(_target_tbl);

  _box_tbl = new dbTable<_dbBox>(db, this, *m._box_tbl);
  ZALLOCATED(_box_tbl);

  _antenna_pin_model_tbl = new dbTable<_dbTechAntennaPinModel>(
      db, this, *m._antenna_pin_model_tbl);
  ZALLOCATED(_antenna_pin_model_tbl);

  _box_itr = new dbBoxItr(_box_tbl);
  ZALLOCATED(_box_itr);

  _mpin_itr = new dbMPinItr(_mpin_tbl);
  ZALLOCATED(_mpin_itr);

  _target_itr = new dbTargetItr(_target_tbl);
  ZALLOCATED(_target_itr);

  _mterm_hash.setTable(_mterm_tbl);
}

_dbMaster::~_dbMaster()
{
  delete _mterm_tbl;
  delete _mpin_tbl;
  delete _target_tbl;
  delete _box_tbl;
  delete _antenna_pin_model_tbl;
  delete _box_itr;
  delete _mpin_itr;
  delete _target_itr;

  if (_name)
    free((void*) _name);
}

dbOStream& operator<<(dbOStream& stream, const _dbMaster& master)
{
  uint* bit_field = (uint*) &master._flags;
  stream << *bit_field;
  stream << master._x;
  stream << master._y;
  stream << master._height;
  stream << master._width;
  stream << master._mterm_cnt;
  stream << master._id;
  stream << master._name;
  stream << master._next_entry;
  stream << master._leq;
  stream << master._eeq;
  stream << master._obstructions;
  stream << master._site;
  stream << master._mterm_hash;
  stream << *master._mterm_tbl;
  stream << *master._mpin_tbl;
  stream << *master._target_tbl;
  stream << *master._box_tbl;
  stream << *master._antenna_pin_model_tbl;
  return stream;
}

dbIStream& operator>>(dbIStream& stream, _dbMaster& master)
{
  uint* bit_field = (uint*) &master._flags;
  stream >> *bit_field;
  stream >> master._x;
  stream >> master._y;
  stream >> master._height;
  stream >> master._width;
  stream >> master._mterm_cnt;
  stream >> master._id;
  stream >> master._name;
  stream >> master._next_entry;
  stream >> master._leq;
  stream >> master._eeq;
  stream >> master._obstructions;
  stream >> master._site;
  stream >> master._mterm_hash;
  stream >> *master._mterm_tbl;
  stream >> *master._mpin_tbl;
  stream >> *master._target_tbl;
  stream >> *master._box_tbl;
  stream >> *master._antenna_pin_model_tbl;
  return stream;
}

dbObjectTable* _dbMaster::getObjectTable(dbObjectType type)
{
  switch (type) {
    case dbMTermObj:
      return _mterm_tbl;
    case dbMPinObj:
      return _mpin_tbl;
    case dbTargetObj:
      return _target_tbl;
    case dbBoxObj:
      return _box_tbl;
    case dbTechAntennaPinModelObj:
      return _antenna_pin_model_tbl;
    default:
      break;  // DIMITRIS_COMP_WARN
  }

  return getTable()->getObjectTable(type);
}

////////////////////////////////////////////////////////////////////
//
// dbMaster - Methods
//
////////////////////////////////////////////////////////////////////

std::string dbMaster::getName()
{
  _dbMaster* master = (_dbMaster*) this;
  return master->_name;
}

const char* dbMaster::getConstName()
{
  _dbMaster* master = (_dbMaster*) this;
  return master->_name;
}

void dbMaster::getOrigin(int& x, int& y)
{
  _dbMaster* master = (_dbMaster*) this;
  x = master->_x;
  y = master->_y;
}

void dbMaster::setOrigin(int x, int y)
{
  _dbMaster* master = (_dbMaster*) this;
  master->_x = x;
  master->_y = y;
}

void* dbMaster::staCell()
{
  _dbMaster* master = (_dbMaster*) this;
  return master->_sta_cell;
}

void dbMaster::staSetCell(void* cell)
{
  _dbMaster* master = (_dbMaster*) this;
  master->_sta_cell = cell;
}

uint dbMaster::getWidth()
{
  _dbMaster* master = (_dbMaster*) this;
  return master->_width;
}

void dbMaster::setWidth(uint w)
{
  _dbMaster* master = (_dbMaster*) this;
  master->_width = w;
}

uint dbMaster::getHeight()
{
  _dbMaster* master = (_dbMaster*) this;
  return master->_height;
}

void dbMaster::setHeight(uint h)
{
  _dbMaster* master = (_dbMaster*) this;
  master->_height = h;
}

dbMasterType dbMaster::getType() const
{
  _dbMaster* master = (_dbMaster*) this;
  return dbMasterType(master->_flags._type);
}

void dbMaster::setType(dbMasterType type)
{
  _dbMaster* master = (_dbMaster*) this;
  master->_flags._type = type.getValue();
}

dbMaster* dbMaster::getLEQ()
{
  _dbMaster* master = (_dbMaster*) this;

  if (master->_leq == 0)
    return NULL;

  _dbLib* lib = (_dbLib*) master->getOwner();
  return (dbMaster*) lib->_master_tbl->getPtr(master->_leq);
}

void dbMaster::setLEQ(dbMaster* leq)
{
  _dbMaster* master = (_dbMaster*) this;
  master->_leq = leq->getImpl()->getOID();
}

dbMaster* dbMaster::getEEQ()
{
  _dbMaster* master = (_dbMaster*) this;

  if (master->_eeq == 0)
    return NULL;

  _dbLib* lib = (_dbLib*) master->getOwner();
  return (dbMaster*) lib->_master_tbl->getPtr(master->_eeq);
}

void dbMaster::setEEQ(dbMaster* eeq)
{
  _dbMaster* master = (_dbMaster*) this;
  master->_eeq = eeq->getImpl()->getOID();
}

dbSet<dbMTerm> dbMaster::getMTerms()
{
  _dbMaster* master = (_dbMaster*) this;
  return dbSet<dbMTerm>(master, master->_mterm_tbl);
}

dbMTerm* dbMaster::findMTerm(const char* name)
{
  _dbMaster* master = (_dbMaster*) this;
  return (dbMTerm*) master->_mterm_hash.find(name);
}

dbMTerm* dbMaster::findMTerm(dbBlock* block, const char* name)
{
  dbMTerm* mterm = findMTerm(name);
  if (mterm)
    return mterm;
  char blk_left_bus_del, blk_right_bus_del, lib_left_bus_del, lib_right_bus_del;
  char ttname[max_name_length];
  uint ii = 0;
  block->getBusDelimeters(blk_left_bus_del, blk_right_bus_del);
  getLib()->getBusDelimeters(lib_left_bus_del, lib_right_bus_del);
  if (lib_left_bus_del == '\0' || lib_right_bus_del == '\0')
    return mterm;

  if (lib_left_bus_del != blk_left_bus_del
      || lib_right_bus_del != blk_right_bus_del) {
    while (name[ii] != '\0') {
      if (name[ii] == blk_left_bus_del)
        ttname[ii] = lib_left_bus_del;
      else if (name[ii] == blk_right_bus_del)
        ttname[ii] = lib_right_bus_del;
      else
        ttname[ii] = name[ii];
      ii++;
    }
    ttname[ii] = '\0';
    mterm = findMTerm(ttname);
  }
  return mterm;
}

dbLib* dbMaster::getLib()
{
  return (dbLib*) getImpl()->getOwner();
}

dbSet<dbBox> dbMaster::getObstructions()
{
  _dbMaster* master = (_dbMaster*) this;
  return dbSet<dbBox>(master, master->_box_itr);
}

bool dbMaster::isFrozen()
{
  _dbMaster* master = (_dbMaster*) this;
  return master->_flags._frozen != 0;
}

int dbMaster::getMTermCount()
{
  _dbMaster* master = (_dbMaster*) this;
  return master->_mterm_cnt;
}

void dbMaster::setSite(dbSite* site)
{
  _dbMaster* master = (_dbMaster*) this;
  master->_site = site->getImpl()->getOID();
}

dbSite* dbMaster::getSite()
{
  _dbMaster* master = (_dbMaster*) this;
  _dbLib* lib = (_dbLib*) master->getOwner();

  if (master->_site == 0)
    return NULL;

  return (dbSite*) lib->_site_tbl->getPtr(master->_site);
}

void dbMaster::setSymmetryX()
{
  _dbMaster* master = (_dbMaster*) this;
  master->_flags._x_symmetry = 1;
}

bool dbMaster::getSymmetryX()
{
  _dbMaster* master = (_dbMaster*) this;
  return master->_flags._x_symmetry != 0;
}

void dbMaster::setSymmetryY()
{
  _dbMaster* master = (_dbMaster*) this;
  master->_flags._y_symmetry = 1;
}

bool dbMaster::getSymmetryY()
{
  _dbMaster* master = (_dbMaster*) this;
  return master->_flags._y_symmetry != 0;
}

void dbMaster::setSymmetryR90()
{
  _dbMaster* master = (_dbMaster*) this;
  master->_flags._R90_symmetry = 1;
}

bool dbMaster::getSymmetryR90()
{
  _dbMaster* master = (_dbMaster*) this;
  return master->_flags._R90_symmetry != 0;
}

void dbMaster::setFrozen()
{
  _dbMaster* master = (_dbMaster*) this;

  if (master->_flags._frozen == 1)
    return;

  master->_flags._frozen = 1;

  // set the order id on the mterm.
  // this id is used to index mterms on a inst-hdr
  dbSet<dbMTerm> mterms = getMTerms();
  dbSet<dbMTerm>::iterator itr;
  int i = 0;

  for (itr = mterms.begin(); itr != mterms.end(); ++itr) {
    _dbMTerm* mterm = (_dbMTerm*) *itr;
    mterm->_order_id = i++;
  }
}
int dbMaster::getOutputIndex()
{
  _dbMaster* master = (_dbMaster*) this;
  return master->_output_mterm_index;
}
void dbMaster::setOutputIndex(int v)
{
  _dbMaster* master = (_dbMaster*) this;
  master->_output_mterm_index = v;
}
void dbMaster::setClockedIndex(int v)
{
  _dbMaster* master = (_dbMaster*) this;
  master->_clocked_mterm_index = v;
}
int dbMaster::getClockedIndex()
{
  _dbMaster* master = (_dbMaster*) this;
  return master->_clocked_mterm_index;
}
void dbMaster::setSequential(uint v)
{
  _dbMaster* master = (_dbMaster*) this;
  master->_flags._sequential = v;
}

bool dbMaster::isSequential()
{
  _dbMaster* master = (_dbMaster*) this;
  return master->_flags._sequential > 0 ? true : false;
}
void dbMaster::setMark(uint mark)
{
  _dbMaster* master = (_dbMaster*) this;
  master->_flags._mark = mark;
}

uint dbMaster::isMarked()
{
  _dbMaster* master = (_dbMaster*) this;
  return master->_flags._mark;
}

void dbMaster::setSpecialPower(bool value)
{
  _dbMaster* master = (_dbMaster*) this;
  master->_flags._special_power = (value == true) ? 1 : 0;
}

bool dbMaster::isSpecialPower()
{
  _dbMaster* master = (_dbMaster*) this;
  return master->_flags._special_power == 1;
}

void dbMaster::getPlacementBoundary(Rect& r)
{
  _dbMaster* master = (_dbMaster*) this;
  r = Rect(0, 0, master->_width, master->_height);
  dbTransform t(Point(-master->_x, -master->_y));
  t.apply(r);
}

void dbMaster::transform(dbTransform& t)
{
  //_dbMaster * master = (_dbMaster *) this;
  dbSet<dbBox> obs = getObstructions();
  dbSet<dbBox>::iterator itr;

  for (itr = obs.begin(); itr != obs.end(); ++itr) {
    _dbBox* box = (_dbBox*) *itr;
    t.apply(box->_shape._rect);
  }

  dbSet<dbMTerm> mterms = getMTerms();
  dbSet<dbMTerm>::iterator mitr;

  for (mitr = mterms.begin(); mitr != mterms.end(); ++mitr) {
    dbMTerm* mterm = *mitr;
    dbSet<dbMPin> mpins = mterm->getMPins();
    dbSet<dbMPin>::iterator pitr;

    for (pitr = mpins.begin(); pitr != mpins.end(); ++pitr) {
      dbMPin* mpin = *pitr;

      dbSet<dbBox> geoms = mpin->getGeometry();
      dbSet<dbBox>::iterator gitr;

      for (gitr = geoms.begin(); gitr != geoms.end(); ++gitr) {
        _dbBox* box = (_dbBox*) *gitr;
        t.apply(box->_shape._rect);
      }
    }
  }
}

int dbMaster::getMasterId()
{
  _dbMaster* master = (_dbMaster*) this;
  return master->_id;
}

dbMaster* dbMaster::create(dbLib* lib_, const char* name_)
{
  if (lib_->findMaster(name_))
    return NULL;

  _dbLib* lib = (_dbLib*) lib_;
  _dbDatabase* db = lib->getDatabase();
  _dbMaster* master = lib->_master_tbl->create();
  master->_name = strdup(name_);
  ZALLOCATED(master->_name);
  master->_id = db->_master_id++;
  lib->_master_hash.insert(master);
  return (dbMaster*) master;
}

dbMaster* dbMaster::getMaster(dbLib* lib_, uint dbid_)
{
  _dbLib* lib = (_dbLib*) lib_;
  return (dbMaster*) lib->_master_tbl->getPtr(dbid_);
}

bool dbMaster::isFiller()
{
  _dbMaster* master = (_dbMaster*) this;
  // dbMasterType type= dbMasterType(master->_flags._type);

  if (getMTermCount() == 2) {
    bool signal = false;
    dbSet<dbMTerm>::iterator itr;
    dbSet<dbMTerm> mterms = getMTerms();
    for (itr = mterms.begin(); itr != mterms.end(); ++itr) {
      dbMTerm* mt = *itr;
      if (!((mt->getSigType() == dbSigType::GROUND)
            || (mt->getSigType() == dbSigType::POWER))) {
        signal = true;
        break;
      }
    }
    if (!signal)
      return true;
  }

  switch (master->_flags._type) {
    case dbMasterType::CORE_SPACER:
      return true;
    default:
      return false;
  }
}

bool dbMaster::isCoreAutoPlaceable()
{
  // Use switch so if new types are added we get a compiler warning.
  switch (getType()) {
    case dbMasterType::CORE:
    case dbMasterType::CORE_FEEDTHRU:
    case dbMasterType::CORE_TIEHIGH:
    case dbMasterType::CORE_TIELOW:
    case dbMasterType::CORE_SPACER:
    case dbMasterType::CORE_WELLTAP:
    case dbMasterType::CORE_ANTENNACELL:
    case dbMasterType::BLOCK:
    case dbMasterType::BLOCK_BLACKBOX:
    case dbMasterType::BLOCK_SOFT:
    case dbMasterType::ENDCAP:
    case dbMasterType::ENDCAP_PRE:
    case dbMasterType::ENDCAP_POST:
    case dbMasterType::ENDCAP_TOPLEFT:
    case dbMasterType::ENDCAP_TOPRIGHT:
    case dbMasterType::ENDCAP_BOTTOMLEFT:
    case dbMasterType::ENDCAP_BOTTOMRIGHT:
      return true;
      // These classes are completely ignored by the placer.
    case dbMasterType::COVER:
    case dbMasterType::COVER_BUMP:
    case dbMasterType::RING:
    case dbMasterType::PAD:
    case dbMasterType::PAD_AREAIO:
    case dbMasterType::PAD_INPUT:
    case dbMasterType::PAD_OUTPUT:
    case dbMasterType::PAD_INOUT:
    case dbMasterType::PAD_POWER:
    case dbMasterType::PAD_SPACER:
    case dbMasterType::NONE:
      return false;
  }
  // gcc warning
  return false;
}

}  // namespace odb
