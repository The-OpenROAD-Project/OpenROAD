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

#include "dbRSeg.h"

#include "db.h"
#include "dbBlock.h"
#include "dbCapNode.h"
#include "dbCommon.h"
#include "dbDatabase.h"
#include "dbJournal.h"
#include "dbNet.h"
#include "dbShape.h"
#include "dbTable.h"
#include "dbTable.hpp"
#include "utl/Logger.h"

namespace odb {

template class dbTable<_dbRSeg>;

bool _dbRSeg::operator==(const _dbRSeg& rhs) const
{
  if (_flags._path_dir != rhs._flags._path_dir)
    return false;

  if (_flags._allocated_cap != rhs._flags._allocated_cap)
    return false;

  if (_source != rhs._source)
    return false;

  if (_target != rhs._target)
    return false;

  if (_xcoord != rhs._xcoord)
    return false;

  if (_ycoord != rhs._ycoord)
    return false;

  if (_next != rhs._next)
    return false;

  return true;
}

void _dbRSeg::differences(dbDiff& diff,
                          const char* field,
                          const _dbRSeg& rhs) const
{
  DIFF_BEGIN
  DIFF_FIELD(_flags._path_dir);
  DIFF_FIELD(_flags._allocated_cap);
  DIFF_FIELD(_source);
  DIFF_FIELD(_target);
  DIFF_FIELD(_xcoord);
  DIFF_FIELD(_ycoord);
  DIFF_FIELD(_next);
  DIFF_END
}

void _dbRSeg::out(dbDiff& diff, char side, const char* field) const
{
  DIFF_OUT_BEGIN
  DIFF_OUT_FIELD(_flags._path_dir);
  DIFF_OUT_FIELD(_flags._allocated_cap);
  DIFF_OUT_FIELD(_source);
  DIFF_OUT_FIELD(_target);
  DIFF_OUT_FIELD(_xcoord);
  DIFF_OUT_FIELD(_ycoord);
  DIFF_OUT_FIELD(_next);
  DIFF_END
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
  uint cornerCnt = block->_corners_per_block;

  ZASSERT((corner >= 0) && ((uint) corner < cornerCnt));
  return (*block->_r_val_tbl)[(seg->getOID() - 1) * cornerCnt + 1 + corner];
}

void dbRSeg::getAllRes(double* res)
{
  _dbRSeg* seg = (_dbRSeg*) this;
  _dbBlock* block = (_dbBlock*) seg->getOwner();
  uint cornerCnt = block->_corners_per_block;

  for (uint ii = 0; ii < cornerCnt; ii++)
    res[ii] = (*block->_r_val_tbl)[(seg->getOID() - 1) * cornerCnt + 1 + ii];
}

void dbRSeg::addAllRes(double* res)
{
  _dbRSeg* seg = (_dbRSeg*) this;
  _dbBlock* block = (_dbBlock*) seg->getOwner();
  uint cornerCnt = block->_corners_per_block;

  for (uint ii = 0; ii < cornerCnt; ii++)
    res[ii] += (*block->_r_val_tbl)[(seg->getOID() - 1) * cornerCnt + 1 + ii];
}

bool dbRSeg::updatedCap()
{
  _dbRSeg* seg = (_dbRSeg*) this;
  return (seg->_flags._update_cap == 1 ? true : false);
}
bool dbRSeg::allocatedCap()
{
  _dbRSeg* seg = (_dbRSeg*) this;
  return (seg->_flags._allocated_cap == 1 ? true : false);
}

bool dbRSeg::pathLowToHigh()
{
  _dbRSeg* seg = (_dbRSeg*) this;
  return (seg->_flags._path_dir == 0 ? true : false);
}

void dbRSeg::addRSegCapacitance(dbRSeg* other)
{
  _dbRSeg* seg = (_dbRSeg*) this;
  if (!seg->_flags._allocated_cap) {
    getTargetCapNode()->addCapnCapacitance(other->getTargetCapNode());
    return;
  }
  _dbRSeg* oseg = (_dbRSeg*) other;
  _dbBlock* block = (_dbBlock*) seg->getOwner();
  uint cornerCnt = ((dbBlock*) block)->getCornerCount();

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
               "ECO: dbRSeg {}, other dbRSeg {}, addRSegCapacitance",
               seg->getId(),
               oseg->getId());
    block->_journal->beginAction(dbJournal::UPDATE_FIELD);
    block->_journal->pushParam(dbRSegObj);
    block->_journal->pushParam(seg->getId());
    block->_journal->pushParam(_dbRSeg::ADDRSEGCAPACITANCE);
    block->_journal->pushParam(oseg->getId());
    block->_journal->endAction();
  }
}

void dbRSeg::addRSegResistance(dbRSeg* other)
{
  _dbRSeg* seg = (_dbRSeg*) this;
  _dbRSeg* oseg = (_dbRSeg*) other;
  _dbBlock* block = (_dbBlock*) seg->getOwner();
  uint cornerCnt = block->_corners_per_block;

  for (uint corner = 0; corner < cornerCnt; corner++) {
    float& value
        = (*block->_r_val_tbl)[(seg->getOID() - 1) * cornerCnt + 1 + corner];
    float& ovalue
        = (*block->_r_val_tbl)[(oseg->getOID() - 1) * cornerCnt + 1 + corner];
    value += ovalue;
  }

  if (block->_journal) {
    debugPrint(getImpl()->getLogger(),
               utl::ODB,
               "DB_ECO",
               1,
               "ECO: dbRSeg {}, other dbRSeg {}, addRSegResistance",
               seg->getId(),
               oseg->getId());
    block->_journal->beginAction(dbJournal::UPDATE_FIELD);
    block->_journal->pushParam(dbRSegObj);
    block->_journal->pushParam(seg->getId());
    block->_journal->pushParam(_dbRSeg::ADDRSEGRESISTANCE);
    block->_journal->pushParam(oseg->getId());
    block->_journal->endAction();
  }
}

void dbRSeg::setResistance(double res, int corner)
{
  _dbRSeg* seg = (_dbRSeg*) this;
  _dbBlock* block = (_dbBlock*) seg->getOwner();
  uint cornerCnt = block->_corners_per_block;
  ZASSERT((corner >= 0) && ((uint) corner < cornerCnt));

  float& value
      = (*block->_r_val_tbl)[(seg->getOID() - 1) * cornerCnt + 1 + corner];
  float prev_value = value;
  value = (float) res;

  if (block->_journal) {
    debugPrint(getImpl()->getLogger(),
               utl::ODB,
               "DB_ECO",
               1,
               "ECO: dbRSeg {}, setResistance {}, corner {}",
               seg->getId(),
               res,
               corner);
    block->_journal->beginAction(dbJournal::UPDATE_FIELD);
    block->_journal->pushParam(dbRSegObj);
    block->_journal->pushParam(seg->getId());
    block->_journal->pushParam(_dbRSeg::RESISTANCE);
    block->_journal->pushParam(prev_value);
    block->_journal->pushParam(value);
    block->_journal->pushParam(corner);
    block->_journal->endAction();
  }
}

void dbRSeg::adjustResistance(float factor, int corner)
{
  _dbRSeg* seg = (_dbRSeg*) this;
  _dbBlock* block = (_dbBlock*) seg->getOwner();
  uint cornerCnt = block->_corners_per_block;
  ZASSERT((corner >= 0) && ((uint) corner < cornerCnt));

  float& value
      = (*block->_r_val_tbl)[(seg->getOID() - 1) * cornerCnt + 1 + corner];
  float prev_value = value;
  value *= factor;

  if (block->_journal) {
    debugPrint(getImpl()->getLogger(),
               utl::ODB,
               "DB_ECO",
               1,
               "ECO: dbRSeg {}, adjustResistance {}, corner {}",
               seg->getId(),
               factor,
               corner);
    block->_journal->beginAction(dbJournal::UPDATE_FIELD);
    block->_journal->pushParam(dbRSegObj);
    block->_journal->pushParam(seg->getId());
    block->_journal->pushParam(_dbRSeg::RESISTANCE);
    block->_journal->pushParam(prev_value);
    block->_journal->pushParam(value);
    block->_journal->pushParam(corner);
    block->_journal->endAction();
  }
}

void dbRSeg::adjustResistance(float factor)
{
  _dbBlock* block = (_dbBlock*) getImpl()->getOwner();
  uint cornerCnt = block->_corners_per_block;
  uint corner;
  for (corner = 0; corner < cornerCnt; corner++)
    adjustResistance(factor, corner);
}

void dbRSeg::setCapacitance(double cap, int corner)
{
  _dbRSeg* seg = (_dbRSeg*) this;
  _dbBlock* block = (_dbBlock*) seg->getOwner();
  uint cornerCnt = block->_corners_per_block;

  if (!seg->_flags._allocated_cap) {
    fprintf(stdout, "WARNING: cap value storage is not allocated\n");
    return;
  }
  seg->_flags._update_cap = 1;
  if (cap == 0.0)
    seg->_flags._update_cap = 0;

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
               "ECO: dbRSeg {}, setCapacitance {}, corner {}",
               seg->getId(),
               cap,
               corner);
    block->_journal->beginAction(dbJournal::UPDATE_FIELD);
    block->_journal->pushParam(dbRSegObj);
    block->_journal->pushParam(seg->getId());
    block->_journal->pushParam(_dbRSeg::CAPACITANCE);
    block->_journal->pushParam(prev_value);
    block->_journal->pushParam(value);
    block->_journal->pushParam(corner);
    block->_journal->endAction();
  }
}

void dbRSeg::adjustSourceCapacitance(float factor, uint corner)
{
  _dbRSeg* seg = (_dbRSeg*) this;
  _dbBlock* block = (_dbBlock*) seg->getOwner();

  if (seg->_flags._allocated_cap != 0)
    return;
  dbCapNode* node = dbCapNode::getCapNode((dbBlock*) block, seg->_source);
  node->adjustCapacitance(factor, corner);
}

void dbRSeg::adjustCapacitance(float factor, uint corner)
{
  _dbRSeg* seg = (_dbRSeg*) this;
  _dbBlock* block = (_dbBlock*) seg->getOwner();
  uint cornerCnt = block->_corners_per_block;

  if (seg->_flags._allocated_cap == 0) {
    _dbBlock* block = (_dbBlock*) seg->getOwner();
    dbCapNode* node = dbCapNode::getCapNode((dbBlock*) block, seg->_target);
    node->adjustCapacitance(factor, corner);
  } else {
    float& value
        = (*block->_c_val_tbl)[(seg->getOID() - 1) * cornerCnt + 1 + corner];
    float prev_value = value;
    value *= factor;

    if (block->_journal) {
      debugPrint(getImpl()->getLogger(),
                 utl::ODB,
                 "DB_ECO",
                 1,
                 "ECO: dbRSeg {}, adjustCapacitance {}, corner {}",
                 seg->getId(),
                 value,
                 0);
      block->_journal->beginAction(dbJournal::UPDATE_FIELD);
      block->_journal->pushParam(dbRSegObj);
      block->_journal->pushParam(seg->getId());
      block->_journal->pushParam(_dbRSeg::CAPACITANCE);
      block->_journal->pushParam(prev_value);
      block->_journal->pushParam(value);
      block->_journal->pushParam(0);
      block->_journal->endAction();
    }
  }
}

void dbRSeg::adjustCapacitance(float factor)
{
  _dbBlock* block = (_dbBlock*) getImpl()->getOwner();
  uint cornerCnt = block->_corners_per_block;
  uint corner;
  for (corner = 0; corner < cornerCnt; corner++)
    adjustCapacitance(factor, corner);
}

double dbRSeg::getCapacitance(int corner)
{
  _dbRSeg* seg = (_dbRSeg*) this;
  _dbBlock* block = (_dbBlock*) seg->getOwner();
  uint cornerCnt = block->_corners_per_block;

  if (seg->_flags._allocated_cap == 0) {
    _dbBlock* block = (_dbBlock*) seg->getOwner();
    dbCapNode* node = dbCapNode::getCapNode((dbBlock*) block, seg->_target);
    return node->getCapacitance(corner);
  } else {
    ZASSERT((corner >= 0) && ((uint) corner < cornerCnt));
    return (*block->_c_val_tbl)[(seg->getOID() - 1) * cornerCnt + 1 + corner];
  }
}

void dbRSeg::getGndCap(double* gndcap, double* totalcap)
{
  _dbRSeg* seg = (_dbRSeg*) this;
  _dbBlock* block = (_dbBlock*) seg->getOwner();
  uint cornerCnt = block->_corners_per_block;
  double gcap;
  if (seg->_flags._allocated_cap == 0) {
    dbCapNode* node = dbCapNode::getCapNode((dbBlock*) block, seg->_target);
    node->getGndCap(gndcap, totalcap);
  } else {
    for (uint ii = 0; ii < cornerCnt; ii++) {
      gcap = (*block->_c_val_tbl)[(seg->getOID() - 1) * cornerCnt + 1 + ii];
      if (gndcap)
        gndcap[ii] = gcap;
      if (totalcap)
        totalcap[ii] = gcap;
    }
  }
}

void dbRSeg::addGndCap(double* gndcap, double* totalcap)
{
  _dbRSeg* seg = (_dbRSeg*) this;
  _dbBlock* block = (_dbBlock*) seg->getOwner();
  uint cornerCnt = block->_corners_per_block;
  double gcap;
  if (seg->_flags._allocated_cap == 0) {
    dbCapNode* node = dbCapNode::getCapNode((dbBlock*) block, seg->_target);
    node->addGndCap(gndcap, totalcap);
  } else {
    for (uint ii = 0; ii < cornerCnt; ii++) {
      gcap = (*block->_c_val_tbl)[(seg->getOID() - 1) * cornerCnt + 1 + ii];
      if (gndcap)
        gndcap[ii] += gcap;
      if (totalcap)
        totalcap[ii] += gcap;
    }
  }
}

double dbRSeg::getSourceCapacitance(int corner)
{
  //_dbBlock * block = (_dbBlock *) getOwner();

  _dbRSeg* seg = (_dbRSeg*) this;

  if (seg->_flags._allocated_cap == 0) {
    _dbBlock* block = (_dbBlock*) seg->getOwner();
    dbCapNode* node = dbCapNode::getCapNode((dbBlock*) block, seg->_source);
    return node->getCapacitance(corner);
  } else {
    return 0.0;
  }
}

dbCapNode* dbRSeg::getTargetCapNode()
{
  _dbBlock* block = (_dbBlock*) getImpl()->getOwner();
  uint target = getTargetNode();

  if (target == 0)
    return NULL;

  _dbCapNode* n = block->_cap_node_tbl->getPtr(target);
  return (dbCapNode*) n;
}

dbCapNode* dbRSeg::getSourceCapNode()
{
  _dbBlock* block = (_dbBlock*) getImpl()->getOwner();
  uint source = getSourceNode();

  if (source == 0)
    return NULL;

  _dbCapNode* n = block->_cap_node_tbl->getPtr(source);
  return (dbCapNode*) n;
}

double dbRSeg::getCapacitance(int corner, double MillerMult)
{
  double cap = getCapacitance(corner);
  double ccCap = 0.0;

  dbCapNode* targetCapNode = getTargetCapNode();

  if (targetCapNode == NULL)
    return cap;

  dbSet<dbCCSeg> ccSegs = targetCapNode->getCCSegs();
  dbSet<dbCCSeg>::iterator ccitr;
  for (ccitr = ccSegs.begin(); ccitr != ccSegs.end(); ++ccitr) {
    dbCCSeg* cc = *ccitr;
    ccCap += cc->getCapacitance(corner);
  }

  cap += MillerMult * ccCap;
  return cap;
}

void dbRSeg::getGndTotalCap(double* gndcap, double* totalcap, double MillerMult)
{
  getGndCap(gndcap, totalcap);
  getTargetCapNode()->accAllCcCap(totalcap, MillerMult);
}

void dbRSeg::addGndTotalCap(double* gndcap, double* totalcap, double MillerMult)
{
  addGndCap(gndcap, totalcap);
  getTargetCapNode()->accAllCcCap(totalcap, MillerMult);
}

void dbRSeg::getCcSegs(std::vector<dbCCSeg*>& ccsegs)
{
  ccsegs.clear();

  dbSet<dbCapNode> capNodes = getNet()->getCapNodes();
  dbSet<dbCapNode>::iterator citr;

  uint target = getTargetNode();

  for (citr = capNodes.begin(); citr != capNodes.end(); ++citr) {
    dbCapNode* n = *citr;
    dbSet<dbCCSeg> ccSegs = n->getCCSegs();
    dbSet<dbCCSeg>::iterator ccitr;

    if (n->getNode() == target) {
      for (ccitr = ccSegs.begin(); ccitr != ccSegs.end(); ++ccitr) {
        dbCCSeg* cc = *ccitr;
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
#if 0
    uint j;
    dbCCSeg *seg;
    for (j=0;j<ccsegs.size();j++)
    {
        seg = ccsegs[j];
        getImpl()->getLogger()->info(utl::ODB, 55, "           CC{} : {}-{}", j, seg->getSourceNode(), seg->getTargetNode()); 
    }
#endif
}

void dbRSeg::printCC()
{
  getImpl()->getLogger()->info(utl::ODB, 56, "rseg {}", getId());
  getTargetCapNode()->printCC();
}

bool dbRSeg::checkCC()
{
  bool rc = getTargetCapNode()->checkCC();
  return rc;
}

void dbRSeg::getCapTable(double* cap)
{
  _dbRSeg* seg = (_dbRSeg*) this;
  _dbBlock* block = (_dbBlock*) seg->getOwner();
  uint cornerCnt = block->_corners_per_block;

  if (seg->_flags._allocated_cap == 0) {
    _dbBlock* block = (_dbBlock*) seg->getOwner();
    dbCapNode* node = dbCapNode::getCapNode((dbBlock*) block, seg->_target);
    node->getCapTable(cap);
  } else {
    for (uint ii = 0; ii < cornerCnt; ii++)
      cap[ii] = (*block->_c_val_tbl)[(seg->getOID() - 1) * cornerCnt + 1 + ii];
  }
}

uint dbRSeg::getSourceNode()
{
  _dbRSeg* seg = (_dbRSeg*) this;
  return seg->_source;
}

void dbRSeg::setNext(uint rid)
{
  _dbRSeg* seg = (_dbRSeg*) this;
  seg->_next = rid;
}

void dbRSeg::setSourceNode(uint source_node)
{
  _dbRSeg* seg = (_dbRSeg*) this;
  _dbBlock* block = (_dbBlock*) seg->getOwner();

  if (block->_journal) {
    debugPrint(getImpl()->getLogger(),
               utl::ODB,
               "DB_ECO",
               1,
               "ECO: dbRSeg {}, setSourceNode {}",
               seg->getId(),
               source_node);
    block->_journal->updateField(
        this, _dbRSeg::SOURCE, seg->_source, source_node);
  }

  seg->_source = source_node;
}

void dbRSeg::setTargetNode(uint target_node)
{
  _dbRSeg* seg = (_dbRSeg*) this;
  _dbBlock* block = (_dbBlock*) seg->getOwner();

  if (block->_journal) {
    debugPrint(getImpl()->getLogger(),
               utl::ODB,
               "DB_ECO",
               1,
               "ECO: dbRSeg {}, setTargetNode {}",
               seg->getId(),
               target_node);
    block->_journal->updateField(
        this, _dbRSeg::TARGET, seg->_target, target_node);
  }

  seg->_target = target_node;
}

uint dbRSeg::getTargetNode()
{
  _dbRSeg* seg = (_dbRSeg*) this;
  return seg->_target;
}

uint dbRSeg::getShapeId()
{
  _dbRSeg* seg = (_dbRSeg*) this;
  dbBlock* block = (dbBlock*) seg->getOwner();
  dbCapNode* node = dbCapNode::getCapNode(block, seg->_target);
  return node->getShapeId();
}

void dbRSeg::setCoords(int x, int y)
{
  _dbRSeg* seg = (_dbRSeg*) this;
  int prev_x = seg->_xcoord;
  int prev_y = seg->_ycoord;
  seg->_xcoord = x;
  seg->_ycoord = y;
  _dbBlock* block = (_dbBlock*) seg->getOwner();
  if (block->_journal) {
    debugPrint(getImpl()->getLogger(),
               utl::ODB,
               "DB_ECO",
               1,
               "ECO: dbRSeg {}, setCoords {} {}",
               seg->getId(),
               x,
               y);
    block->_journal->beginAction(dbJournal::UPDATE_FIELD);
    block->_journal->pushParam(dbRSegObj);
    block->_journal->pushParam(seg->getId());
    block->_journal->pushParam(_dbRSeg::COORDINATES);
    block->_journal->pushParam(prev_x);
    block->_journal->pushParam(x);
    block->_journal->pushParam(prev_y);
    block->_journal->pushParam(y);
    block->_journal->endAction();
  }
}

void dbRSeg::getCoords(int& x, int& y)
{
  _dbRSeg* seg = (_dbRSeg*) this;
  // dbBlock * block = (dbBlock *) getOwner();
  //    dbCapNode *node= dbCapNode::getCapNode(block, seg->_target);
  //    if (node->getTermCoords(x, y))
  //        return;
  x = seg->_xcoord;
  y = seg->_ycoord;
  // node->getCoordY(y);
}

uint dbRSeg::getLengthWidth(uint& w)
{
  dbShape s;
  dbWire* wire = getNet()->getWire();
  wire->getShape(getShapeId(), s);

  w = MIN(s.getDX(), s.getDY());

  return MAX(s.getDX(), s.getDY());
}

dbNet* dbRSeg::getNet()
{
  _dbRSeg* seg = (_dbRSeg*) this;
  _dbBlock* block = (_dbBlock*) seg->getOwner();
  dbCapNode* node = dbCapNode::getCapNode((dbBlock*) block, seg->_target);
  return node->getNet();
}

void dbRSeg::updateShapeId(uint nsid)
{
  _dbRSeg* seg = (_dbRSeg*) this;
  _dbBlock* block = (_dbBlock*) seg->getOwner();
  dbCapNode* node = dbCapNode::getCapNode((dbBlock*) block, seg->_target);
  if (node->isITerm() || node->isBTerm())
    return;
  node->setNode(nsid);
}

dbRSeg* dbRSeg::create(dbNet* net_,
                       int x,
                       int y,
                       uint path_dir,
                       bool allocate_cap)
{
  _dbNet* net = (_dbNet*) net_;
  _dbBlock* block = (_dbBlock*) net->getOwner();
  uint cornerCnt = block->_corners_per_block;

  if (block->_journal) {
    debugPrint(net_->getImpl()->getLogger(),
               utl::ODB,
               "DB_ECO",
               1,
               "ECO: dbRSeg create 2, net id {}, x: {}, y: {}, path_dir: {}, "
               "allocate_cap: {}",
               net->getId(),
               x,
               y,
               path_dir,
               allocate_cap);
    block->_journal->beginAction(dbJournal::CREATE_OBJECT);
    block->_journal->pushParam(dbRSegObj);
    block->_journal->pushParam(net->getId());
    block->_journal->pushParam(x);
    block->_journal->pushParam(y);
    block->_journal->pushParam(path_dir);
    block->_journal->pushParam(allocate_cap);
    block->_journal->endAction();
  }

  _dbRSeg* seg = block->_r_seg_tbl->create();
  uint valueMem = 0;

  if (block->_maxRSegId >= seg->getOID())
    valueMem = 1;
  else
    block->_maxRSegId = seg->getOID();

  seg->_xcoord = x;
  seg->_ycoord = y;

  seg->_flags._path_dir = path_dir;
  // seg->_flags._cnt = block->_num_corners;

  if (valueMem) {
    for (uint ii = 0; ii < cornerCnt; ii++)
      (*block->_r_val_tbl)[(seg->getOID() - 1) * cornerCnt + 1 + ii] = 0.0;
  } else {
    uint resIdx = block->_r_val_tbl->getIdx(cornerCnt, (float) 0.0);
    ZASSERT((seg->getOID() - 1) * cornerCnt + 1 == resIdx);
  }

  // seg->_resIdx= block->_r_val_tbl->size();
  // int i;
  // for( i = 0; i < seg->_flags._cnt; ++i )
  //{
  // block->_r_val_tbl->push_back(0.0);
  //}

  if (allocate_cap) {
    seg->_flags._allocated_cap = 1;

    if (valueMem) {
      for (uint ii = 0; ii < cornerCnt; ii++)
        (*block->_c_val_tbl)[(seg->getOID() - 1) * cornerCnt + 1 + ii] = 0.0;
    } else {
      uint capIdx = block->_c_val_tbl->getIdx(cornerCnt, (float) 0.0);
      ZASSERT((seg->getOID() - 1) * cornerCnt + 1 == capIdx);
    }

    // seg->_capIdx= block->_c_val_tbl->size();
    // for( i = 0; i < seg->_flags._cnt; ++i )
    //{
    // block->_c_val_tbl->push_back(0.0);
    //}
  }

  /* OPT-MEM
  //    seg->_res = (float *) malloc(sizeof(float)*seg->_flags._cnt);
  //    ZALLOCATED(seg->_res);
  //
  //    int i;
  //
  //    for( i = 0; i < seg->_flags._cnt; ++i )
  //    {
  //        seg->_res[i] = 0.0;
  //    }
  //
  //    seg->_cap= NULL;
  //    if (allocate_cap)
  //    {
  //        seg->_flags._allocated_cap= 1;
  //        seg->_cap = (float *) malloc( sizeof(float) * seg->_flags._cnt );
  //        ZALLOCATED( seg->_cap );
  //
  //        int i;
  //        for( i = 0; i < seg->_flags._cnt; ++i )
  //        {
  //            seg->_cap[i] = 0.0;
  //        }
  //    }
  */
  // seg->_net = net->getOID();
  seg->_next = net->_r_segs;
  net->_r_segs = seg->getOID();
  return (dbRSeg*) seg;
}
bool dbRSeg::addToNet()
{
  dbCapNode* cap_node = getTargetCapNode();
  if (cap_node == NULL) {
    cap_node = getSourceCapNode();
    if (cap_node == NULL) {
      getImpl()->getLogger()->warn(
          utl::ODB, 57, "Cannot find cap nodes for Rseg {}", this->getId());
      return false;
    }
  }

  _dbCapNode* seg = (_dbCapNode*) cap_node;
  _dbBlock* block = (_dbBlock*) seg->getOwner();
  _dbNet* net = (_dbNet*) dbNet::getNet((dbBlock*) block, seg->_net);

  _dbRSeg* rseg = (_dbRSeg*) this;
  rseg->_next = net->_r_segs;
  net->_r_segs = rseg->getOID();

  return true;
}

void dbRSeg::destroy(dbRSeg* seg_, dbNet* net_)
{
  _dbRSeg* seg = (_dbRSeg*) seg_;
  _dbNet* net = (_dbNet*) net_;
  _dbBlock* block = (_dbBlock*) seg->getOwner();

  if (block->_journal) {
    debugPrint(net_->getImpl()->getLogger(),
               utl::ODB,
               "DB_ECO",
               1,
               "ECO: dbRSeg destroy seg {}, net {} ({})",
               seg->getId(),
               net->getId(),
               block->_journal->size());
    block->_journal->beginAction(dbJournal::DELETE_OBJECT);
    block->_journal->pushParam(dbRSegObj);
    block->_journal->pushParam(seg->getId());
    block->_journal->pushParam(net->getId());
    block->_journal->endAction();
    debugPrint(net_->getImpl()->getLogger(),
               utl::ODB,
               "DB_ECO",
               1,
               "ECO: dbRSeg destroyed seg {}, net {} ({}) ({} {})",
               seg->getId(),
               net->getId(),
               block->_journal->size(),
               (void*) block,
               (void*) block->_journal);
  }

  dbId<_dbRSeg> c = net->_r_segs;
  _dbRSeg* p = NULL;

  while (c != 0) {
    _dbRSeg* s = block->_r_seg_tbl->getPtr(c);

    if (s == seg) {
      if (p == NULL)
        net->_r_segs = s->_next;
      else
        p->_next = s->_next;
      break;
    }
    p = s;
    c = s->_next;
  }

  dbProperty::destroyProperties(seg);
  block->_r_seg_tbl->destroy(seg);
}

void dbRSeg::destroyS(dbRSeg* seg_)
{
  _dbRSeg* seg = (_dbRSeg*) seg_;
  _dbBlock* block = (_dbBlock*) seg->getOwner();

  if (block->_journal) {
    debugPrint(seg_->getImpl()->getLogger(),
               utl::ODB,
               "DB_ECO",
               1,
               "ECO: dbRSeg simple destroy seg {}",
               seg->getId());
    block->_journal->beginAction(dbJournal::DELETE_OBJECT);
    block->_journal->pushParam(dbRSegObj);
    block->_journal->pushParam(seg->getId());
    block->_journal->pushParam((uint) 0);
    block->_journal->endAction();
  }
  dbProperty::destroyProperties(seg);
  block->_r_seg_tbl->destroy(seg);
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

dbRSeg* dbRSeg::getRSeg(dbBlock* block_, uint dbid_)
{
  _dbBlock* block = (_dbBlock*) block_;
  return (dbRSeg*) block->_r_seg_tbl->getPtr(dbid_);
}

void dbRSeg::mergeRCs(std::vector<dbRSeg*>& mrsegs)
{
  uint rsegcnt = mrsegs.size();
  dbRSeg* finalRSeg = mrsegs[rsegcnt - 1];
  if (rsegcnt <= 1) {
    // finalRSeg->setNext(0);
    setNext(finalRSeg->getId());
    // finalRSeg->setSourceNode(getTargetNode());
    return;
  }
  std::vector<dbCCSeg*> mCcSegs;
  std::vector<dbCapNode*> otherCapns;
  mCcSegs.push_back(NULL);
  otherCapns.push_back(NULL);
  dbRSeg* rseg;
  dbCapNode* tgtCapNode;
  dbCapNode* ccCapNode;
  dbCapNode* finalCapNode = finalRSeg->getTargetCapNode();
  dbCCSeg *ccSeg, *tccSeg;
  int ii;
  uint cid;
  for (ii = rsegcnt - 1; ii >= 0; ii--) {
    rseg = mrsegs[ii];
    tgtCapNode = rseg->getTargetCapNode();
    dbSet<dbCCSeg> ccSegs = tgtCapNode->getCCSegs();
    dbSet<dbCCSeg>::iterator ccitr;
    for (ccitr = ccSegs.begin(); ccitr != ccSegs.end();) {
      ccSeg = *ccitr;
      ccitr++;
      ccCapNode = ccSeg->getTheOtherCapn(tgtCapNode, cid);
      uint ccidx = ccCapNode->getSortIndex();
      if (ccidx == 0) {
        if (tgtCapNode != finalCapNode)  // plug to finalCapNode
          ccSeg->swapCapnode(tgtCapNode, finalCapNode);
        mCcSegs.push_back(ccSeg);
        otherCapns.push_back(ccCapNode);
        ccCapNode->setSortIndex(mCcSegs.size() - 1);
        continue;
      } else {
        tccSeg = mCcSegs[ccidx];
        tccSeg->addCcCapacitance(ccSeg);
        // if (tgtCapNode != finalCapNode)
        // destroy ccSeg - unlink only ccCapNode
        // else
        // destroy ccSeg
        dbCCSeg::destroy(ccSeg);
      }
    }
    if (tgtCapNode != finalCapNode) {
      finalRSeg->addRSegCapacitance(rseg);
      finalRSeg->addRSegResistance(rseg);
      // dbRSeg::destroy(rseg);
      // dbCapNode::destroy(tgtCapNode, false/*destroyCC*/);
    }
  }
  for (ii = 1; ii < (int) otherCapns.size(); ii++)
    otherCapns[ii]->setSortIndex(0);
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

}  // namespace odb
