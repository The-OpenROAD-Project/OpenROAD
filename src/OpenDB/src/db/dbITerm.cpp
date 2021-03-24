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

#include "dbITerm.h"

#include "db.h"
#include "dbArrayTable.h"
#include "dbBTerm.h"
#include "dbBlock.h"
#include "dbBlockCallBackObj.h"
#include "dbChip.h"
#include "dbCommon.h"
#include "dbDatabase.h"
#include "dbDiff.hpp"
#include "dbHier.h"
#include "dbInst.h"
#include "dbInstHdr.h"
#include "dbJournal.h"
#include "dbLib.h"
#include "dbMTerm.h"
#include "dbMaster.h"
#include "dbNet.h"
#include "dbShape.h"
#include "dbTable.h"
#include "dbTable.hpp"
#include "utl/Logger.h"

namespace odb {

template class dbTable<_dbITerm>;

bool _dbITerm::operator==(const _dbITerm& rhs) const
{
  if (_flags._mterm_idx != rhs._flags._mterm_idx)
    return false;

  if (_flags._spef != rhs._flags._spef)
    return false;

  if (_flags._special != rhs._flags._special)
    return false;

  if (_flags._connected != rhs._flags._connected)
    return false;

  if (_ext_id != rhs._ext_id)
    return false;

  if (_net != rhs._net)
    return false;

  if (_inst != rhs._inst)
    return false;

  if (_next_net_iterm != rhs._next_net_iterm)
    return false;

  if (_prev_net_iterm != rhs._prev_net_iterm)
    return false;

  return true;
}

bool _dbITerm::operator<(const _dbITerm& rhs) const
{
  _dbBlock* lhs_blk = (_dbBlock*) getOwner();
  _dbBlock* rhs_blk = (_dbBlock*) rhs.getOwner();

  _dbInst* lhs_inst = lhs_blk->_inst_tbl->getPtr(_inst);
  _dbInst* rhs_inst = rhs_blk->_inst_tbl->getPtr(rhs._inst);
  int r = strcmp(lhs_inst->_name, rhs_inst->_name);

  if (r < 0)
    return true;

  if (r > 0)
    return false;

  _dbMTerm* lhs_mterm = getMTerm();
  _dbMTerm* rhs_mterm = rhs.getMTerm();
  return strcmp(lhs_mterm->_name, rhs_mterm->_name) < 0;
}

void _dbITerm::differences(dbDiff& diff,
                           const char* field,
                           const _dbITerm& rhs) const
{
  if (!diff.deepDiff()) {
    DIFF_BEGIN
    DIFF_FIELD(_net);
    DIFF_FIELD(_inst);
    DIFF_FIELD(_flags._mterm_idx);
    DIFF_FIELD(_flags._spef);
    DIFF_FIELD(_flags._special);
    DIFF_FIELD(_flags._connected);
    DIFF_FIELD(_ext_id);
    DIFF_FIELD(_next_net_iterm);
    DIFF_FIELD(_prev_net_iterm);
    DIFF_END
  } else {
    _dbBlock* lhs_blk = (_dbBlock*) getOwner();
    _dbBlock* rhs_blk = (_dbBlock*) rhs.getOwner();
    _dbInst* lhs_inst = lhs_blk->_inst_tbl->getPtr(_inst);
    _dbInst* rhs_inst = rhs_blk->_inst_tbl->getPtr(rhs._inst);
    _dbMTerm* lhs_mterm = getMTerm();
    _dbMTerm* rhs_mterm = rhs.getMTerm();
    ZASSERT(strcmp(lhs_inst->_name, rhs_inst->_name) == 0);
    ZASSERT(strcmp(lhs_mterm->_name, rhs_mterm->_name) == 0);

    diff.begin_object("<> %s (_dbITerm)\n", lhs_mterm->_name);

    if ((_net != 0) && (rhs._net != 0)) {
      _dbNet* lhs_net = lhs_blk->_net_tbl->getPtr(_net);
      _dbNet* rhs_net = rhs_blk->_net_tbl->getPtr(rhs._net);
      diff.diff("_net", lhs_net->_name, rhs_net->_name);
    } else if (_net != 0) {
      _dbNet* lhs_net = lhs_blk->_net_tbl->getPtr(_net);
      diff.out(dbDiff::LEFT, "_net", lhs_net->_name);
    } else if (rhs._net != 0) {
      _dbNet* rhs_net = rhs_blk->_net_tbl->getPtr(rhs._net);
      diff.out(dbDiff::RIGHT, "_net", rhs_net->_name);
    }

    DIFF_FIELD(_flags._spef);
    DIFF_FIELD(_flags._special);
    DIFF_FIELD(_flags._connected);
    DIFF_FIELD(_ext_id);
    diff.end_object();
  }
}

void _dbITerm::out(dbDiff& diff, char side, const char* field) const
{
  if (!diff.deepDiff()) {
    DIFF_OUT_BEGIN
    DIFF_OUT_FIELD(_net);
    DIFF_OUT_FIELD(_inst);
    DIFF_OUT_FIELD(_flags._mterm_idx);
    DIFF_OUT_FIELD(_flags._spef);
    DIFF_OUT_FIELD(_flags._special);
    DIFF_OUT_FIELD(_flags._connected);
    DIFF_OUT_FIELD(_ext_id);
    DIFF_OUT_FIELD(_next_net_iterm);
    DIFF_OUT_FIELD(_prev_net_iterm);
    DIFF_END
  } else {
    _dbMTerm* mterm = getMTerm();
    diff.begin_object("%c %s (_dbITerm)\n", side, mterm->_name);
    _dbBlock* blk = (_dbBlock*) getOwner();

    if (_net != 0) {
      _dbNet* net = blk->_net_tbl->getPtr(_net);
      diff.out(side, "_net", net->_name);
    }

    DIFF_OUT_FIELD(_flags._spef);
    DIFF_OUT_FIELD(_flags._special);
    DIFF_OUT_FIELD(_flags._connected);
    DIFF_OUT_FIELD(_ext_id);
    diff.end_object();
  }
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

dbInst* dbITerm::getInst()
{
  _dbITerm* iterm = (_dbITerm*) this;
  _dbBlock* block = (_dbBlock*) iterm->getOwner();
  _dbInst* inst = block->_inst_tbl->getPtr(iterm->_inst);
  return (dbInst*) inst;
}

dbNet* dbITerm::getNet()
{
  _dbITerm* iterm = (_dbITerm*) this;
  _dbBlock* block = (_dbBlock*) iterm->getOwner();

  if (iterm->_net == 0)
    return NULL;

  _dbNet* net = block->_net_tbl->getPtr(iterm->_net);
  return (dbNet*) net;
}

dbMTerm* dbITerm::getMTerm()
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

  if (inst->_hierarchy == 0)
    return NULL;

  _dbHier* hier = block->_hier_tbl->getPtr(inst->_hierarchy);

  _dbChip* chip = (_dbChip*) block->getOwner();
  _dbBlock* child = chip->_block_tbl->getPtr(hier->_child_block);
  dbId<_dbBTerm> bterm = hier->_child_bterms[iterm->_flags._mterm_idx];
  return (dbBTerm*) child->_bterm_tbl->getPtr(bterm);
}

dbBlock* dbITerm::getBlock()
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
  //_dbBlock * block = (_dbBlock *) getOwner();
  // dimitri_fix: need to FIX on FULL_ECO uint prev_flags = flagsToUInt(iterm);
#ifdef FULL_ECO
  uint prev_flags = flagsToUInt(iterm);
#endif

  iterm->_flags._special = 1;

#ifdef FULL_ECO
  if (block->_journal) {
    debugPrint(getImpl()->getLogger(),
               utl::ODB,
               "DB_ECO",
               1,
               "ECO: Iterm {}, setSpecial",
               getId());
    block->_journal->updateField(
        this, _dbNet::FLAGS, prev_flags, flagsToUInt(iterm));
  }
#endif
}

void dbITerm::clearSpecial()
{
  _dbITerm* iterm = (_dbITerm*) this;
  //_dbBlock * block = (_dbBlock *) getOwner();
  // dimitri_fix: need to FIX on FULL_ECO //uint prev_flags =
  // flagsToUInt(iterm);
#ifdef FULL_ECO
  uint prev_flags = flagsToUInt(iterm);
#endif

  iterm->_flags._special = 0;

#ifdef FULL_ECO
  if (block->_journal) {
    debugPrint(getImpl()->getLogger(),
               utl::ODB,
               "DB_ECO",
               1,
               "ECO: Iterm {}, clearSpecial\n",
               getId());
    block->_journal->updateField(
        this, _dbNet::FLAGS, prev_flags, flagsToUInt(iterm));
  }
#endif
}

void dbITerm::setSpef(uint v)
{
  _dbITerm* iterm = (_dbITerm*) this;
  //_dbBlock * block = (_dbBlock *) getOwner();
  // dimitri_fix: need to FIX on FULL_ECO //uint prev_flags =
  // flagsToUInt(iterm);
#ifdef FULL_ECO
  uint prev_flags = flagsToUInt(iterm);
#endif

  iterm->_flags._spef = v;

#ifdef FULL_ECO
  if (block->_journal) {
    debugPrint(getImpl()->getLogger(),
               utl::ODB,
               "DB_ECO",
               1,
               "ECO: Iterm {}, setSpef",
               getId());
    block->_journal->updateField(
        this, _dbNet::FLAGS, prev_flags, flagsToUInt(iterm));
  }
#endif
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
  // dimitri_fix: need to FIX on FULL_ECO uint prev_flags = flagsToUInt(iterm);
#ifdef FULL_ECO
  _dbBlock* block = (_dbBlock*) getOwner();
  uint prev_flags = flagsToUInt(iterm);
#endif

  iterm->_flags._connected = 1;

#ifdef FULL_ECO
  if (block->_journal) {
    debugPrint(getImpl()->getLogger(),
               utl::ODB,
               "DB_ECO",
               1,
               "ECO: Iterm {}, setConnected",
               getId());
    block->_journal->updateField(
        this, _dbNet::FLAGS, prev_flags, flagsToUInt(iterm));
  }
#endif
}

void dbITerm::clearConnected()
{
  _dbITerm* iterm = (_dbITerm*) this;
  // uint prev_flags = flagsToUInt(iterm);
#ifdef FULL_ECO
  _dbBlock* block = (_dbBlock*) getOwner();
  uint prev_flags = flagsToUInt(iterm);
#endif

  iterm->_flags._connected = 0;

#ifdef FULL_ECO
  if (block->_journal) {
    debugPrint(getImpl()->getLogger(),
               utl::ODB,
               "DB_ECO",
               1,
               "ECO: Iterm {}, clearConnected",
               getId());
    block->_journal->updateField(
        this, _dbNet::FLAGS, prev_flags, flagsToUInt(iterm));
  }
#endif
}

dbITerm* dbITerm::connect(dbInst* inst_, dbNet* net_, dbMTerm* mterm_)
{
  _dbInst* inst = (_dbInst*) inst_;
  //_dbNet * net = (_dbNet *) net_;
  _dbMTerm* mterm = (_dbMTerm*) mterm_;
  _dbBlock* block = (_dbBlock*) inst->getOwner();
  _dbITerm* iterm = block->_iterm_tbl->getPtr(inst->_iterms[mterm->_order_id]);
  connect((dbITerm*) iterm, net_);
  return (dbITerm*) iterm;
}

void dbITerm::connect(dbITerm* iterm_, dbNet* net_)
{
  _dbITerm* iterm = (_dbITerm*) iterm_;
  _dbNet* net = (_dbNet*) net_;
  _dbBlock* block = (_dbBlock*) iterm->getOwner();

  // Do Nothing if already connected
  if (iterm->_net == net->getOID())
    return;
  for (auto callback : block->_callbacks)
    callback->inDbITermPreConnect(iterm_, net_);

  if (iterm->_net != 0)
    disconnect(iterm_);

  if (block->_journal) {
    debugPrint(iterm->getImpl()->getLogger(),
               utl::ODB,
               "DB_ECO",
               1,
               "ECO: connect Iterm {} to net {}",
               iterm_->getId(),
               net_->getId());
    block->_journal->beginAction(dbJournal::CONNECT_OBJECT);
    block->_journal->pushParam(dbITermObj);
    block->_journal->pushParam(iterm_->getId());
    block->_journal->pushParam(net_->getId());
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

  for (auto callback : block->_callbacks)
    callback->inDbITermPostConnect(iterm_);
}

void dbITerm::disconnect(dbITerm* iterm_)
{
  _dbITerm* iterm = (_dbITerm*) iterm_;

  if (iterm->_net == 0)
    return;

  _dbBlock* block = (_dbBlock*) iterm->getOwner();
  _dbNet* net = block->_net_tbl->getPtr(iterm->_net);
  for (auto callback : block->_callbacks)
    callback->inDbITermPreDisconnect(iterm_);
  if (block->_journal) {
    debugPrint(iterm->getImpl()->getLogger(),
               utl::ODB,
               "DB_ECO",
               1,
               "ECO: disconnect Iterm {}",
               iterm_->getId());
    block->_journal->beginAction(dbJournal::DISCONNECT_OBJECT);
    block->_journal->pushParam(dbITermObj);
    block->_journal->pushParam(iterm_->getId());
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
  for (auto callback : block->_callbacks)
    callback->inDbITermPostDisconnect(iterm_, (dbNet*) net);
}

dbSet<dbITerm>::iterator dbITerm::disconnect(dbSet<dbITerm>::iterator& itr)
{
  dbITerm* it = *itr;
  dbSet<dbITerm>::iterator next = ++itr;
  disconnect(it);
  return next;
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

  if ((sType == dbSigType::GROUND) || (sType == dbSigType::POWER))
    return false;

  if (ioType == dbIoType::OUTPUT)
    return true;

  if (io && (ioType == dbIoType::INOUT))
    return true;

  return false;
}
bool dbITerm::isInputSignal(bool io)
{
  _dbMTerm* mterm = (_dbMTerm*) getMTerm();
  dbSigType sType = dbSigType(mterm->_flags._sig_type);
  dbIoType ioType = dbIoType(mterm->_flags._io_type);

  if ((sType == dbSigType::GROUND) || (sType == dbSigType::POWER))
    return false;

  if (ioType == dbIoType::INPUT)
    return true;

  if (io && (ioType == dbIoType::INOUT))
    return true;

  return false;
}

dbITerm* dbITerm::getITerm(dbBlock* block_, uint dbid)
{
  _dbBlock* block = (_dbBlock*) block_;
  return (dbITerm*) block->_iterm_tbl->getPtr(dbid);
}

bool dbITerm::getAvgXY(int* x, int* y)
{
  dbMTerm* mterm = getMTerm();
  int nn = 0;
  double xx = 0.0;
  double yy = 0.0;
  int px;
  int py;
  dbInst* inst = getInst();
  inst->getOrigin(px, py);
  Point origin = Point(px, py);
  dbOrientType orient = inst->getOrient();
  dbTransform transform(orient, origin);

  dbSet<dbMPin> mpins = mterm->getMPins();
  dbSet<dbMPin>::iterator mpin_itr;
  for (mpin_itr = mpins.begin(); mpin_itr != mpins.end(); mpin_itr++) {
    dbMPin* mpin = *mpin_itr;
    dbSet<dbBox> boxes = mpin->getGeometry();
    dbSet<dbBox>::iterator box_itr;
    for (box_itr = boxes.begin(); box_itr != boxes.end(); box_itr++) {
      dbBox* box = *box_itr;
      Rect rect;
      box->getBox(rect);
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
void dbITerm::print(FILE* fp, const char* trail)
{
  if (fp == NULL) {
    getImpl()->getLogger()->info(utl::ODB,
                                 35,
                                 "{} {} {} {}{}",
                                 getId(),
                                 getMTerm()->getConstName(),
                                 getMTerm()->getMaster()->getConstName(),
                                 getInst()->getConstName(),
                                 trail);
  } else {
    fprintf(fp,
            "%d %s %s %s%s",
            getId(),
            getMTerm()->getConstName(),
            getMTerm()->getMaster()->getConstName(),
            getInst()->getConstName(),
            trail);
  }
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

}  // namespace odb
