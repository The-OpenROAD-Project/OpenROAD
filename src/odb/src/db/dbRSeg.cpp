// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "dbRSeg.h"

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <vector>

#include "dbBlock.h"
#include "dbCapNode.h"
#include "dbCommon.h"
#include "dbCore.h"
#include "dbDatabase.h"
#include "dbJournal.h"
#include "dbNet.h"
#include "dbTable.h"
#include "odb/db.h"
#include "odb/dbObject.h"
#include "odb/dbSet.h"
#include "odb/dbShape.h"
#include "utl/Logger.h"

namespace odb {

template class dbTable<_dbRSeg>;

bool _dbRSeg::operator==(const _dbRSeg& rhs) const
{
  if (flags_.path_dir != rhs.flags_.path_dir) {
    return false;
  }

  if (flags_.allocated_cap != rhs.flags_.allocated_cap) {
    return false;
  }

  if (source_ != rhs.source_) {
    return false;
  }

  if (target_ != rhs.target_) {
    return false;
  }

  if (xcoord_ != rhs.xcoord_) {
    return false;
  }

  if (ycoord_ != rhs.ycoord_) {
    return false;
  }

  if (next_ != rhs.next_) {
    return false;
  }

  return true;
}

////////////////////////////////////////////////////////////////////
//
// dbRSeg - Methods
//
////////////////////////////////////////////////////////////////////

double dbRSeg::getResistance(int corner)
{
  _dbRSeg* seg = (_dbRSeg*) this;
  _dbBlock* block = (_dbBlock*) seg->getOwner();
  uint32_t cornerCnt = block->corners_per_block_;

  assert((corner >= 0) && ((uint32_t) corner < cornerCnt));
  return (*block->r_val_tbl_)[((seg->getOID() - 1) * cornerCnt) + 1 + corner];
}

void dbRSeg::getAllRes(double* res)
{
  _dbRSeg* seg = (_dbRSeg*) this;
  _dbBlock* block = (_dbBlock*) seg->getOwner();
  uint32_t cornerCnt = block->corners_per_block_;

  for (uint32_t ii = 0; ii < cornerCnt; ii++) {
    res[ii] = (*block->r_val_tbl_)[((seg->getOID() - 1) * cornerCnt) + 1 + ii];
  }
}

void dbRSeg::addAllRes(double* res)
{
  _dbRSeg* seg = (_dbRSeg*) this;
  _dbBlock* block = (_dbBlock*) seg->getOwner();
  uint32_t cornerCnt = block->corners_per_block_;

  for (uint32_t ii = 0; ii < cornerCnt; ii++) {
    res[ii] += (*block->r_val_tbl_)[((seg->getOID() - 1) * cornerCnt) + 1 + ii];
  }
}

bool dbRSeg::updatedCap()
{
  _dbRSeg* seg = (_dbRSeg*) this;
  return (seg->flags_.update_cap == 1 ? true : false);
}
bool dbRSeg::allocatedCap()
{
  _dbRSeg* seg = (_dbRSeg*) this;
  return (seg->flags_.allocated_cap == 1 ? true : false);
}

bool dbRSeg::pathLowToHigh()
{
  _dbRSeg* seg = (_dbRSeg*) this;
  return (seg->flags_.path_dir == 0 ? true : false);
}

void dbRSeg::addRSegCapacitance(dbRSeg* other)
{
  _dbRSeg* seg = (_dbRSeg*) this;
  if (!seg->flags_.allocated_cap) {
    getTargetCapNode()->addCapnCapacitance(other->getTargetCapNode());
    return;
  }
  _dbRSeg* oseg = (_dbRSeg*) other;
  _dbBlock* block = (_dbBlock*) seg->getOwner();
  uint32_t cornerCnt = ((dbBlock*) block)->getCornerCount();

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
             "EDIT: dbRSeg {}, other dbRSeg {}, addRSegCapacitance",
             seg->getId(),
             oseg->getId());

  if (block->journal_) {
    block->journal_->beginAction(dbJournal::kUpdateField);
    block->journal_->pushParam(dbRSegObj);
    block->journal_->pushParam(seg->getId());
    block->journal_->pushParam(_dbRSeg::kAddRSegCapacitance);
    block->journal_->pushParam(oseg->getId());
    block->journal_->endAction();
  }
}

void dbRSeg::addRSegResistance(dbRSeg* other)
{
  _dbRSeg* seg = (_dbRSeg*) this;
  _dbRSeg* oseg = (_dbRSeg*) other;
  _dbBlock* block = (_dbBlock*) seg->getOwner();
  uint32_t cornerCnt = block->corners_per_block_;

  for (uint32_t corner = 0; corner < cornerCnt; corner++) {
    float& value
        = (*block->r_val_tbl_)[((seg->getOID() - 1) * cornerCnt) + 1 + corner];
    float& ovalue
        = (*block->r_val_tbl_)[((oseg->getOID() - 1) * cornerCnt) + 1 + corner];
    value += ovalue;
  }

  debugPrint(getImpl()->getLogger(),
             utl::ODB,
             "DB_EDIT",
             2,
             "EDIT: dbRSeg {}, other dbRSeg {}, addRSegResistance",
             seg->getId(),
             oseg->getId());

  if (block->journal_) {
    block->journal_->beginAction(dbJournal::kUpdateField);
    block->journal_->pushParam(dbRSegObj);
    block->journal_->pushParam(seg->getId());
    block->journal_->pushParam(_dbRSeg::kAddRSegResistance);
    block->journal_->pushParam(oseg->getId());
    block->journal_->endAction();
  }
}

void dbRSeg::setResistance(double res, int corner)
{
  _dbRSeg* seg = (_dbRSeg*) this;
  _dbBlock* block = (_dbBlock*) seg->getOwner();
  uint32_t cornerCnt = block->corners_per_block_;
  assert((corner >= 0) && ((uint32_t) corner < cornerCnt));

  float& value
      = (*block->r_val_tbl_)[((seg->getOID() - 1) * cornerCnt) + 1 + corner];
  float prev_value = value;
  value = (float) res;

  debugPrint(getImpl()->getLogger(),
             utl::ODB,
             "DB_EDIT",
             2,
             "EDIT: dbRSeg {}, setResistance {}, corner {}",
             seg->getId(),
             res,
             corner);

  if (block->journal_) {
    block->journal_->beginAction(dbJournal::kUpdateField);
    block->journal_->pushParam(dbRSegObj);
    block->journal_->pushParam(seg->getId());
    block->journal_->pushParam(_dbRSeg::kResistance);
    block->journal_->pushParam(prev_value);
    block->journal_->pushParam(value);
    block->journal_->pushParam(corner);
    block->journal_->endAction();
  }
}

void dbRSeg::adjustResistance(float factor, int corner)
{
  _dbRSeg* seg = (_dbRSeg*) this;
  _dbBlock* block = (_dbBlock*) seg->getOwner();
  uint32_t cornerCnt = block->corners_per_block_;
  assert((corner >= 0) && ((uint32_t) corner < cornerCnt));

  float& value
      = (*block->r_val_tbl_)[((seg->getOID() - 1) * cornerCnt) + 1 + corner];
  float prev_value = value;
  value *= factor;

  debugPrint(getImpl()->getLogger(),
             utl::ODB,
             "DB_EDIT",
             2,
             "EDIT: dbRSeg {}, adjustResistance {}, corner {}",
             seg->getId(),
             factor,
             corner);

  if (block->journal_) {
    block->journal_->beginAction(dbJournal::kUpdateField);
    block->journal_->pushParam(dbRSegObj);
    block->journal_->pushParam(seg->getId());
    block->journal_->pushParam(_dbRSeg::kResistance);
    block->journal_->pushParam(prev_value);
    block->journal_->pushParam(value);
    block->journal_->pushParam(corner);
    block->journal_->endAction();
  }
}

void dbRSeg::adjustResistance(float factor)
{
  _dbBlock* block = (_dbBlock*) getImpl()->getOwner();
  uint32_t cornerCnt = block->corners_per_block_;
  uint32_t corner;
  for (corner = 0; corner < cornerCnt; corner++) {
    adjustResistance(factor, corner);
  }
}

void dbRSeg::setCapacitance(double cap, int corner)
{
  _dbRSeg* seg = (_dbRSeg*) this;
  _dbBlock* block = (_dbBlock*) seg->getOwner();
  uint32_t cornerCnt = block->corners_per_block_;

  if (!seg->flags_.allocated_cap) {
    fprintf(stdout, "WARNING: cap value storage is not allocated\n");
    return;
  }
  seg->flags_.update_cap = 1;
  if (cap == 0.0) {
    seg->flags_.update_cap = 0;
  }

  assert((corner >= 0) && ((uint32_t) corner < cornerCnt));
  float& value
      = (*block->c_val_tbl_)[((seg->getOID() - 1) * cornerCnt) + 1 + corner];
  float prev_value = value;
  value = (float) cap;

  debugPrint(getImpl()->getLogger(),
             utl::ODB,
             "DB_EDIT",
             2,
             "EDIT: dbRSeg {}, setCapacitance {}, corner {}",
             seg->getId(),
             cap,
             corner);

  if (block->journal_) {
    block->journal_->beginAction(dbJournal::kUpdateField);
    block->journal_->pushParam(dbRSegObj);
    block->journal_->pushParam(seg->getId());
    block->journal_->pushParam(_dbRSeg::kCapacitance);
    block->journal_->pushParam(prev_value);
    block->journal_->pushParam(value);
    block->journal_->pushParam(corner);
    block->journal_->endAction();
  }
}

void dbRSeg::adjustSourceCapacitance(float factor, uint32_t corner)
{
  _dbRSeg* seg = (_dbRSeg*) this;
  _dbBlock* block = (_dbBlock*) seg->getOwner();

  if (seg->flags_.allocated_cap != 0) {
    return;
  }
  dbCapNode* node = dbCapNode::getCapNode((dbBlock*) block, seg->source_);
  node->adjustCapacitance(factor, corner);
}

void dbRSeg::adjustCapacitance(float factor, uint32_t corner)
{
  _dbRSeg* seg = (_dbRSeg*) this;
  _dbBlock* block = (_dbBlock*) seg->getOwner();
  uint32_t cornerCnt = block->corners_per_block_;

  if (seg->flags_.allocated_cap == 0) {
    _dbBlock* block = (_dbBlock*) seg->getOwner();
    dbCapNode* node = dbCapNode::getCapNode((dbBlock*) block, seg->target_);
    node->adjustCapacitance(factor, corner);
  } else {
    float& value
        = (*block->c_val_tbl_)[((seg->getOID() - 1) * cornerCnt) + 1 + corner];
    float prev_value = value;
    value *= factor;

    debugPrint(getImpl()->getLogger(),
               utl::ODB,
               "DB_EDIT",
               2,
               "EDIT: dbRSeg {}, adjustCapacitance {}, corner {}",
               seg->getId(),
               value,
               0);

    if (block->journal_) {
      block->journal_->beginAction(dbJournal::kUpdateField);
      block->journal_->pushParam(dbRSegObj);
      block->journal_->pushParam(seg->getId());
      block->journal_->pushParam(_dbRSeg::kCapacitance);
      block->journal_->pushParam(prev_value);
      block->journal_->pushParam(value);
      block->journal_->pushParam(0);
      block->journal_->endAction();
    }
  }
}

void dbRSeg::adjustCapacitance(float factor)
{
  _dbBlock* block = (_dbBlock*) getImpl()->getOwner();
  uint32_t cornerCnt = block->corners_per_block_;
  uint32_t corner;
  for (corner = 0; corner < cornerCnt; corner++) {
    adjustCapacitance(factor, corner);
  }
}

double dbRSeg::getCapacitance(int corner)
{
  _dbRSeg* seg = (_dbRSeg*) this;
  _dbBlock* block = (_dbBlock*) seg->getOwner();
  uint32_t cornerCnt = block->corners_per_block_;

  if (seg->flags_.allocated_cap == 0) {
    _dbBlock* block = (_dbBlock*) seg->getOwner();
    dbCapNode* node = dbCapNode::getCapNode((dbBlock*) block, seg->target_);
    return node->getCapacitance(corner);
  }
  assert((corner >= 0) && ((uint32_t) corner < cornerCnt));
  return (*block->c_val_tbl_)[((seg->getOID() - 1) * cornerCnt) + 1 + corner];
}

void dbRSeg::getGndCap(double* gndcap, double* totalcap)
{
  _dbRSeg* seg = (_dbRSeg*) this;
  _dbBlock* block = (_dbBlock*) seg->getOwner();
  uint32_t cornerCnt = block->corners_per_block_;
  double gcap;
  if (seg->flags_.allocated_cap == 0) {
    dbCapNode* node = dbCapNode::getCapNode((dbBlock*) block, seg->target_);
    node->getGndCap(gndcap, totalcap);
  } else {
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
}

void dbRSeg::addGndCap(double* gndcap, double* totalcap)
{
  _dbRSeg* seg = (_dbRSeg*) this;
  _dbBlock* block = (_dbBlock*) seg->getOwner();
  uint32_t cornerCnt = block->corners_per_block_;
  double gcap;
  if (seg->flags_.allocated_cap == 0) {
    dbCapNode* node = dbCapNode::getCapNode((dbBlock*) block, seg->target_);
    node->addGndCap(gndcap, totalcap);
  } else {
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
}

double dbRSeg::getSourceCapacitance(int corner)
{
  //_dbBlock * block = (_dbBlock *) getOwner();

  _dbRSeg* seg = (_dbRSeg*) this;

  if (seg->flags_.allocated_cap == 0) {
    _dbBlock* block = (_dbBlock*) seg->getOwner();
    dbCapNode* node = dbCapNode::getCapNode((dbBlock*) block, seg->source_);
    return node->getCapacitance(corner);
  }
  return 0.0;
}

dbCapNode* dbRSeg::getTargetCapNode()
{
  _dbBlock* block = (_dbBlock*) getImpl()->getOwner();
  uint32_t target = getTargetNode();

  if (target == 0) {
    return nullptr;
  }

  _dbCapNode* n = block->cap_node_tbl_->getPtr(target);
  return (dbCapNode*) n;
}

dbCapNode* dbRSeg::getSourceCapNode()
{
  _dbBlock* block = (_dbBlock*) getImpl()->getOwner();
  uint32_t source = getSourceNode();

  if (source == 0) {
    return nullptr;
  }

  _dbCapNode* n = block->cap_node_tbl_->getPtr(source);
  return (dbCapNode*) n;
}

double dbRSeg::getCapacitance(const int corner, const double miller_mult)
{
  const double cap = getCapacitance(corner);

  dbCapNode* targetCapNode = getTargetCapNode();

  if (targetCapNode == nullptr) {
    return cap;
  }

  double ccCap = 0.0;
  for (dbCCSeg* cc : targetCapNode->getCCSegs()) {
    ccCap += cc->getCapacitance(corner);
  }

  return cap + (miller_mult * ccCap);
}

void dbRSeg::getGndTotalCap(double* gndcap,
                            double* totalcap,
                            double miller_mult)
{
  getGndCap(gndcap, totalcap);
  getTargetCapNode()->accAllCcCap(totalcap, miller_mult);
}

void dbRSeg::addGndTotalCap(double* gndcap,
                            double* totalcap,
                            double miller_mult)
{
  addGndCap(gndcap, totalcap);
  getTargetCapNode()->accAllCcCap(totalcap, miller_mult);
}

void dbRSeg::getCcSegs(std::vector<dbCCSeg*>& ccsegs)
{
  ccsegs.clear();

  const uint32_t target = getTargetNode();

  for (dbCapNode* n : getNet()->getCapNodes()) {
    if (n->getNode() == target) {
      for (dbCCSeg* cc : n->getCCSegs()) {
        ccsegs.push_back(cc);
      }

      break;
    }
  }
}

void dbRSeg::printCcSegs()
{
  std::vector<dbCCSeg*> ccsegs;
  getCcSegs(ccsegs);
  getImpl()->getLogger()->info(
      utl::ODB, 54, "CC segs of RSeg {}-{}", getSourceNode(), getTargetNode());
}

void dbRSeg::printCC()
{
  getImpl()->getLogger()->info(utl::ODB, 56, "rseg {}", getId());
  getTargetCapNode()->printCC();
}

bool dbRSeg::checkCC()
{
  return getTargetCapNode()->checkCC();
}

void dbRSeg::getCapTable(double* cap)
{
  _dbRSeg* seg = (_dbRSeg*) this;
  _dbBlock* block = (_dbBlock*) seg->getOwner();
  uint32_t cornerCnt = block->corners_per_block_;

  if (seg->flags_.allocated_cap == 0) {
    _dbBlock* block = (_dbBlock*) seg->getOwner();
    dbCapNode* node = dbCapNode::getCapNode((dbBlock*) block, seg->target_);
    node->getCapTable(cap);
  } else {
    for (uint32_t ii = 0; ii < cornerCnt; ii++) {
      cap[ii]
          = (*block->c_val_tbl_)[((seg->getOID() - 1) * cornerCnt) + 1 + ii];
    }
  }
}

uint32_t dbRSeg::getSourceNode()
{
  _dbRSeg* seg = (_dbRSeg*) this;
  return seg->source_;
}

void dbRSeg::setNext(uint32_t rid)
{
  _dbRSeg* seg = (_dbRSeg*) this;
  seg->next_ = rid;
}

void dbRSeg::setSourceNode(uint32_t source_node)
{
  _dbRSeg* seg = (_dbRSeg*) this;
  _dbBlock* block = (_dbBlock*) seg->getOwner();

  debugPrint(getImpl()->getLogger(),
             utl::ODB,
             "DB_EDIT",
             2,
             "EDIT: dbRSeg {}, setSourceNode {}",
             seg->getId(),
             source_node);

  if (block->journal_) {
    block->journal_->updateField(
        this, _dbRSeg::kSource, seg->source_, source_node);
  }

  seg->source_ = source_node;
}

void dbRSeg::setTargetNode(uint32_t target_node)
{
  _dbRSeg* seg = (_dbRSeg*) this;
  _dbBlock* block = (_dbBlock*) seg->getOwner();

  debugPrint(getImpl()->getLogger(),
             utl::ODB,
             "DB_EDIT",
             2,
             "EDIT: dbRSeg {}, setTargetNode {}",
             seg->getId(),
             target_node);

  if (block->journal_) {
    block->journal_->updateField(
        this, _dbRSeg::kTarget, seg->target_, target_node);
  }

  seg->target_ = target_node;
}

uint32_t dbRSeg::getTargetNode()
{
  _dbRSeg* seg = (_dbRSeg*) this;
  return seg->target_;
}

uint32_t dbRSeg::getShapeId()
{
  _dbRSeg* seg = (_dbRSeg*) this;
  dbBlock* block = (dbBlock*) seg->getOwner();
  dbCapNode* node = dbCapNode::getCapNode(block, seg->target_);
  return node->getShapeId();
}

void dbRSeg::setCoords(int x, int y)
{
  _dbRSeg* seg = (_dbRSeg*) this;
  const int prev_x = seg->xcoord_;
  const int prev_y = seg->ycoord_;
  seg->xcoord_ = x;
  seg->ycoord_ = y;
  _dbBlock* block = (_dbBlock*) seg->getOwner();
  debugPrint(getImpl()->getLogger(),
             utl::ODB,
             "DB_EDIT",
             2,
             "EDIT: dbRSeg {}, setCoords {} {}",
             seg->getId(),
             x,
             y);

  if (block->journal_) {
    block->journal_->beginAction(dbJournal::kUpdateField);
    block->journal_->pushParam(dbRSegObj);
    block->journal_->pushParam(seg->getId());
    block->journal_->pushParam(_dbRSeg::kCoordinates);
    block->journal_->pushParam(prev_x);
    block->journal_->pushParam(x);
    block->journal_->pushParam(prev_y);
    block->journal_->pushParam(y);
    block->journal_->endAction();
  }
}

void dbRSeg::getCoords(int& x, int& y)
{
  _dbRSeg* seg = (_dbRSeg*) this;
  // dbBlock * block = (dbBlock *) getOwner();
  //    dbCapNode *node= dbCapNode::getCapNode(block, seg->_target);
  //    if (node->getTermCoords(x, y))
  //        return;
  x = seg->xcoord_;
  y = seg->ycoord_;
  // node->getCoordY(y);
}

uint32_t dbRSeg::getLengthWidth(uint32_t& w)
{
  dbShape s;
  dbWire* wire = getNet()->getWire();
  wire->getShape(getShapeId(), s);

  w = std::min(s.getDX(), s.getDY());

  return std::max(s.getDX(), s.getDY());
}

dbNet* dbRSeg::getNet()
{
  _dbRSeg* seg = (_dbRSeg*) this;
  _dbBlock* block = (_dbBlock*) seg->getOwner();
  dbCapNode* node = dbCapNode::getCapNode((dbBlock*) block, seg->target_);
  return node->getNet();
}

void dbRSeg::updateShapeId(uint32_t nsid)
{
  _dbRSeg* seg = (_dbRSeg*) this;
  _dbBlock* block = (_dbBlock*) seg->getOwner();
  dbCapNode* node = dbCapNode::getCapNode((dbBlock*) block, seg->target_);
  if (node->isITerm() || node->isBTerm()) {
    return;
  }
  node->setNode(nsid);
}

dbRSeg* dbRSeg::create(dbNet* net_,
                       int x,
                       int y,
                       uint32_t path_dir,
                       bool allocate_cap)
{
  _dbNet* net = (_dbNet*) net_;
  _dbBlock* block = (_dbBlock*) net->getOwner();
  uint32_t cornerCnt = block->corners_per_block_;

  debugPrint(net_->getImpl()->getLogger(),
             utl::ODB,
             "DB_EDIT",
             2,
             "EDIT: dbRSeg create 2, net id {}, x: {}, y: {}, path_dir: {}, "
             "allocate_cap: {}",
             net->getId(),
             x,
             y,
             path_dir,
             allocate_cap);

  if (block->journal_) {
    block->journal_->beginAction(dbJournal::kCreateObject);
    block->journal_->pushParam(dbRSegObj);
    block->journal_->pushParam(net->getId());
    block->journal_->pushParam(x);
    block->journal_->pushParam(y);
    block->journal_->pushParam(path_dir);
    block->journal_->pushParam(allocate_cap);
    block->journal_->endAction();
  }

  _dbRSeg* seg = block->r_seg_tbl_->create();
  uint32_t valueMem = 0;

  if (block->max_rseg_id_ >= seg->getOID()) {
    valueMem = 1;
  } else {
    block->max_rseg_id_ = seg->getOID();
  }

  seg->xcoord_ = x;
  seg->ycoord_ = y;

  seg->flags_.path_dir = path_dir;
  // seg->flags_._cnt = block->_num_corners;

  if (valueMem) {
    for (uint32_t ii = 0; ii < cornerCnt; ii++) {
      (*block->r_val_tbl_)[((seg->getOID() - 1) * cornerCnt) + 1 + ii] = 0.0;
    }
  } else {
    [[maybe_unused]] uint32_t resIdx
        = block->r_val_tbl_->getIdx(cornerCnt, (float) 0.0);
    assert((seg->getOID() - 1) * cornerCnt + 1 == resIdx);
  }

  // seg->_resIdx= block->_r_val_tbl->size();
  // int i;
  // for( i = 0; i < seg->flags_._cnt; ++i )
  //{
  // block->_r_val_tbl->push_back(0.0);
  //}

  if (allocate_cap) {
    seg->flags_.allocated_cap = 1;

    if (valueMem) {
      for (uint32_t ii = 0; ii < cornerCnt; ii++) {
        (*block->c_val_tbl_)[((seg->getOID() - 1) * cornerCnt) + 1 + ii] = 0.0;
      }
    } else {
      [[maybe_unused]] const uint32_t capIdx
          = block->c_val_tbl_->getIdx(cornerCnt, (float) 0.0);
      assert((seg->getOID() - 1) * cornerCnt + 1 == capIdx);
    }

    // seg->_capIdx= block->_c_val_tbl->size();
    // for( i = 0; i < seg->flags_._cnt; ++i )
    //{
    // block->_c_val_tbl->push_back(0.0);
    //}
  }

  /* OPT-MEM
  //    seg->_res = (float *) safe_malloc(sizeof(float)*seg->flags_._cnt);
  //
  //    int i;
  //
  //    for( i = 0; i < seg->flags_._cnt; ++i )
  //    {
  //        seg->_res[i] = 0.0;
  //    }
  //
  //    seg->_cap= nullptr;
  //    if (allocate_cap)
  //    {
  //        seg->flags_._allocated_cap= 1;
  //        seg->_cap =
  //             (float *) safe_malloc( sizeof(float) * seg->flags_._cnt );
  //
  //        int i;
  //        for( i = 0; i < seg->flags_._cnt; ++i )
  //        {
  //            seg->_cap[i] = 0.0;
  //        }
  //    }
  */
  // seg->_net = net->getOID();
  seg->next_ = net->r_segs_;
  net->r_segs_ = seg->getOID();
  return (dbRSeg*) seg;
}
bool dbRSeg::addToNet()
{
  dbCapNode* cap_node = getTargetCapNode();
  if (cap_node == nullptr) {
    cap_node = getSourceCapNode();
    if (cap_node == nullptr) {
      getImpl()->getLogger()->warn(
          utl::ODB, 57, "Cannot find cap nodes for Rseg {}", this->getId());
      return false;
    }
  }

  _dbCapNode* seg = (_dbCapNode*) cap_node;
  _dbBlock* block = (_dbBlock*) seg->getOwner();
  _dbNet* net = (_dbNet*) dbNet::getNet((dbBlock*) block, seg->net_);

  _dbRSeg* rseg = (_dbRSeg*) this;
  rseg->next_ = net->r_segs_;
  net->r_segs_ = rseg->getOID();

  return true;
}

void dbRSeg::destroy(dbRSeg* seg_, dbNet* net_)
{
  _dbRSeg* seg = (_dbRSeg*) seg_;
  _dbNet* net = (_dbNet*) net_;
  _dbBlock* block = (_dbBlock*) seg->getOwner();

  debugPrint(net_->getImpl()->getLogger(),
             utl::ODB,
             "DB_EDIT",
             2,
             "EDIT: dbRSeg destroy seg {}, net {} ({})",
             seg->getId(),
             net->getId(),
             (block->journal_ ? block->journal_->size() : 0));

  if (block->journal_) {
    block->journal_->beginAction(dbJournal::kDeleteObject);
    block->journal_->pushParam(dbRSegObj);
    block->journal_->pushParam(seg->getId());
    block->journal_->pushParam(net->getId());
    block->journal_->endAction();
  }
  debugPrint(net_->getImpl()->getLogger(),
             utl::ODB,
             "DB_EDIT",
             2,
             "EDIT: dbRSeg destroyed seg {}, net {} ({}) ({} {})",
             seg->getId(),
             net->getId(),
             (block->journal_ ? block->journal_->size() : 0),
             (void*) block,
             (void*) (block->journal_));

  dbId<_dbRSeg> c = net->r_segs_;
  _dbRSeg* p = nullptr;

  while (c != 0) {
    _dbRSeg* s = block->r_seg_tbl_->getPtr(c);

    if (s == seg) {
      if (p == nullptr) {
        net->r_segs_ = s->next_;
      } else {
        p->next_ = s->next_;
      }
      break;
    }
    p = s;
    c = s->next_;
  }

  dbProperty::destroyProperties(seg);
  block->r_seg_tbl_->destroy(seg);
}

void dbRSeg::destroyS(dbRSeg* seg_)
{
  _dbRSeg* seg = (_dbRSeg*) seg_;
  _dbBlock* block = (_dbBlock*) seg->getOwner();

  debugPrint(seg_->getImpl()->getLogger(),
             utl::ODB,
             "DB_EDIT",
             2,
             "EDIT: dbRSeg simple destroy seg {}",
             seg->getId());

  if (block->journal_) {
    block->journal_->beginAction(dbJournal::kDeleteObject);
    block->journal_->pushParam(dbRSegObj);
    block->journal_->pushParam(seg->getId());
    block->journal_->pushParam((uint32_t) 0);
    block->journal_->endAction();
  }
  dbProperty::destroyProperties(seg);
  block->r_seg_tbl_->destroy(seg);
}

void dbRSeg::destroy(dbRSeg* seg_)
{
  dbRSeg::destroy(seg_, seg_->getNet());
}

dbSet<dbRSeg>::iterator dbRSeg::destroy(dbSet<dbRSeg>::iterator& itr)
{
  dbRSeg* bt = *itr;
  dbSet<dbRSeg>::iterator next = ++itr;
  destroy(bt);
  return next;
}

dbRSeg* dbRSeg::getRSeg(dbBlock* block_, uint32_t dbid_)
{
  _dbBlock* block = (_dbBlock*) block_;
  return (dbRSeg*) block->r_seg_tbl_->getPtr(dbid_);
}

void dbRSeg::mergeRCs(std::vector<dbRSeg*>& mrsegs)
{
  uint32_t rsegcnt = mrsegs.size();
  dbRSeg* finalRSeg = mrsegs[rsegcnt - 1];
  if (rsegcnt <= 1) {
    // finalRSeg->setNext(0);
    setNext(finalRSeg->getId());
    // finalRSeg->setSourceNode(getTargetNode());
    return;
  }
  std::vector<dbCCSeg*> mCcSegs;
  std::vector<dbCapNode*> otherCapns;
  mCcSegs.push_back(nullptr);
  otherCapns.push_back(nullptr);
  dbRSeg* rseg;
  dbCapNode* tgtCapNode;
  dbCapNode* ccCapNode;
  dbCapNode* finalCapNode = finalRSeg->getTargetCapNode();
  dbCCSeg *ccSeg, *tccSeg;
  int ii;
  uint32_t cid;
  for (ii = rsegcnt - 1; ii >= 0; ii--) {
    rseg = mrsegs[ii];
    tgtCapNode = rseg->getTargetCapNode();
    dbSet<dbCCSeg> ccSegs = tgtCapNode->getCCSegs();
    dbSet<dbCCSeg>::iterator ccitr;
    for (ccitr = ccSegs.begin(); ccitr != ccSegs.end();) {
      ccSeg = *ccitr;
      ccitr++;
      ccCapNode = ccSeg->getTheOtherCapn(tgtCapNode, cid);
      uint32_t ccidx = ccCapNode->getSortIndex();
      if (ccidx == 0) {
        if (tgtCapNode != finalCapNode) {  // plug to finalCapNode
          ccSeg->swapCapnode(tgtCapNode, finalCapNode);
        }
        mCcSegs.push_back(ccSeg);
        otherCapns.push_back(ccCapNode);
        ccCapNode->setSortIndex(mCcSegs.size() - 1);
        continue;
      }
      tccSeg = mCcSegs[ccidx];
      tccSeg->addCcCapacitance(ccSeg);
      // if (tgtCapNode != finalCapNode)
      // destroy ccSeg - unlink only ccCapNode
      // else
      // destroy ccSeg
      dbCCSeg::destroy(ccSeg);
    }
    if (tgtCapNode != finalCapNode) {
      finalRSeg->addRSegCapacitance(rseg);
      finalRSeg->addRSegResistance(rseg);
      // dbRSeg::destroy(rseg);
      // dbCapNode::destroy(tgtCapNode, false/*destroyCC*/);
    }
  }
  for (ii = 1; ii < (int) otherCapns.size(); ii++) {
    otherCapns[ii]->setSortIndex(0);
  }
  // finalRSeg->setNext(0);
  setNext(finalRSeg->getId());
  finalRSeg->setSourceNode(mrsegs[0]->getSourceNode());
  for (ii = rsegcnt - 2; ii >= 0; ii--) {
    rseg = mrsegs[ii];
    tgtCapNode = rseg->getTargetCapNode();
    dbRSeg::destroy(rseg);
    dbCapNode::destroy(tgtCapNode, false /*destroyCC*/);
  }
}

void _dbRSeg::collectMemInfo(MemInfo& info)
{
  info.cnt++;
  info.size += sizeof(*this);
}

}  // namespace odb
