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

#include "dbBTerm.h"

#include "db.h"
#include "dbArrayTable.h"
#include "dbBPinItr.h"
#include "dbBlock.h"
#include "dbBox.h"
#include "dbBoxItr.h"
#include "dbChip.h"
#include "dbDatabase.h"
#include "dbDiff.h"
#include "dbDiff.hpp"
#include "dbITerm.h"
#include "dbNet.h"
#include "dbShape.h"
#include "dbTable.h"
#include "dbTable.hpp"
#include "dbTransform.h"
#include "dbBlockCallBackObj.h"

namespace odb {

template class dbTable<_dbBTerm>;

_dbBTerm::_dbBTerm(_dbDatabase*)
{
  _flags._io_type    = dbIoType::INPUT;
  _flags._sig_type   = dbSigType::SIGNAL;
  _flags._orient     = 0;
  _flags._status     = 0;
  _flags._spef       = 0;
  _flags._special    = 0;
  _flags._mark       = 0;
  _flags._spare_bits = 0;
  _ext_id            = 0;
  _name              = 0;
  _sta_vertex_id     = 0;
}

_dbBTerm::_dbBTerm(_dbDatabase*, const _dbBTerm& b)
    : _flags(b._flags),
      _ext_id(b._ext_id),
      _name(NULL),
      _next_entry(b._next_entry),
      _net(b._net),
      _next_bterm(b._next_bterm),
      _prev_bterm(b._prev_bterm),
      _parent_block(b._parent_block),
      _parent_iterm(b._parent_iterm),
      _bpins(b._bpins),
      _ground_pin(b._ground_pin),
      _supply_pin(b._supply_pin),
      _sta_vertex_id(0)
{
  if (b._name) {
    _name = strdup(b._name);
    ZALLOCATED(_name);
  }
}

_dbBTerm::~_dbBTerm()
{
  if (_name)
    free((void*) _name);
}

bool _dbBTerm::operator<(const _dbBTerm& rhs) const
{
  return strcmp(_name, rhs._name) < 0;
}

bool _dbBTerm::operator==(const _dbBTerm& rhs) const
{
  if (_flags._io_type != rhs._flags._io_type)
    return false;

  if (_flags._sig_type != rhs._flags._sig_type)
    return false;

  if (_flags._spef != rhs._flags._spef)
    return false;

  if (_flags._special != rhs._flags._special)
    return false;

  if (_ext_id != rhs._ext_id)
    return false;

  if (_name && rhs._name) {
    if (strcmp(_name, rhs._name) != 0)
      return false;
  } else if (_name || rhs._name)
    return false;

  if (_next_entry != rhs._next_entry)
    return false;

  if (_net != rhs._net)
    return false;

  if (_next_bterm != rhs._next_bterm)
    return false;

  if (_prev_bterm != rhs._prev_bterm)
    return false;

  if (_parent_block != rhs._parent_block)
    return false;

  if (_parent_iterm != rhs._parent_iterm)
    return false;

  if (_bpins != rhs._bpins)
    return false;

  if (_ground_pin != rhs._ground_pin)
    return false;

  if (_supply_pin != rhs._supply_pin)
    return false;

  return true;
}

void _dbBTerm::differences(dbDiff&         diff,
                           const char*     field,
                           const _dbBTerm& rhs) const
{
  _dbBlock* lhs_blk = (_dbBlock*) getOwner();
  _dbBlock* rhs_blk = (_dbBlock*) rhs.getOwner();

  DIFF_BEGIN
  DIFF_FIELD(_name);
  DIFF_FIELD(_flags._io_type);
  DIFF_FIELD(_flags._sig_type);
  DIFF_FIELD(_flags._spef);
  DIFF_FIELD(_flags._special);
  DIFF_FIELD(_ext_id);
  DIFF_FIELD_NO_DEEP(_next_entry);

  if (!diff.deepDiff()) {
    DIFF_FIELD(_net);
  } else {
    _dbNet* lhs_net = lhs_blk->_net_tbl->getPtr(_net);
    _dbNet* rhs_net = rhs_blk->_net_tbl->getPtr(rhs._net);

    if (strcmp(lhs_net->_name, rhs_net->_name) != 0) {
      diff.report("< _net %s\n", lhs_net->_name);
      diff.report("> _net %s\n", rhs_net->_name);
    }
  }

  DIFF_FIELD_NO_DEEP(_next_bterm);
  DIFF_FIELD_NO_DEEP(_prev_bterm);
  DIFF_FIELD_NO_DEEP(_parent_block);
  DIFF_FIELD_NO_DEEP(_parent_iterm);
  DIFF_FIELD_NO_DEEP(_bpins);
  DIFF_FIELD_NO_DEEP(_ground_pin);
  DIFF_FIELD_NO_DEEP(_supply_pin);
  DIFF_END
}

void _dbBTerm::out(dbDiff& diff, char side, const char* field) const
{
  _dbBlock* blk = (_dbBlock*) getOwner();

  DIFF_OUT_BEGIN
  DIFF_OUT_FIELD(_name);
  DIFF_OUT_FIELD(_flags._io_type);
  DIFF_OUT_FIELD(_flags._sig_type);
  DIFF_OUT_FIELD(_flags._spef);
  DIFF_OUT_FIELD(_flags._special);
  DIFF_OUT_FIELD(_ext_id);
  DIFF_OUT_FIELD_NO_DEEP(_next_entry);

  if (!diff.deepDiff()) {
    DIFF_OUT_FIELD(_net);
  } else {
    _dbNet* net = blk->_net_tbl->getPtr(_net);
    diff.report("%c _net %s\n", side, net->_name);
  }

  DIFF_OUT_FIELD_NO_DEEP(_next_bterm);
  DIFF_OUT_FIELD_NO_DEEP(_prev_bterm);
  DIFF_OUT_FIELD_NO_DEEP(_parent_block);
  DIFF_OUT_FIELD_NO_DEEP(_parent_iterm);
  DIFF_OUT_FIELD_NO_DEEP(_bpins);
  DIFF_OUT_FIELD_NO_DEEP(_ground_pin);
  DIFF_OUT_FIELD_NO_DEEP(_supply_pin);
  DIFF_END
}

dbOStream& operator<<(dbOStream& stream, const _dbBTerm& bterm)
{
  uint* bit_field = (uint*) &bterm._flags;
  stream << *bit_field;
  stream << bterm._ext_id;
  stream << bterm._name;
  stream << bterm._next_entry;
  stream << bterm._net;
  stream << bterm._next_bterm;
  stream << bterm._prev_bterm;
  stream << bterm._parent_block;
  stream << bterm._parent_iterm;
  stream << bterm._bpins;
  stream << bterm._ground_pin;
  stream << bterm._supply_pin;
  return stream;
}

dbIStream& operator>>(dbIStream& stream, _dbBTerm& bterm)
{
  uint* bit_field = (uint*) &bterm._flags;
  stream >> *bit_field;
  stream >> bterm._ext_id;
  stream >> bterm._name;
  stream >> bterm._next_entry;
  stream >> bterm._net;
  stream >> bterm._next_bterm;
  stream >> bterm._prev_bterm;
  stream >> bterm._parent_block;
  stream >> bterm._parent_iterm;
  stream >> bterm._bpins;
  stream >> bterm._ground_pin;
  stream >> bterm._supply_pin;

  return stream;
}

////////////////////////////////////////////////////////////////////
//
// dbBTerm - Methods
//
////////////////////////////////////////////////////////////////////

std::string dbBTerm::getName()
{
  _dbBTerm* bterm = (_dbBTerm*) this;
  return bterm->_name;
}

const char* dbBTerm::getConstName()
{
  _dbBTerm* bterm = (_dbBTerm*) this;
  return bterm->_name;
}

bool dbBTerm::rename(const char* name)
{
  _dbBTerm* bterm = (_dbBTerm*) this;
  _dbBlock* block = (_dbBlock*) getBlock();

  if (block->_bterm_hash.hasMember(name))
    return false;

  block->_bterm_hash.remove(bterm);
  free((void*) bterm->_name);
  bterm->_name = strdup(name);
  ZALLOCATED(bterm->_name);
  block->_bterm_hash.insert(bterm);

  return true;
}

void dbBTerm::setSigType(dbSigType type)
{
  _dbBTerm* bterm         = (_dbBTerm*) this;
  bterm->_flags._sig_type = type.getValue();
}

dbSigType dbBTerm::getSigType()
{
  _dbBTerm* bterm = (_dbBTerm*) this;
  return dbSigType(bterm->_flags._sig_type);
}

void dbBTerm::setIoType(dbIoType type)
{
  _dbBTerm* bterm        = (_dbBTerm*) this;
  bterm->_flags._io_type = type.getValue();
}

dbIoType dbBTerm::getIoType()
{
  _dbBTerm* bterm = (_dbBTerm*) this;
  return dbIoType(bterm->_flags._io_type);
}

void dbBTerm::setSpefMark(uint v)
{
  _dbBTerm* bterm     = (_dbBTerm*) this;
  bterm->_flags._spef = v;
}
bool dbBTerm::isSetSpefMark()
{
  _dbBTerm* bterm = (_dbBTerm*) this;
  return bterm->_flags._spef > 0 ? true : false;
}
bool dbBTerm::isSpecial() const
{
  _dbBTerm* bterm = (_dbBTerm*) this;
  return bterm->_flags._special > 0 ? true : false;
}
void dbBTerm::setSpecial()
{
  _dbBTerm* bterm        = (_dbBTerm*) this;
  bterm->_flags._special = 1;
}
void dbBTerm::setMark(uint v)
{
  _dbBTerm* bterm     = (_dbBTerm*) this;
  bterm->_flags._mark = v;
}
bool dbBTerm::isSetMark()
{
  _dbBTerm* bterm = (_dbBTerm*) this;
  return bterm->_flags._mark > 0 ? true : false;
}
void dbBTerm::setExtId(uint v)
{
  _dbBTerm* bterm = (_dbBTerm*) this;
  bterm->_ext_id  = v;
}
uint dbBTerm::getExtId()
{
  _dbBTerm* bterm = (_dbBTerm*) this;
  return bterm->_ext_id;
}

dbNet* dbBTerm::getNet()
{
  _dbBTerm* bterm = (_dbBTerm*) this;
  if (bterm->_net) {
    _dbBlock* block = (_dbBlock*) getBlock();
    _dbNet*   net   = block->_net_tbl->getPtr(bterm->_net);
    return (dbNet*) net;
  } else
    return nullptr;
}

void dbBTerm::connect(dbNet* net_)
{
  _dbBTerm* bterm = (_dbBTerm*) this;
  _dbNet*   net   = (_dbNet*) net_;
  _dbBlock* block = (_dbBlock*) net->getOwner();
  if (bterm->_net)
    bterm->disconnectNet(bterm, block);
  bterm->connectNet(net, block);
}

void dbBTerm::disconnect()
{
  _dbBTerm* bterm = (_dbBTerm*) this;
  if (bterm->_net) {
    _dbBlock* block = (_dbBlock*) bterm->getOwner();
    bterm->disconnectNet(bterm, block);
  }
}

dbSet<dbBPin> dbBTerm::getBPins()
{
  //_dbBTerm * bterm = (_dbBTerm *) this;
  _dbBlock*     block = (_dbBlock*) getBlock();
  dbSet<dbBPin> bpins(this, block->_bpin_itr);
  return bpins;
}

dbITerm* dbBTerm::getITerm()
{
  _dbBTerm* bterm = (_dbBTerm*) this;
  _dbBlock* block = (_dbBlock*) getBlock();

  if (bterm->_parent_block == 0)
    return NULL;

  _dbChip*  chip   = (_dbChip*) block->getOwner();
  _dbBlock* parent = chip->_block_tbl->getPtr(bterm->_parent_block);
  return (dbITerm*) parent->_iterm_tbl->getPtr(bterm->_parent_iterm);
}

dbBlock* dbBTerm::getBlock()
{
  return (dbBlock*) getImpl()->getOwner();
}

bool dbBTerm::getFirstPin(dbShape& shape)
{
  dbSet<dbBPin>           bpins = getBPins();
  dbSet<dbBPin>::iterator bpin_itr;
  for (bpin_itr = bpins.begin(); bpin_itr != bpins.end(); ++bpin_itr) {
    dbBPin* bpin = *bpin_itr;
    
    for(dbBox* box : bpin->getBoxes()){
      if (bpin->getPlacementStatus() == dbPlacementStatus::UNPLACED
          || bpin->getPlacementStatus() == dbPlacementStatus::NONE || box == NULL)
        continue;

      if (box->isVia())  // This is not possible...
        continue;

      Rect r;
      box->getBox(r);
      shape.setSegment(box->getTechLayer(), r);
      return true;
    }
  }

  return false;
}

dbPlacementStatus dbBTerm::getFirstPinPlacementStatus()
{
  dbSet<dbBPin>           bpins = getBPins();
  auto bpin_itr = bpins.begin();
  if (bpin_itr != bpins.end()) {
    dbBPin* bpin = *bpin_itr;
    return bpin->getPlacementStatus();
  }

  return dbPlacementStatus::NONE;
}

bool dbBTerm::getFirstPinLocation(int& x, int& y)
{
  dbSet<dbBPin>           bpins = getBPins();
  dbSet<dbBPin>::iterator bpin_itr;
  for (bpin_itr = bpins.begin(); bpin_itr != bpins.end(); ++bpin_itr) {
    dbBPin* bpin = *bpin_itr;

    dbSet<dbBox> boxes = bpin->getBoxes();
    dbSet<dbBox>::iterator boxItr;
  
    for( boxItr = boxes.begin(); boxItr != boxes.end(); ++boxItr )
    {
      dbBox* box = *boxItr;
      if (bpin->getPlacementStatus() == dbPlacementStatus::UNPLACED
          || bpin->getPlacementStatus() == dbPlacementStatus::NONE || box == NULL)
        continue;

      if (box->isVia())  // This is not possible...
        continue;

      Rect r;
      box->getBox(r);
      x = r.xMin() + (int) (r.dx() >> 1U);
      y = r.yMin() + (int) (r.dy() >> 1U);
      return true;
    }
  }

  x = 0;
  y = 0;
  return false;
}

dbBTerm* dbBTerm::getGroundPin()
{
  _dbBTerm* bterm = (_dbBTerm*) this;
  _dbBlock* block = (_dbBlock*) getBlock();

  if (bterm->_ground_pin == 0)
    return NULL;

  _dbBTerm* ground = block->_bterm_tbl->getPtr(bterm->_ground_pin);
  return (dbBTerm*) ground;
}

void dbBTerm::setGroundPin(dbBTerm* pin)
{
  _dbBTerm* bterm    = (_dbBTerm*) this;
  bterm->_ground_pin = pin->getImpl()->getOID();
}

dbBTerm* dbBTerm::getSupplyPin()
{
  _dbBTerm* bterm = (_dbBTerm*) this;
  _dbBlock* block = (_dbBlock*) getBlock();

  if (bterm->_supply_pin == 0)
    return NULL;

  _dbBTerm* supply = block->_bterm_tbl->getPtr(bterm->_supply_pin);
  return (dbBTerm*) supply;
}

void dbBTerm::setSupplyPin(dbBTerm* pin)
{
  _dbBTerm* bterm    = (_dbBTerm*) this;
  bterm->_supply_pin = pin->getImpl()->getOID();
}

dbBTerm* dbBTerm::create(dbNet* net_, const char* name)
{
  _dbNet*   net   = (_dbNet*) net_;
  _dbBlock* block = (_dbBlock*) net->getOwner();

  if (block->_bterm_hash.hasMember(name))
    return NULL;

  _dbBTerm* bterm = block->_bterm_tbl->create();
  bterm->_name    = strdup(name);
  ZALLOCATED(bterm->_name);
  block->_bterm_hash.insert(bterm);
  for(auto callback:block->_callbacks)
    callback->inDbBTermCreate((dbBTerm*)bterm); 
  bterm->connectNet(net, block);

  return (dbBTerm*) bterm;
}

void _dbBTerm::connectNet(_dbNet* net, _dbBlock* block)
{
  for(auto callback:block->_callbacks)
    callback->inDbBTermPreConnect((dbBTerm*)this,(dbNet*)net); 
  _net = net->getOID();
  if (net->_bterms != 0) {
    _dbBTerm* tail    = block->_bterm_tbl->getPtr(net->_bterms);
    _next_bterm       = net->_bterms;
    tail->_prev_bterm = getOID();
  }
  else
    _next_bterm = 0;
  _prev_bterm = 0;
  net->_bterms = getOID();
  for(auto callback:block->_callbacks)
    callback->inDbBTermPostConnect((dbBTerm*)this); 
}

void dbBTerm::destroy(dbBTerm* bterm_)
{
  _dbBTerm* bterm = (_dbBTerm*) bterm_;
  _dbBlock* block = (_dbBlock*) bterm->getOwner();
  
  // delete bpins
  dbSet<dbBPin>           bpins = bterm_->getBPins();
  dbSet<dbBPin>::iterator itr;

  for (itr = bpins.begin(); itr != bpins.end();) {
    itr = dbBPin::destroy(itr);
  }
  if(bterm->_net)
    bterm->disconnectNet(bterm, block);
  for(auto callback:block->_callbacks)
    callback->inDbBTermDestroy(bterm_); 
  // remove from hash-table
  
  block->_bterm_hash.remove(bterm);
  dbProperty::destroyProperties(bterm);
  block->_bterm_tbl->destroy(bterm);
}

void _dbBTerm::disconnectNet(_dbBTerm* bterm, _dbBlock* block)
{
  // unlink bterm from the net
  for(auto callback:block->_callbacks)
    callback->inDbBTermPreDisconnect((dbBTerm*)this); 
  _dbNet* net = block->_net_tbl->getPtr(bterm->_net);
  uint    id  = bterm->getOID();

  if (net->_bterms == id) {
    net->_bterms = bterm->_next_bterm;

    if (net->_bterms != 0) {
      _dbBTerm* t    = block->_bterm_tbl->getPtr(net->_bterms);
      t->_prev_bterm = 0;
    }
  } else {
    if (bterm->_next_bterm != 0) {
      _dbBTerm* next    = block->_bterm_tbl->getPtr(bterm->_next_bterm);
      next->_prev_bterm = bterm->_prev_bterm;
    }

    if (bterm->_prev_bterm != 0) {
      _dbBTerm* prev    = block->_bterm_tbl->getPtr(bterm->_prev_bterm);
      prev->_next_bterm = bterm->_next_bterm;
    }
  }
  _net = 0;
  for(auto callback:block->_callbacks)
    callback->inDbBTermPostDisConnect((dbBTerm*)this, (dbNet*)net); 
}

dbSet<dbBTerm>::iterator dbBTerm::destroy(dbSet<dbBTerm>::iterator& itr)
{
  dbBTerm*                 bt   = *itr;
  dbSet<dbBTerm>::iterator next = ++itr;
  destroy(bt);
  return next;
}

dbBTerm* dbBTerm::getBTerm(dbBlock* block_, uint dbid_)
{
  _dbBlock* block = (_dbBlock*) block_;
  return (dbBTerm*) block->_bterm_tbl->getPtr(dbid_);
}

uint32_t dbBTerm::staVertexId()
{
  _dbBTerm* iterm = (_dbBTerm*) this;
  return iterm->_sta_vertex_id;
}

void dbBTerm::staSetVertexId(uint32_t id)
{
  _dbBTerm* iterm       = (_dbBTerm*) this;
  iterm->_sta_vertex_id = id;
}

}  // namespace odb
