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

#include "dbBlock.h"
#include "dbDatabase.h"
#include "dbDiff.hpp"
#include "dbHashTable.hpp"
#include "dbInst.h"
#include "dbModBTerm.h"
#include "dbModInst.h"
#include "dbModulePortItr.h"
#include "dbTable.h"
#include "dbTable.hpp"
#include "odb/db.h"
// User Code Begin Includes
#include <string>

#include "dbModNet.h"
#include "dbModuleInstItr.h"
#include "dbModuleModBTermItr.h"
#include "dbModuleModInstItr.h"
#include "dbModuleModNetItr.h"
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
  if (_modbterms != rhs._modbterms) {
    return false;
  }

  return true;
}

bool _dbModule::operator<(const _dbModule& rhs) const
{
  // User Code Begin <
  if (strcmp(_name, rhs._name) >= 0) {
    return false;
  }
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
  DIFF_FIELD(_modbterms);
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
  DIFF_OUT_FIELD(_modbterms);

  DIFF_END
}

_dbModule::_dbModule(_dbDatabase* db)
{
  // User Code Begin Constructor
  _name = nullptr;
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
  _modbterms = r._modbterms;
}

dbIStream& operator>>(dbIStream& stream, _dbModule& obj)
{
  stream >> obj._name;
  stream >> obj._next_entry;
  stream >> obj._insts;
  stream >> obj._mod_inst;
  stream >> obj._modinsts;
  if (obj.getDatabase()->isSchema(db_schema_update_hierarchy)) {
    stream >> obj._modnets;
  }
  if (obj.getDatabase()->isSchema(db_schema_update_hierarchy)) {
    stream >> obj._modbterms;
  }
  return stream;
}

dbOStream& operator<<(dbOStream& stream, const _dbModule& obj)
{
  stream << obj._name;
  stream << obj._next_entry;
  stream << obj._insts;
  stream << obj._mod_inst;
  stream << obj._modinsts;
  if (obj.getDatabase()->isSchema(db_schema_update_hierarchy)) {
    stream << obj._modnets;
  }
  if (obj.getDatabase()->isSchema(db_schema_update_hierarchy)) {
    stream << obj._modbterms;
  }
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

void dbModule::setModInst(dbModInst* mod_inst)
{
  _dbModule* obj = (_dbModule*) this;

  obj->_mod_inst = mod_inst->getImpl()->getOID();
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

const dbModBTerm* dbModule::getHeadDbModBTerm() const
{
  _dbModule* obj = (_dbModule*) this;
  _dbBlock* block_ = (_dbBlock*) obj->getOwner();
  if (obj->_modbterms == 0) {
    return nullptr;
  }
  // note that the odb objects are "pre-pended"
  // so first object is at tail. This means we are returning
  // last object added. The application calling this routine
  // needs to be aware of this (and possibly skip to the end
  // of the list and then use prev to reconstruct creation order).
  else {
    return (dbModBTerm*) (block_->_modbterm_tbl->getPtr(obj->_modbterms));
  }
}

int dbModule::getModInstCount()
{
  _dbModule* module = (_dbModule*) this;
  _dbBlock* block = (_dbBlock*) module->getOwner();
  return (int) ((dbSet<dbModInst>(module, block->_module_modinst_itr)).size());
}

int dbModule::getDbInstCount()
{
  _dbModule* module = (_dbModule*) this;
  _dbBlock* block = (_dbBlock*) module->getOwner();
  return (int) (dbSet<dbInst>(module, block->_module_inst_itr).size());
}

void dbModule::addInst(dbInst* inst)
{
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
  module->_dbinst_hash[inst->getName()] = dbId<_dbInst>(_inst->getOID());

  if (module->_insts == 0) {
    _inst->_module_next = 0;
    _inst->_module_prev = 0;
    module->_insts = _inst->getOID();
  } else {
    _dbInst* cur_head = block->_inst_tbl->getPtr(module->_insts);
    _inst->_module_next = module->_insts;
    module->_insts = _inst->getOID();
    cur_head->_module_prev = _inst->getOID();
  }
}

void _dbModule::removeInst(dbInst* inst)
{
  _dbModule* module = (_dbModule*) this;
  _dbInst* _inst = (_dbInst*) inst;
  uint id = _inst->getOID();

  if (_inst->_module != getOID()) {
    return;
  }

  if (_inst->_flags._dont_touch) {
    _inst->getLogger()->error(
        utl::ODB,
        371,
        "Attempt to remove dont_touch instance {} from parent module",
        _inst->_name);
  }

  _dbBlock* block = (_dbBlock*) getOwner();

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

dbSet<dbModNet> dbModule::getModNets()
{
  _dbModule* module = (_dbModule*) this;
  _dbBlock* block = (_dbBlock*) module->getOwner();
  return dbSet<dbModNet>(module, block->_module_modnet_itr);
}

dbModNet* dbModule::getModNet(const char* net_name)
{
  _dbModule* module = (_dbModule*) this;
  _dbBlock* block = (_dbBlock*) module->getOwner();
  auto it = module->_modnet_hash.find(net_name);
  if (it != module->_modnet_hash.end()) {
    uint db_id = (*it).second;
    return (dbModNet*) block->_modnet_tbl->getPtr(db_id);
  }
  return nullptr;
}

dbSet<dbModInst> dbModule::getModInsts()
{
  _dbModule* module = (_dbModule*) this;
  _dbBlock* block = (_dbBlock*) module->getOwner();
  return dbSet<dbModInst>(module, block->_module_modinst_itr);
}

//
// The ports include higher level views. These have a special
// iterator which knows about how to skip the contents
// of the hierarchical objects (busports)
//

dbSet<dbModBTerm> dbModule::getPorts()
{
  _dbModule* obj = (_dbModule*) this;
  if (obj->_port_iter == nullptr) {
    _dbBlock* block = (_dbBlock*) obj->getOwner();
    obj->_port_iter = new dbModulePortItr(block->_modbterm_tbl);
  }
  return dbSet<dbModBTerm>(this, obj->_port_iter);
}

//
// The modbterms are the leaf level connections
//"flat view"
//
dbSet<dbModBTerm> dbModule::getModBTerms()
{
  _dbModule* module = (_dbModule*) this;
  _dbBlock* block = (_dbBlock*) module->getOwner();
  return dbSet<dbModBTerm>(module, block->_module_modbterm_itr);
}

dbModBTerm* dbModule::getModBTerm(uint id)
{
  _dbModule* module = (_dbModule*) this;
  _dbBlock* block = (_dbBlock*) module->getOwner();
  return (dbModBTerm*) (block->_modbterm_tbl->getObject(id));
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
  if (_block->_module_hash.hasMember(name)) {
    return nullptr;
  }
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

  for (auto modbterm : module->getModBTerms()) {
    block->_modbterm_tbl->destroy((_dbModBTerm*) modbterm);
  }

  for (auto modnet : module->getModNets()) {
    block->_modnet_tbl->destroy((_dbModNet*) modnet);
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

dbModInst* dbModule::findModInst(const char* name)
{
  _dbModule* obj = (_dbModule*) this;
  _dbBlock* par = (_dbBlock*) obj->getOwner();
  auto it = obj->_modinst_hash.find(name);
  if (it != obj->_modinst_hash.end()) {
    auto db_id = (*it).second;
    return (dbModInst*) par->_modinst_tbl->getPtr(db_id);
  }
  return nullptr;
}

dbInst* dbModule::findDbInst(const char* name)
{
  _dbModule* obj = (_dbModule*) this;
  _dbBlock* par = (_dbBlock*) obj->getOwner();
  auto it = obj->_dbinst_hash.find(name);
  if (it != obj->_dbinst_hash.end()) {
    auto db_id = (*it).second;
    return (dbInst*) par->_inst_tbl->getPtr(db_id);
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

dbModBTerm* dbModule::findModBTerm(const char* name)
{
  std::string modbterm_name(name);
  size_t last_idx = modbterm_name.find_last_of('/');
  if (last_idx != std::string::npos) {
    modbterm_name = modbterm_name.substr(last_idx + 1);
  }
  _dbModule* obj = (_dbModule*) this;
  _dbBlock* par = (_dbBlock*) obj->getOwner();
  auto it = obj->_modbterm_hash.find(name);
  if (it != obj->_modbterm_hash.end()) {
    auto db_id = (*it).second;
    return (dbModBTerm*) par->_modbterm_tbl->getPtr(db_id);
  }
  return nullptr;
}

std::string dbModule::getHierarchicalName() const
{
  dbModInst* inst = getModInst();
  if (inst) {
    return inst->getHierarchicalName();
  }
  return "<top>";
}

dbBlock* dbModule::getOwner()
{
  _dbModule* obj = (_dbModule*) this;
  return (dbBlock*) obj->getOwner();
}

// Make a unique copy of module based on cell_name and inst_name
dbModule* dbModule::makeUniqueDbModule(const char* cell_name,
                                       const char* inst_name,
                                       dbBlock* block)

{
  dbModule* module = dbModule::create(block, cell_name);
  if (module != nullptr) {
    return module;
  }

  std::map<std::string, int>& name_id_map
      = ((_dbBlock*) block)->_module_name_id_map;
  std::string orig_cell_name(cell_name);
  std::string module_name = orig_cell_name + '_' + std::string(inst_name);
  do {
    std::string full_name = module_name;
    int& id = name_id_map[module_name];
    if (id > 0) {
      full_name += "_" + std::to_string(id);
    }
    ++id;
    module = dbModule::create(block, full_name.c_str());
  } while (module == nullptr);
  return module;
}

// Do a "deep" copy of old_module based on its instance context into new_module.
// All ports, instances, mod nets and parent/child IO will be copied.
// Connections that span multiple modules needs to be done outside this API.
// new_mod_inst is needed to create module instances for instance name
// uniquification.
void dbModule::copy(dbModule* old_module,
                    dbModule* new_module,
                    dbModInst* new_mod_inst)
{
  // Copy module ports including bus members
  modBTMap mod_bt_map;  // map old mbterm to new mbterm
  copyModulePorts(old_module, new_module, mod_bt_map);

  // Copy module instances and create iterm map
  ITMap it_map;  // map old iterm to new iterm
  copyModuleInsts(old_module, new_module, new_mod_inst, it_map);

  // TODO: handle hierarchical child instances

  // Copy mod nets and connect ports and iterms
  copyModuleModNets(old_module, new_module, mod_bt_map, it_map);

  // Establish boundary IO between parent and child
  copyModuleBoundaryIO(old_module, new_module, new_mod_inst);
}

void dbModule::copyModulePorts(dbModule* old_module,
                               dbModule* new_module,
                               modBTMap& mod_bt_map)
{
  utl::Logger* logger = old_module->getImpl()->getLogger();
  dbSet<dbModBTerm> old_ports = old_module->getModBTerms();
  dbSet<dbModBTerm>::iterator port_iter;
  for (port_iter = old_ports.begin(); port_iter != old_ports.end();
       ++port_iter) {
    dbModBTerm* old_port = *port_iter;
    dbModBTerm* new_port = dbModBTerm::create(new_module, old_port->getName());
    if (new_port) {
      debugPrint(logger,
                 utl::ODB,
                 "replace_design",
                 1,
                 "Created module port {} for old port {}",
                 new_port->getName(),
                 old_port->getName());
      mod_bt_map[old_port] = new_port;
      new_port->setIoType(old_port->getIoType());
    } else {
      logger->error(utl::ODB,
                    456,
                    "Module port {} cannot be created",
                    old_port->getName());
    }

    if (old_port->isBusPort()) {
      dbBusPort* old_bus_port = old_port->getBusPort();
      dbBusPort* new_bus_port = dbBusPort::create(
          new_module, new_port, old_bus_port->getFrom(), old_bus_port->getTo());
      if (new_bus_port) {
        debugPrint(logger,
                   utl::ODB,
                   "replace_design",
                   1,
                   "Created module bus port {}",
                   new_port->getName());
      } else {
        logger->error(utl::ODB,
                      457,
                      "Module bus port {} cannot be created",
                      new_port->getName());
      }
      new_port->setBusPort(new_bus_port);

      // create bus members
      int from_index = old_bus_port->getFrom();
      int to_index = old_bus_port->getTo();
      bool updown = (from_index <= to_index) ? true : false;
      int size = updown ? to_index - from_index + 1 : from_index - to_index + 1;
      for (int i = 0; i < size; i++) {
        int ix = updown ? from_index + i : from_index - i;
        std::string bus_bit_name = std::string(old_port->getName())
                                   + std::string("[") + std::to_string(ix)
                                   + std::string("]");
        dbModBTerm* old_bus_bit = old_bus_port->getBusIndexedElement(i);
        dbModBTerm* new_bus_bit
            = dbModBTerm::create(new_module, bus_bit_name.c_str());
        mod_bt_map[old_bus_bit] = new_bus_bit;
        if (new_bus_bit) {
          debugPrint(logger,
                     utl::ODB,
                     "replace_design",
                     1,
                     "Created module bus bit {}",
                     bus_bit_name);
        } else {
          logger->error(utl::ODB,
                        458,
                        "Module bus bit {} cannot be created",
                        bus_bit_name);
        }
        if (i == 0) {
          new_bus_port->setMembers(new_bus_bit);
        }
        if (i == size - 1) {
          new_bus_port->setLast(new_bus_bit);
        }
        new_bus_bit->setIoType(old_port->getIoType());
      }
    }  // end of bus port handling
  }
  new_module->getModBTerms().reverse();

  debugPrint(logger,
             utl::ODB,
             "replace_design",
             1,
             "copyModulePorts: modBTMap has {} ports",
             mod_bt_map.size());
}

void dbModule::copyModuleInsts(dbModule* old_module,
                               dbModule* new_module,
                               dbModInst* new_mod_inst,
                               ITMap& it_map)
{
  utl::Logger* logger = old_module->getImpl()->getLogger();
  // Add insts to new module
  dbSet<dbInst> old_insts = old_module->getInsts();
  dbSet<dbInst>::iterator inst_iter;
  for (inst_iter = old_insts.begin(); inst_iter != old_insts.end();
       ++inst_iter) {
    dbInst* old_inst = *inst_iter;
    // Change unique instance name from old_inst/leaf to new_inst/leaf
    std::string old_inst_name = old_inst->getName();
    size_t first_idx = old_inst_name.find_first_of('/');
    assert(first_idx != std::string::npos);
    std::string old_leaf_name = old_inst_name.substr(first_idx);
    std::string new_inst_name = new_mod_inst->getName() + old_leaf_name;
    dbInst* new_inst = dbInst::create(old_module->getOwner(),
                                      old_inst->getMaster(),
                                      new_inst_name.c_str(),
                                      /* phyical only */ false,
                                      new_module);
    if (new_inst) {
      debugPrint(logger,
                 utl::ODB,
                 "replace_design",
                 1,
                 "Created module instance {}",
                 new_inst->getName());
    } else {
      logger->error(
          utl::ODB, 459, "Module instance {} cannot be created", new_inst_name);
    }

    // Map old iterms to new iterms and connect iterms that are local to this
    // module only.  Nets that connect to iterms of other modules will be
    // done outside this API in dbModInst::swapMaster().
    dbSet<dbITerm> old_iterms = old_inst->getITerms();
    dbSet<dbITerm> new_iterms = new_inst->getITerms();
    dbSet<dbITerm>::iterator iter1, iter2;
    iter1 = old_iterms.begin();
    iter2 = new_iterms.begin();
    for (; iter1 != old_iterms.end() && iter2 != new_iterms.end();
         ++iter1, ++iter2) {
      dbITerm* old_iterm = *iter1;
      dbITerm* new_iterm = *iter2;
      it_map[old_iterm] = new_iterm;
      debugPrint(logger,
                 utl::ODB,
                 "replace_design",
                 1,
                 "  old iterm {} maps to new iterm {}",
                 old_iterm->getName(),
                 new_iterm->getName());
      dbNet* old_net = old_iterm->getNet();
      if (old_net) {
        // Create a local net only if it connects to iterms inside this module
        std::string net_name = old_net->getName();
        size_t first_idx = net_name.find_first_of('/');
        if (first_idx != std::string::npos) {
          std::string new_net_name
              = new_mod_inst->getName() + net_name.substr(first_idx);
          dbNet* new_net
              = old_module->getOwner()->findNet(new_net_name.c_str());
          if (new_net) {
            new_iterm->connect(new_net);
            debugPrint(logger,
                       utl::ODB,
                       "replace_design",
                       1,
                       "  connected iterm {} to existing local net {}",
                       new_iterm->getName(),
                       new_net->getName());
          } else {
            new_net
                = dbNet::create(old_module->getOwner(), new_net_name.c_str());
            new_iterm->connect(new_net);
            debugPrint(logger,
                       utl::ODB,
                       "replace_design",
                       1,
                       "  Connected iterm {} to new local net {}",
                       new_iterm->getName(),
                       new_net->getName());
          }
        }
      }
    }
  }

  if (new_module->getInsts().reversible()
      && new_module->getInsts().orderReversed()) {
    new_module->getInsts().reverse();
  }
}

void dbModule::copyModuleModNets(dbModule* old_module,
                                 dbModule* new_module,
                                 modBTMap& mod_bt_map,
                                 ITMap& it_map)
{
  utl::Logger* logger = old_module->getImpl()->getLogger();
  debugPrint(logger,
             utl::ODB,
             "replace_design",
             1,
             "copyModuleModNets: modBT_map has {} ports, it_map has {} iterms",
             mod_bt_map.size(),
             it_map.size());
  // Make boundary port connections.
  dbSet<dbModNet> old_nets = old_module->getModNets();
  dbSet<dbModNet>::iterator net_iter;
  for (net_iter = old_nets.begin(); net_iter != old_nets.end(); ++net_iter) {
    dbModNet* old_net = *net_iter;
    dbModNet* new_net = dbModNet::create(new_module, old_net->getName());
    if (new_net) {
      debugPrint(logger,
                 utl::ODB,
                 "replace_design",
                 1,
                 "Created module mod net {}",
                 new_net->getName());
    } else {
      logger->error(utl::ODB,
                    460,
                    "Module mod net {} cannot be created",
                    old_net->getName());
    }

    // Connect dbModBTerms to new mod net
    dbSet<dbModBTerm> mbterms = old_net->getModBTerms();
    dbSet<dbModBTerm>::iterator mb_iter;
    for (mb_iter = mbterms.begin(); mb_iter != mbterms.end(); ++mb_iter) {
      dbModBTerm* old_mbterm = *mb_iter;
      dbModBTerm* new_mbterm = mod_bt_map[old_mbterm];
      if (new_mbterm) {
        new_mbterm->connect(new_net);
        debugPrint(logger,
                   utl::ODB,
                   "replace_design",
                   1,
                   "  connected port {} to mod net {}",
                   new_mbterm->getName(),
                   new_net->getName());
      } else {
        logger->error(utl::ODB,
                      461,
                      "Port {} cannot be connected to mod net {} because it "
                      "does not exist",
                      old_mbterm->getName(),
                      new_net->getName());
      }
    }

    // Connect iterms to new mod net
    dbSet<dbITerm> iterms = old_net->getITerms();
    dbSet<dbITerm>::iterator it_iter;
    for (it_iter = iterms.begin(); it_iter != iterms.end(); ++it_iter) {
      dbITerm* old_iterm = *it_iter;
      dbITerm* new_iterm = it_map[old_iterm];
      if (new_iterm) {
        new_iterm->connect(new_net);
        debugPrint(logger,
                   utl::ODB,
                   "replace_design",
                   1,
                   "  connected iterm {} to mod net {}",
                   new_iterm->getName(),
                   new_net->getName());
      } else {
        logger->error(utl::ODB,
                      462,
                      "Instance terminal {} cannot be connected to mod net {} "
                      "because it does not exist",
                      old_iterm->getName(),
                      new_net->getName());
      }
    }
  }
}

void dbModule::copyModuleBoundaryIO(dbModule* old_module,
                                    dbModule* new_module,
                                    dbModInst* new_mod_inst)
{
  utl::Logger* logger = old_module->getImpl()->getLogger();
  // Establish "parent/child" port connections
  // dbModBTerm is the port seen from inside the dbModule ("child")
  // dbModITerm is the port seen from outside from the dbModInst ("parent")
  dbSet<dbModITerm> mod_iterms = new_mod_inst->getModITerms();
  dbSet<dbModITerm>::iterator iterm_iter;
  for (iterm_iter = mod_iterms.begin(); iterm_iter != mod_iterms.end();
       ++iterm_iter) {
    dbModITerm* old_mod_iterm = *iterm_iter;
    // Connect outside dbModITerm to inside dbModBTerm
    dbModBTerm* new_mod_bterm
        = new_module->findModBTerm(old_mod_iterm->getName());
    if (new_mod_bterm) {
      old_mod_iterm->setChildModBTerm(new_mod_bterm);
      new_mod_bterm->setParentModITerm(old_mod_iterm);
      debugPrint(logger,
                 utl::ODB,
                 "replace_design",
                 1,
                 "Created parent/chlld port connection");
      debugPrint(logger,
                 utl::ODB,
                 "replace_design",
                 1,
                 "  parent mod iterm is {}, child mod bterm is {}",
                 old_mod_iterm->getName(),
                 new_mod_bterm->getName());
    } else {
      logger->error(utl::ODB,
                    463,
                    "Parent/child port connection cannot be created for parent "
                    "mod iterm {} because child mod bterm {} does not exist",
                    old_mod_iterm->getName(),
                    old_mod_iterm->getName());
    }
  }
}

// User Code End dbModulePublicMethods
}  // namespace odb
   // Generator Code End Cpp
