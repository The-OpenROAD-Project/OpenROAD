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

#include "dbCapNode.h"

#include "db.h"
#include "dbBlock.h"
#include "dbCCSeg.h"
#include "dbCCSegItr.h"
#include "dbCommon.h"
#include "dbDatabase.h"
#include "dbJournal.h"
#include "dbNet.h"
#include "dbTable.h"
#include "dbTable.hpp"
#include "utility/Logger.h"

namespace odb {

double getExtCCmult(dbNet* aggressor);

template class dbTable<_dbCapNode>;

bool _dbCapNode::operator==(const _dbCapNode& rhs) const
{
  if (_flags._name != rhs._flags._name)
    return false;

  if (_flags._internal != rhs._flags._internal)
    return false;

  if (_flags._iterm != rhs._flags._iterm)
    return false;

  if (_flags._bterm != rhs._flags._bterm)
    return false;

  if (_flags._branch != rhs._flags._branch)
    return false;

  if (_flags._foreign != rhs._flags._foreign)
    return false;

  if (_flags._childrenCnt != rhs._flags._childrenCnt)
    return false;

  if (_flags._select != rhs._flags._select)
    return false;

  if (_node_num != rhs._node_num)
    return false;

  if (_net != rhs._net)
    return false;

  if (_next != rhs._next)
    return false;

  if (_cc_segs != rhs._cc_segs)
    return false;

  return true;
}

void _dbCapNode::differences(dbDiff& diff,
                             const char* field,
                             const _dbCapNode& rhs) const
{
  DIFF_BEGIN
  DIFF_FIELD(_flags._internal);
  DIFF_FIELD(_flags._iterm);
  DIFF_FIELD(_flags._bterm);
  DIFF_FIELD(_flags._branch);
  DIFF_FIELD(_flags._foreign);
  DIFF_FIELD(_flags._childrenCnt);
  DIFF_FIELD(_flags._select);

  // if (stream.getDatabase()->isSchema(ADS_DB_CAPNODE_NAME))
  DIFF_FIELD(_flags._name);

  DIFF_FIELD(_node_num);
  DIFF_FIELD(_net);
  DIFF_FIELD(_next);
  DIFF_FIELD(_cc_segs);
  DIFF_END
}

void _dbCapNode::out(dbDiff& diff, char side, const char* field) const
{
  DIFF_OUT_BEGIN
  DIFF_OUT_FIELD(_flags._internal);
  DIFF_OUT_FIELD(_flags._iterm);
  DIFF_OUT_FIELD(_flags._bterm);
  DIFF_OUT_FIELD(_flags._branch);
  DIFF_OUT_FIELD(_flags._foreign);
  DIFF_OUT_FIELD(_flags._childrenCnt);
  DIFF_OUT_FIELD(_flags._select);

  // if (stream.getDatabase()->isSchema(ADS_DB_CAPNODE_NAME))
  DIFF_OUT_FIELD(_flags._name);

  DIFF_OUT_FIELD(_node_num);
  DIFF_OUT_FIELD(_net);
  DIFF_OUT_FIELD(_next);
  DIFF_OUT_FIELD(_cc_segs);
  DIFF_END
}

////////////////////////////////////////////////////////////////////
//
// dbCapNode - Methods
//
////////////////////////////////////////////////////////////////////

bool dbCapNode::needAdjustCC(double ccThreshHold)
{
  dbSet<dbCCSeg> ccSegs = getCCSegs();
  dbSet<dbCCSeg>::iterator ccitr;
  uint cornerCnt = ((dbBlock*) getImpl()->getOwner())->getCornerCount();
  uint corner;
  uint cid;
  for (ccitr = ccSegs.begin(); ccitr != ccSegs.end(); ++ccitr) {
    dbCCSeg* cc = *ccitr;
    for (corner = 0; corner < cornerCnt; corner++) {
      if (cc->getTheOtherCapn(this, cid)->getNet()->getCcAdjustFactor() > 0)
        continue;
      if (cc->getCapacitance(corner) >= ccThreshHold)
        return true;
    }
  }
  return false;
}

bool dbCapNode::groundCC(float gndFactor)
{
  bool grounded = false;
  uint vicNetId = getNet()->getId();
  uint agrNetId;
  dbCapNode* agrNode;
  double deltaC;
  uint cid;
  _dbBlock* block = (_dbBlock*) getImpl()->getOwner();
  uint cornerCnt = block->_corners_per_block;
  dbSet<dbCCSeg> ccSegs = getCCSegs();
  dbSet<dbCCSeg>::iterator ccitr;
  for (ccitr = ccSegs.begin(); ccitr != ccSegs.end(); ++ccitr) {
    dbCCSeg* cc = *ccitr;
    agrNode = cc->getTheOtherCapn(this, cid);
    agrNetId = agrNode->getNet()->getId();
    if (agrNetId < vicNetId)
      continue;  //  avoid duplicate grounding
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
    for (uint ii = 0; ii < cornerCnt; ii++) {
      deltaC = cc->getCapacitance(ii) * gndFactor;
      addCapacitance(deltaC, ii);
      agrNode->addCapacitance(deltaC, ii);
    }
  }
  return grounded;
}

void dbCapNode::adjustCC(uint adjOrder,
                         float adjFactor,
                         std::vector<dbCCSeg*>& adjustedCC,
                         std::vector<dbNet*>& halonets)
{
  dbSet<dbCCSeg> ccSegs = getCCSegs();
  dbSet<dbCCSeg>::iterator ccitr;
  dbNet* agrNet;
  uint cid;
  for (ccitr = ccSegs.begin(); ccitr != ccSegs.end(); ++ccitr) {
    dbCCSeg* cc = *ccitr;
    if (cc->isMarked())
      continue;
    agrNet = cc->getTheOtherCapn(this, cid)->getNet();
    if (agrNet->getCcAdjustFactor() > 0
        && agrNet->getCcAdjustOrder() < adjOrder)
      continue;
    adjustedCC.push_back(cc);
    cc->setMark(true);
    cc->adjustCapacitance(adjFactor);
    if (!agrNet->isMark_1ed()) {
      agrNet->setMark_1(true);
      halonets.push_back(agrNet);
    }
  }
}

void dbCapNode::adjustCapacitance(float factor, uint corner)
{
  _dbCapNode* seg = (_dbCapNode*) this;
  _dbBlock* block = (_dbBlock*) seg->getOwner();
  uint cornerCnt = block->_corners_per_block;

  ZASSERT(seg->_flags._foreign > 0);
  ZASSERT(corner < cornerCnt);
  float& value
      = (*block->_c_val_tbl)[(seg->getOID() - 1) * cornerCnt + 1 + corner];
  float prev_value = value;
  value *= factor;

  if (block->_journal) {
    debugPrint(getImpl()->getLogger(),
               utl::ODB,
               "DB_ECO",
               1,
               "ECO: dbCapNode {}, adjustCapacitance {}, corner {}",
               seg->getId(),
               factor,
               corner);
    block->_journal->beginAction(dbJournal::UPDATE_FIELD);
    block->_journal->pushParam(dbCapNodeObj);
    block->_journal->pushParam(seg->getId());
    block->_journal->pushParam(_dbCapNode::CAPACITANCE);
    block->_journal->pushParam(prev_value);
    block->_journal->pushParam(value);
    block->_journal->pushParam(0);
    block->_journal->endAction();
  }
}

void dbCapNode::adjustCapacitance(float factor)
{
  _dbBlock* block = (_dbBlock*) getImpl()->getOwner();
  uint cornerCnt = block->_corners_per_block;
  uint corner;
  for (corner = 0; corner < cornerCnt; corner++)
    adjustCapacitance(factor, corner);
}

double dbCapNode::getCapacitance(uint corner)
{
  _dbCapNode* seg = (_dbCapNode*) this;
  _dbBlock* block = (_dbBlock*) seg->getOwner();
  uint cornerCnt = block->_corners_per_block;

  if (seg->_flags._foreign > 0) {
    ZASSERT(corner < cornerCnt);
    return (*block->_c_val_tbl)[(seg->getOID() - 1) * cornerCnt + 1 + corner];
  } else {
    return 0.0;
  }
}

void dbCapNode::getGndCap(double* gndcap, double* totalcap)
{
  _dbCapNode* seg = (_dbCapNode*) this;
  if (seg->_flags._foreign == 0)
    return;
  _dbBlock* block = (_dbBlock*) seg->getOwner();
  uint cornerCnt = block->_corners_per_block;
  double gcap;
  for (uint ii = 0; ii < cornerCnt; ii++) {
    gcap = (*block->_c_val_tbl)[(seg->getOID() - 1) * cornerCnt + 1 + ii];
    if (gndcap)
      gndcap[ii] = gcap;
    if (totalcap)
      totalcap[ii] = gcap;
  }
}

void dbCapNode::addGndCap(double* gndcap, double* totalcap)
{
  _dbCapNode* seg = (_dbCapNode*) this;
  if (seg->_flags._foreign == 0)
    return;
  _dbBlock* block = (_dbBlock*) seg->getOwner();
  uint cornerCnt = block->_corners_per_block;
  double gcap;
  for (uint ii = 0; ii < cornerCnt; ii++) {
    gcap = (*block->_c_val_tbl)[(seg->getOID() - 1) * cornerCnt + 1 + ii];
    if (gndcap)
      gndcap[ii] += gcap;
    if (totalcap)
      totalcap[ii] += gcap;
  }
}

void dbCapNode::accAllCcCap(double* totalcap, double MillerMult)
{
  if (totalcap == NULL || MillerMult == 0)
    return;
  dbSet<dbCCSeg> ccSegs = getCCSegs();
  dbSet<dbCCSeg>::iterator ccitr;
  for (ccitr = ccSegs.begin(); ccitr != ccSegs.end(); ++ccitr) {
    double ccmult = MillerMult;
    dbCCSeg* cc = *ccitr;
#ifdef TMG_SI
    if (MillerMult < 0)
      ccmult = getExtCCmult(cc->getTheOtherCapn(this, cid)->getNet());
#endif
    cc->accAllCcCap(totalcap, ccmult);
  }
}

void dbCapNode::getGndTotalCap(double* gndcap,
                               double* totalcap,
                               double MillerMult)
{
  getGndCap(gndcap, totalcap);
  accAllCcCap(totalcap, MillerMult);
}

void dbCapNode::addGndTotalCap(double* gndcap,
                               double* totalcap,
                               double MillerMult)
{
  addGndCap(gndcap, totalcap);
  accAllCcCap(totalcap, MillerMult);
}

void dbCapNode::getCapTable(double* cap)
{
  _dbCapNode* seg = (_dbCapNode*) this;
  _dbBlock* block = (_dbBlock*) seg->getOwner();
  uint cornerCnt = block->_corners_per_block;

  for (uint ii = 0; ii < cornerCnt; ii++)
    cap[ii] = (*block->_c_val_tbl)[(seg->getOID() - 1) * cornerCnt + 1 + ii];
}

void dbCapNode::addCapnCapacitance(dbCapNode* other)
{
  _dbCapNode* seg = (_dbCapNode*) this;
  _dbCapNode* oseg = (_dbCapNode*) other;
  _dbBlock* block = (_dbBlock*) seg->getOwner();
  uint cornerCnt = block->_corners_per_block;
  for (uint corner = 0; corner < cornerCnt; corner++) {
    float& value
        = (*block->_c_val_tbl)[(seg->getOID() - 1) * cornerCnt + 1 + corner];
    float& ovalue
        = (*block->_c_val_tbl)[(oseg->getOID() - 1) * cornerCnt + 1 + corner];
    value += ovalue;
  }

  if (block->_journal) {
    debugPrint(getImpl()->getLogger(),
               utl::ODB,
               "DB_ECO",
               1,
               "ECO: dbCapNode {}, other dbCapNode {}, addCapnCapacitance",
               seg->getId(),
               oseg->getId());
    block->_journal->beginAction(dbJournal::UPDATE_FIELD);
    block->_journal->pushParam(dbCapNodeObj);
    block->_journal->pushParam(seg->getId());
    block->_journal->pushParam(_dbCapNode::ADDCAPNCAPACITANCE);
    block->_journal->pushParam(oseg->getId());
    block->_journal->endAction();
  }
}

void dbCapNode::setCapacitance(double cap, int corner)
{
  _dbCapNode* seg = (_dbCapNode*) this;
  _dbBlock* block = (_dbBlock*) seg->getOwner();
  uint cornerCnt = block->_corners_per_block;
  ZASSERT((corner >= 0) && ((uint) corner < cornerCnt));
  float& value
      = (*block->_c_val_tbl)[(seg->getOID() - 1) * cornerCnt + 1 + corner];
  float prev_value = value;
  value = (float) cap;

  if (block->_journal) {
    debugPrint(getImpl()->getLogger(),
               utl::ODB,
               "DB_ECO",
               1,
               "ECO: setCapacitance, corner {}, seg {}, prev: {}, new: {}",
               corner,
               seg->getId(),
               prev_value,
               value);
    block->_journal->beginAction(dbJournal::UPDATE_FIELD);
    block->_journal->pushParam(dbCapNodeObj);
    block->_journal->pushParam(seg->getId());
    block->_journal->pushParam(_dbCapNode::CAPACITANCE);
    block->_journal->pushParam(prev_value);
    block->_journal->pushParam(value);
    block->_journal->pushParam(corner);
    block->_journal->endAction();
  }
}

void dbCapNode::addCapacitance(double cap, int corner)
{
  _dbCapNode* seg = (_dbCapNode*) this;
  _dbBlock* block = (_dbBlock*) seg->getOwner();
  uint cornerCnt = block->_corners_per_block;
  ZASSERT((corner >= 0) && ((uint) corner < cornerCnt));
  float& value
      = (*block->_c_val_tbl)[(seg->getOID() - 1) * cornerCnt + 1 + corner];
  float prev_value = value;
  value += (float) cap;

  if (block->_journal) {
    debugPrint(getImpl()->getLogger(),
               utl::ODB,
               "DB_ECO",
               1,
               "ECO: AddCapacitance, corner {}, seg {}, prev: {}, new: {}",
               corner,
               seg->getId(),
               prev_value,
               value);
    block->_journal->beginAction(dbJournal::UPDATE_FIELD);
    block->_journal->pushParam(dbCapNodeObj);
    block->_journal->pushParam(seg->getId());
    block->_journal->pushParam(_dbCapNode::CAPACITANCE);
    block->_journal->pushParam(prev_value);
    block->_journal->pushParam(value);
    block->_journal->pushParam(corner);
    block->_journal->endAction();
  }
}

bool dbCapNode::isSelect()
{
  _dbCapNode* seg = (_dbCapNode*) this;
  return seg->_flags._select > 0 ? true : false;
}
bool dbCapNode::isForeign()
{
  _dbCapNode* seg = (_dbCapNode*) this;
  return seg->_flags._foreign > 0 ? true : false;
}
bool dbCapNode::isInternal()
{
  _dbCapNode* seg = (_dbCapNode*) this;
  return seg->_flags._internal > 0 ? true : false;
}
bool dbCapNode::isTreeNode()
{
  _dbCapNode* seg = (_dbCapNode*) this;
  uint flags = seg->_flags._branch + seg->_flags._iterm + seg->_flags._bterm;
  return flags > 0 ? true : false;
}
bool dbCapNode::isBranch()
{
  _dbCapNode* seg = (_dbCapNode*) this;
  return seg->_flags._branch > 0 ? true : false;
}
bool dbCapNode::isDangling()
{
  return getChildrenCnt() <= 1 && !isTreeNode();
}

dbITerm* dbCapNode::getITerm(dbBlock* mblock)
{
  _dbCapNode* seg = (_dbCapNode*) this;
  dbBlock* block = mblock ? mblock : (dbBlock*) seg->getOwner();
  if (!seg->_flags._iterm)
    return NULL;
  return dbITerm::getITerm(block, seg->_node_num);
}
dbBTerm* dbCapNode::getBTerm(dbBlock* mblock)
{
  _dbCapNode* seg = (_dbCapNode*) this;
  dbBlock* block = mblock ? mblock : (dbBlock*) seg->getOwner();
  if (!seg->_flags._bterm)
    return NULL;
  return dbBTerm::getBTerm(block, seg->_node_num);
}
bool dbCapNode::isSourceTerm(dbBlock* mblock)
{
  _dbCapNode* seg = (_dbCapNode*) this;
  dbBlock* block = mblock ? mblock : (dbBlock*) seg->getOwner();
  dbIoType iotype;
  if (seg->_flags._iterm) {
    dbITerm* iterm = dbITerm::getITerm(block, seg->_node_num);
    iotype = iterm->getIoType();
    if (iterm->getIoType() == dbIoType::OUTPUT)
      return true;
    else
      return false;
  } else if (seg->_flags._bterm) {
    dbBTerm* bterm = dbBTerm::getBTerm(block, seg->_node_num);
    iotype = bterm->getIoType();
    if (bterm->getIoType() == dbIoType::INPUT)
      return true;
    else
      return false;
  }
  return false;
}
bool dbCapNode::isInoutTerm(dbBlock* mblock)
{
  _dbCapNode* seg = (_dbCapNode*) this;
  dbBlock* block = mblock ? mblock : (dbBlock*) seg->getOwner();
  if (seg->_flags._iterm) {
    dbITerm* iterm = dbITerm::getITerm(block, seg->_node_num);
    if (iterm->getIoType() == dbIoType::INOUT)
      return true;
    else
      return false;
  } else if (seg->_flags._bterm) {
    dbBTerm* bterm = dbBTerm::getBTerm(block, seg->_node_num);
    if (bterm->getIoType() == dbIoType::INOUT)
      return true;
    else
      return false;
  }
  return false;
}
bool dbCapNode::isITerm()
{
  _dbCapNode* seg = (_dbCapNode*) this;
  return seg->_flags._iterm > 0 ? true : false;
}
bool dbCapNode::isName()
{
  _dbCapNode* seg = (_dbCapNode*) this;
  return seg->_flags._name > 0 ? true : false;
}
bool dbCapNode::isBTerm()
{
  _dbCapNode* seg = (_dbCapNode*) this;
  return seg->_flags._bterm > 0 ? true : false;
}

void dbCapNode::resetBTermFlag()
{
  _dbCapNode* seg = (_dbCapNode*) this;
  _dbBlock* block = (_dbBlock*) seg->getOwner();
  uint prev_flags = flagsToUInt(seg);
  seg->_flags._bterm = 0;

  if (block->_journal) {
    debugPrint(getImpl()->getLogger(),
               utl::ODB,
               "DB_ECO",
               1,
               "ECO: resetBTermFlag, id: {}",
               getId());
    block->_journal->updateField(
        this, _dbCapNode::FLAGS, prev_flags, flagsToUInt(seg));
  }
}
void dbCapNode::resetITermFlag()
{
  _dbCapNode* seg = (_dbCapNode*) this;
  _dbBlock* block = (_dbBlock*) seg->getOwner();
  uint prev_flags = flagsToUInt(seg);
  seg->_flags._iterm = 0;

  if (block->_journal) {
    debugPrint(getImpl()->getLogger(),
               utl::ODB,
               "DB_ECO",
               1,
               "ECO: resetITermFlag, id: {}",
               getId());
    block->_journal->updateField(
        this, _dbCapNode::FLAGS, prev_flags, flagsToUInt(seg));
  }
}
void dbCapNode::resetNameFlag()
{
  _dbCapNode* seg = (_dbCapNode*) this;
  _dbBlock* block = (_dbBlock*) seg->getOwner();
  uint prev_flags = flagsToUInt(seg);
  seg->_flags._name = 0;

  if (block->_journal) {
    debugPrint(getImpl()->getLogger(),
               utl::ODB,
               "DB_ECO",
               1,
               "ECO: resetInternalFlag, id: {}",
               getId());
    block->_journal->updateField(
        this, _dbCapNode::FLAGS, prev_flags, flagsToUInt(seg));
  }
}
void dbCapNode::resetInternalFlag()
{
  _dbCapNode* seg = (_dbCapNode*) this;
  _dbBlock* block = (_dbBlock*) seg->getOwner();
  uint prev_flags = flagsToUInt(seg);
  seg->_flags._internal = 0;

  if (block->_journal) {
    debugPrint(getImpl()->getLogger(),
               utl::ODB,
               "DB_ECO",
               1,
               "ECO: resetInternalFlag, id: {}",
               getId());
    block->_journal->updateField(
        this, _dbCapNode::FLAGS, prev_flags, flagsToUInt(seg));
  }
}
void dbCapNode::resetBranchFlag()
{
  _dbCapNode* seg = (_dbCapNode*) this;
  _dbBlock* block = (_dbBlock*) seg->getOwner();
  uint prev_flags = flagsToUInt(seg);
  seg->_flags._branch = 0;

  if (block->_journal) {
    debugPrint(getImpl()->getLogger(),
               utl::ODB,
               "DB_ECO",
               1,
               "ECO: resetBranchFlag, id: {}",
               getId());
    block->_journal->updateField(
        this, _dbCapNode::FLAGS, prev_flags, flagsToUInt(seg));
  }
}
void dbCapNode::resetForeignFlag()
{
  _dbCapNode* seg = (_dbCapNode*) this;
  _dbBlock* block = (_dbBlock*) seg->getOwner();
  uint prev_flags = flagsToUInt(seg);
  seg->_flags._foreign = 0;

  if (block->_journal) {
    debugPrint(getImpl()->getLogger(),
               utl::ODB,
               "DB_ECO",
               1,
               "ECO: resetForeignFlag, id: {}",
               getId());
    block->_journal->updateField(
        this, _dbCapNode::FLAGS, prev_flags, flagsToUInt(seg));
  }
}
void dbCapNode::setBTermFlag()
{
  _dbCapNode* seg = (_dbCapNode*) this;
  _dbBlock* block = (_dbBlock*) seg->getOwner();
  uint prev_flags = flagsToUInt(seg);
  seg->_flags._bterm = 1;

  if (block->_journal) {
    debugPrint(getImpl()->getLogger(),
               utl::ODB,
               "DB_ECO",
               1,
               "ECO: setBTermFlag, id: {}",
               getId());
    block->_journal->updateField(
        this, _dbCapNode::FLAGS, prev_flags, flagsToUInt(seg));
  }
}
void dbCapNode::setITermFlag()
{
  _dbCapNode* seg = (_dbCapNode*) this;
  _dbBlock* block = (_dbBlock*) seg->getOwner();
  uint prev_flags = flagsToUInt(seg);
  seg->_flags._iterm = 1;

  if (block->_journal) {
    debugPrint(getImpl()->getLogger(),
               utl::ODB,
               "DB_ECO",
               1,
               "ECO: setITermFlag, id: {}",
               getId());
    block->_journal->updateField(
        this, _dbCapNode::FLAGS, prev_flags, flagsToUInt(seg));
  }
}
uint dbCapNode::incrChildrenCnt()
{
  _dbCapNode* seg = (_dbCapNode*) this;
  _dbBlock* block = (_dbBlock*) seg->getOwner();
  uint prev_flags = flagsToUInt(seg);
  seg->_flags._childrenCnt++;

  if (block->_journal) {
    debugPrint(getImpl()->getLogger(),
               utl::ODB,
               "DB_ECO",
               1,
               "ECO: incrChildrenCnt, id: {}",
               getId());
    block->_journal->updateField(
        this, _dbCapNode::FLAGS, prev_flags, flagsToUInt(seg));
  }
  return seg->_flags._childrenCnt;
}
uint dbCapNode::getChildrenCnt()
{
  _dbCapNode* seg = (_dbCapNode*) this;
  return seg->_flags._childrenCnt;
}
void dbCapNode::setChildrenCnt(uint cnt)
{
  _dbCapNode* seg = (_dbCapNode*) this;
  seg->_flags._childrenCnt = cnt;
}
void dbCapNode::setBranchFlag()
{
  _dbCapNode* seg = (_dbCapNode*) this;
  _dbBlock* block = (_dbBlock*) seg->getOwner();
  uint prev_flags = flagsToUInt(seg);
  seg->_flags._branch = 1;

  if (block->_journal) {
    debugPrint(getImpl()->getLogger(),
               utl::ODB,
               "DB_ECO",
               1,
               "ECO: setBranchFlag, id: {}",
               getId());
    block->_journal->updateField(
        this, _dbCapNode::FLAGS, prev_flags, flagsToUInt(seg));
  }
}
void dbCapNode::setNameFlag()
{
  _dbCapNode* seg = (_dbCapNode*) this;
  _dbBlock* block = (_dbBlock*) seg->getOwner();
  uint prev_flags = flagsToUInt(seg);
  seg->_flags._name = 1;

  if (block->_journal) {
    debugPrint(getImpl()->getLogger(),
               utl::ODB,
               "DB_ECO",
               1,
               "ECO: setInternalFlag, id: {}",
               getId());
    block->_journal->updateField(
        this, _dbCapNode::FLAGS, prev_flags, flagsToUInt(seg));
  }
}
void dbCapNode::setInternalFlag()
{
  _dbCapNode* seg = (_dbCapNode*) this;
  _dbBlock* block = (_dbBlock*) seg->getOwner();
  uint prev_flags = flagsToUInt(seg);
  seg->_flags._internal = 1;

  if (block->_journal) {
    debugPrint(getImpl()->getLogger(),
               utl::ODB,
               "DB_ECO",
               1,
               "ECO: setInternalFlag, id: {}",
               getId());
    block->_journal->updateField(
        this, _dbCapNode::FLAGS, prev_flags, flagsToUInt(seg));
  }
}
void dbCapNode::setForeignFlag()
{
  _dbCapNode* seg = (_dbCapNode*) this;
  _dbBlock* block = (_dbBlock*) seg->getOwner();
  uint prev_flags = flagsToUInt(seg);
  seg->_flags._foreign = 1;

  if (block->_journal) {
    debugPrint(getImpl()->getLogger(),
               utl::ODB,
               "DB_ECO",
               1,
               "ECO: setForeignFlag, id: {}",
               getId());
    block->_journal->updateField(
        this, _dbCapNode::FLAGS, prev_flags, flagsToUInt(seg));
  }
}
void dbCapNode::setSelect(bool val)
{
  _dbCapNode* seg = (_dbCapNode*) this;
  //_dbBlock * block = (_dbBlock *) getOwner();
  // uint prev_flags = flagsToUInt(seg);
  seg->_flags._select = val ? 1 : 0;

#ifdef FULL_ECO
  if (block->_journal) {
    debugPrint(getImpl()->getLogger(),
               utl::ODB,
               "DB_ECO",
               1,
               "ECO: setSelect to {}, id: {}",
               val,
               getId());
    block->_journal->updateField(
        this, _dbCapNode::FLAGS, prev_flags, flagsToUInt(seg));
  }
#endif
}
void dbCapNode::setNode(uint node)
{
  _dbCapNode* seg = (_dbCapNode*) this;
  _dbBlock* block = (_dbBlock*) seg->getOwner();
  uint prev_node = seg->_node_num;
  seg->_node_num = node;

  if (block->_journal) {
    debugPrint(getImpl()->getLogger(),
               utl::ODB,
               "DB_ECO",
               1,
               "ECO: setNode, id: {}, prev_node: {}, new node: {}",
               getId(),
               prev_node,
               node);
    block->_journal->updateField(this, _dbCapNode::NODE_NUM, prev_node, node);
  }
}
uint dbCapNode::getNode()
{
  _dbCapNode* seg = (_dbCapNode*) this;
  return seg->_node_num;
}

uint dbCapNode::getShapeId()
{
  _dbCapNode* seg = (_dbCapNode*) this;
  dbBlock* block = (dbBlock*) seg->getOwner();
  if (seg->_flags._internal > 0)
    return seg->_node_num;
  else if (seg->_flags._iterm > 0) {
    dbITerm* iterm = dbITerm::getITerm(block, seg->_node_num);
    if (!iterm->getNet() || !iterm->getNet()->getWire())
      return 0;
    return iterm->getNet()->getWire()->getTermJid(iterm->getId());
  } else {
    dbBTerm* bterm = dbBTerm::getBTerm(block, seg->_node_num);
    if (!bterm->getNet() || !bterm->getNet()->getWire())
      return 0;
    return bterm->getNet()->getWire()->getTermJid(-bterm->getId());
  }
}

void dbCapNode::setSortIndex(uint idx)
{
  _dbCapNode* seg = (_dbCapNode*) this;
  //_dbBlock * block = (_dbBlock *) getOwner();
  seg->_flags._sort_index = idx;
}

uint dbCapNode::getSortIndex()
{
  _dbCapNode* seg = (_dbCapNode*) this;
  //_dbBlock * block = (_dbBlock *) getOwner();
  return seg->_flags._sort_index;
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
  if (seg->_flags._iterm > 0) {
    dbITerm* iterm = dbITerm::getITerm(block, seg->_node_num);
    return (iterm->getAvgXY(&x, &y));
  } else if (seg->_flags._bterm > 0) {
    dbBTerm* bterm = dbBTerm::getBTerm(block, seg->_node_num);
    return (bterm->getFirstPinLocation(x, y));
  } else
    return false;
  return true;
}

dbSet<dbCCSeg> dbCapNode::getCCSegs()
{
  _dbCapNode* seg = (_dbCapNode*) this;
  _dbBlock* block = (_dbBlock*) seg->getOwner();
  return dbSet<dbCCSeg>(seg, block->_cc_seg_itr);
}

void dbCapNode::setNext(uint nextid)
{
  _dbCapNode* seg = (_dbCapNode*) this;
  _dbBlock* block = (_dbBlock*) seg->getOwner();
  uint prev_next = seg->_next;
  seg->_next = nextid;

  if (block->_journal) {
    debugPrint(getImpl()->getLogger(),
               utl::ODB,
               "DB_ECO",
               1,
               "ECO: capNode setNext, id: {}, prev_next: {}, new next: {}",
               getId(),
               prev_next,
               nextid);
    block->_journal->updateField(this, _dbCapNode::SETNEXT, prev_next, nextid);
  }
}
void dbCapNode::setNet(uint netid)
{
  _dbCapNode* seg = (_dbCapNode*) this;
  _dbBlock* block = (_dbBlock*) seg->getOwner();
  uint prev_net = seg->_net;
  seg->_net = netid;

  if (block->_journal) {
    debugPrint(getImpl()->getLogger(),
               utl::ODB,
               "DB_ECO",
               1,
               "ECO: setNet, id: {}, prev_net: {}, new net: {}",
               getId(),
               prev_net,
               netid);
    block->_journal->updateField(this, _dbCapNode::SETNET, prev_net, netid);
  }
}
/*
 * This routine exposes the schema... Use an iterator!

dbCapNode *
dbCapNode::getNext(dbBlock *block_)
{
    _dbCapNode * seg = (_dbCapNode *) this;
    if (seg->_next==0)
        return NULL;

    _dbBlock * block = (_dbBlock *) block_;
    return (dbCapNode *) block->_cap_node_tbl->getPtr( seg->_next );
}
*/

dbCapNode* dbCapNode::create(dbNet* net_, uint node, bool foreign)
{
  _dbNet* net = (_dbNet*) net_;
  _dbBlock* block = (_dbBlock*) net->getOwner();
  uint cornerCnt = block->_corners_per_block;
  _dbCapNode* seg = block->_cap_node_tbl->create();

  if (block->_journal) {
    debugPrint(block->getImpl()->getLogger(),
               utl::ODB,
               "DB_ECO",
               1,
               "ECO: dbCapNode::create, net id: {}, node: {}, foreign: {}",
               net->getId(),
               node,
               foreign);
    block->_journal->beginAction(dbJournal::CREATE_OBJECT);
    block->_journal->pushParam(dbCapNodeObj);
    block->_journal->pushParam(net->getId());
    block->_journal->pushParam(node);
    block->_journal->pushParam(foreign);
    block->_journal->endAction();
  }

  seg->_node_num = node;
  // seg->_flags._cnt = block->_num_corners;
  seg->_flags._select = 0;
  seg->_flags._sort_index = 0;

  if (foreign) {
    seg->_flags._foreign = 1;
    if (block->_maxCapNodeId >= seg->getOID()) {
      for (uint ii = 0; ii < cornerCnt; ii++)
        (*block->_c_val_tbl)[(seg->getOID() - 1) * cornerCnt + 1 + ii] = 0.0;
    } else {
      block->_maxCapNodeId = seg->getOID();
      uint capIdx = block->_c_val_tbl->getIdx(cornerCnt, (float) 0.0);
      ZASSERT((seg->getOID() - 1) * cornerCnt + 1 == capIdx);
    }
  }
  seg->_net = net->getOID();
  seg->_next = net->_cap_nodes;
  net->_cap_nodes = seg->getOID();

  return (dbCapNode*) seg;
}
void dbCapNode::addToNet()
{
  _dbCapNode* seg = (_dbCapNode*) this;
  _dbBlock* block = (_dbBlock*) seg->getOwner();
  _dbNet* net = (_dbNet*) dbNet::getNet((dbBlock*) block, seg->_net);

  seg->_next = net->_cap_nodes;
  net->_cap_nodes = seg->getOID();
}

void dbCapNode::destroy(dbCapNode* seg_, bool destroyCC)
{
  _dbCapNode* seg = (_dbCapNode*) seg_;
  _dbBlock* block = (_dbBlock*) seg->getOwner();
  _dbNet* net = (_dbNet*) seg_->getNet();

  for (uint sid = seg->_cc_segs; destroyCC && sid; sid = seg->_cc_segs) {
    _dbCCSeg* s = block->_cc_seg_tbl->getPtr(sid);
    dbCCSeg::destroy((dbCCSeg*) s);
  }

  // unlink the cap-node from the net cap-node list
  dbId<_dbCapNode> c = net->_cap_nodes;
  _dbCapNode* p = NULL;

  while (c != 0) {
    _dbCapNode* s = block->_cap_node_tbl->getPtr(c);

    if (s == seg) {
      if (p == NULL)
        net->_cap_nodes = s->_next;
      else
        p->_next = s->_next;
      break;
    }
    p = s;
    c = s->_next;
  }

  if (block->_journal) {
    debugPrint(net->getImpl()->getLogger(),
               utl::ODB,
               "DB_ECO",
               1,
               "ECO: dbCapNode::destroy, seg id: {}, net id: {}",
               seg->getId(),
               net->getId());
    block->_journal->beginAction(dbJournal::DELETE_OBJECT);
    block->_journal->pushParam(dbCapNodeObj);
    block->_journal->pushParam(seg->getId());
    block->_journal->endAction();
  }

  dbProperty::destroyProperties(seg);
  block->_cap_node_tbl->destroy(seg);
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
  return dbNet::getNet((dbBlock*) block, seg->_net);
}

void dbCapNode::printCC()
{
  _dbCapNode* node = (_dbCapNode*) this;
  dbBlock* block = (dbBlock*) node->getOwner();
  uint ccn = node->_cc_segs;
  if (ccn == 0)
    return;
  dbCCSeg* ccs = dbCCSeg::getCCSeg(block, ccn);
  getImpl()->getLogger()->info(utl::ODB, 25, "  capn {}", getId());
  ccs->printCapnCC(getId());
}

bool dbCapNode::checkCC()
{
  _dbCapNode* node = (_dbCapNode*) this;
  dbBlock* block = (dbBlock*) node->getOwner();
  uint ccn = node->_cc_segs;
  if (ccn == 0)
    return true;
  dbCCSeg* ccs = dbCCSeg::getCCSeg(block, ccn);
  uint rc = ccs->checkCapnCC(getId());
  return rc;
}

dbCapNode* dbCapNode::getCapNode(dbBlock* block_, uint dbid_)
{
  _dbBlock* block = (_dbBlock*) block_;
  return (dbCapNode*) block->_cap_node_tbl->getPtr(dbid_);
}
}  // namespace odb
