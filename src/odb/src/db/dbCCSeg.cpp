// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "dbCCSeg.h"

#include <cassert>
#include <cstdint>
#include <cstdio>

#include "dbBlock.h"
#include "dbCapNode.h"
#include "dbCore.h"
#include "dbDatabase.h"
#include "dbJournal.h"
#include "dbNet.h"
#include "dbTable.h"
#include "odb/db.h"
#include "odb/dbObject.h"
#include "odb/dbSet.h"
#include "utl/Logger.h"

namespace odb {

template class dbTable<_dbCCSeg>;

bool _dbCCSeg::operator==(const _dbCCSeg& rhs) const
{
  if (flags_.spef_mark_1 != rhs.flags_.spef_mark_1) {
    return false;
  }

  if (flags_.mark != rhs.flags_.mark) {
    return false;
  }

  if (flags_.inFileCnt != rhs.flags_.inFileCnt) {
    return false;
  }

  if (cap_node_[0] != rhs.cap_node_[0]) {
    return false;
  }

  if (cap_node_[1] != rhs.cap_node_[1]) {
    return false;
  }

  if (next_[0] != rhs.next_[0]) {
    return false;
  }

  if (next_[1] != rhs.next_[1]) {
    return false;
  }

  return true;
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
      = (*block->cc_val_tbl_)[((seg->getOID() - 1) * block->corners_per_block_)
                              + 1 + corner];
  float prev_value = value;
  value *= factor;

  debugPrint(getImpl()->getLogger(),
             utl::ODB,
             "DB_EDIT",
             2,
             "EDIT: dbCCSeg {}, adjustCapacitance {}, corner {}",
             seg->getId(),
             factor,
             corner);

  if (block->journal_) {
    block->journal_->beginAction(dbJournal::kUpdateField);
    block->journal_->pushParam(dbCCSegObj);
    block->journal_->pushParam(seg->getId());
    block->journal_->pushParam(_dbCCSeg::kCapacitance);
    block->journal_->pushParam(prev_value);
    block->journal_->pushParam(value);
    block->journal_->pushParam(0);
    block->journal_->endAction();
  }
}

void dbCCSeg::adjustCapacitance(float factor)
{
  _dbBlock* block = (_dbBlock*) getImpl()->getOwner();
  uint32_t corner;
  for (corner = 0; corner < block->corners_per_block_; corner++) {
    adjustCapacitance(factor, corner);
  }
}

double dbCCSeg::getCapacitance(int corner)
{
  _dbCCSeg* seg = (_dbCCSeg*) this;
  _dbBlock* block = (_dbBlock*) seg->getOwner();
  uint32_t cornerCnt = block->corners_per_block_;
  assert((corner >= 0) && ((uint32_t) corner < cornerCnt));
  return (*block->cc_val_tbl_)[((seg->getOID() - 1) * cornerCnt) + 1 + corner];
}

void dbCCSeg::accAllCcCap(double* ttcap, double miller_mult)
{
  _dbCCSeg* seg = (_dbCCSeg*) this;
  _dbBlock* block = (_dbBlock*) seg->getOwner();
  uint32_t cornerCnt = block->corners_per_block_;
  for (uint32_t ii = 0; ii < cornerCnt; ii++) {
    ttcap[ii]
        += ((*block->cc_val_tbl_)[((seg->getOID() - 1) * cornerCnt) + 1 + ii])
           * miller_mult;
  }
}

void dbCCSeg::getAllCcCap(double* ttcap)
{
  _dbCCSeg* seg = (_dbCCSeg*) this;
  _dbBlock* block = (_dbBlock*) seg->getOwner();
  uint32_t cornerCnt = block->corners_per_block_;
  for (uint32_t ii = 0; ii < cornerCnt; ii++) {
    ttcap[ii]
        = (*block->cc_val_tbl_)[((seg->getOID() - 1) * cornerCnt) + 1 + ii];
  }
}

void dbCCSeg::setAllCcCap(double* ttcap)
{
  _dbCCSeg* seg = (_dbCCSeg*) this;
  _dbBlock* block = (_dbBlock*) seg->getOwner();
  uint32_t cornerCnt = block->corners_per_block_;
  for (uint32_t ii = 0; ii < cornerCnt; ii++) {
    (*block->cc_val_tbl_)[((seg->getOID() - 1) * cornerCnt) + 1 + ii]
        = ttcap[ii];
  }
  debugPrint(getImpl()->getLogger(),
             utl::ODB,
             "DB_EDIT",
             2,
             "EDIT: dbCCSeg::setAllCcCap, ccseg: {}, caps: {}",
             seg->getId(),
             ttcap[0]);

  if (block->journal_) {
    char ccCaps[400];
    int pos = 0;
    ccCaps[0] = '\0';
    for (uint32_t ii = 0; ii < cornerCnt; ii++) {
      pos += sprintf(&ccCaps[pos], "%f ", ttcap[ii]);
    }
    block->journal_->beginAction(dbJournal::kUpdateField);
    block->journal_->pushParam(dbCCSegObj);
    block->journal_->pushParam(getId());
    block->journal_->pushParam(_dbCCSeg::kSetAllCcCap);
    for (uint32_t ii = 0; ii < cornerCnt; ii++) {
      block->journal_->pushParam(ttcap[ii]);
    }
    block->journal_->endAction();
  }
}

void dbCCSeg::setCapacitance(double cap, int corner)
{
  _dbCCSeg* seg = (_dbCCSeg*) this;
  _dbBlock* block = (_dbBlock*) seg->getOwner();
  uint32_t cornerCnt = block->corners_per_block_;
  assert((corner >= 0) && ((uint32_t) corner < cornerCnt));

  float& value
      = (*block->cc_val_tbl_)[((seg->getOID() - 1) * cornerCnt) + 1 + corner];
  float prev_value = value;
  value = (float) cap;

  debugPrint(getImpl()->getLogger(),
             utl::ODB,
             "DB_EDIT",
             2,
             "EDIT: dbCCSeg {}, setCapacitance {}, corner {}",
             seg->getId(),
             value,
             corner);

  if (block->journal_) {
    block->journal_->beginAction(dbJournal::kUpdateField);
    block->journal_->pushParam(dbCCSegObj);
    block->journal_->pushParam(seg->getId());
    block->journal_->pushParam(_dbCCSeg::kCapacitance);
    block->journal_->pushParam(prev_value);
    block->journal_->pushParam(value);
    block->journal_->pushParam(corner);
    block->journal_->endAction();
  }
}

void dbCCSeg::addCapacitance(double cap, int corner)
{
  _dbCCSeg* seg = (_dbCCSeg*) this;
  _dbBlock* block = (_dbBlock*) seg->getOwner();
  uint32_t cornerCnt = block->corners_per_block_;
  assert((corner >= 0) && ((uint32_t) corner < cornerCnt));

  float& value
      = (*block->cc_val_tbl_)[((seg->getOID() - 1) * cornerCnt) + 1 + corner];
  float prev_value = value;
  value += (float) cap;

  debugPrint(getImpl()->getLogger(),
             utl::ODB,
             "DB_EDIT",
             2,
             "EDIT: dbCCSeg {}, addCapacitance {}, corner {}",
             seg->getId(),
             value,
             corner);

  if (block->journal_) {
    block->journal_->beginAction(dbJournal::kUpdateField);
    block->journal_->pushParam(dbCCSegObj);
    block->journal_->pushParam(seg->getId());
    block->journal_->pushParam(_dbCCSeg::kCapacitance);
    block->journal_->pushParam(prev_value);
    block->journal_->pushParam(value);
    block->journal_->pushParam(corner);
    block->journal_->endAction();
  }
}

void dbCCSeg::addCcCapacitance(dbCCSeg* other)
{
  _dbCCSeg* seg = (_dbCCSeg*) this;
  _dbBlock* block = (_dbBlock*) seg->getOwner();
  _dbCCSeg* oseg = (_dbCCSeg*) other;
  uint32_t cornerCnt = block->corners_per_block_;

  for (uint32_t ii = 0; ii < cornerCnt; ii++) {
    float& value
        = (*block->cc_val_tbl_)[((seg->getOID() - 1) * cornerCnt) + 1 + ii];
    float& ovalue
        = (*block->cc_val_tbl_)[((oseg->getOID() - 1) * cornerCnt) + 1 + ii];
    value += ovalue;
  }

  debugPrint(getImpl()->getLogger(),
             utl::ODB,
             "DB_EDIT",
             2,
             "EDIT: dbCCSeg {}, other dbCCSeg {}, addCcCapacitance",
             seg->getOID(),
             oseg->getOID());

  if (block->journal_) {
    block->journal_->beginAction(dbJournal::kUpdateField);
    block->journal_->pushParam(dbCCSegObj);
    block->journal_->pushParam(seg->getId());
    block->journal_->pushParam(_dbCCSeg::kAddCcCapacitance);
    block->journal_->pushParam(oseg->getId());
    block->journal_->endAction();
  }
}

dbCapNode* dbCCSeg::getSourceCapNode()
{
  _dbCCSeg* seg = (_dbCCSeg*) this;
  _dbBlock* block = (_dbBlock*) seg->getOwner();
  _dbCapNode* n = block->cap_node_tbl_->getPtr(seg->cap_node_[0]);
  return (dbCapNode*) n;
}

dbCapNode* dbCCSeg::getTargetCapNode()
{
  _dbCCSeg* seg = (_dbCCSeg*) this;
  _dbBlock* block = (_dbBlock*) seg->getOwner();
  _dbCapNode* n = block->cap_node_tbl_->getPtr(seg->cap_node_[1]);
  return (dbCapNode*) n;
}

uint32_t dbCCSeg::getSourceNodeNum()
{
  dbCapNode* n = getSourceCapNode();
  return n->getNode();
}

/*
void
dbCCSeg::setSourceNode( uint32_t source_node )
{
    _dbCCSeg * seg = (_dbCCSeg *) this;
    seg->_source_cap_node = source_node;
}
*/

uint32_t dbCCSeg::getTargetNodeNum()
{
  dbCapNode* n = getTargetCapNode();
  return n->getNode();
}

/*
void
dbCCSeg::setTargetNode( uint32_t target_node )
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

uint32_t dbCCSeg::getInfileCnt()
{
  _dbCCSeg* seg = (_dbCCSeg*) this;
  return (seg->flags_.inFileCnt);
}

void dbCCSeg::incrInfileCnt()
{
  _dbCCSeg* seg = (_dbCCSeg*) this;
  seg->flags_.inFileCnt++;
}

bool dbCCSeg::isMarked()
{
  _dbCCSeg* seg = (_dbCCSeg*) this;
  return seg->flags_.mark == 1;
}

void dbCCSeg::setMark(bool value)
{
  _dbCCSeg* seg = (_dbCCSeg*) this;
  seg->flags_.mark = (value == true) ? 1 : 0;
}

void dbCCSeg::printCapnCC(uint32_t capn)
{
  _dbCCSeg* seg = (_dbCCSeg*) this;
  uint32_t sidx;
  if (capn == seg->cap_node_[0]) {
    sidx = 0;
  } else if (capn == seg->cap_node_[1]) {
    sidx = 1;
  } else {
    debugPrint(getImpl()->getLogger(),
               utl::ODB,
               "DB_EDIT",
               2,
               "ccSeg {} has capnd {} {}, not {} !",
               getId(),
               (uint32_t) seg->cap_node_[0],
               (uint32_t) seg->cap_node_[1],
               capn);
    return;
  }
  getImpl()->getLogger()->info(
      utl::ODB,
      21,
      "    ccSeg={} capn0={} next0={} capn1={} next1={}",
      getId(),
      (uint32_t) seg->cap_node_[0],
      (uint32_t) seg->next_[0],
      (uint32_t) seg->cap_node_[1],
      (uint32_t) seg->next_[1]);
  if (seg->next_[sidx] == 0) {
    return;
  }
  dbBlock* block = (dbBlock*) seg->getOwner();
  dbCCSeg* nseg = getCCSeg(block, seg->next_[sidx]);
  nseg->printCapnCC(capn);
}

bool dbCCSeg::checkCapnCC(uint32_t capn)
{
  _dbCCSeg* seg = (_dbCCSeg*) this;
  uint32_t sidx;
  if (capn == seg->cap_node_[0]) {
    sidx = 0;
  } else if (capn == seg->cap_node_[1]) {
    sidx = 1;
  } else {
    getImpl()->getLogger()->info(utl::ODB,
                                 22,
                                 "ccSeg {} has capnd {} {}, not {} !",
                                 getId(),
                                 (uint32_t) seg->cap_node_[0],
                                 (uint32_t) seg->cap_node_[1],
                                 capn);
    return false;
  }
  if (seg->next_[sidx] == 0) {
    return true;
  }
  dbBlock* block = (dbBlock*) seg->getOwner();
  dbCCSeg* nseg = getCCSeg(block, seg->next_[sidx]);
  bool rc = nseg->checkCapnCC(capn);
  return rc;
}

/*
 * TODO: ????
dbCCSeg *
dbCCSeg::relinkTgtCC (dbNet *net_, dbCCSeg *pseg_, uint32_t src_cap_node,
uint32_t tgt_cap_node)
{
    _dbNet *tnet = (_dbNet *)net_;
    _dbCCSeg *pseg = (_dbCCSeg *)pseg_;
    if (pseg && src_cap_node==0 && tgt_cap_node==0)
    {
        tnet->_cc_tgt_segs = pseg->getOID();
        return nullptr;
    }
    dbTable<_dbCCSeg> *cct = ((_dbBlock *)tnet->getOwner())->_cc_seg_tbl;
    _dbCCSeg *seg;
    uint32_t psid = 0;
    uint32_t tsid;
    for (tsid = tnet->_cc_tgt_segs; tsid; tsid = seg->_next_target)
    {
        seg = cct->getPtr(tsid);
        if (seg->_source_cap_node == src_cap_node &&
            seg->_target_cap_node == tgt_cap_node)
            break;
        psid = tsid;
    }
    if (!tsid)
        return nullptr;
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
  uint32_t src_id = src->getOID();
  uint32_t tgt_id = tgt->getOID();
  _dbCCSeg* pccs = nullptr;
  _dbCCSeg* ccs = nullptr;
  uint32_t seg;

  for (seg = tgt->cc_segs_; seg;) {
    ccs = block->cc_seg_tbl_->getPtr(seg);

    if (ccs->cap_node_[0] == tgt_id && ccs->cap_node_[1] == src_id) {
      break;
    }

    if (ccs->cap_node_[1] == tgt_id && ccs->cap_node_[0] == src_id) {
      break;
    }

    pccs = ccs;
    seg = ccs->next(tgt_id);
  }
  if (!seg) {
    return nullptr;
  }
  if (!pccs || !reInsert) {
    return ccs;
  }
  pccs->next_[pccs->idx(tgt_id)] = ccs->next(tgt_id);
  ccs->next_[ccs->idx(tgt_id)] = tgt->cc_segs_;
  tgt->cc_segs_ = ccs->getOID();
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

  uint32_t srcNetId = src_->getNet()->getImpl()->getOID();
  uint32_t tgtNetId = tgt_->getNet()->getImpl()->getOID();

  _dbCapNode* src = (_dbCapNode*) src_;
  _dbCapNode* tgt = (_dbCapNode*) tgt_;
  if (srcNetId > tgtNetId) {
    src = (_dbCapNode*) tgt_;
    tgt = (_dbCapNode*) src_;
  }

  debugPrint(block->getImpl()->getLogger(),
             utl::ODB,
             "DB_EDIT",
             2,
             "EDIT: dbCCSeg::create, nodeA = {}, nodeB = {}, merge = {}",
             src->getOID(),
             tgt->getOID(),
             mergeParallel);

  if (block->journal_) {
    block->journal_->beginAction(dbJournal::kCreateObject);
    block->journal_->pushParam(dbCCSegObj);
    block->journal_->pushParam(src->getOID());
    block->journal_->pushParam(tgt->getOID());
    block->journal_->pushParam(mergeParallel);
    block->journal_->endAction();
  }

  _dbCCSeg* seg;
  if (mergeParallel) {
    seg = findParallelCCSeg(block, src, tgt, true);
    if (seg) {
      return (dbCCSeg*) seg;
    }
  }

  seg = block->cc_seg_tbl_->create();
  // seg->flags_._cnt = block->_num_corners;

  // set corner values
  uint32_t cornerCnt = block->corners_per_block_;
  if (block->max_cc_seg_id_ >= seg->getOID()) {
    for (uint32_t ii = 0; ii < cornerCnt; ii++) {
      (*block->cc_val_tbl_)[((seg->getOID() - 1) * cornerCnt) + 1 + ii] = 0.0;
    }
  } else {
    block->max_cc_seg_id_ = seg->getOID();
    [[maybe_unused]] uint32_t ccCapIdx
        = block->cc_val_tbl_->getIdx(cornerCnt, (float) 0.0);
    assert((seg->getOID() - 1) * cornerCnt + 1 == ccCapIdx);
  }

  seg->cap_node_[0] = src->getOID();
  seg->next_[0] = src->cc_segs_;
  src->cc_segs_ = seg->getOID();

  seg->cap_node_[1] = tgt->getOID();
  seg->next_[1] = tgt->cc_segs_;
  tgt->cc_segs_ = seg->getOID();
  return (dbCCSeg*) seg;
}

static void unlink_cc_seg(_dbBlock* block, _dbCapNode* node, _dbCCSeg* s)
{
  dbId<_dbCapNode> cid = node->getOID();
  uint32_t prev = 0;
  uint32_t next = node->cc_segs_;
  uint32_t seg = s->getOID();

  while (next) {
    if (next == seg) {
      if (prev == 0) {
        node->cc_segs_ = s->next(cid);
      } else {
        _dbCCSeg* p = block->cc_seg_tbl_->getPtr(prev);
        p->next(cid) = s->next(cid);
      }

      break;
    }

    _dbCCSeg* ncc = block->cc_seg_tbl_->getPtr(next);
    prev = next;
    next = ncc->next(cid);
  }
}

void dbCCSeg::unLink_cc_seg(dbCapNode* capn)
{
  _dbBlock* block = (_dbBlock*) getImpl()->getOwner();
  unlink_cc_seg(block, (_dbCapNode*) capn, (_dbCCSeg*) this);
  debugPrint(getImpl()->getLogger(),
             utl::ODB,
             "DB_EDIT",
             2,
             "EDIT: dbCCSeg::unLink, ccseg: {}, capNode: {}",
             getId(),
             capn->getId());

  if (block->journal_) {
    block->journal_->beginAction(dbJournal::kUpdateField);
    block->journal_->pushParam(dbCCSegObj);
    block->journal_->pushParam(getId());
    block->journal_->pushParam(_dbCCSeg::kUnlinkCcSeg);
    block->journal_->pushParam(capn->getId());
    block->journal_->endAction();
  }
}

void dbCCSeg::Link_cc_seg(dbCapNode* capn, uint32_t cseq)
{
  _dbBlock* block = (_dbBlock*) getImpl()->getOwner();
  _dbCapNode* tgt = (_dbCapNode*) capn;
  ((_dbCCSeg*) this)->next_[cseq] = tgt->cc_segs_;
  tgt->cc_segs_ = getId();

  debugPrint(getImpl()->getLogger(),
             utl::ODB,
             "DB_EDIT",
             2,
             "EDIT: dbCCSeg::Link, ccseg: {}, capNode: {}, cseq: {}",
             getId(),
             capn->getId(),
             cseq);

  if (block->journal_) {
    block->journal_->beginAction(dbJournal::kUpdateField);
    block->journal_->pushParam(dbCCSegObj);
    block->journal_->pushParam(getId());
    block->journal_->pushParam(_dbCCSeg::kLinkCcSeg);
    block->journal_->pushParam(capn->getId());
    block->journal_->pushParam(cseq);
    block->journal_->endAction();
  }
}

void dbCCSeg::disconnect(dbCCSeg* tcc_)
{
  _dbCCSeg* tcc = (_dbCCSeg*) tcc_;
  dbBlock* block = (dbBlock*) tcc->getOwner();

  dbCapNode* src = dbCapNode::getCapNode(block, tcc->cap_node_[0]);
  dbCapNode* tgt = dbCapNode::getCapNode(block, tcc->cap_node_[1]);
  tcc_->unLink_cc_seg(src);
  tcc_->unLink_cc_seg(tgt);
}

void dbCCSeg::connect(dbCCSeg* tcc_)
{
  _dbCCSeg* tcc = (_dbCCSeg*) tcc_;
  dbBlock* block = (dbBlock*) tcc->getOwner();

  dbCapNode* src = dbCapNode::getCapNode(block, tcc->cap_node_[0]);
  dbCapNode* tgt = dbCapNode::getCapNode(block, tcc->cap_node_[1]);
  tcc_->Link_cc_seg(src, 0);
  tcc_->Link_cc_seg(tgt, 1);
}
void dbCCSeg::destroy(dbCCSeg* seg_)
{
  _dbCCSeg* seg = (_dbCCSeg*) seg_;
  _dbBlock* block = (_dbBlock*) seg->getOwner();

  debugPrint(block->getImpl()->getLogger(),
             utl::ODB,
             "DB_EDIT",
             2,
             "EDIT: dbCCSeg::destroy, seg id: {}",
             seg->getId());

  if (block->journal_) {
    block->journal_->beginAction(dbJournal::kDeleteObject);
    block->journal_->pushParam(dbCCSegObj);
    block->journal_->pushParam(seg->getId());
    block->journal_->pushParam((uint32_t) 1);  //  regular destroy
    block->journal_->endAction();
  }

  _dbCapNode* src = block->cap_node_tbl_->getPtr(seg->cap_node_[0]);
  _dbCapNode* tgt = block->cap_node_tbl_->getPtr(seg->cap_node_[1]);
  unlink_cc_seg(block, src, seg);
  unlink_cc_seg(block, tgt, seg);
  dbProperty::destroyProperties(seg);
  block->cc_seg_tbl_->destroy(seg);
}
void dbCCSeg::destroyS(dbCCSeg* seg_)
{
  _dbCCSeg* seg = (_dbCCSeg*) seg_;
  _dbBlock* block = (_dbBlock*) seg->getOwner();

  debugPrint(block->getImpl()->getLogger(),
             utl::ODB,
             "DB_EDIT",
             2,
             "EDIT: dbCCSeg::destroy, seg id: {}",
             seg->getId());

  if (block->journal_) {
    block->journal_->beginAction(dbJournal::kDeleteObject);
    block->journal_->pushParam(dbCCSegObj);
    block->journal_->pushParam(seg->getId());
    block->journal_->pushParam((uint32_t) 0);  //  simple destroy
    block->journal_->endAction();
  }

  dbProperty::destroyProperties(seg);
  block->cc_seg_tbl_->destroy(seg);
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
  uint32_t oid = orig->getOID();
  uint32_t nid = newn->getOID();
  uint32_t sidx;
  if (oid == seg->cap_node_[0]) {
    sidx = 0;
  } else if (oid == seg->cap_node_[1]) {
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
  seg->cap_node_[sidx] = nid;
  seg->next_[sidx] = newn->cc_segs_;
  newn->cc_segs_ = seg->getOID();
  debugPrint(getImpl()->getLogger(),
             utl::ODB,
             "DB_EDIT",
             2,
             "EDIT: dbCCSeg {}, origCapNode {}, newCapNode {}, swapCapnode",
             seg->getOID(),
             oid,
             nid);

  if (block->journal_) {
    block->journal_->beginAction(dbJournal::kUpdateField);
    block->journal_->pushParam(dbCCSegObj);
    block->journal_->pushParam(seg->getId());
    block->journal_->pushParam(_dbCCSeg::kSwapCapNode);
    block->journal_->pushParam(oid);
    block->journal_->pushParam(nid);
    block->journal_->endAction();
  }
}

dbCapNode* dbCCSeg::getTheOtherCapn(dbCapNode* oneCap, uint32_t& cid)
{
  _dbCCSeg* seg = (_dbCCSeg*) this;
  _dbBlock* block = (_dbBlock*) seg->getOwner();
  cid = ((_dbCapNode*) oneCap)->getOID() == seg->cap_node_[1] ? 0 : 1;
  _dbCapNode* n = block->cap_node_tbl_->getPtr(seg->cap_node_[cid]);
  return (dbCapNode*) n;
}

dbCCSeg* dbCCSeg::getCCSeg(dbBlock* block_, uint32_t dbid_)
{
  _dbBlock* block = (_dbBlock*) block_;
  return (dbCCSeg*) block->cc_seg_tbl_->getPtr(dbid_);
}

void _dbCCSeg::collectMemInfo(MemInfo& info)
{
  info.cnt++;
  info.size += sizeof(*this);
}

}  // namespace odb
