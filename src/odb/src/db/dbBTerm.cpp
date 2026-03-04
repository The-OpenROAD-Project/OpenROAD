// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "dbBTerm.h"

#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <optional>
#include <string>

#include "dbBPinItr.h"
#include "dbBlock.h"
#include "dbBox.h"
#include "dbBoxItr.h"
#include "dbChip.h"
#include "dbCommon.h"
#include "dbCore.h"
#include "dbDatabase.h"
#include "dbHier.h"
#include "dbITerm.h"
#include "dbInst.h"
#include "dbInstHdr.h"
#include "dbJournal.h"
#include "dbMTerm.h"
#include "dbMaster.h"
#include "dbModNet.h"
#include "dbNet.h"
#include "dbTable.h"
#include "odb/db.h"
#include "odb/dbBlockCallBackObj.h"
#include "odb/dbObject.h"
#include "odb/dbSet.h"
#include "odb/dbShape.h"
#include "odb/dbTransform.h"
#include "odb/isotropy.h"
#include "utl/Logger.h"

namespace odb {

template class dbTable<_dbBTerm>;

_dbBTerm::_dbBTerm(_dbDatabase*)
{
  // For pointer tagging the bottom 3 bits.
  static_assert(alignof(_dbBTerm) % 8 == 0);
  flags_.io_type = dbIoType::INPUT;
  flags_.sig_type = dbSigType::SIGNAL;
  flags_.orient = 0;
  flags_.status = 0;
  flags_.spef = 0;
  flags_.special = 0;
  flags_.mark = 0;
  flags_.spare_bits = 0;
  ext_id_ = 0;
  name_ = nullptr;
  sta_vertex_id_ = 0;
  constraint_region_.mergeInit();
  is_mirrored_ = false;
}

_dbBTerm::_dbBTerm(_dbDatabase*, const _dbBTerm& b)
    : flags_(b.flags_),
      ext_id_(b.ext_id_),
      name_(nullptr),
      next_entry_(b.next_entry_),
      net_(b.net_),
      next_bterm_(b.next_bterm_),
      prev_bterm_(b.prev_bterm_),
      parent_block_(b.parent_block_),
      parent_iterm_(b.parent_iterm_),
      bpins_(b.bpins_),
      ground_pin_(b.ground_pin_),
      supply_pin_(b.supply_pin_),
      sta_vertex_id_(0),
      constraint_region_(b.constraint_region_)
{
  if (b.name_) {
    name_ = safe_strdup(b.name_);
  }
}

_dbBTerm::~_dbBTerm()
{
  if (name_) {
    free((void*) name_);
  }
}

bool _dbBTerm::operator<(const _dbBTerm& rhs) const
{
  return strcmp(name_, rhs.name_) < 0;
}

bool _dbBTerm::operator==(const _dbBTerm& rhs) const
{
  if (flags_.io_type != rhs.flags_.io_type) {
    return false;
  }

  if (flags_.sig_type != rhs.flags_.sig_type) {
    return false;
  }

  if (flags_.spef != rhs.flags_.spef) {
    return false;
  }

  if (flags_.special != rhs.flags_.special) {
    return false;
  }

  if (ext_id_ != rhs.ext_id_) {
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

  if (net_ != rhs.net_) {
    return false;
  }

  if (next_bterm_ != rhs.next_bterm_) {
    return false;
  }

  if (prev_bterm_ != rhs.prev_bterm_) {
    return false;
  }

  if (parent_block_ != rhs.parent_block_) {
    return false;
  }

  if (parent_iterm_ != rhs.parent_iterm_) {
    return false;
  }

  if (bpins_ != rhs.bpins_) {
    return false;
  }

  if (ground_pin_ != rhs.ground_pin_) {
    return false;
  }

  if (supply_pin_ != rhs.supply_pin_) {
    return false;
  }

  return true;
}

dbOStream& operator<<(dbOStream& stream, const _dbBTerm& bterm)
{
  uint32_t* bit_field = (uint32_t*) &bterm.flags_;
  stream << *bit_field;
  stream << bterm.ext_id_;
  stream << bterm.name_;
  stream << bterm.next_entry_;
  stream << bterm.net_;
  stream << bterm.next_bterm_;
  stream << bterm.prev_bterm_;
  stream << bterm.mnet_;
  stream << bterm.next_modnet_bterm_;
  stream << bterm.prev_modnet_bterm_;
  stream << bterm.parent_block_;
  stream << bterm.parent_iterm_;
  stream << bterm.bpins_;
  stream << bterm.ground_pin_;
  stream << bterm.supply_pin_;
  stream << bterm.constraint_region_;
  stream << bterm.mirrored_bterm_;
  stream << bterm.is_mirrored_;

  return stream;
}

dbIStream& operator>>(dbIStream& stream, _dbBTerm& bterm)
{
  dbBlock* block = (dbBlock*) (bterm.getOwner());
  _dbDatabase* db = (_dbDatabase*) (block->getDataBase());
  uint32_t* bit_field = (uint32_t*) &bterm.flags_;
  stream >> *bit_field;
  stream >> bterm.ext_id_;
  stream >> bterm.name_;
  stream >> bterm.next_entry_;
  stream >> bterm.net_;
  stream >> bterm.next_bterm_;
  stream >> bterm.prev_bterm_;
  if (db->isSchema(kSchemaUpdateHierarchy)) {
    stream >> bterm.mnet_;
    stream >> bterm.next_modnet_bterm_;
    stream >> bterm.prev_modnet_bterm_;
  }
  stream >> bterm.parent_block_;
  stream >> bterm.parent_iterm_;
  stream >> bterm.bpins_;
  stream >> bterm.ground_pin_;
  stream >> bterm.supply_pin_;
  if (bterm.getDatabase()->isSchema(kSchemaBtermConstraintRegion)) {
    stream >> bterm.constraint_region_;
  }
  if (bterm.getDatabase()->isSchema(kSchemaBtermMirroredPin)) {
    stream >> bterm.mirrored_bterm_;
  }
  if (bterm.getDatabase()->isSchema(kSchemaBtermIsMirrored)) {
    stream >> bterm.is_mirrored_;
  }

  return stream;
}

////////////////////////////////////////////////////////////////////
//
// dbBTerm - Methods
//
////////////////////////////////////////////////////////////////////

std::string dbBTerm::getName() const
{
  _dbBTerm* bterm = (_dbBTerm*) this;
  return bterm->name_;
}

const char* dbBTerm::getConstName() const
{
  _dbBTerm* bterm = (_dbBTerm*) this;
  return bterm->name_;
}

bool dbBTerm::rename(const char* name)
{
  _dbBTerm* bterm = (_dbBTerm*) this;
  _dbBlock* block = (_dbBlock*) getBlock();

  if (block->bterm_hash_.hasMember(name)) {
    return false;
  }

  block->bterm_hash_.remove(bterm);
  free((void*) bterm->name_);
  bterm->name_ = safe_strdup(name);
  block->bterm_hash_.insert(bterm);

  return true;
}

void dbBTerm::setSigType(dbSigType type)
{
  _dbBTerm* bterm = (_dbBTerm*) this;
  _dbBlock* block = (_dbBlock*) getBlock();
  uint32_t prev_flags = flagsToUInt(bterm);

  bterm->flags_.sig_type = type.getValue();

  debugPrint(getImpl()->getLogger(),
             utl::ODB,
             "DB_EDIT",
             2,
             "EDIT: {} setSigType {}",
             bterm->getDebugName(),
             type.getValue());

  if (block->journal_) {
    block->journal_->updateField(
        this, _dbBTerm::kFlags, prev_flags, flagsToUInt(bterm));
  }

  for (auto callback : block->callbacks_) {
    callback->inDbBTermSetSigType(this, type);
  }
}

dbSigType dbBTerm::getSigType() const
{
  _dbBTerm* bterm = (_dbBTerm*) this;
  return dbSigType(bterm->flags_.sig_type);
}

void dbBTerm::setIoType(dbIoType type)
{
  _dbBTerm* bterm = (_dbBTerm*) this;
  _dbBlock* block = (_dbBlock*) getBlock();
  uint32_t prev_flags = flagsToUInt(bterm);

  bterm->flags_.io_type = type.getValue();

  debugPrint(getImpl()->getLogger(),
             utl::ODB,
             "DB_EDIT",
             2,
             "EDIT: {} setIoType {}",
             bterm->getDebugName(),
             type.getValue());

  if (block->journal_) {
    block->journal_->updateField(
        this, _dbBTerm::kFlags, prev_flags, flagsToUInt(bterm));
  }

  for (auto callback : block->callbacks_) {
    callback->inDbBTermSetIoType(this, type);
  }
}

dbIoType dbBTerm::getIoType() const
{
  _dbBTerm* bterm = (_dbBTerm*) this;
  return dbIoType(bterm->flags_.io_type);
}

void dbBTerm::setSpefMark(uint32_t v)
{
  _dbBTerm* bterm = (_dbBTerm*) this;
  bterm->flags_.spef = v;
}
bool dbBTerm::isSetSpefMark()
{
  _dbBTerm* bterm = (_dbBTerm*) this;
  return bterm->flags_.spef > 0 ? true : false;
}
bool dbBTerm::isSpecial() const
{
  _dbBTerm* bterm = (_dbBTerm*) this;
  return bterm->flags_.special > 0 ? true : false;
}
void dbBTerm::setSpecial()
{
  _dbBTerm* bterm = (_dbBTerm*) this;
  bterm->flags_.special = 1;
}
void dbBTerm::setMark(uint32_t v)
{
  _dbBTerm* bterm = (_dbBTerm*) this;
  bterm->flags_.mark = v;
}
bool dbBTerm::isSetMark()
{
  _dbBTerm* bterm = (_dbBTerm*) this;
  return bterm->flags_.mark > 0 ? true : false;
}
void dbBTerm::setExtId(uint32_t v)
{
  _dbBTerm* bterm = (_dbBTerm*) this;
  bterm->ext_id_ = v;
}
uint32_t dbBTerm::getExtId()
{
  _dbBTerm* bterm = (_dbBTerm*) this;
  return bterm->ext_id_;
}

dbNet* dbBTerm::getNet() const
{
  _dbBTerm* bterm = (_dbBTerm*) this;
  if (bterm->net_) {
    _dbBlock* block = (_dbBlock*) getBlock();
    _dbNet* net = block->net_tbl_->getPtr(bterm->net_);
    return (dbNet*) net;
  }
  return nullptr;
}

dbModNet* dbBTerm::getModNet() const
{
  _dbBTerm* bterm = (_dbBTerm*) this;
  if (bterm->mnet_) {
    _dbBlock* block = (_dbBlock*) getBlock();
    _dbModNet* net = block->modnet_tbl_->getPtr(bterm->mnet_);
    return (dbModNet*) net;
  }
  return nullptr;
}

void dbBTerm::connect(dbNet* db_net, dbModNet* modnet)
{
  connect(db_net);
  connect(modnet);
}

void dbBTerm::connect(dbModNet* mod_net)
{
  if (mod_net == nullptr) {
    return;
  }

  dbModule* parent_module = mod_net->getParent();
  _dbBlock* block = (_dbBlock*) (parent_module->getOwner());
  _dbModNet* _mod_net = (_dbModNet*) mod_net;
  _dbBTerm* bterm = (_dbBTerm*) this;
  if (bterm->mnet_ == _mod_net->getId()) {
    return;
  }
  if (bterm->mnet_) {
    bterm->disconnectModNet(bterm, block);
  }
  bterm->connectModNet(_mod_net, block);
}

void dbBTerm::connect(dbNet* net_)
{
  if (net_ == nullptr) {
    return;
  }

  _dbBTerm* bterm = (_dbBTerm*) this;
  _dbNet* net = (_dbNet*) net_;
  _dbBlock* block = (_dbBlock*) net->getOwner();

  // Same net. Nothing to connect.
  if (bterm->net_ == net_->getId()) {
    return;
  }

  if (net->flags_.dont_touch) {
    net->getLogger()->error(utl::ODB,
                            377,
                            "Attempt to connect bterm to dont_touch net {}",
                            net->name_);
  }

  if (bterm->net_) {
    disconnectDbNet();
  }
  bterm->connectNet(net, block);
}

void dbBTerm::disconnect()
{
  _dbBTerm* bterm = (_dbBTerm*) this;
  if (bterm->net_) {
    _dbBlock* block = (_dbBlock*) bterm->getOwner();

    _dbNet* net = block->net_tbl_->getPtr(bterm->net_);
    if (net->flags_.dont_touch) {
      net->getLogger()->error(
          utl::ODB,
          375,
          "Attempt to disconnect bterm of dont_touch net {}",
          net->name_);
    }
    bterm->disconnectNet(bterm, block);
    if (bterm->mnet_) {
      bterm->disconnectModNet(bterm, block);
    }
  }
}

void dbBTerm::disconnectDbModNet()
{
  _dbBTerm* bterm = (_dbBTerm*) this;
  _dbBlock* block = (_dbBlock*) bterm->getOwner();
  bterm->disconnectModNet(bterm, block);
}

void dbBTerm::disconnectDbNet()
{
  _dbBTerm* bterm = (_dbBTerm*) this;
  if (bterm->net_) {
    _dbBlock* block = (_dbBlock*) bterm->getOwner();

    _dbNet* net = block->net_tbl_->getPtr(bterm->net_);
    if (net->flags_.dont_touch) {
      net->getLogger()->error(
          utl::ODB,
          1106,
          "Attempt to disconnect bterm of dont_touch net {}",
          net->name_);
    }
    bterm->disconnectNet(bterm, block);
  }
}

dbSet<dbBPin> dbBTerm::getBPins() const
{
  _dbBlock* block = (_dbBlock*) getBlock();
  return dbSet<dbBPin>(const_cast<dbBTerm*>(this), block->bpin_itr_);
}

dbITerm* dbBTerm::getITerm()
{
  _dbBTerm* bterm = (_dbBTerm*) this;
  _dbBlock* block = (_dbBlock*) getBlock();

  if (bterm->parent_block_ == 0) {
    return nullptr;
  }

  _dbChip* chip = (_dbChip*) block->getOwner();
  _dbBlock* parent = chip->block_tbl_->getPtr(bterm->parent_block_);
  return (dbITerm*) parent->iterm_tbl_->getPtr(bterm->parent_iterm_);
}

dbBlock* dbBTerm::getBlock() const
{
  return (dbBlock*) getImpl()->getOwner();
}

Rect dbBTerm::getBBox()
{
  Rect bbox;
  bbox.mergeInit();
  for (dbBPin* pin : getBPins()) {
    bbox.merge(pin->getBBox());
  }
  return bbox;
}

bool dbBTerm::getFirstPin(dbShape& shape)
{
  for (dbBPin* bpin : getBPins()) {
    for (dbBox* box : bpin->getBoxes()) {
      if (bpin->getPlacementStatus() == dbPlacementStatus::UNPLACED
          || bpin->getPlacementStatus() == dbPlacementStatus::NONE
          || box == nullptr) {
        continue;
      }

      if (box->isVia()) {  // This is not possible...
        continue;
      }

      Rect r = box->getBox();
      shape.setSegment(box->getTechLayer(), r);
      return true;
    }
  }

  return false;
}

dbPlacementStatus dbBTerm::getFirstPinPlacementStatus()
{
  dbSet<dbBPin> bpins = getBPins();
  if (bpins.empty()) {
    return dbPlacementStatus::NONE;
  }
  return bpins.begin()->getPlacementStatus();
}

bool dbBTerm::getFirstPinLocation(int& x, int& y) const
{
  for (dbBPin* bpin : getBPins()) {
    for (dbBox* box : bpin->getBoxes()) {
      if (bpin->getPlacementStatus() == dbPlacementStatus::UNPLACED
          || bpin->getPlacementStatus() == dbPlacementStatus::NONE
          || box == nullptr) {
        continue;
      }

      if (box->isVia()) {  // This is not possible...
        continue;
      }

      Rect r = box->getBox();
      x = r.xMin() + (int) (r.dx() >> 1U);
      y = r.yMin() + (int) (r.dy() >> 1U);
      return true;
    }
  }

  x = 0;
  y = 0;
  return false;
}

dbBTerm* dbBTerm::getGroundPin()
{
  _dbBTerm* bterm = (_dbBTerm*) this;
  _dbBlock* block = (_dbBlock*) getBlock();

  if (bterm->ground_pin_ == 0) {
    return nullptr;
  }

  _dbBTerm* ground = block->bterm_tbl_->getPtr(bterm->ground_pin_);
  return (dbBTerm*) ground;
}

void dbBTerm::setGroundPin(dbBTerm* pin)
{
  _dbBTerm* bterm = (_dbBTerm*) this;
  bterm->ground_pin_ = pin->getImpl()->getOID();
}

dbBTerm* dbBTerm::getSupplyPin()
{
  _dbBTerm* bterm = (_dbBTerm*) this;
  _dbBlock* block = (_dbBlock*) getBlock();

  if (bterm->supply_pin_ == 0) {
    return nullptr;
  }

  _dbBTerm* supply = block->bterm_tbl_->getPtr(bterm->supply_pin_);
  return (dbBTerm*) supply;
}

void dbBTerm::setSupplyPin(dbBTerm* pin)
{
  _dbBTerm* bterm = (_dbBTerm*) this;
  bterm->supply_pin_ = pin->getImpl()->getOID();
}

dbBTerm* dbBTerm::create(dbNet* net_, const char* name)
{
  _dbNet* net = (_dbNet*) net_;
  _dbBlock* block = (_dbBlock*) net->getOwner();

  if (block->bterm_hash_.hasMember(name)) {
    return nullptr;
  }

  if (net->flags_.dont_touch) {
    net->getLogger()->error(utl::ODB,
                            376,
                            "Attempt to create bterm on dont_touch net {}",
                            net->name_);
  }

  if (block->journal_) {
    block->journal_->beginAction(dbJournal::kCreateObject);
    block->journal_->pushParam(dbBTermObj);
    block->journal_->pushParam(net->getId());
    block->journal_->pushParam(name);
    block->journal_->endAction();
  }

  _dbBTerm* bterm = block->bterm_tbl_->create();
  bterm->name_ = safe_strdup(name);
  block->bterm_hash_.insert(bterm);

  debugPrint(block->getImpl()->getLogger(),
             utl::ODB,
             "DB_EDIT",
             1,
             "EDIT: create {} on {}",
             bterm->getDebugName(),
             net->getDebugName());

  // If there is a parentInst then we need to update the dbMaster's
  // mterms, the parent dbInst's iterms, and the dbHier to match
  dbBlock* block_public = (dbBlock*) block;
  if (dbInst* inst = block_public->getParentInst()) {
    _dbBlock* parent_block = (_dbBlock*) inst->getBlock();
    dbMaster* master = inst->getMaster();
    _dbMaster* master_impl = (_dbMaster*) master;
    _dbInstHdr* inst_hdr = parent_block->inst_hdr_hash_.find(master_impl->id_);
    assert(inst_hdr->inst_cnt_ == 1);

    master_impl->flags_.frozen = 0;  // allow the mterm creation
    auto mterm = (_dbMTerm*) dbMTerm::create(master, name, dbIoType::INOUT);
    master_impl->flags_.frozen = 1;
    mterm->order_id_ = inst_hdr->mterms_.size();
    inst_hdr->mterms_.push_back(mterm->getOID());

    _dbInst* inst_impl = (_dbInst*) inst;
    _dbHier* hier = parent_block->hier_tbl_->getPtr(inst_impl->hierarchy_);
    hier->child_bterms_.push_back(bterm->getOID());

    _dbITerm* iterm = parent_block->iterm_tbl_->create();
    inst_impl->iterms_.push_back(iterm->getOID());
    iterm->flags_.mterm_idx = mterm->order_id_;
    iterm->inst_ = inst_impl->getOID();

    bterm->parent_block_ = parent_block->getOID();
    bterm->parent_iterm_ = inst_impl->iterms_[mterm->order_id_];
  }

  for (auto callback : block->callbacks_) {
    callback->inDbBTermCreate((dbBTerm*) bterm);
  }

  bterm->connectNet(net, block);

  return (dbBTerm*) bterm;
}

void _dbBTerm::connectModNet(_dbModNet* mod_net, _dbBlock* block)
{
  _dbBTerm* bterm = (_dbBTerm*) this;

  mnet_ = mod_net->getOID();

  debugPrint(block->getImpl()->getLogger(),
             utl::ODB,
             "DB_EDIT",
             1,
             "EDIT: connect {} to {}",
             bterm->getDebugName(),
             mod_net->getDebugName());

  if (block->journal_) {
    block->journal_->beginAction(dbJournal::kConnectObject);
    block->journal_->pushParam(dbBTermObj);
    block->journal_->pushParam(bterm->getId());
    // the flat net is left out
    block->journal_->pushParam(0U);
    // modnet
    block->journal_->pushParam(mod_net->getId());
    block->journal_->endAction();
  }

  if (mod_net->bterms_ != 0) {
    _dbBTerm* head = block->bterm_tbl_->getPtr(mod_net->bterms_);
    next_modnet_bterm_ = mod_net->bterms_;
    head->prev_modnet_bterm_ = getOID();
  } else {
    next_modnet_bterm_ = 0;
  }
  prev_modnet_bterm_ = 0;
  mod_net->bterms_ = getOID();
}

void _dbBTerm::connectNet(_dbNet* net, _dbBlock* block)
{
  _dbBTerm* bterm = (_dbBTerm*) this;

  debugPrint(block->getImpl()->getLogger(),
             utl::ODB,
             "DB_EDIT",
             1,
             "EDIT: connect {} to {}",
             bterm->getDebugName(),
             net->getDebugName());

  if (block->journal_) {
    block->journal_->beginAction(dbJournal::kConnectObject);
    block->journal_->pushParam(dbBTermObj);
    block->journal_->pushParam(bterm->getId());
    block->journal_->pushParam(net->getId());
    // modnet is left out, only flat net.
    block->journal_->pushParam(0U);
    block->journal_->endAction();
  }

  for (auto callback : block->callbacks_) {
    callback->inDbBTermPreConnect((dbBTerm*) this, (dbNet*) net);
  }
  net_ = net->getOID();
  if (net->bterms_ != 0) {
    _dbBTerm* tail = block->bterm_tbl_->getPtr(net->bterms_);
    next_bterm_ = net->bterms_;
    tail->prev_bterm_ = getOID();
  } else {
    next_bterm_ = 0;
  }
  prev_bterm_ = 0;
  net->bterms_ = getOID();
  for (auto callback : block->callbacks_) {
    callback->inDbBTermPostConnect((dbBTerm*) this);
  }
}

void dbBTerm::destroy(dbBTerm* bterm_)
{
  _dbBTerm* bterm = (_dbBTerm*) bterm_;
  _dbBlock* block = (_dbBlock*) bterm->getOwner();

  if (bterm->net_) {
    _dbNet* net = block->net_tbl_->getPtr(bterm->net_);
    if (net->flags_.dont_touch) {
      net->getLogger()->error(utl::ODB,
                              374,
                              "Attempt to destroy bterm on dont_touch net {}",
                              net->name_);
    }
  }

  // delete bpins
  dbSet<dbBPin> bpins = bterm_->getBPins();
  dbSet<dbBPin>::iterator itr;

  for (itr = bpins.begin(); itr != bpins.end();) {
    itr = dbBPin::destroy(itr);
  }
  if (bterm->net_) {
    bterm->disconnectNet(bterm, block);
  }
  for (auto callback : block->callbacks_) {
    callback->inDbBTermDestroy(bterm_);
  }

  debugPrint(block->getImpl()->getLogger(),
             utl::ODB,
             "DB_EDIT",
             1,
             "EDIT: delete {}",
             bterm->getDebugName());

  if (block->journal_) {
    block->journal_->beginAction(dbJournal::kDeleteObject);
    block->journal_->pushParam(dbBTermObj);
    block->journal_->pushParam(bterm_->getId());
    block->journal_->endAction();
  }

  block->bterm_hash_.remove(bterm);
  dbProperty::destroyProperties(bterm);
  block->bterm_tbl_->destroy(bterm);
}

void _dbBTerm::disconnectNet(_dbBTerm* bterm, _dbBlock* block)
{
  if (bterm->net_) {
    _dbNet* net = block->net_tbl_->getPtr(bterm->net_);

    debugPrint(block->getImpl()->getLogger(),
               utl::ODB,
               "DB_EDIT",
               1,
               "EDIT: disconnect {} from {}",
               bterm->getDebugName(),
               net->getDebugName());

    // Journal
    if (block->journal_) {
      block->journal_->beginAction(dbJournal::kDisconnectObject);
      block->journal_->pushParam(dbBTermObj);
      block->journal_->pushParam(bterm->getId());
      block->journal_->pushParam(net->getId());
      block->journal_->pushParam(0U);  // no modnet
      block->journal_->endAction();
    }

    // unlink bterm from the net
    for (auto callback : block->callbacks_) {
      callback->inDbBTermPreDisconnect((dbBTerm*) this);
    }

    uint32_t id = bterm->getOID();

    if (net->bterms_ == id) {
      net->bterms_ = bterm->next_bterm_;

      if (net->bterms_ != 0) {
        _dbBTerm* t = block->bterm_tbl_->getPtr(net->bterms_);
        t->prev_bterm_ = 0;
      }
    } else {
      if (bterm->next_bterm_ != 0) {
        _dbBTerm* next = block->bterm_tbl_->getPtr(bterm->next_bterm_);
        next->prev_bterm_ = bterm->prev_bterm_;
      }

      if (bterm->prev_bterm_ != 0) {
        _dbBTerm* prev = block->bterm_tbl_->getPtr(bterm->prev_bterm_);
        prev->next_bterm_ = bterm->next_bterm_;
      }
    }
    net_ = 0;
    for (auto callback : block->callbacks_) {
      callback->inDbBTermPostDisConnect((dbBTerm*) this, (dbNet*) net);
    }
  }
}

void _dbBTerm::disconnectModNet(_dbBTerm* bterm, _dbBlock* block)
{
  if (bterm->mnet_) {
    _dbModNet* mod_net = block->modnet_tbl_->getPtr(bterm->mnet_);

    debugPrint(block->getImpl()->getLogger(),
               utl::ODB,
               "DB_EDIT",
               1,
               "EDIT: disconnect {} from {}",
               bterm->getDebugName(),
               mod_net->getDebugName());

    if (block->journal_) {
      block->journal_->beginAction(dbJournal::kDisconnectObject);
      block->journal_->pushParam(dbBTermObj);
      block->journal_->pushParam(bterm->getId());
      // we are not considering the dbNet
      block->journal_->pushParam(0U);
      block->journal_->pushParam(mod_net->getId());
      block->journal_->endAction();
    }

    uint32_t id = bterm->getOID();
    if (mod_net->bterms_ == id) {
      mod_net->bterms_ = bterm->next_modnet_bterm_;
      if (mod_net->bterms_ != 0) {
        _dbBTerm* t = block->bterm_tbl_->getPtr(mod_net->bterms_);
        t->prev_modnet_bterm_ = 0;
      }
    } else {
      if (bterm->next_modnet_bterm_ != 0) {
        _dbBTerm* next = block->bterm_tbl_->getPtr(bterm->next_modnet_bterm_);
        next->prev_modnet_bterm_ = bterm->prev_modnet_bterm_;
      }
      if (bterm->prev_modnet_bterm_ != 0) {
        _dbBTerm* prev = block->bterm_tbl_->getPtr(bterm->prev_modnet_bterm_);
        prev->next_modnet_bterm_ = bterm->next_modnet_bterm_;
      }
    }

    next_modnet_bterm_ = 0;
    prev_modnet_bterm_ = 0;
    mnet_ = 0;
  }
}

void _dbBTerm::setMirroredConstraintRegion(const Rect& region, _dbBlock* block)
{
  _dbBTerm* bterm = (_dbBTerm*) this;
  const Rect& die_bounds = ((dbBlock*) block)->getDieArea();
  int begin = region.dx() == 0 ? region.yMin() : region.xMin();
  int end = region.dx() == 0 ? region.yMax() : region.xMax();
  Direction2D edge;
  if (region.dx() == 0) {
    edge = region.xMin() == die_bounds.xMin() ? west : east;
  } else {
    edge = region.yMin() == die_bounds.yMin() ? south : north;
  }
  const Rect mirrored_region
      = ((dbBlock*) block)->findConstraintRegion(edge, begin, end);
  bterm->constraint_region_ = mirrored_region;
}

void _dbBTerm::collectMemInfo(MemInfo& info)
{
  info.cnt++;
  info.size += sizeof(*this);

  info.children["name"].add(name_);
}

dbSet<dbBTerm>::iterator dbBTerm::destroy(dbSet<dbBTerm>::iterator& itr)
{
  dbBTerm* bt = *itr;
  dbSet<dbBTerm>::iterator next = ++itr;
  destroy(bt);
  return next;
}

dbBTerm* dbBTerm::getBTerm(dbBlock* block_, uint32_t oid)
{
  _dbBlock* block = (_dbBlock*) block_;
  return (dbBTerm*) block->bterm_tbl_->getPtr(oid);
}

uint32_t dbBTerm::staVertexId()
{
  _dbBTerm* iterm = (_dbBTerm*) this;
  return iterm->sta_vertex_id_;
}

void dbBTerm::staSetVertexId(uint32_t id)
{
  _dbBTerm* iterm = (_dbBTerm*) this;
  iterm->sta_vertex_id_ = id;
}

void dbBTerm::setConstraintRegion(const Rect& constraint_region)
{
  _dbBTerm* bterm = (_dbBTerm*) this;
  bterm->constraint_region_ = constraint_region;

  dbBTerm* mirrored_bterm = getMirroredBTerm();
  if (mirrored_bterm != nullptr && !bterm->constraint_region_.isInverted()
      && mirrored_bterm->getConstraintRegion() == std::nullopt) {
    _dbBlock* block = (_dbBlock*) getBlock();
    _dbBTerm* mirrored = (_dbBTerm*) mirrored_bterm;
    mirrored->setMirroredConstraintRegion(bterm->constraint_region_, block);
  }
}

std::optional<Rect> dbBTerm::getConstraintRegion()
{
  _dbBTerm* bterm = (_dbBTerm*) this;
  const auto& constraint_region = bterm->constraint_region_;
  if (constraint_region.isInverted()) {
    return std::nullopt;
  }

  return bterm->constraint_region_;
}

void dbBTerm::resetConstraintRegion()
{
  _dbBTerm* bterm = (_dbBTerm*) this;
  bterm->constraint_region_.mergeInit();
}

void dbBTerm::setMirroredBTerm(dbBTerm* mirrored_bterm)
{
  _dbBTerm* bterm = (_dbBTerm*) this;

  bterm->mirrored_bterm_ = mirrored_bterm->getImpl()->getOID();
  _dbBTerm* mirrored = (_dbBTerm*) mirrored_bterm;
  mirrored->is_mirrored_ = true;
  mirrored->mirrored_bterm_ = bterm->getImpl()->getOID();

  if (!bterm->constraint_region_.isInverted()
      && mirrored_bterm->getConstraintRegion() == std::nullopt) {
    _dbBlock* block = (_dbBlock*) getBlock();
    mirrored->setMirroredConstraintRegion(bterm->constraint_region_, block);
  } else if (mirrored_bterm->getConstraintRegion() != std::nullopt) {
    getImpl()->getLogger()->warn(utl::ODB,
                                 26,
                                 "Pin {} is mirrored with another pin. The "
                                 "constraint for this pin will be dropped.",
                                 mirrored_bterm->getName());
  }
}

dbBTerm* dbBTerm::getMirroredBTerm()
{
  _dbBTerm* bterm = (_dbBTerm*) this;
  _dbBlock* block = (_dbBlock*) getBlock();

  if (bterm->mirrored_bterm_ == 0) {
    return nullptr;
  }

  _dbBTerm* mirrored_bterm = block->bterm_tbl_->getPtr(bterm->mirrored_bterm_);
  return (dbBTerm*) mirrored_bterm;
}

bool dbBTerm::hasMirroredBTerm()
{
  _dbBTerm* bterm = (_dbBTerm*) this;
  return bterm->mirrored_bterm_ != 0 && !bterm->is_mirrored_;
}

bool dbBTerm::isMirrored()
{
  _dbBTerm* bterm = (_dbBTerm*) this;
  return bterm->is_mirrored_;
}

}  // namespace odb
