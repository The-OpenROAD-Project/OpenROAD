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

#include "db.h"
#include "dbBTerm.h"
#include "dbBTermItr.h"
#include "dbBlock.h"
#include "dbBlockCallBackObj.h"
#include "dbCCSeg.h"
#include "dbCCSegItr.h"
#include "dbCapNode.h"
#include "dbCapNodeItr.h"
#include "dbCommon.h"
#include "dbDatabase.h"
#include "dbDiff.hpp"
#include "dbExtControl.h"
#include "dbGroup.h"
#include "dbITerm.h"
#include "dbITermItr.h"
#include "dbInst.h"
#include "dbJournal.h"
#include "dbMTerm.h"
#include "dbRSeg.h"
#include "dbRSegItr.h"
#include "dbSWire.h"
#include "dbSWireItr.h"
#include "dbSet.h"
#include "dbShape.h"
#include "dbTable.h"
#include "dbTable.hpp"
#include "dbTech.h"
#include "dbTechNonDefaultRule.h"
#include "dbWire.h"
#include "utl/Logger.h"

namespace odb {

template class dbTable<_dbNet>;
static void set_symmetric_diff(dbDiff& diff,
                               std::vector<_dbBTerm*>& lhs,
                               std::vector<_dbBTerm*>& rhs);
static void set_symmetric_diff(dbDiff& diff,
                               std::vector<_dbITerm*>& lhs,
                               std::vector<_dbITerm*>& rhs);

_dbNet::_dbNet(_dbDatabase*, const _dbNet& n)
    : _flags(n._flags),
      _name(NULL),
      _next_entry(n._next_entry),
      _iterms(n._iterms),
      _bterms(n._bterms),
      _wire(n._wire),
      _global_wire(n._global_wire),
      _swires(n._swires),
      _cap_nodes(n._cap_nodes),
      _r_segs(n._r_segs),
      _non_default_rule(n._non_default_rule),
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

_dbNet::_dbNet(_dbDatabase*)
{
  _flags._sig_type = dbSigType::SIGNAL;
  _flags._wire_type = dbWireType::ROUTED;
  _flags._special = 0;
  _flags._wild_connect = 0;
  _flags._wire_ordered = 0;
  _flags._buffered = 0;
  _flags._disconnected = 0;
  _flags._spef = 0;
  _flags._select = 0;
  _flags._mark = 0;
  _flags._mark_1 = 0;
  _flags._wire_altered = 0;
  _flags._extracted = 0;
  _flags._rc_graph = 0;
  _flags._reduced = 0;
  _flags._set_io = 0;
  _flags._io = 0;
  _flags._dont_touch = 0;
  _flags._size_only = 0;
  _flags._fixed_bump = 0;
  _flags._source = dbSourceType::NONE;
  _flags._rc_disconnected = 0;
  _flags._block_rule = 0;
  _name = 0;
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
  if (_name)
    free((void*) _name);
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

  return stream;
}

bool _dbNet::operator<(const _dbNet& rhs) const
{
  return strcmp(_name, rhs._name) < 0;
}

bool _dbNet::operator==(const _dbNet& rhs) const
{
  if (_flags._sig_type != rhs._flags._sig_type)
    return false;

  if (_flags._wire_type != rhs._flags._wire_type)
    return false;

  if (_flags._special != rhs._flags._special)
    return false;

  if (_flags._wild_connect != rhs._flags._wild_connect)
    return false;

  if (_flags._wire_ordered != rhs._flags._wire_ordered)
    return false;

  if (_flags._buffered != rhs._flags._buffered)
    return false;

  if (_flags._disconnected != rhs._flags._disconnected)
    return false;

  if (_flags._spef != rhs._flags._spef)
    return false;

  if (_flags._select != rhs._flags._select)
    return false;

  if (_flags._mark != rhs._flags._mark)
    return false;

  if (_flags._mark_1 != rhs._flags._mark_1)
    return false;

  if (_flags._wire_altered != rhs._flags._wire_altered)
    return false;

  if (_flags._extracted != rhs._flags._extracted)
    return false;

  if (_flags._rc_graph != rhs._flags._rc_graph)
    return false;

  if (_flags._reduced != rhs._flags._reduced)
    return false;

  if (_flags._set_io != rhs._flags._set_io)
    return false;

  if (_flags._io != rhs._flags._io)
    return false;

  if (_flags._dont_touch != rhs._flags._dont_touch)
    return false;

  if (_flags._size_only != rhs._flags._size_only)
    return false;

  if (_flags._fixed_bump != rhs._flags._fixed_bump)
    return false;

  if (_flags._source != rhs._flags._source)
    return false;

  if (_flags._rc_disconnected != rhs._flags._rc_disconnected)
    return false;

  if (_flags._block_rule != rhs._flags._block_rule)
    return false;

  if (_name && rhs._name) {
    if (strcmp(_name, rhs._name) != 0)
      return false;
  } else if (_name || rhs._name)
    return false;

  if (_gndc_calibration_factor != rhs._gndc_calibration_factor)
    return false;
  if (_cc_calibration_factor != rhs._cc_calibration_factor)
    return false;

  if (_next_entry != rhs._next_entry)
    return false;

  if (_iterms != rhs._iterms)
    return false;

  if (_bterms != rhs._bterms)
    return false;

  if (_wire != rhs._wire)
    return false;

  if (_global_wire != rhs._global_wire)
    return false;

  if (_swires != rhs._swires)
    return false;

  if (_cap_nodes != rhs._cap_nodes)
    return false;

  if (_r_segs != rhs._r_segs)
    return false;

  if (_non_default_rule != rhs._non_default_rule)
    return false;

  if (_weight != rhs._weight)
    return false;

  if (_xtalk != rhs._xtalk)
    return false;

  if (_ccAdjustFactor != rhs._ccAdjustFactor)
    return false;

  if (_ccAdjustOrder != rhs._ccAdjustOrder)
    return false;

  if (_groups != rhs._groups)
    return false;

  return true;
}

void _dbNet::differences(dbDiff& diff,
                         const char* field,
                         const _dbNet& rhs) const
{
  _dbBlock* lhs_block = (_dbBlock*) getOwner();
  _dbBlock* rhs_block = (_dbBlock*) rhs.getOwner();

  DIFF_BEGIN
  DIFF_FIELD(_name);
  DIFF_FIELD(_flags._sig_type);
  DIFF_FIELD(_flags._wire_type);
  DIFF_FIELD(_flags._special);
  DIFF_FIELD(_flags._wild_connect);
  DIFF_FIELD(_flags._wire_ordered);
  DIFF_FIELD(_flags._buffered);
  DIFF_FIELD(_flags._disconnected);
  DIFF_FIELD(_flags._spef);
  DIFF_FIELD(_flags._select);
  DIFF_FIELD(_flags._mark);
  DIFF_FIELD(_flags._mark_1);
  DIFF_FIELD(_flags._wire_altered);
  DIFF_FIELD(_flags._extracted);
  DIFF_FIELD(_flags._rc_graph);
  DIFF_FIELD(_flags._reduced);
  DIFF_FIELD(_flags._set_io);
  DIFF_FIELD(_flags._io);
  DIFF_FIELD(_flags._dont_touch);
  DIFF_FIELD(_flags._size_only);
  DIFF_FIELD(_flags._fixed_bump);
  DIFF_FIELD(_flags._source);
  DIFF_FIELD(_flags._rc_disconnected);
  DIFF_FIELD(_flags._block_rule);
  DIFF_FIELD_NO_DEEP(_gndc_calibration_factor);
  DIFF_FIELD_NO_DEEP(_cc_calibration_factor);
  DIFF_FIELD_NO_DEEP(_next_entry);

  if (!diff.deepDiff()) {
    DIFF_FIELD(_bterms);
  } else {
    dbSet<_dbBTerm>::iterator itr;

    dbSet<_dbBTerm> lhs_set((dbObject*) this, lhs_block->_net_bterm_itr);
    std::vector<_dbBTerm*> lhs_vec;

    for (itr = lhs_set.begin(); itr != lhs_set.end(); ++itr)
      lhs_vec.push_back(*itr);

    dbSet<_dbBTerm> rhs_set((dbObject*) &rhs, rhs_block->_net_bterm_itr);
    std::vector<_dbBTerm*> rhs_vec;

    for (itr = rhs_set.begin(); itr != rhs_set.end(); ++itr)
      rhs_vec.push_back(*itr);

    set_symmetric_diff(diff, lhs_vec, rhs_vec);
  }

  if (!diff.deepDiff()) {
    DIFF_FIELD(_iterms);
  } else {
    dbSet<_dbITerm>::iterator itr;

    dbSet<_dbITerm> lhs_set((dbObject*) this, lhs_block->_net_iterm_itr);
    std::vector<_dbITerm*> lhs_vec;

    for (itr = lhs_set.begin(); itr != lhs_set.end(); ++itr)
      lhs_vec.push_back(*itr);

    dbSet<_dbITerm> rhs_set((dbObject*) &rhs, rhs_block->_net_iterm_itr);
    std::vector<_dbITerm*> rhs_vec;

    for (itr = rhs_set.begin(); itr != rhs_set.end(); ++itr)
      rhs_vec.push_back(*itr);

    set_symmetric_diff(diff, lhs_vec, rhs_vec);
  }

  DIFF_OBJECT(_wire, lhs_block->_wire_tbl, rhs_block->_wire_tbl);
  DIFF_OBJECT(_global_wire, lhs_block->_wire_tbl, rhs_block->_wire_tbl);
  DIFF_SET(_swires, lhs_block->_swire_itr, rhs_block->_swire_itr);
  DIFF_SET(_cap_nodes, lhs_block->_cap_node_itr, rhs_block->_cap_node_itr);
  DIFF_SET(_r_segs, lhs_block->_r_seg_itr, rhs_block->_r_seg_itr);
  DIFF_FIELD(_non_default_rule);
  DIFF_FIELD(_weight);
  DIFF_FIELD(_xtalk);
  DIFF_FIELD(_ccAdjustFactor);
  DIFF_FIELD(_ccAdjustOrder);
  DIFF_VECTOR(_groups);
  DIFF_END
}

void _dbNet::out(dbDiff& diff, char side, const char* field) const
{
  _dbBlock* block = (_dbBlock*) getOwner();

  DIFF_OUT_BEGIN
  DIFF_OUT_FIELD(_name);
  DIFF_OUT_FIELD(_flags._sig_type);
  DIFF_OUT_FIELD(_flags._wire_type);
  DIFF_OUT_FIELD(_flags._special);
  DIFF_OUT_FIELD(_flags._wild_connect);
  DIFF_OUT_FIELD(_flags._wire_ordered);
  DIFF_OUT_FIELD(_flags._buffered);
  DIFF_OUT_FIELD(_flags._disconnected);
  DIFF_OUT_FIELD(_flags._spef);
  DIFF_OUT_FIELD(_flags._select);
  DIFF_OUT_FIELD(_flags._mark);
  DIFF_OUT_FIELD(_flags._mark_1);
  DIFF_OUT_FIELD(_flags._wire_altered);
  DIFF_OUT_FIELD(_flags._extracted);
  DIFF_OUT_FIELD(_flags._rc_graph);
  DIFF_OUT_FIELD(_flags._reduced);
  DIFF_OUT_FIELD(_flags._set_io);
  DIFF_OUT_FIELD(_flags._io);
  DIFF_OUT_FIELD(_flags._dont_touch);
  DIFF_OUT_FIELD(_flags._size_only);
  DIFF_OUT_FIELD(_flags._fixed_bump);
  DIFF_OUT_FIELD(_flags._source);
  DIFF_OUT_FIELD(_flags._rc_disconnected);
  DIFF_OUT_FIELD(_flags._block_rule);
  DIFF_OUT_FIELD_NO_DEEP(_gndc_calibration_factor);
  DIFF_OUT_FIELD_NO_DEEP(_cc_calibration_factor);
  DIFF_OUT_FIELD_NO_DEEP(_next_entry);

  if (!diff.deepDiff()) {
    DIFF_OUT_FIELD(_bterms);
  } else {
    dbSet<_dbBTerm>::iterator itr;
    dbSet<_dbBTerm> bterms((dbObject*) this, block->_net_bterm_itr);
    diff.begin_object("%c _bterms\n", side);

    for (itr = bterms.begin(); itr != bterms.end(); ++itr)
      diff.report("%c %s\n", side, (*itr)->_name);

    diff.end_object();
  }

  if (!diff.deepDiff()) {
    DIFF_OUT_FIELD(_iterms);
  } else {
    dbSet<_dbITerm>::iterator itr;
    dbSet<_dbITerm> iterms((dbObject*) this, block->_net_iterm_itr);
    diff.begin_object("%c _iterms\n", side);

    for (itr = iterms.begin(); itr != iterms.end(); ++itr) {
      _dbITerm* it = *itr;
      _dbInst* inst = it->getInst();
      _dbMTerm* mt = it->getMTerm();
      diff.report("%c (%s %s)\n", side, inst->_name, mt->_name);
    }

    diff.end_object();
  }

  DIFF_OUT_OBJECT(_wire, block->_wire_tbl);
  DIFF_OUT_OBJECT(_global_wire, block->_wire_tbl);
  DIFF_OUT_SET(_swires, block->_swire_itr);
  DIFF_OUT_SET(_cap_nodes, block->_cap_node_itr);
  DIFF_OUT_SET(_r_segs, block->_r_seg_itr);
  DIFF_OUT_FIELD(_non_default_rule);
  DIFF_OUT_FIELD(_weight);
  DIFF_OUT_FIELD(_xtalk);
  DIFF_OUT_FIELD(_ccAdjustFactor);
  DIFF_OUT_FIELD(_ccAdjustOrder);
  DIFF_OUT_VECTOR(_groups);
  DIFF_END
}

void set_symmetric_diff(dbDiff& diff,
                        std::vector<_dbBTerm*>& lhs,
                        std::vector<_dbBTerm*>& rhs)
{
  diff.begin_object("<> _bterms\n");

  std::sort(lhs.begin(), lhs.end(), dbDiffCmp<_dbBTerm>());
  std::sort(rhs.begin(), rhs.end(), dbDiffCmp<_dbBTerm>());

  std::vector<_dbBTerm*>::iterator end;
  std::vector<_dbBTerm*> symmetric_diff;

  symmetric_diff.resize(lhs.size() + rhs.size());

  end = std::set_symmetric_difference(lhs.begin(),
                                      lhs.end(),
                                      rhs.begin(),
                                      rhs.end(),
                                      symmetric_diff.begin(),
                                      dbDiffCmp<_dbBTerm>());

  std::vector<_dbBTerm*>::iterator i1 = lhs.begin();
  std::vector<_dbBTerm*>::iterator i2 = rhs.begin();
  std::vector<_dbBTerm*>::iterator sd = symmetric_diff.begin();

  while ((i1 != lhs.end()) && (i2 != rhs.end())) {
    _dbBTerm* o1 = *i1;
    _dbBTerm* o2 = *i2;

    if (o1 == *sd) {
      diff.report("%c %s\n", dbDiff::LEFT, o1->_name);
      ++i1;
      ++sd;
    } else if (o2 == *sd) {
      diff.report("%c %s\n", dbDiff::RIGHT, o2->_name);
      ++i2;
      ++sd;
    } else  // equal keys
    {
      ++i1;
      ++i2;
    }
  }

  for (; i1 != lhs.end(); ++i1) {
    _dbBTerm* o1 = *i1;
    diff.report("%c %s\n", dbDiff::LEFT, o1->_name);
  }

  for (; i2 != rhs.end(); ++i2) {
    _dbBTerm* o2 = *i2;
    diff.report("%c %s\n", dbDiff::RIGHT, o2->_name);
  }

  diff.end_object();
}

void set_symmetric_diff(dbDiff& diff,
                        std::vector<_dbITerm*>& lhs,
                        std::vector<_dbITerm*>& rhs)
{
  diff.begin_object("<> _iterms\n");

  std::sort(lhs.begin(), lhs.end(), dbDiffCmp<_dbITerm>());
  std::sort(rhs.begin(), rhs.end(), dbDiffCmp<_dbITerm>());

  std::vector<_dbITerm*>::iterator end;
  std::vector<_dbITerm*> symmetric_diff;

  symmetric_diff.resize(lhs.size() + rhs.size());

  end = std::set_symmetric_difference(lhs.begin(),
                                      lhs.end(),
                                      rhs.begin(),
                                      rhs.end(),
                                      symmetric_diff.begin(),
                                      dbDiffCmp<_dbITerm>());

  std::vector<_dbITerm*>::iterator i1 = lhs.begin();
  std::vector<_dbITerm*>::iterator i2 = rhs.begin();
  std::vector<_dbITerm*>::iterator sd = symmetric_diff.begin();

  while ((i1 != lhs.end()) && (i2 != rhs.end())) {
    _dbITerm* o1 = *i1;
    _dbITerm* o2 = *i2;

    if (o1 == *sd) {
      _dbInst* inst = o1->getInst();
      _dbMTerm* mterm = o1->getMTerm();
      diff.report("%c (%s %s)\n", dbDiff::LEFT, inst->_name, mterm->_name);
      ++i1;
      ++sd;
    } else if (o2 == *sd) {
      _dbInst* inst = o2->getInst();
      _dbMTerm* mterm = o2->getMTerm();
      diff.report("%c (%s %s)\n", dbDiff::RIGHT, inst->_name, mterm->_name);
      ++i2;
      ++sd;
    } else  // equal keys
    {
      ++i1;
      ++i2;
    }
  }

  for (; i1 != lhs.end(); ++i1) {
    _dbITerm* o1 = *i1;
    _dbInst* inst = o1->getInst();
    _dbMTerm* mterm = o1->getMTerm();
    diff.report("%c (%s %s)\n", dbDiff::LEFT, inst->_name, mterm->_name);
  }

  for (; i2 != rhs.end(); ++i2) {
    _dbITerm* o2 = *i2;
    _dbInst* inst = o2->getInst();
    _dbMTerm* mterm = o2->getMTerm();
    diff.report("%c (%s %s)\n", dbDiff::RIGHT, inst->_name, mterm->_name);
  }

  diff.end_object();
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
  if (idFlag)
    fprintf(fp, " %d", getId());

  _dbNet* net = (_dbNet*) this;
  fprintf(fp, " %s", net->_name);

  if (newLine)
    fprintf(fp, "\n");
}
bool dbNet::rename(const char* name)
{
  _dbNet* net = (_dbNet*) this;
  _dbBlock* block = (_dbBlock*) net->getOwner();

  if (block->_net_hash.hasMember(name))
    return false;

  block->_net_hash.remove(net);
  free((void*) net->_name);
  net->_name = strdup(name);
  ZALLOCATED(net->_name);
  block->_net_hash.insert(net);

  return true;
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
    if (factor == 1.0)
      continue;
    if (corner < 0)
      cc->adjustCapacitance(factor);
    else
      cc->adjustCapacitance(factor, corner);
  }
}

void dbNet::calibrateCouplingCap()
{
  calibrateCouplingCap(-1);
}

bool dbNet::anchoredRSeg()
{
  dbSet<dbRSeg> rSet = getRSegs();
  dbSet<dbRSeg>::iterator rc_itr;
  dbRSeg* rc = NULL;
  for (rc_itr = rSet.begin(); rc_itr != rSet.end(); ++rc_itr) {
    rc = *rc_itr;
    if (rc->getShapeId() != 0)
      return true;
  }
  return false;
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
    if (!capn->isInternal())
      continue;

    uint n = capn->getNode();
    if (max_n < n)
      max_n = n;
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
      if (mapFile)
        fprintf(mapFile, "    %8d : %8d\n", capn->getNode(), cnt);
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
    if (node->needAdjustCC(ccThreshHold))
      needAdjust = true;
  }
  if (!needAdjust)
    return false;

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
  if (((_dbNet*) this)->_ccAdjustFactor < 0)
    return;
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
  if (factor == 1.0)
    return;
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
  if (factor == 1.0)
    return;
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
  if (factor == 1.0)
    return;
  dbSet<dbRSeg> rSet = getRSegs();
  dbSet<dbRSeg>::iterator rc_itr;
  dbRSeg* rc = NULL;
  for (rc_itr = rSet.begin(); rc_itr != rSet.end(); ++rc_itr) {
    rc = *rc_itr;
    rc->adjustResistance(factor, corner);
  }
}
void dbNet::adjustNetRes(float factor)
{
  if (factor == 1.0)
    return;
  dbSet<dbRSeg> rSet = getRSegs();
  dbSet<dbRSeg>::iterator rc_itr;
  dbRSeg* rc = NULL;
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
        || path.point.getY() > bbox->yMax() || path.point.getY() < bbox->yMin())
      return false;
    cnt++;
    if (cnt >= 4)
      return true;
    while (pitr.getNextShape(pathShape)) {
      if (pathShape.point.getX() > bbox->xMax()
          || pathShape.point.getX() < bbox->xMin()
          || pathShape.point.getY() > bbox->yMax()
          || pathShape.point.getY() < bbox->yMin())
        return false;
      cnt++;
      if (cnt >= 4)
        return true;
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

uint dbNet::wireEqual(dbNet* target)
{
  dbWire* srcw = getWire();
  dbWire* tgtw = target->getWire();
  if (srcw == NULL && tgtw == NULL)
    return 0;
  if (srcw == NULL || tgtw == NULL)
    return 3;
  if (!isWireOrdered() || !target->isWireOrdered())
    return 4;
  return (srcw->equal(tgtw));
}

void dbNet::wireMatch(dbNet* target)
{
  dbWire* srcw = getWire();
  dbWire* tgtw = target->getWire();
  if (srcw == NULL && tgtw == NULL)
    return;
  if (srcw == NULL || tgtw == NULL)
    return;
  if (!isWireOrdered() || !target->isWireOrdered())
    return;
  /************************************************ dimitris_fix LOOK_AGAIN */
  // srcw->match(tgtw);
}

void dbNet::donateWire(dbNet* tnet, dbRSeg** new_rsegs)
{
  dbWire* wire = getWire();

  if (!wire || wire->length() == 0)
    return;

  dbWire* twire;

  if (tnet == this)  // discard iterm and bterm by donate
  {
    wire->detach();
    twire = dbWire::create(this);
    wire->donateWireSeg(twire, new_rsegs);
    dbWire::destroy(wire);
  } else {
    twire = tnet->getWire();

    if (!twire)
      twire = dbWire::create(tnet);

    wire->donateWireSeg(twire, new_rsegs);
  }
}

void dbNet::printWire(int fid, int tid, char* type)
{
  FILE* fp;
  char fn[40];
  if (type) {
    sprintf(fn, "%s%d", type, getId());
    fp = fopen(fn, "w");
  } else
    fp = stdout;
  fprintf(fp, "dbWire of Net %d %s :\n", getId(), getConstName());
  if (getWire() && getWire()->length())
    getWire()->printWire(fp, fid, tid);
  if (fp != stdout)
    fclose(fp);
}

void dbNet::printWire()
{
  printWire(0, 0, NULL);
}

void dbNet::printWire(char* type)
{
  printWire(0, 0, type);
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

bool dbNet::isBuffered()
{
  _dbNet* net = (_dbNet*) this;
  return net->_flags._buffered == 1;
}

void dbNet::setBuffered(bool value)
{
  _dbNet* net = (_dbNet*) this;

  _dbBlock* block = (_dbBlock*) net->getOwner();
  uint prev_flags = flagsToUInt(net);

  net->_flags._buffered = (value == true) ? 1 : 0;

  if (block->_journal) {
    debugPrint(getImpl()->getLogger(),
               utl::ODB,
               "DB_ECO",
               1,
               "ECO: net {}, setBuffered: {}",
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
  // dimitri_fix : LOOK_AGAIN for FULL_ECO mode uint prev_flags =
  // flagsToUInt(net);

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
  // dimitri_fix : LOOK_AGAIN for FULL_ECO mode uint prev_flags =
  // flagsToUInt(net);

  net->_flags._wire_altered = (value == true) ? 1 : 0;
  if (value)
    net->_flags._wire_ordered = 0;

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
  // dimitri_fix : LOOK_AGAIN for FULL_ECO mode uint prev_flags =
  // flagsToUInt(net);

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

void dbNet::setReduced(bool value)
{
  _dbNet* net = (_dbNet*) this;
  _dbBlock* block = (_dbBlock*) net->getOwner();
  uint prev_flags = flagsToUInt(net);
  net->_flags._reduced = (value == true) ? 1 : 0;

  if (block->_journal) {
    debugPrint(getImpl()->getLogger(),
               utl::ODB,
               "DB_ECO",
               1,
               "ECO: net {}, setReduced: {}",
               getId(),
               value);
    block->_journal->updateField(
        this, _dbNet::FLAGS, prev_flags, flagsToUInt(net));
  }
}

bool dbNet::isReduced()
{
  _dbNet* net = (_dbNet*) this;
  return net->_flags._reduced == 1;
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
  ;
  dbSet<dbITerm>::iterator iitr = iterms.begin();
  dbITerm* it = *iitr;
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
  ;
  dbSet<dbBTerm>::iterator bitr = bterms.begin();
  dbBTerm* bt = *bitr;
  return bt;
}
dbITerm* dbNet::getFirstOutput()
{
  if (getDrivingITerm() > 0)
    return dbITerm::getITerm((dbBlock*) getImpl()->getOwner(),
                             getDrivingITerm());

  dbSet<dbITerm> iterms = getITerms();
  dbSet<dbITerm>::iterator iitr;

  for (iitr = iterms.begin(); iitr != iterms.end(); ++iitr) {
    dbITerm* tr = *iitr;

    if ((tr->getSigType() == dbSigType::GROUND)
        || (tr->getSigType() == dbSigType::POWER))
      continue;

    if (tr->isClocked())
      continue;

    if (tr->getIoType() != dbIoType::OUTPUT)
      continue;

    return tr;
  }
  // warning(0, "instance %s has no output pin\n", getConstName());
  return NULL;
}
dbITerm* dbNet::get1stSignalInput(bool io)
{
  dbSet<dbITerm> iterms = getITerms();
  dbSet<dbITerm>::iterator iitr;

  for (iitr = iterms.begin(); iitr != iterms.end(); ++iitr) {
    dbITerm* tr = *iitr;

    if ((tr->getSigType() == dbSigType::GROUND)
        || (tr->getSigType() == dbSigType::POWER))
      continue;

    if (tr->getIoType() != dbIoType::INPUT)
      continue;

    if (io && (tr->getIoType() != dbIoType::INOUT))
      continue;

    return tr;
  }
  // warning(0, "instance %s has no output pin\n", getConstName());
  return NULL;
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

  if (net->_swires == 0)
    return NULL;

  return (dbSWire*) block->_swire_tbl->getPtr(net->_swires);
}

dbWire* dbNet::getWire()
{
  _dbNet* net = (_dbNet*) this;
  _dbBlock* block = (_dbBlock*) net->getOwner();

  if (net->_wire == 0)
    return NULL;

  return (dbWire*) block->_wire_tbl->getPtr(net->_wire);
}

dbWire* dbNet::getGlobalWire()
{
  _dbNet* net = (_dbNet*) this;
  _dbBlock* block = (_dbBlock*) net->getOwner();

  if (net->_global_wire == 0)
    return NULL;

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

  if (net->_flags._set_io > 0)
    return net->_flags._io == 1;
  else
    return setIOflag();
}

void dbNet::setSizeOnly(bool v)
{
  _dbNet* net = (_dbNet*) this;
  net->_flags._size_only = v;
}

bool dbNet::isSizeOnly()
{
  _dbNet* net = (_dbNet*) this;
  return net->_flags._size_only == 1;
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
  // dimitri_fix : LOOK_AGAIN for FULL_ECO mode uint prev_flags =
  // flagsToUInt(net);

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
  // dimitri_fix : LOOK_AGAIN for FULL_ECO mode uint prev_flags =
  // flagsToUInt(net);

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

void dbNet::printRSeg(char* type)
{
  FILE* fp;
  char fn[40];
  if (type) {
    sprintf(fn, "%s%d", type, getId());
    fp = fopen(fn, "w");
  } else
    fp = stdout;
  fprintf(fp, "dbRSeg of Net %d %s :\n", getId(), getConstName());
  dbSet<dbRSeg> rSet = getRSegs();
  dbSet<dbRSeg>::iterator rc_itr;
  dbRSeg* rseg;
  int cnt = 0;
  int rx, ry;
  for (rc_itr = rSet.begin(); rc_itr != rSet.end(); ++rc_itr) {
    rseg = *rc_itr;
    rseg->getCoords(rx, ry);
    fprintf(fp,
            "  %d  id= %d, src= %d, tgt= %d, R= %g, C= %g\n",
            cnt,
            rseg->getId(),
            rseg->getSourceNode(),
            rseg->getTargetNode(),
            rseg->getResistance(),
            rseg->getCapacitance());  // zzzz corner ?
    fprintf(fp, "      x= %d, y=%d\n", rx, ry);
    cnt++;
  }
  if (fp != stdout)
    fclose(fp);
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
    if (rseg->getSourceNode() == srcn && rseg->getTargetNode() == tgtn)
      return rseg;
  }
  return NULL;
}

int ttttsv = 0;
void dbNet::createZeroRc(bool foreign)
{
  dbCapNode* cap1 = dbCapNode::create(this, 1, foreign);
  dbITerm* iterm = get1stITerm();
  cap1->setITermFlag();
  cap1->setNode(iterm->getId());
  dbCapNode* cap2 = dbCapNode::create(this, 2, foreign);
  cap2->setInternalFlag();
  if (ttttsv) {
    cap1->setCapacitance(0.0001, 0);
    cap2->setCapacitance(0.0001, 0);
  }
  dbRSeg* rseg1 = dbRSeg::create(
      this, 0 /*x*/, 0 /*y*/, 0 /*path_dir*/, !foreign /*allocate_cap*/);
  dbRSeg* rseg0 = dbRSeg::create(
      this, 0 /*x*/, 0 /*y*/, 0 /*path_dir*/, !foreign /*allocate_cap*/);
  rseg0->setSourceNode(0);
  rseg0->setTargetNode(cap1->getId());
  rseg1->setSourceNode(cap1->getId());
  rseg1->setTargetNode(cap2->getId());
  if (ttttsv)
    rseg1->setResistance(1.0, 0);
  // rseg1->setCapacitance(0.0001, 0);
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
  if (net->_r_segs == 0)
    return NULL;
  dbRSeg* zrc = dbRSeg::getRSeg((dbBlock*) net->getOwner(), net->_r_segs);
  return zrc;
}

dbCapNode* dbNet::findCapNode(uint nodeId)
{
  dbSet<dbCapNode> capNodes = getCapNodes();
  dbSet<dbCapNode>::iterator itr;

  for (itr = capNodes.begin(); itr != capNodes.end(); ++itr) {
    dbCapNode* n = *itr;

    if (n->getNode() == nodeId)
      return n;
  }

  return NULL;
}

void dbNet::printCapN(char* type)
{
  FILE* fp;
  char fn[40];
  if (type) {
    sprintf(fn, "%s%d", type, getId());
    fp = fopen(fn, "w");
  } else
    fp = stdout;
  fprintf(fp, "dbCapNode of Net %d %s :\n", getId(), getConstName());
  dbSet<dbCapNode> capNodes = getCapNodes();
  dbSet<dbCapNode>::iterator citr;

  dbCapNode* capn;
  int cnt = 0;
  for (citr = capNodes.begin(); citr != capNodes.end(); ++citr) {
    capn = *citr;
    uint itermf = capn->isITerm() ? 1 : 0;
    uint btermf = capn->isBTerm() ? 1 : 0;
    uint interf = capn->isInternal() ? 1 : 0;
    uint branch = capn->isBranch() ? 1 : 0;
    uint foreign = capn->isForeign() ? 1 : 0;
    uint treenode = capn->isTreeNode() ? 1 : 0;
    // uint srcbterm = capn->isSourceNodeBterm() ? 1 : 0;
    fprintf(fp,
            "  %d  id= %d, node= %d, childCnt= %d, iterm= %d, bterm= %d, "
            "internal= %d, branch= %d, foreign= %d, treenode= %d\n",
            cnt,
            capn->getId(),
            capn->getNode(),
            capn->getChildrenCnt(),
            itermf,
            btermf,
            interf,
            branch,
            foreign,
            treenode);
    cnt++;
  }
  if (fp != stdout)
    fclose(fp);
}

// void dbNet::donateCornerRC(dbBlock *pblock, dbITerm *donorterm, dbITerm
// *rcvterm, dbRSeg *& bridgeRseg, std::vector<dbCCSeg*> * gndcc, dbRSeg *&
// rtrseg, dbCapNode *& lastrcapnd, dbRSeg *& 1stdrseg, dbRSeg *& dtrseg,
// dbCapNode *& 1stdcapnd)

void dbNet::donateRC(dbITerm* donorterm,
                     dbITerm* rcvterm,
                     dbRSeg*& rtrseg,
                     dbRSeg*& lastrrseg,
                     dbCapNode*& lastrcapnd,
                     uint& ricapndMax,
                     dbRSeg*& fstdrseg,
                     dbRSeg*& dtrseg,
                     dbCapNode*& fstdcapnd,
                     std::vector<dbCCSeg*>* gndcc,
                     dbRSeg*& bridgeRseg)
{
  rtrseg = NULL;
  lastrcapnd = NULL;
  ricapndMax = 0;
  fstdrseg = NULL;
  dtrseg = NULL;
  fstdcapnd = NULL;
  bridgeRseg = NULL;

  dbBlock* block = (dbBlock*) getImpl()->getOwner();
  dbBlock* pblock = block;  // needed in case of independent spef corner
  dbNet* rcvnet = rcvterm->getNet();

  // donor rsegs
  dtrseg = NULL;
  dbRSeg* drseg = NULL;
  dbSet<dbRSeg> drSet = getRSegs();
  dbSet<dbRSeg>::iterator rc_itr;
  for (rc_itr = drSet.begin(); rc_itr != drSet.end(); ++rc_itr) {
    drseg = *rc_itr;
    if (!dtrseg
        && dbCapNode::getCapNode(block, drseg->getSourceNode())
                   ->getITerm(pblock)
               == donorterm) {
      dtrseg = drseg;
      break;
    }
  }
  if (!drseg)
    getImpl()->getLogger()->error(utl::ODB,
                                  49,
                                  "Donor net {} {} has no rc data",
                                  getId(),
                                  getConstName());
  if (!dtrseg)
    getImpl()->getLogger()->error(
        utl::ODB,
        50,
        "Donor net {} {} has no capnode attached to iterm {}/{}",
        getId(),
        getConstName(),
        donorterm->getInst()->getConstName(),
        donorterm->getMTerm()->getConstName());
  fstdrseg = getZeroRSeg();

  // receiver rsegs
  rtrseg = NULL;
  dbRSeg* rrseg = NULL;
  dbSet<dbRSeg> rSet = rcvnet->getRSegs();
  for (rc_itr = rSet.begin(); rc_itr != rSet.end(); ++rc_itr) {
    rrseg = *rc_itr;
    if (!rtrseg
        && dbCapNode::getCapNode(block, rrseg->getTargetNode())
                   ->getITerm(pblock)
               == rcvterm)
      rtrseg = rrseg;
    lastrrseg = rrseg;
  }
  if (!rtrseg)
    getImpl()->getLogger()->error(
        utl::ODB,
        51,
        "Receiver net {} {} has no capnode attached to iterm {}/{}",
        rcvnet->getId(),
        rcvnet->getConstName(),
        rcvterm->getInst()->getConstName(),
        rcvterm->getMTerm()->getConstName());

  // receiver capnodes
  dbCapNode* rcapnd = NULL;
  dbSet<dbCapNode> rnodeSet = rcvnet->getCapNodes();
  dbSet<dbCapNode>::iterator capn_itr;
  for (capn_itr = rnodeSet.begin(); capn_itr != rnodeSet.end(); ++capn_itr) {
    rcapnd = *capn_itr;
    if (rcapnd->isInternal()) {
      if (rcapnd->getNode() > ricapndMax)
        ricapndMax = rcapnd->getNode();
    }
  }
  ricapndMax += 3;
  lastrcapnd = rcapnd;

  uint rcvnid = rcvnet->getId();

  // donor capnodes
  dbCapNode* other = NULL;
  dbCapNode* capnd = NULL;
  uint cCnt = ((dbBlock*) getImpl()->getOwner())->getCornersPerBlock();
  dbSet<dbCapNode> nodeSet = getCapNodes();
  uint cid;
  for (capn_itr = nodeSet.begin(); capn_itr != nodeSet.end(); ++capn_itr) {
    capnd = *capn_itr;
    if (!fstdcapnd)
      fstdcapnd = capnd;
    dbSet<dbCCSeg> ccSegs = capnd->getCCSegs();
    dbCCSeg* ccSeg;
    dbSet<dbCCSeg>::iterator ccitr;
    for (ccitr = ccSegs.begin(); ccitr != ccSegs.end(); ccitr++) {
      ccSeg = *ccitr;
      other = ccSeg->getTheOtherCapn(capnd, cid);
      if (other->getNet() == rcvnet) {
        for (uint ii = 0; ii < cCnt; ii++) {
          capnd->addCapacitance(ccSeg->getCapacitance(ii), ii);
          other->addCapacitance(ccSeg->getCapacitance(ii), ii);
        }
        gndcc->push_back(ccSeg);
        dbCCSeg::disconnect(ccSeg);
      }
    }
    capnd->setNet(rcvnid);
    if (capnd->isInternal())
      capnd->setNode(capnd->getNode() + ricapndMax);
  }

  lastrcapnd->setNext(fstdcapnd->getId());

  dbCapNode* donorSrcCapNode
      = dbCapNode::getCapNode(block, dtrseg->getSourceNode());
  donorSrcCapNode->setInternalFlag();
  donorSrcCapNode->resetITermFlag();
  donorSrcCapNode->setNode(ricapndMax - 1);
  dbCapNode* rcvTgtCapNod
      = dbCapNode::getCapNode(block, rtrseg->getTargetNode());
  rcvTgtCapNod->setInternalFlag();
  rcvTgtCapNod->resetITermFlag();
  rcvTgtCapNod->setNode(ricapndMax - 2);
  bool foreign = block->getExtControl()->_foreign;
  dbRSeg* zrrseg = rcvnet->getZeroRSeg();
  bridgeRseg = dbRSeg::create(
      rcvnet, 0 /*x*/, 0 /*y*/, 0 /*pathDir*/, !foreign /*allocateCap*/);
  rcvnet->set1stRSegId(zrrseg->getId());
  bridgeRseg->setSourceNode(rcvTgtCapNod->getId());
  bridgeRseg->setTargetNode(donorSrcCapNode->getId());
  for (uint ii = 0; ii < cCnt; ii++) {
    bridgeRseg->setResistance(0, ii);
    if (!foreign)
      bridgeRseg->setCapacitance(0, ii);
  }
  lastrrseg->setNext(bridgeRseg->getId());
  bridgeRseg->setNext(dtrseg->getId());

  set1stRSegId(0);
  set1stCapNodeId(0);
}

// void dbNet::donateRC(dbITerm *rcvterm, dbNet *rcvnet)
//{
//    dbBlock *block = (dbBlock *)getOwner();
//    donateCornerRC(block, rcvterm, rcvnet);
//    if (!block->extCornersAreIndependent())
//        return;
//    dbBlock *extBlock;
//    dbNet *dNet;
//    dbNet *rNet;
//    int numcorners = block->getCornerCount();
//    for (int corner = 1; corner < numcorners; corner++) {
//        extBlock = block->findExtCornerBlock(corner);
//        dNet = dbNet::getNet(extBlock, getId());
//        rNet = dbNet::getNet(extBlock, rcvnet->getId());
//        dNet->donateCornerRC(block, rcvterm, rNet);
//    }
//}

void dbNet::unDonateRC(dbRSeg* rtrseg,
                       dbRSeg* lastrrseg,
                       dbITerm* it,
                       dbCapNode* lastrcapnd,
                       uint ricapndCnt,
                       dbRSeg* dtrseg,
                       dbRSeg* fstdrseg,
                       dbCapNode* fstdcapnd,
                       dbITerm* ot,
                       std::vector<dbCCSeg*>* gndcc)
{
  lastrrseg->setNext(0);

  rtrseg->getTargetCapNode()->resetInternalFlag();
  rtrseg->getTargetCapNode()->setITermFlag();
  rtrseg->getTargetCapNode()->setNode(it->getId());
  lastrcapnd->setNext(0);

  dtrseg->getSourceCapNode()->resetInternalFlag();
  dtrseg->getSourceCapNode()->setITermFlag();
  dtrseg->getSourceCapNode()->setNode(ot->getId());

  set1stRSegId(fstdrseg->getId());
  set1stCapNodeId(fstdcapnd->getId());
  uint donorNetId = getId();
  dbSet<dbCapNode> nodeSet = getCapNodes();
  dbSet<dbCapNode>::iterator capn_itr;
  dbCapNode* capnd;
  for (capn_itr = nodeSet.begin(); capn_itr != nodeSet.end(); ++capn_itr) {
    capnd = *capn_itr;
    capnd->setNet(donorNetId);
    if (capnd->isInternal())
      capnd->setNode(capnd->getNode() - ricapndCnt);
  }
  uint cCnt = ((dbBlock*) getImpl()->getOwner())->getCornersPerBlock();
  dbCCSeg* ccseg;
  dbCapNode* srcn;
  dbCapNode* tgtn;
  double cap;
  for (uint ii = 0; ii < gndcc->size(); ii++) {
    ccseg = (*gndcc)[ii];
    srcn = ccseg->getSourceCapNode();
    tgtn = ccseg->getTargetCapNode();
    for (uint jj = 0; jj < cCnt; jj++) {
      cap = ccseg->getCapacitance(jj);
      srcn->addCapacitance(-cap, jj);
      tgtn->addCapacitance(-cap, jj);
    }
    dbCCSeg::connect(ccseg);
  }
}

void dbNet::printWnP(char* type)
{
  char tag[20];
  sprintf(tag, "%s_w_", type);
  printWire(tag);
  sprintf(tag, "%s_r_", type);
  printRSeg(tag);
  sprintf(tag, "%s_c_", type);
  printCapN(tag);
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
      if (seg->_cap_node[0] == cap_id)
        S.push_back((dbCCSeg*) seg);
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
      if (seg->_cap_node[1] == cap_id)
        S.push_back((dbCCSeg*) seg);
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
      if (cn->isITerm())
        (dbITerm::getITerm(block, oid))->setExtId(0);

      else if (cn->isBTerm())
        (dbBTerm::getBTerm(block, oid))->setExtId(0);
    }

    itr = dbCapNode::destroy(itr);
  }
}

void dbNet::destroyRSegs()
{
  dbSet<dbRSeg> segs = getRSegs();
  dbSet<dbRSeg>::iterator sitr;

  for (sitr = segs.begin(); sitr != segs.end();)
    sitr = dbRSeg::destroy(sitr);

  dbRSeg* zrc = getZeroRSeg();
  if (zrc)
    dbRSeg::destroy(zrc);
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
      if (tnet == this)
        tnet = cc->getTargetCapNode()->getNet();
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
    if (netccap[ii] >= ccThreshold && cnets.find(inets[ii]) == cnets.end())
      cnets.insert(inets[ii]);
    inets[ii]->setMark(false);
  }
}

void dbNet::getGndTotalCap(double* gndcap, double* totalcap, double mcf)
{
  dbSigType type = getSigType();
  if ((type == dbSigType::POWER) || (type == dbSigType::GROUND))
    return;
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
      if (first)
        node->getGndTotalCap(gndcap, totalcap, mcf);
      else
        node->addGndTotalCap(gndcap, totalcap, mcf);
      first = false;
    }
  } else {
    dbRSeg* rc;
    dbSet<dbRSeg>::iterator rc_itr;
    for (rc_itr = rSet.begin(); rc_itr != rSet.end(); ++rc_itr) {
      rc = *rc_itr;
      if (first)
        rc->getGndTotalCap(gndcap, totalcap, mcf);
      else
        rc->addGndTotalCap(gndcap, totalcap, mcf);
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
  if ((type == dbSigType::POWER) || (type == dbSigType::GROUND))
    return;
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
  prc->getGndTotalCap(NULL, &totalcap[0], 1 /*mcf*/);
  dbSet<dbRSeg>::iterator rc_itr;
  for (rc_itr = rSet.begin(); rc_itr != rSet.end(); ++rc_itr) {
    rc = *rc_itr;
    mrsegs.push_back(rc);
    if (firstRC && cnt != 1)
      rc->getGndTotalCap(NULL, &totalcap[0], 1 /*mcf*/);
    else
      rc->addGndTotalCap(NULL, &totalcap[0], 1 /*mcf*/);
    cnt++;
    firstRC = false;
    tgtNode = dbCapNode::getCapNode(block, rc->getTargetNode());
    if (rc->getSourceNode() == rc->getTargetNode())
      continue;
    if (!tgtNode->isTreeNode() && totalcap[corner] <= max_cap
        && !tgtNode->isDangling())
      continue;
#ifdef EXT
    prc->mergeRCs(mrsegs);
#endif
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
  if (cc)
    cap += getTotalCouplingCap(corner);
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

  if (rule == NULL) {
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

  if (net->_non_default_rule == 0)
    return NULL;

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
  if (wire == NULL)
    return;
  dbWirePathItr pitr;
  for (pitr.begin(wire); pitr.getNextPath(path);) {
    while (pitr.getNextShape(pshape)) {
      if (pshape.shape.isVia())
        viaCnt++;
      else
        wireCnt++;
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
  if (wire == NULL)
    return;
  dbWirePathItr pitr;
  for (pitr.begin(wire); pitr.getNextPath(path);) {
    while (pitr.getNextShape(pshape)) {
      if (pshape.shape.isVia()) {
        viaCnt++;
        continue;
      }
      wireCnt++;

      uint level = pshape.shape.getTechLayer()->getRoutingLevel();
      if (levelTable)
        levelTable[level]++;
      len += MAX(pshape.shape.xMax() - pshape.shape.xMin(),
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
      if (s->isVia())
        viaCnt++;
      else
        wireCnt++;
    }
  }
}

void dbNet::getWireCount(uint& wireCnt, uint& viaCnt)
{
  if (getSigType() == dbSigType::POWER || getSigType() == dbSigType::GROUND)
    getPowerWireCount(wireCnt, viaCnt);
  else
    getSignalWireCount(wireCnt, viaCnt);
}

uint dbNet::getITermCount()
{
  uint itc = 0;
  dbSet<dbITerm> iterms = getITerms();
  dbSet<dbITerm>::iterator iterm_itr;
  for (iterm_itr = iterms.begin(); iterm_itr != iterms.end(); ++iterm_itr)
    itc++;
  return itc;
}

uint dbNet::getBTermCount()
{
  uint btc = 0;
  dbSet<dbBTerm> bterms = getBTerms();
  dbSet<dbBTerm>::iterator bterm_itr;
  for (bterm_itr = bterms.begin(); bterm_itr != bterms.end(); ++bterm_itr)
    btc++;
  return btc;
}

uint dbNet::getTermCount()
{
  uint itc = getITermCount();
  uint btc = getBTermCount();
  return itc + btc;
}

void dbNet::destroySWires()
{
  _dbNet* net = (_dbNet*) this;

  dbSet<dbSWire> swires = getSWires();
  ;
  dbSet<dbSWire>::iterator sitr;

  for (sitr = swires.begin(); sitr != swires.end();)
    sitr = dbSWire::destroy(sitr);

  net->_swires = 0;
}

dbNet* dbNet::create(dbBlock* block_, const char* name_, bool skipExistingCheck)
{
  _dbBlock* block = (_dbBlock*) block_;

  if (!skipExistingCheck && block->_net_hash.hasMember(name_))
    return NULL;

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
    block->_journal->endAction();
  }

  _dbNet* net = block->_net_tbl->create();
  net->_name = strdup(name_);
  ZALLOCATED(net->_name);
  block->_net_hash.insert(net);

  std::list<dbBlockCallBackObj*>::iterator cbitr;
  for (cbitr = block->_callbacks.begin(); cbitr != block->_callbacks.end();
       ++cbitr)
    (**cbitr)().inDbNetCreate((dbNet*) net);  // client ECO optimization - payam

  return (dbNet*) net;
}

void dbNet::destroy(dbNet* net_)
{
  _dbNet* net = (_dbNet*) net_;
  _dbBlock* block = (_dbBlock*) net->getOwner();

  dbSet<dbITerm> iterms = net_->getITerms();
  dbSet<dbITerm>::iterator iitr;

  for (iitr = iterms.begin(); iitr != iterms.end();)
    iitr = dbITerm::disconnect(iitr);

  dbSet<dbBTerm> bterms = net_->getBTerms();

  dbSet<dbBTerm>::iterator bitr;

  for (bitr = bterms.begin(); bitr != bterms.end();)
    bitr = dbBTerm::destroy(bitr);

  dbSet<dbSWire> swires = net_->getSWires();
  ;
  dbSet<dbSWire>::iterator sitr;

  for (sitr = swires.begin(); sitr != swires.end();)
    sitr = dbSWire::destroy(sitr);

  if (net->_wire != 0) {
    dbWire* wire = (dbWire*) block->_wire_tbl->getPtr(net->_wire);
    dbWire::destroy(wire);
  }

  for (dbId<_dbGroup> _group_id : net->_groups) {
    dbGroup* group = (dbGroup*) block->_group_tbl->getPtr(_group_id);
    group->removeNet(net_);
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
    block->_journal->pushParam(net->getId());
    block->_journal->endAction();
  }

  // Bugzilla #7: The notification of the the net destruction must
  // be done after pin manipulation is completed. The notification is
  // now after the pin disconnection - payam 01/10/2006
  std::list<dbBlockCallBackObj*>::iterator cbitr;
  for (cbitr = block->_callbacks.begin(); cbitr != block->_callbacks.end();
       ++cbitr)
    (**cbitr)().inDbNetDestroy(net_);  // client ECO optimization - payam

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
  if (!block->_net_tbl->validId(dbid_))
    return NULL;
  return (dbNet*) block->_net_tbl->getPtr(dbid_);
}

dbNet* dbNet::getValidNet(dbBlock* block_, const char * name){
  _dbBlock* block = (_dbBlock*) block_;
  if(!block->_net_hash.hasMember(name)) return NULL;
  return (dbNet*) block->_net_hash.find(name);
}

void dbNet::markNets(std::vector<dbNet*>& nets, dbBlock* block, bool mk)
{
  uint j;
  dbNet* net;
  if (nets.size() == 0) {
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
uint dbNet::setLevelAtFanout(uint level,
                             bool fromPI,
                             std::vector<dbInst*>& instVector)
{
  uint cnt = 0;
  dbSet<dbITerm> iterms = getITerms();
  dbSet<dbITerm>::iterator iitr;
  for (iitr = iterms.begin(); iitr != iterms.end(); ++iitr) {
    dbITerm* iterm = *iitr;
    if (!((iterm->getIoType() == dbIoType::INPUT)
          || (iterm->getIoType() == dbIoType::INOUT)))
      continue;

    dbInst* inst = iterm->getInst();
    // if (strcmp("INV_26", inst->getConstName())==0)
    //	notice(0, "inst %d %s\n", inst->getId(), inst->getConstName());

    if (inst->getMaster()->isSequential())
      continue;
    if (inst->getLevel() != 0)
      continue;
    if (inst->getMaster()->getType() != dbMasterType::CORE)
      continue;
    inst->setLevel(level, fromPI);
    instVector.push_back(inst);
    cnt++;
  }
  return cnt;
}

#if 0
//
// Helper fn: given a net create two terms at endpoints (x1,y1) and (x2,y2)
// Return true iff successful.
//
std::pair<dbBTerm *,dbBTerm *>
dbNet::createTerms4SingleNet(int x1, int y1, int x2, int y2, dbTechLayer *inly)
{
  int fwidth = MIN(x2 - x1, y2 - y1);
  uint hwidth = fwidth/2;

  std::pair<dbBTerm *,dbBTerm *> retpr;
  retpr.first  = NULL;
  retpr.second = NULL;

  std::string term_str(this->getName());
  term_str = term_str + "_BL";
  dbBTerm *blterm = dbBTerm::create(this, term_str.c_str());

  if (!blterm)
    return retpr;

  term_str = this->getName();
  term_str = term_str + "_BU";
  dbBTerm *buterm = dbBTerm::create(this, term_str.c_str());

  if (!buterm)
  {
      dbBTerm::destroy(blterm);
      return retpr;
  }

  // TWG: Added bpins
  dbBPin * blpin = dbBPin::create(blterm);
  dbBPin * bupin = dbBPin::create(buterm);

  if ((x2 - x1) == fwidth)
  {
      int x = x1+hwidth;
	  /*
	  if ((y2-y1)==fwidth) {
		  dbBox::create(blpin, inly, -hwidth+x, y1, hwidth+x, hwidth+y1);
		  dbBox::create(bupin, inly, -hwidth+x, y2, hwidth+x, hwidth+y2);
	  }
	  else {
		  dbBox::create(blpin, inly, -hwidth+x, -hwidth+y1, hwidth+x, hwidth+y1);
		  dbBox::create(bupin, inly, -hwidth+x, -hwidth+y2, hwidth+x, hwidth+y2);
	  }
	  */
      dbBox::create(blpin, inly, -hwidth+x, -hwidth+y1, hwidth+x, hwidth+y1);
      dbBox::create(bupin, inly, -hwidth+x, -hwidth+y2, hwidth+x, hwidth+y2);
  }
  else
  {
      int y = y1+hwidth;
      dbBox::create(blpin, inly, -hwidth+x1, -hwidth+y, hwidth+x1, hwidth+y);
      dbBox::create(bupin, inly, -hwidth+x2, -hwidth+y, hwidth+x2, hwidth+y);
  }

  blterm->setSigType(dbSigType::SIGNAL);
  buterm->setSigType(dbSigType::SIGNAL);
  blterm->setIoType(dbIoType::INPUT);
  buterm->setIoType(dbIoType::OUTPUT);
  blpin->setPlacementStatus(dbPlacementStatus::PLACED);
  bupin->setPlacementStatus(dbPlacementStatus::PLACED);

  retpr.first = blterm;
  retpr.second = buterm;
  return retpr;
}

#endif
}  // namespace odb
