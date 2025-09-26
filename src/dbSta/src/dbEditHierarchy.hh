// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

// dbEditHierarchy, Manipulating hierarchical relationships in OpenDB

#pragma once

#include "odb/db.h"

namespace utl {
class Logger;
}

namespace sta {

using utl::Logger;

using odb::dbIoType;
using odb::dbITerm;
using odb::dbModBTerm;
using odb::dbModInst;
using odb::dbModITerm;
using odb::dbModNet;
using odb::dbModule;
using odb::dbNet;

class dbNetwork;
class Pin;

// This class manipulates hierarchical relationships with pins, nets, and
// instances.
class dbEditHierarchy
{
 public:
  dbEditHierarchy(dbNetwork* db_network, Logger* logger)
      : db_network_(db_network), logger_(logger){};
  ~dbEditHierarchy() = default;
  dbEditHierarchy(const dbEditHierarchy&) = delete;
  dbEditHierarchy& operator=(const dbEditHierarchy&) = delete;

  void setLogger(Logger* logger) { logger_ = logger; }
  void getParentHierarchy(dbModule* start_module,
                          std::vector<dbModule*>& parent_hierarchy) const;
  dbModule* findLowestCommonModule(std::vector<dbModule*>& itree1,
                                   std::vector<dbModule*>& itree2) const;
  bool connectionToModuleExists(dbITerm* source_pin,
                                dbModule* dest_module,
                                dbModBTerm*& dest_modbterm,
                                dbModITerm*& dest_moditerm) const;
  void hierarchicalConnect(dbITerm* source_pin,
                           dbITerm* dest_pin,
                           const char* connection_name = "net");
  void hierarchicalConnect(dbITerm* source_pin,
                           dbModITerm* dest_pin,
                           const char* connection_name = "net");

 private:
  void createHierarchyBottomUp(Pin* pin,
                               dbModule* lowest_common_module,
                               const dbIoType& io_type,
                               const char* connection_name,
                               dbModNet*& top_mod_net,
                               dbModITerm*& top_mod_iterm) const;
  void reassociatePinConnection(Pin* pin);
  std::string makeUniqueName(dbModule* module,
                             std::string name,
                             const char* io_type_str = nullptr) const;

  // During the addition of new ports and new wiring we may
  // leave orphaned pins, clean them up.
  void cleanUnusedHierPins(
      const std::vector<dbModule*>& source_parent_tree,
      const std::vector<dbModule*>& dest_parent_tree) const;

  const char* getBaseName(const char* connection_name) const;

  // Debug log methods
  void dlogHierConnStart(dbITerm* source_pin,
                         Pin* dest_pin,
                         const char* connection_name) const;
  void dlogHierConnCreateFlatNet(const std::string& flat_name) const;
  void dlogHierConnConnectSrcToFlatNet(dbITerm* source_pin,
                                       const std::string& flat_name) const;
  void dlogHierConnConnectDstToFlatNet(dbITerm* dest_pin,
                                       dbNet* source_db_net) const;
  void dlogHierConnReusingConnection(dbModule* dest_db_module,
                                     dbModNet* dest_mod_net) const;
  void dlogHierConnCreatingSrcHierarchy(dbITerm* source_pin,
                                        dbModule* lowest_common_module) const;
  void dlogHierConnCreatingDstHierarchy(Pin* dest_pin,
                                        dbModule* lowest_common_module) const;
  void dlogHierConnConnectingInCommon(const char* connection_name,
                                      dbModule* lowest_common_module) const;
  void dlogHierConnCreatingTopNet(const char* connection_name,
                                  dbModule* lowest_common_module) const;
  void dlogHierConnConnectingTopDstPin(dbModITerm* top_mod_dest,
                                       dbModNet* net) const;
  void dlogHierConnConnectingDstPin(dbITerm* dest_pin, dbModNet* top_net) const;
  void dlogHierConnReassociatingDstPin(dbNet* dest_pin_flat_net,
                                       dbModNet* dest_pin_mod_net) const;
  void dlogHierConnReassociatingSrcPin(dbNet* source_pin_flat_net,
                                       dbModNet* source_pin_mod_net) const;
  void dlogHierConnCleaningUpSrc(dbModInst* mi) const;
  void dlogHierConnCleaningUpDst(dbModInst* mi) const;
  void dlogHierConnDone() const;
  void dlogCreateHierBTermAndModNet(
      int level,
      dbModule* cur_module,
      const std::string& new_term_net_name_i) const;
  void dlogCreateHierDisconnectingPin(int level,
                                      dbModule* cur_module,
                                      Pin* pin,
                                      dbModNet* pin_mod_net) const;
  void dlogCreateHierConnectingPin(int level,
                                   dbModule* cur_module,
                                   Pin* pin,
                                   dbModNet* db_mod_net) const;
  void dlogCreateHierCreatingITerm(
      int level,
      dbModule* cur_module,
      dbModInst* parent_inst,
      const std::string& new_term_net_name_i) const;
  void dlogCreateHierConnectingITerm(int level,
                                     dbModule* cur_module,
                                     dbModInst* parent_inst,
                                     const std::string& new_term_net_name_i,
                                     dbModNet* db_mod_net) const;

 private:
  dbNetwork* db_network_ = nullptr;
  Logger* logger_ = nullptr;
};

}  // namespace sta