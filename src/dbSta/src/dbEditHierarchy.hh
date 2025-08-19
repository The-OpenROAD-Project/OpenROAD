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

using odb::dbBlock;
using odb::dbBTerm;
using odb::dbDatabase;
using odb::dbInst;
using odb::dbIoType;
using odb::dbITerm;
using odb::dbLib;
using odb::dbMaster;
using odb::dbModBTerm;
using odb::dbModInst;
using odb::dbModITerm;
using odb::dbModNet;
using odb::dbModule;
using odb::dbMTerm;
using odb::dbNet;
using odb::dbObject;
using odb::dbObjectType;
using odb::dbSet;
using odb::dbSigType;
using odb::Point;

class dbNetwork;

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
  dbModule* findHighestCommonModule(std::vector<dbModule*>& itree1,
                                    std::vector<dbModule*>& itree2) const;
  bool ConnectionToModuleExists(dbITerm* source_pin,
                                dbModule* dest_module,
                                dbModBTerm*& dest_modbterm,
                                dbModITerm*& dest_moditerm) const;
  void hierarchicalConnect(dbITerm* source_pin,
                           dbITerm* dest_pin,
                           const char* connection_name = "net") const;

 private:
  void createHierarchyBottomUp(dbITerm* pin,
                               dbModule* highest_common_module,
                               const dbIoType& io_type,
                               const char* connection_name,
                               dbModNet*& top_mod_net,
                               dbModITerm*& top_mod_iterm) const;
  // Debug log methods
  void dlogHierConnStart(odb::dbITerm* source_pin,
                         odb::dbITerm* dest_pin,
                         const char* connection_name) const;
  void dlogHierConnCreateFlatNet(const std::string& flat_name) const;
  void dlogHierConnConnectSrcToFlatNet(odb::dbITerm* source_pin,
                                       const std::string& flat_name) const;
  void dlogHierConnConnectDstToFlatNet(odb::dbITerm* dest_pin,
                                       odb::dbNet* source_db_net) const;
  void dlogHierConnReusingConnection(odb::dbModule* dest_db_module,
                                     odb::dbModNet* dest_mod_net) const;
  void dlogHierConnCreatingSrcHierarchy(
      odb::dbITerm* source_pin,
      odb::dbModule* highest_common_module) const;
  void dlogHierConnCreatingDstHierarchy(
      odb::dbITerm* dest_pin,
      odb::dbModule* highest_common_module) const;
  void dlogHierConnConnectingInCommon(
      const char* connection_name,
      odb::dbModule* highest_common_module) const;
  void dlogHierConnCreatingTopNet(const char* connection_name,
                                  odb::dbModule* highest_common_module) const;
  void dlogHierConnConnectingTopDstPin(odb::dbModITerm* top_mod_dest,
                                       odb::dbModNet* net) const;
  void dlogHierConnConnectingDstPin(odb::dbITerm* dest_pin,
                                    odb::dbModNet* top_net) const;
  void dlogHierConnReassociatingDstPin(odb::dbNet* dest_pin_flat_net,
                                       odb::dbModNet* dest_pin_mod_net) const;
  void dlogHierConnReassociatingSrcPin(odb::dbNet* source_pin_flat_net,
                                       odb::dbModNet* source_pin_mod_net) const;
  void dlogHierConnCleaningUpSrc(odb::dbModInst* mi) const;
  void dlogHierConnCleaningUpDst(odb::dbModInst* mi) const;
  void dlogHierConnDone() const;
  void dlogCreateHierBTermAndModNet(
      int level,
      odb::dbModule* cur_module,
      const std::string& new_term_net_name_i) const;
  void dlogCreateHierDisconnectingPin(int level,
                                      odb::dbModule* cur_module,
                                      odb::dbITerm* pin,
                                      odb::dbModNet* pin_mod_net) const;
  void dlogCreateHierConnectingPin(int level,
                                   odb::dbModule* cur_module,
                                   odb::dbITerm* pin,
                                   odb::dbModNet* db_mod_net) const;
  void dlogCreateHierCreatingITerm(
      int level,
      odb::dbModule* cur_module,
      odb::dbModInst* parent_inst,
      const std::string& new_term_net_name_i) const;
  void dlogCreateHierConnectingITerm(int level,
                                     odb::dbModule* cur_module,
                                     odb::dbModInst* parent_inst,
                                     const std::string& new_term_net_name_i,
                                     odb::dbModNet* db_mod_net) const;

 private:
  dbNetwork* db_network_ = nullptr;
  Logger* logger_ = nullptr;
};

}  // namespace sta