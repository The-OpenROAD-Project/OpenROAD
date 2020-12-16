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

#include "dbMTerm.h"

#include "db.h"
#include "dbDatabase.h"
#include "dbLib.h"
#include "dbMPinItr.h"
#include "dbMaster.h"
#include "dbTable.h"
#include "dbTable.hpp"
#include "dbTargetItr.h"
#include "dbTechLayerAntennaRule.h"
#include "lefout.h"

namespace odb {

template class dbTable<_dbMTerm>;

bool _dbMTerm::operator==(const _dbMTerm& rhs) const
{
  if (_flags._io_type != rhs._flags._io_type)
    return false;

  if (_flags._sig_type != rhs._flags._sig_type)
    return false;

  if (_order_id != rhs._order_id)
    return false;

  if (_name && rhs._name) {
    if (strcmp(_name, rhs._name) != 0)
      return false;
  } else if (_name || rhs._name)
    return false;

  if (_next_entry != rhs._next_entry)
    return false;

  if (_next_mterm != rhs._next_mterm)
    return false;

  if (_pins != rhs._pins)
    return false;

  if (_targets != rhs._targets)
    return false;

  if (_oxide1 != rhs._oxide1)
    return false;

  if (_oxide2 != rhs._oxide2)
    return false;

  if (_par_met_area != rhs._par_met_area)
    return false;

  if (_par_met_sidearea != rhs._par_met_sidearea)
    return false;

  if (_par_cut_area != rhs._par_cut_area)
    return false;

  if (_diffarea != rhs._diffarea)
    return false;

  return true;
}

void _dbMTerm::differences(dbDiff&         diff,
                           const char*     field,
                           const _dbMTerm& rhs) const
{
  DIFF_BEGIN
  DIFF_FIELD(_flags._io_type);
  DIFF_FIELD(_flags._sig_type);
  DIFF_FIELD(_order_id);
  DIFF_FIELD(_name);
  DIFF_FIELD(_next_entry);
  DIFF_FIELD(_next_mterm);
  DIFF_FIELD(_pins);
  DIFF_FIELD(_targets);
  DIFF_FIELD(_oxide1);
  DIFF_FIELD(_oxide2);
  DIFF_VECTOR_PTR(_par_met_area);
  DIFF_VECTOR_PTR(_par_met_sidearea);
  DIFF_VECTOR_PTR(_par_cut_area);
  DIFF_VECTOR_PTR(_diffarea);
  DIFF_END
}

void _dbMTerm::out(dbDiff& diff, char side, const char* field) const
{
  DIFF_OUT_BEGIN
  DIFF_OUT_FIELD(_flags._io_type);
  DIFF_OUT_FIELD(_flags._sig_type);
  DIFF_OUT_FIELD(_order_id);
  DIFF_OUT_FIELD(_name);
  DIFF_OUT_FIELD(_next_entry);
  DIFF_OUT_FIELD(_next_mterm);
  DIFF_OUT_FIELD(_pins);
  DIFF_OUT_FIELD(_targets);
  DIFF_OUT_FIELD(_oxide1);
  DIFF_OUT_FIELD(_oxide2);
  DIFF_OUT_VECTOR_PTR(_par_met_area);
  DIFF_OUT_VECTOR_PTR(_par_met_sidearea);
  DIFF_OUT_VECTOR_PTR(_par_cut_area);
  DIFF_OUT_VECTOR_PTR(_diffarea);
  DIFF_END
}

////////////////////////////////////////////////////////////////////
//
// _dbMTerm - Methods
//
////////////////////////////////////////////////////////////////////

_dbMTerm::_dbMTerm(_dbDatabase*, const _dbMTerm& m)
    : _flags(m._flags),
      _order_id(m._order_id),
      _name(NULL),
      _next_entry(m._next_entry),
      _next_mterm(m._next_mterm),
      _pins(m._pins),
      _targets(m._targets),
      _oxide1(m._oxide1),
      _oxide2(m._oxide2),
      _sta_port(m._sta_port)
{
  if (m._name) {
    _name = strdup(m._name);
    ZALLOCATED(_name);
  }

  dbVector<_dbTechAntennaAreaElement*>::const_iterator itr;

  for (itr = m._par_met_area.begin(); itr != m._par_met_area.end(); ++itr) {
    _dbTechAntennaAreaElement* e = new _dbTechAntennaAreaElement(*(*itr));
    ZALLOCATED(e);
    _par_met_area.push_back(e);
  }

  for (itr = m._par_met_sidearea.begin(); itr != m._par_met_sidearea.end();
       ++itr) {
    _dbTechAntennaAreaElement* e = new _dbTechAntennaAreaElement(*(*itr));
    ZALLOCATED(e);
    _par_met_sidearea.push_back(e);
  }

  for (itr = m._par_cut_area.begin(); itr != m._par_cut_area.end(); ++itr) {
    _dbTechAntennaAreaElement* e = new _dbTechAntennaAreaElement(*(*itr));
    ZALLOCATED(e);
    _par_cut_area.push_back(e);
  }

  for (itr = m._diffarea.begin(); itr != m._diffarea.end(); ++itr) {
    _dbTechAntennaAreaElement* e = new _dbTechAntennaAreaElement(*(*itr));
    ZALLOCATED(e);
    _diffarea.push_back(e);
  }
}
/*
inline _dbMTerm::~_dbMTerm()
{
    if ( _name )
        free( (void *) _name );

    dbVector<_dbTechAntennaAreaElement *>::iterator  antitr;
    for (antitr = _par_met_area.begin(); antitr != _par_met_area.end();
antitr++) delete *antitr; _par_met_area.clear();

    for (antitr = _par_met_sidearea.begin(); antitr != _par_met_sidearea.end();
antitr++) delete *antitr; _par_met_sidearea.clear();

    for (antitr = _par_cut_area.begin(); antitr != _par_cut_area.end();
antitr++) delete *antitr; _par_cut_area.clear();

    for (antitr = _diffarea.begin(); antitr != _diffarea.end(); antitr++)
      delete *antitr;
    _diffarea.clear();
}
*/
dbOStream& operator<<(dbOStream& stream, const _dbMTerm& mterm)
{
  uint* bit_field = (uint*) &mterm._flags;
  stream << *bit_field;
  stream << mterm._order_id;
  stream << mterm._name;
  stream << mterm._next_entry;
  stream << mterm._next_mterm;
  stream << mterm._pins;
  stream << mterm._targets;
  stream << mterm._oxide1;
  stream << mterm._oxide2;
  stream << mterm._par_met_area;
  stream << mterm._par_met_sidearea;
  stream << mterm._par_cut_area;
  stream << mterm._diffarea;
  return stream;
}

dbIStream& operator>>(dbIStream& stream, _dbMTerm& mterm)
{
  uint* bit_field = (uint*) &mterm._flags;
  stream >> *bit_field;
  stream >> mterm._order_id;
  stream >> mterm._name;
  stream >> mterm._next_entry;
  stream >> mterm._next_mterm;
  stream >> mterm._pins;
  stream >> mterm._targets;
  stream >> mterm._oxide1;
  stream >> mterm._oxide2;
  stream >> mterm._par_met_area;
  stream >> mterm._par_met_sidearea;
  stream >> mterm._par_cut_area;
  stream >> mterm._diffarea;
  return stream;
}

////////////////////////////////////////////////////////////////////
//
// dbMTerm - Methods
//
////////////////////////////////////////////////////////////////////

std::string dbMTerm::getName()
{
  _dbMTerm* mterm = (_dbMTerm*) this;
  return mterm->_name;
}

char* dbMTerm::getName(dbInst* inst, char* ttname)
{
  dbBlock*  block  = (dbBlock*) inst->getImpl()->getOwner();
  dbMaster* master = inst->getMaster();
  return (getName(block, master, ttname));
}

char* dbMTerm::getName(dbBlock* block, dbMaster* master, char* ttname)
{
  char* mtname = (char*) getConstName();
  char blk_left_bus_del, blk_right_bus_del, lib_left_bus_del, lib_right_bus_del;
  uint ii = 0;
  block->getBusDelimeters(blk_left_bus_del, blk_right_bus_del);
  if (blk_left_bus_del == '\0' || blk_right_bus_del == '\0')
    return mtname;
  master->getLib()->getBusDelimeters(lib_left_bus_del, lib_right_bus_del);

  if (lib_left_bus_del != blk_left_bus_del
      || lib_right_bus_del != blk_right_bus_del) {
    while (mtname[ii] != '\0') {
      if (mtname[ii] == lib_left_bus_del)
        ttname[ii] = blk_left_bus_del;
      else if (mtname[ii] == lib_right_bus_del)
        ttname[ii] = blk_right_bus_del;
      else
        ttname[ii] = mtname[ii];
      ii++;
    }
    ttname[ii] = '\0';
    return ttname;
  }
  return mtname;
}

const char* dbMTerm::getConstName()
{
  _dbMTerm* mterm = (_dbMTerm*) this;
  return mterm->_name;
}

dbSigType dbMTerm::getSigType()
{
  _dbMTerm* mterm = (_dbMTerm*) this;
  return dbSigType(mterm->_flags._sig_type);
}

dbIoType dbMTerm::getIoType()
{
  _dbMTerm* mterm = (_dbMTerm*) this;
  return dbIoType(mterm->_flags._io_type);
}

void dbMTerm::setMark(uint v)
{
  _dbMTerm* mterm     = (_dbMTerm*) this;
  mterm->_flags._mark = v;
}
bool dbMTerm::isSetMark()
{
  _dbMTerm* mterm = (_dbMTerm*) this;
  return mterm->_flags._mark > 0 ? true : false;
}

dbMaster* dbMTerm::getMaster()
{
  return (dbMaster*) getImpl()->getOwner();
}

dbSet<dbMPin> dbMTerm::getMPins()
{
  _dbMTerm*  mterm  = (_dbMTerm*) this;
  _dbMaster* master = (_dbMaster*) mterm->getOwner();
  return dbSet<dbMPin>(mterm, master->_mpin_itr);
}

dbSet<dbTarget> dbMTerm::getTargets()
{
  _dbMTerm*  mterm  = (_dbMTerm*) this;
  _dbMaster* master = (_dbMaster*) mterm->getOwner();
  return dbSet<dbTarget>(mterm, master->_target_itr);
}

void* dbMTerm::staPort()
{
  _dbMTerm* mterm = (_dbMTerm*) this;
  return mterm->_sta_port;
}

void dbMTerm::staSetPort(void* port)
{
  _dbMTerm* mterm  = (_dbMTerm*) this;
  mterm->_sta_port = port;
}

int dbMTerm::getIndex()
{
  _dbMTerm* mterm = (_dbMTerm*) this;
  return mterm->_order_id;
}

//
// Add entries for antenna info elements that are not model specific
//
void dbMTerm::addPartialMetalAreaEntry(double inval, dbTechLayer* refly)
{
  _dbMTerm* mterm = (_dbMTerm*) this;
  _dbTechAntennaAreaElement::create(mterm->_par_met_area, inval, refly);
}

void dbMTerm::addPartialMetalSideAreaEntry(double inval, dbTechLayer* refly)
{
  _dbMTerm* mterm = (_dbMTerm*) this;
  _dbTechAntennaAreaElement::create(mterm->_par_met_sidearea, inval, refly);
}

void dbMTerm::addPartialCutAreaEntry(double inval, dbTechLayer* refly)
{
  _dbMTerm* mterm = (_dbMTerm*) this;
  _dbTechAntennaAreaElement::create(mterm->_par_cut_area, inval, refly);
}

void dbMTerm::addDiffAreaEntry(double inval, dbTechLayer* refly)
{
  _dbMTerm* mterm = (_dbMTerm*) this;
  _dbTechAntennaAreaElement::create(mterm->_diffarea, inval, refly);
}

dbTechAntennaPinModel* dbMTerm::createDefaultAntennaModel()
{
  _dbMTerm*               mterm = (_dbMTerm*) this;
  _dbTechAntennaPinModel* m
      = (_dbTechAntennaPinModel*) getDefaultAntennaModel();

  // Reinitialize the object to its default state...
  if (m != NULL) {
    m->~_dbTechAntennaPinModel();
    new (m) _dbTechAntennaPinModel(mterm->getDatabase());
    m->_mterm = getImpl()->getOID();
  } else {
    _dbMaster* master = (_dbMaster*) mterm->getOwner();
    m                 = master->_antenna_pin_model_tbl->create();
    mterm->_oxide1    = m->getOID();
    m->_mterm         = getImpl()->getOID();
  }

  return (dbTechAntennaPinModel*) m;
}

dbTechAntennaPinModel* dbMTerm::createOxide2AntennaModel()
{
  _dbMTerm*               mterm = (_dbMTerm*) this;
  _dbTechAntennaPinModel* m = (_dbTechAntennaPinModel*) getOxide2AntennaModel();

  // Reinitialize the object to its default state...
  if (m != NULL) {
    m->~_dbTechAntennaPinModel();
    new (m) _dbTechAntennaPinModel(mterm->getDatabase());
    m->_mterm = getImpl()->getOID();
  } else {
    _dbMaster* master = (_dbMaster*) mterm->getOwner();
    m                 = master->_antenna_pin_model_tbl->create();
    mterm->_oxide2    = m->getOID();
    m->_mterm         = getImpl()->getOID();
  }

  return (dbTechAntennaPinModel*) m;
}

bool dbMTerm::hasDefaultAntennaModel() const
{
  _dbMTerm* mterm = (_dbMTerm*) this;
  return (mterm->_oxide1 != 0);
}

bool dbMTerm::hasOxide2AntennaModel() const
{
  _dbMTerm* mterm = (_dbMTerm*) this;
  return (mterm->_oxide2 != 0);
}

dbTechAntennaPinModel* dbMTerm::getDefaultAntennaModel() const
{
  _dbMTerm* mterm = (_dbMTerm*) this;

  if (mterm->_oxide1 == 0)
    return NULL;

  _dbMaster* master = (_dbMaster*) mterm->getOwner();
  return (dbTechAntennaPinModel*) master->_antenna_pin_model_tbl->getPtr(
      mterm->_oxide1);
}

dbTechAntennaPinModel* dbMTerm::getOxide2AntennaModel() const
{
  _dbMTerm* mterm = (_dbMTerm*) this;

  if (mterm->_oxide2 == 0)
    return NULL;

  _dbMaster* master = (_dbMaster*) mterm->getOwner();
  return (dbTechAntennaPinModel*) master->_antenna_pin_model_tbl->getPtr(
      mterm->_oxide2);
}

void dbMTerm::getDiffArea(std::vector<std::pair<double, dbTechLayer*>>& data)
{
  _dbMTerm* mterm = (_dbMTerm*) this;
  _dbTechAntennaPinModel::getAntennaValues(
      mterm->getDatabase(), mterm->_diffarea, data);
}

void dbMTerm::writeAntennaLef(lefout& writer) const
{
  _dbMTerm* mterm = (_dbMTerm*) this;

  dbVector<_dbTechAntennaAreaElement*>::iterator ant_iter;

  dbMaster* tpmtr = (dbMaster*) mterm->getOwner();
  dbLib*    tplib = (dbLib*) tpmtr->getImpl()->getOwner();
  dbTech*   tech  = tplib->getTech();

  for (ant_iter = mterm->_par_met_area.begin();
       ant_iter != mterm->_par_met_area.end();
       ant_iter++)
    (*ant_iter)->writeLef("ANTENNAPARTIALMETALAREA", tech, writer);

  for (ant_iter = mterm->_par_met_sidearea.begin();
       ant_iter != mterm->_par_met_sidearea.end();
       ant_iter++)
    (*ant_iter)->writeLef("ANTENNAPARTIALMETALSIDEAREA", tech, writer);

  for (ant_iter = mterm->_par_cut_area.begin();
       ant_iter != mterm->_par_cut_area.end();
       ant_iter++)
    (*ant_iter)->writeLef("ANTENNAPARTIALCUTAREA", tech, writer);

  for (ant_iter = mterm->_diffarea.begin(); ant_iter != mterm->_diffarea.end();
       ant_iter++)
    (*ant_iter)->writeLef("ANTENNADIFFAREA", tech, writer);

  if (hasDefaultAntennaModel())
    getDefaultAntennaModel()->writeLef(tech, writer);

  if (hasOxide2AntennaModel()) {
    fprintf(writer.out(), "        ANTENNAMODEL OXIDE2 ;\n");
    getOxide2AntennaModel()->writeLef(tech, writer);
  }
}

dbMTerm* dbMTerm::create(dbMaster*   master_,
                         const char* name_,
                         dbIoType    io_type_,
                         dbSigType   sig_type_)
{
  _dbMaster* master = (_dbMaster*) master_;

  if (master->_flags._frozen || master->_mterm_hash.hasMember(name_))
    return NULL;

  _dbMTerm* mterm = master->_mterm_tbl->create();
  mterm->_name    = strdup(name_);
  ZALLOCATED(mterm->_name);
  mterm->_flags._io_type  = io_type_;
  mterm->_flags._sig_type = sig_type_;
  if (sig_type_ == dbSigType::CLOCK)
    master_->setSequential(1);
  master->_mterm_hash.insert(mterm);
  master->_mterm_cnt++;
  return (dbMTerm*) mterm;
}

dbMTerm* dbMTerm::getMTerm(dbMaster* master_, uint dbid_)
{
  _dbMaster* master = (_dbMaster*) master_;
  return (dbMTerm*) master->_mterm_tbl->getPtr(dbid_);
}

}  // namespace odb
