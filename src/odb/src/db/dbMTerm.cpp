// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "dbMTerm.h"

#include <string.h>  // NOLINT(modernize-deprecated-headers): for strdup()

#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <string>
#include <utility>
#include <vector>

#include "dbCore.h"
#include "dbDatabase.h"
#include "dbHashTable.hpp"
#include "dbLib.h"
#include "dbMPinItr.h"
#include "dbMaster.h"
#include "dbTable.h"
#include "dbTechLayerAntennaRule.h"
#include "odb/db.h"
#include "odb/dbSet.h"
#include "odb/dbTypes.h"
#include "odb/geom.h"
#include "odb/lefout.h"
#include "spdlog/fmt/ostr.h"

namespace odb {

template class dbTable<_dbMTerm>;

bool _dbMTerm::operator==(const _dbMTerm& rhs) const
{
  if (flags_.io_type != rhs.flags_.io_type) {
    return false;
  }

  if (flags_.sig_type != rhs.flags_.sig_type) {
    return false;
  }

  if (flags_.shape_type != rhs.flags_.shape_type) {
    return false;
  }

  if (order_id_ != rhs.order_id_) {
    return false;
  }

  if (name_ && rhs.name_) {
    if (strcmp(name_, rhs.name_) != 0) {
      return false;
    }
  } else if (name_ || rhs.name_) {
    return false;
  }

  if (next_entry_ != rhs.next_entry_) {
    return false;
  }

  if (next_mterm_ != rhs.next_mterm_) {
    return false;
  }

  if (pins_ != rhs.pins_) {
    return false;
  }

  if (targets_ != rhs.targets_) {
    return false;
  }

  if (oxide1_ != rhs.oxide1_) {
    return false;
  }

  if (oxide2_ != rhs.oxide2_) {
    return false;
  }

  if (par_met_area_ != rhs.par_met_area_) {
    return false;
  }

  if (par_met_sidearea_ != rhs.par_met_sidearea_) {
    return false;
  }

  if (par_cut_area_ != rhs.par_cut_area_) {
    return false;
  }

  if (diffarea_ != rhs.diffarea_) {
    return false;
  }

  return true;
}

////////////////////////////////////////////////////////////////////
//
// _dbMTerm - Methods
//
////////////////////////////////////////////////////////////////////

_dbMTerm::~_dbMTerm()
{
  if (name_) {
    free((void*) name_);
  }

  for (auto elem : par_met_area_) {
    delete elem;
  }
  par_met_area_.clear();

  for (auto elem : par_met_sidearea_) {
    delete elem;
  }
  par_met_sidearea_.clear();

  for (auto elem : par_cut_area_) {
    delete elem;
  }
  par_cut_area_.clear();

  for (auto elem : diffarea_) {
    delete elem;
  }
  diffarea_.clear();
}

dbOStream& operator<<(dbOStream& stream, const _dbMTerm& mterm)
{
  uint32_t* bit_field = (uint32_t*) &mterm.flags_;
  stream << *bit_field;
  stream << mterm.order_id_;
  stream << mterm.name_;
  stream << mterm.next_entry_;
  stream << mterm.next_mterm_;
  stream << mterm.pins_;
  stream << mterm.targets_;
  stream << mterm.oxide1_;
  stream << mterm.oxide2_;
  stream << mterm.par_met_area_;
  stream << mterm.par_met_sidearea_;
  stream << mterm.par_cut_area_;
  stream << mterm.diffarea_;
  return stream;
}

dbIStream& operator>>(dbIStream& stream, _dbMTerm& mterm)
{
  uint32_t* bit_field = (uint32_t*) &mterm.flags_;
  stream >> *bit_field;
  stream >> mterm.order_id_;
  stream >> mterm.name_;
  stream >> mterm.next_entry_;
  stream >> mterm.next_mterm_;
  stream >> mterm.pins_;
  stream >> mterm.targets_;
  stream >> mterm.oxide1_;
  stream >> mterm.oxide2_;
  stream >> mterm.par_met_area_;
  stream >> mterm.par_met_sidearea_;
  stream >> mterm.par_cut_area_;
  stream >> mterm.diffarea_;
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
  return mterm->name_;
}

char* dbMTerm::getName(dbInst* inst, char* ttname)
{
  dbBlock* block = (dbBlock*) inst->getImpl()->getOwner();
  dbMaster* master = inst->getMaster();
  return (getName(block, master, ttname));
}

char* dbMTerm::getName(dbBlock* block, dbMaster* master, char* ttname)
{
  char* mtname = (char*) getConstName();
  char blk_left_bus_del, blk_right_bus_del, lib_left_bus_del, lib_right_bus_del;
  uint32_t ii = 0;
  block->getBusDelimiters(blk_left_bus_del, blk_right_bus_del);
  if (blk_left_bus_del == '\0' || blk_right_bus_del == '\0') {
    return mtname;
  }
  master->getLib()->getBusDelimiters(lib_left_bus_del, lib_right_bus_del);

  if (lib_left_bus_del != blk_left_bus_del
      || lib_right_bus_del != blk_right_bus_del) {
    while (mtname[ii] != '\0') {
      if (mtname[ii] == lib_left_bus_del) {
        ttname[ii] = blk_left_bus_del;
      } else if (mtname[ii] == lib_right_bus_del) {
        ttname[ii] = blk_right_bus_del;
      } else {
        ttname[ii] = mtname[ii];
      }
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
  return mterm->name_;
}

dbSigType dbMTerm::getSigType()
{
  _dbMTerm* mterm = (_dbMTerm*) this;
  return dbSigType(mterm->flags_.sig_type);
}

dbIoType dbMTerm::getIoType()
{
  _dbMTerm* mterm = (_dbMTerm*) this;
  return dbIoType(mterm->flags_.io_type);
}

dbMTermShapeType dbMTerm::getShape()
{
  _dbMTerm* mterm = (_dbMTerm*) this;
  return dbMTermShapeType(mterm->flags_.shape_type);
}

void dbMTerm::setMark(uint32_t v)
{
  _dbMTerm* mterm = (_dbMTerm*) this;
  mterm->flags_.mark = v;
}
bool dbMTerm::isSetMark()
{
  _dbMTerm* mterm = (_dbMTerm*) this;
  return mterm->flags_.mark > 0 ? true : false;
}

dbMaster* dbMTerm::getMaster()
{
  return (dbMaster*) getImpl()->getOwner();
}

dbSet<dbMPin> dbMTerm::getMPins()
{
  _dbMTerm* mterm = (_dbMTerm*) this;
  _dbMaster* master = (_dbMaster*) mterm->getOwner();
  return dbSet<dbMPin>(mterm, master->mpin_itr_);
}

Rect dbMTerm::getBBox()
{
  Rect bbox;
  bbox.mergeInit();
  for (dbMPin* pin : getMPins()) {
    bbox.merge(pin->getBBox());
  }
  return bbox;
}

void* dbMTerm::staPort()
{
  _dbMTerm* mterm = (_dbMTerm*) this;
  return mterm->sta_port_;
}

void dbMTerm::staSetPort(void* port)
{
  _dbMTerm* mterm = (_dbMTerm*) this;
  mterm->sta_port_ = port;
}

int dbMTerm::getIndex()
{
  _dbMTerm* mterm = (_dbMTerm*) this;
  return mterm->order_id_;
}

//
// Add entries for antenna info elements that are not model specific
//
void dbMTerm::addPartialMetalAreaEntry(double inval, dbTechLayer* refly)
{
  _dbMTerm* mterm = (_dbMTerm*) this;
  _dbTechAntennaAreaElement::create(mterm->par_met_area_, inval, refly);
}

void dbMTerm::addPartialMetalSideAreaEntry(double inval, dbTechLayer* refly)
{
  _dbMTerm* mterm = (_dbMTerm*) this;
  _dbTechAntennaAreaElement::create(mterm->par_met_sidearea_, inval, refly);
}

void dbMTerm::addPartialCutAreaEntry(double inval, dbTechLayer* refly)
{
  _dbMTerm* mterm = (_dbMTerm*) this;
  _dbTechAntennaAreaElement::create(mterm->par_cut_area_, inval, refly);
}

void dbMTerm::addDiffAreaEntry(double inval, dbTechLayer* refly)
{
  _dbMTerm* mterm = (_dbMTerm*) this;
  _dbTechAntennaAreaElement::create(mterm->diffarea_, inval, refly);
}

dbTechAntennaPinModel* dbMTerm::createDefaultAntennaModel()
{
  _dbMTerm* mterm = (_dbMTerm*) this;
  _dbTechAntennaPinModel* m
      = (_dbTechAntennaPinModel*) getDefaultAntennaModel();

  // Reinitialize the object to its default state...
  if (m != nullptr) {
    m->~_dbTechAntennaPinModel();
    new (m) _dbTechAntennaPinModel(mterm->getDatabase());
    m->mterm_ = getImpl()->getOID();
  } else {
    _dbMaster* master = (_dbMaster*) mterm->getOwner();
    m = master->antenna_pin_model_tbl_->create();
    mterm->oxide1_ = m->getOID();
    m->mterm_ = getImpl()->getOID();
  }

  return (dbTechAntennaPinModel*) m;
}

dbTechAntennaPinModel* dbMTerm::createOxide2AntennaModel()
{
  _dbMTerm* mterm = (_dbMTerm*) this;
  _dbTechAntennaPinModel* m = (_dbTechAntennaPinModel*) getOxide2AntennaModel();

  // Reinitialize the object to its default state...
  if (m != nullptr) {
    m->~_dbTechAntennaPinModel();
    new (m) _dbTechAntennaPinModel(mterm->getDatabase());
    m->mterm_ = getImpl()->getOID();
  } else {
    _dbMaster* master = (_dbMaster*) mterm->getOwner();
    m = master->antenna_pin_model_tbl_->create();
    mterm->oxide2_ = m->getOID();
    m->mterm_ = getImpl()->getOID();
  }

  return (dbTechAntennaPinModel*) m;
}

bool dbMTerm::hasDefaultAntennaModel() const
{
  _dbMTerm* mterm = (_dbMTerm*) this;
  return (mterm->oxide1_ != 0);
}

bool dbMTerm::hasOxide2AntennaModel() const
{
  _dbMTerm* mterm = (_dbMTerm*) this;
  return (mterm->oxide2_ != 0);
}

dbTechAntennaPinModel* dbMTerm::getDefaultAntennaModel() const
{
  _dbMTerm* mterm = (_dbMTerm*) this;

  if (mterm->oxide1_ == 0) {
    return nullptr;
  }

  _dbMaster* master = (_dbMaster*) mterm->getOwner();
  return (dbTechAntennaPinModel*) master->antenna_pin_model_tbl_->getPtr(
      mterm->oxide1_);
}

dbTechAntennaPinModel* dbMTerm::getOxide2AntennaModel() const
{
  _dbMTerm* mterm = (_dbMTerm*) this;

  if (mterm->oxide2_ == 0) {
    return nullptr;
  }

  _dbMaster* master = (_dbMaster*) mterm->getOwner();
  return (dbTechAntennaPinModel*) master->antenna_pin_model_tbl_->getPtr(
      mterm->oxide2_);
}

void dbMTerm::getDiffArea(std::vector<std::pair<double, dbTechLayer*>>& data)
{
  _dbMTerm* mterm = (_dbMTerm*) this;
  _dbTechAntennaPinModel::getAntennaValues(
      mterm->getDatabase(), mterm->diffarea_, data);
}

void dbMTerm::writeAntennaLef(lefout& writer) const
{
  _dbMTerm* mterm = (_dbMTerm*) this;

  dbMaster* tpmtr = (dbMaster*) mterm->getOwner();
  dbLib* tplib = (dbLib*) tpmtr->getImpl()->getOwner();
  dbTech* tech = tplib->getTech();

  for (auto ant : mterm->par_met_area_) {
    ant->writeLef("ANTENNAPARTIALMETALAREA", tech, writer);
  }

  for (auto ant : mterm->par_met_sidearea_) {
    ant->writeLef("ANTENNAPARTIALMETALSIDEAREA", tech, writer);
  }

  for (auto ant : mterm->par_cut_area_) {
    ant->writeLef("ANTENNAPARTIALCUTAREA", tech, writer);
  }

  for (auto ant : mterm->diffarea_) {
    ant->writeLef("ANTENNADIFFAREA", tech, writer);
  }

  if (hasDefaultAntennaModel()) {
    getDefaultAntennaModel()->writeLef(tech, writer);
  }

  if (hasOxide2AntennaModel()) {
    fmt::print(writer.out(), "        ANTENNAMODEL OXIDE2 ;\n");
    getOxide2AntennaModel()->writeLef(tech, writer);
  }
}

dbMTerm* dbMTerm::create(dbMaster* master,
                         const char* name,
                         dbIoType io_type,
                         dbSigType sig_type,
                         dbMTermShapeType shape_type)
{
  _dbMaster* master_impl = (_dbMaster*) master;

  if (master_impl->flags_.frozen || master_impl->mterm_hash_.hasMember(name)) {
    return nullptr;
  }

  _dbMTerm* impl = master_impl->mterm_tbl_->create();
  impl->name_ = strdup(name);
  impl->flags_.io_type = io_type.getValue();
  impl->flags_.shape_type = shape_type;
  master_impl->mterm_hash_.insert(impl);
  master_impl->mterm_cnt_++;

  dbMTerm* mterm = (dbMTerm*) impl;
  mterm->setSigType(sig_type);
  return mterm;
}

void dbMTerm::setSigType(dbSigType type)
{
  _dbMTerm* mterm = (_dbMTerm*) this;
  mterm->flags_.sig_type = type.getValue();
  if (type == dbSigType::CLOCK) {
    getMaster()->setSequential(true);
  }
}

dbMTerm* dbMTerm::getMTerm(dbMaster* master_, uint32_t dbid_)
{
  _dbMaster* master = (_dbMaster*) master_;
  return (dbMTerm*) master->mterm_tbl_->getPtr(dbid_);
}

void _dbMTerm::collectMemInfo(MemInfo& info)
{
  info.cnt++;
  info.size += sizeof(*this);

  info.children["name"].add(name_);

  // These fields have unusal pointer ownship semantics relative to
  // the rest of odb (not a table but a vector of owning pointers).
  // Should be just by value.
  info.children["_par_met_area"].add(par_met_area_);
  info.children["_par_met_area"].size
      += par_met_area_.size() * sizeof(_dbTechAntennaAreaElement);
  info.children["_par_met_sidearea"].add(par_met_sidearea_);
  info.children["_par_met_sidearea"].size
      += par_met_sidearea_.size() * sizeof(_dbTechAntennaAreaElement);
  info.children["_par_cut_area"].add(par_cut_area_);
  info.children["_par_cut_area"].size
      += par_cut_area_.size() * sizeof(_dbTechAntennaAreaElement);
  info.children["_diffarea"].add(diffarea_);
  info.children["_diffarea"].size
      += diffarea_.size() * sizeof(_dbTechAntennaAreaElement);
}

}  // namespace odb
