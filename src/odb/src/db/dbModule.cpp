///////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// Copyright (c) 2020, The Regents of the University of California
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

// Generator Code Begin Cpp
#include "dbModule.h"

#include "db.h"
#include "dbBlock.h"
#include "dbDatabase.h"
#include "dbDiff.hpp"
#include "dbHashTable.hpp"
#include "dbInst.h"
#include "dbModBTerm.h"
#include "dbModInst.h"
#include "dbTable.h"
#include "dbTable.hpp"
// User Code Begin Includes
#include <string>

#include "dbInst.h"
#include "dbModInst.h"
#include "dbModNet.h"
#include "dbModuleInstItr.h"
#include "dbModuleModInstItr.h"
#include "utl/Logger.h"
// User Code End Includes
namespace odb {
template class dbTable<_dbModule>;

bool _dbModule::operator==(const _dbModule& rhs) const
{
  if (_name != rhs._name) {
    return false;
  }
  if (_next_entry != rhs._next_entry) {
    return false;
  }
  if (_insts != rhs._insts) {
    return false;
  }
  if (_mod_inst != rhs._mod_inst) {
    return false;
  }
  if (_modinsts != rhs._modinsts) {
    return false;
  }
  if (_modnets != rhs._modnets) {
    return false;
  }

  return true;
}

bool _dbModule::operator<(const _dbModule& rhs) const
{
  // User Code Begin <
  if (strcmp(_name, rhs._name) >= 0)
    return false;
  // User Code End <
  return true;
}

void _dbModule::differences(dbDiff& diff,
                            const char* field,
                            const _dbModule& rhs) const
{
  DIFF_BEGIN
  DIFF_FIELD(_name);
  DIFF_FIELD(_next_entry);
  DIFF_FIELD(_insts);
  DIFF_FIELD(_mod_inst);
  DIFF_FIELD(_modinsts);
  DIFF_FIELD(_modnets);
  DIFF_END
}

void _dbModule::out(dbDiff& diff, char side, const char* field) const
{
  DIFF_OUT_BEGIN
  DIFF_OUT_FIELD(_name);
  DIFF_OUT_FIELD(_next_entry);
  DIFF_OUT_FIELD(_insts);
  DIFF_OUT_FIELD(_mod_inst);
  DIFF_OUT_FIELD(_modinsts);
  DIFF_OUT_FIELD(_modnets);

  DIFF_END
}

_dbModule::_dbModule(_dbDatabase* db)
{
  // User Code Begin Constructor
  _name = 0;
  _insts = 0;
  _modinsts = 0;
  _mod_inst = 0;
  // User Code End Constructor
}

_dbModule::_dbModule(_dbDatabase* db, const _dbModule& r)
{
  _name = r._name;
  _next_entry = r._next_entry;
  _insts = r._insts;
  _mod_inst = r._mod_inst;
  _modinsts = r._modinsts;
  _modnets = r._modnets;
}

dbIStream& operator>>(dbIStream& stream, _dbModule& obj)
{
  stream >> obj._name;
  stream >> obj._next_entry;
  stream >> obj._insts;
  stream >> obj._mod_inst;
  stream >> obj._modinsts;
  stream >> obj._modnets;
  stream >> obj._port_vec;
  stream >> obj._modinst_vec;
  stream >> obj._dbinst_vec;
  stream >> obj._port_map;
  stream >> obj._modnet_map;
  return stream;
}

dbOStream& operator<<(dbOStream& stream, const _dbModule& obj)
{
  stream << obj._name;
  stream << obj._next_entry;
  stream << obj._insts;
  stream << obj._mod_inst;
  stream << obj._modinsts;
  stream << obj._modnets;
  stream << obj._port_vec;
  stream << obj._modinst_vec;
  stream << obj._dbinst_vec;
  stream << obj._port_map;
  stream << obj._modnet_map;
  return stream;
}

_dbModule::~_dbModule()
{
  if (_name) {
    free((void*) _name);
  }
}

////////////////////////////////////////////////////////////////////
//
// dbModule - Methods
//
////////////////////////////////////////////////////////////////////

const char* dbModule::getName() const
{
  _dbModule* obj = (_dbModule*) this;
  return obj->_name;
}

dbModInst* dbModule::getModInst() const
{
  _dbModule* obj = (_dbModule*) this;
  if (obj->_mod_inst == 0) {
    return nullptr;
  }
  _dbBlock* par = (_dbBlock*) obj->getOwner();
  return (dbModInst*) par->_modinst_tbl->getPtr(obj->_mod_inst);
}

// User Code Begin dbModulePublicMethods

std::vector<dbId<dbModBTerm>>& dbModule::getPortVec() const
{
  _dbModule* obj = (_dbModule*) this;
  return obj->_port_vec;
}

std::vector<dbId<dbModInst>>& dbModule::getModInstVec() const
{
  _dbModule* obj = (_dbModule*) this;
  return obj->_modinst_vec;
}

std::string dbModule::getHierarchicalName(std::string& separator)
{
  std::string ret = hierarchicalNameR(separator);
  // strip out the top module name
  size_t first_ix = ret.find_first_of('/');
  if (first_ix != std::string::npos)
    ret = ret.substr(first_ix + 1);
  return ret;
}

std::string dbModule::hierarchicalNameR(std::string& separator)
{
  dbBlock* block = getOwner();
  if (this == block->getTopModule()) {
    return (std::string(getName()));
  }
  dbModInst* local_inst = getModInst();
  std::string local_name = local_inst->getName();
  dbModule* parent = local_inst->getParent();
  return parent->hierarchicalNameR(separator) + separator + local_name;
}

/*
Traverse up the hierarchy until a net which is driven by
either a root level port or an instance is found.
When reporting the connections of a net we traverse up through
the hierarchy through ports.

Example:
module root (a,b,c);
gate u1(a,i1); //a -> drives net i1.
one u2(a,b,c);
endmodule

module one (a,b,c)
two u3 (a,b,c);
endmodule

module two (a,b,c);
somegate u2(a,b,c); //driver of a is i1 in root.
endmodule

If we ask to see the connections on u2/u3/u2 (this is instance of somegate)
Then the pin a should be driven by i1 (ie the net output by u1 in root).
Whereas all the others are the b,c at the root.

 */
void dbModule::highestModWithNetNamed(const char* modbterm_name,
                                      dbModule*& highest_module,
                                      std::string& highest_net_name)
{
  unsigned port_ix = 0;
  if (findPortIx(highest_net_name.c_str(), port_ix)) {
    dbModInst* local_inst = getModInst();
    dbModITerm* mod_iterm = nullptr;
    //
    // convert a port name to the moditerm name
    // alternative do it by index using port_ix into
    // iterm vec.
    //
    std::string local_inst_name = std::string(local_inst->getName());
    size_t last_idx = local_inst_name.find_last_of('/');
    if (last_idx != std::string::npos) {
      local_inst_name = local_inst_name.substr(last_idx + 1);
    }

    std::string moditerm_name
        = local_inst_name + "/" + std::string(modbterm_name);
    if (local_inst->findModITerm(moditerm_name.c_str(), mod_iterm)) {
      assert(mod_iterm);
      highest_net_name = mod_iterm->getNet()->getName();
      // get the iterm on the port index
      // get the net on iterm and return its name
      highest_module = local_inst->getParent();
      // go up the hierarchy until the net and port don't have the same name,
      // or we reach the top.
      highest_module->highestModWithNetNamed(
          highest_net_name.c_str(), highest_module, highest_net_name);
    }
  }
}

std::vector<dbId<dbInst>>& dbModule::getDbInstVec() const
{
  _dbModule* obj = (_dbModule*) this;
  return obj->_dbinst_vec;
}

dbModInst* dbModule::getModInst(dbId<dbModInst> el)
{
  _dbModule* module = (_dbModule*) this;
  _dbBlock* block = (_dbBlock*) module->getOwner();
  dbId<_dbModInst> converted_el(el.id());
  return ((dbModInst*) (block->_modinst_tbl->getPtr(converted_el)));
}

dbInst* dbModule::getdbInst(dbId<dbInst> el)
{
  _dbModule* module = (_dbModule*) this;
  _dbBlock* block = (_dbBlock*) module->getOwner();
  dbId<_dbInst> converted_el(el.id());
  return ((dbInst*) (block->_inst_tbl->getPtr(converted_el)));
}

void dbModule::addInstInHierarchy(dbInst* inst)
{
  _dbInst* _inst = (_dbInst*) inst;
  _dbModule* _module = (_dbModule*) this;

  unsigned id = _inst->getOID();
  dbId<dbInst> db_id(id);
  _module->_dbinst_vec.push_back(db_id);
}

size_t dbModule::getModInstCount()
{
  _dbModule* module = (_dbModule*) this;
  return module->_modinst_vec.size();
}

size_t dbModule::getDbInstCount()
{
  _dbModule* module = (_dbModule*) this;
  return module->_dbinst_vec.size();
}

void dbModule::addInst(dbInst* inst)
{
  static int debug;
  debug++;
  _dbModule* module = (_dbModule*) this;
  _dbInst* _inst = (_dbInst*) inst;
  _dbBlock* block = (_dbBlock*) module->getOwner();

  if (_inst->_flags._physical_only) {
    _inst->getLogger()->error(
        utl::ODB,
        297,
        "Physical only instance {} can't be added to module {}",
        inst->getName(),
        getName());
  }

  if (_inst->_module == module->getOID()) {
    return;  // already in this module
  }

  if (_inst->_flags._dont_touch) {
    _inst->getLogger()->error(
        utl::ODB,
        367,
        "Attempt to change the module of dont_touch instance {}",
        _inst->_name);
  }

  if (_inst->_module != 0) {
    dbModule* mod = dbModule::getModule((dbBlock*) block, _inst->_module);
    ((_dbModule*) mod)->removeInst(inst);
  }

  _inst->_module = module->getOID();

  if (module->_insts != 0) {
    _dbInst* tail = block->_inst_tbl->getPtr(module->_insts);
    _inst->_module_next = module->_insts;
    _inst->_module_prev = 0;
    tail->_module_prev = _inst->getOID();
  } else {
    _inst->_module_next = 0;
    _inst->_module_prev = 0;
  }

  module->_insts = _inst->getOID();
}

void _dbModule::removeInst(dbInst* inst)
{
  _dbModule* module = (_dbModule*) this;
  _dbInst* _inst = (_dbInst*) inst;
  if (_inst->_module != getOID())
    return;

  if (_inst->_flags._dont_touch) {
    _inst->getLogger()->error(
        utl::ODB,
        371,
        "Attempt to remove dont_touch instance {} from parent module",
        _inst->_name);
  }

  _dbBlock* block = (_dbBlock*) getOwner();
  uint id = _inst->getOID();

  if (module->_insts == id) {
    module->_insts = _inst->_module_next;

    if (module->_insts != 0) {
      _dbInst* t = block->_inst_tbl->getPtr(module->_insts);
      t->_module_prev = 0;
    }
  } else {
    if (_inst->_module_next != 0) {
      _dbInst* next = block->_inst_tbl->getPtr(_inst->_module_next);
      next->_module_prev = _inst->_module_prev;
    }

    if (_inst->_module_prev != 0) {
      _dbInst* prev = block->_inst_tbl->getPtr(_inst->_module_prev);
      prev->_module_next = _inst->_module_next;
    }
  }

  _inst->_module = 0;
  _inst->_module_next = 0;
  _inst->_module_prev = 0;
}

dbSet<dbModInst> dbModule::getChildren()
{
  _dbModule* module = (_dbModule*) this;
  _dbBlock* block = (_dbBlock*) module->getOwner();
  return dbSet<dbModInst>(module, block->_module_modinst_itr);
}

dbSet<dbInst> dbModule::getInsts()
{
  _dbModule* module = (_dbModule*) this;
  _dbBlock* block = (_dbBlock*) module->getOwner();
  return dbSet<dbInst>(module, block->_module_inst_itr);
}

dbModule* dbModule::create(dbBlock* block, const char* name)
{
  _dbBlock* _block = (_dbBlock*) block;
  if (_block->_module_hash.hasMember(name))
    return nullptr;
  _dbModule* module = _block->_module_tbl->create();
  module->_name = strdup(name);
  ZALLOCATED(module->_name);
  _block->_module_hash.insert(module);
  return (dbModule*) module;
}

void dbModule::destroy(dbModule* module)
{
  _dbModule* _module = (_dbModule*) module;
  _dbBlock* block = (_dbBlock*) _module->getOwner();

  if (block->_top_module == module->getId()) {
    _module->getLogger()->error(
        utl::ODB, 298, "The top module can't be destroyed.");
  }

  if (_module->_mod_inst != 0) {
    // Destroying the modInst will destroy this module too.
    dbModInst::destroy(module->getModInst());
    return;
  }

  dbSet<dbModInst> modinsts = module->getChildren();
  dbSet<dbModInst>::iterator itr;
  for (itr = modinsts.begin(); itr != modinsts.end();) {
    itr = dbModInst::destroy(itr);
  }

  dbSet<dbInst> insts = module->getInsts();
  dbSet<dbInst>::iterator inst_itr;
  for (inst_itr = insts.begin(); inst_itr != insts.end();) {
    inst_itr = dbInst::destroy(inst_itr);
  }

  dbProperty::destroyProperties(_module);
  block->_module_hash.remove(_module);
  block->_module_tbl->destroy(_module);
}

dbModule* dbModule::getModule(dbBlock* block_, uint dbid_)
{
  _dbBlock* block = (_dbBlock*) block_;
  return (dbModule*) block->_module_tbl->getPtr(dbid_);
}

void dbModule::removeModInst(const char* name)
{
  _dbModule* cur_module = (_dbModule*) this;
  _dbBlock* block = (_dbBlock*) cur_module->getOwner();
  for (std::vector<odb::dbId<odb::dbModInst>>::iterator it
       = cur_module->_modinst_vec.begin();
       it != cur_module->_modinst_vec.end();
       it++) {
    unsigned id = (*it).id();
    dbId<_dbModInst> converted_el(id);
    dbModInst* mi = (dbModInst*) (block->_modinst_tbl->getPtr(converted_el));
    if (mi && !strcmp(mi->getName(), name)) {
      cur_module->_modinst_vec.erase(it);
      return;
    }
  }
}

dbModInst* dbModule::findModInst(const char* name)
{
  _dbModule* cur_module = (_dbModule*) this;
  _dbBlock* block = (_dbBlock*) cur_module->getOwner();
  for (auto m : cur_module->_modinst_vec) {
    unsigned id = m.id();
    dbId<_dbModInst> converted_el(id);
    dbModInst* mi = (dbModInst*) (block->_modinst_tbl->getPtr(converted_el));
    if (mi && !strcmp(mi->getName(), name))
      return (dbModInst*) mi;
  }
  return nullptr;
}

dbInst* dbModule::findDbInst(const char* name)
{
  _dbModule* cur_module = (_dbModule*) this;
  _dbBlock* block = (_dbBlock*) cur_module->getOwner();
  for (auto m : cur_module->_dbinst_vec) {
    unsigned id = m.id();
    dbId<_dbInst> converted_el(id);
    dbInst* mi = (dbInst*) (block->_inst_tbl->getPtr(converted_el));
    if (mi && !strcmp(mi->getName().c_str(), name))
      return (dbInst*) mi;
  }
  return nullptr;
}

static void getLeafInsts(dbModule* module, std::vector<dbInst*>& insts)
{
  for (dbInst* inst : module->getInsts()) {
    insts.push_back(inst);
  }

  for (dbModInst* inst : module->getChildren()) {
    getLeafInsts(inst->getMaster(), insts);
  }
}

std::vector<dbInst*> dbModule::getLeafInsts()
{
  std::vector<dbInst*> insts;
  odb::getLeafInsts(this, insts);
  return insts;
}

bool dbModule::findPortIx(const char* port_name, unsigned& ix) const
{
  _dbModule* obj = (_dbModule*) this;
  std::string port_name_str(port_name);
  if (obj->_port_map.find(port_name_str) != obj->_port_map.end()) {
    ix = obj->_port_map[port_name_str];
    return true;
  }
  return false;
}

std::string dbModule::getHierarchicalName() const
{
  dbModInst* inst = getModInst();
  if (inst) {
    return inst->getHierarchicalName();
  } else {
    return "<top>";
  }
}

void* dbModule::getStaCell()
{
  _dbModule* module = (_dbModule*) this;
  return module->_sta_cell;
}

void dbModule::staSetCell(void* cell)
{
  _dbModule* module = (_dbModule*) this;
  module->_sta_cell = cell;
}

bool dbModule::findModBTerm(const char* port_name, dbModBTerm*& ret)
{
  ret = nullptr;
  _dbModule* _obj = (_dbModule*) this;
  _dbBlock* block = (_dbBlock*) _obj->getOwner();
  for (auto el : _obj->_port_vec) {
    // type conversion
    unsigned id = el.id();
    dbId<_dbModBTerm> converted_el(id);
    dbModBTerm* candidate
        = (dbModBTerm*) (block->_modbterm_tbl->getPtr(converted_el));
    if (!strcmp(candidate->getName(), port_name)) {
      ret = candidate;
      return true;
    }
  }
  return false;
}

dbModNet* dbModule::getModNet(const char* name)
{
  _dbModule* obj = (_dbModule*) this;
  _dbBlock* _block = (_dbBlock*) obj->getOwner();
  std::string name_str(name);
  if (obj->_modnet_map.find(name_str) != obj->_modnet_map.end()) {
    dbId<dbModNet> mnet_id = obj->_modnet_map[name_str];
    dbId<_dbModNet> _mnet_id(mnet_id.id());
    return (dbModNet*) (_block->_modnet_tbl->getPtr(_mnet_id));
  }
  return nullptr;
}

dbBlock* dbModule::getOwner()
{
  _dbModule* obj = (_dbModule*) this;
  return (dbBlock*) obj->getOwner();
}

dbModBTerm* dbModule::getdbModBTerm(dbBlock* block,
                                    dbId<dbModBTerm> modbterm_id)
{
  _dbBlock* _block = (_dbBlock*) block;
  // do a weird type conversion
  unsigned id = modbterm_id.id();
  dbId<_dbModBTerm> val(id);
  return (dbModBTerm*) (_block->_modbterm_tbl->getPtr(val));
}

dbModBTerm* dbModule::getdbModBTerm(dbId<dbModBTerm> modbterm_id)
{
  _dbModule* obj = (_dbModule*) this;
  _dbBlock* _block = (_dbBlock*) obj->getOwner();
  // do a weird type conversion
  unsigned id = modbterm_id.id();
  dbId<_dbModBTerm> val(id);
  return (dbModBTerm*) (_block->_modbterm_tbl->getPtr(val));
}

// User Code End dbModulePublicMethods
}  // namespace odb
   // Generator Code End Cpp
