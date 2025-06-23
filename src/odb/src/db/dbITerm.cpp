// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "dbITerm.h"

#include <map>
#include <utility>
#include <vector>

#include "dbAccessPoint.h"
#include "dbArrayTable.h"
#include "dbBTerm.h"
#include "dbBlock.h"
#include "dbChip.h"
#include "dbCommon.h"
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
#include "dbTable.hpp"
#include "odb/db.h"
#include "odb/dbBlockCallBackObj.h"
#include "odb/dbShape.h"
#include "utl/Logger.h"
namespace odb {

template class dbTable<_dbITerm>;

bool _dbITerm::operator==(const _dbITerm& rhs) const
{
  if (_flags._mterm_idx != rhs._flags._mterm_idx) {
    return false;
  }

  if (_flags._spef != rhs._flags._spef) {
    return false;
  }

  if (_flags._special != rhs._flags._special) {
    return false;
  }

  if (_flags._connected != rhs._flags._connected) {
    return false;
  }

  if (_ext_id != rhs._ext_id) {
    return false;
  }

  if (_net != rhs._net) {
    return false;
  }

  if (_inst != rhs._inst) {
    return false;
  }

  if (_next_net_iterm != rhs._next_net_iterm) {
    return false;
  }

  if (_prev_net_iterm != rhs._prev_net_iterm) {
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

  _dbInst* lhs_inst = lhs_blk->_inst_tbl->getPtr(_inst);
  _dbInst* rhs_inst = rhs_blk->_inst_tbl->getPtr(rhs._inst);
  int r = strcmp(lhs_inst->_name, rhs_inst->_name);

  if (r < 0) {
    return true;
  }

  if (r > 0) {
    return false;
  }

  _dbMTerm* lhs_mterm = getMTerm();
  _dbMTerm* rhs_mterm = rhs.getMTerm();
  return strcmp(lhs_mterm->_name, rhs_mterm->_name) < 0;
}

_dbMTerm* _dbITerm::getMTerm() const
{
  _dbBlock* block = (_dbBlock*) getOwner();
  _dbInst* inst = block->_inst_tbl->getPtr(_inst);
  _dbInstHdr* inst_hdr = block->_inst_hdr_tbl->getPtr(inst->_inst_hdr);
  _dbDatabase* db = getDatabase();
  _dbLib* lib = db->_lib_tbl->getPtr(inst_hdr->_lib);
  _dbMaster* master = lib->_master_tbl->getPtr(inst_hdr->_master);
  dbId<_dbMTerm> mterm = inst_hdr->_mterms[_flags._mterm_idx];
  return master->_mterm_tbl->getPtr(mterm);
}

_dbInst* _dbITerm::getInst() const
{
  _dbBlock* block = (_dbBlock*) getOwner();
  _dbInst* inst = block->_inst_tbl->getPtr(_inst);
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
  _dbInst* inst = block->_inst_tbl->getPtr(iterm->_inst);
  if (inst == nullptr) {
    iterm->getLogger()->critical(
        utl::ODB, 446, "dbITerm does not have dbInst.");
  }
  return (dbInst*) inst;
}

dbNet* dbITerm::getNet()
{
  _dbITerm* iterm = (_dbITerm*) this;
  _dbBlock* block = (_dbBlock*) iterm->getOwner();

  if (iterm->_net == 0) {
    return nullptr;
  }

  _dbNet* net = block->_net_tbl->getPtr(iterm->_net);
  return (dbNet*) net;
}

dbMTerm* dbITerm::getMTerm() const
{
  _dbITerm* iterm = (_dbITerm*) this;
  _dbBlock* block = (_dbBlock*) iterm->getOwner();
  _dbInst* inst = block->_inst_tbl->getPtr(iterm->_inst);
  _dbInstHdr* inst_hdr = block->_inst_hdr_tbl->getPtr(inst->_inst_hdr);
  _dbDatabase* db = iterm->getDatabase();
  _dbLib* lib = db->_lib_tbl->getPtr(inst_hdr->_lib);
  _dbMaster* master = lib->_master_tbl->getPtr(inst_hdr->_master);
  dbId<_dbMTerm> mterm = inst_hdr->_mterms[iterm->_flags._mterm_idx];
  return (dbMTerm*) master->_mterm_tbl->getPtr(mterm);
}

dbBTerm* dbITerm::getBTerm()
{
  _dbITerm* iterm = (_dbITerm*) this;
  _dbBlock* block = (_dbBlock*) iterm->getOwner();
  _dbInst* inst = block->_inst_tbl->getPtr(iterm->_inst);

  if (inst->_hierarchy == 0) {
    return nullptr;
  }

  _dbHier* hier = block->_hier_tbl->getPtr(inst->_hierarchy);

  _dbChip* chip = (_dbChip*) block->getOwner();
  _dbBlock* child = chip->_block_tbl->getPtr(hier->_child_block);
  dbId<_dbBTerm> bterm = hier->_child_bterms[iterm->_flags._mterm_idx];
  return (dbBTerm*) child->_bterm_tbl->getPtr(bterm);
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
  iterm->_flags._clocked = v;
}
bool dbITerm::isClocked()
{
  bool masterFlag = getMTerm()->getSigType() == dbSigType::CLOCK ? true : false;
  _dbITerm* iterm = (_dbITerm*) this;
  return iterm->_flags._clocked > 0 || masterFlag ? true : false;
}
void dbITerm::setMark(uint v)
{
  _dbITerm* iterm = (_dbITerm*) this;
  iterm->_flags._mark = v;
}
bool dbITerm::isSetMark()
{
  _dbITerm* iterm = (_dbITerm*) this;
  return iterm->_flags._mark > 0 ? true : false;
}

bool dbITerm::isSpecial()
{
  _dbITerm* iterm = (_dbITerm*) this;
  return iterm->_flags._special == 1;
}

void dbITerm::setSpecial()
{
  _dbITerm* iterm = (_dbITerm*) this;
  iterm->_flags._special = 1;
}

void dbITerm::clearSpecial()
{
  _dbITerm* iterm = (_dbITerm*) this;
  iterm->_flags._special = 0;
}

void dbITerm::setSpef(uint v)
{
  _dbITerm* iterm = (_dbITerm*) this;
  iterm->_flags._spef = v;
}

bool dbITerm::isSpef()
{
  _dbITerm* iterm = (_dbITerm*) this;
  return (iterm->_flags._spef > 0) ? true : false;
}

void dbITerm::setExtId(uint v)
{
  _dbITerm* iterm = (_dbITerm*) this;
  iterm->_ext_id = v;
}

uint dbITerm::getExtId()
{
  _dbITerm* iterm = (_dbITerm*) this;
  return iterm->_ext_id;
}

bool dbITerm::isConnected()
{
  _dbITerm* iterm = (_dbITerm*) this;
  return iterm->_flags._connected == 1;
}

void dbITerm::setConnected()
{
  _dbITerm* iterm = (_dbITerm*) this;
  iterm->_flags._connected = 1;
}

void dbITerm::clearConnected()
{
  _dbITerm* iterm = (_dbITerm*) this;
  iterm->_flags._connected = 0;
}

void dbITerm::connect(dbNet* db_net, dbModNet* db_mod_net)
{
  connect(db_net);
  connect(db_mod_net);
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
  if (iterm->_net == net->getOID()) {
    return;
  }

  if (inst->_flags._dont_touch) {
    inst->getLogger()->error(
        utl::ODB,
        369,
        "Attempt to connect iterm of dont_touch instance {}",
        inst->_name);
  }

  if (net->_flags._dont_touch) {
    inst->getLogger()->error(utl::ODB,
                             373,
                             "Attempt to connect iterm to dont_touch net {}",
                             net->_name);
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
  if (iterm->_net != 0) {
    disconnectDbNet();
  }

  for (auto callback : block->_callbacks) {
    callback->inDbITermPreConnect(this, net_);
  }

  if (block->_journal) {
    debugPrint(iterm->getImpl()->getLogger(),
               utl::ODB,
               "DB_ECO",
               1,
               "ECO: connect Iterm {} to net {}",
               getId(),
               net_->getId());
    block->_journal->beginAction(dbJournal::CONNECT_OBJECT);
    block->_journal->pushParam(dbITermObj);
    block->_journal->pushParam(getId());
    block->_journal->pushParam(net_->getId());
    // put in a fake modnet here
    block->_journal->pushParam(0);
    block->_journal->endAction();
  }

  iterm->_net = net->getOID();

  if (net->_iterms != 0) {
    _dbITerm* tail = block->_iterm_tbl->getPtr(net->_iterms);
    iterm->_next_net_iterm = net->_iterms;
    iterm->_prev_net_iterm = 0;
    tail->_prev_net_iterm = iterm->getOID();
  } else {
    iterm->_next_net_iterm = 0;
    iterm->_prev_net_iterm = 0;
  }

  net->_iterms = iterm->getOID();

  for (auto callback : block->_callbacks) {
    callback->inDbITermPostConnect(this);
  }
}

dbModNet* dbITerm::getModNet()
{
  _dbITerm* iterm = (_dbITerm*) this;
  _dbBlock* block = (_dbBlock*) iterm->getOwner();
  if (iterm->_mnet == 0) {
    return nullptr;
  }
  _dbModNet* net = block->_modnet_tbl->getPtr(iterm->_mnet);
  return ((dbModNet*) (net));
}

void dbITerm::connect(dbModNet* mod_net)
{
  _dbITerm* iterm = (_dbITerm*) this;
  _dbModNet* _mod_net = (_dbModNet*) mod_net;
  _dbBlock* block = (_dbBlock*) iterm->getOwner();
  _dbInst* inst = iterm->getInst();

  if (iterm->_mnet == _mod_net->getId()) {
    return;
  }

  // If already connected, disconnect just the modnet (so we don't
  // accidentally blow away prior flat net connections)

  if (iterm->_mnet != 0) {
    disconnectModNet();
  }

  iterm->_mnet = _mod_net->getId();

  if (inst->_flags._dont_touch) {
    inst->getLogger()->error(
        utl::ODB,
        397,
        "Attempt to connect iterm of dont_touch instance {}",
        inst->_name);
  }

  if (block->_journal) {
    debugPrint(iterm->getImpl()->getLogger(),
               utl::ODB,
               "DB_ECO",
               1,
               "ECO: connect Iterm {} to modnet {}",
               getId(),
               _mod_net->getId());
    block->_journal->beginAction(dbJournal::CONNECT_OBJECT);
    block->_journal->pushParam(dbITermObj);
    block->_journal->pushParam(getId());
    block->_journal->pushParam(0);  // empty slot for dbNet, just dbModNet
    block->_journal->pushParam(_mod_net->getId());
    block->_journal->endAction();
  }

  if (_mod_net->_iterms != 0) {
    _dbITerm* head = block->_iterm_tbl->getPtr(_mod_net->_iterms);
    iterm->_next_modnet_iterm = _mod_net->_iterms;
    // prev is this one
    head->_prev_modnet_iterm = iterm->getOID();
  } else {
    iterm->_next_modnet_iterm = 0;
  }
  iterm->_prev_modnet_iterm = 0;
  _mod_net->_iterms = iterm->getOID();
}

// disconnect both modnet and flat net from an iterm
void dbITerm::disconnect()
{
  _dbITerm* iterm = (_dbITerm*) this;

  if (iterm->_net == 0 && iterm->_mnet == 0) {
    return;
  }

  _dbInst* inst = iterm->getInst();
  if (inst->_flags._dont_touch) {
    inst->getLogger()->error(
        utl::ODB,
        370,
        "Attempt to disconnect term {} of dont_touch instance {}",
        getMTerm()->getName(),
        inst->_name);
  }

  _dbBlock* block = (_dbBlock*) iterm->getOwner();
  _dbNet* net
      = iterm->_net == 0 ? nullptr : block->_net_tbl->getPtr(iterm->_net);
  _dbModNet* mod_net
      = iterm->_mnet == 0 ? nullptr : block->_modnet_tbl->getPtr(iterm->_mnet);

  if (net && net->_flags._dont_touch) {
    inst->getLogger()->error(
        utl::ODB,
        372,
        "Attempt to disconnect iterm {} of dont_touch net {}",
        getName(),
        net->_name);
  }

  if (block->_journal) {
    debugPrint(iterm->getImpl()->getLogger(),
               utl::ODB,
               "DB_ECO",
               1,
               "ECO: disconnect Iterm {}",
               getId());

    block->_journal->beginAction(dbJournal::DISCONNECT_OBJECT);
    block->_journal->pushParam(dbITermObj);
    block->_journal->pushParam(getId());
    block->_journal->pushParam(net ? net->getOID() : 0U);
    block->_journal->pushParam(mod_net ? mod_net->getOID() : 0U);
    block->_journal->endAction();
  }

  for (auto callback : block->_callbacks) {
    callback->inDbITermPreDisconnect(this);
  }

  uint id = iterm->getOID();

  if (net) {
    if (net->_iterms == id) {
      net->_iterms = iterm->_next_net_iterm;
      if (net->_iterms != 0) {
        _dbITerm* t = block->_iterm_tbl->getPtr(net->_iterms);
        t->_prev_net_iterm = 0;
      }
    } else {
      if (iterm->_next_net_iterm != 0) {
        _dbITerm* next = block->_iterm_tbl->getPtr(iterm->_next_net_iterm);
        next->_prev_net_iterm = iterm->_prev_net_iterm;
      }
      if (iterm->_prev_net_iterm != 0) {
        _dbITerm* prev = block->_iterm_tbl->getPtr(iterm->_prev_net_iterm);
        prev->_next_net_iterm = iterm->_next_net_iterm;
      }
    }
    iterm->_net = 0;
    for (auto callback : block->_callbacks) {
      callback->inDbITermPostDisconnect(this, (dbNet*) net);
    }
  }

  if (mod_net) {
    if (mod_net->_iterms == id) {
      mod_net->_iterms = iterm->_next_modnet_iterm;
      if (mod_net->_iterms != 0) {
        _dbITerm* t = block->_iterm_tbl->getPtr(mod_net->_iterms);
        t->_prev_modnet_iterm = 0;
      }
    } else {
      if (iterm->_next_modnet_iterm != 0) {
        _dbITerm* next = block->_iterm_tbl->getPtr(iterm->_next_modnet_iterm);
        next->_prev_modnet_iterm = iterm->_prev_modnet_iterm;
      }
      if (iterm->_prev_modnet_iterm != 0) {
        _dbITerm* prev = block->_iterm_tbl->getPtr(iterm->_prev_modnet_iterm);
        prev->_next_modnet_iterm = iterm->_next_modnet_iterm;
      }
    }
    iterm->_mnet = 0;
  }
}

// disconnect the dbNetonly and allow journalling

void dbITerm::disconnectDbNet()
{
  _dbITerm* iterm = (_dbITerm*) this;

  if (iterm->_net == 0) {
    return;
  }

  _dbInst* inst = iterm->getInst();
  if (inst->_flags._dont_touch) {
    inst->getLogger()->error(
        utl::ODB,
        1104,
        "Attempt to disconnect term {} of dont_touch instance {}",
        getMTerm()->getName(),
        inst->_name);
  }
  _dbBlock* block = (_dbBlock*) iterm->getOwner();
  _dbNet* net = block->_net_tbl->getPtr(iterm->_net);

  if (net->_flags._dont_touch) {
    inst->getLogger()->error(
        utl::ODB,
        1105,
        "Attempt to disconnect iterm {} of dont_touch net {}",
        getName(),
        net->_name);
  }

  for (auto callback : block->_callbacks) {
    callback->inDbITermPreDisconnect(this);
  }
  if (block->_journal) {
    debugPrint(iterm->getImpl()->getLogger(),
               utl::ODB,
               "DB_ECO",
               1,
               "ECO: disconnect Iterm {} to net {}",
               getId(),
               net->getId());
    block->_journal->beginAction(dbJournal::DISCONNECT_OBJECT);
    block->_journal->pushParam(dbITermObj);
    block->_journal->pushParam(getId());
    block->_journal->pushParam(net->getOID());
    // Note we don't remove the mod net part here, just stub
    // out the modnet id.
    block->_journal->pushParam(0);
    block->_journal->endAction();
  }

  uint id = iterm->getOID();

  if (net->_iterms == id) {
    net->_iterms = iterm->_next_net_iterm;
    if (net->_iterms != 0) {
      _dbITerm* t = block->_iterm_tbl->getPtr(net->_iterms);
      t->_prev_net_iterm = 0;
    }
  } else {
    if (iterm->_next_net_iterm != 0) {
      _dbITerm* next = block->_iterm_tbl->getPtr(iterm->_next_net_iterm);
      next->_prev_net_iterm = iterm->_prev_net_iterm;
    }
    if (iterm->_prev_net_iterm != 0) {
      _dbITerm* prev = block->_iterm_tbl->getPtr(iterm->_prev_net_iterm);
      prev->_next_net_iterm = iterm->_next_net_iterm;
    }
  }
  iterm->_net = 0;
  for (auto callback : block->_callbacks) {
    callback->inDbITermPostDisconnect(this, (dbNet*) net);
  }
}

//
// Disconnect the mod net and allow journaling
//
void dbITerm::disconnectModNet()
{
  _dbITerm* iterm = (_dbITerm*) this;
  _dbBlock* block = (_dbBlock*) iterm->getOwner();

  if (iterm->_mnet != 0) {
    _dbModNet* mod_net = block->_modnet_tbl->getPtr(iterm->_mnet);

    if (block->_journal) {
      debugPrint(iterm->getImpl()->getLogger(),
                 utl::ODB,
                 "DB_ECO",
                 1,
                 "ECO: disconnect Iterm {} to modnet {}",
                 getId(),
                 iterm->_mnet);
      block->_journal->beginAction(dbJournal::DISCONNECT_OBJECT);
      block->_journal->pushParam(dbITermObj);
      block->_journal->pushParam(getId());
      // empty dbNet part, just the mod net being undone
      block->_journal->pushParam(0);                  // no dbNet id
      block->_journal->pushParam(mod_net->getOID());  // the modnet
      block->_journal->endAction();
    }

    if (mod_net->_iterms == getId()) {
      mod_net->_iterms = iterm->_next_modnet_iterm;
      if (mod_net->_iterms != 0) {
        _dbITerm* t = block->_iterm_tbl->getPtr(mod_net->_iterms);
        t->_prev_modnet_iterm = 0;
      }
    } else {
      if (iterm->_next_modnet_iterm != 0) {
        _dbITerm* next = block->_iterm_tbl->getPtr(iterm->_next_modnet_iterm);
        next->_prev_modnet_iterm = iterm->_prev_modnet_iterm;
      }
      if (iterm->_prev_modnet_iterm != 0) {
        _dbITerm* prev = block->_iterm_tbl->getPtr(iterm->_prev_modnet_iterm);
        prev->_next_modnet_iterm = iterm->_next_modnet_iterm;
      }
    }
    iterm->_mnet = 0;
  }
}

dbSigType dbITerm::getSigType()
{
  _dbMTerm* mterm = (_dbMTerm*) getMTerm();
  return dbSigType(mterm->_flags._sig_type);
}
dbIoType dbITerm::getIoType()
{
  _dbMTerm* mterm = (_dbMTerm*) getMTerm();
  return dbIoType(mterm->_flags._io_type);
}
bool dbITerm::isOutputSignal(bool io)
{
  _dbMTerm* mterm = (_dbMTerm*) getMTerm();
  dbSigType sType = dbSigType(mterm->_flags._sig_type);
  dbIoType ioType = dbIoType(mterm->_flags._io_type);

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
  dbSigType sType = dbSigType(mterm->_flags._sig_type);
  dbIoType ioType = dbIoType(mterm->_flags._io_type);

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

dbITerm* dbITerm::getITerm(dbBlock* block_, uint dbid)
{
  _dbBlock* block = (_dbBlock*) block_;
  return (dbITerm*) block->_iterm_tbl->getPtr(dbid);
}

Rect dbITerm::getBBox()
{
  dbMTerm* term = getMTerm();
  Rect bbox = term->getBBox();
  const odb::dbTransform inst_xfm = getInst()->getTransform();
  inst_xfm.apply(bbox);
  return bbox;
}

bool dbITerm::getAvgXY(int* x, int* y)
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
  return iterm->_sta_vertex_id;
}

void dbITerm::staSetVertexId(uint32_t id)
{
  _dbITerm* iterm = (_dbITerm*) this;
  iterm->_sta_vertex_id = id;
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
}

std::map<dbMPin*, std::vector<dbAccessPoint*>> dbITerm::getAccessPoints() const
{
  _dbBlock* block = (_dbBlock*) getBlock();
  auto mterm = getMTerm();
  uint pin_access_idx = getInst()->getPinAccessIdx();
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
  std::sort(sorted_aps.begin(),
            sorted_aps.end(),
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

  info.children_["aps"].add(aps_);
}

}  // namespace odb
