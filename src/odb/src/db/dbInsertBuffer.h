// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include <atomic>
#include <set>
#include <string>

#include "dbNet.h"
#include "odb/db.h"
#include "odb/dbObject.h"
#include "odb/dbTypes.h"
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
                                 const Point* loc = nullptr,
                                 const char* new_buf_base_name = nullptr,
                                 const char* new_net_base_name = nullptr,
                                 const dbNameUniquifyType& uniquify
                                 = dbNameUniquifyType::ALWAYS);
  dbInst* insertBufferAfterDriver(dbObject* drvr_output_term,
                                  const dbMaster* buffer_master,
                                  const Point* loc = nullptr,
                                  const char* new_buf_base_name = nullptr,
                                  const char* new_net_base_name = nullptr,
                                  const dbNameUniquifyType& uniquify
                                  = dbNameUniquifyType::ALWAYS);
  dbInst* insertBufferBeforeLoads(std::set<dbObject*>& load_pins,
                                  const dbMaster* buffer_master,
                                  const Point* loc = nullptr,
                                  const char* new_buf_base_name = nullptr,
                                  const char* new_net_base_name = nullptr,
                                  const dbNameUniquifyType& uniquify
                                  = dbNameUniquifyType::ALWAYS,
                                  bool loads_on_diff_nets = false);
  // Establish hierarchical connections (dbModNet) between a driver and a load.
  // - It creates modnet and flat net connections through multiple hierarchies.
  // - Ensures that the flat net name matches to any of corresponding modnet
  //   names. If needed, flat net name can be renamed to the modnet name
  //   connected to the driver pin.
  void hierarchicalConnect(dbObject* driver, dbObject* load);

 private:
  void resetMembers();
  dbInst* insertBufferSimple(dbObject* term_obj,
                             const dbMaster* buffer_master,
                             const Point* loc,
                             const char* new_buf_base_name,
                             const char* new_net_base_name,
                             const dbNameUniquifyType& uniquify,
                             bool insertBefore);
  bool validateTermAndGetModNet(dbObject* term_obj, dbModNet*& mod_net) const;
  dbInst* checkAndCreateBuffer();
  bool checkDontTouch(dbITerm* iterm) const;
  void placeNewBuffer(dbInst* buffer_inst, const Point* loc, dbObject* term);
  void rewireBufferSimple(bool insertBefore,
                          dbModNet* orig_mod_net,
                          dbObject* term);
  bool getPinLocation(dbObject* pin, int& x, int& y) const;
  Point computeCentroid(dbObject* drvr_pin,
                        const std::set<dbObject*>& load_pins) const;
  dbNet* createNewFlatNet(std::set<dbObject*>& connected_terms);
  std::string makeUniqueHierName(dbModule* module,
                                 const std::string& base_name,
                                 const char* suffix = nullptr) const;
  int getModuleDepth(dbModule* mod) const;
  dbModule* findLCA(dbModule* m1, dbModule* m2) const;
  bool stitchLoadToDriver(dbITerm* load_pin,
                          dbITerm* drvr_term,
                          const std::set<dbObject*>& load_pins);
  dbModNet* getFirstModNetInFaninOfLoads(
      const std::set<dbObject*>& load_pins,
      const std::set<dbModNet*>& modnets_in_target_module) const;
  dbModNet* getFirstDriverModNetInTargetModule(
      const std::set<dbModNet*>& modnets_in_target_module) const;
  bool checkAllLoadsAreTargets(dbModNet* net,
                               const std::set<dbObject*>& load_pins);
  void populateReusableNets(const std::set<dbObject*>& load_pins);

  //------------------------------------------------------------------
  // Helper functions for hierarchicalConnect
  //------------------------------------------------------------------
  dbModNet* getModNet(dbObject* obj) const;
  dbNet* getNet(dbObject* obj) const;
  dbModule* getModule(dbObject* obj) const;
  std::string getName(dbObject* obj) const;
  void connect(dbObject* obj, dbNet* net);
  void connect(dbObject* obj, dbModNet* net);
  void ensureFlatNetConnection(dbObject* driver, dbObject* load);
  void connectSameModule(dbObject* driver,
                         dbObject* load,
                         dbModule* driver_mod);
  dbModNet* ensureModNet(dbObject* obj,
                         dbModule* mod,
                         const char* suffix = nullptr);
  dbObject* traceUp(dbObject* current_obj,
                    dbModule* current_mod,
                    dbModule* target_mod,
                    dbIoType io_type,
                    const char* suffix);
  void connectDifferentModule(dbObject* driver,
                              dbObject* load,
                              dbModule* driver_mod,
                              dbModule* load_mod);

  //------------------------------------------------------------------
  // Helper functions for stitchLoadToDriver
  //------------------------------------------------------------------
  bool tryReuseParentPath(dbObject*& load_obj,
                          dbModule*& current_module,
                          dbModITerm*& top_mod_iterm,
                          const std::set<dbObject*>& load_pins);
  bool tryReuseModNetInModule(dbObject*& load_obj,
                              dbModule*& current_module,
                              dbModITerm*& top_mod_iterm);
  void createNewHierConnection(dbObject*& load_obj,
                               dbModule*& current_module,
                               dbModITerm*& top_mod_iterm,
                               const std::string& base_name);
  void advanceToParentModule(dbObject*& load_obj,
                             dbModule*& current_module,
                             dbModITerm*& top_mod_iterm,
                             dbModITerm* next_mod_iterm);
  void performFinalConnections(dbITerm* load_pin,
                               dbITerm* drvr_term,
                               dbModITerm* top_mod_iterm);

  //------------------------------------------------------------------
  // Helper functions for insertBufferBeforeLoads
  //------------------------------------------------------------------
  dbModule* validateLoadPinsAndFindLCA(std::set<dbObject*>& load_pins,
                                       std::set<dbNet*>& other_dbnets,
                                       bool loads_on_diff_nets) const;
  void createNewFlatAndHierNets(std::set<dbObject*>& load_pins);
  void rewireBufferLoadPins(std::set<dbObject*>& load_pins);
  void placeBufferAtLocation(dbInst* buffer_inst,
                             const Point* loc,
                             std::set<dbObject*>& load_pins);
  void setBufferAttributes(dbInst* buffer_inst);

 private:
  //------------------------------------------------------------------
  // Debug logging functions
  //------------------------------------------------------------------
  void dlogBeforeLoadsParams(const std::set<dbObject*>& load_pins,
                             const Point* loc,
                             bool loads_on_diff_nets) const;
  void dlogTargetLoadPin(dbObject* load_obj) const;
  void dlogDifferentDbNet(const std::string& net_name) const;
  void dlogLCAModule(dbModule* target_module) const;
  void dlogDumpNets(const std::set<dbNet*>& other_dbnets) const;
  void dlogCreatingNewHierNet(const char* base_name) const;
  void dlogConnectedBufferInputFlat() const;
  void dlogConnectedBufferInputHier(dbModNet* orig_mod_net) const;
  void dlogConnectedBufferInputViaHierConnect(dbObject* drvr_term) const;
  void dlogConnectedBufferOutput() const;
  void dlogCreatingHierConn(int load_idx, int num_loads, dbITerm* load) const;
  void dlogMovedBTermLoad(int load_idx, int num_loads, dbBTerm* load) const;
  void dlogPlacingBuffer(dbInst* buffer_inst, const Point& loc) const;
  void dlogInsertBufferSuccess(dbInst* buffer_inst) const;
  void dlogInsertBufferStart(int count, const char* mode) const;
  void dlogSeparator() const;

 private:
  dbNet* net_{nullptr};  // Target net. The net to which the buffer is inserted
  dbBlock* block_{nullptr};
  utl::Logger* logger_{nullptr};

  // Insert buffer state
  dbITerm* buf_input_iterm_{nullptr};   // Input iterm of the new buffer
  dbITerm* buf_output_iterm_{nullptr};  // Output iterm of the new buffer
  dbNet* new_flat_net_{nullptr};    // New flat net connected to the new buffer
  dbModNet* new_mod_net_{nullptr};  // New modnet connected to the new buffer
  dbModule* target_module_{nullptr};        // Target module for the new buffer
  const dbMaster* buffer_master_{nullptr};  // Buffer master
  const char* new_buf_base_name_{nullptr};  // Base name for the new buffer
  const char* new_net_base_name_{nullptr};  // Base name for the new nets
  dbNameUniquifyType uniquify_{dbNameUniquifyType::ALWAYS};
  dbObject* orig_drvr_pin_{nullptr};  // Original driver pin of net_

  // Caches for hierarchical connection

  // dbModNet has target loads only (true) or
  // mixed target and non-target loads (false)
  std::map<dbModNet*, bool> is_target_only_cache_;

  // Indicates a dbModNet in a module can be reused (w/o port punching) or not
  std::map<dbModule*, std::set<dbModNet*>> module_reusable_nets_;

  //------------------------------------------------------------------
  // Static attribute
  //------------------------------------------------------------------
  inline static std::atomic<int> insert_buffer_call_count_{0};
};

}  // namespace odb
