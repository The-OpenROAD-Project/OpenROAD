// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "dbNet.h"

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iterator>
#include <set>
#include <string>
#include <type_traits>
#include <vector>

#include "boost/container/small_vector.hpp"
#include "dbBPin.h"
#include "dbBTerm.h"
#include "dbBTermItr.h"
#include "dbBlock.h"
#include "dbCCSeg.h"
#include "dbCCSegItr.h"
#include "dbCapNode.h"
#include "dbCapNodeItr.h"
#include "dbCommon.h"
#include "dbCore.h"
#include "dbDatabase.h"
#include "dbGroup.h"
#include "dbGuide.h"
#include "dbGuideItr.h"
#include "dbITerm.h"
#include "dbITermItr.h"
#include "dbInsertBuffer.h"
#include "dbInst.h"
#include "dbJournal.h"
#include "dbMTerm.h"
#include "dbMaster.h"
#include "dbModNet.h"
#include "dbModule.h"
#include "dbNetTrack.h"
#include "dbNetTrackItr.h"
#include "dbRSeg.h"
#include "dbRSegItr.h"
#include "dbSWire.h"
#include "dbSWireItr.h"
#include "dbTable.h"
#include "dbTech.h"
#include "dbTechNonDefaultRule.h"
#include "dbWire.h"
#include "odb/db.h"
#include "odb/dbBlockCallBackObj.h"
#include "odb/dbExtControl.h"
#include "odb/dbObject.h"
#include "odb/dbSet.h"
#include "odb/dbShape.h"
#include "odb/dbTypes.h"
#include "odb/dbUtil.h"
#include "odb/geom.h"
#include "utl/Logger.h"

namespace odb {

template class dbTable<_dbNet>;

_dbNet::_dbNet(_dbDatabase* db, const _dbNet& n)
    : flags_(n.flags_),
      name_(nullptr),
      next_entry_(n.next_entry_),
      iterms_(n.iterms_),
      bterms_(n.bterms_),
      wire_(n.wire_),
      global_wire_(n.global_wire_),
      swires_(n.swires_),
      cap_nodes_(n.cap_nodes_),
      r_segs_(n.r_segs_),
      non_default_rule_(n.non_default_rule_),
      guides_(n.guides_),
      tracks_(n.tracks_),
      groups_(n.groups_),
      weight_(n.weight_),
      xtalk_(n.xtalk_),
      cc_adjust_factor_(n.cc_adjust_factor_),
      cc_adjust_order_(n.cc_adjust_order_)

{
  if (n.name_) {
    name_ = safe_strdup(n.name_);
  }
  driving_iterm_ = -1;
}

_dbNet::_dbNet(_dbDatabase* db)
{
  flags_.sig_type = dbSigType::SIGNAL;
  flags_.wire_type = dbWireType::ROUTED;
  flags_.special = 0;
  flags_.wild_connect = 0;
  flags_.wire_ordered = 0;
  flags_.unused2 = 0;
  flags_.disconnected = 0;
  flags_.spef = 0;
  flags_.select = 0;
  flags_.mark = 0;
  flags_.mark_1 = 0;
  flags_.wire_altered = 0;
  flags_.extracted = 0;
  flags_.rc_graph = 0;
  flags_.unused = 0;
  flags_.set_io = 0;
  flags_.io = 0;
  flags_.dont_touch = 0;
  flags_.fixed_bump = 0;
  flags_.source = dbSourceType::NONE;
  flags_.rc_disconnected = 0;
  flags_.block_rule = 0;
  flags_.has_jumpers = 0;
  name_ = nullptr;
  gndc_calibration_factor_ = 1.0;
  cc_calibration_factor_ = 1.0;
  weight_ = 1;
  xtalk_ = 0;
  cc_adjust_factor_ = -1;
  cc_adjust_order_ = 0;
  driving_iterm_ = -1;
}

_dbNet::~_dbNet()
{
  if (name_) {
    free((void*) name_);
  }
}

dbOStream& operator<<(dbOStream& stream, const _dbNet& net)
{
  uint32_t* bit_field = (uint32_t*) &net.flags_;
  stream << *bit_field;
  stream << net.name_;
  stream << net.gndc_calibration_factor_;
  stream << net.cc_calibration_factor_;
  stream << net.next_entry_;
  stream << net.iterms_;
  stream << net.bterms_;
  stream << net.wire_;
  stream << net.global_wire_;
  stream << net.swires_;
  stream << net.cap_nodes_;
  stream << net.r_segs_;
  stream << net.non_default_rule_;
  stream << net.weight_;
  stream << net.xtalk_;
  stream << net.cc_adjust_factor_;
  stream << net.cc_adjust_order_;
  stream << net.groups_;
  stream << net.guides_;
  stream << net.tracks_;
  return stream;
}

dbIStream& operator>>(dbIStream& stream, _dbNet& net)
{
  uint32_t* bit_field = (uint32_t*) &net.flags_;
  stream >> *bit_field;
  stream >> net.name_;
  stream >> net.gndc_calibration_factor_;
  stream >> net.cc_calibration_factor_;
  stream >> net.next_entry_;
  stream >> net.iterms_;
  stream >> net.bterms_;
  stream >> net.wire_;
  stream >> net.global_wire_;
  stream >> net.swires_;
  stream >> net.cap_nodes_;
  stream >> net.r_segs_;
  stream >> net.non_default_rule_;
  stream >> net.weight_;
  stream >> net.xtalk_;
  stream >> net.cc_adjust_factor_;
  stream >> net.cc_adjust_order_;
  stream >> net.groups_;
  stream >> net.guides_;
  _dbDatabase* db = net.getImpl()->getDatabase();
  if (db->isSchema(kSchemaNetTracks)) {
    stream >> net.tracks_;
  }

  return stream;
}

bool _dbNet::operator<(const _dbNet& rhs) const
{
  return strcmp(name_, rhs.name_) < 0;
}

bool _dbNet::operator==(const _dbNet& rhs) const
{
  if (flags_.sig_type != rhs.flags_.sig_type) {
    return false;
  }

  if (flags_.wire_type != rhs.flags_.wire_type) {
    return false;
  }

  if (flags_.special != rhs.flags_.special) {
    return false;
  }

  if (flags_.wild_connect != rhs.flags_.wild_connect) {
    return false;
  }

  if (flags_.wire_ordered != rhs.flags_.wire_ordered) {
    return false;
  }

  if (flags_.disconnected != rhs.flags_.disconnected) {
    return false;
  }

  if (flags_.spef != rhs.flags_.spef) {
    return false;
  }

  if (flags_.select != rhs.flags_.select) {
    return false;
  }

  if (flags_.mark != rhs.flags_.mark) {
    return false;
  }

  if (flags_.mark_1 != rhs.flags_.mark_1) {
    return false;
  }

  if (flags_.wire_altered != rhs.flags_.wire_altered) {
    return false;
  }

  if (flags_.extracted != rhs.flags_.extracted) {
    return false;
  }

  if (flags_.rc_graph != rhs.flags_.rc_graph) {
    return false;
  }

  if (flags_.set_io != rhs.flags_.set_io) {
    return false;
  }

  if (flags_.io != rhs.flags_.io) {
    return false;
  }

  if (flags_.dont_touch != rhs.flags_.dont_touch) {
    return false;
  }

  if (flags_.fixed_bump != rhs.flags_.fixed_bump) {
    return false;
  }

  if (flags_.source != rhs.flags_.source) {
    return false;
  }

  if (flags_.rc_disconnected != rhs.flags_.rc_disconnected) {
    return false;
  }

  if (flags_.block_rule != rhs.flags_.block_rule) {
    return false;
  }

  if (name_ && rhs.name_) {
    if (strcmp(name_, rhs.name_) != 0) {
      return false;
    }
  } else if (name_ || rhs.name_) {
    return false;
  }

  if (gndc_calibration_factor_ != rhs.gndc_calibration_factor_) {
    return false;
  }
  if (cc_calibration_factor_ != rhs.cc_calibration_factor_) {
    return false;
  }

  if (next_entry_ != rhs.next_entry_) {
    return false;
  }

  if (iterms_ != rhs.iterms_) {
    return false;
  }

  if (bterms_ != rhs.bterms_) {
    return false;
  }

  if (wire_ != rhs.wire_) {
    return false;
  }

  if (global_wire_ != rhs.global_wire_) {
    return false;
  }

  if (swires_ != rhs.swires_) {
    return false;
  }

  if (cap_nodes_ != rhs.cap_nodes_) {
    return false;
  }

  if (r_segs_ != rhs.r_segs_) {
    return false;
  }

  if (non_default_rule_ != rhs.non_default_rule_) {
    return false;
  }

  if (weight_ != rhs.weight_) {
    return false;
  }

  if (xtalk_ != rhs.xtalk_) {
    return false;
  }

  if (cc_adjust_factor_ != rhs.cc_adjust_factor_) {
    return false;
  }

  if (cc_adjust_order_ != rhs.cc_adjust_order_) {
    return false;
  }

  if (groups_ != rhs.groups_) {
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

std::string dbNet::getName() const
{
  _dbNet* net = (_dbNet*) this;
  return net->name_;
}

const char* dbNet::getConstName() const
{
  _dbNet* net = (_dbNet*) this;
  return net->name_;
}

void dbNet::printNetName(FILE* fp, bool idFlag, bool newLine)
{
  if (idFlag) {
    fprintf(fp, " %d", getId());
  }

  _dbNet* net = (_dbNet*) this;
  fprintf(fp, " %s", net->name_);

  if (newLine) {
    fprintf(fp, "\n");
  }
}

bool dbNet::rename(const char* name)
{
  _dbNet* net = (_dbNet*) this;
  _dbBlock* block = (_dbBlock*) net->getOwner();

  if (block->net_hash_.hasMember(name)) {
    return false;
  }

  debugPrint(getImpl()->getLogger(),
             utl::ODB,
             "DB_EDIT",
             1,
             "EDIT: {}, rename to '{}'",
             net->getDebugName(),
             name);

  if (block->journal_) {
    block->journal_->updateField(this, _dbNet::kName, net->name_, name);
  }

  block->net_hash_.remove(net);
  free((void*) net->name_);
  net->name_ = safe_strdup(name);
  block->net_hash_.insert(net);

  return true;
}

void dbNet::swapNetNames(dbNet* source, bool ok_to_journal)
{
  _dbNet* dest_net = (_dbNet*) this;
  _dbNet* source_net = (_dbNet*) source;
  _dbBlock* block = (_dbBlock*) source_net->getOwner();

  char* dest_name_ptr = dest_net->name_;
  char* source_name_ptr = source_net->name_;

  debugPrint(getImpl()->getLogger(),
             utl::ODB,
             "DB_EDIT",
             1,
             "EDIT: swap dbNet name between {} and {}",
             source->getDebugName(),
             dest_net->getDebugName());

  // allow undo..
  if (block->journal_ && ok_to_journal) {
    block->journal_->beginAction(dbJournal::kSwapObject);
    // a name
    block->journal_->pushParam(dbNameObj);
    // the type of name swap
    block->journal_->pushParam(dbNetObj);
    // stash the source and dest in that order,
    // let undo reorder
    block->journal_->pushParam(source_net->getId());
    block->journal_->pushParam(dest_net->getId());
    block->journal_->endAction();
  }

  block->net_hash_.remove(dest_net);
  block->net_hash_.remove(source_net);

  // swap names without copy, just swap the pointers
  dest_net->name_ = source_name_ptr;
  source_net->name_ = dest_name_ptr;

  block->net_hash_.insert(dest_net);
  block->net_hash_.insert(source_net);
}

bool dbNet::isRCDisconnected()
{
  _dbNet* net = (_dbNet*) this;
  return net->flags_.rc_disconnected == 1;
}

void dbNet::setRCDisconnected(bool value)
{
  _dbNet* net = (_dbNet*) this;
  net->flags_.rc_disconnected = value;
}

int dbNet::getWeight()
{
  _dbNet* net = (_dbNet*) this;
  return net->weight_;
}

void dbNet::setWeight(int weight)
{
  _dbNet* net = (_dbNet*) this;
  net->weight_ = weight;
}

dbSourceType dbNet::getSourceType()
{
  _dbNet* net = (_dbNet*) this;
  dbSourceType t(net->flags_.source);
  return t;
}

void dbNet::setSourceType(dbSourceType type)
{
  _dbNet* net = (_dbNet*) this;
  net->flags_.source = type;
}

int dbNet::getXTalkClass()
{
  _dbNet* net = (_dbNet*) this;
  return net->xtalk_;
}

void dbNet::setXTalkClass(int value)
{
  _dbNet* net = (_dbNet*) this;
  net->xtalk_ = value;
}

float dbNet::getCcAdjustFactor()
{
  _dbNet* net = (_dbNet*) this;
  return net->cc_adjust_factor_;
}

void dbNet::setCcAdjustFactor(float factor)
{
  _dbNet* net = (_dbNet*) this;
  net->cc_adjust_factor_ = factor;
}

uint32_t dbNet::getCcAdjustOrder()
{
  _dbNet* net = (_dbNet*) this;
  return net->cc_adjust_order_;
}

void dbNet::setCcAdjustOrder(uint32_t order)
{
  _dbNet* net = (_dbNet*) this;
  net->cc_adjust_order_ = order;
}

void dbNet::setDrivingITerm(const dbITerm* iterm)
{
  _dbNet* net = (_dbNet*) this;
  net->driving_iterm_ = (iterm) ? iterm->getId() : 0;
}

dbITerm* dbNet::getDrivingITerm() const
{
  _dbNet* net = (_dbNet*) this;
  if (net->driving_iterm_ <= 0) {
    return nullptr;
  }
  return dbITerm::getITerm(getBlock(), net->driving_iterm_);
}

bool dbNet::hasFixedBump()
{
  _dbNet* net = (_dbNet*) this;
  return net->flags_.fixed_bump == 1;
}

void dbNet::setFixedBump(bool value)
{
  _dbNet* net = (_dbNet*) this;
  net->flags_.fixed_bump = value;
}

void dbNet::setWireType(dbWireType wire_type)
{
  _dbNet* net = (_dbNet*) this;
  _dbBlock* block = (_dbBlock*) net->getOwner();
  uint32_t prev_flags = flagsToUInt(net);
  net->flags_.wire_type = wire_type.getValue();

  debugPrint(getImpl()->getLogger(),
             utl::ODB,
             "DB_EDIT",
             2,
             "EDIT: {}, setWireType: {}",
             net->getDebugName(),
             wire_type.getValue());

  if (block->journal_) {
    block->journal_->updateField(
        this, _dbNet::kFlags, prev_flags, flagsToUInt(net));
  }
}

dbWireType dbNet::getWireType() const
{
  _dbNet* net = (_dbNet*) this;
  return dbWireType(net->flags_.wire_type);
}

dbSigType dbNet::getSigType() const
{
  _dbNet* net = (_dbNet*) this;
  return dbSigType(net->flags_.sig_type);
}

void dbNet::setSigType(dbSigType sig_type)
{
  _dbNet* net = (_dbNet*) this;
  _dbBlock* block = (_dbBlock*) net->getOwner();
  uint32_t prev_flags = flagsToUInt(net);
  net->flags_.sig_type = sig_type.getValue();

  debugPrint(getImpl()->getLogger(),
             utl::ODB,
             "DB_EDIT",
             2,
             "EDIT: {}, setSigType: {}",
             net->getDebugName(),
             sig_type.getValue());

  if (block->journal_) {
    block->journal_->updateField(
        this, _dbNet::kFlags, prev_flags, flagsToUInt(net));
  }
}

float dbNet::getGndcCalibFactor()
{
  _dbNet* net = (_dbNet*) this;
  return net->gndc_calibration_factor_;
}

void dbNet::setGndcCalibFactor(float gndcCalib)
{
  _dbNet* net = (_dbNet*) this;
  net->gndc_calibration_factor_ = gndcCalib;
}

float dbNet::getRefCc()
{
  _dbNet* net = (_dbNet*) this;
  return net->ref_cc_;
}

void dbNet::setRefCc(float cap)
{
  _dbNet* net = (_dbNet*) this;
  net->ref_cc_ = cap;
}

float dbNet::getCcCalibFactor()
{
  _dbNet* net = (_dbNet*) this;
  return net->cc_calibration_factor_;
}

void dbNet::setCcCalibFactor(float ccCalib)
{
  _dbNet* net = (_dbNet*) this;
  net->cc_calibration_factor_ = ccCalib;
}

float dbNet::getDbCc()
{
  _dbNet* net = (_dbNet*) this;
  return net->db_cc_;
}

void dbNet::setDbCc(float cap)
{
  _dbNet* net = (_dbNet*) this;
  net->db_cc_ = cap;
}

void dbNet::addDbCc(float cap)
{
  _dbNet* net = (_dbNet*) this;
  net->db_cc_ += cap;
}

float dbNet::getCcMatchRatio()
{
  _dbNet* net = (_dbNet*) this;
  return net->cc_match_ratio_;
}

void dbNet::setCcMatchRatio(float ratio)
{
  _dbNet* net = (_dbNet*) this;
  net->cc_match_ratio_ = ratio;
}

void dbNet::calibrateCouplingCap(int corner)
{
  const float srcnetCcCalibFactor = getCcCalibFactor();
  std::vector<dbCCSeg*> ccSet;
  getSrcCCSegs(ccSet);
  for (dbCCSeg* cc : ccSet) {
    const float tgtnetCcCalibFactor = cc->getTargetNet()->getCcCalibFactor();
    const float factor = (srcnetCcCalibFactor + tgtnetCcCalibFactor) / 2;
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

uint32_t dbNet::getRSegCount()
{
  return getRSegs().size();
}

uint32_t dbNet::maxInternalCapNum()
{
  uint32_t max_n = 0;
  for (dbCapNode* capn : getCapNodes()) {
    if (!capn->isInternal()) {
      continue;
    }

    const uint32_t n = capn->getNode();
    max_n = std::max(max_n, n);
  }
  return max_n;
}
void dbNet::collapseInternalCapNum(FILE* cap_node_map)
{
  uint32_t cnt = 1;
  for (dbCapNode* capn : getCapNodes()) {
    cnt++;
    if (capn->isInternal()) {
      if (cap_node_map) {
        fprintf(cap_node_map, "    %8d : %8d\n", capn->getNode(), cnt);
      }
      capn->setNode(cnt);
    }
  }
}

uint32_t dbNet::getCapNodeCount()
{
  return getCapNodes().size();
}

uint32_t dbNet::getCcCount()
{
  uint32_t count = 0;
  for (dbCapNode* node : getCapNodes()) {
    count += node->getCCSegs().size();
  }
  return count;
}

bool dbNet::groundCC(const float gndFactor)
{
  bool grounded = false;

  for (dbCapNode* node : getCapNodes()) {
    grounded |= node->groundCC(gndFactor);
  }
  return grounded;
}

bool dbNet::adjustCC(uint32_t adjOrder,
                     float adjFactor,
                     double ccThreshHold,
                     std::vector<dbCCSeg*>& adjustedCC,
                     std::vector<dbNet*>& halonets)
{
  _dbNet* net = (_dbNet*) this;
  if (net->cc_adjust_factor_ > 0) {
    getImpl()->getLogger()->warn(
        utl::ODB,
        48,
        "Net {} {} had been CC adjusted by {}. Please unadjust first.",
        getId(),
        getConstName(),
        net->cc_adjust_factor_);
    return false;
  }
  bool needAdjust = false;
  for (dbCapNode* node : getCapNodes()) {
    if (node->needAdjustCC(ccThreshHold)) {
      needAdjust = true;
    }
  }
  if (!needAdjust) {
    return false;
  }

  for (dbCapNode* node : getCapNodes()) {
    node->adjustCC(adjOrder, adjFactor, adjustedCC, halonets);
  }
  net->cc_adjust_factor_ = adjFactor;
  net->cc_adjust_order_ = adjOrder;
  return true;
}

void dbNet::undoAdjustedCC(std::vector<dbCCSeg*>& adjustedCC,
                           std::vector<dbNet*>& halonets)
{
  _dbNet* net = (_dbNet*) this;
  if (net->cc_adjust_factor_ < 0) {
    return;
  }
  const uint32_t adjOrder = net->cc_adjust_order_;
  const float adjFactor = 1 / net->cc_adjust_factor_;
  for (dbCapNode* node : getCapNodes()) {
    node->adjustCC(adjOrder, adjFactor, adjustedCC, halonets);
  }
  net->cc_adjust_factor_ = -1;
  net->cc_adjust_order_ = 0;
}

void dbNet::adjustNetGndCap(uint32_t corner, float factor)
{
  if (factor == 1.0) {
    return;
  }
  bool foreign = ((dbBlock*) getImpl()->getOwner())->getExtControl()->_foreign;
  if (foreign) {
    for (dbCapNode* node : getCapNodes()) {
      node->adjustCapacitance(factor, corner);
    }
  } else {
    for (dbRSeg* rc : getRSegs()) {
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
    for (dbCapNode* node : getCapNodes()) {
      node->adjustCapacitance(factor);
    }
  } else {
    for (dbRSeg* rc : getRSegs()) {
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
void dbNet::adjustNetRes(float factor, uint32_t corner)
{
  if (factor == 1.0) {
    return;
  }
  for (dbRSeg* rc : getRSegs()) {
    rc->adjustResistance(factor, corner);
  }
}
void dbNet::adjustNetRes(float factor)
{
  if (factor == 1.0) {
    return;
  }
  for (dbRSeg* rc : getRSegs()) {
    rc->adjustResistance(factor);
  }
}

bool dbNet::isSpef()
{
  _dbNet* net = (_dbNet*) this;
  return net->flags_.spef == 1;
}

void dbNet::setSpef(bool value)
{
  _dbNet* net = (_dbNet*) this;
  _dbBlock* block = (_dbBlock*) net->getOwner();
  uint32_t prev_flags = flagsToUInt(net);
  net->flags_.spef = (value == true) ? 1 : 0;

  debugPrint(getImpl()->getLogger(),
             utl::ODB,
             "DB_EDIT",
             2,
             "EDIT: {}, setSpef: {}",
             net->getDebugName(),
             value);

  if (block->journal_) {
    block->journal_->updateField(
        this, _dbNet::kFlags, prev_flags, flagsToUInt(net));
  }
}

bool dbNet::isSelect()
{
  _dbNet* net = (_dbNet*) this;
  return net->flags_.select == 1;
}

void dbNet::setSelect(bool value)
{
  _dbNet* net = (_dbNet*) this;
  _dbBlock* block = (_dbBlock*) net->getOwner();
  uint32_t prev_flags = flagsToUInt(net);
  net->flags_.select = (value == true) ? 1 : 0;

  debugPrint(getImpl()->getLogger(),
             utl::ODB,
             "DB_EDIT",
             2,
             "EDIT: {}, setSelect: {}",
             net->getDebugName(),
             value);

  if (block->journal_) {
    block->journal_->updateField(
        this, _dbNet::kFlags, prev_flags, flagsToUInt(net));
  }
}

bool dbNet::isEnclosed(Rect* bbox)  // assuming no intersection
{
  dbWire* wire = getWire();
  dbWirePathItr pitr;
  dbWirePath path;
  dbWirePathShape pathShape;
  pitr.begin(wire);
  uint32_t cnt = 0;
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
  return net->flags_.mark == 1;
}

void dbNet::setMark(bool value)
{
  _dbNet* net = (_dbNet*) this;
  _dbBlock* block = (_dbBlock*) net->getOwner();
  uint32_t prev_flags = flagsToUInt(net);
  net->flags_.mark = (value == true) ? 1 : 0;

  debugPrint(getImpl()->getLogger(),
             utl::ODB,
             "DB_EDIT",
             2,
             "EDIT: {}, setMark: {}",
             net->getDebugName(),
             value);

  if (block->journal_) {
    block->journal_->updateField(
        this, _dbNet::kFlags, prev_flags, flagsToUInt(net));
  }
}

bool dbNet::isMark_1ed()
{
  _dbNet* net = (_dbNet*) this;
  return net->flags_.mark_1 == 1;
}

void dbNet::setMark_1(bool value)
{
  _dbNet* net = (_dbNet*) this;
  _dbBlock* block = (_dbBlock*) net->getOwner();
  uint32_t prev_flags = flagsToUInt(net);
  net->flags_.mark_1 = (value == true) ? 1 : 0;

  debugPrint(getImpl()->getLogger(),
             utl::ODB,
             "DB_EDIT",
             2,
             "EDIT: {}, setMark_1: {}",
             net->getDebugName(),
             value);

  if (block->journal_) {
    block->journal_->updateField(
        this, _dbNet::kFlags, prev_flags, flagsToUInt(net));
  }
}

bool dbNet::isWireOrdered()
{
  _dbNet* net = (_dbNet*) this;
  return net->flags_.wire_ordered == 1;
}

void dbNet::setWireOrdered(bool value)
{
  _dbNet* net = (_dbNet*) this;

  _dbBlock* block = (_dbBlock*) net->getOwner();
  uint32_t prev_flags = flagsToUInt(net);

  net->flags_.wire_ordered = (value == true) ? 1 : 0;

  debugPrint(getImpl()->getLogger(),
             utl::ODB,
             "DB_EDIT",
             2,
             "EDIT: {}, setWireOrdered: {}",
             net->getDebugName(),
             value);

  if (block->journal_) {
    block->journal_->updateField(
        this, _dbNet::kFlags, prev_flags, flagsToUInt(net));
  }
}

bool dbNet::isDisconnected()
{
  _dbNet* net = (_dbNet*) this;
  return net->flags_.disconnected == 1;
}

void dbNet::setDisconnected(bool value)
{
  _dbNet* net = (_dbNet*) this;

  _dbBlock* block = (_dbBlock*) net->getOwner();
  uint32_t prev_flags = flagsToUInt(net);

  net->flags_.disconnected = (value == true) ? 1 : 0;

  debugPrint(getImpl()->getLogger(),
             utl::ODB,
             "DB_EDIT",
             2,
             "EDIT: {}, setDisconnected: {}",
             net->getDebugName(),
             value);

  if (block->journal_) {
    block->journal_->updateField(
        this, _dbNet::kFlags, prev_flags, flagsToUInt(net));
  }
}

void dbNet::setWireAltered(bool value)
{
  _dbNet* net = (_dbNet*) this;

  _dbBlock* block = (_dbBlock*) net->getOwner();
  uint32_t prev_flags = flagsToUInt(net);

  net->flags_.wire_altered = (value == true) ? 1 : 0;
  if (value) {
    net->flags_.wire_ordered = 0;
  }

  debugPrint(getImpl()->getLogger(),
             utl::ODB,
             "DB_EDIT",
             2,
             "EDIT: {}, setWireAltered: {}",
             net->getDebugName(),
             value);

  if (block->journal_) {
    block->journal_->updateField(
        this, _dbNet::kFlags, prev_flags, flagsToUInt(net));
  }
}

bool dbNet::isWireAltered()
{
  _dbNet* net = (_dbNet*) this;
  return net->flags_.wire_altered == 1;
}

void dbNet::setExtracted(bool value)
{
  _dbNet* net = (_dbNet*) this;

  _dbBlock* block = (_dbBlock*) net->getOwner();
  uint32_t prev_flags = flagsToUInt(net);

  net->flags_.extracted = (value == true) ? 1 : 0;

  debugPrint(getImpl()->getLogger(),
             utl::ODB,
             "DB_EDIT",
             2,
             "EDIT: {}, setExtracted: {}",
             net->getDebugName(),
             value);

  if (block->journal_) {
    block->journal_->updateField(
        this, _dbNet::kFlags, prev_flags, flagsToUInt(net));
  }
}

bool dbNet::isExtracted()
{
  _dbNet* net = (_dbNet*) this;
  return net->flags_.extracted == 1;
}

void dbNet::setRCgraph(bool value)
{
  _dbNet* net = (_dbNet*) this;

  _dbBlock* block = (_dbBlock*) net->getOwner();
  uint32_t prev_flags = flagsToUInt(net);

  net->flags_.rc_graph = (value == true) ? 1 : 0;

  debugPrint(getImpl()->getLogger(),
             utl::ODB,
             "DB_EDIT",
             2,
             "EDIT: {}, setRCgraph: {}",
             net->getDebugName(),
             value);

  if (block->journal_) {
    block->journal_->updateField(
        this, _dbNet::kFlags, prev_flags, flagsToUInt(net));
  }
}

bool dbNet::isRCgraph()
{
  _dbNet* net = (_dbNet*) this;
  return net->flags_.rc_graph == 1;
}

dbBlock* dbNet::getBlock() const
{
  return (dbBlock*) getImpl()->getOwner();
}

dbSet<dbITerm> dbNet::getITerms() const
{
  _dbNet* net = (_dbNet*) this;
  _dbBlock* block = (_dbBlock*) net->getOwner();
  return dbSet<dbITerm>(net, block->net_iterm_itr_);
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

dbSet<dbBTerm> dbNet::getBTerms() const
{
  _dbNet* net = (_dbNet*) this;
  _dbBlock* block = (_dbBlock*) net->getOwner();
  return dbSet<dbBTerm>(net, block->net_bterm_itr_);
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

dbObject* dbNet::getFirstDriverTerm() const
{
  if (getSigType().isSupply()) {
    return nullptr;
  }

  for (dbITerm* iterm : getITerms()) {
    if (iterm->getSigType().isSupply()) {
      continue;
    }

    if (iterm->isClocked()) {
      continue;
    }

    if (iterm->getIoType() == dbIoType::OUTPUT
        || iterm->getIoType() == dbIoType::INOUT) {
      return iterm;
    }
  }

  for (dbBTerm* bterm : getBTerms()) {
    if (bterm->getSigType().isSupply()) {
      continue;
    }

    if (bterm->getIoType() == dbIoType::INPUT
        || bterm->getIoType() == dbIoType::INOUT) {
      return bterm;
    }
  }

  return nullptr;
}

dbInst* dbNet::getFirstDriverInst() const
{
  dbObject* drvr = getFirstDriverTerm();
  if (drvr != nullptr && drvr->getObjectType() == dbITermObj) {
    return static_cast<dbITerm*>(drvr)->getInst();
  }
  return nullptr;
}

dbITerm* dbNet::getFirstOutput() const
{
  if (dbITerm* drvrIterm = getDrivingITerm()) {
    return drvrIterm;
  }

  for (dbITerm* tr : getITerms()) {
    if (tr->getSigType().isSupply()) {
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

  return nullptr;
}

dbITerm* dbNet::get1stSignalInput(bool io)
{
  for (dbITerm* tr : getITerms()) {
    if (tr->getSigType().isSupply()) {
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

  return nullptr;
}

dbSet<dbSWire> dbNet::getSWires()
{
  _dbNet* net = (_dbNet*) this;
  _dbBlock* block = (_dbBlock*) net->getOwner();
  return dbSet<dbSWire>(net, block->swire_itr_);
}

dbSWire* dbNet::getFirstSWire()
{
  _dbNet* net = (_dbNet*) this;
  _dbBlock* block = (_dbBlock*) net->getOwner();

  if (net->swires_ == 0) {
    return nullptr;
  }

  return (dbSWire*) block->swire_tbl_->getPtr(net->swires_);
}

dbWire* dbNet::getWire()
{
  _dbNet* net = (_dbNet*) this;
  _dbBlock* block = (_dbBlock*) net->getOwner();

  if (net->wire_ == 0) {
    return nullptr;
  }

  return (dbWire*) block->wire_tbl_->getPtr(net->wire_);
}

dbWire* dbNet::getGlobalWire()
{
  _dbNet* net = (_dbNet*) this;
  _dbBlock* block = (_dbBlock*) net->getOwner();

  if (net->global_wire_ == 0) {
    return nullptr;
  }

  return (dbWire*) block->wire_tbl_->getPtr(net->global_wire_);
}

bool dbNet::setIOflag()
{
  _dbNet* net = (_dbNet*) this;
  _dbBlock* block = (_dbBlock*) net->getOwner();
  const uint32_t prev_flags = flagsToUInt(net);
  net->flags_.set_io = 1;
  net->flags_.io = 0;
  const uint32_t n = getBTerms().size();

  if (n > 0) {
    net->flags_.io = 1;
  }

  debugPrint(getImpl()->getLogger(),
             utl::ODB,
             "DB_EDIT",
             1,
             "EDIT: {}, setIOFlag",
             net->getDebugName());

  if (block->journal_) {
    block->journal_->updateField(
        this, _dbNet::kFlags, prev_flags, flagsToUInt(net));
  }

  return (n > 0);
}

bool dbNet::isIO()
{
  _dbNet* net = (_dbNet*) this;

  if (net->flags_.set_io > 0) {
    return net->flags_.io == 1;
  }
  return setIOflag();
}

void dbNet::setDoNotTouch(bool v)
{
  _dbNet* net = (_dbNet*) this;
  net->flags_.dont_touch = v;
}

bool dbNet::isDoNotTouch() const
{
  _dbNet* net = (_dbNet*) this;
  return net->flags_.dont_touch == 1;
}

bool dbNet::isSpecial() const
{
  _dbNet* net = (_dbNet*) this;
  return net->flags_.special == 1;
}

void dbNet::setSpecial()
{
  _dbNet* net = (_dbNet*) this;

  _dbBlock* block = (_dbBlock*) net->getOwner();
  uint32_t prev_flags = flagsToUInt(net);

  net->flags_.special = 1;

  debugPrint(getImpl()->getLogger(),
             utl::ODB,
             "DB_EDIT",
             2,
             "EDIT: {}, setSpecial",
             net->getDebugName());

  if (block->journal_) {
    block->journal_->updateField(
        this, _dbNet::kFlags, prev_flags, flagsToUInt(net));
  }
}

void dbNet::clearSpecial()
{
  _dbNet* net = (_dbNet*) this;

  _dbBlock* block = (_dbBlock*) net->getOwner();
  uint32_t prev_flags = flagsToUInt(net);

  net->flags_.special = 0;

  debugPrint(getImpl()->getLogger(),
             utl::ODB,
             "DB_EDIT",
             2,
             "EDIT: {}, clearSpecial",
             net->getDebugName());

  if (block->journal_) {
    block->journal_->updateField(
        this, _dbNet::kFlags, prev_flags, flagsToUInt(net));
  }
}

bool dbNet::isConnected(const dbNet* other) const
{
  return (this == other);
}

bool dbNet::isConnected(const dbModNet* other) const
{
  if (other == nullptr) {
    return false;
  }
  dbNet* net = other->findRelatedNet();
  return isConnected(net);
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
  return net->flags_.wild_connect == 1;
}

void dbNet::setWildConnected()
{
  _dbNet* net = (_dbNet*) this;

  _dbBlock* block = (_dbBlock*) net->getOwner();
  uint32_t prev_flags = flagsToUInt(net);
  // uint32_t prev_flags = flagsToUInt(net);

  net->flags_.wild_connect = 1;

  debugPrint(getImpl()->getLogger(),
             utl::ODB,
             "DB_EDIT",
             2,
             "EDIT: {}, setWildConnected",
             net->getDebugName());

  if (block->journal_) {
    block->journal_->updateField(
        this, _dbNet::kFlags, prev_flags, flagsToUInt(net));
  }
}

void dbNet::clearWildConnected()
{
  _dbNet* net = (_dbNet*) this;

  _dbBlock* block = (_dbBlock*) net->getOwner();
  uint32_t prev_flags = flagsToUInt(net);
  // uint32_t prev_flags = flagsToUInt(net);

  net->flags_.wild_connect = 0;

  debugPrint(getImpl()->getLogger(),
             utl::ODB,
             "DB_EDIT",
             2,
             "EDIT: {}, clearWildConnected",
             net->getDebugName());

  if (block->journal_) {
    block->journal_->updateField(
        this, _dbNet::kFlags, prev_flags, flagsToUInt(net));
  }
}

dbSet<dbRSeg> dbNet::getRSegs()
{
  _dbNet* net = (_dbNet*) this;
  _dbBlock* block = (_dbBlock*) net->getOwner();
  return dbSet<dbRSeg>(net, block->r_seg_itr_);
}

void dbNet::reverseRSegs()
{
  dbSet<dbRSeg> rSet = getRSegs();
  rSet.reverse();
  _dbBlock* block = (_dbBlock*) getImpl()->getOwner();

  debugPrint(getImpl()->getLogger(),
             utl::ODB,
             "DB_EDIT",
             2,
             "EDIT: {}, reverse rsegs sequence",
             getDebugName());

  if (block->journal_) {
    block->journal_->beginAction(dbJournal::kUpdateField);
    block->journal_->pushParam(dbNetObj);
    block->journal_->pushParam(getId());
    block->journal_->pushParam(_dbNet::kReverseRSeg);
    block->journal_->endAction();
  }
}

dbRSeg* dbNet::findRSeg(uint32_t srcn, uint32_t tgtn)
{
  for (dbRSeg* rseg : getRSegs()) {
    if (rseg->getSourceNode() == srcn && rseg->getTargetNode() == tgtn) {
      return rseg;
    }
  }
  return nullptr;
}

void dbNet::set1stRSegId(uint32_t rseg_id)
{
  _dbNet* net = (_dbNet*) this;
  _dbBlock* block = (_dbBlock*) net->getOwner();
  uint32_t pid = net->r_segs_;
  net->r_segs_ = rseg_id;

  debugPrint(getImpl()->getLogger(),
             utl::ODB,
             "DB_EDIT",
             2,
             "EDIT: {}, set 1stRSegNode {}",
             getDebugName(),
             rseg_id);

  if (block->journal_) {
    block->journal_->beginAction(dbJournal::kUpdateField);
    block->journal_->pushParam(dbNetObj);
    block->journal_->pushParam(getId());
    block->journal_->pushParam(_dbNet::kHeadRSeg);
    block->journal_->pushParam(pid);
    block->journal_->pushParam(rseg_id);
    block->journal_->endAction();
  }
}

uint32_t dbNet::get1stRSegId()
{
  _dbNet* net = (_dbNet*) this;
  return net->r_segs_;
}

dbRSeg* dbNet::getZeroRSeg()
{
  _dbNet* net = (_dbNet*) this;
  if (net->r_segs_ == 0) {
    return nullptr;
  }
  dbRSeg* zrc = dbRSeg::getRSeg((dbBlock*) net->getOwner(), net->r_segs_);
  return zrc;
}

dbCapNode* dbNet::findCapNode(uint32_t nodeId)
{
  for (dbCapNode* n : getCapNodes()) {
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
  return dbSet<dbCapNode>(net, block->cap_node_itr_);
}

void dbNet::setTermExtIds(int capId)  // 1: capNodeId, 0: reset
{
  dbSet<dbCapNode> nodeSet = getCapNodes();
  dbSet<dbCapNode>::iterator rc_itr;
  _dbBlock* block = (_dbBlock*) getImpl()->getOwner();

  debugPrint(getImpl()->getLogger(),
             utl::ODB,
             "DB_EDIT",
             2,
             "EDIT: {} {} term extId",
             (capId) ? "set" : "reset",
             getDebugName());

  if (block->journal_) {
    block->journal_->beginAction(dbJournal::kUpdateField);
    block->journal_->pushParam(dbNetObj);
    block->journal_->pushParam(getId());
    block->journal_->pushParam(_dbNet::kTermExtId);
    block->journal_->pushParam(capId);
    block->journal_->endAction();
  }

  for (rc_itr = nodeSet.begin(); rc_itr != nodeSet.end(); ++rc_itr) {
    dbCapNode* capNode = *rc_itr;
    if (capNode->isBTerm()) {
      uint32_t nodeId = capNode->getNode();
      dbBTerm* bterm = dbBTerm::getBTerm((dbBlock*) block, nodeId);
      uint32_t extId = capId ? capNode->getId() : 0;
      bterm->setExtId(extId);
      continue;
    }

    if (capNode->isITerm()) {
      uint32_t nodeId = capNode->getNode();
      dbITerm* iterm = dbITerm::getITerm((dbBlock*) block, nodeId);
      uint32_t extId = capId ? capNode->getId() : 0;
      iterm->setExtId(extId);
    }
  }
}
void dbNet::set1stCapNodeId(uint32_t capn_id)
{
  _dbNet* net = (_dbNet*) this;
  _dbBlock* block = (_dbBlock*) net->getOwner();
  uint32_t pid = net->cap_nodes_;
  net->cap_nodes_ = capn_id;

  debugPrint(getImpl()->getLogger(),
             utl::ODB,
             "DB_EDIT",
             2,
             "EDIT: {} set 1stCapNode {}",
             net->getDebugName(),
             capn_id);

  if (block->journal_) {
    block->journal_->beginAction(dbJournal::kUpdateField);
    block->journal_->pushParam(dbNetObj);
    block->journal_->pushParam(getId());
    block->journal_->pushParam(_dbNet::kHeadCapNode);
    block->journal_->pushParam(pid);
    block->journal_->pushParam(capn_id);
    block->journal_->endAction();
  }
}

uint32_t dbNet::get1stCapNodeId()
{
  _dbNet* net = (_dbNet*) this;
  return net->cap_nodes_;
}

void dbNet::reverseCCSegs()
{
  for (dbCapNode* node : getCapNodes()) {
    node->getCCSegs().reverse();
  }
}

void dbNet::getSrcCCSegs(std::vector<dbCCSeg*>& S)
{
  for (dbCapNode* node : getCapNodes()) {
    const uint32_t cap_id = node->getImpl()->getOID();
    for (dbCCSeg* seg : node->getCCSegs()) {
      _dbCCSeg* seg_impl = (_dbCCSeg*) seg;
      if (seg_impl->cap_node_[0] == cap_id) {
        S.push_back(seg);
      }
    }
  }
}

void dbNet::getTgtCCSegs(std::vector<dbCCSeg*>& S)
{
  for (dbCapNode* node : getCapNodes()) {
    const uint32_t cap_id = node->getImpl()->getOID();
    for (dbCCSeg* seg : node->getCCSegs()) {
      _dbCCSeg* seg_impl = (_dbCCSeg*) seg;
      if (seg_impl->cap_node_[1] == cap_id) {
        S.push_back(seg);
      }
    }
  }
}

void dbNet::destroyCapNodes(bool cleanExtid)
{
  dbBlock* block = (dbBlock*) getImpl()->getOwner();
  dbSet<dbCapNode> cap_nodes = getCapNodes();
  dbSet<dbCapNode>::iterator itr;

  for (itr = cap_nodes.begin(); itr != cap_nodes.end();) {
    dbCapNode* cn = *itr;
    uint32_t oid = cn->getNode();

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
  for (dbCapNode* n : getCapNodes()) {
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

void dbNet::getCouplingNets(const uint32_t corner,
                            const double ccThreshold,
                            std::set<dbNet*>& cnets)
{
  std::vector<dbNet*> inets;
  std::vector<double> netccap;

  for (dbCapNode* n : getCapNodes()) {
    for (dbCCSeg* cc : n->getCCSegs()) {
      const double cccap = cc->getCapacitance(corner);
      dbNet* tnet = cc->getSourceCapNode()->getNet();
      if (tnet == this) {
        tnet = cc->getTargetCapNode()->getNet();
      }
      if (tnet->isMarked()) {
        for (uint32_t ii = 0; ii < inets.size(); ii++) {
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
  for (uint32_t ii = 0; ii < inets.size(); ii++) {
    if (netccap[ii] >= ccThreshold && cnets.find(inets[ii]) == cnets.end()) {
      cnets.insert(inets[ii]);
    }
    inets[ii]->setMark(false);
  }
}

void dbNet::getGndTotalCap(double* gndcap, double* totalcap, double miller_mult)
{
  dbSigType type = getSigType();
  if (type.isSupply()) {
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
    for (dbCapNode* node : getCapNodes()) {
      if (first) {
        node->getGndTotalCap(gndcap, totalcap, miller_mult);
      } else {
        node->addGndTotalCap(gndcap, totalcap, miller_mult);
      }
      first = false;
    }
  } else {
    for (dbRSeg* rc : rSet) {
      if (first) {
        rc->getGndTotalCap(gndcap, totalcap, miller_mult);
      } else {
        rc->addGndTotalCap(gndcap, totalcap, miller_mult);
      }
      first = false;
    }
  }
}

void dbNet::preExttreeMergeRC(double max_cap, uint32_t corner)
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
  bool firstRC = true;
  uint32_t cnt = 1;
  prc->getGndTotalCap(nullptr, &totalcap[0], 1 /*miller_mult*/);
  for (dbRSeg* rc : rSet) {
    mrsegs.push_back(rc);
    if (firstRC && cnt != 1) {
      rc->getGndTotalCap(nullptr, &totalcap[0], 1 /*miller_mult*/);
    } else {
      rc->addGndTotalCap(nullptr, &totalcap[0], 1 /*miller_mult*/);
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

double dbNet::getTotalCouplingCap(uint32_t corner)
{
  double cap = 0.0;
  for (dbCapNode* n : getCapNodes()) {
    for (dbCCSeg* cc : n->getCCSegs()) {
      cap += cc->getCapacitance(corner);
    }
  }

  return cap;
}

double dbNet::getTotalCapacitance(uint32_t corner, bool cc)
{
  double cap = 0.0;
  double cap1 = 0.0;
  bool foreign = ((dbBlock*) getImpl()->getOwner())->getExtControl()->_foreign;

  if (foreign) {
    for (dbCapNode* node : getCapNodes()) {
      cap1 = node->getCapacitance(corner);
      cap += cap1;
    }
  } else {
    for (dbRSeg* rc : getRSegs()) {
      cap1 = rc->getCapacitance(corner);
      cap += cap1;
    }
  }
  if (cc) {
    cap += getTotalCouplingCap(corner);
  }
  return cap;
}

double dbNet::getTotalResistance(uint32_t corner)
{
  double cap = 0.0;

  for (dbRSeg* rc : getRSegs()) {
    cap += rc->getResistance(corner);
  }
  return cap;
}

void dbNet::setNonDefaultRule(dbTechNonDefaultRule* rule)
{
  _dbNet* net = (_dbNet*) this;
  _dbBlock* block = (_dbBlock*) net->getOwner();
  uint32_t prev_rule = net->non_default_rule_;
  bool prev_block_rule = net->flags_.block_rule;

  if (rule == nullptr) {
    net->non_default_rule_ = 0U;
    net->flags_.block_rule = 0;
  } else {
    net->non_default_rule_ = rule->getImpl()->getOID();
    net->flags_.block_rule = rule->isBlockRule();
  }

  debugPrint(getImpl()->getLogger(),
             utl::ODB,
             "DB_EDIT",
             2,
             "EDIT: {}, setNonDefaultRule: ",
             getDebugName(),
             (rule) ? rule->getImpl()->getOID() : 0);

  if (block->journal_) {
    // block->_journal->updateField(this, _dbNet::NON_DEFAULT_RULE, prev_rule,
    // net->_non_default_rule );
    block->journal_->beginAction(dbJournal::kUpdateField);
    block->journal_->pushParam(rule->getObjectType());
    block->journal_->pushParam(rule->getId());
    block->journal_->pushParam(_dbNet::kNonDefaultRule);
    block->journal_->pushParam(prev_rule);
    block->journal_->pushParam((uint32_t) net->non_default_rule_);
    block->journal_->pushParam(prev_block_rule);
    block->journal_->pushParam((bool) net->flags_.block_rule);
    block->journal_->endAction();
  }
}

dbTechNonDefaultRule* dbNet::getNonDefaultRule()
{
  _dbNet* net = (_dbNet*) this;

  if (net->non_default_rule_ == 0) {
    return nullptr;
  }

  dbDatabase* db = (dbDatabase*) net->getDatabase();

  if (net->flags_.block_rule) {
    _dbBlock* block = (_dbBlock*) net->getOwner();
    return (dbTechNonDefaultRule*) block->non_default_rule_tbl_->getPtr(
        net->non_default_rule_);
  }

  _dbTech* tech = (_dbTech*) db->getTech();
  return (dbTechNonDefaultRule*) tech->non_default_rule_tbl_->getPtr(
      net->non_default_rule_);
}

void dbNet::getSignalWireCount(uint32_t& wireCnt, uint32_t& viaCnt)
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
void dbNet::getNetStats(uint32_t& wireCnt,
                        uint32_t& viaCnt,
                        uint32_t& len,
                        uint32_t& layerCnt,
                        uint32_t* levelTable)
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

      uint32_t level = pshape.shape.getTechLayer()->getRoutingLevel();
      if (levelTable) {
        levelTable[level]++;
      }
      len += std::max(pshape.shape.xMax() - pshape.shape.xMin(),
                      pshape.shape.yMax() - pshape.shape.yMin());
    }
  }
}
void dbNet::getPowerWireCount(uint32_t& wireCnt, uint32_t& viaCnt)
{
  for (dbSWire* swire : getSWires()) {
    for (dbSBox* s : swire->getWires()) {
      if (s->isVia()) {
        viaCnt++;
      } else {
        wireCnt++;
      }
    }
  }
}

void dbNet::getWireCount(uint32_t& wireCnt, uint32_t& viaCnt)
{
  if (getSigType() == dbSigType::POWER || getSigType() == dbSigType::GROUND) {
    getPowerWireCount(wireCnt, viaCnt);
  } else {
    getSignalWireCount(wireCnt, viaCnt);
  }
}

uint32_t dbNet::getITermCount()
{
  return getITerms().size();
}

uint32_t dbNet::getBTermCount()
{
  return getBTerms().size();
}

uint32_t dbNet::getTermCount()
{
  return getITermCount() + getBTermCount();
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

  net->swires_ = 0;
}

dbNet* dbNet::create(dbBlock* block_, const char* name_, bool skipExistingCheck)
{
  _dbBlock* block = (_dbBlock*) block_;

  if (!skipExistingCheck && block->net_hash_.hasMember(name_)) {
    return nullptr;
  }

  _dbNet* net = block->net_tbl_->create();
  if (block->journal_) {
    block->journal_->beginAction(dbJournal::kCreateObject);
    block->journal_->pushParam(dbNetObj);
    block->journal_->pushParam(name_);
    block->journal_->pushParam(net->getOID());
    block->journal_->endAction();
  }

  net->name_ = safe_strdup(name_);
  block->net_hash_.insert(net);

  debugPrint(block->getImpl()->getLogger(),
             utl::ODB,
             "DB_EDIT",
             1,
             "EDIT: create {}",
             net->getDebugName());

  for (auto cb : block->callbacks_) {
    cb->inDbNetCreate((dbNet*) net);
  }

  return (dbNet*) net;
}

dbNet* dbNet::create(dbBlock* block,
                     const char* name,
                     const dbNameUniquifyType& uniquify,
                     dbModule* parent_module)
{
  std::string net_name = block->makeNewNetName(
      parent_module ? parent_module->getModInst() : nullptr, name, uniquify);
  return create(block, net_name.c_str());
}

void dbNet::destroy(dbNet* net_)
{
  _dbNet* net = (_dbNet*) net_;
  _dbBlock* block = (_dbBlock*) net->getOwner();
  dbBlock* dbblock = (dbBlock*) block;

  if (net->flags_.dont_touch) {
    net->getLogger()->error(
        utl::ODB, 364, "Attempt to destroy dont_touch net {}", net->name_);
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

  if (net->wire_ != 0) {
    dbWire* wire = (dbWire*) block->wire_tbl_->getPtr(net->wire_);
    dbWire::destroy(wire);
  }

  for (const dbId<_dbGroup>& _group_id : net->groups_) {
    dbGroup* group = (dbGroup*) block->group_tbl_->getPtr(_group_id);
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

  debugPrint(block->getImpl()->getLogger(),
             utl::ODB,
             "DB_EDIT",
             1,
             "EDIT: delete {}",
             net->getDebugName());

  if (block->journal_) {
    block->journal_->beginAction(dbJournal::kDeleteObject);
    block->journal_->pushParam(dbNetObj);
    block->journal_->pushParam(net_->getName());
    block->journal_->pushParam(net->getOID());
    uint32_t* flags = (uint32_t*) &net->flags_;
    block->journal_->pushParam(*flags);
    block->journal_->pushParam(net->non_default_rule_);
    block->journal_->endAction();
  }

  for (auto cb : block->callbacks_) {
    cb->inDbNetDestroy(net_);
  }

  dbProperty::destroyProperties(net);
  block->net_hash_.remove(net);
  block->net_tbl_->destroy(net);
}

dbSet<dbNet>::iterator dbNet::destroy(dbSet<dbNet>::iterator& itr)
{
  dbNet* bt = *itr;
  dbSet<dbNet>::iterator next = ++itr;
  destroy(bt);
  return next;
}

dbNet* dbNet::getNet(dbBlock* block, uint32_t oid)
{
  _dbBlock* block_impl = (_dbBlock*) block;
  return (dbNet*) block_impl->net_tbl_->getPtr(oid);
}

dbNet* dbNet::getValidNet(dbBlock* block, uint32_t oid)
{
  _dbBlock* block_impl = (_dbBlock*) block;
  if (!block_impl->net_tbl_->validId(oid)) {
    return nullptr;
  }
  return (dbNet*) block_impl->net_tbl_->getPtr(oid);
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

  // 1. Connect all terminals of in_net to this net.

  // in_net->getITerms() returns a terminal iterator, and iterm->connect() can
  // invalidate the iterator by disconnecting a dbITerm.
  // Calling iterm->connect() during iteration with the iterator is not safe.
  // Thus create another vector for safe iterms iteration.
  dbSet<dbITerm> iterms_set = in_net->getITerms();
  boost::container::small_vector<dbITerm*, 16> iterms(iterms_set.begin(),
                                                      iterms_set.end());
  for (dbITerm* iterm : iterms) {
    iterm->connect(this);
  }

  // Create vector for safe iteration.
  dbSet<dbBTerm> bterms_set = in_net->getBTerms();
  boost::container::small_vector<dbBTerm*, 16> bterms(bterms_set.begin(),
                                                      bterms_set.end());
  for (dbBTerm* bterm : bterms) {
    bterm->connect(this);
  }

  for (auto callback : block->callbacks_) {
    callback->inDbNetPostMerge(this, in_net);
  }

  // 2. Destroy in_net
  destroy(in_net);
}

void dbNet::markNets(std::vector<dbNet*>& nets, dbBlock* block, bool mk)
{
  if (nets.empty()) {
    for (dbNet* net : block->getNets()) {
      net->setMark(mk);
    }
  } else {
    for (dbNet* net : nets) {
      net->setMark(mk);
    }
  }
}

dbSet<dbGuide> dbNet::getGuides() const
{
  _dbNet* net = (_dbNet*) this;
  _dbBlock* block = (_dbBlock*) net->getOwner();
  return dbSet<dbGuide>(net, block->guide_itr_);
}

void dbNet::clearGuides()
{
  dbSet<dbGuide> guides = getGuides();
  dbSet<dbGuide>::iterator itr = guides.begin();
  while (itr != guides.end()) {
    itr = dbGuide::destroy(itr);
  }
}

dbSet<dbNetTrack> dbNet::getTracks() const
{
  _dbNet* net = (_dbNet*) this;
  _dbBlock* block = (_dbBlock*) net->getOwner();
  return dbSet<dbNetTrack>(net, block->net_track_itr_);
}

void dbNet::clearTracks()
{
  dbSet<dbNetTrack> tracks = getTracks();
  dbSet<dbNetTrack>::iterator itr = tracks.begin();
  while (itr != tracks.end()) {
    itr = dbNetTrack::destroy(itr);
  }
}

bool dbNet::hasJumpers()
{
  bool has_jumpers = false;
  _dbNet* net = (_dbNet*) this;
  _dbDatabase* db = net->getImpl()->getDatabase();
  if (db->isSchema(kSchemaHasJumpers)) {
    has_jumpers = net->flags_.has_jumpers == 1;
  }
  return has_jumpers;
}

void dbNet::setJumpers(bool has_jumpers)
{
  _dbNet* net = (_dbNet*) this;
  _dbDatabase* db = net->getImpl()->getDatabase();
  if (db->isSchema(kSchemaHasJumpers)) {
    net->flags_.has_jumpers = has_jumpers ? 1 : 0;
  }
}

void dbNet::checkSanity() const
{
  // Check net itself
  std::vector<std::string> drvr_info_list;
  dbUtil::findBTermDrivers(this, drvr_info_list);
  dbUtil::findITermDrivers(this, drvr_info_list);
  dbUtil::checkNetSanity(this, drvr_info_list);

  // Check the consistency with the related dbModNet
  checkSanityModNetConsistency();
}

dbModInst* dbNet::findMainParentModInst() const
{
  dbBlock* block = getBlock();
  const char delim = block->getHierarchyDelimiter();
  const std::string net_name = getName();
  const size_t last_delim_pos = net_name.find_last_of(delim);

  if (last_delim_pos != std::string::npos) {
    const std::string net_parent_hier_name = net_name.substr(0, last_delim_pos);
    return block->findModInst(net_parent_hier_name.c_str());
  }

  return nullptr;
}

dbModule* dbNet::findMainParentModule() const
{
  dbModInst* parent_mod_inst = findMainParentModInst();
  if (parent_mod_inst) {
    return parent_mod_inst->getMaster();
  }

  return getBlock()->getTopModule();
}

bool dbNet::findRelatedModNets(std::set<dbModNet*>& modnet_set) const
{
  modnet_set.clear();

  if (getSigType().isSupply()) {
    return false;
  }

  boost::container::small_vector<dbModNet*, 16> modnets_to_visit;

  // Helper to add a modnet to the result set and the visit queue if it's new.
  auto visitIfNew = [&](dbModNet* modnet) {
    if (modnet && modnet_set.insert(modnet).second) {
      modnets_to_visit.push_back(modnet);
    }
  };

  // Find initial set of modnets from the current dbNet.
  for (dbITerm* iterm : getITerms()) {
    visitIfNew(iterm->getModNet());
  }
  for (dbBTerm* bterm : getBTerms()) {
    visitIfNew(bterm->getModNet());
  }

  // Perform a DFS traversal to find all connected modnets.
  while (!modnets_to_visit.empty()) {
    dbModNet* current_mod_net = modnets_to_visit.back();
    modnets_to_visit.pop_back();

    for (dbModITerm* mod_iterm : current_mod_net->getModITerms()) {
      if (dbModBTerm* mod_bterm = mod_iterm->getChildModBTerm()) {
        visitIfNew(mod_bterm->getModNet());
      }
    }

    for (dbModBTerm* mod_bterm : current_mod_net->getModBTerms()) {
      if (dbModITerm* mod_iterm = mod_bterm->getParentModITerm()) {
        visitIfNew(mod_iterm->getModNet());
      }
    }
  }

  return !modnet_set.empty();
}

void dbNet::dump(bool show_modnets) const
{
  utl::Logger* logger = getImpl()->getLogger();
  logger->report("--------------------------------------------------");
  logger->report("dbNet: {} (id={})", getName(), getId());
  logger->report(
      "  Parent Block: {} (id={})", getBlock()->getName(), getBlock()->getId());
  logger->report("  SigType: {}", getSigType().getString());
  logger->report("  WireType: {}", getWireType().getString());
  if (isSpecial()) {
    logger->report("  Special: true");
  }
  if (isDoNotTouch()) {
    logger->report("  DoNotTouch: true");
  }

  logger->report("  ITerms ({}):", getITerms().size());
  for (dbITerm* term : getITerms()) {
    logger->report("    - {} ({}, {}, id={})",
                   term->getName(),
                   term->getSigType().getString(),
                   term->getIoType().getString(),
                   term->getId());
  }

  logger->report("  BTerms ({}):", getBTerms().size());
  for (dbBTerm* term : getBTerms()) {
    logger->report("    - {} ({}, {}, id={})",
                   term->getName(),
                   term->getSigType().getString(),
                   term->getIoType().getString(),
                   term->getId());
  }
  logger->report("--------------------------------------------------");

  if (show_modnets) {
    std::set<dbModNet*> modnets;
    findRelatedModNets(modnets);
    for (dbModNet* modnet : modnets) {
      modnet->dump();
    }
  }
}

void _dbNet::collectMemInfo(MemInfo& info)
{
  info.cnt++;
  info.size += sizeof(*this);

  info.children["name"].add(name_);
  info.children["groups"].add(groups_);
}

bool dbNet::isDeeperThan(const dbNet* net) const
{
  std::string this_name = getName();
  std::string other_name = net->getName();

  char delim = getBlock()->getHierarchyDelimiter();
  size_t this_depth = std::count(this_name.begin(), this_name.end(), delim);
  size_t other_depth = std::count(other_name.begin(), other_name.end(), delim);

  return (other_depth < this_depth);
}

dbModNet* dbNet::findModNetInHighestHier() const
{
  std::set<dbModNet*> modnets;
  if (findRelatedModNets(modnets) == false) {
    return nullptr;
  }

  dbModNet* highest = nullptr;
  size_t min_delimiters = (size_t) -1;
  char delim = getBlock()->getHierarchyDelimiter();

  // Network::highestConnectedNet(Net *net) compares level of hierarchy and
  // hierarchical net name as a tie breaker.
  // For consistency, this API also uses the hierarchical net name as a tie
  // breaker.
  for (dbModNet* modnet : modnets) {
    std::string name = modnet->getHierarchicalName();
    size_t num_delimiters = std::count(name.begin(), name.end(), delim);
    if (highest == nullptr || num_delimiters < min_delimiters
        || (num_delimiters == min_delimiters
            && name < highest->getHierarchicalName())) {  // name = tie breaker
      min_delimiters = num_delimiters;
      highest = modnet;
    }
  }

  return highest;
}

void dbNet::renameWithModNetInHighestHier()
{
  dbModNet* highest_mod_net = findModNetInHighestHier();
  if (highest_mod_net) {
    rename(highest_mod_net->getHierarchicalName().c_str());
  }
}

bool dbNet::isInternalTo(dbModule* module) const
{
  // If it's connected to any top-level ports (BTerms), it's not internal.
  if (!getBTerms().empty()) {
    return false;
  }

  // Check all instance terminals (ITerms) it's connected to.
  for (dbITerm* iterm : getITerms()) {
    if (iterm->getInst()->getModule() != module) {
      return false;
    }
  }

  return true;
}

void dbNet::checkSanityModNetConsistency() const
{
  bool issued_warning = false;
  utl::Logger* logger = getImpl()->getLogger();

  // 1. Find all related dbModNets with this dbNet.
  std::set<dbModNet*> related_modnets;
  findRelatedModNets(related_modnets);
  if (related_modnets.empty()) {
    return;
  }

  // 2. Find all ITerms and BTerms connected with this dbNet.
  std::set<dbITerm*> flat_iterms;
  for (dbITerm* iterm : getITerms()) {
    flat_iterms.insert(iterm);
  }

  std::set<dbBTerm*> flat_bterms;
  for (dbBTerm* bterm : getBTerms()) {
    flat_bterms.insert(bterm);
  }

  // 3. Find all ITerms and BTerms connected with all the related dbModNets.
  std::set<dbITerm*> hier_iterms;
  std::set<dbBTerm*> hier_bterms;
  for (dbModNet* modnet : related_modnets) {
    for (dbITerm* iterm : modnet->getITerms()) {
      hier_iterms.insert(iterm);
    }
    for (dbBTerm* bterm : modnet->getBTerms()) {
      hier_bterms.insert(bterm);
    }
  }

  // 4. If found any inconsistency, report the difference.

  // 4.1. Compare ITerms
  boost::container::small_vector<dbITerm*, 16> iterms_in_flat_only;
  std::ranges::set_difference(
      flat_iterms, hier_iterms, std::back_inserter(iterms_in_flat_only));

  if (iterms_in_flat_only.empty() == false) {
    issued_warning = true;
    logger->warn(utl::ODB,
                 484,
                 "SanityCheck: dbNet '{}' has ITerms not present in its "
                 "related dbModNets.",
                 getName());
    for (dbITerm* iterm : iterms_in_flat_only) {
      logger->warn(utl::ODB, 485, "  - ITerm: {}", iterm->getName());
    }
  }

  boost::container::small_vector<dbITerm*, 16> iterms_in_hier_only;
  std::ranges::set_difference(
      hier_iterms, flat_iterms, std::back_inserter(iterms_in_hier_only));

  if (iterms_in_hier_only.empty() == false) {
    issued_warning = true;
    logger->warn(utl::ODB,
                 488,
                 "SanityCheck: dbNet '{}' is missing ITerms that are present "
                 "in its related dbModNets.",
                 getName());
    for (dbITerm* iterm : iterms_in_hier_only) {
      logger->warn(utl::ODB,
                   489,
                   "  - ITerm: {} (in hier, not in flat)",
                   iterm->getName());
    }
  }

  // 4.2. Compare BTerms
  boost::container::small_vector<dbBTerm*, 16> bterms_in_flat_only;
  std::ranges::set_difference(
      flat_bterms, hier_bterms, std::back_inserter(bterms_in_flat_only));

  if (bterms_in_flat_only.empty() == false) {
    issued_warning = true;
    logger->warn(utl::ODB,
                 486,
                 "SanityCheck: dbNet '{}' has BTerms not present in its "
                 "related dbModNets.",
                 getName());
    for (dbBTerm* bterm : bterms_in_flat_only) {
      logger->warn(utl::ODB, 487, "  - BTerm: {}", bterm->getName());
    }
  }

  boost::container::small_vector<dbBTerm*, 16> bterms_in_hier_only;
  std::ranges::set_difference(
      hier_bterms, flat_bterms, std::back_inserter(bterms_in_hier_only));

  if (bterms_in_hier_only.empty() == false) {
    issued_warning = true;
    logger->warn(utl::ODB,
                 490,
                 "SanityCheck: dbNet '{}' is missing BTerms that are present "
                 "in its related dbModNets.",
                 getName());
    for (dbBTerm* bterm : bterms_in_hier_only) {
      logger->warn(utl::ODB,
                   491,
                   "  - BTerm: {} (in hier net, not in flat net)",
                   bterm->getName());
    }
  }

  // Print debug information
  if (issued_warning) {
    dump(true);
  }
}

void dbNet::dumpConnectivity(int level) const
{
  utl::Logger* logger = getImpl()->getLogger();
  logger->report("--------------------------------------------------");
  logger->report("Connectivity for dbNet: {} (id={})", getName(), getId());

  std::set<const dbObject*> visited;
  _dbNet::dumpNetConnectivity(this, level, 1, visited, logger);

  std::set<dbModNet*> modnets;
  if (findRelatedModNets(modnets)) {
    for (dbModNet* modnet : modnets) {
      _dbNet::dumpModNetConnectivity(modnet, level, 1, visited, logger);
    }
  }

  logger->report("--------------------------------------------------");
}

void _dbNet::dumpConnectivityRecursive(const dbObject* obj,
                                       int max_level,
                                       int level,
                                       std::set<const dbObject*>& visited,
                                       utl::Logger* logger)
{
  if (level > max_level || obj == nullptr) {
    return;
  }

  std::string details;
  if (obj->getObjectType() == dbITermObj) {
    const dbITerm* iterm = static_cast<const dbITerm*>(obj);
    details = fmt::format(" (master: {}, io: {})",
                          iterm->getInst()->getMaster()->getName(),
                          iterm->getIoType().getString());
  } else if (obj->getObjectType() == dbBTermObj) {
    const dbBTerm* bterm = static_cast<const dbBTerm*>(obj);
    details = fmt::format(" (io: {})", bterm->getIoType().getString());
  } else if (obj->getObjectType() == dbModITermObj) {
    const dbModITerm* moditerm = static_cast<const dbModITerm*>(obj);
    if (dbModBTerm* modbterm = moditerm->getChildModBTerm()) {
      details = fmt::format(" (module: {}, io: {})",
                            moditerm->getParent()->getMaster()->getName(),
                            modbterm->getIoType().getString());
    } else {
      details = fmt::format(" (module: {})",
                            moditerm->getParent()->getMaster()->getName());
    }
  } else if (obj->getObjectType() == dbModBTermObj) {
    const dbModBTerm* modbterm = static_cast<const dbModBTerm*>(obj);
    details = fmt::format(" (io: {})", modbterm->getIoType().getString());
  }

  if (visited.contains(obj)) {
    logger->report("{:>{}}-> {} {} (id={}){}",
                   "",
                   level * 2,
                   obj->getTypeName(),
                   obj->getName(),
                   obj->getId(),
                   details,
                   " [visited]");
    return;
  }

  logger->report("{:>{}}- {} {} (id={}){}",
                 "",
                 level * 2,
                 obj->getTypeName(),
                 obj->getName(),
                 obj->getId(),
                 details);
  visited.insert(obj);

  switch (obj->getObjectType()) {
    case dbNetObj:
      dumpNetConnectivity(static_cast<const dbNet*>(obj),
                          max_level,
                          level + 1,
                          visited,
                          logger);
      break;
    case dbModNetObj:
      dumpModNetConnectivity(static_cast<const odb::dbModNet*>(obj),
                             max_level,
                             level + 1,
                             visited,
                             logger);
      break;
    case dbITermObj: {
      const dbITerm* iterm = static_cast<const dbITerm*>(obj);
      _dbNet::dumpConnectivityRecursive(
          iterm->getNet(), max_level, level + 1, visited, logger);
      _dbNet::dumpConnectivityRecursive(
          iterm->getModNet(), max_level, level + 1, visited, logger);
      _dbNet::dumpConnectivityRecursive(
          iterm->getInst(), max_level, level + 1, visited, logger);
      break;
    }
    case dbBTermObj: {
      const dbBTerm* bterm = static_cast<const dbBTerm*>(obj);
      _dbNet::dumpConnectivityRecursive(
          bterm->getNet(), max_level, level + 1, visited, logger);
      _dbNet::dumpConnectivityRecursive(
          bterm->getModNet(), max_level, level + 1, visited, logger);
      break;
    }
    case dbModITermObj: {
      const dbModITerm* moditerm = static_cast<const dbModITerm*>(obj);
      _dbNet::dumpConnectivityRecursive(
          moditerm->getModNet(), max_level, level + 1, visited, logger);
      _dbNet::dumpConnectivityRecursive(
          moditerm->getParent(), max_level, level + 1, visited, logger);
      break;
    }
    case dbModBTermObj: {
      const dbModBTerm* modbterm = static_cast<const dbModBTerm*>(obj);
      _dbNet::dumpConnectivityRecursive(
          modbterm->getModNet(), max_level, level + 1, visited, logger);
      break;
    }
    case dbInstObj: {
      const dbInst* inst = static_cast<const dbInst*>(obj);
      for (dbITerm* iterm : inst->getITerms()) {
        _dbNet::dumpConnectivityRecursive(
            iterm, max_level, level + 1, visited, logger);
      }
      break;
    }
    default:
      // Not an object type we traverse from, do nothing.
      break;
  }
}

void _dbNet::dumpNetConnectivity(const dbNet* net,
                                 int max_level,
                                 int level,
                                 std::set<const dbObject*>& visited,
                                 utl::Logger* logger)
{
  boost::container::small_vector<const dbObject*, 16> inputs;
  boost::container::small_vector<const dbObject*, 16> outputs;
  boost::container::small_vector<const dbObject*, 16> others;

  for (dbITerm* iterm : net->getITerms()) {
    if (iterm->getIoType() == dbIoType::INPUT) {
      inputs.push_back(iterm);
    } else if (iterm->getIoType() == dbIoType::OUTPUT) {
      outputs.push_back(iterm);
    } else {
      others.push_back(iterm);
    }
  }
  for (dbBTerm* bterm : net->getBTerms()) {
    if (bterm->getIoType() == dbIoType::INPUT) {
      inputs.push_back(bterm);
    } else if (bterm->getIoType() == dbIoType::OUTPUT) {
      outputs.push_back(bterm);
    } else {
      others.push_back(bterm);
    }
  }

  for (const dbObject* in_term : inputs) {
    _dbNet::dumpConnectivityRecursive(
        in_term, max_level, level, visited, logger);
  }
  for (const dbObject* other_term : others) {
    _dbNet::dumpConnectivityRecursive(
        other_term, max_level, level, visited, logger);
  }
  for (const dbObject* out_term : outputs) {
    _dbNet::dumpConnectivityRecursive(
        out_term, max_level, level, visited, logger);
  }
}

void _dbNet::dumpModNetConnectivity(const dbModNet* modnet,
                                    int max_level,
                                    int level,
                                    std::set<const dbObject*>& visited,
                                    utl::Logger* logger)
{
  boost::container::small_vector<const dbObject*, 16> inputs;
  boost::container::small_vector<const dbObject*, 16> outputs;
  boost::container::small_vector<const dbObject*, 16> others;

  auto classifyTerm = [&](const auto* term) {
    dbIoType io_type;
    if constexpr (std::is_same_v<std::decay_t<decltype(*term)>, dbITerm>
                  || std::is_same_v<std::decay_t<decltype(*term)>, dbBTerm>) {
      io_type = term->getIoType();
    } else if constexpr (std::is_same_v<std::decay_t<decltype(*term)>,
                                        dbModITerm>) {
      if (dbModBTerm* bterm = term->getChildModBTerm()) {
        io_type = bterm->getIoType();
      } else {
        others.push_back(term);
        return;
      }
    } else if constexpr (std::is_same_v<std::decay_t<decltype(*term)>,
                                        dbModBTerm>) {
      io_type = term->getIoType();
    }

    if (io_type == dbIoType::INPUT) {
      inputs.push_back(term);
    } else if (io_type == dbIoType::OUTPUT) {
      outputs.push_back(term);
    } else {
      others.push_back(term);
    }
  };

  for (dbITerm* iterm : modnet->getITerms()) {
    classifyTerm(iterm);
  }
  for (dbBTerm* bterm : modnet->getBTerms()) {
    classifyTerm(bterm);
  }
  for (dbModITerm* moditerm : modnet->getModITerms()) {
    classifyTerm(moditerm);
  }
  for (dbModBTerm* modbterm : modnet->getModBTerms()) {
    classifyTerm(modbterm);
  }

  for (const dbObject* in_term : inputs) {
    _dbNet::dumpConnectivityRecursive(
        in_term, max_level, level, visited, logger);
  }
  for (const dbObject* other_term : others) {
    _dbNet::dumpConnectivityRecursive(
        other_term, max_level, level, visited, logger);
  }
  for (const dbObject* out_term : outputs) {
    _dbNet::dumpConnectivityRecursive(
        out_term, max_level, level, visited, logger);
  }
}

dbInst* dbNet::insertBufferBeforeLoad(dbObject* load_input_term,
                                      const dbMaster* buffer_master,
                                      const Point* loc,
                                      const char* new_buf_base_name,
                                      const char* new_net_base_name,
                                      const dbNameUniquifyType& uniquify)
{
  dbInsertBuffer insert_buffer(this);
  return insert_buffer.insertBufferBeforeLoad(load_input_term,
                                              buffer_master,
                                              loc,
                                              new_buf_base_name,
                                              new_net_base_name,
                                              uniquify);
}

dbInst* dbNet::insertBufferAfterDriver(dbObject* drvr_output_term,
                                       const dbMaster* buffer_master,
                                       const Point* loc,
                                       const char* new_buf_base_name,
                                       const char* new_net_base_name,
                                       const dbNameUniquifyType& uniquify)
{
  dbInsertBuffer insert_buffer(this);
  return insert_buffer.insertBufferAfterDriver(drvr_output_term,
                                               buffer_master,
                                               loc,
                                               new_buf_base_name,
                                               new_net_base_name,
                                               uniquify);
}

dbInst* dbNet::insertBufferBeforeLoads(const std::set<dbObject*>& load_pins,
                                       const dbMaster* buffer_master,
                                       const Point* loc,
                                       const char* new_buf_base_name,
                                       const char* new_net_base_name,
                                       const dbNameUniquifyType& uniquify,
                                       bool loads_on_diff_nets)
{
  dbInsertBuffer insert_buffer(this);
  return insert_buffer.insertBufferBeforeLoads(load_pins,
                                               buffer_master,
                                               loc,
                                               new_buf_base_name,
                                               new_net_base_name,
                                               uniquify,
                                               loads_on_diff_nets);
}

dbInst* dbNet::insertBufferBeforeLoads(const std::vector<dbObject*>& load_pins,
                                       const dbMaster* buffer_master,
                                       const Point* loc,
                                       const char* new_buf_base_name,
                                       const char* new_net_base_name,
                                       const dbNameUniquifyType& uniquify,
                                       bool loads_on_diff_nets)
{
  std::set<dbObject*> load_pins_set(load_pins.begin(), load_pins.end());
  return insertBufferBeforeLoads(load_pins_set,
                                 buffer_master,
                                 loc,
                                 new_buf_base_name,
                                 new_net_base_name,
                                 uniquify,
                                 loads_on_diff_nets);
}

void dbNet::hierarchicalConnect(dbObject* driver, dbObject* load)
{
  dbInsertBuffer insert_buffer(this);
  insert_buffer.hierarchicalConnect(driver, load);
}

}  // namespace odb
