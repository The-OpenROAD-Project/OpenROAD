// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "dbCapNode.h"

#include <cassert>
#include <cstdint>
#include <vector>

#include "dbBlock.h"
#include "dbCCSeg.h"
#include "dbCCSegItr.h"
#include "dbCommon.h"
#include "dbCore.h"
#include "dbDatabase.h"
#include "dbJournal.h"
#include "dbNet.h"
#include "dbTable.h"
#include "odb/db.h"
#include "odb/dbObject.h"
#include "odb/dbSet.h"
#include "odb/dbTypes.h"
#include "utl/Logger.h"

namespace odb {

template class dbTable<_dbCapNode>;

bool _dbCapNode::operator==(const _dbCapNode& rhs) const
{
  if (flags_.name != rhs.flags_.name) {
    return false;
  }

  if (flags_.internal != rhs.flags_.internal) {
    return false;
  }

  if (flags_.iterm != rhs.flags_.iterm) {
    return false;
  }

  if (flags_.bterm != rhs.flags_.bterm) {
    return false;
  }

  if (flags_.branch != rhs.flags_.branch) {
    return false;
  }

  if (flags_.foreign != rhs.flags_.foreign) {
    return false;
  }

  if (flags_.childrenCnt != rhs.flags_.childrenCnt) {
    return false;
  }

  if (flags_.select != rhs.flags_.select) {
    return false;
  }

  if (node_num_ != rhs.node_num_) {
    return false;
  }

  if (net_ != rhs.net_) {
    return false;
  }

  if (next_ != rhs.next_) {
    return false;
  }

  if (cc_segs_ != rhs.cc_segs_) {
    return false;
  }

  return true;
}

////////////////////////////////////////////////////////////////////
//
// dbCapNode - Methods
//
////////////////////////////////////////////////////////////////////

bool dbCapNode::needAdjustCC(double ccThreshHold)
{
  const uint32_t cornerCnt
      = ((dbBlock*) getImpl()->getOwner())->getCornerCount();
  for (dbCCSeg* cc : getCCSegs()) {
    for (uint32_t corner = 0; corner < cornerCnt; corner++) {
      uint32_t cid;
      if (cc->getTheOtherCapn(this, cid)->getNet()->getCcAdjustFactor() > 0) {
        continue;
      }
      if (cc->getCapacitance(corner) >= ccThreshHold) {
        return true;
      }
    }
  }
  return false;
}

bool dbCapNode::groundCC(float gndFactor)
{
  bool grounded = false;
  const uint32_t vicNetId = getNet()->getId();
  _dbBlock* block = (_dbBlock*) getImpl()->getOwner();
  const uint32_t cornerCnt = block->corners_per_block_;
  for (dbCCSeg* cc : getCCSegs()) {
    uint32_t cid;
    dbCapNode* agrNode = cc->getTheOtherCapn(this, cid);
    uint32_t agrNetId = agrNode->getNet()->getId();
    if (agrNetId < vicNetId) {
      continue;  //  avoid duplicate grounding
    }
    if (agrNetId == vicNetId) {
      getImpl()->getLogger()->warn(
          utl::ODB,
          24,
          "cc seg {} has both capNodes {} {} from the same net {} . "
          "ignored by groundCC .",
          cc->getId(),
          getId(),
          agrNode->getId(),
          agrNetId);
      continue;
    }
    grounded = true;
    for (uint32_t ii = 0; ii < cornerCnt; ii++) {
      const double deltaC = cc->getCapacitance(ii) * gndFactor;
      addCapacitance(deltaC, ii);
      agrNode->addCapacitance(deltaC, ii);
    }
  }
  return grounded;
}

void dbCapNode::adjustCC(uint32_t adjOrder,
                         float adjFactor,
                         std::vector<dbCCSeg*>& adjustedCC,
                         std::vector<dbNet*>& halonets)
{
  for (dbCCSeg* cc : getCCSegs()) {
    if (cc->isMarked()) {
      continue;
    }
    uint32_t cid;
    dbNet* agrNet = cc->getTheOtherCapn(this, cid)->getNet();
    if (agrNet->getCcAdjustFactor() > 0
        && agrNet->getCcAdjustOrder() < adjOrder) {
      continue;
    }
    adjustedCC.push_back(cc);
    cc->setMark(true);
    cc->adjustCapacitance(adjFactor);
    if (!agrNet->isMark_1ed()) {
      agrNet->setMark_1(true);
      halonets.push_back(agrNet);
    }
  }
}

void dbCapNode::adjustCapacitance(float factor, uint32_t corner)
{
  _dbCapNode* seg = (_dbCapNode*) this;
  _dbBlock* block = (_dbBlock*) seg->getOwner();
  uint32_t cornerCnt = block->corners_per_block_;

  assert(seg->flags_.foreign > 0);
  assert(corner < cornerCnt);
  float& value
      = (*block->c_val_tbl_)[((seg->getOID() - 1) * cornerCnt) + 1 + corner];
  float prev_value = value;
  value *= factor;

  debugPrint(getImpl()->getLogger(),
             utl::ODB,
             "DB_EDIT",
             2,
             "EDIT: dbCapNode {}, adjustCapacitance {}, corner {}",
             seg->getId(),
             factor,
             corner);

  if (block->journal_) {
    block->journal_->beginAction(dbJournal::kUpdateField);
    block->journal_->pushParam(dbCapNodeObj);
    block->journal_->pushParam(seg->getId());
    block->journal_->pushParam(_dbCapNode::kCapacitance);
    block->journal_->pushParam(prev_value);
    block->journal_->pushParam(value);
    block->journal_->pushParam(0);
    block->journal_->endAction();
  }
}

void dbCapNode::adjustCapacitance(float factor)
{
  _dbBlock* block = (_dbBlock*) getImpl()->getOwner();
  uint32_t cornerCnt = block->corners_per_block_;
  uint32_t corner;
  for (corner = 0; corner < cornerCnt; corner++) {
    adjustCapacitance(factor, corner);
  }
}

double dbCapNode::getCapacitance(uint32_t corner)
{
  _dbCapNode* seg = (_dbCapNode*) this;
  _dbBlock* block = (_dbBlock*) seg->getOwner();
  uint32_t cornerCnt = block->corners_per_block_;

  if (seg->flags_.foreign > 0) {
    assert(corner < cornerCnt);
    return (*block->c_val_tbl_)[((seg->getOID() - 1) * cornerCnt) + 1 + corner];
  }
  return 0.0;
}

void dbCapNode::getGndCap(double* gndcap, double* totalcap)
{
  _dbCapNode* seg = (_dbCapNode*) this;
  if (seg->flags_.foreign == 0) {
    return;
  }
  _dbBlock* block = (_dbBlock*) seg->getOwner();
  uint32_t cornerCnt = block->corners_per_block_;
  double gcap;
  for (uint32_t ii = 0; ii < cornerCnt; ii++) {
    gcap = (*block->c_val_tbl_)[((seg->getOID() - 1) * cornerCnt) + 1 + ii];
    if (gndcap) {
      gndcap[ii] = gcap;
    }
    if (totalcap) {
      totalcap[ii] = gcap;
    }
  }
}

void dbCapNode::addGndCap(double* gndcap, double* totalcap)
{
  _dbCapNode* seg = (_dbCapNode*) this;
  if (seg->flags_.foreign == 0) {
    return;
  }
  _dbBlock* block = (_dbBlock*) seg->getOwner();
  uint32_t cornerCnt = block->corners_per_block_;
  double gcap;
  for (uint32_t ii = 0; ii < cornerCnt; ii++) {
    gcap = (*block->c_val_tbl_)[((seg->getOID() - 1) * cornerCnt) + 1 + ii];
    if (gndcap) {
      gndcap[ii] += gcap;
    }
    if (totalcap) {
      totalcap[ii] += gcap;
    }
  }
}

void dbCapNode::accAllCcCap(double* totalcap, double miller_mult)
{
  if (totalcap == nullptr || miller_mult == 0) {
    return;
  }
  for (dbCCSeg* cc : getCCSegs()) {
    cc->accAllCcCap(totalcap, miller_mult);
  }
}

void dbCapNode::getGndTotalCap(double* gndcap,
                               double* totalcap,
                               double miller_mult)
{
  getGndCap(gndcap, totalcap);
  accAllCcCap(totalcap, miller_mult);
}

void dbCapNode::addGndTotalCap(double* gndcap,
                               double* totalcap,
                               double miller_mult)
{
  addGndCap(gndcap, totalcap);
  accAllCcCap(totalcap, miller_mult);
}

void dbCapNode::getCapTable(double* cap)
{
  _dbCapNode* seg = (_dbCapNode*) this;
  _dbBlock* block = (_dbBlock*) seg->getOwner();
  uint32_t cornerCnt = block->corners_per_block_;

  for (uint32_t ii = 0; ii < cornerCnt; ii++) {
    cap[ii] = (*block->c_val_tbl_)[((seg->getOID() - 1) * cornerCnt) + 1 + ii];
  }
}

void dbCapNode::addCapnCapacitance(dbCapNode* other)
{
  _dbCapNode* seg = (_dbCapNode*) this;
  _dbCapNode* oseg = (_dbCapNode*) other;
  _dbBlock* block = (_dbBlock*) seg->getOwner();
  uint32_t cornerCnt = block->corners_per_block_;
  for (uint32_t corner = 0; corner < cornerCnt; corner++) {
    float& value
        = (*block->c_val_tbl_)[((seg->getOID() - 1) * cornerCnt) + 1 + corner];
    float& ovalue
        = (*block->c_val_tbl_)[((oseg->getOID() - 1) * cornerCnt) + 1 + corner];
    value += ovalue;
  }

  debugPrint(getImpl()->getLogger(),
             utl::ODB,
             "DB_EDIT",
             2,
             "EDIT: dbCapNode {}, other dbCapNode {}, addCapnCapacitance",
             seg->getId(),
             oseg->getId());

  if (block->journal_) {
    block->journal_->beginAction(dbJournal::kUpdateField);
    block->journal_->pushParam(dbCapNodeObj);
    block->journal_->pushParam(seg->getId());
    block->journal_->pushParam(_dbCapNode::kAddCapnCapacitance);
    block->journal_->pushParam(oseg->getId());
    block->journal_->endAction();
  }
}

void dbCapNode::setCapacitance(double cap, int corner)
{
  _dbCapNode* seg = (_dbCapNode*) this;
  _dbBlock* block = (_dbBlock*) seg->getOwner();
  uint32_t cornerCnt = block->corners_per_block_;
  assert((corner >= 0) && ((uint32_t) corner < cornerCnt));
  float& value
      = (*block->c_val_tbl_)[((seg->getOID() - 1) * cornerCnt) + 1 + corner];
  float prev_value = value;
  value = (float) cap;

  debugPrint(getImpl()->getLogger(),
             utl::ODB,
             "DB_EDIT",
             2,
             "EDIT: setCapacitance, corner {}, seg {}, prev: {}, new: {}",
             corner,
             seg->getId(),
             prev_value,
             value);

  if (block->journal_) {
    block->journal_->beginAction(dbJournal::kUpdateField);
    block->journal_->pushParam(dbCapNodeObj);
    block->journal_->pushParam(seg->getId());
    block->journal_->pushParam(_dbCapNode::kCapacitance);
    block->journal_->pushParam(prev_value);
    block->journal_->pushParam(value);
    block->journal_->pushParam(corner);
    block->journal_->endAction();
  }
}

void dbCapNode::addCapacitance(double cap, int corner)
{
  _dbCapNode* seg = (_dbCapNode*) this;
  _dbBlock* block = (_dbBlock*) seg->getOwner();
  uint32_t cornerCnt = block->corners_per_block_;
  assert((corner >= 0) && ((uint32_t) corner < cornerCnt));
  float& value
      = (*block->c_val_tbl_)[((seg->getOID() - 1) * cornerCnt) + 1 + corner];
  float prev_value = value;
  value += (float) cap;

  debugPrint(getImpl()->getLogger(),
             utl::ODB,
             "DB_EDIT",
             2,
             "EDIT: AddCapacitance, corner {}, seg {}, prev: {}, new: {}",
             corner,
             seg->getId(),
             prev_value,
             value);

  if (block->journal_) {
    block->journal_->beginAction(dbJournal::kUpdateField);
    block->journal_->pushParam(dbCapNodeObj);
    block->journal_->pushParam(seg->getId());
    block->journal_->pushParam(_dbCapNode::kCapacitance);
    block->journal_->pushParam(prev_value);
    block->journal_->pushParam(value);
    block->journal_->pushParam(corner);
    block->journal_->endAction();
  }
}

bool dbCapNode::isSelect()
{
  _dbCapNode* seg = (_dbCapNode*) this;
  return seg->flags_.select > 0 ? true : false;
}
bool dbCapNode::isForeign()
{
  _dbCapNode* seg = (_dbCapNode*) this;
  return seg->flags_.foreign > 0 ? true : false;
}
bool dbCapNode::isInternal()
{
  _dbCapNode* seg = (_dbCapNode*) this;
  return seg->flags_.internal > 0 ? true : false;
}
bool dbCapNode::isTreeNode()
{
  _dbCapNode* seg = (_dbCapNode*) this;
  uint32_t flags = seg->flags_.branch + seg->flags_.iterm + seg->flags_.bterm;
  return flags > 0 ? true : false;
}
bool dbCapNode::isBranch()
{
  _dbCapNode* seg = (_dbCapNode*) this;
  return seg->flags_.branch > 0 ? true : false;
}
bool dbCapNode::isDangling()
{
  return getChildrenCnt() <= 1 && !isTreeNode();
}

dbITerm* dbCapNode::getITerm(dbBlock* mblock)
{
  _dbCapNode* seg = (_dbCapNode*) this;
  dbBlock* block = mblock ? mblock : (dbBlock*) seg->getOwner();
  if (!seg->flags_.iterm) {
    return nullptr;
  }
  return dbITerm::getITerm(block, seg->node_num_);
}
dbBTerm* dbCapNode::getBTerm(dbBlock* mblock)
{
  _dbCapNode* seg = (_dbCapNode*) this;
  dbBlock* block = mblock ? mblock : (dbBlock*) seg->getOwner();
  if (!seg->flags_.bterm) {
    return nullptr;
  }
  return dbBTerm::getBTerm(block, seg->node_num_);
}
bool dbCapNode::isSourceTerm(dbBlock* mblock)
{
  _dbCapNode* seg = (_dbCapNode*) this;
  dbBlock* block = mblock ? mblock : (dbBlock*) seg->getOwner();
  dbIoType iotype;
  if (seg->flags_.iterm) {
    dbITerm* iterm = dbITerm::getITerm(block, seg->node_num_);
    iotype = iterm->getIoType();
    if (iterm->getIoType() == dbIoType::OUTPUT) {
      return true;
    }
    return false;
  }
  if (seg->flags_.bterm) {
    dbBTerm* bterm = dbBTerm::getBTerm(block, seg->node_num_);
    iotype = bterm->getIoType();
    if (bterm->getIoType() == dbIoType::INPUT) {
      return true;
    }
    return false;
  }
  return false;
}
bool dbCapNode::isInoutTerm(dbBlock* mblock)
{
  _dbCapNode* seg = (_dbCapNode*) this;
  dbBlock* block = mblock ? mblock : (dbBlock*) seg->getOwner();
  if (seg->flags_.iterm) {
    dbITerm* iterm = dbITerm::getITerm(block, seg->node_num_);
    if (iterm->getIoType() == dbIoType::INOUT) {
      return true;
    }
    return false;
  }
  if (seg->flags_.bterm) {
    dbBTerm* bterm = dbBTerm::getBTerm(block, seg->node_num_);
    if (bterm->getIoType() == dbIoType::INOUT) {
      return true;
    }
    return false;
  }
  return false;
}
bool dbCapNode::isITerm()
{
  _dbCapNode* seg = (_dbCapNode*) this;
  return seg->flags_.iterm > 0 ? true : false;
}
bool dbCapNode::isName()
{
  _dbCapNode* seg = (_dbCapNode*) this;
  return seg->flags_.name > 0 ? true : false;
}
bool dbCapNode::isBTerm()
{
  _dbCapNode* seg = (_dbCapNode*) this;
  return seg->flags_.bterm > 0 ? true : false;
}

void dbCapNode::resetBTermFlag()
{
  _dbCapNode* seg = (_dbCapNode*) this;
  _dbBlock* block = (_dbBlock*) seg->getOwner();
  uint32_t prev_flags = flagsToUInt(seg);
  seg->flags_.bterm = 0;

  debugPrint(getImpl()->getLogger(),
             utl::ODB,
             "DB_EDIT",
             2,
             "EDIT: resetBTermFlag, id: {}",
             getId());

  if (block->journal_) {
    block->journal_->updateField(
        this, _dbCapNode::kFlags, prev_flags, flagsToUInt(seg));
  }
}
void dbCapNode::resetITermFlag()
{
  _dbCapNode* seg = (_dbCapNode*) this;
  _dbBlock* block = (_dbBlock*) seg->getOwner();
  uint32_t prev_flags = flagsToUInt(seg);
  seg->flags_.iterm = 0;

  debugPrint(getImpl()->getLogger(),
             utl::ODB,
             "DB_EDIT",
             2,
             "EDIT: resetITermFlag, id: {}",
             getId());

  if (block->journal_) {
    block->journal_->updateField(
        this, _dbCapNode::kFlags, prev_flags, flagsToUInt(seg));
  }
}
void dbCapNode::resetNameFlag()
{
  _dbCapNode* seg = (_dbCapNode*) this;
  _dbBlock* block = (_dbBlock*) seg->getOwner();
  uint32_t prev_flags = flagsToUInt(seg);
  seg->flags_.name = 0;

  debugPrint(getImpl()->getLogger(),
             utl::ODB,
             "DB_EDIT",
             2,
             "EDIT: resetInternalFlag, id: {}",
             getId());

  if (block->journal_) {
    block->journal_->updateField(
        this, _dbCapNode::kFlags, prev_flags, flagsToUInt(seg));
  }
}
void dbCapNode::resetInternalFlag()
{
  _dbCapNode* seg = (_dbCapNode*) this;
  _dbBlock* block = (_dbBlock*) seg->getOwner();
  uint32_t prev_flags = flagsToUInt(seg);
  seg->flags_.internal = 0;

  debugPrint(getImpl()->getLogger(),
             utl::ODB,
             "DB_EDIT",
             2,
             "EDIT: resetInternalFlag, id: {}",
             getId());

  if (block->journal_) {
    block->journal_->updateField(
        this, _dbCapNode::kFlags, prev_flags, flagsToUInt(seg));
  }
}
void dbCapNode::resetBranchFlag()
{
  _dbCapNode* seg = (_dbCapNode*) this;
  _dbBlock* block = (_dbBlock*) seg->getOwner();
  uint32_t prev_flags = flagsToUInt(seg);
  seg->flags_.branch = 0;

  debugPrint(getImpl()->getLogger(),
             utl::ODB,
             "DB_EDIT",
             2,
             "EDIT: resetBranchFlag, id: {}",
             getId());

  if (block->journal_) {
    block->journal_->updateField(
        this, _dbCapNode::kFlags, prev_flags, flagsToUInt(seg));
  }
}
void dbCapNode::resetForeignFlag()
{
  _dbCapNode* seg = (_dbCapNode*) this;
  _dbBlock* block = (_dbBlock*) seg->getOwner();
  uint32_t prev_flags = flagsToUInt(seg);
  seg->flags_.foreign = 0;

  debugPrint(getImpl()->getLogger(),
             utl::ODB,
             "DB_EDIT",
             2,
             "EDIT: resetForeignFlag, id: {}",
             getId());

  if (block->journal_) {
    block->journal_->updateField(
        this, _dbCapNode::kFlags, prev_flags, flagsToUInt(seg));
  }
}
void dbCapNode::setBTermFlag()
{
  _dbCapNode* seg = (_dbCapNode*) this;
  _dbBlock* block = (_dbBlock*) seg->getOwner();
  uint32_t prev_flags = flagsToUInt(seg);
  seg->flags_.bterm = 1;

  debugPrint(getImpl()->getLogger(),
             utl::ODB,
             "DB_EDIT",
             2,
             "EDIT: setBTermFlag, id: {}",
             getId());

  if (block->journal_) {
    block->journal_->updateField(
        this, _dbCapNode::kFlags, prev_flags, flagsToUInt(seg));
  }
}
void dbCapNode::setITermFlag()
{
  _dbCapNode* seg = (_dbCapNode*) this;
  _dbBlock* block = (_dbBlock*) seg->getOwner();
  uint32_t prev_flags = flagsToUInt(seg);
  seg->flags_.iterm = 1;

  debugPrint(getImpl()->getLogger(),
             utl::ODB,
             "DB_EDIT",
             2,
             "EDIT: setITermFlag, id: {}",
             getId());

  if (block->journal_) {
    block->journal_->updateField(
        this, _dbCapNode::kFlags, prev_flags, flagsToUInt(seg));
  }
}
uint32_t dbCapNode::incrChildrenCnt()
{
  _dbCapNode* seg = (_dbCapNode*) this;
  _dbBlock* block = (_dbBlock*) seg->getOwner();
  uint32_t prev_flags = flagsToUInt(seg);
  seg->flags_.childrenCnt++;

  debugPrint(getImpl()->getLogger(),
             utl::ODB,
             "DB_EDIT",
             2,
             "EDIT: incrChildrenCnt, id: {}",
             getId());

  if (block->journal_) {
    block->journal_->updateField(
        this, _dbCapNode::kFlags, prev_flags, flagsToUInt(seg));
  }
  return seg->flags_.childrenCnt;
}
uint32_t dbCapNode::getChildrenCnt()
{
  _dbCapNode* seg = (_dbCapNode*) this;
  return seg->flags_.childrenCnt;
}
void dbCapNode::setChildrenCnt(uint32_t cnt)
{
  _dbCapNode* seg = (_dbCapNode*) this;
  seg->flags_.childrenCnt = cnt;
}
void dbCapNode::setBranchFlag()
{
  _dbCapNode* seg = (_dbCapNode*) this;
  _dbBlock* block = (_dbBlock*) seg->getOwner();
  uint32_t prev_flags = flagsToUInt(seg);
  seg->flags_.branch = 1;

  debugPrint(getImpl()->getLogger(),
             utl::ODB,
             "DB_EDIT",
             2,
             "EDIT: setBranchFlag, id: {}",
             getId());

  if (block->journal_) {
    block->journal_->updateField(
        this, _dbCapNode::kFlags, prev_flags, flagsToUInt(seg));
  }
}
void dbCapNode::setNameFlag()
{
  _dbCapNode* seg = (_dbCapNode*) this;
  _dbBlock* block = (_dbBlock*) seg->getOwner();
  uint32_t prev_flags = flagsToUInt(seg);
  seg->flags_.name = 1;

  debugPrint(getImpl()->getLogger(),
             utl::ODB,
             "DB_EDIT",
             2,
             "EDIT: setNameFlag, id: {}",
             getId());

  if (block->journal_) {
    block->journal_->updateField(
        this, _dbCapNode::kFlags, prev_flags, flagsToUInt(seg));
  }
}
void dbCapNode::setInternalFlag()
{
  _dbCapNode* seg = (_dbCapNode*) this;
  _dbBlock* block = (_dbBlock*) seg->getOwner();
  uint32_t prev_flags = flagsToUInt(seg);
  seg->flags_.internal = 1;

  debugPrint(getImpl()->getLogger(),
             utl::ODB,
             "DB_EDIT",
             2,
             "EDIT: setInternalFlag, id: {}",
             getId());

  if (block->journal_) {
    block->journal_->updateField(
        this, _dbCapNode::kFlags, prev_flags, flagsToUInt(seg));
  }
}
void dbCapNode::setForeignFlag()
{
  _dbCapNode* seg = (_dbCapNode*) this;
  _dbBlock* block = (_dbBlock*) seg->getOwner();
  uint32_t prev_flags = flagsToUInt(seg);
  seg->flags_.foreign = 1;

  debugPrint(getImpl()->getLogger(),
             utl::ODB,
             "DB_EDIT",
             2,
             "EDIT: setForeignFlag, id: {}",
             getId());

  if (block->journal_) {
    block->journal_->updateField(
        this, _dbCapNode::kFlags, prev_flags, flagsToUInt(seg));
  }
}
void dbCapNode::setSelect(bool val)
{
  _dbCapNode* seg = (_dbCapNode*) this;
  //_dbBlock * block = (_dbBlock *) getOwner();
  // uint32_t prev_flags = flagsToUInt(seg);
  seg->flags_.select = val ? 1 : 0;
}
void dbCapNode::setNode(uint32_t node)
{
  _dbCapNode* seg = (_dbCapNode*) this;
  _dbBlock* block = (_dbBlock*) seg->getOwner();
  uint32_t prev_node = seg->node_num_;
  seg->node_num_ = node;

  debugPrint(getImpl()->getLogger(),
             utl::ODB,
             "DB_EDIT",
             2,
             "EDIT: setNode, id: {}, prev_node: {}, new node: {}",
             getId(),
             prev_node,
             node);

  if (block->journal_) {
    block->journal_->updateField(this, _dbCapNode::kNodeNum, prev_node, node);
  }
}
uint32_t dbCapNode::getNode()
{
  _dbCapNode* seg = (_dbCapNode*) this;
  return seg->node_num_;
}

uint32_t dbCapNode::getShapeId()
{
  _dbCapNode* seg = (_dbCapNode*) this;
  dbBlock* block = (dbBlock*) seg->getOwner();
  if (seg->flags_.internal > 0) {
    return seg->node_num_;
  }
  if (seg->flags_.iterm > 0) {
    dbITerm* iterm = dbITerm::getITerm(block, seg->node_num_);
    if (!iterm->getNet() || !iterm->getNet()->getWire()) {
      return 0;
    }
    return iterm->getNet()->getWire()->getTermJid(iterm->getId());
  }
  dbBTerm* bterm = dbBTerm::getBTerm(block, seg->node_num_);
  if (!bterm->getNet() || !bterm->getNet()->getWire()) {
    return 0;
  }
  return bterm->getNet()->getWire()->getTermJid(-bterm->getId());
}

void dbCapNode::setSortIndex(uint32_t idx)
{
  _dbCapNode* seg = (_dbCapNode*) this;
  //_dbBlock * block = (_dbBlock *) getOwner();
  seg->flags_.sort_index = idx;
}

uint32_t dbCapNode::getSortIndex()
{
  _dbCapNode* seg = (_dbCapNode*) this;
  //_dbBlock * block = (_dbBlock *) getOwner();
  return seg->flags_.sort_index;
}

// void
// dbCapNode::setCoordY(int y)
//{
//    _dbCapNode * seg = (_dbCapNode *) this;
//    seg->_ycoord = y;
//}
//
// void
// dbCapNode::getCoordY(int & y)
//{
//    _dbCapNode * seg = (_dbCapNode *) this;
//    y = seg->_ycoord;
//}

bool dbCapNode::getTermCoords(int& x, int& y, dbBlock* mblock)
{
  _dbCapNode* seg = (_dbCapNode*) this;
  dbBlock* block = mblock ? mblock : (dbBlock*) seg->getOwner();
  if (seg->flags_.iterm > 0) {
    dbITerm* iterm = dbITerm::getITerm(block, seg->node_num_);
    return (iterm->getAvgXY(&x, &y));
  }
  if (seg->flags_.bterm > 0) {
    dbBTerm* bterm = dbBTerm::getBTerm(block, seg->node_num_);
    return (bterm->getFirstPinLocation(x, y));
  }
  return false;
}

dbSet<dbCCSeg> dbCapNode::getCCSegs()
{
  _dbCapNode* seg = (_dbCapNode*) this;
  _dbBlock* block = (_dbBlock*) seg->getOwner();
  return dbSet<dbCCSeg>(seg, block->cc_seg_itr_);
}

void dbCapNode::setNext(uint32_t nextid)
{
  _dbCapNode* seg = (_dbCapNode*) this;
  _dbBlock* block = (_dbBlock*) seg->getOwner();
  uint32_t prev_next = seg->next_;
  seg->next_ = nextid;

  debugPrint(getImpl()->getLogger(),
             utl::ODB,
             "DB_EDIT",
             2,
             "EDIT: capNode setNext, id: {}, prev_next: {}, new next: {}",
             getId(),
             prev_next,
             nextid);

  if (block->journal_) {
    block->journal_->updateField(this, _dbCapNode::kSetNext, prev_next, nextid);
  }
}
void dbCapNode::setNet(uint32_t netid)
{
  _dbCapNode* seg = (_dbCapNode*) this;
  _dbBlock* block = (_dbBlock*) seg->getOwner();
  uint32_t prev_net = seg->net_;
  seg->net_ = netid;

  debugPrint(getImpl()->getLogger(),
             utl::ODB,
             "DB_EDIT",
             2,
             "EDIT: setNet, id: {}, prev_net: {}, new net: {}",
             getId(),
             prev_net,
             netid);

  if (block->journal_) {
    block->journal_->updateField(this, _dbCapNode::kSetNet, prev_net, netid);
  }
}
/*
 * This routine exposes the schema... Use an iterator!

dbCapNode *
dbCapNode::getNext(dbBlock *block_)
{
    _dbCapNode * seg = (_dbCapNode *) this;
    if (seg->_next==0)
        return nullptr;

    _dbBlock * block = (_dbBlock *) block_;
    return (dbCapNode *) block->_cap_node_tbl->getPtr( seg->_next );
}
*/

dbCapNode* dbCapNode::create(dbNet* net_, uint32_t node, bool foreign)
{
  _dbNet* net = (_dbNet*) net_;
  _dbBlock* block = (_dbBlock*) net->getOwner();
  uint32_t cornerCnt = block->corners_per_block_;
  _dbCapNode* seg = block->cap_node_tbl_->create();

  debugPrint(block->getImpl()->getLogger(),
             utl::ODB,
             "DB_EDIT",
             2,
             "EDIT: dbCapNode::create, net id: {}, node: {}, foreign: {}",
             net->getId(),
             node,
             foreign);

  if (block->journal_) {
    block->journal_->beginAction(dbJournal::kCreateObject);
    block->journal_->pushParam(dbCapNodeObj);
    block->journal_->pushParam(net->getId());
    block->journal_->pushParam(node);
    block->journal_->pushParam(foreign);
    block->journal_->endAction();
  }

  seg->node_num_ = node;
  // seg->flags_._cnt = block->_num_corners;
  seg->flags_.select = 0;
  seg->flags_.sort_index = 0;

  if (foreign) {
    seg->flags_.foreign = 1;
    if (block->max_cap_node_id_ >= seg->getOID()) {
      for (uint32_t ii = 0; ii < cornerCnt; ii++) {
        (*block->c_val_tbl_)[((seg->getOID() - 1) * cornerCnt) + 1 + ii] = 0.0;
      }
    } else {
      block->max_cap_node_id_ = seg->getOID();
      [[maybe_unused]] uint32_t capIdx
          = block->c_val_tbl_->getIdx(cornerCnt, (float) 0.0);
      assert((seg->getOID() - 1) * cornerCnt + 1 == capIdx);
    }
  }
  seg->net_ = net->getOID();
  seg->next_ = net->cap_nodes_;
  net->cap_nodes_ = seg->getOID();

  return (dbCapNode*) seg;
}
void dbCapNode::addToNet()
{
  _dbCapNode* seg = (_dbCapNode*) this;
  _dbBlock* block = (_dbBlock*) seg->getOwner();
  _dbNet* net = (_dbNet*) dbNet::getNet((dbBlock*) block, seg->net_);

  seg->next_ = net->cap_nodes_;
  net->cap_nodes_ = seg->getOID();
}

void dbCapNode::destroy(dbCapNode* seg_, bool destroyCC)
{
  _dbCapNode* seg = (_dbCapNode*) seg_;
  _dbBlock* block = (_dbBlock*) seg->getOwner();
  _dbNet* net = (_dbNet*) seg_->getNet();

  for (uint32_t sid = seg->cc_segs_; destroyCC && sid; sid = seg->cc_segs_) {
    _dbCCSeg* s = block->cc_seg_tbl_->getPtr(sid);
    dbCCSeg::destroy((dbCCSeg*) s);
  }

  // unlink the cap-node from the net cap-node list
  dbId<_dbCapNode> c = net->cap_nodes_;
  _dbCapNode* p = nullptr;

  while (c != 0) {
    _dbCapNode* s = block->cap_node_tbl_->getPtr(c);

    if (s == seg) {
      if (p == nullptr) {
        net->cap_nodes_ = s->next_;
      } else {
        p->next_ = s->next_;
      }
      break;
    }
    p = s;
    c = s->next_;
  }

  debugPrint(net->getImpl()->getLogger(),
             utl::ODB,
             "DB_EDIT",
             2,
             "EDIT: dbCapNode::destroy, seg id: {}, net id: {}",
             seg->getId(),
             net->getId());

  if (block->journal_) {
    block->journal_->beginAction(dbJournal::kDeleteObject);
    block->journal_->pushParam(dbCapNodeObj);
    block->journal_->pushParam(seg->getId());
    block->journal_->endAction();
  }

  dbProperty::destroyProperties(seg);
  block->cap_node_tbl_->destroy(seg);
}

dbSet<dbCapNode>::iterator dbCapNode::destroy(dbSet<dbCapNode>::iterator& itr)
{
  dbCapNode* bt = *itr;
  dbSet<dbCapNode>::iterator next = ++itr;
  destroy(bt);
  return next;
}
dbNet* dbCapNode::getNet()
{
  _dbCapNode* seg = (_dbCapNode*) this;
  _dbBlock* block = (_dbBlock*) seg->getOwner();
  return dbNet::getNet((dbBlock*) block, seg->net_);
}

void dbCapNode::printCC()
{
  _dbCapNode* node = (_dbCapNode*) this;
  dbBlock* block = (dbBlock*) node->getOwner();
  uint32_t ccn = node->cc_segs_;
  if (ccn == 0) {
    return;
  }
  dbCCSeg* ccs = dbCCSeg::getCCSeg(block, ccn);
  getImpl()->getLogger()->info(utl::ODB, 25, "  capn {}", getId());
  ccs->printCapnCC(getId());
}

bool dbCapNode::checkCC()
{
  _dbCapNode* node = (_dbCapNode*) this;
  dbBlock* block = (dbBlock*) node->getOwner();
  uint32_t ccn = node->cc_segs_;
  if (ccn == 0) {
    return true;
  }
  dbCCSeg* ccs = dbCCSeg::getCCSeg(block, ccn);
  uint32_t rc = ccs->checkCapnCC(getId());
  return rc;
}

dbCapNode* dbCapNode::getCapNode(dbBlock* block_, uint32_t dbid_)
{
  _dbBlock* block = (_dbBlock*) block_;
  return (dbCapNode*) block->cap_node_tbl_->getPtr(dbid_);
}

void _dbCapNode::collectMemInfo(MemInfo& info)
{
  info.cnt++;
  info.size += sizeof(*this);
}

}  // namespace odb
