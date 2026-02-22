// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2020-2025, The OpenROAD Authors

#include <cassert>
#include <cstring>
#include <fstream>
#include <iterator>
#include <map>
#include <set>
#include <string>
#include <vector>

// Generator Code Begin Cpp
#include <cstdlib>

#include "dbBlock.h"
#include "dbCore.h"
#include "dbDatabase.h"
#include "dbHashTable.hpp"
#include "dbJournal.h"
#include "dbModITerm.h"
#include "dbModInst.h"
#include "dbModule.h"
#include "dbTable.h"
#include "odb/db.h"
// User Code Begin Includes
#include <cstdint>

#include "dbCommon.h"
#include "dbGroup.h"
#include "dbModBTerm.h"
#include "dbModNet.h"
#include "dbModuleModInstItr.h"
#include "dbModuleModInstModITermItr.h"
#include "dbSwapMasterSanityChecker.h"
#include "odb/dbBlockCallBackObj.h"
#include "odb/dbObject.h"
#include "odb/dbSet.h"
#include "utl/Logger.h"
// User Code End Includes
namespace odb {
template class dbTable<_dbModInst>;

bool _dbModInst::operator==(const _dbModInst& rhs) const
{
  if (name_ != rhs.name_) {
    return false;
  }
  if (next_entry_ != rhs.next_entry_) {
    return false;
  }
  if (parent_ != rhs.parent_) {
    return false;
  }
  if (module_next_ != rhs.module_next_) {
    return false;
  }
  if (master_ != rhs.master_) {
    return false;
  }
  if (group_next_ != rhs.group_next_) {
    return false;
  }
  if (group_ != rhs.group_) {
    return false;
  }
  if (moditerms_ != rhs.moditerms_) {
    return false;
  }

  return true;
}

bool _dbModInst::operator<(const _dbModInst& rhs) const
{
  // User Code Begin <
  if (strcmp(name_, rhs.name_) >= 0) {
    return false;
  }
  // User Code End <
  return true;
}

_dbModInst::_dbModInst(_dbDatabase* db)
{
  // User Code Begin Constructor
  name_ = nullptr;
  parent_ = 0;
  module_next_ = 0;
  moditerms_ = 0;
  master_ = 0;
  group_ = 0;
  group_next_ = 0;
  // User Code End Constructor
}

dbIStream& operator>>(dbIStream& stream, _dbModInst& obj)
{
  stream >> obj.name_;
  stream >> obj.next_entry_;
  stream >> obj.parent_;
  stream >> obj.module_next_;
  stream >> obj.master_;
  stream >> obj.group_next_;
  stream >> obj.group_;
  // User Code Begin >>
  dbBlock* block = (dbBlock*) (obj.getOwner());
  _dbDatabase* db_ = (_dbDatabase*) (block->getDataBase());
  if (db_->isSchema(kSchemaUpdateHierarchy)) {
    stream >> obj.moditerms_;
  }
  // User Code End >>
  return stream;
}

dbOStream& operator<<(dbOStream& stream, const _dbModInst& obj)
{
  stream << obj.name_;
  stream << obj.next_entry_;
  stream << obj.parent_;
  stream << obj.module_next_;
  stream << obj.master_;
  stream << obj.group_next_;
  stream << obj.group_;
  // User Code Begin <<
  stream << obj.moditerms_;
  // User Code End <<
  return stream;
}

void _dbModInst::collectMemInfo(MemInfo& info)
{
  info.cnt++;
  info.size += sizeof(*this);

  // User Code Begin collectMemInfo
  info.children["name"].add(name_);
  info.children["moditerm_hash"].add(moditerm_hash_);
  // User Code End collectMemInfo
}

_dbModInst::~_dbModInst()
{
  if (name_) {
    free((void*) name_);
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
  return obj->name_;
}

dbModule* dbModInst::getParent() const
{
  _dbModInst* obj = (_dbModInst*) this;
  if (obj->parent_ == 0) {
    return nullptr;
  }
  _dbBlock* par = (_dbBlock*) obj->getOwner();
  return (dbModule*) par->module_tbl_->getPtr(obj->parent_);
}

dbModule* dbModInst::getMaster() const
{
  _dbModInst* obj = (_dbModInst*) this;
  if (obj->master_ == 0) {
    return nullptr;
  }
  _dbBlock* par = (_dbBlock*) obj->getOwner();
  return (dbModule*) par->module_tbl_->getPtr(obj->master_);
}

dbGroup* dbModInst::getGroup() const
{
  _dbModInst* obj = (_dbModInst*) this;
  if (obj->group_ == 0) {
    return nullptr;
  }
  _dbBlock* par = (_dbBlock*) obj->getOwner();
  return (dbGroup*) par->group_tbl_->getPtr(obj->group_);
}

// User Code Begin dbModInstPublicMethods
dbModInst* dbModInst::create(dbModule* parentModule,
                             dbModule* masterModule,
                             const char* name)
{
  _dbModule* module = (_dbModule*) parentModule;
  _dbBlock* block = (_dbBlock*) module->getOwner();
  _dbModule* master = (_dbModule*) masterModule;

  if (master->mod_inst_ != 0) {
    return nullptr;
  }

  dbModInst* ret = nullptr;
  ret = ((dbModule*) module)->findModInst(name);
  if (ret) {
    return nullptr;
  }

  _dbModInst* modinst = block->modinst_tbl_->create();

  if (block->journal_) {
    block->journal_->beginAction(dbJournal::kCreateObject);
    block->journal_->pushParam(dbModInstObj);
    block->journal_->pushParam(name);
    block->journal_->pushParam(modinst->getId());
    block->journal_->pushParam(module->getId());
    block->journal_->pushParam(master->getId());
    block->journal_->endAction();
  }

  modinst->name_ = safe_strdup(name);
  modinst->master_ = master->getOID();
  modinst->parent_ = module->getOID();
  // push to head of list in block
  modinst->module_next_ = module->modinsts_;
  module->modinsts_ = modinst->getOID();
  master->mod_inst_ = modinst->getOID();
  module->modinst_hash_[modinst->name_] = modinst->getOID();

  debugPrint(block->getImpl()->getLogger(),
             utl::ODB,
             "DB_EDIT",
             1,
             "EDIT: create {}",
             modinst->getDebugName());

  for (dbBlockCallBackObj* cb : block->callbacks_) {
    cb->inDbModInstCreate((dbModInst*) modinst);
  }

  return (dbModInst*) modinst;
}

void dbModInst::destroy(dbModInst* modinst)
{
  _dbModInst* _modinst = (_dbModInst*) modinst;
  _dbBlock* _block = (_dbBlock*) _modinst->getOwner();
  _dbModule* _module = (_dbModule*) modinst->getParent();

  _dbModule* _master = (_dbModule*) modinst->getMaster();

  // Note that we only destroy the module instance, not the module
  // itself

  dbSet<dbModITerm> moditerms = modinst->getModITerms();
  dbSet<dbModITerm>::iterator moditerm_itr;
  for (moditerm_itr = moditerms.begin(); moditerm_itr != moditerms.end();) {
    dbModITerm* moditerm = *moditerm_itr;
    // pins disconnected before deletion, so we restore pin before
    // trying to connect on journalling restore of modInst.
    moditerm->disconnect();
    moditerm_itr = dbModITerm::destroy(moditerm_itr);
  }

  for (auto cb : _block->callbacks_) {
    cb->inDbModInstDestroy(modinst);
  }

  // This must be called after callbacks because they need _mod_inst
  _master->mod_inst_.clear();

  // unlink from parent start
  uint32_t id = _modinst->getOID();
  _dbModInst* prev = nullptr;
  uint32_t cur = _module->modinsts_;
  while (cur) {
    _dbModInst* c = _block->modinst_tbl_->getPtr(cur);
    if (cur == id) {
      if (prev == nullptr) {
        _module->modinsts_ = _modinst->module_next_;
      } else {
        prev->module_next_ = _modinst->module_next_;
      }
      break;
    }
    prev = c;
    cur = c->module_next_;
  }

  dbProperty::destroyProperties(_modinst);

  debugPrint(_block->getImpl()->getLogger(),
             utl::ODB,
             "DB_EDIT",
             1,
             "EDIT: delete {}",
             modinst->getDebugName());

  // Assure that dbModInst obj is restored first by being journalled last.
  if (_block->journal_) {
    _block->journal_->beginAction(dbJournal::kDeleteObject);
    _block->journal_->pushParam(dbModInstObj);
    _block->journal_->pushParam(modinst->getName());
    _block->journal_->pushParam(modinst->getId());
    _block->journal_->pushParam(_module->getId());
    _block->journal_->pushParam(_master->getId());
    _block->journal_->pushParam(_modinst->group_);
    _block->journal_->endAction();
  }

  // unlink from parent end
  if (_modinst->group_) {
    modinst->getGroup()->removeModInst(modinst);
  }

  _dbModule* _parent = (_dbModule*) (modinst->getParent());
  _parent->modinst_hash_.erase(modinst->getName());
  _block->modinst_tbl_->destroy(_modinst);
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
  return dbSet<dbModITerm>(_mod_inst, _block->module_modinstmoditerm_itr_);
}

dbModInst* dbModInst::getModInst(dbBlock* block_, uint32_t dbid_)
{
  _dbBlock* block = (_dbBlock*) block_;
  return (dbModInst*) block->modinst_tbl_->getPtr(dbid_);
}

std::string dbModInst::getHierarchicalName() const
{
  _dbModInst* _obj = (_dbModInst*) this;
  dbBlock* block = (dbBlock*) _obj->getOwner();
  const char* inst_name = getName();
  dbModule* parent = getParent();
  if (parent == block->getTopModule()) {
    return inst_name;
  }
  return fmt::format("{}{}{}",
                     parent->getModInst()->getHierarchicalName(),
                     block->getHierarchyDelimiter(),
                     inst_name);
}

dbModITerm* dbModInst::findModITerm(const char* name)
{
  _dbModInst* obj = (_dbModInst*) this;
  _dbBlock* par = (_dbBlock*) obj->getOwner();
  auto it = obj->moditerm_hash_.find(name);
  if (it != obj->moditerm_hash_.end()) {
    auto db_id = (*it).second;
    return (dbModITerm*) par->moditerm_tbl_->getPtr(db_id);
  }
  return nullptr;
}

dbModNet* dbModInst::findHierNet(const char* base_name) const
{
  dbModule* master = getMaster();
  return master->getModNet(base_name);
}

dbNet* dbModInst::findFlatNet(const char* base_name) const
{
  dbModule* parent = getParent();
  if (parent) {
    dbBlock* block = parent->getOwner();
    fmt::memory_buffer full_name_buf;
    fmt::format_to(std::back_inserter(full_name_buf),
                   "{}{}{}",
                   getHierarchicalName(),
                   block->getHierarchyDelimiter(),
                   base_name);
    return block->findNet(full_name_buf.data());
  }
  return nullptr;
}

bool dbModInst::findNet(const char* base_name,
                        dbNet*& flat_net,
                        dbModNet*& hier_net) const
{
  flat_net = findFlatNet(base_name);
  hier_net = findHierNet(base_name);
  return (flat_net || hier_net);
}

void dbModInst::removeUnusedPortsAndPins()
{
  _dbModInst* obj = (_dbModInst*) this;
  utl::Logger* logger = obj->getLogger();
  debugPrint(logger,
             utl::ODB,
             "remove_unused_ports",
             1,
             "begin RemoveUnusedPortsAndPins for dbModInst '{}'",
             getName());

  dbModule* module = this->getMaster();
  dbSet<dbModBTerm> modbterms = module->getModBTerms();
  std::set<dbModBTerm*> busmodbterms;  // harvest the bus modbterms

  // 1. Traverse in modbterm order so we can skip over any unused pins in a bus.
  int bus_ix = 0;
  for (dbModBTerm* mod_bterm : modbterms) {
    // Avoid removing unused ports from a bus
    // when we hit a bus port, we count down from the size
    // skipping the bus elements
    // Note that dbSet<dbModBTerm> preserves the insertion order.
    //
    // Layout:
    // mod_bterm (head_element describing size)
    // mod_bterm[size-1],...mod_bterm[0] -- bus elements
    //
    if (mod_bterm->isBusPort()) {
      dbBusPort* bus_port = mod_bterm->getBusPort();
      bus_ix = bus_port->getSize();  // count down
      busmodbterms.insert(mod_bterm);
      continue;
    }
    if (bus_ix != 0) {
      bus_ix--;
      busmodbterms.insert(mod_bterm);
    }
  }

  // 2. Find unused ports that do not have internal connections
  std::set<dbModITerm*> kill_set;
  for (dbModITerm* mod_iterm : getModITerms()) {
    dbModBTerm* mod_bterm = module->findModBTerm(mod_iterm->getName());
    assert(mod_bterm != nullptr);

    if (busmodbterms.contains(mod_bterm)) {
      continue;  // Do not remove bus ports
    }

    // Check internal connectivity (inside the dbModule master).
    bool unused_in_module = true;
    if (dbModNet* int_net = mod_bterm->getModNet()) {
      bool has_int_mod_inst_connection = !int_net->getModITerms().empty();
      bool has_top_port_connection = !int_net->getBTerms().empty();
      bool has_int_inst_connection = !int_net->getITerms().empty();
      bool has_feedthrough_connection = int_net->getModBTerms().size() > 1;
      if (has_int_mod_inst_connection || has_top_port_connection
          || has_int_inst_connection || has_feedthrough_connection) {
        unused_in_module = false;
      }
    }

    if (unused_in_module) {
      kill_set.insert(mod_iterm);
    }
  }

  // 3. Remove unused ports in kill_set
  for (auto mod_iterm : kill_set) {
    dbModNet* moditerm_m_net = mod_iterm->getModNet();
    dbModBTerm* mod_bterm = module->findModBTerm(mod_iterm->getName());
    assert(mod_bterm != nullptr);

    dbModNet* modbterm_m_net = mod_bterm->getModNet();

    // Do the destruction in order for benefit of journaller
    // so we always have a dbModBTerm..
    // first destroy net, then dbModIterm, then dbModbterm.
    mod_iterm->disconnect();
    mod_bterm->disconnect();

    // First destroy the net
    if (moditerm_m_net && moditerm_m_net->getModITerms().empty()
        && moditerm_m_net->getModBTerms().empty()) {
      dbModNet::destroy(moditerm_m_net);
    }

    // Now destroy the iterm
    dbModITerm::destroy(mod_iterm);

    if (modbterm_m_net && modbterm_m_net->getModITerms().empty()
        && modbterm_m_net->getModBTerms().empty()) {
      dbModNet::destroy(modbterm_m_net);
    }

    // Finally the bterm
    dbModBTerm::destroy(mod_bterm);
  }

  debugPrint(
      logger,
      utl::ODB,
      "remove_unused_ports",
      1,
      "end RemoveUnusedPortsAndPins for dbModInst '{}', removed {} iterms",
      getName(),
      kill_set.size());
}

// debugPrint for replace_design level 1
#define debugRDPrint1(format_str, ...) \
  debugPrint(logger, utl::ODB, "replace_design", 1, format_str, ##__VA_ARGS__)
// debugPrint for replace_design level 2
#define debugRDPrint2(format_str, ...) \
  debugPrint(logger, utl::ODB, "replace_design", 2, format_str, ##__VA_ARGS__)

// Swap one hierarchical module with another one.
// New module is not allowed to have multiple levels of hierarchy for now.
// Newly instantiated modules are uniquified.
dbModInst* dbModInst::swapMaster(dbModule* new_module)
{
  dbModule* old_module = getMaster();
  utl::Logger* logger = getImpl()->getLogger();

  // Helper to remove dangling nets from a block
  auto removeDanglingNets = [](dbBlock* block, utl::Logger* logger) {
    std::vector<dbNet*> nets_to_delete;
    for (dbNet* net : block->getNets()) {
      if (net->getITerms().empty() && net->getBTerms().empty()
          && !net->isSpecial()) {
        nets_to_delete.emplace_back(net);
      } else {
        debugRDPrint2("  retained net {} with {} iterms and {} bterms",
                      net->getName(),
                      net->getITerms().size(),
                      net->getBTerms().size());
      }
    }
    for (dbNet* net : nets_to_delete) {
      debugRDPrint2("  deleted dangling net {}", net->getName());
      dbNet::destroy(net);
    }
  };

  // 1. Check if swap is allowed
  if (!old_module->canSwapWith(new_module)) {
    return nullptr;
  }

  // Write out the design before replacement for debugging
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

  // 2. Create a uniquified copy of new_module and its ports
  _dbModule::modBTMap mod_bt_map;
  dbModule* new_module_copy = dbModule::makeUniqueDbModule(
      new_module->getName(), this->getName(), getMaster()->getOwner());
  if (new_module_copy) {
    // Copy module ports from new_module to new_module_copy.
    // - This allows dbModITerms to be connected to dbModBTerms when they are
    //   created later.
    _dbModule::copyModulePorts(  // NOLINT
        new_module,
        new_module_copy,
        mod_bt_map);
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

  // 3. Save mod nets and mod iterms
  // - Because creating a new mod inst doesn't create them automatically.
  std::string new_name = this->getName();
  dbModule* parent = this->getParent();
  std::map<std::string, dbModNet*> name_mod_net_map;
  for (dbModITerm* old_mod_iterm : this->getModITerms()) {
    dbModNet* old_mod_net = old_mod_iterm->getModNet();
    name_mod_net_map[old_mod_iterm->getName()] = old_mod_net;
  }

  // 4. Build a map (key: modBTerm_name, value: dbNet)
  // - To store modBTerm-dbNet connectivity before old module is deleted.
  debugRDPrint1("Build a map(modBTerm_name:dbNet) with old module '{}'",
                old_module->getName());
  std::map<std::string, dbNet*> modbterm_name_flat_net_map;
  for (dbModBTerm* old_modbterm : old_module->getModBTerms()) {
    dbModITerm* old_mod_iterm = old_modbterm->getParentModITerm();
    if (old_mod_iterm == nullptr) {
      debugRDPrint2("  modBTerm '{}' does not have a corresponding modITerm.",
                    old_modbterm->getName());
      continue;
    }

    // Get the external mod net connected to the mod iterm
    dbModNet* ext_mod_net = old_mod_iterm->getModNet();
    if (ext_mod_net == nullptr) {
      continue;
    }

    debugRDPrint1("  modBTerm '{}' connects to mod net '{}'",
                  old_modbterm->getName(),
                  ext_mod_net->getName());

    // Find the flat net connected to old_mod_net
    dbNet* flat_net = ext_mod_net->findRelatedNet();
    if (flat_net == nullptr) {
      debugRDPrint2(
          "  ERROR: modBTerm '{}' connects to mod net '{}' that does not "
          "have a flat net.",
          old_modbterm->getName(),
          ext_mod_net->getName());
      continue;
    }

    // If the flat net has external connection (external instance or BTerm),
    // it should be inserted into modbterm_name_flat_net_map.
    bool has_external_connection = (flat_net->getBTerms().empty() == false);
    if (has_external_connection == false) {
      for (dbITerm* iterm : flat_net->getITerms()) {
        if (!old_module->containsDbInst(iterm->getInst())) {
          has_external_connection = true;
          break;
        }
      }
    }

    // Store the mapping if there is external connection
    if (has_external_connection) {
      modbterm_name_flat_net_map[old_modbterm->getName()] = flat_net;
      debugRDPrint2("  insert on map[modBTerm '{}'] = flat net '{}'",
                    old_modbterm->getName(),
                    flat_net->getName());
      if (logger->debugCheck(utl::ODB, "replace_design", 3)) {
        flat_net->dump();
      }
    }
  }

  // 5. Delete current mod inst
  dbModInst::destroy(this);

  // 6. Create a new mod inst of new_module_copy
  dbModInst* new_mod_inst
      = dbModInst::create(parent, new_module_copy, new_name.c_str());
  if (!new_mod_inst) {
    logger->error(utl::ODB, 471, "Mod instance {} cannot be created", new_name);
    return nullptr;
  }

  // 7. Create mod iterms and connect to old mod nets
  for (const auto& [name, old_mod_net] : name_mod_net_map) {
    // Find the corresponding ModBTerm in the new master module
    dbModBTerm* new_mod_bterm = new_module_copy->findModBTerm(name.c_str());
    assert(new_mod_bterm != nullptr);
    dbModITerm* new_mod_iterm
        = dbModITerm::create(new_mod_inst, name.c_str(), new_mod_bterm);
    if (new_mod_iterm && old_mod_net) {
      new_mod_iterm->connect(old_mod_net);
    }
  }
  debugRDPrint1("New mod inst has {} mod iterms",
                new_mod_inst->getModITerms().size());

  // 8. Backup old dbModule to child block
  _dbModule::copyToChildBlock(old_module);
  debugRDPrint1("Copied to child block and deleted old module {} ",
                old_module->getName());

  // 9. Delete the old dbModule
  dbModule::destroy(old_module);

  // 10. Remove dangling internal nets of the old module
  // - Note that internal nets of old module belongs to parent block.
  //   So they should be removed from parent block explicitly.
  removeDanglingNets(parent->getOwner(), logger);

  // 11. Deep copy contents of new_module to new_module_copy
  // - This will create internal nets and instances under new_module_copy
  // - But nets crossing the module boundary are not connected yet.
  _dbModule::copy(  // NOLINT
      new_module,
      new_module_copy,
      new_mod_inst,
      mod_bt_map);
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

  // 12. Connect nets crossing the hierarchical boundary
  debugRDPrint1("Connecting nets that span module boundary");
  for (const auto& [bterm_name, flat_net] : modbterm_name_flat_net_map) {
    debugRDPrint1("  map_entry[modBTermName '{}'] = flat net '{}'",
                  bterm_name,
                  flat_net->getName());

    dbModBTerm* new_modbterm
        = new_module_copy->findModBTerm(bterm_name.c_str());
    if (new_modbterm == nullptr) {
      logger->error(utl::ODB,
                    466,
                    "modBTerm '{}' is not found in copied module '{}'",
                    bterm_name,
                    new_module_copy->getName());
      return nullptr;
    }
    dbModNet* new_mod_net = new_modbterm->getModNet();
    debugRDPrint1("  patching flat net '{}' to new mod net '{}'",
                  flat_net->getName(),
                  new_mod_net ? new_mod_net->getName() : "<none>");
    if (new_mod_net == nullptr) {
      continue;
    }

    // Connect flat net to new mod net iterms
    // Copy to a vector because disconnect/connect can change the dbSet
    // while iterating.
    dbSet<dbITerm> new_iterm_set = new_mod_net->getITerms();
    if (new_iterm_set.empty()) {
      debugRDPrint1("    new modnet '{}' has no iterms",
                    new_mod_net->getName());
      continue;
    }

    std::vector<dbITerm*> new_iterms(new_iterm_set.begin(),
                                     new_iterm_set.end());
    for (dbITerm* new_iterm : new_iterms) {
      // Disconnect the old connection
      // - e.g., the new_iterm on the new module might be connected to new
      // modnet that is created when the new module is cloned.
      new_iterm->disconnect();  // Disconnect both flat/hier nets on new_iterm
      debugRDPrint1("    disconnected all conns from iterm {}",
                    new_iterm->getName());

      // Connect the flat and hier nets
      new_iterm->connect(flat_net, new_mod_net);
      debugRDPrint1(
          "    connected iterm '{}' to flat net '{}' and hier net '{}'",
          new_iterm->getName(),
          flat_net->getName(),
          new_mod_net->getName());
    }
  }

  // 13. Final clean up
  removeDanglingNets(parent->getOwner(), logger);

  // Write out the design after replacement for debugging
  if (logger->debugCheck(utl::ODB, "replace_design", 1)) {
    std::ofstream outfile("after_replace_top.txt");
    new_mod_inst->getMaster()->getOwner()->debugPrintContent(outfile);
    for (dbBlock* child_block : parent->getOwner()->getChildren()) {
      std::string filename = "after_replace_" + child_block->getName() + ".txt";
      std::ofstream outfile(filename);
      child_block->debugPrintContent(outfile);
    }
  }

  if (logger->debugCheck(utl::ODB, "replace_design_check_sanity", 1)) {
    dbSwapMasterSanityChecker checker(new_mod_inst, new_module, logger);
    checker.run();
  }

  return new_mod_inst;
}

bool dbModInst::containsDbInst(dbInst* inst) const
{
  dbModule* master = getMaster();
  if (master == nullptr) {
    return false;
  }

  // Check direct child dbInsts
  for (dbInst* child_inst : master->getInsts()) {
    if (child_inst == inst) {
      return true;
    }
  }

  // Recursively check child dbModInsts
  for (dbModInst* child_mod_inst : master->getModInsts()) {
    if (child_mod_inst->containsDbInst(inst)) {
      return true;
    }
  }

  return false;
}

bool dbModInst::containsDbModInst(dbModInst* inst) const
{
  dbModule* master = getMaster();
  if (master == nullptr) {
    return false;
  }

  // Recursively check child dbModInsts
  for (dbModInst* child_mod_inst : master->getModInsts()) {
    if (child_mod_inst == inst || child_mod_inst->containsDbModInst(inst)) {
      return true;
    }
  }

  return false;
}

// User Code End dbModInstPublicMethods
}  // namespace odb
// Generator Code End Cpp
