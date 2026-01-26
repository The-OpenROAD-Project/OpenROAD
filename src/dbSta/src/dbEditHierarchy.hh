// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

// dbEditHierarchy, Manipulating hierarchical relationships in OpenDB

#pragma once

#include <string>
#include <string_view>
#include <vector>

#include "odb/db.h"
#include "odb/dbTypes.h"
#include "utl/Logger.h"

namespace sta {

class dbNetwork;
class Pin;

// This class manipulates hierarchical relationships with pins, nets, and
// instances.
class dbEditHierarchy
{
 public:
  dbEditHierarchy(dbNetwork* db_network, utl::Logger* logger)
      : db_network_(db_network), logger_(logger)
  {
  }
  ~dbEditHierarchy() = default;
  dbEditHierarchy(const dbEditHierarchy&) = delete;
  dbEditHierarchy& operator=(const dbEditHierarchy&) = delete;

  void setLogger(utl::Logger* logger) { logger_ = logger; }
  void getParentHierarchy(odb::dbModule* start_module,
                          std::vector<odb::dbModule*>& parent_hierarchy) const;
  odb::dbModule* findLowestCommonModule(
      std::vector<odb::dbModule*>& itree1,
      std::vector<odb::dbModule*>& itree2) const;
  bool connectionToModuleExists(odb::dbITerm* source_pin,
                                odb::dbModule* dest_module,
                                odb::dbModBTerm*& dest_modbterm,
                                odb::dbModITerm*& dest_moditerm) const;
  void hierarchicalConnect(odb::dbITerm* source_pin,
                           odb::dbITerm* dest_pin,
                           const char* connection_name = "net");
  void hierarchicalConnect(odb::dbITerm* source_pin,
                           odb::dbModITerm* dest_pin,
                           const char* connection_name = "net");

 private:
  void createHierarchyBottomUp(Pin* pin,
                               odb::dbModule* lowest_common_module,
                               const odb::dbIoType& io_type,
                               const char* connection_name,
                               odb::dbModNet*& top_mod_net,
                               odb::dbModITerm*& top_mod_iterm) const;
  void reassociatePinConnection(Pin* pin);
  std::string makeUniqueName(odb::dbModule* module,
                             std::string_view name,
                             const char* io_type_str = nullptr) const;

  // During the addition of new ports and new wiring we may
  // leave orphaned pins, clean them up.
  void cleanUnusedHierPins(
      const std::vector<odb::dbModule*>& source_parent_tree,
      const std::vector<odb::dbModule*>& dest_parent_tree) const;

  const char* getBaseName(const char* connection_name) const;

  // Debug log methods
  void dlogHierConnStart(odb::dbITerm* source_pin,
                         Pin* dest_pin,
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
      odb::dbModule* lowest_common_module) const;
  void dlogHierConnCreatingDstHierarchy(
      Pin* dest_pin,
      odb::dbModule* lowest_common_module) const;
  void dlogHierConnConnectingInCommon(
      const char* connection_name,
      odb::dbModule* lowest_common_module) const;
  void dlogHierConnCreatingTopNet(const char* connection_name,
                                  odb::dbModule* lowest_common_module) const;
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
                                      Pin* pin,
                                      odb::dbModNet* pin_mod_net) const;
  void dlogCreateHierConnectingPin(int level,
                                   odb::dbModule* cur_module,
                                   Pin* pin,
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
  utl::Logger* logger_ = nullptr;
};

}  // namespace sta
