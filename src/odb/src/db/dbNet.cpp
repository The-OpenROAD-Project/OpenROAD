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

#include "dbNet.h"

#include <algorithm>
#include <vector>

#include "dbBTerm.h"
#include "dbBTermItr.h"
#include "dbBlock.h"
#include "dbCCSeg.h"
#include "dbCCSegItr.h"
#include "dbCapNode.h"
#include "dbCapNodeItr.h"
#include "dbCommon.h"
#include "dbDatabase.h"
#include "dbGroup.h"
#include "dbGuide.h"
#include "dbGuideItr.h"
#include "dbITerm.h"
#include "dbITermItr.h"
#include "dbInst.h"
#include "dbJournal.h"
#include "dbMTerm.h"
#include "dbNetTrack.h"
#include "dbNetTrackItr.h"
#include "dbRSeg.h"
#include "dbRSegItr.h"
#include "dbSWire.h"
#include "dbSWireItr.h"
#include "dbTable.h"
#include "dbTable.hpp"
#include "dbTech.h"
#include "dbTechNonDefaultRule.h"
#include "dbWire.h"
#include "odb/db.h"
#include "odb/dbBlockCallBackObj.h"
#include "odb/dbExtControl.h"
#include "odb/dbSet.h"
#include "odb/dbShape.h"
#include "utl/Logger.h"

namespace odb {

template class dbTable<_dbNet>;

_dbNet::_dbNet(_dbDatabase* db, const _dbNet& n)
    : _flags(n._flags),
      _name(nullptr),
      _next_entry(n._next_entry),
      _iterms(n._iterms),
      _bterms(n._bterms),
      _wire(n._wire),
      _global_wire(n._global_wire),
      _swires(n._swires),
      _cap_nodes(n._cap_nodes),
      _r_segs(n._r_segs),
      _non_default_rule(n._non_default_rule),
      guides_(n.guides_),
      tracks_(n.tracks_),
      _groups(n._groups),
      _weight(n._weight),
      _xtalk(n._xtalk),
      _ccAdjustFactor(n._ccAdjustFactor),
      _ccAdjustOrder(n._ccAdjustOrder)

{
  if (n._name) {
    _name = strdup(n._name);
    ZALLOCATED(_name);
  }
  _drivingIterm = -1;
}

_dbNet::_dbNet(_dbDatabase* db)
{
  _flags._sig_type = dbSigType::SIGNAL;
  _flags._wire_type = dbWireType::ROUTED;
  _flags._special = 0;
  _flags._wild_connect = 0;
  _flags._wire_ordered = 0;
  _flags._unused2 = 0;
  _flags._disconnected = 0;
  _flags._spef = 0;
  _flags._select = 0;
  _flags._mark = 0;
  _flags._mark_1 = 0;
  _flags._wire_altered = 0;
  _flags._extracted = 0;
  _flags._rc_graph = 0;
  _flags._unused = 0;
  _flags._set_io = 0;
  _flags._io = 0;
  _flags._dont_touch = 0;
  _flags._fixed_bump = 0;
  _flags._source = dbSourceType::NONE;
  _flags._rc_disconnected = 0;
  _flags._block_rule = 0;
  _flags._has_jumpers = 0;
  _name = nullptr;
  _gndc_calibration_factor = 1.0;
  _cc_calibration_factor = 1.0;
  _weight = 1;
  _xtalk = 0;
  _ccAdjustFactor = -1;
  _ccAdjustOrder = 0;
  _drivingIterm = -1;
}

_dbNet::~_dbNet()
{
  if (_name) {
    free((void*) _name);
  }
}

dbOStream& operator<<(dbOStream& stream, const _dbNet& net)
{
  uint* bit_field = (uint*) &net._flags;
  stream << *bit_field;
  stream << net._name;
  stream << net._gndc_calibration_factor;
  stream << net._cc_calibration_factor;
  stream << net._next_entry;
  stream << net._iterms;
  stream << net._bterms;
  stream << net._wire;
  stream << net._global_wire;
  stream << net._swires;
  stream << net._cap_nodes;
  stream << net._r_segs;
  stream << net._non_default_rule;
  stream << net._weight;
  stream << net._xtalk;
  stream << net._ccAdjustFactor;
  stream << net._ccAdjustOrder;
  stream << net._groups;
  stream << net.guides_;
  stream << net.tracks_;
  return stream;
}

dbIStream& operator>>(dbIStream& stream, _dbNet& net)
{
  uint* bit_field = (uint*) &net._flags;
  stream >> *bit_field;
  stream >> net._name;
  stream >> net._gndc_calibration_factor;
  stream >> net._cc_calibration_factor;
  stream >> net._next_entry;
  stream >> net._iterms;
  stream >> net._bterms;
  stream >> net._wire;
  stream >> net._global_wire;
  stream >> net._swires;
  stream >> net._cap_nodes;
  stream >> net._r_segs;
  stream >> net._non_default_rule;
  stream >> net._weight;
  stream >> net._xtalk;
  stream >> net._ccAdjustFactor;
  stream >> net._ccAdjustOrder;
  stream >> net._groups;
  stream >> net.guides_;
  _dbDatabase* db = net.getImpl()->getDatabase();
  if (db->isSchema(db_schema_net_tracks)) {
    stream >> net.tracks_;
  }

  return stream;
}

bool _dbNet::operator<(const _dbNet& rhs) const
{
  return strcmp(_name, rhs._name) < 0;
}

bool _dbNet::operator==(const _dbNet& rhs) const
{
  if (_flags._sig_type != rhs._flags._sig_type) {
    return false;
  }

  if (_flags._wire_type != rhs._flags._wire_type) {
    return false;
  }

  if (_flags._special != rhs._flags._special) {
    return false;
  }

  if (_flags._wild_connect != rhs._flags._wild_connect) {
    return false;
  }

  if (_flags._wire_ordered != rhs._flags._wire_ordered) {
    return false;
  }

  if (_flags._disconnected != rhs._flags._disconnected) {
    return false;
  }

  if (_flags._spef != rhs._flags._spef) {
    return false;
  }

  if (_flags._select != rhs._flags._select) {
    return false;
  }

  if (_flags._mark != rhs._flags._mark) {
    return false;
  }

  if (_flags._mark_1 != rhs._flags._mark_1) {
    return false;
  }

  if (_flags._wire_altered != rhs._flags._wire_altered) {
    return false;
  }

  if (_flags._extracted != rhs._flags._extracted) {
    return false;
  }

  if (_flags._rc_graph != rhs._flags._rc_graph) {
    return false;
  }

  if (_flags._set_io != rhs._flags._set_io) {
    return false;
  }

  if (_flags._io != rhs._flags._io) {
    return false;
  }

  if (_flags._dont_touch != rhs._flags._dont_touch) {
    return false;
  }

  if (_flags._fixed_bump != rhs._flags._fixed_bump) {
    return false;
  }

  if (_flags._source != rhs._flags._source) {
    return false;
  }

  if (_flags._rc_disconnected != rhs._flags._rc_disconnected) {
    return false;
  }

  if (_flags._block_rule != rhs._flags._block_rule) {
    return false;
  }

  if (_name && rhs._name) {
    if (strcmp(_name, rhs._name) != 0) {
      return false;
    }
  } else if (_name || rhs._name) {
    return false;
  }

  if (_gndc_calibration_factor != rhs._gndc_calibration_factor) {
    return false;
  }
  if (_cc_calibration_factor != rhs._cc_calibration_factor) {
    return false;
  }

  if (_next_entry != rhs._next_entry) {
    return false;
  }

  if (_iterms != rhs._iterms) {
    return false;
  }

  if (_bterms != rhs._bterms) {
    return false;
  }

  if (_wire != rhs._wire) {
    return false;
  }

  if (_global_wire != rhs._global_wire) {
    return false;
  }

  if (_swires != rhs._swires) {
    return false;
  }

  if (_cap_nodes != rhs._cap_nodes) {
    return false;
  }

  if (_r_segs != rhs._r_segs) {
    return false;
  }

  if (_non_default_rule != rhs._non_default_rule) {
    return false;
  }

  if (_weight != rhs._weight) {
    return false;
  }

  if (_xtalk != rhs._xtalk) {
    return false;
  }

  if (_ccAdjustFactor != rhs._ccAdjustFactor) {
    return false;
  }

  if (_ccAdjustOrder != rhs._ccAdjustOrder) {
    return false;
  }

  if (_groups != rhs._groups) {
    return false;
  }

  if (guides_ != rhs.guides_) {
    return false;
  }

  if (tracks_ != rhs.tracks_) {
    return false;
  }

  return true;
}

////////////////////////////////////////////////////////////////////
//
// dbNet - Methods
//
////////////////////////////////////////////////////////////////////

std::string dbNet::getName()
{
  _dbNet* net = (_dbNet*) this;
  return net->_name;
}

const char* dbNet::getConstName()
{
  _dbNet* net = (_dbNet*) this;
  return net->_name;
}
void dbNet::printNetName(FILE* fp, bool idFlag, bool newLine)
{
  if (idFlag) {
    fprintf(fp, " %d", getId());
  }

  _dbNet* net = (_dbNet*) this;
  fprintf(fp, " %s", net->_name);

  if (newLine) {
    fprintf(fp, "\n");
  }
}
bool dbNet::rename(const char* name)
{
  _dbNet* net = (_dbNet*) this;
  _dbBlock* block = (_dbBlock*) net->getOwner();

  if (block->_net_hash.hasMember(name)) {
    return false;
  }

  block->_net_hash.remove(net);
  free((void*) net->_name);
  net->_name = strdup(name);
  ZALLOCATED(net->_name);
  block->_net_hash.insert(net);

  return true;
}

void dbNet::swapNetNames(dbNet* source, bool ok_to_journal)
{
  _dbNet* dest_net = (_dbNet*) this;
  _dbNet* source_net = (_dbNet*) source;
  _dbBlock* block = (_dbBlock*) source_net->getOwner();

  char* dest_name_ptr = dest_net->_name;
  char* source_name_ptr = source_net->_name;

  // allow undo..
  if (block->_journal && ok_to_journal) {
    block->_journal->beginAction(dbJournal::SWAP_OBJECT);
    // a name
    block->_journal->pushParam(dbNameObj);
    // the type of name swap
    block->_journal->pushParam(dbNetObj);
    // stash the source and dest in that order,
    // let undo reorder
    block->_journal->pushParam(source_net->getId());
    block->_journal->pushParam(dest_net->getId());
    block->_journal->endAction();
  }

  block->_net_hash.remove(dest_net);
  block->_net_hash.remove(source_net);

  // swap names without copy, just swap the pointers
  dest_net->_name = source_name_ptr;
  source_net->_name = dest_name_ptr;

  block->_net_hash.insert(dest_net);
  block->_net_hash.insert(source_net);
}

bool dbNet::isRCDisconnected()
{
  _dbNet* net = (_dbNet*) this;
  return net->_flags._rc_disconnected == 1;
}

void dbNet::setRCDisconnected(bool value)
{
  _dbNet* net = (_dbNet*) this;
  net->_flags._rc_disconnected = value;
}

int dbNet::getWeight()
{
  _dbNet* net = (_dbNet*) this;
  return net->_weight;
}

void dbNet::setWeight(int weight)
{
  _dbNet* net = (_dbNet*) this;
  net->_weight = weight;
}

dbSourceType dbNet::getSourceType()
{
  _dbNet* net = (_dbNet*) this;
  dbSourceType t(net->_flags._source);
  return t;
}

void dbNet::setSourceType(dbSourceType type)
{
  _dbNet* net = (_dbNet*) this;
  net->_flags._source = type;
}

int dbNet::getXTalkClass()
{
  _dbNet* net = (_dbNet*) this;
  return net->_xtalk;
}

void dbNet::setXTalkClass(int value)
{
  _dbNet* net = (_dbNet*) this;
  net->_xtalk = value;
}

float dbNet::getCcAdjustFactor()
{
  _dbNet* net = (_dbNet*) this;
  return net->_ccAdjustFactor;
}

void dbNet::setCcAdjustFactor(float factor)
{
  _dbNet* net = (_dbNet*) this;
  net->_ccAdjustFactor = factor;
}

uint dbNet::getCcAdjustOrder()
{
  _dbNet* net = (_dbNet*) this;
  return net->_ccAdjustOrder;
}

void dbNet::setCcAdjustOrder(uint order)
{
  _dbNet* net = (_dbNet*) this;
  net->_ccAdjustOrder = order;
}

void dbNet::setDrivingITerm(int id)
{
  _dbNet* net = (_dbNet*) this;
  net->_drivingIterm = id;
}
int dbNet::getDrivingITerm()
{
  _dbNet* net = (_dbNet*) this;
  return net->_drivingIterm;
}
bool dbNet::hasFixedBump()
{
  _dbNet* net = (_dbNet*) this;
  return net->_flags._fixed_bump == 1;
}

void dbNet::setFixedBump(bool value)
{
  _dbNet* net = (_dbNet*) this;
  net->_flags._fixed_bump = value;
}

void dbNet::setWireType(dbWireType wire_type)
{
  _dbNet* net = (_dbNet*) this;
  _dbBlock* block = (_dbBlock*) net->getOwner();
  uint prev_flags = flagsToUInt(net);
  net->_flags._wire_type = wire_type.getValue();

  if (block->_journal) {
    debugPrint(getImpl()->getLogger(),
               utl::ODB,
               "DB_ECO",
               1,
               "ECO: net {}, setWireType: {}",
               getId(),
               wire_type.getValue());
    block->_journal->updateField(
        this, _dbNet::FLAGS, prev_flags, flagsToUInt(net));
  }
}

dbWireType dbNet::getWireType()
{
  _dbNet* net = (_dbNet*) this;
  return dbWireType(net->_flags._wire_type);
}

dbSigType dbNet::getSigType()
{
  _dbNet* net = (_dbNet*) this;
  return dbSigType(net->_flags._sig_type);
}

void dbNet::setSigType(dbSigType sig_type)
{
  _dbNet* net = (_dbNet*) this;
  _dbBlock* block = (_dbBlock*) net->getOwner();
  uint prev_flags = flagsToUInt(net);
  net->_flags._sig_type = sig_type.getValue();

  if (block->_journal) {
    debugPrint(getImpl()->getLogger(),
               utl::ODB,
               "DB_ECO",
               1,
               "ECO: net {}, setSigType: {}",
               getId(),
               sig_type.getValue());
    block->_journal->updateField(
        this, _dbNet::FLAGS, prev_flags, flagsToUInt(net));
  }
}

float dbNet::getGndcCalibFactor()
{
  _dbNet* net = (_dbNet*) this;
  return net->_gndc_calibration_factor;
}

void dbNet::setGndcCalibFactor(float gndcCalib)
{
  _dbNet* net = (_dbNet*) this;
  net->_gndc_calibration_factor = gndcCalib;
}

float dbNet::getRefCc()
{
  _dbNet* net = (_dbNet*) this;
  return net->_refCC;
}

void dbNet::setRefCc(float cap)
{
  _dbNet* net = (_dbNet*) this;
  net->_refCC = cap;
}

float dbNet::getCcCalibFactor()
{
  _dbNet* net = (_dbNet*) this;
  return net->_cc_calibration_factor;
}

void dbNet::setCcCalibFactor(float ccCalib)
{
  _dbNet* net = (_dbNet*) this;
  net->_cc_calibration_factor = ccCalib;
}

float dbNet::getDbCc()
{
  _dbNet* net = (_dbNet*) this;
  return net->_dbCC;
}

void dbNet::setDbCc(float cap)
{
  _dbNet* net = (_dbNet*) this;
  net->_dbCC = cap;
}

void dbNet::addDbCc(float cap)
{
  _dbNet* net = (_dbNet*) this;
  net->_dbCC += cap;
}

float dbNet::getCcMatchRatio()
{
  _dbNet* net = (_dbNet*) this;
  return net->_CcMatchRatio;
}

void dbNet::setCcMatchRatio(float ratio)
{
  _dbNet* net = (_dbNet*) this;
  net->_CcMatchRatio = ratio;
}

void dbNet::calibrateCouplingCap(int corner)
{
  float srcnetCcCalibFactor = getCcCalibFactor();
  float factor, tgtnetCcCalibFactor;
  std::vector<dbCCSeg*> ccSet;
  getSrcCCSegs(ccSet);
  std::vector<dbCCSeg*>::iterator cc_itr;
  dbCCSeg* cc;
  for (cc_itr = ccSet.begin(); cc_itr != ccSet.end(); ++cc_itr) {
    cc = *cc_itr;
    tgtnetCcCalibFactor = cc->getTargetNet()->getCcCalibFactor();
    factor = (srcnetCcCalibFactor + tgtnetCcCalibFactor) / 2;
    if (factor == 1.0) {
      continue;
    }
    if (corner < 0) {
      cc->adjustCapacitance(factor);
    } else {
      cc->adjustCapacitance(factor, corner);
    }
  }
}

void dbNet::calibrateCouplingCap()
{
  calibrateCouplingCap(-1);
}

uint dbNet::getRSegCount()
{
  dbSet<dbRSeg> rSet = getRSegs();
  uint cnt = rSet.size();
  return cnt;
}

uint dbNet::maxInternalCapNum()
{
  uint max_n = 0;
  dbSet<dbCapNode> nodeSet = getCapNodes();
  dbSet<dbCapNode>::iterator rc_itr;
  dbCapNode* capn;
  for (rc_itr = nodeSet.begin(); rc_itr != nodeSet.end(); ++rc_itr) {
    capn = *rc_itr;
    if (!capn->isInternal()) {
      continue;
    }

    uint n = capn->getNode();
    if (max_n < n) {
      max_n = n;
    }
  }
  return max_n;
}
void dbNet::collapseInternalCapNum(FILE* mapFile)
{
  dbSet<dbCapNode> nodeSet = getCapNodes();
  uint cnt = 1;
  dbSet<dbCapNode>::iterator rc_itr;
  dbCapNode* capn;
  for (rc_itr = nodeSet.begin(); rc_itr != nodeSet.end(); ++rc_itr) {
    cnt++;
    capn = *rc_itr;
    if (capn->isInternal()) {
      if (mapFile) {
        fprintf(mapFile, "    %8d : %8d\n", capn->getNode(), cnt);
      }
      capn->setNode(cnt);
    }
  }
}

uint dbNet::getCapNodeCount()
{
  dbSet<dbCapNode> nodeSet = getCapNodes();
  uint cnt = nodeSet.size();
  return cnt;
}

uint dbNet::getCcCount()
{
  dbSet<dbCapNode> nodeSet = getCapNodes();
  dbSet<dbCapNode>::iterator rc_itr;
  dbSet<dbCCSeg> ccSegs;
  dbCapNode* node;
  uint count = 0;
  for (rc_itr = nodeSet.begin(); rc_itr != nodeSet.end(); ++rc_itr) {
    node = *rc_itr;
    ccSegs = node->getCCSegs();
    count += ccSegs.size();
  }
  return count;
}

bool dbNet::groundCC(float gndFactor)
{
  dbSet<dbCapNode> nodeSet = getCapNodes();
  dbSet<dbCapNode>::iterator rc_itr;
  dbCapNode* node;
  bool grounded = false;

  for (rc_itr = nodeSet.begin(); rc_itr != nodeSet.end(); ++rc_itr) {
    node = *rc_itr;
    grounded |= node->groundCC(gndFactor);
  }
  return grounded;
}

bool dbNet::adjustCC(uint adjOrder,
                     float adjFactor,
                     double ccThreshHold,
                     std::vector<dbCCSeg*>& adjustedCC,
                     std::vector<dbNet*>& halonets)
{
  if (((_dbNet*) this)->_ccAdjustFactor > 0) {
    getImpl()->getLogger()->warn(
        utl::ODB,
        48,
        "Net {} {} had been CC adjusted by {}. Please unadjust first.",
        getId(),
        getConstName(),
        ((_dbNet*) this)->_ccAdjustFactor);
    return false;
  }
  dbSet<dbCapNode> nodeSet = getCapNodes();
  dbSet<dbCapNode>::iterator rc_itr;
  dbCapNode* node;
  bool needAdjust = false;
  for (rc_itr = nodeSet.begin(); !needAdjust && rc_itr != nodeSet.end();
       ++rc_itr) {
    node = *rc_itr;
    if (node->needAdjustCC(ccThreshHold)) {
      needAdjust = true;
    }
  }
  if (!needAdjust) {
    return false;
  }

  for (rc_itr = nodeSet.begin(); rc_itr != nodeSet.end(); ++rc_itr) {
    node = *rc_itr;
    node->adjustCC(adjOrder, adjFactor, adjustedCC, halonets);
  }
  ((_dbNet*) this)->_ccAdjustFactor = adjFactor;
  ((_dbNet*) this)->_ccAdjustOrder = adjOrder;
  return true;
}

void dbNet::undoAdjustedCC(std::vector<dbCCSeg*>& adjustedCC,
                           std::vector<dbNet*>& halonets)
{
  if (((_dbNet*) this)->_ccAdjustFactor < 0) {
    return;
  }
  uint adjOrder = ((_dbNet*) this)->_ccAdjustOrder;
  float adjFactor = 1 / ((_dbNet*) this)->_ccAdjustFactor;
  dbSet<dbCapNode> nodeSet = getCapNodes();
  dbSet<dbCapNode>::iterator rc_itr;
  dbCapNode* node;
  for (rc_itr = nodeSet.begin(); rc_itr != nodeSet.end(); ++rc_itr) {
    node = *rc_itr;
    node->adjustCC(adjOrder, adjFactor, adjustedCC, halonets);
  }
  ((_dbNet*) this)->_ccAdjustFactor = -1;
  ((_dbNet*) this)->_ccAdjustOrder = 0;
}

void dbNet::adjustNetGndCap(uint corner, float factor)
{
  if (factor == 1.0) {
    return;
  }
  bool foreign = ((dbBlock*) getImpl()->getOwner())->getExtControl()->_foreign;
  if (foreign) {
    dbSet<dbCapNode> nodeSet = getCapNodes();
    dbSet<dbCapNode>::iterator rc_itr;
    dbCapNode* node;
    for (rc_itr = nodeSet.begin(); rc_itr != nodeSet.end(); ++rc_itr) {
      node = *rc_itr;
      node->adjustCapacitance(factor, corner);
    }
  } else {
    dbSet<dbRSeg> rSet = getRSegs();
    dbSet<dbRSeg>::iterator rc_itr;
    dbRSeg* rc;
    for (rc_itr = rSet.begin(); rc_itr != rSet.end(); ++rc_itr) {
      rc = *rc_itr;
      rc->adjustCapacitance(factor, corner);
    }
  }
}
void dbNet::adjustNetGndCap(float factor)
{
  if (factor == 1.0) {
    return;
  }
  bool foreign = ((dbBlock*) getImpl()->getOwner())->getExtControl()->_foreign;
  if (foreign) {
    dbSet<dbCapNode> nodeSet = getCapNodes();
    dbSet<dbCapNode>::iterator rc_itr;
    dbCapNode* node;
    for (rc_itr = nodeSet.begin(); rc_itr != nodeSet.end(); ++rc_itr) {
      node = *rc_itr;
      node->adjustCapacitance(factor);
    }
  } else {
    dbSet<dbRSeg> rSet = getRSegs();
    dbSet<dbRSeg>::iterator rc_itr;
    dbRSeg* rc;
    for (rc_itr = rSet.begin(); rc_itr != rSet.end(); ++rc_itr) {
      rc = *rc_itr;
      rc->adjustCapacitance(factor);
    }
  }
}

void dbNet::calibrateGndCap()
{
  adjustNetGndCap(getGndcCalibFactor());
}

void dbNet::calibrateCapacitance()
{
  calibrateGndCap();
  calibrateCouplingCap();
}
void dbNet::adjustNetRes(float factor, uint corner)
{
  if (factor == 1.0) {
    return;
  }
  dbSet<dbRSeg> rSet = getRSegs();
  dbSet<dbRSeg>::iterator rc_itr;
  dbRSeg* rc = nullptr;
  for (rc_itr = rSet.begin(); rc_itr != rSet.end(); ++rc_itr) {
    rc = *rc_itr;
    rc->adjustResistance(factor, corner);
  }
}
void dbNet::adjustNetRes(float factor)
{
  if (factor == 1.0) {
    return;
  }
  dbSet<dbRSeg> rSet = getRSegs();
  dbSet<dbRSeg>::iterator rc_itr;
  dbRSeg* rc = nullptr;
  for (rc_itr = rSet.begin(); rc_itr != rSet.end(); ++rc_itr) {
    rc = *rc_itr;
    rc->adjustResistance(factor);
  }
}

bool dbNet::isSpef()
{
  _dbNet* net = (_dbNet*) this;
  return net->_flags._spef == 1;
}

void dbNet::setSpef(bool value)
{
  _dbNet* net = (_dbNet*) this;
  _dbBlock* block = (_dbBlock*) net->getOwner();
  uint prev_flags = flagsToUInt(net);
  net->_flags._spef = (value == true) ? 1 : 0;

  if (block->_journal) {
    debugPrint(getImpl()->getLogger(),
               utl::ODB,
               "DB_ECO",
               1,
               "ECO: net {}, setSpef: {}",
               getId(),
               value);
    block->_journal->updateField(
        this, _dbNet::FLAGS, prev_flags, flagsToUInt(net));
  }
}

bool dbNet::isSelect()
{
  _dbNet* net = (_dbNet*) this;
  return net->_flags._select == 1;
}

void dbNet::setSelect(bool value)
{
  _dbNet* net = (_dbNet*) this;
  _dbBlock* block = (_dbBlock*) net->getOwner();
  uint prev_flags = flagsToUInt(net);
  net->_flags._select = (value == true) ? 1 : 0;

  if (block->_journal) {
    debugPrint(getImpl()->getLogger(),
               utl::ODB,
               "DB_ECO",
               1,
               "ECO: net {}, setSelect: {}",
               getId(),
               value);
    block->_journal->updateField(
        this, _dbNet::FLAGS, prev_flags, flagsToUInt(net));
  }
}

bool dbNet::isEnclosed(Rect* bbox)  // assuming no intersection
{
  dbWire* wire = getWire();
  dbWirePathItr pitr;
  dbWirePath path;
  dbWirePathShape pathShape;
  pitr.begin(wire);
  uint cnt = 0;
  while (pitr.getNextPath(path)) {
    if (path.point.getX() > bbox->xMax() || path.point.getX() < bbox->xMin()
        || path.point.getY() > bbox->yMax()
        || path.point.getY() < bbox->yMin()) {
      return false;
    }
    cnt++;
    if (cnt >= 4) {
      return true;
    }
    while (pitr.getNextShape(pathShape)) {
      if (pathShape.point.getX() > bbox->xMax()
          || pathShape.point.getX() < bbox->xMin()
          || pathShape.point.getY() > bbox->yMax()
          || pathShape.point.getY() < bbox->yMin()) {
        return false;
      }
      cnt++;
      if (cnt >= 4) {
        return true;
      }
    }
  }
  return true;
}

bool dbNet::isMarked()
{
  _dbNet* net = (_dbNet*) this;
  return net->_flags._mark == 1;
}

void dbNet::setMark(bool value)
{
  _dbNet* net = (_dbNet*) this;
  _dbBlock* block = (_dbBlock*) net->getOwner();
  uint prev_flags = flagsToUInt(net);
  net->_flags._mark = (value == true) ? 1 : 0;

  if (block->_journal) {
    debugPrint(getImpl()->getLogger(),
               utl::ODB,
               "DB_ECO",
               1,
               "ECO: net {}, setMark: {}",
               getId(),
               value);
    block->_journal->updateField(
        this, _dbNet::FLAGS, prev_flags, flagsToUInt(net));
  }
}

bool dbNet::isMark_1ed()
{
  _dbNet* net = (_dbNet*) this;
  return net->_flags._mark_1 == 1;
}

void dbNet::setMark_1(bool value)
{
  _dbNet* net = (_dbNet*) this;
  _dbBlock* block = (_dbBlock*) net->getOwner();
  uint prev_flags = flagsToUInt(net);
  net->_flags._mark_1 = (value == true) ? 1 : 0;

  if (block->_journal) {
    debugPrint(getImpl()->getLogger(),
               utl::ODB,
               "DB_ECO",
               1,
               "ECO: net {}, setMark_1: {}",
               getId(),
               value);
    block->_journal->updateField(
        this, _dbNet::FLAGS, prev_flags, flagsToUInt(net));
  }
}

bool dbNet::isWireOrdered()
{
  _dbNet* net = (_dbNet*) this;
  return net->_flags._wire_ordered == 1;
}

void dbNet::setWireOrdered(bool value)
{
  _dbNet* net = (_dbNet*) this;

  _dbBlock* block = (_dbBlock*) net->getOwner();
  uint prev_flags = flagsToUInt(net);

  net->_flags._wire_ordered = (value == true) ? 1 : 0;

  if (block->_journal) {
    debugPrint(getImpl()->getLogger(),
               utl::ODB,
               "DB_ECO",
               1,
               "ECO: net {}, setWireOrdered: {}",
               getId(),
               value);
    block->_journal->updateField(
        this, _dbNet::FLAGS, prev_flags, flagsToUInt(net));
  }
}

bool dbNet::isDisconnected()
{
  _dbNet* net = (_dbNet*) this;
  return net->_flags._disconnected == 1;
}

void dbNet::setDisconnected(bool value)
{
  _dbNet* net = (_dbNet*) this;

  _dbBlock* block = (_dbBlock*) net->getOwner();
  uint prev_flags = flagsToUInt(net);

  net->_flags._disconnected = (value == true) ? 1 : 0;

  if (block->_journal) {
    debugPrint(getImpl()->getLogger(),
               utl::ODB,
               "DB_ECO",
               1,
               "ECO: net {}, setDisconnected: {}",
               getId(),
               value);
    block->_journal->updateField(
        this, _dbNet::FLAGS, prev_flags, flagsToUInt(net));
  }
}

void dbNet::setWireAltered(bool value)
{
  _dbNet* net = (_dbNet*) this;

  _dbBlock* block = (_dbBlock*) net->getOwner();
  uint prev_flags = flagsToUInt(net);

  net->_flags._wire_altered = (value == true) ? 1 : 0;
  if (value) {
    net->_flags._wire_ordered = 0;
  }

  if (block->_journal) {
    debugPrint(getImpl()->getLogger(),
               utl::ODB,
               "DB_ECO",
               1,
               "ECO: net {}, setWireAltered: {}",
               getId(),
               value);
    block->_journal->updateField(
        this, _dbNet::FLAGS, prev_flags, flagsToUInt(net));
  }
}

bool dbNet::isWireAltered()
{
  _dbNet* net = (_dbNet*) this;
  return net->_flags._wire_altered == 1;
}

void dbNet::setExtracted(bool value)
{
  _dbNet* net = (_dbNet*) this;

  _dbBlock* block = (_dbBlock*) net->getOwner();
  uint prev_flags = flagsToUInt(net);

  net->_flags._extracted = (value == true) ? 1 : 0;

  if (block->_journal) {
    debugPrint(getImpl()->getLogger(),
               utl::ODB,
               "DB_ECO",
               1,
               "ECO: net {}, setExtracted: {}",
               getId(),
               value);
    block->_journal->updateField(
        this, _dbNet::FLAGS, prev_flags, flagsToUInt(net));
  }
}

bool dbNet::isExtracted()
{
  _dbNet* net = (_dbNet*) this;
  return net->_flags._extracted == 1;
}

void dbNet::setRCgraph(bool value)
{
  _dbNet* net = (_dbNet*) this;

  _dbBlock* block = (_dbBlock*) net->getOwner();
  uint prev_flags = flagsToUInt(net);

  net->_flags._rc_graph = (value == true) ? 1 : 0;

  if (block->_journal) {
    debugPrint(getImpl()->getLogger(),
               utl::ODB,
               "DB_ECO",
               1,
               "ECO: net {}, setRCgraph: {}",
               getId(),
               value);
    block->_journal->updateField(
        this, _dbNet::FLAGS, prev_flags, flagsToUInt(net));
  }
}

bool dbNet::isRCgraph()
{
  _dbNet* net = (_dbNet*) this;
  return net->_flags._rc_graph == 1;
}

dbBlock* dbNet::getBlock()
{
  return (dbBlock*) getImpl()->getOwner();
}

dbSet<dbITerm> dbNet::getITerms()
{
  _dbNet* net = (_dbNet*) this;
  _dbBlock* block = (_dbBlock*) net->getOwner();
  return dbSet<dbITerm>(net, block->_net_iterm_itr);
}

dbITerm* dbNet::get1stITerm()
{
  dbSet<dbITerm> iterms = getITerms();

  dbITerm* it = nullptr;
  dbSet<dbITerm>::iterator iitr = iterms.begin();
  if (iitr != iterms.end()) {
    it = *iitr;
  }
  return it;
}

dbSet<dbBTerm> dbNet::getBTerms()
{
  _dbNet* net = (_dbNet*) this;
  _dbBlock* block = (_dbBlock*) net->getOwner();
  return dbSet<dbBTerm>(net, block->_net_bterm_itr);
}

dbBTerm* dbNet::get1stBTerm()
{
  dbSet<dbBTerm> bterms = getBTerms();

  dbBTerm* bt = nullptr;
  dbSet<dbBTerm>::iterator bitr = bterms.begin();
  if (bitr != bterms.end()) {
    bt = *bitr;
  }
  return bt;
}
dbITerm* dbNet::getFirstOutput()
{
  if (getDrivingITerm() > 0) {
    return dbITerm::getITerm((dbBlock*) getImpl()->getOwner(),
                             getDrivingITerm());
  }

  dbSet<dbITerm> iterms = getITerms();
  dbSet<dbITerm>::iterator iitr;

  for (iitr = iterms.begin(); iitr != iterms.end(); ++iitr) {
    dbITerm* tr = *iitr;

    if ((tr->getSigType() == dbSigType::GROUND)
        || (tr->getSigType() == dbSigType::POWER)) {
      continue;
    }

    if (tr->isClocked()) {
      continue;
    }

    if (tr->getIoType() != dbIoType::OUTPUT) {
      continue;
    }

    return tr;
  }
  // warning(0, "instance %s has no output pin\n", getConstName());
  return nullptr;
}
dbITerm* dbNet::get1stSignalInput(bool io)
{
  dbSet<dbITerm> iterms = getITerms();
  dbSet<dbITerm>::iterator iitr;

  for (iitr = iterms.begin(); iitr != iterms.end(); ++iitr) {
    dbITerm* tr = *iitr;

    if ((tr->getSigType() == dbSigType::GROUND)
        || (tr->getSigType() == dbSigType::POWER)) {
      continue;
    }

    if (tr->getIoType() != dbIoType::INPUT) {
      continue;
    }

    if (io && (tr->getIoType() != dbIoType::INOUT)) {
      continue;
    }

    return tr;
  }
  // warning(0, "instance %s has no output pin\n", getConstName());
  return nullptr;
}

dbSet<dbSWire> dbNet::getSWires()
{
  _dbNet* net = (_dbNet*) this;
  _dbBlock* block = (_dbBlock*) net->getOwner();
  return dbSet<dbSWire>(net, block->_swire_itr);
}
dbSWire*  // Dimitris 9/11/07
dbNet::getFirstSWire()
{
  _dbNet* net = (_dbNet*) this;
  _dbBlock* block = (_dbBlock*) net->getOwner();

  if (net->_swires == 0) {
    return nullptr;
  }

  return (dbSWire*) block->_swire_tbl->getPtr(net->_swires);
}

dbWire* dbNet::getWire()
{
  _dbNet* net = (_dbNet*) this;
  _dbBlock* block = (_dbBlock*) net->getOwner();

  if (net->_wire == 0) {
    return nullptr;
  }

  return (dbWire*) block->_wire_tbl->getPtr(net->_wire);
}

dbWire* dbNet::getGlobalWire()
{
  _dbNet* net = (_dbNet*) this;
  _dbBlock* block = (_dbBlock*) net->getOwner();

  if (net->_global_wire == 0) {
    return nullptr;
  }

  return (dbWire*) block->_wire_tbl->getPtr(net->_global_wire);
}

bool dbNet::setIOflag()
{
  _dbNet* net = (_dbNet*) this;
  _dbBlock* block = (_dbBlock*) net->getOwner();
  uint prev_flags = flagsToUInt(net);
  net->_flags._set_io = 1;
  net->_flags._io = 0;
  dbSet<dbBTerm> bterms = getBTerms();
  uint n = bterms.size();

  if (n > 0) {
    net->_flags._io = 1;

    if (block->_journal) {
      debugPrint(getImpl()->getLogger(),
                 utl::ODB,
                 "DB_ECO",
                 1,
                 "ECO: net {}, setIOFlag",
                 getId());
      block->_journal->updateField(
          this, _dbNet::FLAGS, prev_flags, flagsToUInt(net));
    }

    return true;
  }

  if (block->_journal) {
    debugPrint(getImpl()->getLogger(),
               utl::ODB,
               "DB_ECO",
               1,
               "ECO: net {}, setIOFlag",
               getId());
    block->_journal->updateField(
        this, _dbNet::FLAGS, prev_flags, flagsToUInt(net));
  }

  return false;
}
bool dbNet::isIO()
{
  _dbNet* net = (_dbNet*) this;

  if (net->_flags._set_io > 0) {
    return net->_flags._io == 1;
  }
  return setIOflag();
}

void dbNet::setDoNotTouch(bool v)
{
  _dbNet* net = (_dbNet*) this;
  net->_flags._dont_touch = v;
}

bool dbNet::isDoNotTouch()
{
  _dbNet* net = (_dbNet*) this;
  return net->_flags._dont_touch == 1;
}

bool dbNet::isSpecial()
{
  _dbNet* net = (_dbNet*) this;
  return net->_flags._special == 1;
}

void dbNet::setSpecial()
{
  _dbNet* net = (_dbNet*) this;

  _dbBlock* block = (_dbBlock*) net->getOwner();
  uint prev_flags = flagsToUInt(net);

  net->_flags._special = 1;

  if (block->_journal) {
    debugPrint(getImpl()->getLogger(),
               utl::ODB,
               "DB_ECO",
               1,
               "ECO: net {}, setSpecial",
               getId());
    block->_journal->updateField(
        this, _dbNet::FLAGS, prev_flags, flagsToUInt(net));
  }
}

void dbNet::clearSpecial()
{
  _dbNet* net = (_dbNet*) this;

  _dbBlock* block = (_dbBlock*) net->getOwner();
  uint prev_flags = flagsToUInt(net);

  net->_flags._special = 0;

  if (block->_journal) {
    debugPrint(getImpl()->getLogger(),
               utl::ODB,
               "DB_ECO",
               1,
               "ECO: net {}, clearSpecial",
               getId());
    block->_journal->updateField(
        this, _dbNet::FLAGS, prev_flags, flagsToUInt(net));
  }
}

bool dbNet::isConnectedByAbutment()
{
  if (getITermCount() > 2 || getBTermCount() > 0) {
    return false;
  }

  bool first_mterm = true;
  std::vector<Rect> first_pin_boxes;
  for (dbITerm* iterm : getITerms()) {
    dbMTerm* mterm = iterm->getMTerm();
    dbMaster* master = mterm->getMaster();
    if (!master->isBlock()) {
      return false;
    }

    dbInst* inst = iterm->getInst();
    if (inst->isPlaced()) {
      const dbTransform transform = inst->getTransform();

      for (dbMPin* mpin : mterm->getMPins()) {
        for (dbBox* box : mpin->getGeometry()) {
          dbTechLayer* tech_layer = box->getTechLayer();
          if (tech_layer->getType() != dbTechLayerType::ROUTING) {
            continue;
          }
          odb::Rect rect = box->getBox();
          transform.apply(rect);
          if (first_mterm) {
            first_pin_boxes.push_back(rect);
          } else {
            for (const Rect& first_pin_box : first_pin_boxes) {
              if (rect.intersects(first_pin_box)) {
                return true;
              }
            }
          }
        }
      }
    }
    first_mterm = false;
  }

  return false;
}

bool dbNet::isWildConnected()
{
  _dbNet* net = (_dbNet*) this;
  return net->_flags._wild_connect == 1;
}

void dbNet::setWildConnected()
{
  _dbNet* net = (_dbNet*) this;

  _dbBlock* block = (_dbBlock*) net->getOwner();
  uint prev_flags = flagsToUInt(net);
  // uint prev_flags = flagsToUInt(net);

  net->_flags._wild_connect = 1;

  if (block->_journal) {
    debugPrint(getImpl()->getLogger(),
               utl::ODB,
               "DB_ECO",
               1,
               "ECO: net {}, setWildConnected",
               getId());
    block->_journal->updateField(
        this, _dbNet::FLAGS, prev_flags, flagsToUInt(net));
  }
}

void dbNet::clearWildConnected()
{
  _dbNet* net = (_dbNet*) this;

  _dbBlock* block = (_dbBlock*) net->getOwner();
  uint prev_flags = flagsToUInt(net);
  // uint prev_flags = flagsToUInt(net);

  net->_flags._wild_connect = 0;

  if (block->_journal) {
    debugPrint(getImpl()->getLogger(),
               utl::ODB,
               "DB_ECO",
               1,
               "ECO: net {}, clearWildConnected",
               getId());
    block->_journal->updateField(
        this, _dbNet::FLAGS, prev_flags, flagsToUInt(net));
  }
}

dbSet<dbRSeg> dbNet::getRSegs()
{
  _dbNet* net = (_dbNet*) this;
  _dbBlock* block = (_dbBlock*) net->getOwner();
  return dbSet<dbRSeg>(net, block->_r_seg_itr);
}

void dbNet::reverseRSegs()
{
  dbSet<dbRSeg> rSet = getRSegs();
  rSet.reverse();
  _dbBlock* block = (_dbBlock*) getImpl()->getOwner();
  if (block->_journal) {
    debugPrint(getImpl()->getLogger(),
               utl::ODB,
               "DB_ECO",
               1,
               "ECO: dbNet {}, reverse rsegs sequence",
               getId());
    block->_journal->beginAction(dbJournal::UPDATE_FIELD);
    block->_journal->pushParam(dbNetObj);
    block->_journal->pushParam(getId());
    block->_journal->pushParam(_dbNet::REVERSE_RSEG);
    block->_journal->endAction();
  }
}

dbRSeg* dbNet::findRSeg(uint srcn, uint tgtn)
{
  dbSet<dbRSeg> segs = getRSegs();
  dbSet<dbRSeg>::iterator sitr;

  dbRSeg* rseg;
  for (sitr = segs.begin(); sitr != segs.end(); sitr++) {
    rseg = *sitr;
    if (rseg->getSourceNode() == srcn && rseg->getTargetNode() == tgtn) {
      return rseg;
    }
  }
  return nullptr;
}

void dbNet::set1stRSegId(uint rid)
{
  _dbNet* net = (_dbNet*) this;
  _dbBlock* block = (_dbBlock*) net->getOwner();
  uint pid = net->_r_segs;
  net->_r_segs = rid;
  if (block->_journal) {
    debugPrint(getImpl()->getLogger(),
               utl::ODB,
               "DB_ECO",
               1,
               "ECO: dbNet {}, set 1stRSegNode {}",
               getId(),
               rid);
    block->_journal->beginAction(dbJournal::UPDATE_FIELD);
    block->_journal->pushParam(dbNetObj);
    block->_journal->pushParam(getId());
    block->_journal->pushParam(_dbNet::HEAD_RSEG);
    block->_journal->pushParam(pid);
    block->_journal->pushParam(rid);
    block->_journal->endAction();
  }
}

uint dbNet::get1stRSegId()
{
  _dbNet* net = (_dbNet*) this;
  return net->_r_segs;
}

dbRSeg* dbNet::getZeroRSeg()
{
  _dbNet* net = (_dbNet*) this;
  if (net->_r_segs == 0) {
    return nullptr;
  }
  dbRSeg* zrc = dbRSeg::getRSeg((dbBlock*) net->getOwner(), net->_r_segs);
  return zrc;
}

dbCapNode* dbNet::findCapNode(uint nodeId)
{
  dbSet<dbCapNode> capNodes = getCapNodes();
  dbSet<dbCapNode>::iterator itr;

  for (itr = capNodes.begin(); itr != capNodes.end(); ++itr) {
    dbCapNode* n = *itr;

    if (n->getNode() == nodeId) {
      return n;
    }
  }

  return nullptr;
}

dbSet<dbCapNode> dbNet::getCapNodes()
{
  _dbNet* net = (_dbNet*) this;
  _dbBlock* block = (_dbBlock*) net->getOwner();
  return dbSet<dbCapNode>(net, block->_cap_node_itr);
}

void dbNet::setTermExtIds(int capId)  // 1: capNodeId, 0: reset
{
  dbSet<dbCapNode> nodeSet = getCapNodes();
  dbSet<dbCapNode>::iterator rc_itr;
  _dbBlock* block = (_dbBlock*) getImpl()->getOwner();

  if (block->_journal) {
    if (capId) {
      debugPrint(getImpl()->getLogger(),
                 utl::ODB,
                 "DB_ECO",
                 1,
                 "ECO: set net {} term extId",
                 getId());
    } else
      debugPrint(getImpl()->getLogger(),
                 utl::ODB,
                 "DB_ECO",
                 1,
                 "ECO: reset net {} term extId",
                 getId());
    block->_journal->beginAction(dbJournal::UPDATE_FIELD);
    block->_journal->pushParam(dbNetObj);
    block->_journal->pushParam(getId());
    block->_journal->pushParam(_dbNet::TERM_EXTID);
    block->_journal->pushParam(capId);
    block->_journal->endAction();
  }

  for (rc_itr = nodeSet.begin(); rc_itr != nodeSet.end(); ++rc_itr) {
    dbCapNode* capNode = *rc_itr;
    if (capNode->isBTerm()) {
      uint nodeId = capNode->getNode();
      dbBTerm* bterm = dbBTerm::getBTerm((dbBlock*) block, nodeId);
      uint extId = capId ? capNode->getId() : 0;
      bterm->setExtId(extId);
      continue;
    }

    if (capNode->isITerm()) {
      uint nodeId = capNode->getNode();
      dbITerm* iterm = dbITerm::getITerm((dbBlock*) block, nodeId);
      uint extId = capId ? capNode->getId() : 0;
      iterm->setExtId(extId);
    }
  }
}
void dbNet::set1stCapNodeId(uint cid)
{
  _dbNet* net = (_dbNet*) this;
  _dbBlock* block = (_dbBlock*) net->getOwner();
  uint pid = net->_cap_nodes;
  net->_cap_nodes = cid;
  if (block->_journal) {
    debugPrint(getImpl()->getLogger(),
               utl::ODB,
               "DB_ECO",
               1,
               "ECO: dbNet {}, set 1stCapNode {}",
               getId(),
               cid);
    block->_journal->beginAction(dbJournal::UPDATE_FIELD);
    block->_journal->pushParam(dbNetObj);
    block->_journal->pushParam(getId());
    block->_journal->pushParam(_dbNet::HEAD_CAPNODE);
    block->_journal->pushParam(pid);
    block->_journal->pushParam(cid);
    block->_journal->endAction();
  }
}

uint dbNet::get1stCapNodeId()
{
  _dbNet* net = (_dbNet*) this;
  return net->_cap_nodes;
}

void dbNet::reverseCCSegs()
{
  dbSet<dbCapNode> nodes = getCapNodes();
  dbSet<dbCapNode>::iterator itr;

  for (itr = nodes.begin(); itr != nodes.end(); ++itr) {
    dbCapNode* node = *itr;
    node->getCCSegs().reverse();
  }
}

void dbNet::getSrcCCSegs(std::vector<dbCCSeg*>& S)
{
  dbSet<dbCapNode> nodes = getCapNodes();
  dbSet<dbCapNode>::iterator itr;

  for (itr = nodes.begin(); itr != nodes.end(); ++itr) {
    dbCapNode* node = *itr;
    uint cap_id = node->getImpl()->getOID();
    dbSet<dbCCSeg> segs = node->getCCSegs();
    dbSet<dbCCSeg>::iterator sitr;

    for (sitr = segs.begin(); sitr != segs.end(); ++sitr) {
      _dbCCSeg* seg = (_dbCCSeg*) *sitr;
      if (seg->_cap_node[0] == cap_id) {
        S.push_back((dbCCSeg*) seg);
      }
    }
  }
}

void dbNet::getTgtCCSegs(std::vector<dbCCSeg*>& S)
{
  dbSet<dbCapNode> nodes = getCapNodes();
  dbSet<dbCapNode>::iterator itr;

  for (itr = nodes.begin(); itr != nodes.end(); ++itr) {
    dbCapNode* node = *itr;
    uint cap_id = node->getImpl()->getOID();
    dbSet<dbCCSeg> segs = node->getCCSegs();
    dbSet<dbCCSeg>::iterator sitr;

    for (sitr = segs.begin(); sitr != segs.end(); ++sitr) {
      _dbCCSeg* seg = (_dbCCSeg*) *sitr;
      if (seg->_cap_node[1] == cap_id) {
        S.push_back((dbCCSeg*) seg);
      }
    }
  }
}

/*
void
dbNet::unlinkCapNodes()
{
    _dbNet * net = (_dbNet *) this;
    net->_cap_nodes = 0;
}
*/

void dbNet::destroyCapNodes(bool cleanExtid)
{
  dbBlock* block = (dbBlock*) getImpl()->getOwner();
  dbSet<dbCapNode> cap_nodes = getCapNodes();
  dbSet<dbCapNode>::iterator itr;

  for (itr = cap_nodes.begin(); itr != cap_nodes.end();) {
    dbCapNode* cn = *itr;
    uint oid = cn->getNode();

    if (cleanExtid) {
      if (cn->isITerm()) {
        (dbITerm::getITerm(block, oid))->setExtId(0);

      } else if (cn->isBTerm()) {
        (dbBTerm::getBTerm(block, oid))->setExtId(0);
      }
    }

    itr = dbCapNode::destroy(itr);
  }
}

void dbNet::destroyRSegs()
{
  dbSet<dbRSeg> segs = getRSegs();
  dbSet<dbRSeg>::iterator sitr;

  for (sitr = segs.begin(); sitr != segs.end();) {
    sitr = dbRSeg::destroy(sitr);
  }

  dbRSeg* zrc = getZeroRSeg();
  if (zrc) {
    dbRSeg::destroy(zrc);
  }
}

void dbNet::destroyCCSegs()
{
  dbSet<dbCapNode> capNodes = getCapNodes();
  dbSet<dbCapNode>::iterator citr;

  for (citr = capNodes.begin(); citr != capNodes.end(); ++citr) {
    dbCapNode* n = *citr;
    dbSet<dbCCSeg> ccSegs = n->getCCSegs();
    dbSet<dbCCSeg>::iterator ccitr;

    for (ccitr = ccSegs.begin();
         ccitr != ccSegs.end();)  // ++ccitr here after destroy(cc) would crash
    {
      dbCCSeg* cc = *ccitr;
      ++ccitr;
      dbCCSeg::destroy(cc);
    }
  }
}

void dbNet::getCouplingNets(uint corner,
                            double ccThreshold,
                            std::set<dbNet*>& cnets)
{
  dbSet<dbCapNode> capNodes = getCapNodes();
  dbSet<dbCapNode>::iterator citr;
  std::vector<dbNet*> inets;
  std::vector<double> netccap;
  dbNet* tnet;
  double cccap;
  uint ii;

  for (citr = capNodes.begin(); citr != capNodes.end(); ++citr) {
    dbCapNode* n = *citr;
    dbSet<dbCCSeg> ccSegs = n->getCCSegs();
    dbSet<dbCCSeg>::iterator ccitr;

    for (ccitr = ccSegs.begin(); ccitr != ccSegs.end(); ++ccitr) {
      dbCCSeg* cc = *ccitr;
      cccap = cc->getCapacitance(corner);
      tnet = cc->getSourceCapNode()->getNet();
      if (tnet == this) {
        tnet = cc->getTargetCapNode()->getNet();
      }
      if (tnet->isMarked()) {
        for (ii = 0; ii < inets.size(); ii++) {
          if (inets[ii] == tnet) {
            netccap[ii] += cccap;
            break;
          }
        }
        continue;
      }
      netccap.push_back(cccap);
      inets.push_back(tnet);
      tnet->setMark(true);
    }
  }
  for (ii = 0; ii < inets.size(); ii++) {
    if (netccap[ii] >= ccThreshold && cnets.find(inets[ii]) == cnets.end()) {
      cnets.insert(inets[ii]);
    }
    inets[ii]->setMark(false);
  }
}

void dbNet::getGndTotalCap(double* gndcap, double* totalcap, double mcf)
{
  dbSigType type = getSigType();
  if ((type == dbSigType::POWER) || (type == dbSigType::GROUND)) {
    return;
  }
  dbSet<dbRSeg> rSet = getRSegs();
  if (rSet.begin() == rSet.end()) {
    getImpl()->getLogger()->warn(utl::ODB,
                                 52,
                                 "Net {}, {} has no extraction data",
                                 getId(),
                                 getConstName());
    return;
  }
  bool foreign = ((dbBlock*) getImpl()->getOwner())->getExtControl()->_foreign;
  bool first = true;
  if (foreign) {
    dbSet<dbCapNode> nodeSet = getCapNodes();
    dbSet<dbCapNode>::iterator rc_itr;
    dbCapNode* node;
    for (rc_itr = nodeSet.begin(); rc_itr != nodeSet.end(); ++rc_itr) {
      node = *rc_itr;
      if (first) {
        node->getGndTotalCap(gndcap, totalcap, mcf);
      } else {
        node->addGndTotalCap(gndcap, totalcap, mcf);
      }
      first = false;
    }
  } else {
    dbRSeg* rc;
    dbSet<dbRSeg>::iterator rc_itr;
    for (rc_itr = rSet.begin(); rc_itr != rSet.end(); ++rc_itr) {
      rc = *rc_itr;
      if (first) {
        rc->getGndTotalCap(gndcap, totalcap, mcf);
      } else {
        rc->addGndTotalCap(gndcap, totalcap, mcf);
      }
      first = false;
    }
  }
}

void dbNet::preExttreeMergeRC(double max_cap, uint corner)
{
  dbBlock* block = (dbBlock*) (getImpl()->getOwner());
  double totalcap[ADS_MAX_CORNER];
  dbCapNode* tgtNode;
  std::vector<dbRSeg*> mrsegs;
  dbSigType type = getSigType();
  if ((type == dbSigType::POWER) || (type == dbSigType::GROUND)) {
    return;
  }
  dbSet<dbRSeg> rSet = getRSegs();
  if (rSet.begin() == rSet.end()) {
    getImpl()->getLogger()->warn(utl::ODB,
                                 53,
                                 "Net {}, {} has no extraction data",
                                 getId(),
                                 getConstName());
    return;
  }
  dbRSeg* prc = getZeroRSeg();
  dbRSeg* rc;
  bool firstRC = true;
  uint cnt = 1;
  prc->getGndTotalCap(nullptr, &totalcap[0], 1 /*mcf*/);
  dbSet<dbRSeg>::iterator rc_itr;
  for (rc_itr = rSet.begin(); rc_itr != rSet.end(); ++rc_itr) {
    rc = *rc_itr;
    mrsegs.push_back(rc);
    if (firstRC && cnt != 1) {
      rc->getGndTotalCap(nullptr, &totalcap[0], 1 /*mcf*/);
    } else {
      rc->addGndTotalCap(nullptr, &totalcap[0], 1 /*mcf*/);
    }
    cnt++;
    firstRC = false;
    tgtNode = dbCapNode::getCapNode(block, rc->getTargetNode());
    if (rc->getSourceNode() == rc->getTargetNode()) {
      continue;
    }
    if (!tgtNode->isTreeNode() && totalcap[corner] <= max_cap
        && !tgtNode->isDangling()) {
      continue;
    }
    prc = rc;
    mrsegs.clear();
    firstRC = true;
  }
}

void dbNet::destroyParasitics()
{
  dbBlock* block = (dbBlock*) getImpl()->getOwner();
  std::vector<dbNet*> nets;
  nets.push_back(this);
  block->destroyParasitics(nets);
}

double dbNet::getTotalCouplingCap(uint corner)
{
  double cap = 0.0;
  dbSet<dbCapNode> capNodes = getCapNodes();
  dbSet<dbCapNode>::iterator citr;

  for (citr = capNodes.begin(); citr != capNodes.end(); ++citr) {
    dbCapNode* n = *citr;
    dbSet<dbCCSeg> ccSegs = n->getCCSegs();
    dbSet<dbCCSeg>::iterator ccitr;

    for (ccitr = ccSegs.begin(); ccitr != ccSegs.end(); ++ccitr) {
      dbCCSeg* cc = *ccitr;
      cap += cc->getCapacitance(corner);
    }
  }

  return cap;
}

double dbNet::getTotalCapacitance(uint corner, bool cc)
{
  double cap = 0.0;
  double cap1 = 0.0;
  bool foreign = ((dbBlock*) getImpl()->getOwner())->getExtControl()->_foreign;

  if (foreign) {
    dbSet<dbCapNode> nodeSet = getCapNodes();
    dbSet<dbCapNode>::iterator rc_itr;
    dbCapNode* node;
    for (rc_itr = nodeSet.begin(); rc_itr != nodeSet.end(); ++rc_itr) {
      node = *rc_itr;
      cap1 = node->getCapacitance(corner);
      cap += cap1;
    }
  } else {
    dbSet<dbRSeg> rSet = getRSegs();
    dbSet<dbRSeg>::iterator rc_itr;
    dbRSeg* rc;
    for (rc_itr = rSet.begin(); rc_itr != rSet.end(); ++rc_itr) {
      rc = *rc_itr;
      cap1 = rc->getCapacitance(corner);
      cap += cap1;
    }
  }
  if (cc) {
    cap += getTotalCouplingCap(corner);
  }
  return cap;
}

double dbNet::getTotalResistance(uint corner)
{
  dbSet<dbRSeg> rSet = this->getRSegs();
  dbSet<dbRSeg>::iterator rc_itr;

  double cap = 0.0;

  for (rc_itr = rSet.begin(); rc_itr != rSet.end(); ++rc_itr) {
    dbRSeg* rc = *rc_itr;

    cap += rc->getResistance(corner);
  }
  return cap;
}

void dbNet::setNonDefaultRule(dbTechNonDefaultRule* rule)
{
  _dbNet* net = (_dbNet*) this;
  _dbBlock* block = (_dbBlock*) net->getOwner();
  uint prev_rule = net->_non_default_rule;
  bool prev_block_rule = net->_flags._block_rule;

  if (rule == nullptr) {
    net->_non_default_rule = 0U;
    net->_flags._block_rule = 0;
  } else {
    net->_non_default_rule = rule->getImpl()->getOID();
    net->_flags._block_rule = rule->isBlockRule();
  }

  if (block->_journal) {
    debugPrint(getImpl()->getLogger(),
               utl::ODB,
               "DB_ECO",
               1,
               "ECO: net {}, setNonDefaultRule: ",
               getId(),
               (rule) ? rule->getImpl()->getOID() : 0);
    // block->_journal->updateField(this, _dbNet::NON_DEFAULT_RULE, prev_rule,
    // net->_non_default_rule );
    block->_journal->beginAction(dbJournal::UPDATE_FIELD);
    block->_journal->pushParam(rule->getObjectType());
    block->_journal->pushParam(rule->getId());
    block->_journal->pushParam(_dbNet::NON_DEFAULT_RULE);
    block->_journal->pushParam(prev_rule);
    block->_journal->pushParam((uint) net->_non_default_rule);
    block->_journal->pushParam(prev_block_rule);
    block->_journal->pushParam((bool) net->_flags._block_rule);
    block->_journal->endAction();
  }
}

dbTechNonDefaultRule* dbNet::getNonDefaultRule()
{
  _dbNet* net = (_dbNet*) this;

  if (net->_non_default_rule == 0) {
    return nullptr;
  }

  dbDatabase* db = (dbDatabase*) net->getDatabase();

  if (net->_flags._block_rule) {
    _dbBlock* block = (_dbBlock*) net->getOwner();
    return (dbTechNonDefaultRule*) block->_non_default_rule_tbl->getPtr(
        net->_non_default_rule);
  }

  _dbTech* tech = (_dbTech*) db->getTech();
  return (dbTechNonDefaultRule*) tech->_non_default_rule_tbl->getPtr(
      net->_non_default_rule);
}

void dbNet::getSignalWireCount(uint& wireCnt, uint& viaCnt)
{
  dbWirePath path;
  dbWirePathShape pshape;
  dbWire* wire = getWire();
  if (wire == nullptr) {
    return;
  }
  dbWirePathItr pitr;
  for (pitr.begin(wire); pitr.getNextPath(path);) {
    while (pitr.getNextShape(pshape)) {
      if (pshape.shape.isVia()) {
        viaCnt++;
      } else {
        wireCnt++;
      }
    }
  }
}
void dbNet::getNetStats(uint& wireCnt,
                        uint& viaCnt,
                        uint& len,
                        uint& layerCnt,
                        uint* levelTable)
{
  len = 0;
  wireCnt = 0;
  viaCnt = 0;
  layerCnt = 0;
  dbWirePath path;
  dbWirePathShape pshape;
  dbWire* wire = getWire();
  if (wire == nullptr) {
    return;
  }
  dbWirePathItr pitr;
  for (pitr.begin(wire); pitr.getNextPath(path);) {
    while (pitr.getNextShape(pshape)) {
      if (pshape.shape.isVia()) {
        viaCnt++;
        continue;
      }
      wireCnt++;

      uint level = pshape.shape.getTechLayer()->getRoutingLevel();
      if (levelTable) {
        levelTable[level]++;
      }
      len += std::max(pshape.shape.xMax() - pshape.shape.xMin(),
                      pshape.shape.yMax() - pshape.shape.yMin());
    }
  }
}
void dbNet::getPowerWireCount(uint& wireCnt, uint& viaCnt)
{
  dbSet<dbSWire> swires = getSWires();
  dbSet<dbSWire>::iterator itr;
  for (itr = swires.begin(); itr != swires.end(); ++itr) {
    dbSWire* swire = *itr;
    dbSet<dbSBox> wires = swire->getWires();
    dbSet<dbSBox>::iterator box_itr;
    for (box_itr = wires.begin(); box_itr != wires.end(); ++box_itr) {
      dbSBox* s = *box_itr;
      if (s->isVia()) {
        viaCnt++;
      } else {
        wireCnt++;
      }
    }
  }
}

void dbNet::getWireCount(uint& wireCnt, uint& viaCnt)
{
  if (getSigType() == dbSigType::POWER || getSigType() == dbSigType::GROUND) {
    getPowerWireCount(wireCnt, viaCnt);
  } else {
    getSignalWireCount(wireCnt, viaCnt);
  }
}

uint dbNet::getITermCount()
{
  uint itc = 0;
  dbSet<dbITerm> iterms = getITerms();
  dbSet<dbITerm>::iterator iterm_itr;
  for (iterm_itr = iterms.begin(); iterm_itr != iterms.end(); ++iterm_itr) {
    itc++;
  }
  return itc;
}

uint dbNet::getBTermCount()
{
  uint btc = 0;
  dbSet<dbBTerm> bterms = getBTerms();
  dbSet<dbBTerm>::iterator bterm_itr;
  for (bterm_itr = bterms.begin(); bterm_itr != bterms.end(); ++bterm_itr) {
    btc++;
  }
  return btc;
}

uint dbNet::getTermCount()
{
  uint itc = getITermCount();
  uint btc = getBTermCount();
  return itc + btc;
}

Rect dbNet::getTermBBox()
{
  Rect net_box;
  net_box.mergeInit();

  for (dbITerm* iterm : getITerms()) {
    int x, y;
    if (iterm->getAvgXY(&x, &y)) {
      Rect iterm_rect(x, y, x, y);
      net_box.merge(iterm_rect);
    } else {
      // This clause is sort of worthless because getAvgXY prints
      // a warning when it fails.
      dbInst* inst = iterm->getInst();
      dbBox* inst_box = inst->getBBox();
      int center_x = (inst_box->xMin() + inst_box->xMax()) / 2;
      int center_y = (inst_box->yMin() + inst_box->yMax()) / 2;
      Rect inst_center(center_x, center_y, center_x, center_y);
      net_box.merge(inst_center);
    }
  }

  for (dbBTerm* bterm : getBTerms()) {
    for (dbBPin* bpin : bterm->getBPins()) {
      dbPlacementStatus status = bpin->getPlacementStatus();
      if (status.isPlaced()) {
        Rect pin_bbox = bpin->getBBox();
        int center_x = (pin_bbox.xMin() + pin_bbox.xMax()) / 2;
        int center_y = (pin_bbox.yMin() + pin_bbox.yMax()) / 2;
        Rect pin_center(center_x, center_y, center_x, center_y);
        net_box.merge(pin_center);
      }
    }
  }
  return net_box;
}

void dbNet::destroySWires()
{
  _dbNet* net = (_dbNet*) this;

  dbSet<dbSWire> swires = getSWires();

  for (auto sitr = swires.begin(); sitr != swires.end();) {
    sitr = dbSWire::destroy(sitr);
  }

  net->_swires = 0;
}

dbNet* dbNet::create(dbBlock* block_, const char* name_, bool skipExistingCheck)
{
  _dbBlock* block = (_dbBlock*) block_;

  if (!skipExistingCheck && block->_net_hash.hasMember(name_)) {
    return nullptr;
  }

  _dbNet* net = block->_net_tbl->create();
  if (block->_journal) {
    debugPrint(block->getImpl()->getLogger(),
               utl::ODB,
               "DB_ECO",
               1,
               "ECO: create net, name {}",
               name_);
    block->_journal->beginAction(dbJournal::CREATE_OBJECT);
    block->_journal->pushParam(dbNetObj);
    block->_journal->pushParam(name_);
    block->_journal->pushParam(net->getOID());
    block->_journal->endAction();
  }

  net->_name = strdup(name_);
  ZALLOCATED(net->_name);
  block->_net_hash.insert(net);

  std::list<dbBlockCallBackObj*>::iterator cbitr;
  for (cbitr = block->_callbacks.begin(); cbitr != block->_callbacks.end();
       ++cbitr) {
    (**cbitr)().inDbNetCreate((dbNet*) net);  // client ECO optimization - payam
  }

  return (dbNet*) net;
}

void dbNet::destroy(dbNet* net_)
{
  _dbNet* net = (_dbNet*) net_;
  _dbBlock* block = (_dbBlock*) net->getOwner();
  dbBlock* dbblock = (dbBlock*) block;

  if (net->_flags._dont_touch) {
    net->getLogger()->error(
        utl::ODB, 364, "Attempt to destroy dont_touch net {}", net->_name);
  }

  dbSet<dbITerm> iterms = net_->getITerms();
  dbSet<dbITerm>::iterator iitr = iterms.begin();

  while (iitr != iterms.end()) {
    dbITerm* iterm = *iitr;
    ++iitr;
    iterm->disconnect();
  }

  dbSet<dbBTerm> bterms = net_->getBTerms();
  for (auto bitr = bterms.begin(); bitr != bterms.end();) {
    bitr = dbBTerm::destroy(bitr);
  }

  dbSet<dbSWire> swires = net_->getSWires();
  for (auto sitr = swires.begin(); sitr != swires.end();) {
    sitr = dbSWire::destroy(sitr);
  }

  if (net->_wire != 0) {
    dbWire* wire = (dbWire*) block->_wire_tbl->getPtr(net->_wire);
    dbWire::destroy(wire);
  }

  for (dbId<_dbGroup> _group_id : net->_groups) {
    dbGroup* group = (dbGroup*) block->_group_tbl->getPtr(_group_id);
    group->removeNet(net_);
  }

  dbSet<dbGuide> guides = net_->getGuides();
  for (auto gitr = guides.begin(); gitr != guides.end();) {
    gitr = dbGuide::destroy(gitr);
  }

  dbSet<dbGlobalConnect> connects = dbblock->getGlobalConnects();
  for (auto gitr = connects.begin(); gitr != connects.end();) {
    if (gitr->getNet()->getId() == net_->getId()) {
      gitr = dbGlobalConnect::destroy(gitr);
    } else {
      gitr++;
    }
  }

  if (block->_journal) {
    debugPrint(block->getImpl()->getLogger(),
               utl::ODB,
               "DB_ECO",
               1,
               "ECO: destroy net, id: {}",
               net->getId());
    block->_journal->beginAction(dbJournal::DELETE_OBJECT);
    block->_journal->pushParam(dbNetObj);
    block->_journal->pushParam(net_->getName());
    block->_journal->pushParam(net->getOID());
    uint* flags = (uint*) &net->_flags;
    block->_journal->pushParam(*flags);
    block->_journal->pushParam(net->_non_default_rule);
    block->_journal->endAction();
  }

  // Bugzilla #7: The notification of the the net destruction must
  // be done after pin manipulation is completed. The notification is
  // now after the pin disconnection - payam 01/10/2006
  std::list<dbBlockCallBackObj*>::iterator cbitr;
  for (cbitr = block->_callbacks.begin(); cbitr != block->_callbacks.end();
       ++cbitr) {
    (**cbitr)().inDbNetDestroy(net_);  // client ECO optimization - payam
  }

  dbProperty::destroyProperties(net);
  block->_net_hash.remove(net);
  block->_net_tbl->destroy(net);
}

dbSet<dbNet>::iterator dbNet::destroy(dbSet<dbNet>::iterator& itr)
{
  dbNet* bt = *itr;
  dbSet<dbNet>::iterator next = ++itr;
  destroy(bt);
  return next;
}

dbNet* dbNet::getNet(dbBlock* block_, uint dbid_)
{
  _dbBlock* block = (_dbBlock*) block_;
  return (dbNet*) block->_net_tbl->getPtr(dbid_);
}

dbNet* dbNet::getValidNet(dbBlock* block_, uint dbid_)
{
  _dbBlock* block = (_dbBlock*) block_;
  if (!block->_net_tbl->validId(dbid_)) {
    return nullptr;
  }
  return (dbNet*) block->_net_tbl->getPtr(dbid_);
}

bool dbNet::canMergeNet(dbNet* in_net)
{
  if (isDoNotTouch() || in_net->isDoNotTouch()) {
    return false;
  }

  for (dbITerm* iterm : in_net->getITerms()) {
    if (iterm->getInst()->isDoNotTouch()) {
      return false;
    }
  }

  return true;
}

void dbNet::mergeNet(dbNet* in_net)
{
  _dbNet* net = (_dbNet*) this;
  _dbBlock* block = (_dbBlock*) net->getOwner();

  std::vector<dbITerm*> iterms;
  for (dbITerm* iterm : in_net->getITerms()) {
    iterms.push_back(iterm);
  }

  for (auto callback : block->_callbacks) {
    callback->inDbNetPreMerge(this, in_net);
  }

  for (dbITerm* iterm : iterms) {
    iterm->connect(this);
  }

  for (dbBTerm* bterm : in_net->getBTerms()) {
    bterm->connect(this);
  }
}

void dbNet::markNets(std::vector<dbNet*>& nets, dbBlock* block, bool mk)
{
  uint j;
  dbNet* net;
  if (nets.empty()) {
    dbSet<dbNet> bnets = block->getNets();
    dbSet<dbNet>::iterator nitr;
    for (nitr = bnets.begin(); nitr != bnets.end(); ++nitr) {
      net = (dbNet*) *nitr;
      net->setMark(mk);
    }
  } else {
    for (j = 0; j < nets.size(); j++) {
      net = nets[j];
      net->setMark(mk);
    }
  }
}

dbSet<dbGuide> dbNet::getGuides() const
{
  _dbNet* net = (_dbNet*) this;
  _dbBlock* block = (_dbBlock*) net->getOwner();
  return dbSet<dbGuide>(net, block->_guide_itr);
}

void dbNet::clearGuides()
{
  auto guides = getGuides();
  dbSet<dbGuide>::iterator itr = guides.begin();
  while (itr != guides.end()) {
    auto curGuide = *itr++;
    dbGuide::destroy(curGuide);
  }
}

dbSet<dbNetTrack> dbNet::getTracks() const
{
  _dbNet* net = (_dbNet*) this;
  _dbBlock* block = (_dbBlock*) net->getOwner();
  return dbSet<dbNetTrack>(net, block->_net_track_itr);
}

void dbNet::clearTracks()
{
  auto tracks = getTracks();
  dbSet<dbNetTrack>::iterator itr = tracks.begin();
  while (itr != tracks.end()) {
    auto curTrack = *itr++;
    dbNetTrack::destroy(curTrack);
  }
}

bool dbNet::hasJumpers()
{
  bool has_jumpers = false;
  _dbNet* net = (_dbNet*) this;
  _dbDatabase* db = net->getImpl()->getDatabase();
  if (db->isSchema(db_schema_has_jumpers)) {
    has_jumpers = net->_flags._has_jumpers == 1;
  }
  return has_jumpers;
}

void dbNet::setJumpers(bool has_jumpers)
{
  _dbNet* net = (_dbNet*) this;
  _dbDatabase* db = net->getImpl()->getDatabase();
  if (db->isSchema(db_schema_has_jumpers)) {
    net->_flags._has_jumpers = has_jumpers ? 1 : 0;
  }
}

void _dbNet::collectMemInfo(MemInfo& info)
{
  info.cnt++;
  info.size += sizeof(*this);

  info.children_["name"].add(_name);
  info.children_["groups"].add(_groups);
}

}  // namespace odb
