// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "dbInst.h"

#include <algorithm>
#include <string>
#include <vector>

#include "dbAccessPoint.h"
#include "dbArrayTable.h"
#include "dbArrayTable.hpp"
#include "dbBTerm.h"
#include "dbBlock.h"
#include "dbBox.h"
#include "dbChip.h"
#include "dbCommon.h"
#include "dbDatabase.h"
#include "dbGroup.h"
#include "dbHier.h"
#include "dbITerm.h"
#include "dbITermItr.h"
#include "dbInstHdr.h"
#include "dbJournal.h"
#include "dbLib.h"
#include "dbMTerm.h"
#include "dbMaster.h"
#include "dbModule.h"
#include "dbNet.h"
#include "dbNullIterator.h"
#include "dbRegion.h"
#include "dbTable.h"
#include "dbTable.hpp"
#include "odb/db.h"
#include "odb/dbBlockCallBackObj.h"
#include "odb/dbSet.h"
#include "odb/dbTransform.h"
#include "utl/Logger.h"

namespace odb {

template class dbTable<_dbInst>;

class sortMTerm
{
 public:
  bool operator()(_dbMTerm* m1, _dbMTerm* m2)
  {
    return strcmp(m1->_name, m2->_name) < 0;
  }
};

class sortITerm
{
  _dbBlock* _block;

 public:
  sortITerm(_dbBlock* block) { _block = block; }

  bool operator()(uint it1, uint it2)
  {
    _dbITerm* iterm1 = _block->_iterm_tbl->getPtr(it1);
    _dbITerm* iterm2 = _block->_iterm_tbl->getPtr(it2);
    return iterm1->_flags._mterm_idx < iterm2->_flags._mterm_idx;
  }
};

void _dbInst::setInstBBox(_dbInst* inst)
{
  _dbBlock* block = (_dbBlock*) inst->getOwner();
  _dbBox* box = block->_box_tbl->getPtr(inst->_bbox);
  block->remove_rect(box->_shape._rect);

  dbMaster* master = ((dbInst*) inst)->getMaster();
  master->getPlacementBoundary(box->_shape._rect);
  dbTransform transform(inst->_flags._orient, Point(inst->_x, inst->_y));
  transform.apply(box->_shape._rect);
  block->add_rect(box->_shape._rect);
}

_dbInst::_dbInst(_dbDatabase*)
{
  _flags._orient = dbOrientType::R0;
  _flags._status = dbPlacementStatus::NONE;
  _flags._user_flag_1 = 0;
  _flags._user_flag_2 = 0;
  _flags._user_flag_3 = 0;
  _flags._physical_only = 0;
  _flags._dont_touch = 0;
  _flags._eco_create = 0;
  _flags._eco_destroy = 0;
  _flags._eco_modify = 0;
  _flags._source = dbSourceType::NONE;
  //_flags._spare_bits = 0;
  _flags._level = 0;
  _name = nullptr;
  _x = 0;
  _y = 0;
  _weight = 0;
  pin_access_idx_ = -1;
}

_dbInst::_dbInst(_dbDatabase*, const _dbInst& i)
    : _flags(i._flags),
      _name(nullptr),
      _x(i._x),
      _y(i._y),
      _weight(i._weight),
      _next_entry(i._next_entry),
      _inst_hdr(i._inst_hdr),
      _bbox(i._bbox),
      _region(i._region),
      _module(i._module),
      _group(i._group),
      _region_next(i._region_next),
      _module_next(i._module_next),
      _group_next(i._group_next),
      _region_prev(i._region_prev),
      _module_prev(i._module_prev),
      _hierarchy(i._hierarchy),
      _iterms(i._iterms),
      _halo(i._halo),
      pin_access_idx_(i.pin_access_idx_)
{
  if (i._name) {
    _name = strdup(i._name);
    ZALLOCATED(_name);
  }
}

_dbInst::~_dbInst()
{
  if (_name) {
    free((void*) _name);
  }
}

dbOStream& operator<<(dbOStream& stream, const _dbInst& inst)
{
  uint* bit_field = (uint*) &inst._flags;
  stream << *bit_field;
  stream << inst._name;
  stream << inst._x;
  stream << inst._y;
  stream << inst._weight;
  stream << inst._next_entry;
  stream << inst._inst_hdr;
  stream << inst._bbox;
  stream << inst._region;
  stream << inst._module;
  stream << inst._group;
  stream << inst._region_next;
  stream << inst._module_next;
  stream << inst._group_next;
  stream << inst._region_prev;
  stream << inst._module_prev;
  stream << inst._hierarchy;
  stream << inst._iterms;
  stream << inst._halo;
  stream << inst.pin_access_idx_;
  return stream;
}

dbIStream& operator>>(dbIStream& stream, _dbInst& inst)
{
  uint* bit_field = (uint*) &inst._flags;
  stream >> *bit_field;
  stream >> inst._name;
  stream >> inst._x;
  stream >> inst._y;
  stream >> inst._weight;
  stream >> inst._next_entry;
  stream >> inst._inst_hdr;
  stream >> inst._bbox;
  stream >> inst._region;
  stream >> inst._module;
  stream >> inst._group;
  stream >> inst._region_next;
  stream >> inst._module_next;
  stream >> inst._group_next;
  stream >> inst._region_prev;
  stream >> inst._module_prev;
  stream >> inst._hierarchy;
  stream >> inst._iterms;
  stream >> inst._halo;
  stream >> inst.pin_access_idx_;

  dbDatabase* db = (dbDatabase*) (inst.getDatabase());
  if (((_dbDatabase*) db)->isSchema(db_schema_db_remove_hash)) {
    _dbBlock* block = (_dbBlock*) (db->getChip()->getBlock());
    _dbModule* module = nullptr;
    // if the instance has no module parent put in the top module
    // We sometimes see instances with _module set to 0 (possibly
    // introduced downstream) so we stick them in the hash for the
    // top module.
    if (inst._module == 0) {
      module = (_dbModule*) (((dbBlock*) block)->getTopModule());
    } else {
      module = block->_module_tbl->getPtr(inst._module);
    }
    if (inst._name) {
      module->_dbinst_hash[inst._name] = dbId<_dbInst>(inst.getId());
    }
  }
  return stream;
}

bool _dbInst::operator<(const _dbInst& rhs) const
{
  return strcmp(_name, rhs._name) < 0;
}

bool _dbInst::operator==(const _dbInst& rhs) const
{
  if (_flags._orient != rhs._flags._orient) {
    return false;
  }

  if (_flags._status != rhs._flags._status) {
    return false;
  }

  if (_flags._user_flag_1 != rhs._flags._user_flag_1) {
    return false;
  }

  if (_flags._user_flag_2 != rhs._flags._user_flag_2) {
    return false;
  }

  if (_flags._user_flag_3 != rhs._flags._user_flag_3) {
    return false;
  }

  if (_flags._physical_only != rhs._flags._physical_only) {
    return false;
  }

  if (_flags._dont_touch != rhs._flags._dont_touch) {
    return false;
  }

  if (_flags._source != rhs._flags._source) {
    return false;
  }

  if (_name && rhs._name) {
    if (strcmp(_name, rhs._name) != 0) {
      return false;
    }
  } else if (_name || rhs._name) {
    return false;
  }

  if (_x != rhs._x) {
    return false;
  }

  if (_y != rhs._y) {
    return false;
  }

  if (_weight != rhs._weight) {
    return false;
  }

  if (_next_entry != rhs._next_entry) {
    return false;
  }

  if (_inst_hdr != rhs._inst_hdr) {
    return false;
  }

  if (_bbox != rhs._bbox) {
    return false;
  }

  if (_region != rhs._region) {
    return false;
  }

  if (_module != rhs._module) {
    return false;
  }

  if (_group != rhs._group) {
    return false;
  }

  if (_region_next != rhs._region_next) {
    return false;
  }

  if (_module_next != rhs._module_next) {
    return false;
  }

  if (_group_next != rhs._group_next) {
    return false;
  }

  if (_region_prev != rhs._region_prev) {
    return false;
  }

  if (_module_prev != rhs._module_prev) {
    return false;
  }

  if (_hierarchy != rhs._hierarchy) {
    return false;
  }

  if (_iterms != rhs._iterms) {
    return false;
  }

  if (_halo != rhs._halo) {
    return false;
  }

  if (pin_access_idx_ != rhs.pin_access_idx_) {
    return false;
  }

  return true;
}

////////////////////////////////////////////////////////////////////
//
// dbInst - Methods
//
////////////////////////////////////////////////////////////////////

std::string dbInst::getName()
{
  _dbInst* inst = (_dbInst*) this;
  return inst->_name;
}

const char* dbInst::getConstName()
{
  _dbInst* inst = (_dbInst*) this;
  return inst->_name;
}

bool dbInst::isNamed(const char* name)
{
  _dbInst* inst = (_dbInst*) this;
  if (!strcmp(inst->_name, name)) {
    return true;
  }
  return false;
}

bool dbInst::rename(const char* name)
{
  _dbInst* inst = (_dbInst*) this;
  _dbBlock* block = (_dbBlock*) inst->getOwner();

  if (block->_inst_hash.hasMember(name)) {
    return false;
  }

  block->_inst_hash.remove(inst);
  free((void*) inst->_name);
  inst->_name = strdup(name);
  ZALLOCATED(inst->_name);
  block->_inst_hash.insert(inst);

  return true;
}

Point dbInst::getOrigin()
{
  _dbInst* inst = (_dbInst*) this;
  return {inst->_x, inst->_y};
}

void dbInst::setOrigin(int x, int y)
{
  _dbInst* inst = (_dbInst*) this;
  _dbBlock* block = (_dbBlock*) inst->getOwner();
  int prev_x = inst->_x;
  int prev_y = inst->_y;
  const auto placement_status = getPlacementStatus();
  if (placement_status.isPlaced() && prev_x == x && prev_y == y) {
    return;
  }
  if (placement_status.isFixed()) {
    inst->getLogger()->error(utl::ODB,
                             359,
                             "Attempt to change the origin of {} instance {}",
                             getPlacementStatus().getString(),
                             getName());
  }

  for (auto callback : block->_callbacks) {
    callback->inDbPreMoveInst(this);
  }

  inst->_x = x;
  inst->_y = y;
  _dbInst::setInstBBox(inst);

  if (block->_journal) {
    debugPrint(getImpl()->getLogger(),
               utl::ODB,
               "DB_ECO",
               1,
               "ECO: setOrigin {}, {}",
               x,
               y);
    block->_journal->beginAction(dbJournal::UPDATE_FIELD);
    block->_journal->pushParam(inst->getObjectType());
    block->_journal->pushParam(inst->getId());
    block->_journal->pushParam(_dbInst::ORIGIN);
    block->_journal->pushParam(prev_x);
    block->_journal->pushParam(prev_y);
    block->_journal->pushParam(inst->_x);
    block->_journal->pushParam(inst->_y);
    block->_journal->endAction();
  }

  for (auto iterm_idx : inst->_iterms) {
    dbITerm* iterm = (dbITerm*) block->_iterm_tbl->getPtr(iterm_idx);
    iterm->clearPrefAccessPoints();
  }

  block->_flags._valid_bbox = 0;
  for (auto callback : block->_callbacks) {
    callback->inDbPostMoveInst(this);
  }
}

void dbInst::setLocationOrient(dbOrientType orient)
{
  int x, y;
  getLocation(x, y);
  setOrient(orient);
  setLocation(x, y);
}

void dbInst::getLocation(int& x, int& y) const
{
  const _dbInst* inst = (const _dbInst*) this;
  _dbBlock* block = (_dbBlock*) inst->getOwner();
  _dbBox* bbox = block->_box_tbl->getPtr(inst->_bbox);
  x = bbox->_shape._rect.xMin();
  y = bbox->_shape._rect.yMin();
}

Point dbInst::getLocation() const
{
  int x;
  int y;
  getLocation(x, y);
  return {x, y};
}

void dbInst::setLocation(int x, int y)
{
  dbMaster* master = getMaster();
  Rect bbox;
  master->getPlacementBoundary(bbox);
  dbTransform t(getOrient());
  t.apply(bbox);
  setOrigin(x - bbox.xMin(), y - bbox.yMin());
}

dbBox* dbInst::getBBox()
{
  _dbInst* inst = (_dbInst*) this;
  _dbBlock* block = (_dbBlock*) inst->getOwner();
  return (dbBox*) block->_box_tbl->getPtr(inst->_bbox);
}

dbOrientType dbInst::getOrient()
{
  _dbInst* inst = (_dbInst*) this;
  return dbOrientType(inst->_flags._orient);
}

void dbInst::setOrient(dbOrientType orient)
{
  if (orient == getOrient()) {
    return;
  }
  _dbInst* inst = (_dbInst*) this;
  _dbBlock* block = (_dbBlock*) inst->getOwner();

  if (getPlacementStatus().isFixed()) {
    inst->getLogger()->error(
        utl::ODB,
        360,
        "Attempt to change the orientation of {} instance {}",
        getPlacementStatus().getString(),
        getName());
  }
  for (auto callback : block->_callbacks) {
    callback->inDbPreMoveInst(this);
  }
  uint prev_flags = flagsToUInt(inst);
  inst->_flags._orient = orient.getValue();
  _dbInst::setInstBBox(inst);

  if (block->_journal) {
    debugPrint(getImpl()->getLogger(),
               utl::ODB,
               "DB_ECO",
               1,
               "ECO: setOrient {}",
               orient.getValue());
    block->_journal->updateField(
        this, _dbInst::FLAGS, prev_flags, flagsToUInt(inst));
  }

  for (auto iterm_idx : inst->_iterms) {
    dbITerm* iterm = (dbITerm*) block->_iterm_tbl->getPtr(iterm_idx);
    iterm->clearPrefAccessPoints();
  }

  block->_flags._valid_bbox = 0;
  for (auto callback : block->_callbacks) {
    callback->inDbPostMoveInst(this);
  }
}

dbPlacementStatus dbInst::getPlacementStatus()
{
  _dbInst* inst = (_dbInst*) this;
  return dbPlacementStatus(inst->_flags._status);
}

void dbInst::setPlacementStatus(dbPlacementStatus status)
{
  _dbInst* inst = (_dbInst*) this;
  _dbBlock* block = (_dbBlock*) inst->getOwner();

  if (inst->_flags._status == status) {
    return;
  }

  for (auto callback : block->_callbacks) {
    callback->inDbInstPlacementStatusBefore(this, status);
  }

  uint prev_flags = flagsToUInt(inst);
  inst->_flags._status = status.getValue();
  block->_flags._valid_bbox = 0;
  if (block->_journal) {
    debugPrint(getImpl()->getLogger(),
               utl::ODB,
               "DB_ECO",
               1,
               "ECO: setPlacementStatus {}",
               status.getValue());
    block->_journal->updateField(
        this, _dbInst::FLAGS, prev_flags, flagsToUInt(inst));
  }
}

dbTransform dbInst::getTransform()
{
  _dbInst* inst = (_dbInst*) this;
  return dbTransform(inst->_flags._orient, Point(inst->_x, inst->_y));
}

void dbInst::setTransform(const dbTransform& t)
{
  setOrient(t.getOrient());
  Point offset = t.getOffset();
  setOrigin(offset.x(), offset.y());
}

static void getParentTransform(dbInst* inst, dbTransform& t)
{
  dbInst* parent = inst->getParent();

  if (parent == nullptr) {
    t = dbTransform();
  } else {
    getParentTransform(parent, t);
    dbTransform x = parent->getTransform();
    x.concat(t);
    t = x;
  }
}

void dbInst::getHierTransform(dbTransform& t)
{
  getParentTransform(this, t);
  dbTransform x = getTransform();
  x.concat(t);
  t = x;
}

bool dbInst::getEcoCreate()
{
  _dbInst* inst = (_dbInst*) this;
  return inst->_flags._eco_create;
}
void dbInst::setEcoCreate(bool v)
{
  _dbInst* inst = (_dbInst*) this;
  // _dbBlock * block = (_dbBlock *) getOwner();
  // uint prev_flags = flagsToUInt(inst);
  if (v) {
    inst->_flags._eco_create = 1;
  } else {
    inst->_flags._eco_create = 0;
  }
}
bool dbInst::getEcoDestroy()
{
  _dbInst* inst = (_dbInst*) this;
  return inst->_flags._eco_destroy;
}
void dbInst::setEcoDestroy(bool v)
{
  _dbInst* inst = (_dbInst*) this;
  // _dbBlock * block = (_dbBlock *) getOwner();
  // uint prev_flags = flagsToUInt(inst);
  if (v) {
    inst->_flags._eco_destroy = 1;
  } else {
    inst->_flags._eco_destroy = 0;
  }
}
bool dbInst::getEcoModify()
{
  _dbInst* inst = (_dbInst*) this;
  return inst->_flags._eco_modify;
}
void dbInst::setEcoModify(bool v)
{
  _dbInst* inst = (_dbInst*) this;
  // _dbBlock * block = (_dbBlock *) getOwner();
  // uint prev_flags = flagsToUInt(inst);
  if (v) {
    inst->_flags._eco_modify = 1;
  } else {
    inst->_flags._eco_modify = 0;
  }
}
bool dbInst::getUserFlag1()
{
  _dbInst* inst = (_dbInst*) this;
  return inst->_flags._user_flag_1 == 1;
}

void dbInst::setUserFlag1()
{
  _dbInst* inst = (_dbInst*) this;
  _dbBlock* block = (_dbBlock*) inst->getOwner();
  uint prev_flags = flagsToUInt(inst);
  inst->_flags._user_flag_1 = 1;

  if (block->_journal) {
    block->_journal->updateField(
        this, _dbInst::FLAGS, prev_flags, flagsToUInt(inst));
  }
}

void dbInst::clearUserFlag1()
{
  _dbInst* inst = (_dbInst*) this;
  _dbBlock* block = (_dbBlock*) inst->getOwner();
  uint prev_flags = flagsToUInt(inst);
  inst->_flags._user_flag_1 = 0;

  if (block->_journal) {
    block->_journal->updateField(
        this, _dbInst::FLAGS, prev_flags, flagsToUInt(inst));
  }
}

bool dbInst::getUserFlag2()
{
  _dbInst* inst = (_dbInst*) this;
  return inst->_flags._user_flag_2 == 1;
}

void dbInst::setUserFlag2()
{
  _dbInst* inst = (_dbInst*) this;
  _dbBlock* block = (_dbBlock*) inst->getOwner();
  uint prev_flags = flagsToUInt(inst);
  inst->_flags._user_flag_2 = 1;

  if (block->_journal) {
    block->_journal->updateField(
        this, _dbInst::FLAGS, prev_flags, flagsToUInt(inst));
  }
}

void dbInst::clearUserFlag2()
{
  _dbInst* inst = (_dbInst*) this;
  _dbBlock* block = (_dbBlock*) inst->getOwner();
  uint prev_flags = flagsToUInt(inst);
  inst->_flags._user_flag_2 = 0;

  if (block->_journal) {
    block->_journal->updateField(
        this, _dbInst::FLAGS, prev_flags, flagsToUInt(inst));
  }
}

bool dbInst::getUserFlag3()
{
  _dbInst* inst = (_dbInst*) this;
  return inst->_flags._user_flag_3 == 1;
}

void dbInst::setUserFlag3()
{
  _dbInst* inst = (_dbInst*) this;
  _dbBlock* block = (_dbBlock*) inst->getOwner();
  uint prev_flags = flagsToUInt(inst);
  inst->_flags._user_flag_3 = 1;

  if (block->_journal) {
    block->_journal->updateField(
        this, _dbInst::FLAGS, prev_flags, flagsToUInt(inst));
  }
}

void dbInst::clearUserFlag3()
{
  _dbInst* inst = (_dbInst*) this;
  _dbBlock* block = (_dbBlock*) inst->getOwner();
  uint prev_flags = flagsToUInt(inst);
  inst->_flags._user_flag_3 = 0;

  if (block->_journal) {
    block->_journal->updateField(
        this, _dbInst::FLAGS, prev_flags, flagsToUInt(inst));
  }
}

void dbInst::setDoNotTouch(bool v)
{
  _dbInst* inst = (_dbInst*) this;
  inst->_flags._dont_touch = v;
}

bool dbInst::isDoNotTouch()
{
  _dbInst* inst = (_dbInst*) this;
  return inst->_flags._dont_touch == 1;
}

dbBlock* dbInst::getBlock()
{
  return (dbBlock*) getImpl()->getOwner();
}

dbMaster* dbInst::getMaster() const
{
  _dbInst* inst = (_dbInst*) this;
  _dbBlock* block = (_dbBlock*) inst->getOwner();
  _dbInstHdr* inst_hdr = block->_inst_hdr_tbl->getPtr(inst->_inst_hdr);
  _dbDatabase* db = inst->getDatabase();
  _dbLib* lib = db->_lib_tbl->getPtr(inst_hdr->_lib);
  return (dbMaster*) lib->_master_tbl->getPtr(inst_hdr->_master);
}

bool dbInst::isBlock() const
{
  return getMaster()->isBlock();
}

bool dbInst::isCore() const
{
  return getMaster()->isCore();
}

bool dbInst::isPad() const
{
  return getMaster()->isPad();
}

bool dbInst::isEndCap() const
{
  return getMaster()->isEndCap();
}

dbSet<dbITerm> dbInst::getITerms()
{
  _dbInst* inst = (_dbInst*) this;
  _dbBlock* block = (_dbBlock*) inst->getOwner();
  return dbSet<dbITerm>(inst, block->_inst_iterm_itr);
}

dbITerm* dbInst::findITerm(const char* name)
{
  _dbInst* inst = (_dbInst*) this;
  _dbBlock* block = (_dbBlock*) inst->getOwner();
  dbMaster* master = getMaster();
  _dbMTerm* mterm = (_dbMTerm*) master->findMTerm((dbBlock*) block, name);

  if (mterm == nullptr) {
    return nullptr;
  }

  return (dbITerm*) block->_iterm_tbl->getPtr(inst->_iterms[mterm->_order_id]);
}

dbRegion* dbInst::getRegion()
{
  _dbInst* inst = (_dbInst*) this;

  if (inst->_region == 0) {
    return nullptr;
  }

  _dbBlock* block = (_dbBlock*) inst->getOwner();
  _dbRegion* r = block->_region_tbl->getPtr(inst->_region);
  return (dbRegion*) r;
}

dbModule* dbInst::getModule()
{
  _dbInst* inst = (_dbInst*) this;

  if (inst->_module == 0) {
    return nullptr;
  }

  _dbBlock* block = (_dbBlock*) inst->getOwner();
  _dbModule* module = block->_module_tbl->getPtr(inst->_module);
  return (dbModule*) module;
}

dbGroup* dbInst::getGroup()
{
  _dbInst* inst = (_dbInst*) this;

  if (inst->_group == 0) {
    return nullptr;
  }

  _dbBlock* block = (_dbBlock*) inst->getOwner();
  _dbGroup* group = block->_group_tbl->getPtr(inst->_group);
  return (dbGroup*) group;
}

dbBox* dbInst::getHalo()
{
  _dbInst* inst = (_dbInst*) this;

  if (inst->_halo == 0) {
    return nullptr;
  }

  _dbBlock* block = (_dbBlock*) inst->getOwner();
  _dbBox* b = block->_box_tbl->getPtr(inst->_halo);
  return (dbBox*) b;
}

void dbInst::getConnectivity(std::vector<dbInst*>& result,
                             dbSigType::Value type)
{
  for (dbITerm* iterm : getITerms()) {
    dbNet* net = iterm->getNet();

    if (net == nullptr) {
      continue;
    }

    if ((net != nullptr) && (((_dbNet*) net)->_flags._sig_type != type)) {
      continue;
    }

    for (dbITerm* net_iterm : net->getITerms()) {
      dbInst* inst = net_iterm->getInst();

      if (inst != this) {
        result.push_back(inst);
      }
    }
  }

  // remove duplicates
  std::sort(result.begin(), result.end());
  auto end_itr = std::unique(result.begin(), result.end());
  result.erase(end_itr, result.end());
}

bool dbInst::resetHierarchy(bool verbose)
{
  _dbInst* inst = (_dbInst*) this;

  if (inst->_hierarchy) {
    if (verbose) {
      getImpl()->getLogger()->info(
          utl::ODB, 37, "instance bound to a block {}", inst->_hierarchy.id());
    }
    inst->_hierarchy = 0;
    return true;
  }
  /*
      if ( block->_parent_inst ) {
          warning(0, "block already bound to an instance %d\n",
     block->_parent_inst.id()); if (force) { warning(0, "Forced Initialize to
     0\n"); block->_parent_inst= 0; } else { return false;
          }
      }
  */
  return false;
}
bool dbInst::bindBlock(dbBlock* block_, bool force)
{
  _dbInst* inst = (_dbInst*) this;
  _dbBlock* block = (_dbBlock*) block_;

  if (inst->_hierarchy) {
    getImpl()->getLogger()->warn(utl::ODB,
                                 38,
                                 "instance already bound to a block {}",
                                 inst->_hierarchy.id());
    if (force) {
      getImpl()->getLogger()->warn(utl::ODB, 39, "Forced Initialize to 0");
      inst->_hierarchy = 0;
    } else {
      return false;
    }
  }

  if (block->_parent_inst) {
    getImpl()->getLogger()->warn(utl::ODB,
                                 40,
                                 "block already bound to an instance {}",
                                 block->_parent_inst.id());
    if (force) {
      getImpl()->getLogger()->warn(utl::ODB, 41, "Forced Initialize to 0");
      block->_parent_inst = 0;
    } else {
      return false;
    }
  }

  if (block_->getParent() != getBlock()) {
    getImpl()->getLogger()->warn(
        utl::ODB, 42, "block not a direct child of instance owner");
    return false;
  }

  _dbHier* hier = _dbHier::create(this, block_);

  // _dbHier::create fails if bterms cannot be mapped (1-to-1) to mterms
  if (hier == nullptr) {
    getImpl()->getLogger()->warn(utl::ODB, 43, "_dbHier::create fails");
    return false;
  }
  return true;
}

void dbInst::unbindBlock()
{
  _dbInst* inst = (_dbInst*) this;

  if (inst->_hierarchy) {
    _dbBlock* block = (_dbBlock*) inst->getOwner();
    _dbHier* hier = block->_hier_tbl->getPtr(inst->_hierarchy);
    _dbHier::destroy(hier);
  }
}

dbBlock* dbInst::getChild()
{
  _dbInst* inst = (_dbInst*) this;

  if (inst->_hierarchy == 0) {
    return nullptr;
  }

  _dbBlock* block = (_dbBlock*) inst->getOwner();
  _dbChip* chip = (_dbChip*) block->getOwner();
  _dbHier* hier = block->_hier_tbl->getPtr(inst->_hierarchy);
  _dbBlock* child = chip->_block_tbl->getPtr(hier->_child_block);
  return (dbBlock*) child;
}

bool dbInst::isHierarchical()
{
  _dbInst* inst = (_dbInst*) this;
  return inst->_hierarchy != 0;
}

bool dbInst::isPhysicalOnly()
{
  _dbInst* inst = (_dbInst*) this;

  return inst->_module == 0;
}

dbInst* dbInst::getParent()
{
  dbBlock* block = (dbBlock*) getImpl()->getOwner();
  return block->getParentInst();
}

dbSet<dbInst> dbInst::getChildren()
{
  dbBlock* child = getChild();

  if (child == nullptr) {
    return dbSet<dbInst>(child, &dbNullIterator::null_iterator);
  }

  return child->getInsts();
}

int dbInst::getWeight()
{
  _dbInst* inst = (_dbInst*) this;
  return inst->_weight;
}

void dbInst::setWeight(int weight)
{
  _dbInst* inst = (_dbInst*) this;
  inst->_weight = weight;
}

dbSourceType dbInst::getSourceType()
{
  _dbInst* inst = (_dbInst*) this;
  dbSourceType t(inst->_flags._source);
  return t;
}

void dbInst::setSourceType(dbSourceType type)
{
  _dbInst* inst = (_dbInst*) this;
  inst->_flags._source = type;
}

dbITerm* dbInst::getITerm(dbMTerm* mterm_)
{
  _dbInst* inst = (_dbInst*) this;
  _dbBlock* block = (_dbBlock*) inst->getOwner();
  _dbMTerm* mterm = (_dbMTerm*) mterm_;
  _dbITerm* iterm = block->_iterm_tbl->getPtr(inst->_iterms[mterm->_order_id]);
  return (dbITerm*) iterm;
}
dbITerm* dbInst::getITerm(uint mterm_order_id)
{
  _dbInst* inst = (_dbInst*) this;
  _dbBlock* block = (_dbBlock*) inst->getOwner();
  _dbITerm* iterm = block->_iterm_tbl->getPtr(inst->_iterms[mterm_order_id]);
  return (dbITerm*) iterm;
}
bool dbInst::swapMaster(dbMaster* new_master_)
{
  const char* newMasterName = new_master_->getConstName();
  const char* oldMasterName = this->getMaster()->getConstName();

  _dbInst* inst = (_dbInst*) this;
  _dbBlock* block = (_dbBlock*) inst->getOwner();
  dbMaster* old_master_ = getMaster();

  if (inst->_flags._dont_touch) {
    inst->getLogger()->error(
        utl::ODB,
        368,
        "Attempt to change master of dont_touch instance {}",
        inst->_name);
  }

  if (inst->_hierarchy) {
    getImpl()->getLogger()->warn(utl::ODB,
                                 44,
                                 "Failed(_hierarchy) to swap: {} -> {} {}",
                                 oldMasterName,
                                 newMasterName,
                                 this->getConstName());
    return false;
  }

  if (block->_journal) {
    debugPrint(
        getImpl()->getLogger(), utl::ODB, "DB_ECO", 1, "ECO: swapMaster");
    dbLib* old_lib = old_master_->getLib();
    dbLib* new_lib = new_master_->getLib();
    block->_journal->beginAction(dbJournal::SWAP_OBJECT);
    block->_journal->pushParam(dbInstObj);
    block->_journal->pushParam(inst->getId());
    block->_journal->pushParam(old_lib->getId());
    block->_journal->pushParam(old_master_->getId());
    block->_journal->pushParam(new_lib->getId());
    block->_journal->pushParam(new_master_->getId());
    block->_journal->endAction();
  }

  for (auto cb : block->_callbacks) {
    cb->inDbInstSwapMasterBefore(this, new_master_);
  }

  //
  // Ensure the mterms are equivalent
  //
  std::vector<_dbMTerm*> new_terms;

  for (dbMTerm* mterm : new_master_->getMTerms()) {
    new_terms.push_back((_dbMTerm*) mterm);
  }

  std::vector<_dbMTerm*> old_terms;

  for (dbMTerm* mterm : old_master_->getMTerms()) {
    old_terms.push_back((_dbMTerm*) mterm);
  }

  if (old_terms.size() != new_terms.size()) {
    getImpl()->getLogger()->warn(utl::ODB,
                                 45,
                                 "Failed(termSize) to swap: {} -> {} {}",
                                 oldMasterName,
                                 newMasterName,
                                 this->getConstName());
    return false;
  }

  std::vector<uint> idx_map(old_terms.size());
  std::sort(new_terms.begin(), new_terms.end(), sortMTerm());
  std::sort(old_terms.begin(), old_terms.end(), sortMTerm());
  std::vector<_dbMTerm*>::iterator i1 = new_terms.begin();
  std::vector<_dbMTerm*>::iterator i2 = old_terms.begin();

  for (; i1 != new_terms.end() && i2 != old_terms.end(); ++i1, ++i2) {
    _dbMTerm* t1 = *i1;
    _dbMTerm* t2 = *i2;

    if (strcmp(t1->_name, t2->_name) != 0) {
      break;
    }

    idx_map[t2->_order_id] = t1->_order_id;
  }

  // mterms are not equivalent
  if (i1 != new_terms.end() || i2 != old_terms.end()) {
    getImpl()->getLogger()->warn(utl::ODB,
                                 46,
                                 "Failed(mtermEquiv) to swap: {} -> {} {}",
                                 oldMasterName,
                                 newMasterName,
                                 this->getConstName());
    return false;
  }

  for (auto iterm_idx : inst->_iterms) {
    dbITerm* iterm = (dbITerm*) block->_iterm_tbl->getPtr(iterm_idx);
    iterm->clearPrefAccessPoints();
  }

  // remove reference to inst_hdr
  _dbInstHdr* old_inst_hdr
      = block->_inst_hdr_hash.find(((_dbMaster*) old_master_)->_id);
  old_inst_hdr->_inst_cnt--;

  // delete the old-inst-hdr if required
  if (old_inst_hdr->_inst_cnt == 0) {
    dbInstHdr::destroy((dbInstHdr*) old_inst_hdr);
  }

  // add reference to new inst_hdr
  _dbInstHdr* new_inst_hdr
      = block->_inst_hdr_hash.find(((_dbMaster*) new_master_)->_id);

  // create a new inst-hdr if needed
  if (new_inst_hdr == nullptr) {
    new_inst_hdr = (_dbInstHdr*) dbInstHdr::create((dbBlock*) block,
                                                   (dbMaster*) new_master_);
  }

  new_inst_hdr->_inst_cnt++;
  inst->_inst_hdr = new_inst_hdr->getOID();

  // set new bbox based on new master
  _dbInst::setInstBBox(inst);

  // The next two steps invalidates any dbSet<dbITerm> iterators.

  // 1) update the iterm-mterm-idx
  uint cnt = inst->_iterms.size();

  uint i;
  for (i = 0; i < cnt; ++i) {
    _dbITerm* it = block->_iterm_tbl->getPtr(inst->_iterms[i]);
    uint old_idx = it->_flags._mterm_idx;
    it->_flags._mterm_idx = idx_map[old_idx];
  }

  // 2) reorder the iterms vector
  sortITerm itermCmp(block);
  std::sort(inst->_iterms.begin(), inst->_iterms.end(), itermCmp);

  // Notification
  for (auto cb : block->_callbacks) {
    cb->inDbInstSwapMasterAfter(this);
  }

  return true;
}

void dbInst::setPinAccessIdx(uint idx)
{
  _dbInst* inst = (_dbInst*) this;
  inst->pin_access_idx_ = idx;
}

uint dbInst::getPinAccessIdx() const
{
  _dbInst* inst = (_dbInst*) this;
  return inst->pin_access_idx_;
}

dbInst* dbInst::create(dbBlock* block_,
                       dbMaster* master_,
                       const char* name_,
                       bool physical_only,
                       dbModule* target_module)
{
  return create(block_, master_, name_, nullptr, physical_only, target_module);
}

dbInst* dbInst::create(dbBlock* block_,
                       dbMaster* master_,
                       const char* name_,
                       dbRegion* region,
                       bool physical_only,
                       dbModule* parent_module)
{
  _dbBlock* block = (_dbBlock*) block_;
  if (block->_inst_hash.hasMember(name_)) {
    return nullptr;
  }

  _dbMaster* master = (_dbMaster*) master_;
  _dbInstHdr* inst_hdr = block->_inst_hdr_hash.find(master->_id);
  if (inst_hdr == nullptr) {
    inst_hdr
        = (_dbInstHdr*) dbInstHdr::create((dbBlock*) block, (dbMaster*) master);
  }

  _dbInst* inst = block->_inst_tbl->create();

  if (block->_journal) {
    debugPrint(block->getImpl()->getLogger(),
               utl::ODB,
               "DB_ECO",
               1,
               "ECO: dbInst:create");
    dbLib* lib = master_->getLib();
    block->_journal->beginAction(dbJournal::CREATE_OBJECT);
    block->_journal->pushParam(dbInstObj);
    block->_journal->pushParam(lib->getId());
    block->_journal->pushParam(master_->getId());
    block->_journal->pushParam(name_);
    // need to add dbModNet
    // dbModule (scope)
    block->_journal->pushParam(inst->getOID());
    block->_journal->endAction();
  }

  inst->_name = strdup(name_);
  ZALLOCATED(inst->_name);
  inst->_inst_hdr = inst_hdr->getOID();
  block->_inst_hash.insert(inst);
  inst_hdr->_inst_cnt++;

  // create the iterms
  uint mterm_cnt = inst_hdr->_mterms.size();
  inst->_iterms.resize(mterm_cnt);

  for (int i = 0; i < mterm_cnt; ++i) {
    _dbITerm* iterm = block->_iterm_tbl->create();
    inst->_iterms[i] = iterm->getOID();
    iterm->_flags._mterm_idx = i;
    iterm->_inst = inst->getOID();
  }

  _dbBox* box = block->_box_tbl->create();
  box->_shape._rect.init(0, 0, master->_width, master->_height);
  box->_flags._owner_type = dbBoxOwner::INST;
  box->_owner = inst->getOID();
  inst->_bbox = box->getOID();

  block->add_rect(box->_shape._rect);

  inst->_flags._physical_only = physical_only;
  if (!physical_only) {
    if (parent_module) {
      parent_module->addInst((dbInst*) inst);
    } else {
      block_->getTopModule()->addInst((dbInst*) inst);
    }
  }

  if (region) {
    region->addInst((dbInst*) inst);
    for (dbBlockCallBackObj* cb : block->_callbacks) {
      cb->inDbInstCreate((dbInst*) inst, region);
    }
  } else {
    for (dbBlockCallBackObj* cb : block->_callbacks) {
      cb->inDbInstCreate((dbInst*) inst);
    }
  }

  for (int i = 0; i < mterm_cnt; ++i) {
    _dbITerm* iterm = block->_iterm_tbl->getPtr(inst->_iterms[i]);
    for (dbBlockCallBackObj* cb : block->_callbacks) {
      cb->inDbITermCreate((dbITerm*) iterm);
    }
  }

  return (dbInst*) inst;
}

dbInst* dbInst::create(dbBlock* top_block,
                       dbBlock* child_block,
                       const char* name)
{
  if (top_block->findInst(name)) {
    top_block->getImpl()->getLogger()->error(
        utl::ODB,
        436,
        "Attempt to create instance with duplicate name: {}",
        name);
  }
  // Find or create a dbLib to put the new dbMaster in.
  dbDatabase* db = top_block->getDataBase();
  dbTech* tech = child_block->getTech();
  dbLib* lib = nullptr;
  for (auto l : db->getLibs()) {
    if (l->getTech() == tech) {
      lib = l;
      break;
    }
  }
  if (!lib) {
    std::string lib_name = child_block->getName() + tech->getName();
    lib = dbLib::create(db, lib_name.c_str(), child_block->getTech());
  }
  auto master = dbMaster::create(lib, child_block->getName().c_str());
  master->setType(dbMasterType::BLOCK);
  auto bbox = child_block->getBBox();
  master->setWidth(bbox->getDX());
  master->setHeight(bbox->getDY());
  for (auto term : child_block->getBTerms()) {
    dbMTerm::create(
        master, term->getName().c_str(), term->getIoType(), term->getSigType());
  }
  master->setFrozen();
  auto inst = dbInst::create(top_block, master, name);
  inst->bindBlock(child_block);

  return inst;
}

dbInst* dbInst::makeUniqueDbInst(dbBlock* block,
                                 dbMaster* master,
                                 const char* name,
                                 bool physical_only,
                                 dbModule* target_module)
{
  dbInst* inst
      = dbInst::create(block, master, name, physical_only, target_module);
  if (inst) {
    return inst;
  }

  std::unordered_map<std::string, int>& name_id_map
      = ((_dbBlock*) block)->_inst_name_id_map;
  std::string inst_base_name(name);
  do {
    std::string full_name = inst_base_name;
    int& id = name_id_map[inst_base_name];
    if (id > 0) {
      full_name += "_" + std::to_string(id);
    }
    ++id;
    inst = dbInst::create(
        block, master, full_name.c_str(), physical_only, target_module);
  } while (inst == nullptr);

  return inst;
}

void dbInst::destroy(dbInst* inst_)
{
  _dbInst* inst = (_dbInst*) inst_;
  _dbBlock* block = (_dbBlock*) inst->getOwner();

  if (inst->_flags._dont_touch) {
    inst->getLogger()->error(utl::ODB,
                             362,
                             "Attempt to destroy dont_touch instance {}",
                             inst->_name);
  }

  uint i;
  uint n = inst->_iterms.size();

  // Delete these in reverse order so undo creates the in
  // the correct order.
  for (i = 0; i < n; ++i) {
    dbId<_dbITerm> id = inst->_iterms[n - 1 - i];
    _dbITerm* _iterm = block->_iterm_tbl->getPtr(id);
    dbITerm* iterm = (dbITerm*) _iterm;
    iterm->disconnect();
    if (inst_->getPinAccessIdx() >= 0) {
      for (const auto& [pin, aps] : iterm->getAccessPoints()) {
        for (auto ap : aps) {
          _dbAccessPoint* _ap = (_dbAccessPoint*) ap;
          _ap->iterms_.erase(
              std::remove_if(_ap->iterms_.begin(),
                             _ap->iterms_.end(),
                             [id](const auto& id_in) { return id_in == id; }),
              _ap->iterms_.end());
        }
      }
    }

    // Notify when pins are deleted (assumption: pins are destroyed only when
    // the related instance is destroyed)
    for (auto cb : block->_callbacks) {
      cb->inDbITermDestroy((dbITerm*) _iterm);
    }

    dbProperty::destroyProperties(_iterm);
    block->_iterm_tbl->destroy(_iterm);
    inst->_iterms.pop_back();
  }

  dbModule* module = inst_->getModule();
  if (module) {
    ((_dbModule*) module)->_dbinst_hash.erase(inst_->getName());
  }

  if (block->_journal) {
    debugPrint(block->getImpl()->getLogger(),
               utl::ODB,
               "DB_ECO",
               1,
               "ECO: dbInst:destroy");
    auto master = inst_->getMaster();
    block->_journal->beginAction(dbJournal::DELETE_OBJECT);
    block->_journal->pushParam(dbInstObj);
    block->_journal->pushParam(master->getLib()->getId());
    block->_journal->pushParam(master->getId());
    block->_journal->pushParam(inst_->getName().c_str());
    block->_journal->pushParam(inst_->getId());
    uint* flags = (uint*) &inst->_flags;
    block->_journal->pushParam(*flags);
    block->_journal->pushParam(inst->_x);
    block->_journal->pushParam(inst->_y);
    block->_journal->pushParam(inst->_group);
    block->_journal->pushParam(inst->_module);
    block->_journal->pushParam(inst->_region);
    block->_journal->endAction();
  }

  dbRegion* region = inst_->getRegion();

  if (region) {
    region->removeInst(inst_);
  }

  if (module) {
    ((_dbModule*) module)->removeInst(inst_);
  }

  if (inst->_group) {
    inst_->getGroup()->removeInst(inst_);
  }

  for (auto cb : block->_callbacks) {
    cb->inDbInstDestroy(inst_);
  }

  _dbMaster* master = (_dbMaster*) inst_->getMaster();
  _dbInstHdr* inst_hdr = block->_inst_hdr_hash.find(master->_id);
  inst_hdr->_inst_cnt--;

  if (inst_hdr->_inst_cnt == 0) {
    dbInstHdr::destroy((dbInstHdr*) inst_hdr);
  }

  if (inst->_halo) {
    _dbBox* halo = block->_box_tbl->getPtr(inst->_halo);
    dbProperty::destroyProperties(halo);
    block->_box_tbl->destroy(halo);
  }

  _dbBox* box = block->_box_tbl->getPtr(inst->_bbox);
  block->remove_rect(box->_shape._rect);
  block->_inst_hash.remove(inst);
  dbProperty::destroyProperties(inst);
  block->_inst_tbl->destroy(inst);
  dbProperty::destroyProperties(box);
  block->_box_tbl->destroy(box);
}

dbSet<dbInst>::iterator dbInst::destroy(dbSet<dbInst>::iterator& itr)
{
  dbInst* bt = *itr;
  dbSet<dbInst>::iterator next = ++itr;
  destroy(bt);
  return next;
}

dbInst* dbInst::getInst(dbBlock* block_, uint dbid_)
{
  _dbBlock* block = (_dbBlock*) block_;
  return (dbInst*) block->_inst_tbl->getPtr(dbid_);
}

dbInst* dbInst::getValidInst(dbBlock* block_, uint dbid_)
{
  _dbBlock* block = (_dbBlock*) block_;
  if (!block->_inst_tbl->validId(dbid_)) {
    return nullptr;
  }
  return (dbInst*) block->_inst_tbl->getPtr(dbid_);
}
dbITerm* dbInst::getFirstOutput()
{
  for (dbITerm* tr : getITerms()) {
    if (tr->getSigType().isSupply()) {
      continue;
    }

    if (tr->getIoType() != dbIoType::OUTPUT) {
      continue;
    }

    return tr;
  }
  getImpl()->getLogger()->warn(
      utl::ODB, 47, "instance {} has no output pin", getConstName());
  return nullptr;
}

void _dbInst::collectMemInfo(MemInfo& info)
{
  info.cnt++;
  info.size += sizeof(*this);

  info.children_["name"].add(_name);
  info.children_["iterms"].add(_iterms);
}

}  // namespace odb
