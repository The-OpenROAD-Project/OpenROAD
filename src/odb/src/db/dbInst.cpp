// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "dbInst.h"

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <string>
#include <unordered_map>
#include <vector>

#include "dbAccessPoint.h"
#include "dbBTerm.h"
#include "dbBlock.h"
#include "dbBox.h"
#include "dbChip.h"
#include "dbCommon.h"
#include "dbCore.h"
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
#include "dbModInst.h"
#include "dbModule.h"
#include "dbNet.h"
#include "dbNullIterator.h"
#include "dbRegion.h"
#include "dbScanInst.h"
#include "dbTable.h"
#include "odb/db.h"
#include "odb/dbBlockCallBackObj.h"
#include "odb/dbObject.h"
#include "odb/dbSet.h"
#include "odb/dbTransform.h"
#include "odb/dbTypes.h"
#include "utl/Logger.h"
#include "utl/algorithms.h"

namespace odb {

template class dbTable<_dbInst>;

class sortMTerm
{
 public:
  bool operator()(_dbMTerm* m1, _dbMTerm* m2)
  {
    return strcmp(m1->name_, m2->name_) < 0;
  }
};

class sortITerm
{
  _dbBlock* block_;

 public:
  sortITerm(_dbBlock* block) { block_ = block; }

  bool operator()(uint32_t it1, uint32_t it2)
  {
    _dbITerm* iterm1 = block_->iterm_tbl_->getPtr(it1);
    _dbITerm* iterm2 = block_->iterm_tbl_->getPtr(it2);
    return iterm1->flags_.mterm_idx < iterm2->flags_.mterm_idx;
  }
};

void _dbInst::setInstBBox(_dbInst* inst)
{
  _dbBlock* block = (_dbBlock*) inst->getOwner();
  _dbBox* box = block->box_tbl_->getPtr(inst->bbox_);
  block->remove_rect(box->shape_.rect);

  dbMaster* master = ((dbInst*) inst)->getMaster();
  master->getPlacementBoundary(box->shape_.rect);
  dbTransform transform(inst->flags_.orient, Point(inst->x_, inst->y_));
  transform.apply(box->shape_.rect);
  block->add_rect(box->shape_.rect);
}

_dbInst::_dbInst(_dbDatabase*)
{
  flags_.orient = dbOrientType::R0;
  flags_.status = dbPlacementStatus::NONE;
  flags_.user_flag_1 = 0;
  flags_.user_flag_2 = 0;
  flags_.user_flag_3 = 0;
  flags_.physical_only = 0;
  flags_.dont_touch = 0;
  flags_.eco_create = 0;
  flags_.eco_destroy = 0;
  flags_.eco_modify = 0;
  flags_.source = dbSourceType::NONE;
  // flags_._spare_bits = 0;
  flags_.level = 0;
  name_ = nullptr;
  x_ = 0;
  y_ = 0;
  weight_ = 0;
  pin_access_idx_ = -1;
}

_dbInst::_dbInst(_dbDatabase*, const _dbInst& i)
    : flags_(i.flags_),
      name_(nullptr),
      x_(i.x_),
      y_(i.y_),
      weight_(i.weight_),
      next_entry_(i.next_entry_),
      inst_hdr_(i.inst_hdr_),
      bbox_(i.bbox_),
      region_(i.region_),
      module_(i.module_),
      group_(i.group_),
      region_next_(i.region_next_),
      module_next_(i.module_next_),
      group_next_(i.group_next_),
      region_prev_(i.region_prev_),
      module_prev_(i.module_prev_),
      hierarchy_(i.hierarchy_),
      iterms_(i.iterms_),
      halo_(i.halo_),
      pin_access_idx_(i.pin_access_idx_)
{
  if (i.name_) {
    name_ = safe_strdup(i.name_);
  }
}

_dbInst::~_dbInst()
{
  if (name_) {
    free((void*) name_);
  }
}

dbOStream& operator<<(dbOStream& stream, const _dbInst& inst)
{
  uint32_t* bit_field = (uint32_t*) &inst.flags_;
  stream << *bit_field;
  stream << inst.name_;
  stream << inst.x_;
  stream << inst.y_;
  stream << inst.weight_;
  stream << inst.next_entry_;
  stream << inst.inst_hdr_;
  stream << inst.bbox_;
  stream << inst.region_;
  stream << inst.module_;
  stream << inst.group_;
  stream << inst.region_next_;
  stream << inst.module_next_;
  stream << inst.group_next_;
  stream << inst.region_prev_;
  stream << inst.module_prev_;
  stream << inst.hierarchy_;
  stream << inst.iterms_;
  stream << inst.halo_;
  stream << inst.pin_access_idx_;
  return stream;
}

dbIStream& operator>>(dbIStream& stream, _dbInst& inst)
{
  uint32_t* bit_field = (uint32_t*) &inst.flags_;
  stream >> *bit_field;
  stream >> inst.name_;
  stream >> inst.x_;
  stream >> inst.y_;
  stream >> inst.weight_;
  stream >> inst.next_entry_;
  stream >> inst.inst_hdr_;
  stream >> inst.bbox_;
  stream >> inst.region_;
  stream >> inst.module_;
  stream >> inst.group_;
  stream >> inst.region_next_;
  stream >> inst.module_next_;
  stream >> inst.group_next_;
  stream >> inst.region_prev_;
  stream >> inst.module_prev_;
  stream >> inst.hierarchy_;
  stream >> inst.iterms_;
  stream >> inst.halo_;
  stream >> inst.pin_access_idx_;
  return stream;
}

bool _dbInst::operator<(const _dbInst& rhs) const
{
  return strcmp(name_, rhs.name_) < 0;
}

bool _dbInst::operator==(const _dbInst& rhs) const
{
  if (flags_.orient != rhs.flags_.orient) {
    return false;
  }

  if (flags_.status != rhs.flags_.status) {
    return false;
  }

  if (flags_.user_flag_1 != rhs.flags_.user_flag_1) {
    return false;
  }

  if (flags_.user_flag_2 != rhs.flags_.user_flag_2) {
    return false;
  }

  if (flags_.user_flag_3 != rhs.flags_.user_flag_3) {
    return false;
  }

  if (flags_.physical_only != rhs.flags_.physical_only) {
    return false;
  }

  if (flags_.dont_touch != rhs.flags_.dont_touch) {
    return false;
  }

  if (flags_.source != rhs.flags_.source) {
    return false;
  }

  if (name_ && rhs.name_) {
    if (strcmp(name_, rhs.name_) != 0) {
      return false;
    }
  } else if (name_ || rhs.name_) {
    return false;
  }

  if (x_ != rhs.x_) {
    return false;
  }

  if (y_ != rhs.y_) {
    return false;
  }

  if (weight_ != rhs.weight_) {
    return false;
  }

  if (next_entry_ != rhs.next_entry_) {
    return false;
  }

  if (inst_hdr_ != rhs.inst_hdr_) {
    return false;
  }

  if (bbox_ != rhs.bbox_) {
    return false;
  }

  if (region_ != rhs.region_) {
    return false;
  }

  if (module_ != rhs.module_) {
    return false;
  }

  if (group_ != rhs.group_) {
    return false;
  }

  if (region_next_ != rhs.region_next_) {
    return false;
  }

  if (module_next_ != rhs.module_next_) {
    return false;
  }

  if (group_next_ != rhs.group_next_) {
    return false;
  }

  if (region_prev_ != rhs.region_prev_) {
    return false;
  }

  if (module_prev_ != rhs.module_prev_) {
    return false;
  }

  if (hierarchy_ != rhs.hierarchy_) {
    return false;
  }

  if (iterms_ != rhs.iterms_) {
    return false;
  }

  if (halo_ != rhs.halo_) {
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

std::string dbInst::getName() const
{
  _dbInst* inst = (_dbInst*) this;
  return inst->name_;
}

const char* dbInst::getConstName() const
{
  _dbInst* inst = (_dbInst*) this;
  return inst->name_;
}

bool dbInst::isNamed(const char* name)
{
  _dbInst* inst = (_dbInst*) this;
  if (!strcmp(inst->name_, name)) {
    return true;
  }
  return false;
}

bool dbInst::rename(const char* name)
{
  _dbInst* inst = (_dbInst*) this;
  _dbBlock* block = (_dbBlock*) inst->getOwner();

  if (block->inst_hash_.hasMember(name)) {
    return false;
  }

  debugPrint(getImpl()->getLogger(),
             utl::ODB,
             "DB_EDIT",
             1,
             "EDIT: {}, rename to '{}'",
             inst->getDebugName(),
             name);

  if (block->journal_) {
    block->journal_->updateField(this, _dbInst::kName, inst->name_, name);
  }

  block->inst_hash_.remove(inst);
  free((void*) inst->name_);
  inst->name_ = safe_strdup(name);
  block->inst_hash_.insert(inst);

  return true;
}

Point dbInst::getOrigin()
{
  _dbInst* inst = (_dbInst*) this;
  return {inst->x_, inst->y_};
}

void dbInst::setOrigin(int x, int y)
{
  _dbInst* inst = (_dbInst*) this;
  _dbBlock* block = (_dbBlock*) inst->getOwner();
  int prev_x = inst->x_;
  int prev_y = inst->y_;
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

  for (auto callback : block->callbacks_) {
    callback->inDbPreMoveInst(this);
  }

  inst->x_ = x;
  inst->y_ = y;
  _dbInst::setInstBBox(inst);

  debugPrint(getImpl()->getLogger(),
             utl::ODB,
             "DB_EDIT",
             2,
             "EDIT: {} setOrigin {}, {}",
             inst->getDebugName(),
             x,
             y);

  if (block->journal_) {
    block->journal_->beginAction(dbJournal::kUpdateField);
    block->journal_->pushParam(inst->getObjectType());
    block->journal_->pushParam(inst->getId());
    block->journal_->pushParam(_dbInst::kOrigin);
    block->journal_->pushParam(prev_x);
    block->journal_->pushParam(prev_y);
    block->journal_->pushParam(inst->x_);
    block->journal_->pushParam(inst->y_);
    block->journal_->endAction();
  }

  block->flags_.valid_bbox = 0;
  for (auto callback : block->callbacks_) {
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
  _dbBox* bbox = block->box_tbl_->getPtr(inst->bbox_);
  x = bbox->shape_.rect.xMin();
  y = bbox->shape_.rect.yMin();
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
  return (dbBox*) block->box_tbl_->getPtr(inst->bbox_);
}

dbOrientType dbInst::getOrient()
{
  _dbInst* inst = (_dbInst*) this;
  return dbOrientType(inst->flags_.orient);
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
  for (auto callback : block->callbacks_) {
    callback->inDbPreMoveInst(this);
  }
  uint32_t prev_flags = flagsToUInt(inst);
  inst->flags_.orient = orient.getValue();
  _dbInst::setInstBBox(inst);

  debugPrint(getImpl()->getLogger(),
             utl::ODB,
             "DB_EDIT",
             2,
             "EDIT: {} setOrient {}",
             inst->getDebugName(),
             orient.getValue());

  if (block->journal_) {
    block->journal_->updateField(
        this, _dbInst::kFlags, prev_flags, flagsToUInt(inst));
  }

  block->flags_.valid_bbox = 0;
  for (auto callback : block->callbacks_) {
    callback->inDbPostMoveInst(this);
  }
}

dbPlacementStatus dbInst::getPlacementStatus()
{
  _dbInst* inst = (_dbInst*) this;
  return dbPlacementStatus(inst->flags_.status);
}

void dbInst::setPlacementStatus(dbPlacementStatus status)
{
  _dbInst* inst = (_dbInst*) this;
  _dbBlock* block = (_dbBlock*) inst->getOwner();

  if (inst->flags_.status == status) {
    return;
  }

  for (auto callback : block->callbacks_) {
    callback->inDbInstPlacementStatusBefore(this, status);
  }

  uint32_t prev_flags = flagsToUInt(inst);
  inst->flags_.status = status.getValue();
  block->flags_.valid_bbox = 0;

  debugPrint(getImpl()->getLogger(),
             utl::ODB,
             "DB_EDIT",
             2,
             "EDIT: {} setPlacementStatus {}",
             inst->getDebugName(),
             status.getValue());

  if (block->journal_) {
    block->journal_->updateField(
        this, _dbInst::kFlags, prev_flags, flagsToUInt(inst));
  }
}

dbTransform dbInst::getTransform()
{
  _dbInst* inst = (_dbInst*) this;
  return dbTransform(inst->flags_.orient, Point(inst->x_, inst->y_));
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
  return inst->flags_.eco_create;
}
void dbInst::setEcoCreate(bool v)
{
  _dbInst* inst = (_dbInst*) this;
  // _dbBlock * block = (_dbBlock *) getOwner();
  // uint32_t prev_flags = flagsToUInt(inst);
  if (v) {
    inst->flags_.eco_create = 1;
  } else {
    inst->flags_.eco_create = 0;
  }
}
bool dbInst::getEcoDestroy()
{
  _dbInst* inst = (_dbInst*) this;
  return inst->flags_.eco_destroy;
}
void dbInst::setEcoDestroy(bool v)
{
  _dbInst* inst = (_dbInst*) this;
  // _dbBlock * block = (_dbBlock *) getOwner();
  // uint32_t prev_flags = flagsToUInt(inst);
  if (v) {
    inst->flags_.eco_destroy = 1;
  } else {
    inst->flags_.eco_destroy = 0;
  }
}
bool dbInst::getEcoModify()
{
  _dbInst* inst = (_dbInst*) this;
  return inst->flags_.eco_modify;
}
void dbInst::setEcoModify(bool v)
{
  _dbInst* inst = (_dbInst*) this;
  // _dbBlock * block = (_dbBlock *) getOwner();
  // uint32_t prev_flags = flagsToUInt(inst);
  if (v) {
    inst->flags_.eco_modify = 1;
  } else {
    inst->flags_.eco_modify = 0;
  }
}
bool dbInst::getUserFlag1()
{
  _dbInst* inst = (_dbInst*) this;
  return inst->flags_.user_flag_1 == 1;
}

void dbInst::setUserFlag1()
{
  _dbInst* inst = (_dbInst*) this;
  _dbBlock* block = (_dbBlock*) inst->getOwner();
  uint32_t prev_flags = flagsToUInt(inst);
  inst->flags_.user_flag_1 = 1;

  if (block->journal_) {
    block->journal_->updateField(
        this, _dbInst::kFlags, prev_flags, flagsToUInt(inst));
  }
}

void dbInst::clearUserFlag1()
{
  _dbInst* inst = (_dbInst*) this;
  _dbBlock* block = (_dbBlock*) inst->getOwner();
  uint32_t prev_flags = flagsToUInt(inst);
  inst->flags_.user_flag_1 = 0;

  if (block->journal_) {
    block->journal_->updateField(
        this, _dbInst::kFlags, prev_flags, flagsToUInt(inst));
  }
}

bool dbInst::getUserFlag2()
{
  _dbInst* inst = (_dbInst*) this;
  return inst->flags_.user_flag_2 == 1;
}

void dbInst::setUserFlag2()
{
  _dbInst* inst = (_dbInst*) this;
  _dbBlock* block = (_dbBlock*) inst->getOwner();
  uint32_t prev_flags = flagsToUInt(inst);
  inst->flags_.user_flag_2 = 1;

  if (block->journal_) {
    block->journal_->updateField(
        this, _dbInst::kFlags, prev_flags, flagsToUInt(inst));
  }
}

void dbInst::clearUserFlag2()
{
  _dbInst* inst = (_dbInst*) this;
  _dbBlock* block = (_dbBlock*) inst->getOwner();
  uint32_t prev_flags = flagsToUInt(inst);
  inst->flags_.user_flag_2 = 0;

  if (block->journal_) {
    block->journal_->updateField(
        this, _dbInst::kFlags, prev_flags, flagsToUInt(inst));
  }
}

bool dbInst::getUserFlag3()
{
  _dbInst* inst = (_dbInst*) this;
  return inst->flags_.user_flag_3 == 1;
}

void dbInst::setUserFlag3()
{
  _dbInst* inst = (_dbInst*) this;
  _dbBlock* block = (_dbBlock*) inst->getOwner();
  uint32_t prev_flags = flagsToUInt(inst);
  inst->flags_.user_flag_3 = 1;

  if (block->journal_) {
    block->journal_->updateField(
        this, _dbInst::kFlags, prev_flags, flagsToUInt(inst));
  }
}

void dbInst::clearUserFlag3()
{
  _dbInst* inst = (_dbInst*) this;
  _dbBlock* block = (_dbBlock*) inst->getOwner();
  uint32_t prev_flags = flagsToUInt(inst);
  inst->flags_.user_flag_3 = 0;

  if (block->journal_) {
    block->journal_->updateField(
        this, _dbInst::kFlags, prev_flags, flagsToUInt(inst));
  }
}

void dbInst::setDoNotTouch(bool v)
{
  _dbInst* inst = (_dbInst*) this;
  inst->flags_.dont_touch = v;
}

bool dbInst::isDoNotTouch()
{
  _dbInst* inst = (_dbInst*) this;
  return inst->flags_.dont_touch == 1;
}

dbBlock* dbInst::getBlock() const
{
  return (dbBlock*) getImpl()->getOwner();
}

dbMaster* dbInst::getMaster() const
{
  _dbInst* inst = (_dbInst*) this;
  _dbBlock* block = (_dbBlock*) inst->getOwner();
  _dbInstHdr* inst_hdr = block->inst_hdr_tbl_->getPtr(inst->inst_hdr_);
  _dbDatabase* db = inst->getDatabase();
  _dbLib* lib = db->lib_tbl_->getPtr(inst_hdr->lib_);
  return (dbMaster*) lib->master_tbl_->getPtr(inst_hdr->master_);
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

dbScanInst* dbInst::getScanInst() const
{
  _dbInst* inst = (_dbInst*) this;
  _dbBlock* block = (_dbBlock*) inst->getOwner();
  auto itr = block->inst_scan_inst_map_.find(inst->getId());

  if (itr == block->inst_scan_inst_map_.end()) {
    return nullptr;
  }

  dbId<_dbScanInst> scan_inst_id = itr->second;
  _dbScanInst* scan_inst = block->scan_inst_tbl_->getPtr(scan_inst_id);

  return (dbScanInst*) scan_inst;
}

dbSet<dbITerm> dbInst::getITerms() const
{
  _dbInst* inst = (_dbInst*) this;
  _dbBlock* block = (_dbBlock*) inst->getOwner();
  return dbSet<dbITerm>(inst, block->inst_iterm_itr_);
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

  // sta may callback from inDbITermDestroy to find another iterm of
  // the same instance (eg Latches::latchDtoQEnable).  We have to
  // check if the entry is valid here as that iterm may already have
  // been destroyed!

  dbId<_dbITerm> id = inst->iterms_[mterm->order_id_];
  if (!id.isValid()) {
    return nullptr;
  }
  return (dbITerm*) block->iterm_tbl_->getPtr(id);
}

dbRegion* dbInst::getRegion()
{
  _dbInst* inst = (_dbInst*) this;

  if (inst->region_ == 0) {
    return nullptr;
  }

  _dbBlock* block = (_dbBlock*) inst->getOwner();
  _dbRegion* r = block->region_tbl_->getPtr(inst->region_);
  return (dbRegion*) r;
}

dbModule* dbInst::getModule()
{
  _dbInst* inst = (_dbInst*) this;

  if (inst->module_ == 0) {
    return nullptr;
  }

  _dbBlock* block = (_dbBlock*) inst->getOwner();
  _dbModule* module = block->module_tbl_->getPtr(inst->module_);
  return (dbModule*) module;
}

dbGroup* dbInst::getGroup()
{
  _dbInst* inst = (_dbInst*) this;

  if (inst->group_ == 0) {
    return nullptr;
  }

  _dbBlock* block = (_dbBlock*) inst->getOwner();
  _dbGroup* group = block->group_tbl_->getPtr(inst->group_);
  return (dbGroup*) group;
}

dbBox* dbInst::getHalo()
{
  _dbInst* inst = (_dbInst*) this;

  if (inst->halo_ == 0) {
    return nullptr;
  }

  _dbBlock* block = (_dbBlock*) inst->getOwner();
  _dbBox* b = block->box_tbl_->getPtr(inst->halo_);
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

    if ((net != nullptr) && (((_dbNet*) net)->flags_.sig_type != type)) {
      continue;
    }

    for (dbITerm* net_iterm : net->getITerms()) {
      dbInst* inst = net_iterm->getInst();

      if (inst != this) {
        result.push_back(inst);
      }
    }
  }

  utl::sort_and_unique(result);
}

bool dbInst::resetHierarchy(bool verbose)
{
  _dbInst* inst = (_dbInst*) this;

  if (inst->hierarchy_) {
    if (verbose) {
      getImpl()->getLogger()->info(
          utl::ODB, 37, "instance bound to a block {}", inst->hierarchy_.id());
    }
    inst->hierarchy_ = 0;
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

  if (inst->hierarchy_) {
    getImpl()->getLogger()->warn(utl::ODB,
                                 38,
                                 "instance already bound to a block {}",
                                 inst->hierarchy_.id());
    if (force) {
      getImpl()->getLogger()->warn(utl::ODB, 39, "Forced Initialize to 0");
      inst->hierarchy_ = 0;
    } else {
      return false;
    }
  }

  if (block->parent_inst_) {
    getImpl()->getLogger()->warn(utl::ODB,
                                 40,
                                 "block already bound to an instance {}",
                                 block->parent_inst_.id());
    if (force) {
      getImpl()->getLogger()->warn(utl::ODB, 41, "Forced Initialize to 0");
      block->parent_inst_ = 0;
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

  if (inst->hierarchy_) {
    _dbBlock* block = (_dbBlock*) inst->getOwner();
    _dbHier* hier = block->hier_tbl_->getPtr(inst->hierarchy_);
    _dbHier::destroy(hier);
  }
}

dbBlock* dbInst::getChild()
{
  _dbInst* inst = (_dbInst*) this;

  if (inst->hierarchy_ == 0) {
    return nullptr;
  }

  _dbBlock* block = (_dbBlock*) inst->getOwner();
  _dbChip* chip = (_dbChip*) block->getOwner();
  _dbHier* hier = block->hier_tbl_->getPtr(inst->hierarchy_);
  _dbBlock* child = chip->block_tbl_->getPtr(hier->child_block_);
  return (dbBlock*) child;
}

bool dbInst::isHierarchical()
{
  _dbInst* inst = (_dbInst*) this;
  return inst->hierarchy_ != 0;
}

bool dbInst::isPhysicalOnly()
{
  _dbInst* inst = (_dbInst*) this;

  return inst->module_ == 0;
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
  return inst->weight_;
}

void dbInst::setWeight(int weight)
{
  _dbInst* inst = (_dbInst*) this;
  inst->weight_ = weight;
}

dbSourceType dbInst::getSourceType()
{
  _dbInst* inst = (_dbInst*) this;
  dbSourceType t(inst->flags_.source);
  return t;
}

void dbInst::setSourceType(dbSourceType type)
{
  _dbInst* inst = (_dbInst*) this;
  inst->flags_.source = type;
}

dbITerm* dbInst::getITerm(dbMTerm* mterm_)
{
  _dbInst* inst = (_dbInst*) this;
  _dbBlock* block = (_dbBlock*) inst->getOwner();
  _dbMTerm* mterm = (_dbMTerm*) mterm_;
  _dbITerm* iterm = block->iterm_tbl_->getPtr(inst->iterms_[mterm->order_id_]);
  return (dbITerm*) iterm;
}
dbITerm* dbInst::getITerm(uint32_t mterm_order_id)
{
  _dbInst* inst = (_dbInst*) this;
  _dbBlock* block = (_dbBlock*) inst->getOwner();
  _dbITerm* iterm = block->iterm_tbl_->getPtr(inst->iterms_[mterm_order_id]);
  return (dbITerm*) iterm;
}
bool dbInst::swapMaster(dbMaster* new_master_)
{
  const char* newMasterName = new_master_->getConstName();
  const char* oldMasterName = this->getMaster()->getConstName();

  _dbInst* inst = (_dbInst*) this;
  _dbBlock* block = (_dbBlock*) inst->getOwner();
  dbMaster* old_master_ = getMaster();

  if (inst->flags_.dont_touch) {
    inst->getLogger()->error(
        utl::ODB,
        368,
        "Attempt to change master of dont_touch instance {}",
        inst->name_);
  }

  if (inst->hierarchy_) {
    getImpl()->getLogger()->warn(utl::ODB,
                                 44,
                                 "Failed(_hierarchy) to swap: {} -> {} {}",
                                 oldMasterName,
                                 newMasterName,
                                 this->getConstName());
    return false;
  }

  debugPrint(getImpl()->getLogger(),
             utl::ODB,
             "DB_EDIT",
             1,
             "EDIT: swapMaster on {} from master '{}' to '{}'",
             inst->getDebugName(),
             oldMasterName,
             newMasterName);

  if (block->journal_) {
    dbLib* old_lib = old_master_->getLib();
    dbLib* new_lib = new_master_->getLib();
    block->journal_->beginAction(dbJournal::kSwapObject);
    block->journal_->pushParam(dbInstObj);
    block->journal_->pushParam(inst->getId());
    block->journal_->pushParam(old_lib->getId());
    block->journal_->pushParam(old_master_->getId());
    block->journal_->pushParam(new_lib->getId());
    block->journal_->pushParam(new_master_->getId());
    block->journal_->endAction();
  }

  for (auto cb : block->callbacks_) {
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

  std::vector<uint32_t> idx_map(old_terms.size());
  std::ranges::sort(new_terms, sortMTerm());
  std::ranges::sort(old_terms, sortMTerm());
  std::vector<_dbMTerm*>::iterator i1 = new_terms.begin();
  std::vector<_dbMTerm*>::iterator i2 = old_terms.begin();

  for (; i1 != new_terms.end() && i2 != old_terms.end(); ++i1, ++i2) {
    _dbMTerm* t1 = *i1;
    _dbMTerm* t2 = *i2;

    if (strcmp(t1->name_, t2->name_) != 0) {
      break;
    }

    idx_map[t2->order_id_] = t1->order_id_;
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

  // remove reference to inst_hdr
  _dbInstHdr* old_inst_hdr
      = block->inst_hdr_hash_.find(((_dbMaster*) old_master_)->id_);
  old_inst_hdr->inst_cnt_--;

  // delete the old-inst-hdr if required
  if (old_inst_hdr->inst_cnt_ == 0) {
    dbInstHdr::destroy((dbInstHdr*) old_inst_hdr);
  }

  // add reference to new inst_hdr
  _dbInstHdr* new_inst_hdr
      = block->inst_hdr_hash_.find(((_dbMaster*) new_master_)->id_);

  // create a new inst-hdr if needed
  if (new_inst_hdr == nullptr) {
    new_inst_hdr
        = (_dbInstHdr*) dbInstHdr::create((dbBlock*) block, new_master_);
  }

  new_inst_hdr->inst_cnt_++;
  inst->inst_hdr_ = new_inst_hdr->getOID();

  // set new bbox based on new master
  _dbInst::setInstBBox(inst);

  // The next two steps invalidates any dbSet<dbITerm> iterators.

  // 1) update the iterm-mterm-idx
  uint32_t cnt = inst->iterms_.size();

  uint32_t i;
  for (i = 0; i < cnt; ++i) {
    _dbITerm* it = block->iterm_tbl_->getPtr(inst->iterms_[i]);
    uint32_t old_idx = it->flags_.mterm_idx;
    it->flags_.mterm_idx = idx_map[old_idx];
  }

  // 2) reorder the iterms vector
  sortITerm itermCmp(block);
  std::ranges::sort(inst->iterms_, itermCmp);

  // Notification
  for (auto cb : block->callbacks_) {
    cb->inDbInstSwapMasterAfter(this);
  }

  return true;
}

void dbInst::setPinAccessIdx(uint32_t idx)
{
  _dbInst* inst = (_dbInst*) this;
  inst->pin_access_idx_ = idx;
}

uint32_t dbInst::getPinAccessIdx() const
{
  _dbInst* inst = (_dbInst*) this;
  return inst->pin_access_idx_;
}

dbInst* dbInst::create(dbBlock* block,
                       dbMaster* master,
                       const char* name,
                       bool physical_only,
                       dbModule* parent_module)
{
  return create(block, master, name, nullptr, physical_only, parent_module);
}

dbInst* dbInst::create(dbBlock* block_,
                       dbMaster* master_,
                       const char* name_,
                       dbRegion* region,
                       bool physical_only,
                       dbModule* parent_module)
{
  _dbBlock* block = (_dbBlock*) block_;
  if (block->inst_hash_.hasMember(name_)) {
    return nullptr;
  }

  _dbMaster* master = (_dbMaster*) master_;
  _dbInstHdr* inst_hdr = block->inst_hdr_hash_.find(master->id_);
  if (inst_hdr == nullptr) {
    inst_hdr
        = (_dbInstHdr*) dbInstHdr::create((dbBlock*) block, (dbMaster*) master);
  }

  _dbInst* inst_impl = block->inst_tbl_->create();
  dbInst* inst = reinterpret_cast<dbInst*>(inst_impl);

  if (block->journal_) {
    dbLib* lib = master_->getLib();
    block->journal_->beginAction(dbJournal::kCreateObject);
    block->journal_->pushParam(dbInstObj);
    block->journal_->pushParam(lib->getId());
    block->journal_->pushParam(master_->getId());
    block->journal_->pushParam(name_);
    // need to add dbModNet
    // dbModule (scope)
    block->journal_->pushParam(inst_impl->getOID());
    block->journal_->endAction();
  }

  inst_impl->name_ = safe_strdup(name_);
  inst_impl->inst_hdr_ = inst_hdr->getOID();
  block->inst_hash_.insert(inst_impl);
  inst_hdr->inst_cnt_++;

  debugPrint(block->getImpl()->getLogger(),
             utl::ODB,
             "DB_EDIT",
             1,
             "EDIT: create {} master '{}'",
             inst->getDebugName(),
             master_->getName());

  // create the iterms
  uint32_t mterm_cnt = inst_hdr->mterms_.size();
  inst_impl->iterms_.resize(mterm_cnt);

  for (int i = 0; i < mterm_cnt; ++i) {
    _dbITerm* iterm = block->iterm_tbl_->create();
    inst_impl->iterms_[i] = iterm->getOID();
    iterm->flags_.mterm_idx = i;
    iterm->inst_ = inst_impl->getOID();
  }

  _dbBox* box = block->box_tbl_->create();
  box->shape_.rect.init(0, 0, master->width_, master->height_);
  box->flags_.owner_type = dbBoxOwner::INST;
  box->owner_ = inst_impl->getOID();
  inst_impl->bbox_ = box->getOID();

  block->add_rect(box->shape_.rect);

  inst_impl->flags_.physical_only = physical_only;

  // Add the new instance to the parent module.
  bool parent_is_top = parent_module == nullptr || parent_module->isTop();
  if (physical_only == false || parent_is_top) {
    if (parent_module) {
      parent_module->addInst((dbInst*) inst_impl);
    } else {
      block_->getTopModule()->addInst((dbInst*) inst_impl);
    }
  }

  if (region) {
    region->addInst((dbInst*) inst_impl);
  }
  for (dbBlockCallBackObj* cb : block->callbacks_) {
    cb->inDbInstCreate((dbInst*) inst_impl);
  }

  for (int i = 0; i < mterm_cnt; ++i) {
    _dbITerm* iterm = block->iterm_tbl_->getPtr(inst_impl->iterms_[i]);
    for (dbBlockCallBackObj* cb : block->callbacks_) {
      cb->inDbITermCreate((dbITerm*) iterm);
    }
  }

  return (dbInst*) inst_impl;
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

dbInst* dbInst::create(dbBlock* block,
                       dbMaster* master,
                       const char* base_name,
                       const dbNameUniquifyType& uniquify,
                       dbModule* parent_module)
{
  std::string inst_name = block->makeNewInstName(
      parent_module ? parent_module->getModInst() : nullptr,
      base_name,
      uniquify);
  return create(block, master, inst_name.c_str(), false, parent_module);
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
      = ((_dbBlock*) block)->inst_name_id_map_;
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

  if (inst->flags_.dont_touch) {
    inst->getLogger()->error(utl::ODB,
                             362,
                             "Attempt to destroy dont_touch instance {}",
                             inst->name_);
  }

  dbScanInst* scan_inst = inst_->getScanInst();
  if (scan_inst) {
    inst->getLogger()->error(
        utl::ODB,
        505,
        "Attempt to destroy instance {} with an associated scan inst.",
        inst->name_);
  }

  uint32_t i;
  uint32_t n = inst->iterms_.size();

  // Delete these in reverse order so undo creates the in
  // the correct order.
  for (i = 0; i < n; ++i) {
    const int index = n - 1 - i;
    dbId<_dbITerm> id = inst->iterms_[index];
    _dbITerm* _iterm = block->iterm_tbl_->getPtr(id);
    dbITerm* iterm = (dbITerm*) _iterm;
    iterm->disconnect();
    if (inst_->getPinAccessIdx() >= 0) {
      for (const auto& [pin, aps] : iterm->getAccessPoints()) {
        for (auto ap : aps) {
          _dbAccessPoint* _ap = (_dbAccessPoint*) ap;
          auto [first, last] = std::ranges::remove_if(
              _ap->iterms_, [id](const auto& id_in) { return id_in == id; });
          _ap->iterms_.erase(first, last);
        }
      }
    }

    // Notify when pins are deleted (assumption: pins are destroyed only when
    // the related instance is destroyed)
    for (auto cb : block->callbacks_) {
      cb->inDbITermDestroy((dbITerm*) _iterm);
    }

    dbProperty::destroyProperties(_iterm);
    block->iterm_tbl_->destroy(_iterm);
    inst->iterms_[index] = dbId<_dbITerm>();  // clear
  }
  inst->iterms_.clear();

  dbModule* module = inst_->getModule();
  if (module) {
    ((_dbModule*) module)->dbinst_hash_.erase(inst_->getName());
  }

  debugPrint(block->getImpl()->getLogger(),
             utl::ODB,
             "DB_EDIT",
             1,
             "EDIT: delete {}",
             inst->getDebugName());

  if (block->journal_) {
    auto master = inst_->getMaster();
    block->journal_->beginAction(dbJournal::kDeleteObject);
    block->journal_->pushParam(dbInstObj);
    block->journal_->pushParam(master->getLib()->getId());
    block->journal_->pushParam(master->getId());
    block->journal_->pushParam(inst_->getName().c_str());
    block->journal_->pushParam(inst_->getId());
    uint32_t* flags = (uint32_t*) &inst->flags_;
    block->journal_->pushParam(*flags);
    block->journal_->pushParam(inst->x_);
    block->journal_->pushParam(inst->y_);
    block->journal_->pushParam(inst->group_);
    block->journal_->pushParam(inst->module_);
    block->journal_->pushParam(inst->region_);
    block->journal_->endAction();
  }

  dbRegion* region = inst_->getRegion();

  if (region) {
    region->removeInst(inst_);
  }

  if (module) {
    ((_dbModule*) module)->removeInst(inst_);
  }

  if (inst->group_) {
    inst_->getGroup()->removeInst(inst_);
  }

  for (auto cb : block->callbacks_) {
    cb->inDbInstDestroy(inst_);
  }

  _dbMaster* master = (_dbMaster*) inst_->getMaster();
  _dbInstHdr* inst_hdr = block->inst_hdr_hash_.find(master->id_);
  inst_hdr->inst_cnt_--;

  if (inst_hdr->inst_cnt_ == 0) {
    dbInstHdr::destroy((dbInstHdr*) inst_hdr);
  }

  if (inst->halo_) {
    _dbBox* halo = block->box_tbl_->getPtr(inst->halo_);
    dbProperty::destroyProperties(halo);
    block->box_tbl_->destroy(halo);
  }

  _dbBox* box = block->box_tbl_->getPtr(inst->bbox_);
  block->remove_rect(box->shape_.rect);
  block->inst_hash_.remove(inst);
  dbProperty::destroyProperties(inst);
  block->inst_tbl_->destroy(inst);
  dbProperty::destroyProperties(box);
  block->box_tbl_->destroy(box);
}

dbSet<dbInst>::iterator dbInst::destroy(dbSet<dbInst>::iterator& itr)
{
  dbInst* bt = *itr;
  dbSet<dbInst>::iterator next = ++itr;
  destroy(bt);
  return next;
}

dbInst* dbInst::getInst(dbBlock* block_, uint32_t dbid_)
{
  _dbBlock* block = (_dbBlock*) block_;
  return (dbInst*) block->inst_tbl_->getPtr(dbid_);
}

dbInst* dbInst::getValidInst(dbBlock* block_, uint32_t dbid_)
{
  _dbBlock* block = (_dbBlock*) block_;
  if (!block->inst_tbl_->validId(dbid_)) {
    return nullptr;
  }
  return (dbInst*) block->inst_tbl_->getPtr(dbid_);
}

dbITerm* dbInst::getFirstInput() const
{
  for (dbITerm* tr : getITerms()) {
    if (tr->getSigType().isSupply()) {
      continue;
    }

    if (tr->getIoType() != dbIoType::INPUT) {
      continue;
    }

    return tr;
  }
  getImpl()->getLogger()->warn(
      utl::ODB, 172, "instance {} has no input pin", getConstName());
  return nullptr;
}

dbITerm* dbInst::getFirstOutput() const
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

  info.children["name"].add(name_);
  info.children["iterms"].add(iterms_);
}

}  // namespace odb
