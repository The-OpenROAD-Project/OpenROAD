// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "dbITerm.h"

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <map>
#include <utility>
#include <vector>

#include "dbAccessPoint.h"
#include "dbBTerm.h"
#include "dbBlock.h"
#include "dbChip.h"
#include "dbCommon.h"
#include "dbCore.h"
#include "dbDatabase.h"
#include "dbHier.h"
#include "dbInst.h"
#include "dbInstHdr.h"
#include "dbJournal.h"
#include "dbLib.h"
#include "dbMPin.h"
#include "dbMTerm.h"
#include "dbMaster.h"
#include "dbModNet.h"
#include "dbNet.h"
#include "dbTable.h"
#include "odb/db.h"
#include "odb/dbBlockCallBackObj.h"
#include "odb/dbObject.h"
#include "odb/dbShape.h"
#include "odb/geom.h"
#include "utl/Logger.h"
namespace odb {

template class dbTable<_dbITerm>;

bool _dbITerm::operator==(const _dbITerm& rhs) const
{
  if (flags_.mterm_idx != rhs.flags_.mterm_idx) {
    return false;
  }

  if (flags_.spef != rhs.flags_.spef) {
    return false;
  }

  if (flags_.special != rhs.flags_.special) {
    return false;
  }

  if (flags_.connected != rhs.flags_.connected) {
    return false;
  }

  if (ext_id_ != rhs.ext_id_) {
    return false;
  }

  if (net_ != rhs.net_) {
    return false;
  }

  if (inst_ != rhs.inst_) {
    return false;
  }

  if (next_net_iterm_ != rhs.next_net_iterm_) {
    return false;
  }

  if (prev_net_iterm_ != rhs.prev_net_iterm_) {
    return false;
  }

  if (aps_ != rhs.aps_) {
    return false;
  }

  return true;
}

bool _dbITerm::operator<(const _dbITerm& rhs) const
{
  _dbBlock* lhs_blk = (_dbBlock*) getOwner();
  _dbBlock* rhs_blk = (_dbBlock*) rhs.getOwner();

  _dbInst* lhs_inst = lhs_blk->inst_tbl_->getPtr(inst_);
  _dbInst* rhs_inst = rhs_blk->inst_tbl_->getPtr(rhs.inst_);
  int r = strcmp(lhs_inst->name_, rhs_inst->name_);

  if (r < 0) {
    return true;
  }

  if (r > 0) {
    return false;
  }

  _dbMTerm* lhs_mterm = getMTerm();
  _dbMTerm* rhs_mterm = rhs.getMTerm();
  return strcmp(lhs_mterm->name_, rhs_mterm->name_) < 0;
}

_dbMTerm* _dbITerm::getMTerm() const
{
  _dbBlock* block = (_dbBlock*) getOwner();
  _dbInst* inst = block->inst_tbl_->getPtr(inst_);
  _dbInstHdr* inst_hdr = block->inst_hdr_tbl_->getPtr(inst->inst_hdr_);
  _dbDatabase* db = getDatabase();
  _dbLib* lib = db->lib_tbl_->getPtr(inst_hdr->lib_);
  _dbMaster* master = lib->master_tbl_->getPtr(inst_hdr->master_);
  dbId<_dbMTerm> mterm = inst_hdr->mterms_[flags_.mterm_idx];
  return master->mterm_tbl_->getPtr(mterm);
}

_dbInst* _dbITerm::getInst() const
{
  _dbBlock* block = (_dbBlock*) getOwner();
  _dbInst* inst = block->inst_tbl_->getPtr(inst_);
  return inst;
}

////////////////////////////////////////////////////////////////////
//
// dbITerm - Methods
//
////////////////////////////////////////////////////////////////////

dbInst* dbITerm::getInst() const
{
  _dbITerm* iterm = (_dbITerm*) this;
  _dbBlock* block = (_dbBlock*) iterm->getOwner();
  _dbInst* inst = block->inst_tbl_->getPtr(iterm->inst_);
  if (inst == nullptr) {
    iterm->getLogger()->critical(
        utl::ODB, 446, "dbITerm does not have dbInst.");
  }
  return (dbInst*) inst;
}

dbNet* dbITerm::getNet() const
{
  _dbITerm* iterm = (_dbITerm*) this;
  _dbBlock* block = (_dbBlock*) iterm->getOwner();

  if (iterm->net_ == 0) {
    return nullptr;
  }

  _dbNet* net = block->net_tbl_->getPtr(iterm->net_);
  return (dbNet*) net;
}

dbMTerm* dbITerm::getMTerm() const
{
  _dbITerm* iterm = (_dbITerm*) this;
  _dbBlock* block = (_dbBlock*) iterm->getOwner();
  _dbInst* inst = block->inst_tbl_->getPtr(iterm->inst_);
  _dbInstHdr* inst_hdr = block->inst_hdr_tbl_->getPtr(inst->inst_hdr_);
  _dbDatabase* db = iterm->getDatabase();
  _dbLib* lib = db->lib_tbl_->getPtr(inst_hdr->lib_);
  _dbMaster* master = lib->master_tbl_->getPtr(inst_hdr->master_);
  dbId<_dbMTerm> mterm = inst_hdr->mterms_[iterm->flags_.mterm_idx];
  return (dbMTerm*) master->mterm_tbl_->getPtr(mterm);
}

dbBTerm* dbITerm::getBTerm()
{
  _dbITerm* iterm = (_dbITerm*) this;
  _dbBlock* block = (_dbBlock*) iterm->getOwner();
  _dbInst* inst = block->inst_tbl_->getPtr(iterm->inst_);

  if (inst->hierarchy_ == 0) {
    return nullptr;
  }

  _dbHier* hier = block->hier_tbl_->getPtr(inst->hierarchy_);

  _dbChip* chip = (_dbChip*) block->getOwner();
  _dbBlock* child = chip->block_tbl_->getPtr(hier->child_block_);
  dbId<_dbBTerm> bterm = hier->child_bterms_[iterm->flags_.mterm_idx];
  return (dbBTerm*) child->bterm_tbl_->getPtr(bterm);
}

std::string dbITerm::getName(const char separator) const
{
  return getInst()->getName() + separator + getMTerm()->getName();
}

dbBlock* dbITerm::getBlock() const
{
  return (dbBlock*) getImpl()->getOwner();
}

void dbITerm::setClocked(bool v)
{
  _dbITerm* iterm = (_dbITerm*) this;
  iterm->flags_.clocked = v;
}

bool dbITerm::isClocked()
{
  bool masterFlag = getMTerm()->getSigType() == dbSigType::CLOCK ? true : false;
  _dbITerm* iterm = (_dbITerm*) this;
  return iterm->flags_.clocked > 0 || masterFlag ? true : false;
}

void dbITerm::setMark(uint32_t v)
{
  _dbITerm* iterm = (_dbITerm*) this;
  iterm->flags_.mark = v;
}

bool dbITerm::isSetMark()
{
  _dbITerm* iterm = (_dbITerm*) this;
  return iterm->flags_.mark > 0 ? true : false;
}

bool dbITerm::isSpecial()
{
  _dbITerm* iterm = (_dbITerm*) this;
  return iterm->flags_.special == 1;
}

void dbITerm::setSpecial()
{
  _dbITerm* iterm = (_dbITerm*) this;
  iterm->flags_.special = 1;
}

void dbITerm::clearSpecial()
{
  _dbITerm* iterm = (_dbITerm*) this;
  iterm->flags_.special = 0;
}

void dbITerm::setSpef(uint32_t v)
{
  _dbITerm* iterm = (_dbITerm*) this;
  iterm->flags_.spef = v;
}

bool dbITerm::isSpef()
{
  _dbITerm* iterm = (_dbITerm*) this;
  return (iterm->flags_.spef > 0) ? true : false;
}

void dbITerm::setExtId(uint32_t v)
{
  _dbITerm* iterm = (_dbITerm*) this;
  iterm->ext_id_ = v;
}

uint32_t dbITerm::getExtId()
{
  _dbITerm* iterm = (_dbITerm*) this;
  return iterm->ext_id_;
}

bool dbITerm::isConnected()
{
  _dbITerm* iterm = (_dbITerm*) this;
  return iterm->flags_.connected == 1;
}

void dbITerm::setConnected()
{
  _dbITerm* iterm = (_dbITerm*) this;
  iterm->flags_.connected = 1;
}

void dbITerm::clearConnected()
{
  _dbITerm* iterm = (_dbITerm*) this;
  iterm->flags_.connected = 0;
}

/*
Warning: this does not do a reassociate. Specifically it will
not make sure that every dbModNet has just one dbNet associated
with it. To assure that, use dbNetwork::connectPin
*/

void dbITerm::connect(dbNet* db_net, dbModNet* db_mod_net)
{
  connect(db_mod_net);
  connect(db_net);
}

void dbITerm::connect(dbNet* net_)
{
  _dbITerm* iterm = (_dbITerm*) this;
  _dbNet* net = (_dbNet*) net_;
  _dbBlock* block = (_dbBlock*) iterm->getOwner();
  _dbInst* inst = iterm->getInst();

  if (!net_) {
    inst->getLogger()->error(
        utl::ODB, 440, "Attempt to connect iterm {} to a null net", getName());
  }

  // Do Nothing if already connected
  if (iterm->net_ == net->getOID()) {
    return;
  }

  if (inst->flags_.dont_touch) {
    inst->getLogger()->error(
        utl::ODB,
        369,
        "Attempt to connect iterm of dont_touch instance {}",
        inst->name_);
  }

  if (net->flags_.dont_touch) {
    inst->getLogger()->error(utl::ODB,
                             373,
                             "Attempt to connect iterm to dont_touch net {}",
                             net->name_);
  }

  if (net_->getBlock() != getInst()->getBlock()) {
    inst->getLogger()->error(utl::ODB,
                             433,
                             "Connecting instances on different dies into "
                             "one net is currently not supported");
  }

  //
  // Note we only disconnect the dbnet part.
  // so we use disconnectDbNet (to blow away
  // both the hierarchical net and the flat net
  // use disconnect() ).
  //
  if (iterm->net_ != 0) {
    disconnectDbNet();
  }

  for (auto callback : block->callbacks_) {
    callback->inDbITermPreConnect(this, net_);
  }

  debugPrint(getImpl()->getLogger(),
             utl::ODB,
             "DB_EDIT",
             1,
             "EDIT: connect {} to {}",
             iterm->getDebugName(),
             net->getDebugName());

  if (block->journal_) {
    block->journal_->beginAction(dbJournal::kConnectObject);
    block->journal_->pushParam(dbITermObj);
    block->journal_->pushParam(getId());
    block->journal_->pushParam(net_->getId());
    // put in a fake modnet here
    block->journal_->pushParam(0);
    block->journal_->endAction();
  }

  iterm->net_ = net->getOID();

  if (net->iterms_ != 0) {
    _dbITerm* tail = block->iterm_tbl_->getPtr(net->iterms_);
    iterm->next_net_iterm_ = net->iterms_;
    iterm->prev_net_iterm_ = 0;
    tail->prev_net_iterm_ = iterm->getOID();
  } else {
    iterm->next_net_iterm_ = 0;
    iterm->prev_net_iterm_ = 0;
  }

  net->iterms_ = iterm->getOID();

  for (auto callback : block->callbacks_) {
    callback->inDbITermPostConnect(this);
  }
}

dbModNet* dbITerm::getModNet() const
{
  _dbITerm* iterm = (_dbITerm*) this;
  _dbBlock* block = (_dbBlock*) iterm->getOwner();
  if (iterm->mnet_ == 0) {
    return nullptr;
  }
  _dbModNet* net = block->modnet_tbl_->getPtr(iterm->mnet_);
  return ((dbModNet*) (net));
}

void dbITerm::connect(dbModNet* mod_net)
{
  _dbITerm* iterm = (_dbITerm*) this;
  _dbModNet* _mod_net = (_dbModNet*) mod_net;
  _dbBlock* block = (_dbBlock*) iterm->getOwner();
  _dbInst* inst = iterm->getInst();

  if (iterm->mnet_ == _mod_net->getId()) {
    return;
  }

  // If already connected, disconnect just the modnet (so we don't
  // accidentally blow away prior flat net connections)

  if (iterm->mnet_ != 0) {
    disconnectDbModNet();
  }

  iterm->mnet_ = _mod_net->getId();

  if (inst->flags_.dont_touch) {
    inst->getLogger()->error(
        utl::ODB,
        397,
        "Attempt to connect iterm of dont_touch instance {}",
        inst->name_);
  }

  debugPrint(iterm->getImpl()->getLogger(),
             utl::ODB,
             "DB_EDIT",
             1,
             "EDIT: connect {} to {}",
             iterm->getDebugName(),
             mod_net->getDebugName());

  if (block->journal_) {
    block->journal_->beginAction(dbJournal::kConnectObject);
    block->journal_->pushParam(dbITermObj);
    block->journal_->pushParam(getId());
    block->journal_->pushParam(0);  // empty slot for dbNet, just dbModNet
    block->journal_->pushParam(_mod_net->getId());
    block->journal_->endAction();
  }

  if (_mod_net->iterms_ != 0) {
    _dbITerm* head = block->iterm_tbl_->getPtr(_mod_net->iterms_);
    iterm->next_modnet_iterm_ = _mod_net->iterms_;
    // prev is this one
    head->prev_modnet_iterm_ = iterm->getOID();
  } else {
    iterm->next_modnet_iterm_ = 0;
  }
  iterm->prev_modnet_iterm_ = 0;
  _mod_net->iterms_ = iterm->getOID();
}

// disconnect both modnet and flat net from an iterm
void dbITerm::disconnect()
{
  _dbITerm* iterm = (_dbITerm*) this;

  if (iterm->net_ == 0 && iterm->mnet_ == 0) {
    return;
  }

  _dbInst* inst = iterm->getInst();
  if (inst->flags_.dont_touch) {
    inst->getLogger()->error(
        utl::ODB,
        370,
        "Attempt to disconnect term {} of dont_touch instance {}",
        getMTerm()->getName(),
        inst->name_);
  }

  _dbBlock* block = (_dbBlock*) iterm->getOwner();
  _dbNet* net
      = iterm->net_ == 0 ? nullptr : block->net_tbl_->getPtr(iterm->net_);
  _dbModNet* mod_net_impl
      = iterm->mnet_ == 0 ? nullptr : block->modnet_tbl_->getPtr(iterm->mnet_);
  dbModNet* mod_net = (dbModNet*) mod_net_impl;

  if (net && net->flags_.dont_touch) {
    inst->getLogger()->error(
        utl::ODB,
        372,
        "Attempt to disconnect iterm {} of dont_touch net {}",
        getName(),
        net->name_);
  }

  debugPrint(getImpl()->getLogger(),
             utl::ODB,
             "DB_EDIT",
             1,
             "EDIT: disconnect {} from {} and {}",
             iterm->getDebugName(),
             (net) ? net->getDebugName() : "dbNet(NULL)",
             (mod_net) ? mod_net->getDebugName() : "dbModNet(NULL)");

  if (block->journal_) {
    block->journal_->beginAction(dbJournal::kDisconnectObject);
    block->journal_->pushParam(dbITermObj);
    block->journal_->pushParam(getId());
    block->journal_->pushParam(net ? net->getOID() : 0U);
    block->journal_->pushParam(mod_net_impl ? mod_net_impl->getOID() : 0U);
    block->journal_->endAction();
  }

  for (auto callback : block->callbacks_) {
    callback->inDbITermPreDisconnect(this);
  }

  uint32_t id = iterm->getOID();

  if (net) {
    if (net->iterms_ == id) {
      net->iterms_ = iterm->next_net_iterm_;
      if (net->iterms_ != 0) {
        _dbITerm* t = block->iterm_tbl_->getPtr(net->iterms_);
        t->prev_net_iterm_ = 0;
      }
    } else {
      if (iterm->next_net_iterm_ != 0) {
        _dbITerm* next = block->iterm_tbl_->getPtr(iterm->next_net_iterm_);
        next->prev_net_iterm_ = iterm->prev_net_iterm_;
      }
      if (iterm->prev_net_iterm_ != 0) {
        _dbITerm* prev = block->iterm_tbl_->getPtr(iterm->prev_net_iterm_);
        prev->next_net_iterm_ = iterm->next_net_iterm_;
      }
    }
    iterm->net_ = 0;
    for (auto callback : block->callbacks_) {
      callback->inDbITermPostDisconnect(this, (dbNet*) net);
    }
  }

  if (mod_net_impl) {
    if (mod_net_impl->iterms_ == id) {
      mod_net_impl->iterms_ = iterm->next_modnet_iterm_;
      if (mod_net_impl->iterms_ != 0) {
        _dbITerm* t = block->iterm_tbl_->getPtr(mod_net_impl->iterms_);
        t->prev_modnet_iterm_ = 0;
      }
    } else {
      if (iterm->next_modnet_iterm_ != 0) {
        _dbITerm* next = block->iterm_tbl_->getPtr(iterm->next_modnet_iterm_);
        next->prev_modnet_iterm_ = iterm->prev_modnet_iterm_;
      }
      if (iterm->prev_modnet_iterm_ != 0) {
        _dbITerm* prev = block->iterm_tbl_->getPtr(iterm->prev_modnet_iterm_);
        prev->next_modnet_iterm_ = iterm->next_modnet_iterm_;
      }
    }
    iterm->mnet_ = 0;
  }
}

// disconnect the dbNetonly and allow journalling

void dbITerm::disconnectDbNet()
{
  _dbITerm* iterm = (_dbITerm*) this;

  if (iterm->net_ == 0) {
    return;
  }

  _dbInst* inst = iterm->getInst();
  if (inst->flags_.dont_touch) {
    inst->getLogger()->error(
        utl::ODB,
        1104,
        "Attempt to disconnect term {} of dont_touch instance {}",
        getMTerm()->getName(),
        inst->name_);
  }
  _dbBlock* block = (_dbBlock*) iterm->getOwner();
  _dbNet* net = block->net_tbl_->getPtr(iterm->net_);

  if (net->flags_.dont_touch) {
    inst->getLogger()->error(
        utl::ODB,
        1105,
        "Attempt to disconnect iterm {} of dont_touch net {}",
        getName(),
        net->name_);
  }

  for (auto callback : block->callbacks_) {
    callback->inDbITermPreDisconnect(this);
  }

  debugPrint(iterm->getImpl()->getLogger(),
             utl::ODB,
             "DB_EDIT",
             1,
             "EDIT: disconnect {} from {}",
             iterm->getDebugName(),
             net->getDebugName());

  if (block->journal_) {
    block->journal_->beginAction(dbJournal::kDisconnectObject);
    block->journal_->pushParam(dbITermObj);
    block->journal_->pushParam(getId());
    block->journal_->pushParam(net->getOID());
    // Note we don't remove the mod net part here, just stub
    // out the modnet id.
    block->journal_->pushParam(0);
    block->journal_->endAction();
  }

  uint32_t id = iterm->getOID();

  if (net->iterms_ == id) {
    net->iterms_ = iterm->next_net_iterm_;
    if (net->iterms_ != 0) {
      _dbITerm* t = block->iterm_tbl_->getPtr(net->iterms_);
      t->prev_net_iterm_ = 0;
    }
  } else {
    if (iterm->next_net_iterm_ != 0) {
      _dbITerm* next = block->iterm_tbl_->getPtr(iterm->next_net_iterm_);
      next->prev_net_iterm_ = iterm->prev_net_iterm_;
    }
    if (iterm->prev_net_iterm_ != 0) {
      _dbITerm* prev = block->iterm_tbl_->getPtr(iterm->prev_net_iterm_);
      prev->next_net_iterm_ = iterm->next_net_iterm_;
    }
  }
  iterm->net_ = 0;
  for (auto callback : block->callbacks_) {
    callback->inDbITermPostDisconnect(this, (dbNet*) net);
  }
}

//
// Disconnect the mod net and allow journaling
//
void dbITerm::disconnectDbModNet()
{
  _dbITerm* iterm = (_dbITerm*) this;
  _dbBlock* block = (_dbBlock*) iterm->getOwner();

  if (iterm->mnet_ != 0) {
    _dbModNet* mod_net = block->modnet_tbl_->getPtr(iterm->mnet_);

    debugPrint(iterm->getImpl()->getLogger(),
               utl::ODB,
               "DB_EDIT",
               1,
               "EDIT: disconnect {} from {}",
               iterm->getDebugName(),
               mod_net->getDebugName());

    if (block->journal_) {
      block->journal_->beginAction(dbJournal::kDisconnectObject);
      block->journal_->pushParam(dbITermObj);
      block->journal_->pushParam(getId());
      // empty dbNet part, just the mod net being undone
      block->journal_->pushParam(0);                  // no dbNet id
      block->journal_->pushParam(mod_net->getOID());  // the modnet
      block->journal_->endAction();
    }

    if (mod_net->iterms_ == getId()) {
      mod_net->iterms_ = iterm->next_modnet_iterm_;
      if (mod_net->iterms_ != 0) {
        _dbITerm* t = block->iterm_tbl_->getPtr(mod_net->iterms_);
        t->prev_modnet_iterm_ = 0;
      }
    } else {
      if (iterm->next_modnet_iterm_ != 0) {
        _dbITerm* next = block->iterm_tbl_->getPtr(iterm->next_modnet_iterm_);
        next->prev_modnet_iterm_ = iterm->prev_modnet_iterm_;
      }
      if (iterm->prev_modnet_iterm_ != 0) {
        _dbITerm* prev = block->iterm_tbl_->getPtr(iterm->prev_modnet_iterm_);
        prev->next_modnet_iterm_ = iterm->next_modnet_iterm_;
      }
    }

    iterm->next_modnet_iterm_ = 0;
    iterm->prev_modnet_iterm_ = 0;
    iterm->mnet_ = 0;
  }
}

dbSigType dbITerm::getSigType() const
{
  _dbMTerm* mterm = (_dbMTerm*) getMTerm();
  return dbSigType(mterm->flags_.sig_type);
}
dbIoType dbITerm::getIoType() const
{
  _dbMTerm* mterm = (_dbMTerm*) getMTerm();
  return dbIoType(mterm->flags_.io_type);
}
bool dbITerm::isOutputSignal(bool io)
{
  _dbMTerm* mterm = (_dbMTerm*) getMTerm();
  dbSigType sType = dbSigType(mterm->flags_.sig_type);
  dbIoType ioType = dbIoType(mterm->flags_.io_type);

  if ((sType == dbSigType::GROUND) || (sType == dbSigType::POWER)) {
    return false;
  }

  if (ioType == dbIoType::OUTPUT) {
    return true;
  }

  if (io && (ioType == dbIoType::INOUT)) {
    return true;
  }

  return false;
}
bool dbITerm::isInputSignal(bool io)
{
  _dbMTerm* mterm = (_dbMTerm*) getMTerm();
  dbSigType sType = dbSigType(mterm->flags_.sig_type);
  dbIoType ioType = dbIoType(mterm->flags_.io_type);

  if ((sType == dbSigType::GROUND) || (sType == dbSigType::POWER)) {
    return false;
  }

  if (ioType == dbIoType::INPUT) {
    return true;
  }

  if (io && (ioType == dbIoType::INOUT)) {
    return true;
  }

  return false;
}

dbITerm* dbITerm::getITerm(dbBlock* block_, uint32_t dbid)
{
  _dbBlock* block = (_dbBlock*) block_;
  return (dbITerm*) block->iterm_tbl_->getPtr(dbid);
}

Rect dbITerm::getBBox()
{
  dbMTerm* term = getMTerm();
  Rect bbox = term->getBBox();
  const odb::dbTransform inst_xfm = getInst()->getTransform();
  inst_xfm.apply(bbox);
  return bbox;
}

bool dbITerm::getAvgXY(int* x, int* y) const
{
  dbMTerm* mterm = getMTerm();
  int nn = 0;
  double xx = 0.0;
  double yy = 0.0;
  dbInst* inst = getInst();
  const dbTransform transform = inst->getTransform();

  for (dbMPin* mpin : mterm->getMPins()) {
    for (dbBox* box : mpin->getGeometry()) {
      Rect rect = box->getBox();
      transform.apply(rect);
      xx += rect.xMin() + rect.xMax();
      yy += rect.yMin() + rect.yMax();
      nn += 2;
    }
  }
  if (!nn) {
    getImpl()->getLogger()->warn(
        utl::ODB,
        34,
        "Can not find physical location of iterm {}/{}",
        getInst()->getConstName(),
        getMTerm()->getConstName());
    return false;
  }
  xx /= nn;
  yy /= nn;
  *x = int(xx);
  *y = int(yy);
  return true;
}

uint32_t dbITerm::staVertexId()
{
  _dbITerm* iterm = (_dbITerm*) this;
  return iterm->sta_vertex_id_;
}

void dbITerm::staSetVertexId(uint32_t id)
{
  _dbITerm* iterm = (_dbITerm*) this;
  iterm->sta_vertex_id_ = id;
}

void dbITerm::setAccessPoint(dbMPin* pin, dbAccessPoint* ap)
{
  _dbITerm* iterm = (_dbITerm*) this;
  if (ap != nullptr) {
    iterm->aps_[pin->getImpl()->getOID()] = ap->getImpl()->getOID();
    _dbAccessPoint* _ap = (_dbAccessPoint*) ap;
    _ap->iterms_.push_back(iterm->getOID());
  } else {
    iterm->aps_[pin->getImpl()->getOID()] = dbId<_dbAccessPoint>();
  }

  _dbBlock* block = (_dbBlock*) iterm->getOwner();
  for (auto callback : block->callbacks_) {
    callback->inDbITermPostSetAccessPoints(this);
  }
}

std::map<dbMPin*, std::vector<dbAccessPoint*>> dbITerm::getAccessPoints() const
{
  _dbBlock* block = (_dbBlock*) getBlock();
  auto mterm = getMTerm();
  uint32_t pin_access_idx = getInst()->getPinAccessIdx();
  std::map<dbMPin*, std::vector<dbAccessPoint*>> aps;
  for (auto mpin : mterm->getMPins()) {
    _dbMPin* pin = (_dbMPin*) mpin;
    if (pin->aps_.size() > pin_access_idx) {
      for (const auto& id : pin->aps_[pin_access_idx]) {
        aps[mpin].push_back((dbAccessPoint*) block->ap_tbl_->getPtr(id));
      }
    }
  }
  return aps;
}

std::vector<dbAccessPoint*> dbITerm::getPrefAccessPoints() const
{
  _dbBlock* block = (_dbBlock*) getBlock();
  _dbITerm* iterm = (_dbITerm*) this;
  std::vector<std::pair<dbId<_dbMPin>, dbId<_dbAccessPoint>>> sorted_aps;

  for (auto& [pin_id, ap_id] : iterm->aps_) {
    if (ap_id.isValid()) {
      sorted_aps.emplace_back(pin_id, ap_id);
    }
  }
  // sort to maintain iterator stability, and backwards compatibility with
  // std::map which used to be used to store aps.
  std::ranges::sort(
      sorted_aps,
      [](const std::pair<dbId<_dbMPin>, dbId<_dbAccessPoint>>& a,
         const std::pair<dbId<_dbMPin>, dbId<_dbAccessPoint>>& b) {
        return a.first < b.first;
      });

  std::vector<dbAccessPoint*> aps;
  aps.reserve(sorted_aps.size());
  for (auto& [pin_id, ap_id] : sorted_aps) {
    aps.push_back((dbAccessPoint*) block->ap_tbl_->getPtr(ap_id));
  }

  return aps;
}

void dbITerm::clearPrefAccessPoints()
{
  _dbITerm* iterm = (_dbITerm*) this;
  // Clear aps_ map instead of destroying dbAccessPoint object to prevent
  // destroying APs of other iterms.
  iterm->aps_.clear();
}

std::vector<std::pair<dbTechLayer*, Rect>> dbITerm::getGeometries() const
{
  const dbTransform transform = getInst()->getTransform();

  std::vector<std::pair<dbTechLayer*, Rect>> geometries;
  for (dbMPin* mpin : getMTerm()->getMPins()) {
    for (dbBox* box : mpin->getGeometry()) {
      Rect rect = box->getBox();
      transform.apply(rect);
      geometries.emplace_back(box->getTechLayer(), rect);
    }
  }

  return geometries;
}

void _dbITerm::collectMemInfo(MemInfo& info)
{
  info.cnt++;
  info.size += sizeof(*this);

  info.children["aps"].add(aps_);
}

}  // namespace odb
