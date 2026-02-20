// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include <atomic>
#include <map>
#include <optional>
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
  dbInsertBuffer() = delete;
  dbInsertBuffer(dbNet* net);
  dbInsertBuffer(const dbInsertBuffer&) = delete;
  dbInsertBuffer& operator=(const dbInsertBuffer&) = delete;
  dbInsertBuffer(dbInsertBuffer&&) = delete;
  dbInsertBuffer& operator=(dbInsertBuffer&&) = delete;
  ~dbInsertBuffer() = default;

  dbInst* insertBufferBeforeLoad(
      dbObject* load_input_term,
      const dbMaster* buffer_master,
      const Point* loc = nullptr,
      const char* new_buf_base_name = kDefaultBufBaseName,
      const char* new_net_base_name = kDefaultNetBaseName,
      const dbNameUniquifyType& uniquify = dbNameUniquifyType::ALWAYS);
  dbInst* insertBufferAfterDriver(
      dbObject* drvr_output_term,
      const dbMaster* buffer_master,
      const Point* loc = nullptr,
      const char* new_buf_base_name = kDefaultBufBaseName,
      const char* new_net_base_name = kDefaultNetBaseName,
      const dbNameUniquifyType& uniquify = dbNameUniquifyType::ALWAYS);
  dbInst* insertBufferBeforeLoads(
      const std::set<dbObject*>& load_pins,
      const dbMaster* buffer_master,
      const Point* loc = nullptr,
      const char* new_buf_base_name = kDefaultBufBaseName,
      const char* new_net_base_name = kDefaultNetBaseName,
      const dbNameUniquifyType& uniquify = dbNameUniquifyType::ALWAYS,
      bool loads_on_diff_nets = false);

  ///
  /// Establish hierarchical connections (dbModNet) between a driver and a load.
  /// - It creates modnet and flat net connections through multiple hierarchies.
  /// - Ensures that the flat net name matches to any of corresponding modnet
  ///   names. If needed, flat net name can be renamed to the modnet name
  ///   connected to the driver pin.
  ///
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
  dbInst* checkAndCreateBuffer();
  bool checkDontTouch(const dbITerm* iterm) const;
  dbNet* createNewFlatNet(const std::set<dbObject*>& connected_terms);
  std::string makeUniqueHierName(const dbModule* module,
                                 const std::string& base_name,
                                 const char* suffix = nullptr) const;
  int getModuleDepth(const dbModule* mod) const;
  dbModule* findLCA(dbModule* m1, dbModule* m2) const;
  dbModNet* getFirstDriverModNetInTargetModule(
      const std::set<dbModNet*>& modnets_in_target_module) const;
  bool checkAllLoadsAreTargets(dbModNet* net,
                               const std::set<dbObject*>& load_pins) const;
  bool isMarkedAsNotReusable(dbModNet* net) const;
  std::optional<bool> getCachedReusability(dbModNet* net) const;
  void markModNetReusability(dbModNet* net, bool is_reusable) const;
  bool getPinLocation(const dbObject* pin, int& x, int& y) const;
  bool computeCentroid(const dbObject* drvr_pin,
                       const std::set<dbObject*>& load_pins,
                       Point& result) const;
  void placeBufferAtLocation(dbInst* buffer_inst,
                             const Point& loc,
                             const char* reason = "argument");
  void placeBufferAtPin(dbInst* buffer_inst, const dbObject* term);
  void placeBufferAtCentroid(dbInst* buffer_inst,
                             const dbObject* drvr_pin,
                             const std::set<dbObject*>& load_pins);

  ///
  /// This function identifies hierarchical nets (dbModNet) that can be reused
  /// during buffer insertion. Once a modnet is considered resuable, no new port
  /// punching and new modnet creation is required.
  /// A modnet is considered reusable if all of its loads are also target load
  /// pins for the buffer being inserted.
  ///
  void populateReusableModNets(const std::set<dbObject*>& load_pins);

  ///
  /// Traverses the netlist upstream (fanin cone) from the given net and marks
  /// all visited dbModNets as 'false' in is_target_only_cache_. This prevents
  /// reusing any net that is part of the buffer's fanin, avoiding loops.
  ///
  void markFaninModNetsNotReusable(dbModNet* net);
  void checkSanity() const;

  //------------------------------------------------------------------
  // Helper functions for hierarchicalConnect
  //------------------------------------------------------------------
  dbModNet* getModNet(const dbObject* obj) const;
  dbNet* getNet(const dbObject* obj) const;
  dbModule* getModule(const dbObject* obj) const;
  std::string getName(const dbObject* obj) const;
  void connect(dbObject* obj, dbNet* net);
  void connect(dbObject* obj, dbModNet* net);
  void ensureFlatNetConnection(dbObject* driver, dbObject* load);
  void connectSameModule(dbObject* driver,
                         dbObject* load,
                         dbModule* driver_mod);
  dbModNet* ensureModNet(dbObject* obj,
                         dbModule* mod,
                         dbNet* corresponding_flat_net = nullptr,
                         const char* suffix = nullptr);

  void connectPeerITerms(dbModule* mod,
                         dbModNet* mod_net,
                         dbNet* corresponding_flat_net);

  ///
  /// Trace up the module hierarchy from current_mod to target_mod,
  /// creating hierarchical ports (dbModBTerm/dbModITerm) and modNets as needed.
  /// Returns the dbObject (dbModITerm) at target_mod level.
  /// - current_obj: Starting point (dbITerm, dbBTerm, or dbModITerm)
  /// - io_type: Port direction for created hierarchical ports
  /// - suffix: Optional suffix for port names
  /// - corresponding_flat_net: The flat net to use for name collision avoidance
  ///
  dbObject* traceUp(dbObject* current_obj,
                    dbModule* current_mod,
                    dbModule* target_mod,
                    dbIoType io_type,
                    const char* suffix,
                    dbNet* corresponding_flat_net = nullptr);
  void connectDifferentModule(dbObject* driver,
                              dbObject* load,
                              dbModule* driver_mod,
                              dbModule* load_mod);
  bool stitchLoadToDriver(dbITerm* load_pin,
                          dbITerm* drvr_term,
                          const std::set<dbObject*>& load_pins);

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
  // Helper functions for insertBufferSimple
  //------------------------------------------------------------------
  void validateArgumentsSimple(const dbObject* term_obj,
                               const dbMaster* buffer_master) const;
  bool validateTermAndGetModNet(const dbObject* term_obj,
                                dbModNet*& mod_net) const;
  void rewireBufferSimple(bool insertBefore,
                          dbModNet* orig_mod_net,
                          dbObject* term);

  //------------------------------------------------------------------
  // Helper functions for insertBufferBeforeLoads
  //------------------------------------------------------------------
  void validateArgumentsBeforeLoads(const std::set<dbObject*>& load_pins,
                                    const dbMaster* buffer_master) const;
  dbModule* validateLoadPinsAndFindLCA(const std::set<dbObject*>& load_pins,
                                       bool loads_on_diff_nets) const;
  void createNewFlatAndHierNets(const std::set<dbObject*>& load_pins);
  void rewireBufferLoadPins(const std::set<dbObject*>& load_pins);
  void setBufferAttributes(dbInst* buffer_inst);
  void validateBufferMaster() const;

  //------------------------------------------------------------------
  // Debug logging functions
  //------------------------------------------------------------------
  void dlogBeforeLoadsParams(const std::set<dbObject*>& load_pins,
                             const Point* loc,
                             bool loads_on_diff_nets) const;
  void dlogTargetLoadPin(const dbObject* load_obj) const;
  void dlogDifferentDbNet(const dbNet* net) const;
  void dlogLCAModule(const dbModule* target_module) const;
  void dlogDumpNets(const std::set<dbNet*>& other_dbnets) const;
  void dlogCreatingNewHierNet(const char* base_name) const;
  void dlogConnectedBufferInputFlat() const;
  void dlogConnectedBufferInputHier(const dbModNet* orig_mod_net) const;
  void dlogConnectedBufferInputViaHierConnect(const dbObject* drvr_term) const;
  void dlogConnectedBufferOutput() const;
  void dlogCreatingHierConn(int load_idx,
                            int num_loads,
                            const dbITerm* load) const;
  void dlogMovedBTermLoad(int load_idx,
                          int num_loads,
                          const dbBTerm* load) const;
  void dlogPlacedBuffer(const dbInst* buffer_inst,
                        const Point& loc,
                        const char* reason) const;
  void dlogUnplacedBuffer(const dbInst* buffer_inst, const char* reason) const;
  void dlogInsertBufferSuccess(const dbInst* buffer_inst) const;
  void dlogInsertBufferStart(int count, const char* mode) const;
  void dlogSeparator() const;

  //------------------------------------------------------------------
  // Class attributes
  //------------------------------------------------------------------
  dbNet* net_{nullptr};  // Target net. The net to which the buffer is inserted
  dbBlock* block_{nullptr};
  utl::Logger* logger_{nullptr};

  // Insert buffer state
  dbITerm* buf_input_iterm_{nullptr};   // Input iterm of the new buffer
  dbITerm* buf_output_iterm_{nullptr};  // Output iterm of the new buffer
  dbNet* new_flat_net_{nullptr};    // New flat net connected to the new buffer
  dbModNet* new_mod_net_{nullptr};  // New modnet connected to the new buffer
  dbModule* target_module_{nullptr};  // Target module for the new buffer
  const dbMaster* buffer_master_{nullptr};
  const char* new_buf_base_name_{kDefaultBufBaseName};
  const char* new_net_base_name_{kDefaultNetBaseName};
  dbNameUniquifyType uniquify_{dbNameUniquifyType::ALWAYS};
  dbObject* orig_drvr_pin_{nullptr};  // Original driver pin of net_

  // Caches for hierarchical connection

  // dbModNet has target loads only (true) or
  // mixed target and non-target loads (false)
  mutable std::map<dbModNet*, bool> is_target_only_cache_;

  // Indicates a dbModNet in a module can be reused (w/o port punching) or not
  std::map<dbModule*, std::set<dbModNet*>> module_reusable_nets_;

  //------------------------------------------------------------------
  // Static attributes
  //------------------------------------------------------------------
  inline static std::atomic<int> insert_buffer_call_count_{0};
};

}  // namespace odb
