// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "dbMaster.h"

#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <string>

#include "dbBlock.h"
#include "dbBox.h"
#include "dbBoxItr.h"
#include "dbCommon.h"
#include "dbCore.h"
#include "dbDatabase.h"
#include "dbHashTable.h"
#include "dbHashTable.hpp"
#include "dbLib.h"
#include "dbMPin.h"
#include "dbMPinItr.h"
#include "dbMTerm.h"
#include "dbMasterEdgeType.h"
#include "dbPolygon.h"
#include "dbPolygonItr.h"
#include "dbSite.h"
#include "dbTable.h"
#include "dbTechLayerAntennaRule.h"
#include "odb/db.h"
#include "odb/dbObject.h"
#include "odb/dbSet.h"
#include "odb/dbTransform.h"
#include "odb/dbTypes.h"
#include "odb/geom.h"
#include "utl/Logger.h"

namespace odb {

class _dbInstHdr;

template class dbHashTable<_dbMTerm>;
template class dbTable<_dbMaster>;

bool _dbMaster::operator==(const _dbMaster& rhs) const
{
  if (flags_.frozen != rhs.flags_.frozen) {
    return false;
  }

  if (flags_.x_symmetry != rhs.flags_.x_symmetry) {
    return false;
  }

  if (flags_.y_symmetry != rhs.flags_.y_symmetry) {
    return false;
  }

  if (flags_.R90_symmetry != rhs.flags_.R90_symmetry) {
    return false;
  }

  if (flags_.type != rhs.flags_.type) {
    return false;
  }

  if (x_ != rhs.x_) {
    return false;
  }

  if (y_ != rhs.y_) {
    return false;
  }

  if (height_ != rhs.height_) {
    return false;
  }

  if (width_ != rhs.width_) {
    return false;
  }

  if (mterm_cnt_ != rhs.mterm_cnt_) {
    return false;
  }

  if (id_ != rhs.id_) {
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

  if (leq_ != rhs.leq_) {
    return false;
  }

  if (eeq_ != rhs.eeq_) {
    return false;
  }

  if (obstructions_ != rhs.obstructions_) {
    return false;
  }

  if (poly_obstructions_ != rhs.poly_obstructions_) {
    return false;
  }

  if (lib_for_site_ != rhs.lib_for_site_) {
    return false;
  }

  if (site_ != rhs.site_) {
    return false;
  }

  if (mterm_hash_ != rhs.mterm_hash_) {
    return false;
  }

  if (*mterm_tbl_ != *rhs.mterm_tbl_) {
    return false;
  }

  if (*mpin_tbl_ != *rhs.mpin_tbl_) {
    return false;
  }

  if (*box_tbl_ != *rhs.box_tbl_) {
    return false;
  }

  if (*poly_box_tbl_ != *rhs.poly_box_tbl_) {
    return false;
  }

  if (*antenna_pin_model_tbl_ != *rhs.antenna_pin_model_tbl_) {
    return false;
  }

  if (*edge_types_tbl_ != *rhs.edge_types_tbl_) {
    return false;
  }

  return true;
}

////////////////////////////////////////////////////////////////////
//
// _dbMaster - Methods
//
////////////////////////////////////////////////////////////////////
_dbMaster::_dbMaster(_dbDatabase* db)
{
  flags_.x_symmetry = 0;
  flags_.y_symmetry = 0;
  flags_.R90_symmetry = 0;
  flags_.type = dbMasterType::CORE;
  flags_.frozen = 0;
  flags_.mark = 0;
  flags_.special_power = 0;
  flags_.sequential = 0;
  flags_.spare_bits_19 = 0;

  x_ = 0;
  y_ = 0;
  height_ = 0;
  width_ = 0;
  mterm_cnt_ = 0;
  id_ = 0;
  name_ = nullptr;

  mterm_tbl_ = new dbTable<_dbMTerm, 4>(
      db, this, (GetObjTbl_t) &_dbMaster::getObjectTable, dbMTermObj);

  mpin_tbl_ = new dbTable<_dbMPin, 4>(
      db, this, (GetObjTbl_t) &_dbMaster::getObjectTable, dbMPinObj);

  box_tbl_ = new dbTable<_dbBox, 8>(
      db, this, (GetObjTbl_t) &_dbMaster::getObjectTable, dbBoxObj);

  poly_box_tbl_ = new dbTable<_dbPolygon, 8>(
      db, this, (GetObjTbl_t) &_dbMaster::getObjectTable, dbPolygonObj);

  antenna_pin_model_tbl_ = new dbTable<_dbTechAntennaPinModel, 8>(
      db,
      this,
      (GetObjTbl_t) &_dbMaster::getObjectTable,
      dbTechAntennaPinModelObj);
  edge_types_tbl_ = new dbTable<_dbMasterEdgeType, 8>(
      db, this, (GetObjTbl_t) &_dbMaster::getObjectTable, dbMasterEdgeTypeObj);

  box_itr_ = new dbBoxItr<8>(box_tbl_, poly_box_tbl_, true);

  pbox_itr_ = new dbPolygonItr(poly_box_tbl_);

  pbox_box_itr_ = new dbBoxItr<8>(box_tbl_, poly_box_tbl_, false);

  mpin_itr_ = new dbMPinItr(mpin_tbl_);

  mterm_hash_.setTable(mterm_tbl_);

  sta_cell_ = nullptr;
}

_dbMaster::~_dbMaster()
{
  delete mterm_tbl_;
  delete mpin_tbl_;
  delete box_tbl_;
  delete poly_box_tbl_;
  delete antenna_pin_model_tbl_;
  delete edge_types_tbl_;
  delete box_itr_;
  delete pbox_itr_;
  delete pbox_box_itr_;
  delete mpin_itr_;

  if (name_) {
    free((void*) name_);
  }
}

dbOStream& operator<<(dbOStream& stream, const _dbMaster& master)
{
  uint32_t* bit_field = (uint32_t*) &master.flags_;
  stream << *bit_field;
  stream << master.x_;
  stream << master.y_;
  stream << master.height_;
  stream << master.width_;
  stream << master.mterm_cnt_;
  stream << master.id_;
  stream << master.name_;
  stream << master.next_entry_;
  stream << master.leq_;
  stream << master.eeq_;
  stream << master.obstructions_;
  stream << master.poly_obstructions_;
  stream << master.lib_for_site_;
  stream << master.site_;
  stream << master.mterm_hash_;
  stream << *master.mterm_tbl_;
  stream << *master.mpin_tbl_;
  stream << *master.box_tbl_;
  stream << *master.poly_box_tbl_;
  stream << *master.antenna_pin_model_tbl_;
  stream << *master.edge_types_tbl_;
  return stream;
}

dbIStream& operator>>(dbIStream& stream, _dbMaster& master)
{
  _dbDatabase* db = master.getImpl()->getDatabase();
  uint32_t* bit_field = (uint32_t*) &master.flags_;
  stream >> *bit_field;
  stream >> master.x_;
  stream >> master.y_;
  stream >> master.height_;
  stream >> master.width_;
  stream >> master.mterm_cnt_;
  stream >> master.id_;
  stream >> master.name_;
  stream >> master.next_entry_;
  stream >> master.leq_;
  stream >> master.eeq_;
  stream >> master.obstructions_;
  if (db->isSchema(kSchemaPolygon)) {
    stream >> master.poly_obstructions_;
  }
  if (db->isSchema(kSchemaDbmasterLibForSite)) {
    stream >> master.lib_for_site_;
  } else {
    // The site was copied into the same dbLib previously
    master.lib_for_site_ = master.getOwner()->getId();
  }
  stream >> master.site_;
  stream >> master.mterm_hash_;
  stream >> *master.mterm_tbl_;
  stream >> *master.mpin_tbl_;
  if (!db->isSchema(kSchemaRmTarget)) {
    // obsolete table is always unpopulated so type/values unimportant
    dbTable<_dbMaster, 4> dummy(nullptr, nullptr, nullptr, dbDatabaseObj);
    stream >> dummy;
  }
  stream >> *master.box_tbl_;
  if (db->isSchema(kSchemaPolygon)) {
    stream >> *master.poly_box_tbl_;
  }
  stream >> *master.antenna_pin_model_tbl_;
  if (db->isSchema(kSchemaMasterEdgeType)) {
    stream >> *master.edge_types_tbl_;
  }
  return stream;
}

dbObjectTable* _dbMaster::getObjectTable(dbObjectType type)
{
  switch (type) {
    case dbMTermObj:
      return mterm_tbl_;
    case dbMPinObj:
      return mpin_tbl_;
    case dbBoxObj:
      return box_tbl_;
    case dbPolygonObj:
      return poly_box_tbl_;
    case dbTechAntennaPinModelObj:
      return antenna_pin_model_tbl_;
    case dbMasterEdgeTypeObj:
      return edge_types_tbl_;
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

std::string dbMaster::getName() const
{
  const _dbMaster* master = (const _dbMaster*) this;
  return master->name_;
}

const char* dbMaster::getConstName() const
{
  const _dbMaster* master = (const _dbMaster*) this;
  return master->name_;
}

Point dbMaster::getOrigin()
{
  _dbMaster* master = (_dbMaster*) this;
  return {master->x_, master->y_};
}

void dbMaster::setOrigin(int x, int y)
{
  _dbMaster* master = (_dbMaster*) this;
  master->x_ = x;
  master->y_ = y;
}

void* dbMaster::staCell()
{
  _dbMaster* master = (_dbMaster*) this;
  return master->sta_cell_;
}

void dbMaster::staSetCell(void* cell)
{
  _dbMaster* master = (_dbMaster*) this;
  master->sta_cell_ = cell;
}

uint32_t dbMaster::getWidth() const
{
  _dbMaster* master = (_dbMaster*) this;
  return master->width_;
}

void dbMaster::setWidth(uint32_t w)
{
  _dbMaster* master = (_dbMaster*) this;
  master->width_ = w;
}

uint32_t dbMaster::getHeight() const
{
  _dbMaster* master = (_dbMaster*) this;
  return master->height_;
}

void dbMaster::setHeight(uint32_t h)
{
  _dbMaster* master = (_dbMaster*) this;
  master->height_ = h;
}

int64_t dbMaster::getArea() const
{
  return getWidth() * static_cast<int64_t>(getHeight());
}

dbMasterType dbMaster::getType() const
{
  _dbMaster* master = (_dbMaster*) this;
  return dbMasterType(master->flags_.type);
}

void dbMaster::setType(dbMasterType type)
{
  _dbMaster* master = (_dbMaster*) this;
  master->flags_.type = type.getValue();
}

dbMaster* dbMaster::getLEQ()
{
  _dbMaster* master = (_dbMaster*) this;

  if (master->leq_ == 0) {
    return nullptr;
  }

  _dbLib* lib = (_dbLib*) master->getOwner();
  return (dbMaster*) lib->master_tbl_->getPtr(master->leq_);
}

void dbMaster::setLEQ(dbMaster* leq)
{
  _dbMaster* master = (_dbMaster*) this;
  master->leq_ = leq->getImpl()->getOID();
}

dbMaster* dbMaster::getEEQ()
{
  _dbMaster* master = (_dbMaster*) this;

  if (master->eeq_ == 0) {
    return nullptr;
  }

  _dbLib* lib = (_dbLib*) master->getOwner();
  return (dbMaster*) lib->master_tbl_->getPtr(master->eeq_);
}

void dbMaster::setEEQ(dbMaster* eeq)
{
  _dbMaster* master = (_dbMaster*) this;
  master->eeq_ = eeq->getImpl()->getOID();
}

dbSet<dbMTerm> dbMaster::getMTerms()
{
  _dbMaster* master = (_dbMaster*) this;
  return dbSet<dbMTerm>(master, master->mterm_tbl_);
}

dbSet<dbMasterEdgeType> dbMaster::getEdgeTypes()
{
  _dbMaster* master = (_dbMaster*) this;
  return dbSet<dbMasterEdgeType>(master, master->edge_types_tbl_);
}

void dbMaster::clearPinAccess(int pin_access_idx)
{
  for (auto mterm : getMTerms()) {
    for (auto pin : mterm->getMPins()) {
      pin->clearPinAccess(pin_access_idx);
    }
  }
}

dbMTerm* dbMaster::findMTerm(const char* name)
{
  _dbMaster* master = (_dbMaster*) this;
  return (dbMTerm*) master->mterm_hash_.find(name);
}

dbMTerm* dbMaster::findMTerm(dbBlock* block, const char* name)
{
  dbMTerm* mterm = findMTerm(name);
  if (mterm) {
    return mterm;
  }
  char blk_left_bus_del, blk_right_bus_del;
  block->getBusDelimiters(blk_left_bus_del, blk_right_bus_del);

  char lib_left_bus_del, lib_right_bus_del;
  ;
  getLib()->getBusDelimiters(lib_left_bus_del, lib_right_bus_del);

  if (lib_left_bus_del == '\0' || lib_right_bus_del == '\0') {
    return mterm;
  }

  uint32_t ii = 0;
  std::string ttname(name);
  if (lib_left_bus_del != blk_left_bus_del
      || lib_right_bus_del != blk_right_bus_del) {
    while (name[ii] != '\0') {
      if (name[ii] == blk_left_bus_del) {
        ttname[ii] = lib_left_bus_del;
      } else if (name[ii] == blk_right_bus_del) {
        ttname[ii] = lib_right_bus_del;
      }
      ii++;
    }
    mterm = findMTerm(ttname.c_str());
  }
  return mterm;
}

dbLib* dbMaster::getLib()
{
  return (dbLib*) getImpl()->getOwner();
}

dbSet<dbBox> dbMaster::getObstructions(bool include_decomposed_polygons)
{
  _dbMaster* master = (_dbMaster*) this;
  if (include_decomposed_polygons) {
    return dbSet<dbBox>(master, master->box_itr_);
  }
  return dbSet<dbBox>(master, master->pbox_box_itr_);
}

dbSet<dbPolygon> dbMaster::getPolygonObstructions()
{
  _dbMaster* master = (_dbMaster*) this;
  return dbSet<dbPolygon>(master, master->pbox_itr_);
}

bool dbMaster::isFrozen()
{
  _dbMaster* master = (_dbMaster*) this;
  return master->flags_.frozen != 0;
}

int dbMaster::getMTermCount()
{
  _dbMaster* master = (_dbMaster*) this;
  return master->mterm_cnt_;
}

void dbMaster::setSite(dbSite* site)
{
  _dbMaster* master = (_dbMaster*) this;
  master->lib_for_site_ = site->getLib()->getImpl()->getOID();
  master->site_ = site->getImpl()->getOID();
}

dbSite* dbMaster::getSite()
{
  _dbMaster* master = (_dbMaster*) this;

  if (master->site_ == 0) {
    return nullptr;
  }

  _dbDatabase* db = (_dbDatabase*) getDb();
  _dbLib* lib = (_dbLib*) db->lib_tbl_->getPtr(master->lib_for_site_);

  return (dbSite*) lib->site_tbl_->getPtr(master->site_);
}

void dbMaster::setSymmetryX()
{
  _dbMaster* master = (_dbMaster*) this;
  master->flags_.x_symmetry = 1;
}

bool dbMaster::getSymmetryX()
{
  _dbMaster* master = (_dbMaster*) this;
  return master->flags_.x_symmetry != 0;
}

void dbMaster::setSymmetryY()
{
  _dbMaster* master = (_dbMaster*) this;
  master->flags_.y_symmetry = 1;
}

bool dbMaster::getSymmetryY()
{
  _dbMaster* master = (_dbMaster*) this;
  return master->flags_.y_symmetry != 0;
}

void dbMaster::setSymmetryR90()
{
  _dbMaster* master = (_dbMaster*) this;
  master->flags_.R90_symmetry = 1;
}

bool dbMaster::getSymmetryR90()
{
  _dbMaster* master = (_dbMaster*) this;
  return master->flags_.R90_symmetry != 0;
}

void dbMaster::setFrozen()
{
  _dbMaster* master = (_dbMaster*) this;

  if (master->flags_.frozen == 1) {
    return;
  }

  master->flags_.frozen = 1;

  // set the order id on the mterm.
  // this id is used to index mterms on a inst-hdr
  int i = 0;
  for (dbMTerm* mterm : getMTerms()) {
    ((_dbMTerm*) mterm)->order_id_ = i++;
  }
}

void dbMaster::setSequential(bool v)
{
  _dbMaster* master = (_dbMaster*) this;
  master->flags_.sequential = v;
}

bool dbMaster::isSequential()
{
  _dbMaster* master = (_dbMaster*) this;
  return master->flags_.sequential;
}
void dbMaster::setMark(uint32_t mark)
{
  _dbMaster* master = (_dbMaster*) this;
  master->flags_.mark = mark;
}

uint32_t dbMaster::isMarked()
{
  _dbMaster* master = (_dbMaster*) this;
  return master->flags_.mark;
}

void dbMaster::setSpecialPower(bool value)
{
  _dbMaster* master = (_dbMaster*) this;
  master->flags_.special_power = (value == true) ? 1 : 0;
}

bool dbMaster::isSpecialPower()
{
  _dbMaster* master = (_dbMaster*) this;
  return master->flags_.special_power == 1;
}

void dbMaster::getPlacementBoundary(Rect& r)
{
  _dbMaster* master = (_dbMaster*) this;
  r = Rect(0, 0, master->width_, master->height_);
  dbTransform t(Point(-master->x_, -master->y_));
  t.apply(r);
}

void dbMaster::transform(dbTransform& t)
{
  for (dbBox* box : getObstructions()) {
    t.apply(((_dbBox*) box)->shape_.rect);
  }

  for (dbMTerm* mterm : getMTerms()) {
    for (dbMPin* mpin : mterm->getMPins()) {
      for (dbBox* box : mpin->getGeometry()) {
        t.apply(((_dbBox*) box)->shape_.rect);
      }
    }
  }
}

int dbMaster::getMasterId()
{
  _dbMaster* master = (_dbMaster*) this;
  return master->id_;
}

dbMaster* dbMaster::create(dbLib* lib_, const char* name_)
{
  if (lib_->findMaster(name_)) {
    return nullptr;
  }

  _dbLib* lib = (_dbLib*) lib_;
  _dbDatabase* db = lib->getDatabase();
  _dbMaster* master = lib->master_tbl_->create();
  master->name_ = safe_strdup(name_);
  master->id_ = db->master_id_++;
  lib->master_hash_.insert(master);
  return (dbMaster*) master;
}

void dbMaster::destroy(dbMaster* master)
{
  auto db = master->getDb();
  _dbMaster* master_impl = (_dbMaster*) master;
  if (db->getChip() && db->getChip()->getBlock()) {
    _dbBlock* block = (_dbBlock*) db->getChip()->getBlock();
    _dbInstHdr* inst_hdr = block->inst_hdr_hash_.find(master_impl->id_);
    if (inst_hdr) {
      master->getImpl()->getLogger()->error(
          utl::ODB,
          431,
          "Can't delete master {} which still has instances",
          master->getName());
    }
  }

  _dbLib* lib = (_dbLib*) master->getLib();
  lib->master_hash_.remove(master_impl);
  lib->master_tbl_->destroy(master_impl);
}

dbMaster* dbMaster::getMaster(dbLib* lib_, uint32_t dbid_)
{
  _dbLib* lib = (_dbLib*) lib_;
  return (dbMaster*) lib->master_tbl_->getPtr(dbid_);
}

bool dbMaster::isFiller()
{
  _dbMaster* master = (_dbMaster*) this;

  switch (master->flags_.type) {
    case dbMasterType::CORE_SPACER:
      return true;
    case dbMasterType::CORE:
    case dbMasterType::CORE_FEEDTHRU:
    case dbMasterType::CORE_TIEHIGH:
    case dbMasterType::CORE_TIELOW:
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
    case dbMasterType::ENDCAP_LEF58_BOTTOMEDGE:
    case dbMasterType::ENDCAP_LEF58_TOPEDGE:
    case dbMasterType::ENDCAP_LEF58_RIGHTEDGE:
    case dbMasterType::ENDCAP_LEF58_LEFTEDGE:
    case dbMasterType::ENDCAP_LEF58_RIGHTBOTTOMEDGE:
    case dbMasterType::ENDCAP_LEF58_LEFTBOTTOMEDGE:
    case dbMasterType::ENDCAP_LEF58_RIGHTTOPEDGE:
    case dbMasterType::ENDCAP_LEF58_LEFTTOPEDGE:
    case dbMasterType::ENDCAP_LEF58_RIGHTBOTTOMCORNER:
    case dbMasterType::ENDCAP_LEF58_LEFTBOTTOMCORNER:
    case dbMasterType::ENDCAP_LEF58_RIGHTTOPCORNER:
    case dbMasterType::ENDCAP_LEF58_LEFTTOPCORNER:
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
      return false;
  }
  // gcc warning
  return false;
}

bool dbMaster::isCoreAutoPlaceable()
{
  // Use switch so if new types are added we get a compiler warning.
  switch (getType().getValue()) {
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
    case dbMasterType::ENDCAP_LEF58_BOTTOMEDGE:
    case dbMasterType::ENDCAP_LEF58_TOPEDGE:
    case dbMasterType::ENDCAP_LEF58_RIGHTEDGE:
    case dbMasterType::ENDCAP_LEF58_LEFTEDGE:
    case dbMasterType::ENDCAP_LEF58_RIGHTBOTTOMEDGE:
    case dbMasterType::ENDCAP_LEF58_LEFTBOTTOMEDGE:
    case dbMasterType::ENDCAP_LEF58_RIGHTTOPEDGE:
    case dbMasterType::ENDCAP_LEF58_LEFTTOPEDGE:
    case dbMasterType::ENDCAP_LEF58_RIGHTBOTTOMCORNER:
    case dbMasterType::ENDCAP_LEF58_LEFTBOTTOMCORNER:
    case dbMasterType::ENDCAP_LEF58_RIGHTTOPCORNER:
    case dbMasterType::ENDCAP_LEF58_LEFTTOPCORNER:
      return true;
      // These classes are completely ignored by the placer.
    case dbMasterType::ENDCAP_TOPLEFT:
    case dbMasterType::ENDCAP_TOPRIGHT:
    case dbMasterType::ENDCAP_BOTTOMLEFT:
    case dbMasterType::ENDCAP_BOTTOMRIGHT:
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
      return false;
  }
  // gcc warning
  return false;
}

void _dbMaster::collectMemInfo(MemInfo& info)
{
  info.cnt++;
  info.size += sizeof(*this);

  info.children["name"].add(name_);
  info.children["mterm_hash"].add(mterm_hash_);
  mterm_tbl_->collectMemInfo(info.children["mterm"]);
  mpin_tbl_->collectMemInfo(info.children["mpin"]);
  box_tbl_->collectMemInfo(info.children["box"]);
  poly_box_tbl_->collectMemInfo(info.children["poly_box"]);
  antenna_pin_model_tbl_->collectMemInfo(info.children["antenna_pin_model"]);
  edge_types_tbl_->collectMemInfo(info.children["edge_types"]);
}

}  // namespace odb
