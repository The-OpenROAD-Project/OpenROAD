///////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// Copyright (c) 2020, OpenRoad Project
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

// Generator Code Begin cpp
#include "dbGroup.h"

#include "db.h"
#include "dbBlock.h"
#include "dbBox.h"
#include "dbDatabase.h"
#include "dbDiff.hpp"
#include "dbGroupInstItr.h"
#include "dbGroupItr.h"
#include "dbGroupModInstItr.h"
#include "dbHashTable.hpp"
#include "dbInst.h"
#include "dbModInst.h"
#include "dbNet.h"
#include "dbTable.h"
#include "dbTable.hpp"
// User Code Begin includes
#include "dbGroupGroundNetItr.h"
#include "dbGroupPowerNetItr.h"
// User Code End includes
namespace odb {

// User Code Begin definitions
// User Code End definitions
template class dbTable<_dbGroup>;

bool _dbGroup::operator==(const _dbGroup& rhs) const
{
  if (flags_._type != rhs.flags_._type)
    return false;

  if (flags_._box != rhs.flags_._box)
    return false;

  if (_name != rhs._name)
    return false;

  if (_box != rhs._box)
    return false;

  if (_next_entry != rhs._next_entry)
    return false;

  if (_group_next != rhs._group_next)
    return false;

  if (_parent_group != rhs._parent_group)
    return false;

  if (_insts != rhs._insts)
    return false;

  if (_modinsts != rhs._modinsts)
    return false;

  if (_groups != rhs._groups)
    return false;

  // User Code Begin ==
  if (_power_nets != rhs._power_nets)
    return false;

  if (_ground_nets != rhs._ground_nets)
    return false;
  // User Code End ==
  return true;
}
bool _dbGroup::operator<(const _dbGroup& rhs) const
{
  // User Code Begin <
  if (strcmp(_name, rhs._name) >= 0)
    return false;
  if (flags_._type >= rhs.flags_._type)
    return false;
  if (_box >= rhs._box)
    return false;
  // User Code End <
  return true;
}
void _dbGroup::differences(dbDiff&         diff,
                           const char*     field,
                           const _dbGroup& rhs) const
{
  DIFF_BEGIN

  DIFF_FIELD(flags_._type);
  DIFF_FIELD(flags_._box);
  DIFF_FIELD(_name);
  DIFF_FIELD(_box);
  DIFF_FIELD(_next_entry);
  DIFF_FIELD(_group_next);
  DIFF_FIELD(_parent_group);
  DIFF_FIELD(_insts);
  DIFF_FIELD(_modinsts);
  DIFF_FIELD(_groups);
  // User Code Begin differences

  DIFF_VECTOR(_power_nets);

  DIFF_VECTOR(_ground_nets);

  // User Code End differences
  DIFF_END
}
void _dbGroup::out(dbDiff& diff, char side, const char* field) const
{
  DIFF_OUT_BEGIN
  DIFF_OUT_FIELD(flags_._type);
  DIFF_OUT_FIELD(flags_._box);
  DIFF_OUT_FIELD(_name);
  DIFF_OUT_FIELD(_box);
  DIFF_OUT_FIELD(_next_entry);
  DIFF_OUT_FIELD(_group_next);
  DIFF_OUT_FIELD(_parent_group);
  DIFF_OUT_FIELD(_insts);
  DIFF_OUT_FIELD(_modinsts);
  DIFF_OUT_FIELD(_groups);

  // User Code Begin out

  DIFF_OUT_VECTOR(_power_nets);

  DIFF_OUT_VECTOR(_ground_nets);

  // User Code End out
  DIFF_END
}
_dbGroup::_dbGroup(_dbDatabase* db)
{
  uint32_t* flags__bit_field = (uint32_t*) &flags_;
  *flags__bit_field          = 0;
  // User Code Begin constructor
  // User Code End constructor
}
_dbGroup::_dbGroup(_dbDatabase* db, const _dbGroup& r)
{
  flags_._type       = r.flags_._type;
  flags_._box        = r.flags_._box;
  flags_.spare_bits_ = r.flags_.spare_bits_;
  _name              = r._name;
  _box               = r._box;
  _next_entry        = r._next_entry;
  _group_next        = r._group_next;
  _parent_group      = r._parent_group;
  _insts             = r._insts;
  _modinsts          = r._modinsts;
  _groups            = r._groups;
  // User Code Begin CopyConstructor
  _power_nets  = r._power_nets;
  _ground_nets = r._ground_nets;
  // User Code End CopyConstructor
}

dbIStream& operator>>(dbIStream& stream, _dbGroup& obj)
{
  uint32_t* flags__bit_field = (uint32_t*) &obj.flags_;
  stream >> *flags__bit_field;
  stream >> obj._name;
  stream >> obj._box;
  stream >> obj._next_entry;
  stream >> obj._group_next;
  stream >> obj._parent_group;
  stream >> obj._insts;
  stream >> obj._modinsts;
  stream >> obj._groups;
  stream >> obj._power_nets;
  stream >> obj._ground_nets;
  // User Code Begin >>
  // User Code End >>
  return stream;
}
dbOStream& operator<<(dbOStream& stream, const _dbGroup& obj)
{
  uint32_t* flags__bit_field = (uint32_t*) &obj.flags_;
  stream << *flags__bit_field;
  stream << obj._name;
  stream << obj._box;
  stream << obj._next_entry;
  stream << obj._group_next;
  stream << obj._parent_group;
  stream << obj._insts;
  stream << obj._modinsts;
  stream << obj._groups;
  stream << obj._power_nets;
  stream << obj._ground_nets;
  // User Code Begin <<
  // User Code End <<
  return stream;
}

_dbGroup::~_dbGroup()
{
  if (_name)
    free((void*) _name);
  // User Code Begin Destructor
  // User Code End Destructor
}

// User Code Begin PrivateMethods
// User Code End PrivateMethods

////////////////////////////////////////////////////////////////////
//
// dbGroup - Methods
//
////////////////////////////////////////////////////////////////////

const char* dbGroup::getName() const
{
  _dbGroup* obj = (_dbGroup*) this;
  return obj->_name;
}

Rect dbGroup::getBox() const
{
  _dbGroup* obj = (_dbGroup*) this;
  return obj->_box;
}

void dbGroup::setParentGroup(dbGroup* _parent_group)
{
  _dbGroup* obj = (_dbGroup*) this;

  obj->_parent_group = _parent_group->getImpl()->getOID();
}

dbGroup* dbGroup::getParentGroup() const
{
  _dbGroup* obj = (_dbGroup*) this;
  if (obj->_parent_group == 0)
    return NULL;
  _dbBlock* par = (_dbBlock*) obj->getOwner();
  return (dbGroup*) par->_group_tbl->getPtr(obj->_parent_group);
}

// User Code Begin dbGroupPublicMethods
void dbGroup::setType(dbGroupType _type)
{
  _dbGroup* obj = (_dbGroup*) this;

  obj->flags_._type = (uint) _type;
}

dbGroup::dbGroupType dbGroup::getType() const
{
  _dbGroup* obj = (_dbGroup*) this;

  return (dbGroup::dbGroupType) obj->flags_._type;
}

void dbGroup::setBox(Rect _box)
{
  _dbGroup* obj    = (_dbGroup*) this;
  obj->flags_._box = 1;
  obj->_box        = _box;
}

bool dbGroup::hasBox()
{
  _dbGroup* obj = (_dbGroup*) this;
  return obj->flags_._box;
}

void dbGroup::addModInst(dbModInst* modinst)
{
  _dbGroup*   _group   = (_dbGroup*) this;
  _dbModInst* _modinst = (_dbModInst*) modinst;
  if (_modinst->_group != 0)
    modinst->getGroup()->removeModInst(modinst);
  _modinst->_group      = _group->getOID();
  _modinst->_group_next = _group->_modinsts;
  _group->_modinsts     = _modinst->getOID();
}

void dbGroup::removeModInst(dbModInst* modinst)
{
  _dbGroup*   _group   = (_dbGroup*) this;
  _dbModInst* _modinst = (_dbModInst*) modinst;
  if (_modinst->_group != _group->getOID())
    return;
  _dbBlock*   _block = (_dbBlock*) _group->getOwner();
  uint        id     = _modinst->getOID();
  _dbModInst* prev   = NULL;
  uint        cur    = _group->_modinsts;
  while (cur) {
    _dbModInst* c = _block->_modinst_tbl->getPtr(cur);
    if (cur == id) {
      if (prev == NULL)
        _group->_modinsts = _modinst->_group_next;
      else
        prev->_group_next = _modinst->_group_next;
      break;
    }
    prev = c;
    cur  = c->_group_next;
  }
  _modinst->_group      = 0;
  _modinst->_group_next = 0;
}

dbSet<dbModInst> dbGroup::getModInsts()
{
  _dbGroup* _group = (_dbGroup*) this;
  _dbBlock* block  = (_dbBlock*) _group->getOwner();
  return dbSet<dbModInst>(_group, block->_group_modinst_itr);
}

void dbGroup::addInst(dbInst* inst)
{
  _dbGroup* _group = (_dbGroup*) this;
  _dbInst*  _inst  = (_dbInst*) inst;
  if (_inst->_group != 0)
    inst->getGroup()->removeInst(inst);
  _inst->_group      = _group->getOID();
  _inst->_group_next = _group->_insts;
  _group->_insts     = _inst->getOID();
}

void dbGroup::removeInst(dbInst* inst)
{
  _dbGroup* _group = (_dbGroup*) this;
  _dbInst*  _inst  = (_dbInst*) inst;
  if (_inst->_group != _group->getOID())
    return;
  _dbBlock* _block = (_dbBlock*) _group->getOwner();
  uint      id     = _inst->getOID();
  _dbInst*  prev   = NULL;
  uint      cur    = _group->_insts;
  while (cur) {
    _dbInst* c = _block->_inst_tbl->getPtr(cur);
    if (cur == id) {
      if (prev == NULL)
        _group->_insts = _inst->_group_next;
      else
        prev->_group_next = _inst->_group_next;
      break;
    }
    prev = c;
    cur  = c->_group_next;
  }
  _inst->_group      = 0;
  _inst->_group_next = 0;
}

dbSet<dbInst> dbGroup::getInsts()
{
  _dbGroup* _group = (_dbGroup*) this;
  _dbBlock* block  = (_dbBlock*) _group->getOwner();
  return dbSet<dbInst>(_group, block->_group_inst_itr);
}

void dbGroup::addGroup(dbGroup* child)
{
  _dbGroup* _group = (_dbGroup*) this;
  _dbGroup* _child = (_dbGroup*) child;
  if (_child->_parent_group != 0)
    child->getParentGroup()->removeGroup(child);
  _child->_parent_group = _group->getOID();
  _child->_group_next   = _group->_groups;
  _group->_groups       = _child->getOID();
}

void dbGroup::removeGroup(dbGroup* child)
{
  _dbGroup* _group = (_dbGroup*) this;
  _dbGroup* _child = (_dbGroup*) child;
  if (_child->_parent_group != _group->getOID())
    return;
  _dbBlock* _block = (_dbBlock*) _group->getOwner();
  uint      id     = _child->getOID();
  _dbGroup* prev   = NULL;
  uint      cur    = _group->_groups;
  while (cur) {
    _dbGroup* c = _block->_group_tbl->getPtr(cur);
    if (cur == id) {
      if (prev == NULL)
        _group->_groups = _child->_group_next;
      else
        prev->_group_next = _child->_group_next;
      break;
    }
    prev = c;
    cur  = c->_group_next;
  }
  _child->_parent_group = 0;
  _child->_group_next   = 0;
}

dbSet<dbGroup> dbGroup::getGroups()
{
  _dbGroup* _group = (_dbGroup*) this;
  _dbBlock* block  = (_dbBlock*) _group->getOwner();
  return dbSet<dbGroup>(_group, block->_group_itr);
}

void dbGroup::addPowerNet(dbNet* net)
{
  _dbGroup* _group = (_dbGroup*) this;
  _dbNet*   _net   = (_dbNet*) net;
  for (dbId<_dbNet> _child : _group->_power_nets)
    if (_child == _net->getOID())
      return;
  bool                             found = false;
  dbVector<dbId<_dbNet>>::iterator it;
  for (it = _group->_ground_nets.begin();
       it != _group->_ground_nets.end() && !found;
       it++)
    if (*it == _net->getOID()) {
      _group->_ground_nets.erase(it--);
      found = true;
    }
  _group->_power_nets.push_back(_net->getOID());
  if (!found)
    _net->_groups.push_back(_group->getOID());
}

void dbGroup::addGroundNet(dbNet* net)
{
  _dbGroup* _group = (_dbGroup*) this;
  _dbNet*   _net   = (_dbNet*) net;
  for (dbId<_dbNet> _child : _group->_ground_nets)
    if (_child == _net->getOID())
      return;
  bool                             found = false;
  dbVector<dbId<_dbNet>>::iterator it;
  for (it = _group->_power_nets.begin();
       it != _group->_power_nets.end() && !found;
       it++)
    if (*it == _net->getOID()) {
      _group->_power_nets.erase(it--);
      found = true;
    }
  _group->_ground_nets.push_back(_net->getOID());
  if (!found)
    _net->_groups.push_back(_group->getOID());
}

void dbGroup::removeNet(dbNet* net)
{
  _dbGroup*                        _group = (_dbGroup*) this;
  _dbNet*                          _net   = (_dbNet*) net;
  bool                             found  = false;
  dbVector<dbId<_dbNet>>::iterator net_itr;
  for (net_itr = _group->_power_nets.begin();
       net_itr != _group->_power_nets.end() && !found;
       net_itr++)
    if (*net_itr == _net->getOID()) {
      _group->_power_nets.erase(net_itr--);
      found = true;
    }
  for (net_itr = _group->_ground_nets.begin();
       net_itr != _group->_ground_nets.end() && !found;
       net_itr++)
    if (*net_itr == _net->getOID()) {
      _group->_ground_nets.erase(net_itr--);
      found = true;
    }
  if (found) {
    dbVector<dbId<_dbGroup>>::iterator group_itr;
    for (group_itr = _net->_groups.begin(); group_itr != _net->_groups.end();
         group_itr++)
      if (*group_itr == _group->getOID()) {
        _net->_groups.erase(group_itr--);
        return;
      }
  }
}

dbSet<dbNet> dbGroup::getPowerNets()
{
  _dbGroup* _group = (_dbGroup*) this;
  _dbBlock* _block = (_dbBlock*) _group->getOwner();
  return dbSet<dbNet>(_group, _block->_group_power_net_itr);
}

dbSet<dbNet> dbGroup::getGroundNets()
{
  _dbGroup* _group = (_dbGroup*) this;
  _dbBlock* _block = (_dbBlock*) _group->getOwner();
  return dbSet<dbNet>(_group, _block->_group_ground_net_itr);
}

dbGroup* dbGroup::create(dbBlock* block, const char* name)
{
  _dbBlock* _block = (_dbBlock*) block;
  if (_block->_group_hash.hasMember(name))
    return nullptr;
  _dbGroup* _group = _block->_group_tbl->create();
  _group->_name    = strdup(name);
  ZALLOCATED(_group->_name);
  _group->flags_._type = PHYSICAL_CLUSTER;
  _block->_group_hash.insert(_group);
  return (dbGroup*) _group;
}

dbGroup* dbGroup::create(dbBlock*    block,
                         const char* name,
                         int         x1,
                         int         y1,
                         int         x2,
                         int         y2)
{
  _dbBlock* _block = (_dbBlock*) block;
  if (_block->_group_hash.hasMember(name))
    return nullptr;
  _dbGroup* _group = _block->_group_tbl->create();
  _group->_name    = strdup(name);
  ZALLOCATED(_group->_name);
  _group->flags_._type = VOLTAGE_DOMAIN;
  _group->flags_._box  = 1;
  _block->_group_hash.insert(_group);
  _group->_box.init(x1, y1, x2, y2);
  return (dbGroup*) _group;
}

dbGroup* dbGroup::create(dbGroup* parent, const char* name)
{
  _dbGroup* _parent = (_dbGroup*) parent;
  _dbBlock* _block  = (_dbBlock*) _parent->getOwner();
  if (_block->_group_hash.hasMember(name))
    return nullptr;
  _dbGroup* _group = _block->_group_tbl->create();
  _group->_name    = strdup(name);
  ZALLOCATED(_group->_name);
  _group->flags_._type = PHYSICAL_CLUSTER;
  _block->_group_hash.insert(_group);
  parent->addGroup((dbGroup*) _group);
  return (dbGroup*) _group;
}

void dbGroup::destroy(dbGroup* group)
{
  _dbGroup* _group = (_dbGroup*) group;
  _dbBlock* block  = (_dbBlock*) _group->getOwner();
  for (auto inst : group->getInsts()) {
    group->removeInst(inst);
  }
  for (auto modinst : group->getModInsts()) {
    group->removeModInst(modinst);
  }
  for (auto child : group->getGroups()) {
    group->removeGroup(child);
  }
  if (_group->_parent_group != 0)
    group->getParentGroup()->removeGroup(group);
  dbProperty::destroyProperties(_group);
  block->_group_hash.remove(_group);
  block->_group_tbl->destroy(_group);
}

dbGroup* dbGroup::getGroup(dbBlock* block_, uint dbid_)
{
  _dbBlock* block = (_dbBlock*) block_;
  return (dbGroup*) block->_group_tbl->getPtr(dbid_);
}

// User Code End dbGroupPublicMethods
}  // namespace odb
   // Generator Code End cpp