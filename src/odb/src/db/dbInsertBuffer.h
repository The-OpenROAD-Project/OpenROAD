// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include <set>
#include <string>

#include "dbNet.h"
#include "odb/db.h"
#include "odb/geom.h"

namespace odb {

class dbBlock;
class dbInst;
class dbMaster;
class dbITerm;
class dbBTerm;
class dbModNet;
class dbModule;

class dbInsertBuffer
{
 public:
  dbInsertBuffer(dbNet* net);
  dbInst* insertBufferBeforeLoad(dbObject* load_input_term,
                                 const dbMaster* buffer_master,
                                 const Point* loc,
                                 const char* base_name,
                                 const dbNameUniquifyType& uniquify);
  dbInst* insertBufferAfterDriver(dbObject* drvr_output_term,
                                  const dbMaster* buffer_master,
                                  const Point* loc,
                                  const char* base_name,
                                  const dbNameUniquifyType& uniquify);
  dbInst* insertBufferBeforeLoads(std::set<dbObject*>& load_pins,
                                  const dbMaster* buffer_master,
                                  const Point* loc,
                                  const char* base_name,
                                  const dbNameUniquifyType& uniquify,
                                  bool loads_on_same_db_net);
  void hierarchicalConnect(dbITerm* driver, dbITerm* load);

 private:
  void resetMembers();
  dbInst* insertBufferCommon(dbObject* term_obj,
                             const dbMaster* buffer_master,
                             const Point* loc,
                             const char* base_name,
                             const dbNameUniquifyType& uniquify,
                             bool insertBefore);
  bool checkAndGetTerm(dbObject* term_obj,
                       dbITerm*& iterm,
                       dbBTerm*& bterm,
                       dbModNet*& mod_net);
  dbInst* checkAndCreateBuffer();
  bool checkDontTouch(dbITerm* drvr_iterm, dbITerm* load_iterm);
  void placeNewBuffer(dbInst* buffer_inst,
                      const Point* loc,
                      dbITerm* term,
                      dbBTerm* bterm);
  void rewireBufferSimple(bool insertBefore,
                          dbITerm* buf_input_iterm,
                          dbITerm* buf_output_iterm,
                          dbNet* orig_net,
                          dbModNet* orig_mod_net,
                          dbNet* new_net,
                          dbITerm* term_iterm,
                          dbBTerm* term_bterm);
  Point computeCentroid(const std::set<dbObject*>& pins);
  dbNet* createBufferNet(std::set<dbObject*>& connected_terms);
  std::string makeUniqueHierName(dbModule* module,
                                 const std::string& base_name,
                                 const char* suffix) const;
  int getModuleDepth(dbModule* mod) const;
  dbModule* findLCA(dbModule* m1, dbModule* m2) const;
  bool createHierarchicalConnection(dbITerm* load_pin,
                                    dbITerm* drvr_term,
                                    const std::set<dbObject*>& load_pins);
  dbModNet* getFirstModNetInFaninOfLoads(
      const std::set<dbObject*>& load_pins,
      const std::set<dbModNet*>& modnets_in_target_module);
  dbModNet* getFirstDriverModNetInTargetModule(
      const std::set<dbModNet*>& modnets_in_target_module);

  //------------------------------------------------------------------
  // Step functions for insertBufferBeforeLoads
  //------------------------------------------------------------------
  dbModule* validateLoadPinsAndFindLCA(std::set<dbObject*>& load_pins,
                                       std::set<dbNet*>& other_dbnets,
                                       bool loads_on_same_db_net);
  void createNewBufferNet(std::set<dbObject*>& load_pins);
  void rewireBufferLoadPins(std::set<dbObject*>& load_pins);
  void placeBufferAtLocation(dbInst* buffer_inst,
                             const Point* loc,
                             std::set<dbObject*>& load_pins);
  void setBufferAttributes(dbInst* buffer_inst);

 private:
  //------------------------------------------------------------------
  // Debug logging helper functions
  //------------------------------------------------------------------
  void dlogBeforeLoadsParams(const std::set<dbObject*>& load_pins,
                             const Point* loc,
                             bool loads_on_same_db_net) const;
  void dlogTargetLoadPin(dbObject* load_obj) const;
  void dlogDifferentDbNet(const std::string& net_name) const;
  void dlogLCAModule(dbModule* target_module) const;
  void dlogDumpNets(const std::set<dbNet*>& other_dbnets) const;
  void dlogCreatingNewHierNet(const char* base_name) const;
  void dlogConnectedBufferInputFlat() const;
  void dlogConnectedBufferInputHier(dbModNet* orig_mod_net) const;
  void dlogConnectedBufferInputViaHierConnect(dbITerm* drvr_iterm) const;
  void dlogConnectedBufferOutput() const;
  void dlogCreatingHierConn(int load_idx, int num_loads, dbITerm* load) const;
  void dlogMovedBTermLoad(int load_idx, int num_loads, dbBTerm* load) const;
  void dlogPlacingBuffer(dbInst* buffer_inst, const Point& loc) const;
  void dlogInsertBufferSuccess(dbInst* buffer_inst) const;
  void dlogSeparator() const;

 private:
  dbNet* net_ = nullptr;
  dbBlock* block_ = nullptr;
  utl::Logger* logger_ = nullptr;

  // Insert buffer state
  dbITerm* buf_input_iterm_ = nullptr;
  dbITerm* buf_output_iterm_ = nullptr;
  dbNet* new_flat_net_ = nullptr;
  dbModNet* new_mod_net_ = nullptr;
  dbModule* target_module_ = nullptr;
  const dbMaster* buffer_master_ = nullptr;
  const char* base_name_ = nullptr;
  dbNameUniquifyType uniquify_ = dbNameUniquifyType::ALWAYS;
};

}  // namespace odb
