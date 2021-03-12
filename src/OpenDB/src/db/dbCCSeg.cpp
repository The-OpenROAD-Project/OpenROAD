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

#include "dbCCSeg.h"

#include "db.h"
#include "dbBlock.h"
#include "dbCapNode.h"
#include "dbDatabase.h"
#include "dbJournal.h"
#include "dbNet.h"
#include "dbTable.h"
#include "dbTable.hpp"
#include "utility/Logger.h"

namespace odb {

template class dbTable<_dbCCSeg>;

bool _dbCCSeg::operator==(const _dbCCSeg& rhs) const
{
  if (_flags._spef_mark_1 != rhs._flags._spef_mark_1)
    return false;

  if (_flags._mark != rhs._flags._mark)
    return false;

  if (_flags._inFileCnt != rhs._flags._inFileCnt)
    return false;

  if (_cap_node[0] != rhs._cap_node[0])
    return false;

  if (_cap_node[1] != rhs._cap_node[1])
    return false;

  if (_next[0] != rhs._next[0])
    return false;

  if (_next[1] != rhs._next[1])
    return false;

  return true;
}

void _dbCCSeg::differences(dbDiff& diff,
                           const char* field,
                           const _dbCCSeg& rhs) const
{
  DIFF_BEGIN
  DIFF_FIELD(_flags._spef_mark_1);
  DIFF_FIELD(_flags._mark);
  DIFF_FIELD(_flags._inFileCnt);
  DIFF_FIELD(_cap_node[0]);
  DIFF_FIELD(_cap_node[1]);
  DIFF_FIELD(_next[0]);
  DIFF_FIELD(_next[1]);
  DIFF_END
}

void _dbCCSeg::out(dbDiff& diff, char side, const char* field) const
{
  DIFF_OUT_BEGIN
  DIFF_OUT_FIELD(_flags._spef_mark_1);
  DIFF_OUT_FIELD(_flags._mark);
  DIFF_OUT_FIELD(_flags._inFileCnt);
  DIFF_OUT_FIELD(_cap_node[0]);
  DIFF_OUT_FIELD(_cap_node[1]);
  DIFF_OUT_FIELD(_next[0]);
  DIFF_OUT_FIELD(_next[1]);
  DIFF_END
}

////////////////////////////////////////////////////////////////////
//
// dbCCSeg - Methods
//
////////////////////////////////////////////////////////////////////

void dbCCSeg::adjustCapacitance(float factor, int corner)
{
  _dbCCSeg* seg = (_dbCCSeg*) this;
  _dbBlock* block = (_dbBlock*) seg->getOwner();

  float& value
      = (*block->_cc_val_tbl)[(seg->getOID() - 1) * block->_corners_per_block
                              + 1 + corner];
  float prev_value = value;
  value *= factor;

  if (block->_journal) {
    debugPrint(getImpl()->getLogger(),
               utl::ODB,
               "DB_ECO",
               1,
               "ECO: dbCCSeg {}, adjustCapacitance {}, corner {}",
               seg->getId(),
               factor,
               corner);
    block->_journal->beginAction(dbJournal::UPDATE_FIELD);
    block->_journal->pushParam(dbCCSegObj);
    block->_journal->pushParam(seg->getId());
    block->_journal->pushParam(_dbCCSeg::CAPACITANCE);
    block->_journal->pushParam(prev_value);
    block->_journal->pushParam(value);
    block->_journal->pushParam(0);
    block->_journal->endAction();
  }
}

void dbCCSeg::adjustCapacitance(float factor)
{
  _dbBlock* block = (_dbBlock*) getImpl()->getOwner();
  uint corner;
  for (corner = 0; corner < block->_corners_per_block; corner++)
    adjustCapacitance(factor, corner);
}

double dbCCSeg::getCapacitance(int corner)
{
  _dbCCSeg* seg = (_dbCCSeg*) this;
  _dbBlock* block = (_dbBlock*) seg->getOwner();
  uint cornerCnt = block->_corners_per_block;
  ZASSERT((corner >= 0) && ((uint) corner < cornerCnt));
  return (*block->_cc_val_tbl)[(seg->getOID() - 1) * cornerCnt + 1 + corner];
}

void dbCCSeg::accAllCcCap(double* ttcap, double MillerMult)
{
  _dbCCSeg* seg = (_dbCCSeg*) this;
  _dbBlock* block = (_dbBlock*) seg->getOwner();
  uint cornerCnt = block->_corners_per_block;
  for (uint ii = 0; ii < cornerCnt; ii++) {
    ttcap[ii]
        += ((*block->_cc_val_tbl)[(seg->getOID() - 1) * cornerCnt + 1 + ii])
           * MillerMult;
  }
}

void dbCCSeg::getAllCcCap(double* ttcap)
{
  _dbCCSeg* seg = (_dbCCSeg*) this;
  _dbBlock* block = (_dbBlock*) seg->getOwner();
  uint cornerCnt = block->_corners_per_block;
  for (uint ii = 0; ii < cornerCnt; ii++) {
    ttcap[ii] = (*block->_cc_val_tbl)[(seg->getOID() - 1) * cornerCnt + 1 + ii];
  }
}

void dbCCSeg::setAllCcCap(double* ttcap)
{
  _dbCCSeg* seg = (_dbCCSeg*) this;
  _dbBlock* block = (_dbBlock*) seg->getOwner();
  uint cornerCnt = block->_corners_per_block;
  for (uint ii = 0; ii < cornerCnt; ii++) {
    (*block->_cc_val_tbl)[(seg->getOID() - 1) * cornerCnt + 1 + ii] = ttcap[ii];
  }
  if (block->_journal) {
    char ccCaps[400];
    int pos = 0;
    ccCaps[0] = '\0';
    for (uint ii = 0; ii < cornerCnt; ii++)
      pos += sprintf(&ccCaps[pos], "%f ", ttcap[ii]);
    debugPrint(getImpl()->getLogger(),
               utl::ODB,
               "DB_ECO",
               1,
               "ECO: dbCCSeg::setAllCcCap, ccseg: {}, caps: {}",
               seg->getId(),
               ttcap[0]);
    block->_journal->beginAction(dbJournal::UPDATE_FIELD);
    block->_journal->pushParam(dbCCSegObj);
    block->_journal->pushParam(getId());
    block->_journal->pushParam(_dbCCSeg::SETALLCCCAP);
    for (uint ii = 0; ii < cornerCnt; ii++)
      block->_journal->pushParam(ttcap[ii]);
    block->_journal->endAction();
  }
}

void dbCCSeg::setCapacitance(double cap, int corner)
{
  _dbCCSeg* seg = (_dbCCSeg*) this;
  _dbBlock* block = (_dbBlock*) seg->getOwner();
  uint cornerCnt = block->_corners_per_block;
  ZASSERT((corner >= 0) && ((uint) corner < cornerCnt));

  float& value
      = (*block->_cc_val_tbl)[(seg->getOID() - 1) * cornerCnt + 1 + corner];
  float prev_value = value;
  value = (float) cap;

  if (block->_journal) {
    debugPrint(getImpl()->getLogger(),
               utl::ODB,
               "DB_ECO",
               1,
               "ECO: dbCCSeg {}, setCapacitance {}, corner {}",
               seg->getId(),
               value,
               corner);
    block->_journal->beginAction(dbJournal::UPDATE_FIELD);
    block->_journal->pushParam(dbCCSegObj);
    block->_journal->pushParam(seg->getId());
    block->_journal->pushParam(_dbCCSeg::CAPACITANCE);
    block->_journal->pushParam(prev_value);
    block->_journal->pushParam(value);
    block->_journal->pushParam(corner);
    block->_journal->endAction();
  }
}

void dbCCSeg::addCapacitance(double cap, int corner)
{
  _dbCCSeg* seg = (_dbCCSeg*) this;
  _dbBlock* block = (_dbBlock*) seg->getOwner();
  uint cornerCnt = block->_corners_per_block;
  ZASSERT((corner >= 0) && ((uint) corner < cornerCnt));

  float& value
      = (*block->_cc_val_tbl)[(seg->getOID() - 1) * cornerCnt + 1 + corner];
  float prev_value = value;
  value += (float) cap;

  if (block->_journal) {
    debugPrint(getImpl()->getLogger(),
               utl::ODB,
               "DB_ECO",
               1,
               "ECO: dbCCSeg {}, addCapacitance {}, corner {}",
               seg->getId(),
               value,
               corner);
    block->_journal->beginAction(dbJournal::UPDATE_FIELD);
    block->_journal->pushParam(dbCCSegObj);
    block->_journal->pushParam(seg->getId());
    block->_journal->pushParam(_dbCCSeg::CAPACITANCE);
    block->_journal->pushParam(prev_value);
    block->_journal->pushParam(value);
    block->_journal->pushParam(corner);
    block->_journal->endAction();
  }
}

void dbCCSeg::addCcCapacitance(dbCCSeg* other)
{
  _dbCCSeg* seg = (_dbCCSeg*) this;
  _dbBlock* block = (_dbBlock*) seg->getOwner();
  _dbCCSeg* oseg = (_dbCCSeg*) other;
  uint cornerCnt = block->_corners_per_block;

  for (uint ii = 0; ii < cornerCnt; ii++) {
    float& value
        = (*block->_cc_val_tbl)[(seg->getOID() - 1) * cornerCnt + 1 + ii];
    float& ovalue
        = (*block->_cc_val_tbl)[(oseg->getOID() - 1) * cornerCnt + 1 + ii];
    value += ovalue;
  }

  if (block->_journal) {
    debugPrint(getImpl()->getLogger(),
               utl::ODB,
               "DB_ECO",
               1,
               "ECO: dbCCSeg {}, other dbCCSeg {}, addCcCapacitance",
               seg->getOID(),
               oseg->getOID());
    block->_journal->beginAction(dbJournal::UPDATE_FIELD);
    block->_journal->pushParam(dbCCSegObj);
    block->_journal->pushParam(seg->getId());
    block->_journal->pushParam(_dbCCSeg::ADDCCCAPACITANCE);
    block->_journal->pushParam(oseg->getId());
    block->_journal->endAction();
  }
}

dbCapNode* dbCCSeg::getSourceCapNode()
{
  _dbCCSeg* seg = (_dbCCSeg*) this;
  _dbBlock* block = (_dbBlock*) seg->getOwner();
  _dbCapNode* n = block->_cap_node_tbl->getPtr(seg->_cap_node[0]);
  return (dbCapNode*) n;
}

dbCapNode* dbCCSeg::getTargetCapNode()
{
  _dbCCSeg* seg = (_dbCCSeg*) this;
  _dbBlock* block = (_dbBlock*) seg->getOwner();
  _dbCapNode* n = block->_cap_node_tbl->getPtr(seg->_cap_node[1]);
  return (dbCapNode*) n;
}

uint dbCCSeg::getSourceNodeNum()
{
  dbCapNode* n = getSourceCapNode();
  return n->getNode();
}

/*
void
dbCCSeg::setSourceNode( uint source_node )
{
    _dbCCSeg * seg = (_dbCCSeg *) this;
    seg->_source_cap_node = source_node;
}
*/

uint dbCCSeg::getTargetNodeNum()
{
  dbCapNode* n = getTargetCapNode();
  return n->getNode();
}

/*
void
dbCCSeg::setTargetNode( uint target_node )
{
    _dbCCSeg * seg = (_dbCCSeg *) this;
    seg->_target_cap_node = target_node;
}
*/

dbNet* dbCCSeg::getSourceNet()
{
  dbCapNode* node = getSourceCapNode();
  return node->getNet();
}

dbNet* dbCCSeg::getTargetNet()
{
  dbCapNode* node = getTargetCapNode();
  return node->getNet();
}

uint dbCCSeg::getInfileCnt()
{
  _dbCCSeg* seg = (_dbCCSeg*) this;
  return (seg->_flags._inFileCnt);
}

void dbCCSeg::incrInfileCnt()
{
  _dbCCSeg* seg = (_dbCCSeg*) this;
  seg->_flags._inFileCnt++;
}

bool dbCCSeg::isMarked()
{
  _dbCCSeg* seg = (_dbCCSeg*) this;
  return seg->_flags._mark == 1;
}

void dbCCSeg::setMark(bool value)
{
  _dbCCSeg* seg = (_dbCCSeg*) this;
  seg->_flags._mark = (value == true) ? 1 : 0;
}

void dbCCSeg::printCapnCC(uint capn)
{
  _dbCCSeg* seg = (_dbCCSeg*) this;
  uint sidx;
  if (capn == seg->_cap_node[0])
    sidx = 0;
  else if (capn == seg->_cap_node[1])
    sidx = 1;
  else {
    debugPrint(getImpl()->getLogger(),
               utl::ODB,
               "DB_ECO",
               1,
               "ccSeg {} has capnd {} {}, not {} !",
               getId(),
               (uint) seg->_cap_node[0],
               (uint) seg->_cap_node[1],
               capn);
    return;
  }
  getImpl()->getLogger()->info(
      utl::ODB,
      21,
      "    ccSeg={} capn0={} next0={} capn1={} next1={}",
      getId(),
      (uint) seg->_cap_node[0],
      (uint) seg->_next[0],
      (uint) seg->_cap_node[1],
      (uint) seg->_next[1]);
  if (seg->_next[sidx] == 0) {
    return;
  }
  dbBlock* block = (dbBlock*) seg->getOwner();
  dbCCSeg* nseg = getCCSeg(block, seg->_next[sidx]);
  nseg->printCapnCC(capn);
}

bool dbCCSeg::checkCapnCC(uint capn)
{
  _dbCCSeg* seg = (_dbCCSeg*) this;
  uint sidx;
  if (capn == seg->_cap_node[0])
    sidx = 0;
  else if (capn == seg->_cap_node[1])
    sidx = 1;
  else {
    getImpl()->getLogger()->info(utl::ODB,
                                 22,
                                 "ccSeg {} has capnd {} {}, not {} !",
                                 getId(),
                                 (uint) seg->_cap_node[0],
                                 (uint) seg->_cap_node[1],
                                 capn);
    return false;
  }
  if (seg->_next[sidx] == 0) {
    return true;
  }
  dbBlock* block = (dbBlock*) seg->getOwner();
  dbCCSeg* nseg = getCCSeg(block, seg->_next[sidx]);
  bool rc = nseg->checkCapnCC(capn);
  return rc;
}

/*
 * TODO: ????
dbCCSeg *
dbCCSeg::relinkTgtCC (dbNet *net_, dbCCSeg *pseg_, uint src_cap_node, uint
tgt_cap_node)
{
    _dbNet *tnet = (_dbNet *)net_;
    _dbCCSeg *pseg = (_dbCCSeg *)pseg_;
    if (pseg && src_cap_node==0 && tgt_cap_node==0)
    {
        tnet->_cc_tgt_segs = pseg->getOID();
        return NULL;
    }
    dbTable<_dbCCSeg> *cct = ((_dbBlock *)tnet->getOwner())->_cc_seg_tbl;
    _dbCCSeg *seg;
    uint psid = 0;
    uint tsid;
    for (tsid = tnet->_cc_tgt_segs; tsid; tsid = seg->_next_target)
    {
        seg = cct->getPtr(tsid);
        if (seg->_source_cap_node == src_cap_node &&
            seg->_target_cap_node == tgt_cap_node)
            break;
        psid = tsid;
    }
    if (!tsid)
        return NULL;
    if (psid)
        cct->getPtr(psid)->_next_target = seg->_next_target;
    else
        tnet->_cc_tgt_segs = seg->_next_target;
    seg->_next_target = 0;
    if (pseg)
        ((_dbCCSeg *)pseg)->_next_target = tsid;
    return (dbCCSeg *) seg;
}
*/

static _dbCCSeg* findParallelCCSeg(_dbBlock* block,
                                   _dbCapNode* src,
                                   _dbCapNode* tgt,
                                   bool reInsert)
{
  uint src_id = src->getOID();
  uint tgt_id = tgt->getOID();
  _dbCCSeg* pccs = NULL;
  _dbCCSeg* ccs = NULL;
  uint seg;

  for (seg = tgt->_cc_segs; seg;) {
    ccs = block->_cc_seg_tbl->getPtr(seg);

    if (ccs->_cap_node[0] == tgt_id && ccs->_cap_node[1] == src_id)
      break;

    if (ccs->_cap_node[1] == tgt_id && ccs->_cap_node[0] == src_id)
      break;

    pccs = ccs;
    seg = ccs->next(tgt_id);
  }
  if (!seg)
    return NULL;
  if (!pccs || !reInsert)
    return ccs;
  pccs->_next[pccs->idx(tgt_id)] = ccs->next(tgt_id);
  ccs->_next[ccs->idx(tgt_id)] = tgt->_cc_segs;
  tgt->_cc_segs = ccs->getOID();
  return ccs;
}

dbCCSeg* dbCCSeg::findCC(dbCapNode* nodeA, dbCapNode* nodeB)
{
  _dbBlock* block = (_dbBlock*) nodeA->getImpl()->getOwner();
  _dbCCSeg* seg = findParallelCCSeg(
      block, (_dbCapNode*) nodeA, (_dbCapNode*) nodeB, false);
  return (dbCCSeg*) seg;
}

dbCCSeg* dbCCSeg::create(dbCapNode* src_, dbCapNode* tgt_, bool mergeParallel)
{
  _dbBlock* block = (_dbBlock*) src_->getImpl()->getOwner();

  uint srcNetId = src_->getNet()->getImpl()->getOID();
  uint tgtNetId = tgt_->getNet()->getImpl()->getOID();

  _dbCapNode* src = (_dbCapNode*) src_;
  _dbCapNode* tgt = (_dbCapNode*) tgt_;
  if (srcNetId > tgtNetId) {
    src = (_dbCapNode*) tgt_;
    tgt = (_dbCapNode*) src_;
  }

  if (block->_journal) {
    debugPrint(block->getImpl()->getLogger(),
               utl::ODB,
               "DB_ECO",
               1,
               "ECO: dbCCSeg::create, nodeA = {}, nodeB = {}, merge = {}",
               src->getOID(),
               tgt->getOID(),
               mergeParallel);

    block->_journal->beginAction(dbJournal::CREATE_OBJECT);
    block->_journal->pushParam(dbCCSegObj);
    block->_journal->pushParam(src->getOID());
    block->_journal->pushParam(tgt->getOID());
    block->_journal->pushParam(mergeParallel);
    block->_journal->endAction();
  }

  _dbCCSeg* seg;
  if (mergeParallel && (seg = findParallelCCSeg(block, src, tgt, true)))
    return (dbCCSeg*) seg;

  seg = block->_cc_seg_tbl->create();
  // seg->_flags._cnt = block->_num_corners;

  // set corner values
  uint cornerCnt = block->_corners_per_block;
  if (block->_maxCCSegId >= seg->getOID()) {
    for (uint ii = 0; ii < cornerCnt; ii++)
      (*block->_cc_val_tbl)[(seg->getOID() - 1) * cornerCnt + 1 + ii] = 0.0;
  } else {
    block->_maxCCSegId = seg->getOID();
    uint ccCapIdx = block->_cc_val_tbl->getIdx(cornerCnt, (float) 0.0);
    ZASSERT((seg->getOID() - 1) * cornerCnt + 1 == ccCapIdx);
  }

  seg->_cap_node[0] = src->getOID();
  seg->_next[0] = src->_cc_segs;
  src->_cc_segs = seg->getOID();

  seg->_cap_node[1] = tgt->getOID();
  seg->_next[1] = tgt->_cc_segs;
  tgt->_cc_segs = seg->getOID();
  return (dbCCSeg*) seg;
}

static void unlink_cc_seg(_dbBlock* block, _dbCapNode* node, _dbCCSeg* s)
{
  dbId<_dbCapNode> cid = node->getOID();
  uint prev = 0;
  uint next = node->_cc_segs;
  uint seg = s->getOID();

  while (next) {
    if (next == seg) {
      if (prev == 0)
        node->_cc_segs = s->next(cid);
      else {
        _dbCCSeg* p = block->_cc_seg_tbl->getPtr(prev);
        p->next(cid) = s->next(cid);
      }

      break;
    }

    _dbCCSeg* ncc = block->_cc_seg_tbl->getPtr(next);
    prev = next;
    next = ncc->next(cid);
  }
}

void dbCCSeg::unLink_cc_seg(dbCapNode* capn)
{
  _dbBlock* block = (_dbBlock*) getImpl()->getOwner();
  unlink_cc_seg(block, (_dbCapNode*) capn, (_dbCCSeg*) this);
  if (block->_journal) {
    debugPrint(getImpl()->getLogger(),
               utl::ODB,
               "DB_ECO",
               1,
               "ECO: dbCCSeg::unLink, ccseg: {}, capNode: {}",
               getId(),
               capn->getId());
    block->_journal->beginAction(dbJournal::UPDATE_FIELD);
    block->_journal->pushParam(dbCCSegObj);
    block->_journal->pushParam(getId());
    block->_journal->pushParam(_dbCCSeg::UNLINKCCSEG);
    block->_journal->pushParam(capn->getId());
    block->_journal->endAction();
  }
}

void dbCCSeg::Link_cc_seg(dbCapNode* capn, uint cseq)
{
  _dbBlock* block = (_dbBlock*) getImpl()->getOwner();
  _dbCapNode* tgt = (_dbCapNode*) capn;
  ((_dbCCSeg*) this)->_next[cseq] = tgt->_cc_segs;
  tgt->_cc_segs = getId();

  if (block->_journal) {
    debugPrint(getImpl()->getLogger(),
               utl::ODB,
               "DB_ECO",
               1,
               "ECO: dbCCSeg::Link, ccseg: {}, capNode: {}, cseq: {}",
               getId(),
               capn->getId(),
               cseq);
    block->_journal->beginAction(dbJournal::UPDATE_FIELD);
    block->_journal->pushParam(dbCCSegObj);
    block->_journal->pushParam(getId());
    block->_journal->pushParam(_dbCCSeg::LINKCCSEG);
    block->_journal->pushParam(capn->getId());
    block->_journal->pushParam(cseq);
    block->_journal->endAction();
  }
}

void dbCCSeg::disconnect(dbCCSeg* tcc_)
{
  _dbCCSeg* tcc = (_dbCCSeg*) tcc_;
  dbBlock* block = (dbBlock*) tcc->getOwner();

  dbCapNode* src = dbCapNode::getCapNode(block, tcc->_cap_node[0]);
  dbCapNode* tgt = dbCapNode::getCapNode(block, tcc->_cap_node[1]);
  tcc_->unLink_cc_seg(src);
  tcc_->unLink_cc_seg(tgt);
}

void dbCCSeg::connect(dbCCSeg* tcc_)
{
  _dbCCSeg* tcc = (_dbCCSeg*) tcc_;
  dbBlock* block = (dbBlock*) tcc->getOwner();

  dbCapNode* src = dbCapNode::getCapNode(block, tcc->_cap_node[0]);
  dbCapNode* tgt = dbCapNode::getCapNode(block, tcc->_cap_node[1]);
  tcc_->Link_cc_seg(src, 0);
  tcc_->Link_cc_seg(tgt, 1);
}
void dbCCSeg::destroy(dbCCSeg* seg_)
{
  _dbCCSeg* seg = (_dbCCSeg*) seg_;
  _dbBlock* block = (_dbBlock*) seg->getOwner();

  if (block->_journal) {
    debugPrint(block->getImpl()->getLogger(),
               utl::ODB,
               "DB_ECO",
               1,
               "ECO: dbCCSeg::destroy, seg id: {}",
               seg->getId());
    block->_journal->beginAction(dbJournal::DELETE_OBJECT);
    block->_journal->pushParam(dbCCSegObj);
    block->_journal->pushParam(seg->getId());
    block->_journal->pushParam((uint) 1);  //  regular destroy
    block->_journal->endAction();
  }

  _dbCapNode* src = block->_cap_node_tbl->getPtr(seg->_cap_node[0]);
  _dbCapNode* tgt = block->_cap_node_tbl->getPtr(seg->_cap_node[1]);
  unlink_cc_seg(block, src, seg);
  unlink_cc_seg(block, tgt, seg);
  dbProperty::destroyProperties(seg);
  block->_cc_seg_tbl->destroy(seg);
}
void dbCCSeg::destroyS(dbCCSeg* seg_)
{
  _dbCCSeg* seg = (_dbCCSeg*) seg_;
  _dbBlock* block = (_dbBlock*) seg->getOwner();

  if (block->_journal) {
    debugPrint(block->getImpl()->getLogger(),
               utl::ODB,
               "DB_ECO",
               1,
               "ECO: dbCCSeg::destroy, seg id: {}",
               seg->getId());
    block->_journal->beginAction(dbJournal::DELETE_OBJECT);
    block->_journal->pushParam(dbCCSegObj);
    block->_journal->pushParam(seg->getId());
    block->_journal->pushParam((uint) 0);  //  simple destroy
    block->_journal->endAction();
  }

  dbProperty::destroyProperties(seg);
  block->_cc_seg_tbl->destroy(seg);
}

dbSet<dbCCSeg>::iterator dbCCSeg::destroy(dbSet<dbCCSeg>::iterator& itr)
{
  dbCCSeg* bt = *itr;
  dbSet<dbCCSeg>::iterator next = ++itr;
  destroy(bt);
  return next;
}

void dbCCSeg::swapCapnode(dbCapNode* orig_, dbCapNode* new_)
{
  _dbCCSeg* seg = (_dbCCSeg*) this;
  _dbBlock* block = (_dbBlock*) seg->getOwner();
  _dbCapNode* orig = (_dbCapNode*) orig_;
  _dbCapNode* newn = (_dbCapNode*) new_;
  uint oid = orig->getOID();
  uint nid = newn->getOID();
  uint sidx;
  if (oid == seg->_cap_node[0]) {
    sidx = 0;
  } else if (oid == seg->_cap_node[1]) {
    sidx = 1;
  } else {
    getImpl()->getLogger()->error(
        utl::ODB,
        23,
        "CCSeg {} does not have orig capNode {}. Can not swap.",
        seg->getOID(),
        oid);
  }
  unlink_cc_seg(block, orig, seg);
  seg->_cap_node[sidx] = nid;
  seg->_next[sidx] = newn->_cc_segs;
  newn->_cc_segs = seg->getOID();
  if (block->_journal) {
    debugPrint(getImpl()->getLogger(),
               utl::ODB,
               "DB_ECO",
               1,
               "ECO: dbCCSeg {}, origCapNode {}, newCapNode {}, swapCapnode",
               seg->getOID(),
               oid,
               nid);
    block->_journal->beginAction(dbJournal::UPDATE_FIELD);
    block->_journal->pushParam(dbCCSegObj);
    block->_journal->pushParam(seg->getId());
    block->_journal->pushParam(_dbCCSeg::SWAPCAPNODE);
    block->_journal->pushParam(oid);
    block->_journal->pushParam(nid);
    block->_journal->endAction();
  }
}

dbCapNode* dbCCSeg::getTheOtherCapn(dbCapNode* oneCap, uint& cid)
{
  _dbCCSeg* seg = (_dbCCSeg*) this;
  _dbBlock* block = (_dbBlock*) seg->getOwner();
  cid = ((_dbCapNode*) oneCap)->getOID() == seg->_cap_node[1] ? 0 : 1;
  _dbCapNode* n = block->_cap_node_tbl->getPtr(seg->_cap_node[cid]);
  return (dbCapNode*) n;
}

dbCCSeg* dbCCSeg::getCCSeg(dbBlock* block_, uint dbid_)
{
  _dbBlock* block = (_dbBlock*) block_;
  return (dbCCSeg*) block->_cc_seg_tbl->getPtr(dbid_);
}

}  // namespace odb
