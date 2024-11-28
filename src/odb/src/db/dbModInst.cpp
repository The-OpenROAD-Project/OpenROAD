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
#include "dbModInst.h"

#include "dbBlock.h"
#include "dbDatabase.h"
#include "dbDiff.hpp"
#include "dbHashTable.hpp"
#include "dbModITerm.h"
#include "dbModule.h"
#include "dbTable.h"
#include "dbTable.hpp"
#include "odb/db.h"
// User Code Begin Includes
#include "dbGroup.h"
#include "dbModBTerm.h"
#include "dbModuleModInstItr.h"
#include "dbModuleModInstModITermItr.h"
// User Code End Includes
namespace odb {
template class dbTable<_dbModInst>;

bool _dbModInst::operator==(const _dbModInst& rhs) const
{
  if (_name != rhs._name) {
    return false;
  }
  if (_next_entry != rhs._next_entry) {
    return false;
  }
  if (_parent != rhs._parent) {
    return false;
  }
  if (_module_next != rhs._module_next) {
    return false;
  }
  if (_master != rhs._master) {
    return false;
  }
  if (_group_next != rhs._group_next) {
    return false;
  }
  if (_group != rhs._group) {
    return false;
  }
  if (_moditerms != rhs._moditerms) {
    return false;
  }

  return true;
}

bool _dbModInst::operator<(const _dbModInst& rhs) const
{
  // User Code Begin <
  if (strcmp(_name, rhs._name) >= 0) {
    return false;
  }
  // User Code End <
  return true;
}

void _dbModInst::differences(dbDiff& diff,
                             const char* field,
                             const _dbModInst& rhs) const
{
  DIFF_BEGIN
  DIFF_FIELD(_name);
  DIFF_FIELD(_next_entry);
  DIFF_FIELD(_parent);
  DIFF_FIELD(_module_next);
  DIFF_FIELD(_master);
  DIFF_FIELD(_group_next);
  DIFF_FIELD(_group);
  DIFF_FIELD(_moditerms);
  DIFF_END
}

void _dbModInst::out(dbDiff& diff, char side, const char* field) const
{
  DIFF_OUT_BEGIN
  DIFF_OUT_FIELD(_name);
  DIFF_OUT_FIELD(_next_entry);
  DIFF_OUT_FIELD(_parent);
  DIFF_OUT_FIELD(_module_next);
  DIFF_OUT_FIELD(_master);
  DIFF_OUT_FIELD(_group_next);
  DIFF_OUT_FIELD(_group);
  DIFF_OUT_FIELD(_moditerms);

  DIFF_END
}

_dbModInst::_dbModInst(_dbDatabase* db)
{
  // User Code Begin Constructor
  _name = nullptr;
  _parent = 0;
  _module_next = 0;
  _moditerms = 0;
  _master = 0;
  _group = 0;
  _group_next = 0;
  // User Code End Constructor
}

_dbModInst::_dbModInst(_dbDatabase* db, const _dbModInst& r)
{
  _name = r._name;
  _next_entry = r._next_entry;
  _parent = r._parent;
  _module_next = r._module_next;
  _master = r._master;
  _group_next = r._group_next;
  _group = r._group;
  _moditerms = r._moditerms;
}

dbIStream& operator>>(dbIStream& stream, _dbModInst& obj)
{
  stream >> obj._name;
  stream >> obj._next_entry;
  stream >> obj._parent;
  stream >> obj._module_next;
  stream >> obj._master;
  stream >> obj._group_next;
  stream >> obj._group;
  // User Code Begin >>
  dbBlock* block = (dbBlock*) (obj.getOwner());
  _dbDatabase* db_ = (_dbDatabase*) (block->getDataBase());
  if (db_->isSchema(db_schema_update_hierarchy)) {
    stream >> obj._moditerms;
  }
  if (db_->isSchema(db_schema_db_remove_hash)) {
    _dbBlock* block = (_dbBlock*) (((dbDatabase*) db_)->getChip()->getBlock());
    _dbModule* module = block->_module_tbl->getPtr(obj._parent);
    if (obj._name) {
      module->_modinst_hash[obj._name] = obj.getId();
    }
  }
  // User Code End >>
  return stream;
}

dbOStream& operator<<(dbOStream& stream, const _dbModInst& obj)
{
  stream << obj._name;
  stream << obj._next_entry;
  stream << obj._parent;
  stream << obj._module_next;
  stream << obj._master;
  stream << obj._group_next;
  stream << obj._group;
  // User Code Begin <<
  dbBlock* block = (dbBlock*) (obj.getOwner());
  auto db_ = (_dbDatabase*) (block->getDataBase());
  if (db_->isSchema(db_schema_update_hierarchy)) {
    stream << obj._moditerms;
  }
  // User Code End <<
  return stream;
}

_dbModInst::~_dbModInst()
{
  if (_name) {
    free((void*) _name);
  }
}

////////////////////////////////////////////////////////////////////
//
// dbModInst - Methods
//
////////////////////////////////////////////////////////////////////

const char* dbModInst::getName() const
{
  _dbModInst* obj = (_dbModInst*) this;
  return obj->_name;
}

dbModule* dbModInst::getParent() const
{
  _dbModInst* obj = (_dbModInst*) this;
  if (obj->_parent == 0) {
    return nullptr;
  }
  _dbBlock* par = (_dbBlock*) obj->getOwner();
  return (dbModule*) par->_module_tbl->getPtr(obj->_parent);
}

dbModule* dbModInst::getMaster() const
{
  _dbModInst* obj = (_dbModInst*) this;
  if (obj->_master == 0) {
    return nullptr;
  }
  _dbBlock* par = (_dbBlock*) obj->getOwner();
  return (dbModule*) par->_module_tbl->getPtr(obj->_master);
}

dbGroup* dbModInst::getGroup() const
{
  _dbModInst* obj = (_dbModInst*) this;
  if (obj->_group == 0) {
    return nullptr;
  }
  _dbBlock* par = (_dbBlock*) obj->getOwner();
  return (dbGroup*) par->_group_tbl->getPtr(obj->_group);
}

// User Code Begin dbModInstPublicMethods
dbModInst* dbModInst::create(dbModule* parentModule,
                             dbModule* masterModule,
                             const char* name)
{
  _dbModule* module = (_dbModule*) parentModule;
  _dbBlock* block = (_dbBlock*) module->getOwner();
  _dbModule* master = (_dbModule*) masterModule;

  if (master->_mod_inst != 0) {
    return nullptr;
  }

  dbModInst* ret = nullptr;
  ret = ((dbModule*) module)->findModInst(name);
  if (ret) {
    return nullptr;
  }

  _dbModInst* modinst = block->_modinst_tbl->create();
  modinst->_name = strdup(name);
  ZALLOCATED(modinst->_name);
  modinst->_master = master->getOID();
  modinst->_parent = module->getOID();
  // push to head of list in block
  modinst->_module_next = module->_modinsts;
  module->_modinsts = modinst->getOID();
  master->_mod_inst = modinst->getOID();
  module->_modinst_hash[modinst->_name] = modinst->getOID();
  return (dbModInst*) modinst;
}

void dbModInst::destroy(dbModInst* modinst)
{
  _dbModInst* _modinst = (_dbModInst*) modinst;
  _dbBlock* block = (_dbBlock*) _modinst->getOwner();
  _dbModule* module = (_dbModule*) modinst->getParent();

  _dbModule* master = (_dbModule*) modinst->getMaster();
  master->_mod_inst = dbId<_dbModInst>();  // clear
  dbModule::destroy((dbModule*) master);

  // remove the moditerm connections
  for (auto moditerm : modinst->getModITerms()) {
    moditerm->disconnect();
  }
  // remove the moditerms
  for (auto moditerm : modinst->getModITerms()) {
    block->_moditerm_tbl->destroy((_dbModITerm*) moditerm);
  }
  // unlink from parent start
  uint id = _modinst->getOID();
  _dbModInst* prev = nullptr;
  uint cur = module->_modinsts;
  while (cur) {
    _dbModInst* c = block->_modinst_tbl->getPtr(cur);
    if (cur == id) {
      if (prev == nullptr) {
        module->_modinsts = _modinst->_module_next;
      } else {
        prev->_module_next = _modinst->_module_next;
      }
      break;
    }
    prev = c;
    cur = c->_module_next;
  }
  // unlink from parent end
  if (_modinst->_group) {
    modinst->getGroup()->removeModInst(modinst);
  }
  dbProperty::destroyProperties(_modinst);
  _dbModule* parent = (_dbModule*) (modinst->getParent());
  parent->_modinst_hash.erase(modinst->getName());
  block->_modinst_tbl->destroy(_modinst);
}

dbSet<dbModInst>::iterator dbModInst::destroy(dbSet<dbModInst>::iterator& itr)
{
  dbModInst* modinst = *itr;
  dbSet<dbModInst>::iterator next = ++itr;
  destroy(modinst);
  return next;
}

dbSet<dbModITerm> dbModInst::getModITerms()
{
  _dbModInst* _mod_inst = (_dbModInst*) this;
  _dbBlock* _block = (_dbBlock*) _mod_inst->getOwner();
  return dbSet<dbModITerm>(_mod_inst, _block->_module_modinstmoditerm_itr);
}

dbModInst* dbModInst::getModInst(dbBlock* block_, uint dbid_)
{
  _dbBlock* block = (_dbBlock*) block_;
  return (dbModInst*) block->_modinst_tbl->getPtr(dbid_);
}

std::string dbModInst::getHierarchicalName() const
{
  _dbModInst* _obj = (_dbModInst*) this;
  dbBlock* block = (dbBlock*) _obj->getOwner();
  std::string inst_name = std::string(getName());
  dbModule* parent = getParent();
  if (parent == block->getTopModule()) {
    return inst_name;
  }
  return parent->getModInst()->getHierarchicalName() + "/" + inst_name;
}

dbModITerm* dbModInst::findModITerm(const char* name)
{
  _dbModInst* obj = (_dbModInst*) this;
  _dbBlock* par = (_dbBlock*) obj->getOwner();
  auto it = obj->_moditerm_hash.find(name);
  if (it != obj->_moditerm_hash.end()) {
    auto db_id = (*it).second;
    return (dbModITerm*) par->_moditerm_tbl->getPtr(db_id);
  }
  return nullptr;
}

void dbModInst::RemoveUnusedPortsAndPins()
{
  std::set<dbModITerm*> kill_set;
  dbModule* module = this->getMaster();
  dbSet<dbModITerm> moditerms = getModITerms();
  dbSet<dbModBTerm> modbterms = module->getModBTerms();

  for (dbModITerm* mod_iterm : moditerms) {
    dbModBTerm* mod_bterm = module->findModBTerm(mod_iterm->getName());
    dbModNet* mod_net = mod_bterm->getModNet();
    dbSet<dbModITerm> dest_mod_iterms = mod_net->getModITerms();
    dbSet<dbBTerm> dest_bterms = mod_net->getBTerms();
    dbSet<dbITerm> dest_iterms = mod_net->getITerms();
    if (dest_mod_iterms.size() == 0 && dest_bterms.size() == 0
        && dest_iterms.size() == 0) {
      kill_set.insert(mod_iterm);
    }
  }
  moditerms = getModITerms();
  modbterms = module->getModBTerms();
  for (auto mod_iterm : kill_set) {
    dbModBTerm* mod_bterm = module->findModBTerm(mod_iterm->getName());
    dbModNet* modbterm_m_net = mod_bterm->getModNet();
    mod_bterm->disconnect();
    dbModBTerm::destroy(mod_bterm);
    dbModNet* moditerm_m_net = mod_iterm->getModNet();
    mod_iterm->disconnect();
    dbModITerm::destroy(mod_iterm);
    if (modbterm_m_net && modbterm_m_net->getBTerms().size() == 0
        && modbterm_m_net->getITerms().size() == 0
        && modbterm_m_net->getModITerms().size() == 0
        && modbterm_m_net->getModBTerms().size() == 0) {
      dbModNet::destroy(modbterm_m_net);
    }
    if (moditerm_m_net && moditerm_m_net->getBTerms().size() == 0
        && moditerm_m_net->getITerms().size() == 0
        && moditerm_m_net->getModITerms().size() == 0
        && moditerm_m_net->getModBTerms().size() == 0) {
      dbModNet::destroy(moditerm_m_net);
    }
  }
}

// Swap one hierarchical module with another one.
// Two modules must have identical number of ports and port names need to match.
// Functional equivalence is not required.
// New module is not allowed to have multiple levels of hierarchy for now.
// Newly instantiated modules are uniquified and old module instances are
// deleted.
bool dbModInst::swapMaster(dbModule* new_module)
{
  _dbModInst* inst = (_dbModInst*) this;
  utl::Logger* logger = getImpl()->getLogger();

  dbModule* old_module = getMaster();
  const char* old_module_name = old_module->getName();
  const char* new_module_name = new_module->getName();

  // Check if module names differ
  if (strcmp(old_module_name, new_module_name) == 0) {
    logger->warn(utl::ODB,
                 470,
                 "The modules cannot be swapped because the new module {} is "
                 "identical to the existing module",
                 new_module_name);
    return false;
  }

  // Check if number of module ports match
  dbSet<dbModBTerm> old_bterms = old_module->getModBTerms();
  dbSet<dbModBTerm> new_bterms = new_module->getModBTerms();
  if (old_bterms.size() != new_bterms.size()) {
    logger->warn(utl::ODB,
                 453,
                 "The modules cannot be swapped because module {} "
                 "has {} ports but module {} has {} ports",
                 old_module_name,
                 old_bterms.size(),
                 new_module_name,
                 new_bterms.size());
    return false;
  }

  // Check if module port names match
  std::vector<_dbModBTerm*> new_ports;
  std::vector<_dbModBTerm*> old_ports;
  dbSet<dbModBTerm>::iterator iter;
  for (iter = old_bterms.begin(); iter != old_bterms.end(); ++iter) {
    old_ports.push_back((_dbModBTerm*) *iter);
  }
  for (iter = new_bterms.begin(); iter != new_bterms.end(); ++iter) {
    new_ports.push_back((_dbModBTerm*) *iter);
  }
  std::sort(new_ports.begin(),
            new_ports.end(),
            [](_dbModBTerm* port1, _dbModBTerm* port2) {
              return strcmp(port1->_name, port2->_name) < 0;
            });
  std::sort(old_ports.begin(),
            old_ports.end(),
            [](_dbModBTerm* port1, _dbModBTerm* port2) {
              return strcmp(port1->_name, port2->_name) < 0;
            });
  std::vector<_dbModBTerm*>::iterator i1 = new_ports.begin();
  std::vector<_dbModBTerm*>::iterator i2 = old_ports.begin();
  for (; i1 != new_ports.end() && i2 != old_ports.end(); ++i1, ++i2) {
    _dbModBTerm* t1 = *i1;
    _dbModBTerm* t2 = *i2;
    if (t1 == nullptr) {
      logger->error(
          utl::ODB, 464, "Module {} has a null port", new_module_name);
    }
    if (t2 == nullptr) {
      logger->error(
          utl::ODB, 465, "Module {} has a null port", old_module_name);
    }
    if (strcmp(t1->_name, t2->_name) != 0) {
      break;
    }
  }
  if (i1 != new_ports.end() || i2 != old_ports.end()) {
    const char* new_port_name
        = (i1 != new_ports.end() && *i1) ? (*i1)->_name : "N/A";
    const char* old_port_name
        = (i2 != old_ports.end() && *i2) ? (*i2)->_name : "N/A";
    logger->warn(utl::ODB,
                 454,
                 "The modules cannot be swapped because module {} "
                 "has port {} but module {} has port {}",
                 old_module_name,
                 old_port_name,
                 new_module_name,
                 new_port_name);
    return false;
  }

  dbModule* new_module_copy = dbModule::makeUniqueDbModule(
      new_module->getName(), this->getName(), getMaster()->getOwner());
  if (new_module_copy) {
    debugPrint(logger,
               utl::ODB,
               "replace_design",
               1,
               "Created uniquified module {}",
               new_module_copy->getName());
  } else {
    logger->error(utl::ODB,
                  455,
                  "Unique module {} cannot be created",
                  new_module->getName());
  }
  dbModule::copy(new_module, new_module_copy, this);
  _dbModule* new_master = (_dbModule*) new_module_copy;
  if (logger->debugCheck(utl::ODB, "replace_design", 2)) {
    dbSet<dbInst> insts = new_module_copy->getInsts();
    dbSet<dbInst>::iterator inst_iter;
    for (inst_iter = insts.begin(); inst_iter != insts.end(); ++inst_iter) {
      dbInst* inst = *inst_iter;
      logger->report("new_module_copy {} instance {} has the following iterms",
                     new_module_copy->getName(),
                     inst->getName());
      dbSet<dbITerm> iterms = inst->getITerms();
      dbSet<dbITerm>::iterator it_iter;
      for (it_iter = iterms.begin(); it_iter != iterms.end(); ++it_iter) {
        dbITerm* iterm = *it_iter;
        logger->report("  iterm {}", iterm->getName());
      }
    }
  }

  // Map old mod nets to new mod nets based on new_module_copy
  std::map<dbModNet*, dbModNet*> mod_map;  // old mod net -> new mod net
  for (iter = old_bterms.begin(); iter != old_bterms.end(); ++iter) {
    dbModBTerm* old_bterm = *iter;
    dbModBTerm* new_bterm = new_module_copy->findModBTerm(old_bterm->getName());
    if (new_bterm == nullptr) {
      logger->error(utl::ODB,
                    466,
                    "modBTerm for {} is not found in copied module {}",
                    old_bterm->getName(),
                    new_module_copy->getName());
    }
    dbModNet* old_mod_net = old_bterm->getModNet();
    dbModNet* new_mod_net = new_bterm->getModNet();
    if (new_mod_net && old_mod_net) {
      mod_map[old_mod_net] = new_mod_net;
      debugPrint(logger,
                 utl::ODB,
                 "replace_design",
                 1,
                 "old mod net {} maps to new mod net {}",
                 old_mod_net->getName(),
                 new_mod_net->getName());
    }
  }

  // Patch connections such that boundary nets connect to new module iterms
  // instead of old module iterms
  inst->_master = new_master->getOID();
  new_master->_mod_inst = inst->getOID();
  debugPrint(logger,
             utl::ODB,
             "replace_design",
             1,
             "Connecting nets that span module boundary");
  for (const auto& [old_mod_net, new_mod_net] : mod_map) {
    dbSet<dbITerm> old_iterms = old_mod_net->getITerms();
    dbSet<dbITerm> new_iterms = new_mod_net->getITerms();
    dbSet<dbITerm>::iterator it_iter;
    for (it_iter = old_iterms.begin(); it_iter != old_iterms.end(); ++it_iter) {
      dbITerm* old_iterm = *it_iter;
      dbNet* flat_net = old_iterm->getNet();
      // iterm may be connected to another hierarchical instance, so save it
      // before disconnecting
      dbModNet* other_mod_net = old_iterm->getModNet();
      if (other_mod_net == old_mod_net) {
        other_mod_net = nullptr;
      }
      old_iterm->disconnect();
      debugPrint(logger,
                 utl::ODB,
                 "replace_design",
                 1,
                 "  disconnected old iterm {} from flat net {} + other mod net "
                 "{} for mod net {}",
                 old_iterm->getName(),
                 (flat_net ? flat_net->getName() : "none"),
                 (other_mod_net ? other_mod_net->getName() : "none"),
                 old_mod_net->getName());
      dbSet<dbITerm>::iterator new_it_iter;
      for (new_it_iter = new_iterms.begin(); new_it_iter != new_iterms.end();
           ++new_it_iter) {
        dbITerm* new_iterm = *new_it_iter;
        debugPrint(logger,
                   utl::ODB,
                   "replace_design",
                   1,
                   "  connecting iterm {} of mod net {}",
                   new_iterm->getName(),
                   new_mod_net->getName());
        if (flat_net) {
          new_iterm->connect(flat_net);
        }
        if (other_mod_net) {
          new_iterm->connect(other_mod_net);
        }
        debugPrint(logger,
                   utl::ODB,
                   "replace_design",
                   1,
                   "  connected new iterm {} to flat net {} + other mod net {} "
                   "for mod net {}",
                   new_iterm->getName(),
                   (flat_net ? flat_net->getName() : "none"),
                   (other_mod_net ? other_mod_net->getName() : "none"),
                   old_mod_net->getName());
      }
    }
  }

  // TODO: remove old module insts without destroying old module itself
  // dbModule::destroy(old_module);

  return true;
}

// User Code End dbModInstPublicMethods
}  // namespace odb
// Generator Code End Cpp
