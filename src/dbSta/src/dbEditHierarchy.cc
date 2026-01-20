// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

// dbEditHierarchy, Manipulating hierarchical relationships in OpenDB

#include "dbEditHierarchy.hh"

#include <algorithm>
#include <cassert>
#include <set>
#include <string>
#include <vector>

#include "db_sta/dbNetwork.hh"
#include "odb/db.h"
#include "sta/NetworkClass.hh"
#include "utl/Logger.h"

namespace sta {

void dbEditHierarchy::getParentHierarchy(
    dbModule* start_module,
    std::vector<dbModule*>& parent_hierarchy) const
{
  dbModule* top_module = db_network_->block()->getTopModule();
  dbModule* cur_module = start_module;
  while (cur_module) {
    parent_hierarchy.push_back(cur_module);
    if (cur_module == top_module) {
      return;
    }
    cur_module = cur_module->getModInst()
                     ? cur_module->getModInst()->getParent()
                     : nullptr;
  }
}

dbModule* dbEditHierarchy::findLowestCommonModule(
    std::vector<dbModule*>& itree1,
    std::vector<dbModule*>& itree2) const
{
  int ix1 = itree1.size();
  int ix2 = itree2.size();
  int limit = std::min(ix1, ix2);
  dbModule* top_module = db_network_->block()->getTopModule();

  // reverse traversal. (note hierarchy stored so top is end of list)
  // get to first divergence
  std::vector<dbModule*>::reverse_iterator itree1_iter = itree1.rbegin();
  std::vector<dbModule*>::reverse_iterator itree2_iter = itree2.rbegin();
  dbModule* common_module = top_module;
  if (limit > 0) {
    for (int i = 0; i != limit; i++) {
      if (*itree1_iter != *itree2_iter) {
        return common_module;
      }
      common_module = *itree1_iter;
      itree1_iter++;
      itree2_iter++;
    }
  }
  return common_module;  // default to top
}

class PinModuleConnection : public PinVisitor
{
 public:
  PinModuleConnection(const dbNetwork* nwk, const dbModule* target_module);
  void operator()(const Pin* pin) override;
  dbModBTerm* getModBTerm() const { return dest_modbterm_; }
  dbModITerm* getModITerm() const { return dest_moditerm_; }

 private:
  const dbNetwork* db_network_;
  const dbModule* target_module_;
  dbModBTerm* dest_modbterm_{nullptr};
  dbModITerm* dest_moditerm_{nullptr};
};

PinModuleConnection::PinModuleConnection(const dbNetwork* nwk,
                                         const dbModule* target_module)
    : db_network_(nwk), target_module_(target_module)
{
}

void PinModuleConnection::operator()(const Pin* pin)
{
  dbITerm* iterm;
  dbBTerm* bterm;
  dbModITerm* moditerm;

  db_network_->staToDb(pin, iterm, bterm, moditerm);
  (void) (iterm);
  (void) (bterm);
  if (moditerm) {
    dbModBTerm* modbterm = moditerm->getChildModBTerm();
    if (modbterm->getParent() == target_module_) {
      dest_modbterm_ = modbterm;
    }
  }
}

bool dbEditHierarchy::connectionToModuleExists(dbITerm* source_pin,
                                               dbModule* dest_module,
                                               dbModBTerm*& dest_modbterm,
                                               dbModITerm*& dest_moditerm) const
{
  PinModuleConnection visitor(db_network_, dest_module);
  NetSet visited_nets(db_network_);
  db_network_->visitConnectedPins(
      db_network_->net(db_network_->dbToSta(source_pin)),
      visitor,
      visited_nets);
  if (visitor.getModBTerm() != nullptr) {
    dest_modbterm = visitor.getModBTerm();
    return true;
  }
  if (visitor.getModITerm() != nullptr) {
    dest_moditerm = visitor.getModITerm();
    return true;
  }
  return false;
}

// Create all intermediate hierarchical pin and net from the pin hierarchy to
// the lowest_common_module hierarchy
//
// pin: start pin at the bottom
// lowest_common_module: the target hierarchy for the bottom-up hierarchy
//                        creation
// io_type:
//  - dbIoType::OUTPUT: create output pins from pin's module -> common module
//  - dbIoType::INPUT:  create input pins from pin's module -> common module
// connection_name: name for the new pins & nets
// top_mod_net: newly created ModNet in the lowest common hierarchy
// top_mod_iterm: newly created ModITerm in the lowest common hierarchy
void dbEditHierarchy::createHierarchyBottomUp(Pin* pin,
                                              dbModule* lowest_common_module,
                                              const dbIoType& io_type,
                                              const char* connection_name,
                                              dbModNet*& top_mod_net,
                                              dbModITerm*& top_mod_iterm) const
{
  int level = 0;
  dbITerm* iterm = nullptr;
  dbModITerm* moditerm = nullptr;
  dbBTerm* bterm = nullptr;
  db_network_->staToDb(pin, iterm, bterm, moditerm);

  dbModule* cur_module = nullptr;
  if (iterm) {
    cur_module = iterm->getInst()->getModule();
  } else if (moditerm) {
    cur_module = moditerm->getParent()->getParent();
  }

  if (cur_module == nullptr) {
    logger_->error(utl::ORD,
                   69,
                   "Module of Pin '{}' cannot be null.",
                   db_network_->name(pin));
  }

  dbModNet* db_mod_net = nullptr;
  const char* io_type_str = (io_type == dbIoType::OUTPUT) ? "o" : "i";

  // Decide a new unique pin/net name
  std::string new_term_net_name
      = makeUniqueName(cur_module, connection_name, io_type_str);

  while (cur_module != lowest_common_module) {
    // Create BTerm & ModNet (if not exist) and connect them
    dlogCreateHierBTermAndModNet(level, cur_module, new_term_net_name);
    dbModBTerm* mod_bterm
        = dbModBTerm::create(cur_module, new_term_net_name.c_str());

    db_mod_net = cur_module->getModNet(new_term_net_name.c_str());
    if (db_mod_net == nullptr) {
      db_mod_net = dbModNet::create(cur_module, new_term_net_name.c_str());
    }

    mod_bterm->connect(db_mod_net);
    mod_bterm->setIoType(io_type);
    mod_bterm->setSigType(dbSigType::SIGNAL);

    // Make connection at leaf level
    if (level == 0) {
      dbModNet* pin_mod_net = db_network_->hierNet(pin);
      if (pin_mod_net) {
        // if pin is already connected. disconnect it
        dlogCreateHierDisconnectingPin(level, cur_module, pin, pin_mod_net);
        db_network_->disconnectPin(pin, (Net*) pin_mod_net);
      }
      dlogCreateHierConnectingPin(level, cur_module, pin, db_mod_net);
      db_network_->connectPin(pin, (Net*) db_mod_net);
    }

    // Set next target hierarchy (goes up to the parent)
    dbModInst* parent_inst = cur_module->getModInst();
    cur_module = parent_inst->getParent();
    level++;

    // Create ITerm in parent hierarchy
    dlogCreateHierCreatingITerm(
        level, cur_module, parent_inst, new_term_net_name);
    dbModITerm* mod_iterm
        = dbModITerm::create(parent_inst, new_term_net_name.c_str(), mod_bterm);

    // Retry to get a new unique pin/net name in the new hierarchy
    new_term_net_name
        = makeUniqueName(cur_module, connection_name, io_type_str);

    // Create ModNet for the ITerm
    if (io_type == dbIoType::OUTPUT
        || (io_type == dbIoType::INPUT && cur_module != lowest_common_module)) {
      db_mod_net = dbModNet::create(cur_module, new_term_net_name.c_str());
      mod_iterm->connect(db_mod_net);
      dlogCreateHierConnectingITerm(
          level, cur_module, parent_inst, new_term_net_name, db_mod_net);
    }

    // Save the top level pin and net for final connection later
    top_mod_iterm = mod_iterm;
    top_mod_net = db_mod_net;
  }
}

/*
Connect any two leaf instance pins anywhere in hierarchy adding pins/nets/ports
on the hierarchical objects.

Note that this will also make sure there is a "flat" connection between source
and destination pins.

We supply the "preferred connection name" via connection_name.
This is used to name the flat net if required and also the dbModBTerms and
dbModITerms are used to build up the hierarhical connection. Note that
connection_name should be a base name, not a full name.
*/
void dbEditHierarchy::hierarchicalConnect(dbITerm* source_pin,
                                          dbITerm* dest_pin,
                                          const char* connection_name)
{
  assert(source_pin != nullptr);
  assert(dest_pin != nullptr);
  connection_name = getBaseName(connection_name);
  dlogHierConnStart(
      source_pin, db_network_->dbToSta(dest_pin), connection_name);

  //
  // 1. Connect source and dest pins directly in flat flow
  //
  dbNet* source_db_net = source_pin->getNet();
  dbNet* dest_db_net = dest_pin->getNet();
  if (db_network_->hasHierarchy() == false) {
    // If both source pin and dest pin do not have a corresponding flat net,
    // Create a new net and connect it with source pin.
    if (source_db_net == nullptr && dest_db_net == nullptr) {
      Net* new_net = db_network_->makeNet(
          connection_name,
          db_network_->parent(db_network_->dbToSta(source_pin->getInst())),
          odb::dbNameUniquifyType::IF_NEEDED);
      source_db_net = db_network_->staToDb(new_net);
      source_pin->connect(source_db_net);
    }

    // Connect
    if (source_db_net) {
      dest_pin->connect(source_db_net);
    } else {
      assert(dest_db_net);
      source_pin->connect(dest_db_net);
    }

    dlogHierConnDone();
    return;  // Done here in a flat flow
  }

  //
  // 2. Make sure there is a direct flat net connection
  // Recall the hierarchical connections are overlayed
  // onto the flat db network, so we have both worlds
  // co-existing, something we respect even when making
  // new hierarchical connections.
  //
  if (!source_db_net) {
    dlogHierConnCreateFlatNet(connection_name);
    Net* new_net = db_network_->makeNet(
        connection_name,
        db_network_->parent(db_network_->dbToSta(source_pin->getInst())),
        odb::dbNameUniquifyType::IF_NEEDED);
    source_db_net = db_network_->staToDb(new_net);
    source_pin->connect(source_db_net);
    dlogHierConnConnectSrcToFlatNet(source_pin, connection_name);
  }
  if (!db_network_->isConnected(db_network_->dbToSta(source_pin),
                                db_network_->dbToSta(dest_pin))) {
    dest_pin->connect(source_db_net);
    dlogHierConnConnectDstToFlatNet(dest_pin, source_db_net);
  }

  //
  // 3. Make the hierarchical connection.
  // in case when pins in different modules
  //

  // Get the scope (dbModule*) of the source and destination pins
  dbModule* source_db_module = source_pin->getInst()->getModule();
  dbModule* dest_db_module = dest_pin->getInst()->getModule();
  // it is possible that one or other of the pins is not involved
  // in hierarchy, which is ok, and the source/dest modnet will be null
  dbModNet* source_db_mod_net = source_pin->getModNet();
  if (source_db_module != dest_db_module) {
    //
    // 3.1. Attempt to factor connection (minimize punch through), and return
    //
    dbModBTerm* dest_modbterm = nullptr;
    dbModITerm* dest_moditerm = nullptr;
    // Check do we already have a connection between the source and destination
    // pins? If so, reuse it.
    if (connectionToModuleExists(
            source_pin, dest_db_module, dest_modbterm, dest_moditerm)) {
      dbModNet* dest_mod_net = nullptr;
      if (dest_modbterm) {
        dest_mod_net = dest_modbterm->getModNet();
      } else if (dest_moditerm) {
        dest_mod_net = dest_moditerm->getModNet();
      }
      if (dest_mod_net) {
        dlogHierConnReusingConnection(dest_db_module, dest_mod_net);
        Pin* sta_dest_pin = db_network_->dbToSta(dest_pin);
        dbNet* dest_flat_net = db_network_->flatNet(sta_dest_pin);
        db_network_->disconnectPin(sta_dest_pin);
        db_network_->connectPin(
            sta_dest_pin, (Net*) dest_flat_net, (Net*) dest_mod_net);
        return;
      }
    }

    // 3.2. No existing connection. Find lowest common module, traverse up
    // adding pins/nets and make connection in lowest common module
    std::vector<dbModule*> source_parent_tree;
    std::vector<dbModule*> dest_parent_tree;
    getParentHierarchy(source_db_module, source_parent_tree);
    getParentHierarchy(dest_db_module, dest_parent_tree);

    dbModule* lowest_common_module
        = findLowestCommonModule(source_parent_tree, dest_parent_tree);

    // 3.3. Make source hierarchy (bottom to top).
    // - source_pin -> lowest_common_module
    // - Make output pins and nets intermediate hierarchies
    // - Goes up from source hierarchy to lowest common hierarchy
    dbModNet* top_source_mod_net = source_db_mod_net;
    dbModITerm* top_source_mod_iterm = nullptr;
    if (source_db_module != lowest_common_module) {
      dlogHierConnCreatingSrcHierarchy(source_pin, lowest_common_module);
      createHierarchyBottomUp(db_network_->dbToSta(source_pin),
                              lowest_common_module,
                              dbIoType::OUTPUT,
                              connection_name,
                              top_source_mod_net,
                              top_source_mod_iterm);
    }

    // 3.4. Make dest hierarchy (bottom to top)
    // - lowest_common_module -> destination_pin
    // - Make input pins and nets intermediate hierarchies
    // - Goes up from source hierarchy to lowest common hierarchy
    dbModNet* top_dest_mod_net = nullptr;
    dbModITerm* top_dest_mod_iterm = nullptr;
    if (dest_db_module != lowest_common_module) {
      dlogHierConnCreatingDstHierarchy(db_network_->dbToSta(dest_pin),
                                       lowest_common_module);
      createHierarchyBottomUp(db_network_->dbToSta(dest_pin),
                              lowest_common_module,
                              dbIoType::INPUT,
                              connection_name,
                              top_dest_mod_net,
                              top_dest_mod_iterm);
    }

    // 3.5. Finally do the connection in the lowest common module
    if (top_dest_mod_iterm) {
      dlogHierConnConnectingInCommon(connection_name, lowest_common_module);

      // if we don't have a top net (case when we are connecting source at top
      // to hierarchically created pin), create one in the lowest module
      if (!top_source_mod_net) {
        dlogHierConnCreatingTopNet(connection_name, lowest_common_module);

        // Get base name of source_pin_flat_net
        Pin* sta_source_pin = db_network_->dbToSta(source_pin);
        dbNet* source_pin_flat_net = db_network_->flatNet(sta_source_pin);
        std::string base_name = fmt::format(
            "{}", db_network_->name(db_network_->dbToSta(source_pin_flat_net)));

        // Decide a new unique net name to avoid collisions in the lowest common
        // hierarchy
        std::string unique_name
            = makeUniqueName(lowest_common_module, base_name);

        // Create and connect dbModNet
        source_db_mod_net
            = dbModNet::create(lowest_common_module, unique_name.c_str());
        top_dest_mod_iterm->connect(source_db_mod_net);
        db_network_->disconnectPin(sta_source_pin);
        db_network_->connectPin(sta_source_pin,
                                (Net*) source_pin_flat_net,
                                (Net*) source_db_mod_net);
        top_source_mod_net = source_db_mod_net;
      } else {
        top_dest_mod_iterm->connect(top_source_mod_net);
      }
      dlogHierConnConnectingTopDstPin(top_dest_mod_iterm, top_source_mod_net);
    } else {
      dest_pin->connect(top_source_mod_net);
      dlogHierConnConnectingDstPin(dest_pin, top_source_mod_net);
    }

    // 3.6. What we are doing here is making sure that the
    // hierarchical nets at the source and the destination
    // are correctly associated. In the above code
    // we are wiring/unwiring modnets without regard to the
    // flat net association. We clean that up here.
    // Note we cannot reassociate until after we have built
    // the hiearchy tree
    Pin* sta_source_pin = db_network_->dbToSta(source_pin);
    Pin* sta_dest_pin = db_network_->dbToSta(dest_pin);
    db_network_->reassociatePinConnection(sta_source_pin);
    db_network_->reassociatePinConnection(sta_dest_pin);

    // 3.7. Rename flat net name if needed
    source_db_net->renameWithModNetInHighestHier();

    // 3.8. During the addition of new ports and new wiring we may
    // leave orphaned pins, clean them up.
    cleanUnusedHierPins(source_parent_tree, dest_parent_tree);
  }

  dlogHierConnDone();
}

void dbEditHierarchy::hierarchicalConnect(dbITerm* source_pin,
                                          dbModITerm* dest_pin,
                                          const char* connection_name)
{
  assert(source_pin != nullptr);
  assert(dest_pin != nullptr);
  connection_name = getBaseName(connection_name);
  dlogHierConnStart(
      source_pin, db_network_->dbToSta(dest_pin), connection_name);

  //
  // 1. Get the load pins through the dest_pin
  // - for each input dbITerm, check if the instance of the dbIterm belongs to
  // the dest_pin hierarchy.
  dbModNet* dest_mod_net = dest_pin->getModNet();
  dbModBTerm* dest_mod_bterm = dest_pin->getChildModBTerm();
  dbModNet* dest_mod_bterm_net = dest_mod_bterm->getModNet();

  std::set<dbITerm*> load_iterms;
  dbModInst* dest_mod_inst = dest_pin->getParent();
  for (dbITerm* iterm : dest_mod_bterm_net->getITerms()) {
    if (iterm == source_pin) {
      continue;  // Skip the source pin
    }

    dbInst* load_inst = iterm->getInst();
    if (dest_mod_inst->containsDbInst(load_inst)) {
      load_iterms.insert(iterm);
    }
  }

  //
  // 2. Reconnect flat net
  //

  // 2.1. Disconnect the leaf-level load pins of the dest_pin
  for (dbITerm* load_iterm : load_iterms) {
    load_iterm->disconnectDbNet();
  }

  // 2.2. Create a new flat net
  Net* new_flat_net_sta
      = db_network_->makeNet(connection_name,
                             db_network_->topInstance(),
                             odb::dbNameUniquifyType::IF_NEEDED);
  dbNet* new_flat_net = db_network_->staToDb(new_flat_net_sta);

  // 2.3. Connect: driver pin -> new flat net
  source_pin->connect(new_flat_net);

  // 2.4. Connect: New flat net -> leaf-level load pins
  for (dbITerm* load_iterm : load_iterms) {
    load_iterm->connect(new_flat_net);
  }

  //
  // 3. Reconnect hier net
  //

  // 3.1. Disconnect dest_pin from its old hier net.
  if (dest_mod_net) {
    dest_pin->disconnect();
  }

  // 3.2. Get the scope (dbModule*) of the source and destination pins
  dbModule* source_db_module = source_pin->getInst()->getModule();
  dbModule* dest_db_module = dest_pin->getParent()->getParent();

  std::vector<dbModule*> source_parent_tree;
  std::vector<dbModule*> dest_parent_tree;
  getParentHierarchy(source_db_module, source_parent_tree);
  getParentHierarchy(dest_db_module, dest_parent_tree);

  dbModule* lowest_common_module
      = findLowestCommonModule(source_parent_tree, dest_parent_tree);

  // 3.3. Make source hierarchy (bottom to top).
  dbModNet* top_source_mod_net = nullptr;
  dbModITerm* top_source_mod_iterm = nullptr;
  if (source_db_module != lowest_common_module) {
    dlogHierConnCreatingSrcHierarchy(source_pin, lowest_common_module);
    createHierarchyBottomUp(db_network_->dbToSta(source_pin),
                            lowest_common_module,
                            dbIoType::OUTPUT,
                            connection_name,
                            top_source_mod_net,
                            top_source_mod_iterm);
  } else {
    top_source_mod_net = source_pin->getModNet();
    if (!top_source_mod_net) {
      std::string unique_name
          = makeUniqueName(lowest_common_module, connection_name);
      top_source_mod_net
          = dbModNet::create(source_db_module, unique_name.c_str());
      source_pin->connect(top_source_mod_net);
    }
  }

  // 3.4. Make dest hierarchy (bottom to top)
  dbModNet* top_dest_mod_net = nullptr;
  dbModITerm* top_dest_mod_iterm = dest_pin;
  if (dest_db_module != lowest_common_module) {
    dlogHierConnCreatingDstHierarchy(db_network_->dbToSta(dest_pin),
                                     lowest_common_module);
    createHierarchyBottomUp(db_network_->dbToSta(dest_pin),
                            lowest_common_module,
                            dbIoType::INPUT,
                            connection_name,
                            top_dest_mod_net,
                            top_dest_mod_iterm);
  }

  // 3.5. Finally do the connection in the lowest common module
  top_dest_mod_iterm->connect(top_source_mod_net);

  // 3.6. Reassociate the pins to ensure consistency between flat and
  // hierarchical nets.
  Pin* sta_source_pin = db_network_->dbToSta(source_pin);
  Pin* sta_dest_pin = db_network_->dbToSta(dest_pin);
  db_network_->reassociatePinConnection(sta_source_pin);
  db_network_->reassociatePinConnection(sta_dest_pin);

  // 3.7. Rename flat net name if needed
  new_flat_net->renameWithModNetInHighestHier();

  // 3.8. During the addition of new ports and new wiring we may
  // leave orphaned pins, clean them up.
  cleanUnusedHierPins(source_parent_tree, dest_parent_tree);

  dlogHierConnDone();
}

void dbEditHierarchy::cleanUnusedHierPins(
    const std::vector<dbModule*>& source_parent_tree,
    const std::vector<dbModule*>& dest_parent_tree) const
{
  std::set<dbModInst*> cleaned_up;
  for (auto module_to_clean_up : source_parent_tree) {
    dbModInst* mi = module_to_clean_up->getModInst();
    if (mi) {
      dlogHierConnCleaningUpSrc(mi);
      mi->removeUnusedPortsAndPins();
      cleaned_up.insert(mi);
    }
  }

  for (auto module_to_clean_up : dest_parent_tree) {
    dbModInst* mi = module_to_clean_up->getModInst();
    if (mi) {
      if (cleaned_up.find(mi) == cleaned_up.end()) {
        dlogHierConnCleaningUpDst(mi);
        mi->removeUnusedPortsAndPins();
        cleaned_up.insert(mi);
      }
    }
  }
}

std::string dbEditHierarchy::makeUniqueName(dbModule* module,
                                            std::string_view name,
                                            const char* io_type_str) const
{
  std::string base_name;
  if (io_type_str) {
    base_name = fmt::format("{}_{}", name, io_type_str);
  } else {
    base_name = name;
  }

  std::string unique_name = base_name;
  int id = 0;
  while (module->findModBTerm(unique_name.c_str())
         || module->getModNet(unique_name.c_str())) {
    id++;
    unique_name = fmt::format("{}_{}", base_name, id);
  }
  return unique_name;
}

const char* dbEditHierarchy::getBaseName(const char* connection_name) const
{
  return db_network_->block()->getBaseName(connection_name);
}

void dbEditHierarchy::dlogHierConnStart(dbITerm* source_pin,
                                        Pin* dest_pin,
                                        const char* connection_name) const
{
  debugPrint(logger_,
             utl::ORD,
             "hierarchicalConnect",
             1,
             "1. Start: {}/{} -> {} (net: {})",
             source_pin->getInst()->getName(),
             source_pin->getMTerm()->getName(),
             db_network_->pathName(dest_pin),
             connection_name);
}

void dbEditHierarchy::dlogHierConnCreateFlatNet(
    const std::string& flat_name) const
{
  debugPrint(logger_,
             utl::ORD,
             "hierarchicalConnect",
             2,
             "2.1. Creating flat net {}",
             flat_name);
}

void dbEditHierarchy::dlogHierConnConnectSrcToFlatNet(
    dbITerm* source_pin,
    const std::string& flat_name) const
{
  debugPrint(logger_,
             utl::ORD,
             "hierarchicalConnect",
             2,
             "2.2. Connecting src pin {}/{} to flat net {}",
             source_pin->getInst()->getName(),
             source_pin->getMTerm()->getName(),
             flat_name);
}

void dbEditHierarchy::dlogHierConnConnectDstToFlatNet(
    dbITerm* dest_pin,
    dbNet* source_db_net) const
{
  debugPrint(logger_,
             utl::ORD,
             "hierarchicalConnect",
             2,
             "2.3. Connecting dst pin {}/{} to flat net {}",
             dest_pin->getInst()->getName(),
             dest_pin->getMTerm()->getName(),
             source_db_net->getName());
}

void dbEditHierarchy::dlogHierConnReusingConnection(
    dbModule* dest_db_module,
    dbModNet* dest_mod_net) const
{
  debugPrint(logger_,
             utl::ORD,
             "hierarchicalConnect",
             2,
             "3.1. Reusing existing connection to module {} through net {}",
             dest_db_module->getHierarchicalName(),
             dest_mod_net->getName());
}

void dbEditHierarchy::dlogHierConnCreatingSrcHierarchy(
    dbITerm* source_pin,
    dbModule* lowest_common_module) const
{
  debugPrint(logger_,
             utl::ORD,
             "hierarchicalConnect",
             3,
             "3.2. Creating hierarchy: src '{}/{}' -> common module '{}':",
             source_pin->getInst()->getName(),
             source_pin->getMTerm()->getName(),
             lowest_common_module->getHierarchicalName());
}

void dbEditHierarchy::dlogHierConnCreatingDstHierarchy(
    Pin* dest_pin,
    dbModule* lowest_common_module) const
{
  debugPrint(logger_,
             utl::ORD,
             "hierarchicalConnect",
             3,
             "3.3. Creating hierarchy: common module '{}' -> dst '{}':",
             lowest_common_module->getHierarchicalName(),
             db_network_->pathName(dest_pin));
}

void dbEditHierarchy::dlogHierConnConnectingInCommon(
    const char* connection_name,
    dbModule* lowest_common_module) const
{
  debugPrint(logger_,
             utl::ORD,
             "hierarchicalConnect",
             2,
             "3.4. Connecting net '{}' in common module '{}'",
             connection_name,
             lowest_common_module->getHierarchicalName());
}

void dbEditHierarchy::dlogHierConnCreatingTopNet(
    const char* connection_name,
    dbModule* lowest_common_module) const
{
  debugPrint(logger_,
             utl::ORD,
             "hierarchicalConnect",
             2,
             "  Creating top net '{}' in common module '{}'",
             connection_name,
             lowest_common_module->getHierarchicalName());
}

void dbEditHierarchy::dlogHierConnConnectingTopDstPin(dbModITerm* top_mod_dest,
                                                      dbModNet* net) const
{
  debugPrint(logger_,
             utl::ORD,
             "hierarchicalConnect",
             2,
             "  Connecting top dest pin '{}/{}' to top net '{}'",
             top_mod_dest->getParent()->getHierarchicalName(),
             top_mod_dest->getName(),
             net->getName());
}

void dbEditHierarchy::dlogHierConnConnectingDstPin(dbITerm* dest_pin,
                                                   dbModNet* top_net) const
{
  debugPrint(logger_,
             utl::ORD,
             "hierarchicalConnect",
             2,
             "  Connecting dest pin '{}/{}' to top net '{}'",
             dest_pin->getInst()->getName(),
             dest_pin->getMTerm()->getName(),
             top_net->getName());
}

void dbEditHierarchy::dlogHierConnReassociatingDstPin(
    dbNet* dest_pin_flat_net,
    dbModNet* dest_pin_mod_net) const
{
  debugPrint(
      logger_,
      utl::ORD,
      "hierarchicalConnect",
      2,
      "3.5. Re-associating dest pin with flat net '{}' and hier net '{}'",
      dest_pin_flat_net ? dest_pin_flat_net->getName() : "null",
      dest_pin_mod_net->getName());
}

void dbEditHierarchy::dlogHierConnReassociatingSrcPin(
    dbNet* source_pin_flat_net,
    dbModNet* source_pin_mod_net) const
{
  debugPrint(
      logger_,
      utl::ORD,
      "hierarchicalConnect",
      2,
      "3.5. Re-associating source pin with flat net '{}' and hier net '{}'",
      source_pin_flat_net ? source_pin_flat_net->getName() : "null",
      source_pin_mod_net->getName());
}

void dbEditHierarchy::dlogHierConnCleaningUpSrc(dbModInst* mi) const
{
  debugPrint(logger_,
             utl::ORD,
             "hierarchicalConnect",
             2,
             "3.6. Cleaning up unused ports/pins on source-side ModInst '{}'",
             mi->getHierarchicalName());
}

void dbEditHierarchy::dlogHierConnCleaningUpDst(dbModInst* mi) const
{
  debugPrint(logger_,
             utl::ORD,
             "hierarchicalConnect",
             2,
             "3.6. Cleaning up unused ports/pins on destination-side "
             "ModInst {}",
             mi->getHierarchicalName());
}

void dbEditHierarchy::dlogHierConnDone() const
{
  debugPrint(logger_,
             utl::ORD,
             "hierarchicalConnect",
             1,
             "Done ------------------------------------------");
}

void dbEditHierarchy::dlogCreateHierBTermAndModNet(
    int level,
    dbModule* cur_module,
    const std::string& new_term_net_name_i) const
{
  debugPrint(logger_,
             utl::ORD,
             "hierarchicalConnect",
             3,
             "  Hier(lv {}, {}): Creating BTerm '{}' & ModNet '{}'",
             level,
             cur_module->getHierarchicalName(),
             new_term_net_name_i,
             new_term_net_name_i);
}

void dbEditHierarchy::dlogCreateHierDisconnectingPin(
    int level,
    dbModule* cur_module,
    Pin* pin,
    dbModNet* pin_mod_net) const
{
  debugPrint(logger_,
             utl::ORD,
             "hierarchicalConnect",
             4,
             "  Hier(lv {}, {}): pin '{}' is already connected to "
             "'{}', disconnecting.",
             level,
             cur_module->getHierarchicalName(),
             db_network_->pathName(pin),
             pin_mod_net->getName());
}

void dbEditHierarchy::dlogCreateHierConnectingPin(int level,
                                                  dbModule* cur_module,
                                                  Pin* pin,
                                                  dbModNet* db_mod_net) const
{
  debugPrint(logger_,
             utl::ORD,
             "hierarchicalConnect",
             4,
             "  Hier(lv {}, {}): Connecting pin '{}' to ModNet '{}'",
             level,
             cur_module->getHierarchicalName(),
             db_network_->pathName(pin),
             db_mod_net->getName());
}

void dbEditHierarchy::dlogCreateHierCreatingITerm(
    int level,
    dbModule* cur_module,
    dbModInst* parent_inst,
    const std::string& new_term_net_name_i) const
{
  debugPrint(logger_,
             utl::ORD,
             "hierarchicalConnect",
             3,
             "  Hier(lv {}, {}): Creating ITerm '{}/{}'",
             level,
             cur_module->getHierarchicalName(),
             parent_inst->getHierarchicalName(),
             new_term_net_name_i);
}

void dbEditHierarchy::dlogCreateHierConnectingITerm(
    int level,
    dbModule* cur_module,
    dbModInst* parent_inst,
    const std::string& new_term_net_name_i,
    dbModNet* db_mod_net) const
{
  debugPrint(logger_,
             utl::ORD,
             "hierarchicalConnect",
             3,
             "  Hier(lv {}, {}): Connecting ITerm '{}/{}' to ModNet '{}'",
             level,
             cur_module->getHierarchicalName(),
             parent_inst->getHierarchicalName(),
             new_term_net_name_i,
             db_mod_net->getName());
}

}  // namespace sta
