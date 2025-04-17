// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2020-2025, The OpenROAD Authors

#include <algorithm>
#include <fstream>
#include <map>
#include <set>
#include <string>
#include <vector>

// Generator Code Begin Cpp
#include "dbBlock.h"
#include "dbDatabase.h"
#include "dbHashTable.hpp"
#include "dbJournal.h"
#include "dbModITerm.h"
#include "dbModInst.h"
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

void _dbModInst::collectMemInfo(MemInfo& info)
{
  info.cnt++;
  info.size += sizeof(*this);

  // User Code Begin collectMemInfo
  info.children_["name"].add(_name);
  info.children_["moditerm_hash"].add(_moditerm_hash);
  // User Code End collectMemInfo
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

  if (block->_journal) {
    block->_journal->beginAction(dbJournal::CREATE_OBJECT);
    block->_journal->pushParam(dbModInstObj);
    block->_journal->pushParam(name);
    block->_journal->pushParam(modinst->getId());
    block->_journal->pushParam(module->getId());
    block->_journal->pushParam(master->getId());
    block->_journal->endAction();
  }

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
  _dbBlock* _block = (_dbBlock*) _modinst->getOwner();
  _dbModule* _module = (_dbModule*) modinst->getParent();

  _dbModule* _master = (_dbModule*) modinst->getMaster();

  if (_block->_journal) {
    _block->_journal->beginAction(dbJournal::DELETE_OBJECT);
    _block->_journal->pushParam(dbModInstObj);
    _block->_journal->pushParam(modinst->getName());
    _block->_journal->pushParam(modinst->getId());
    _block->_journal->pushParam(_module->getId());
    _block->_journal->pushParam(_master->getId());
    _block->_journal->endAction();
  }

  _master->_mod_inst = dbId<_dbModInst>();  // clear

  // Note that we only destroy the module instance, not the module
  // itself

  dbSet<dbModITerm> moditerms = modinst->getModITerms();
  dbSet<dbModITerm>::iterator moditerm_itr;
  for (moditerm_itr = moditerms.begin(); moditerm_itr != moditerms.end();) {
    dbModITerm* moditerm = *moditerm_itr;
    moditerm->disconnect();
    moditerm_itr = dbModITerm::destroy(moditerm_itr);
  }

  // unlink from parent start
  uint id = _modinst->getOID();
  _dbModInst* prev = nullptr;
  uint cur = _module->_modinsts;
  while (cur) {
    _dbModInst* c = _block->_modinst_tbl->getPtr(cur);
    if (cur == id) {
      if (prev == nullptr) {
        _module->_modinsts = _modinst->_module_next;
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
  _dbModule* _parent = (_dbModule*) (modinst->getParent());
  _parent->_modinst_hash.erase(modinst->getName());
  _block->_modinst_tbl->destroy(_modinst);
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
  int bus_ix = 0;

  // traverse in modbterm order so we can skip over
  // any unused pins in a bus.

  for (dbModBTerm* mod_bterm : modbterms) {
    dbModNet* mod_net = nullptr;

    // Avoid removing unused ports from a bus
    // when we hit a bus port, we count down from the size
    // skipping the bus elements
    // Layout:
    // mod_bterm (head_element describing size)
    // mod_bterm[size-1],...mod_bterm[0] -- bus elements
    //
    if (mod_bterm->isBusPort()) {
      dbBusPort* bus_port = mod_bterm->getBusPort();
      bus_ix = bus_port->getSize();  // count down
      continue;
    }
    if (bus_ix != 0) {
      bus_ix = bus_ix - 1;
      continue;
    }

    dbModITerm* mod_iterm = mod_bterm->getParentModITerm();
    mod_net = mod_bterm->getModNet();
    if (mod_net) {
      dbSet<dbModITerm> dest_mod_iterms = mod_net->getModITerms();
      dbSet<dbBTerm> dest_bterms = mod_net->getBTerms();
      dbSet<dbITerm> dest_iterms = mod_net->getITerms();
      if (dest_mod_iterms.size() == 0 && dest_bterms.size() == 0
          && dest_iterms.size() == 0) {
        kill_set.insert(mod_iterm);
      }
    }
  }

  for (dbModITerm* mod_iterm : moditerms) {
    dbModBTerm* mod_bterm = module->findModBTerm(mod_iterm->getName());
    dbModNet* mod_net = mod_bterm->getModNet();
    if (mod_net) {
      dbSet<dbModITerm> dest_mod_iterms = mod_net->getModITerms();
      dbSet<dbBTerm> dest_bterms = mod_net->getBTerms();
      dbSet<dbITerm> dest_iterms = mod_net->getITerms();
      if (dest_mod_iterms.size() == 0 && dest_bterms.size() == 0
          && dest_iterms.size() == 0) {
        kill_set.insert(mod_iterm);
      }
    }
  }

  moditerms = getModITerms();
  modbterms = module->getModBTerms();
  for (auto mod_iterm : kill_set) {
    dbModNet* moditerm_m_net = mod_iterm->getModNet();
    dbModBTerm* mod_bterm = module->findModBTerm(mod_iterm->getName());
    dbModNet* modbterm_m_net = mod_bterm->getModNet();

    // Do the destruction in order for benefit of journaller
    // so we always have a dbModBTerm..
    // first destroy net, then dbModIterm, then dbModbterm.
    mod_iterm->disconnect();
    mod_bterm->disconnect();

    // First destroy the net
    if (moditerm_m_net && moditerm_m_net->getBTerms().size() == 0
        && moditerm_m_net->getITerms().size() == 0
        && moditerm_m_net->getModITerms().size() == 0
        && moditerm_m_net->getModBTerms().size() == 0) {
      dbModNet::destroy(moditerm_m_net);
    }

    // Now destroy the iterm
    dbModITerm::destroy(mod_iterm);
    if (modbterm_m_net && modbterm_m_net->getBTerms().size() == 0
        && modbterm_m_net->getITerms().size() == 0
        && modbterm_m_net->getModITerms().size() == 0
        && modbterm_m_net->getModBTerms().size() == 0) {
      dbModNet::destroy(modbterm_m_net);
    }
    // Finally the bterm
    dbModBTerm::destroy(mod_bterm);
  }
}

// debugPrint for replace_design level 1
#define debugRDPrint1(format_str, ...) \
  debugPrint(logger, utl::ODB, "replace_design", 1, format_str, ##__VA_ARGS__)
// debugPrint for replace_design level 2
#define debugRDPrint2(format_str, ...) \
  debugPrint(logger, utl::ODB, "replace_design", 2, format_str, ##__VA_ARGS__)

// Check if two hierarchical modules are swappable.
// Two modules must have identical number of ports and port names need to match.
// Functional equivalence is not required.
bool canSwapModules(dbModule* old_module,
                    dbModule* new_module,
                    utl::Logger* logger)
{
  std::string old_module_name = old_module->getName();
  std::string new_module_name = new_module->getName();

  // Check if module names differ
  if (old_module_name == new_module_name) {
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
  std::vector<_dbModBTerm*> old_ports, new_ports;
  for (auto bterm : old_bterms) {
    old_ports.push_back((_dbModBTerm*) bterm);
  }
  for (auto bterm : new_bterms) {
    new_ports.push_back((_dbModBTerm*) bterm);
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

  return true;
}

// Swap one hierarchical module with another one.
// New module is not allowed to have multiple levels of hierarchy for now.
// Newly instantiated modules are uniquified.
dbModInst* dbModInst::swapMaster(dbModule* new_module)
{
  dbModule* old_module = getMaster();
  utl::Logger* logger = getImpl()->getLogger();

  if (!canSwapModules(old_module, new_module, logger)) {
    return nullptr;
  }

  if (logger->debugCheck(utl::ODB, "replace_design", 1)) {
    std::ofstream outfile("before_replace_top.txt");
    getMaster()->getOwner()->debugPrintContent(outfile);
    for (dbBlock* child_block : getMaster()->getOwner()->getChildren()) {
      std::string filename
          = "before_replace_" + child_block->getName() + ".txt";
      std::ofstream outfile(filename);
      child_block->debugPrintContent(outfile);
    }
  }

  dbModule* new_module_copy = dbModule::makeUniqueDbModule(
      new_module->getName(), this->getName(), getMaster()->getOwner());
  if (new_module_copy) {
    debugRDPrint1("Created uniquified module {} in block {}",
                  new_module_copy->getName(),
                  new_module_copy->getOwner()->getName());
  } else {
    logger->error(utl::ODB,
                  455,
                  "Unique module {} cannot be created",
                  new_module->getName());
    return nullptr;
  }

  std::string new_name = this->getName();
  dbModule* parent = this->getParent();
  // save mod nets and mod iterms because creating a new mod inst doesn't
  // create them automatically
  std::map<std::string, dbModNet*> name_mod_net_map;
  for (dbModITerm* old_mod_iterm : this->getModITerms()) {
    dbModNet* old_mod_net = old_mod_iterm->getModNet();
    name_mod_net_map[old_mod_iterm->getName()] = old_mod_net;
  }

  // Delete current mod inst and create a new one
  dbModInst::destroy(this);
  dbModInst* new_mod_inst
      = dbModInst::create(parent, new_module_copy, new_name.c_str());
  if (!new_mod_inst) {
    logger->error(utl::ODB, 471, "Mod instance {} cannot be created", new_name);
    return nullptr;
  }

  // Add mod iterms and connect to old mod nets
  for (const auto& [name, old_mod_net] : name_mod_net_map) {
    dbModITerm* new_mod_iterm = dbModITerm::create(new_mod_inst, name.c_str());
    if (new_mod_iterm && old_mod_net) {
      new_mod_iterm->connect(old_mod_net);
    }
  }
  debugRDPrint1("New mod inst has {} mod iterms",
                new_mod_inst->getModITerms().size());

  dbModule::copy(new_module, new_module_copy, new_mod_inst);  // NOLINT
  if (logger->debugCheck(utl::ODB, "replace_design", 2)) {
    for (dbInst* inst : new_module_copy->getInsts()) {
      logger->report("new_module_copy {} instance {} has the following iterms:",
                     new_module_copy->getName(),
                     inst->getName());
      for (dbITerm* iterm : inst->getITerms()) {
        logger->report("  iterm {}", iterm->getName());
      }
    }
  }

  // Map old mod nets to new mod nets based on new_module_copy
  dbSet<dbModBTerm> old_bterms = old_module->getModBTerms();
  std::map<dbModNet*, dbModNet*> mod_map;  // old mod net -> new mod net
  for (dbModBTerm* old_bterm : old_bterms) {
    dbModBTerm* new_bterm = new_module_copy->findModBTerm(old_bterm->getName());
    if (new_bterm == nullptr) {
      logger->error(utl::ODB,
                    466,
                    "modBTerm for {} is not found in copied module {}",
                    old_bterm->getName(),
                    new_module_copy->getName());
      return nullptr;
    }
    dbModNet* old_mod_net = old_bterm->getModNet();
    dbModNet* new_mod_net = new_bterm->getModNet();
    if (new_mod_net && old_mod_net) {
      mod_map[old_mod_net] = new_mod_net;
      debugRDPrint1("old mod net {} maps to new mod net {}",
                    old_mod_net->getName(),
                    new_mod_net->getName());
    }
  }

  // Patch connections such that boundary nets connect to new module iterms
  // instead of old module iterms
  debugRDPrint1("Connecting nets that span module boundary");
  for (const auto& [old_mod_net, new_mod_net] : mod_map) {
    dbSet<dbITerm> new_iterm_set = new_mod_net->getITerms();
    // Copy new iterms because new_mod_net iterm list can change
    std::vector<dbITerm*> new_iterms(new_iterm_set.begin(),
                                     new_iterm_set.end());
    for (dbITerm* old_iterm : old_mod_net->getITerms()) {
      dbNet* flat_net = old_iterm->getNet();
      if (flat_net) {
        old_iterm->disconnectDbNet();
        debugRDPrint1("  disconnected old iterm {} from flat net {}",
                      old_iterm->getName(),
                      flat_net->getName());
      }
      // iterm may be connected to another hierarchical instance
      dbModNet* other_mod_net = old_iterm->getModNet();
      if (other_mod_net != old_mod_net) {
        old_iterm->disconnectModNet();
        old_iterm->connect(old_mod_net);  // Reconnect old mod net for later use
        debugRDPrint1("  disconnected old iterm {} from other mod net {}",
                      old_iterm->getName(),
                      other_mod_net->getName());
      } else {
        other_mod_net = nullptr;
      }
      for (dbITerm* new_iterm : new_iterms) {
        if (flat_net) {
          // Bug Fix:
          // Explicitly kill connections to new_iterm
          // kill any old modnet connections and flat nets.
          //(for example the new_iterm on the new module
          // might be connected to some modnet).
          new_iterm->disconnect();  // kills both flat and hier net on new_iterm
          debugRDPrint1("  disconnected all conns from new iterm {}",
                        new_iterm->getName());
          // Connect the flat net, clears the old
          // flat net if any, but not the mod net
          new_iterm->connect(flat_net);
          debugRDPrint1("  connected new iterm {} to flat net {}",
                        new_iterm->getName(),
                        flat_net->getName());
          // this is needed because all mod nets were disconnected
          new_iterm->connect(new_mod_net);
          debugRDPrint1("  connected new iterm {} to mod net {}",
                        new_iterm->getName(),
                        new_mod_net->getName());
        }
        if (other_mod_net) {
          new_iterm->connect(other_mod_net);
          debugRDPrint1("  connected new iterm {} to other mod net {}",
                        new_iterm->getName(),
                        other_mod_net->getName());

        }  // clang-format off
      }  // for each new_iterm
    }  //  for each old_iterm
  }  // for each [old_mod_net, new_mod_net] pair
  // clang-format on

  // Remove any dangling nets
  std::vector<dbNet*> nets_to_delete;
  for (dbNet* net : parent->getOwner()->getNets()) {
    if (net->getITerms().empty()) {
      nets_to_delete.emplace_back(net);
    }
  }
  for (dbNet* net : nets_to_delete) {
    debugRDPrint1("  deleted dangling net {}", net->getName());
    dbNet::destroy(net);
  }

  dbModule::copyToChildBlock(old_module);
  debugRDPrint1("Copied to child block and deleted old module {}",
                old_module->getName());
  dbModule::destroy(old_module);

  if (logger->debugCheck(utl::ODB, "replace_design", 1)) {
    std::ofstream outfile("after_replace_top.txt");
    new_mod_inst->getMaster()->getOwner()->debugPrintContent(outfile);
    for (dbBlock* child_block : parent->getOwner()->getChildren()) {
      std::string filename = "after_replace_" + child_block->getName() + ".txt";
      std::ofstream outfile(filename);
      child_block->debugPrintContent(outfile);
    }
  }

  return new_mod_inst;
}

// User Code End dbModInstPublicMethods
}  // namespace odb
// Generator Code End Cpp
