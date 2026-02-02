// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include <memory>
#include <set>
#include <string>
#include <unordered_set>

#include "odb/db.h"
#include "odb/dbObject.h"
#include "odb/dbSet.h"
#include "odb/dbTypes.h"
#include "odb/geom.h"
#include "sta/ConcreteNetwork.hh"
#include "sta/GraphClass.hh"
#include "sta/Liberty.hh"
#include "sta/Network.hh"
#include "sta/NetworkClass.hh"
#include "utl/Logger.h"

namespace sta {

class dbNetwork;
class dbEditHierarchy;

// This class handles callbacks from the network to the listeners
class dbNetworkObserver
{
 public:
  virtual ~dbNetworkObserver();

  virtual void postReadLiberty() = 0;
  virtual void postReadDb() {}

 private:
  dbNetwork* owner_ = nullptr;
  friend class dbNetwork;
};

// This adapter implements the network api for OpenDB.
// ConcreteNetwork is used for library/cell/port functions only.
class dbNetwork : public ConcreteNetwork
{
  friend class dbEditHierarchy;

 public:
  dbNetwork();
  ~dbNetwork() override;

  void init(odb::dbDatabase* db, utl::Logger* logger);
  void setBlock(odb::dbBlock* block);
  void clear() override;
  CellPortIterator* portIterator(const Cell* cell) const override;

  // Sanity checkers
  int checkAxioms(odb::dbObject* obj = nullptr) const;
  void checkSanityModBTerms() const;
  void checkSanityModITerms() const;
  void checkSanityModuleInsts() const;
  void checkSanityModInstTerms() const;
  void checkSanityUnusedModules() const;
  void checkSanityTermConnectivity() const;
  void checkSanityNetConnectivity(odb::dbObject* obj = nullptr) const;
  void checkSanityInstNames() const;
  void checkSanityNetNames() const;
  void checkSanityModNetNamesInModule(odb::dbModule* module) const;
  void checkSanityNetDrvrPinMapConsistency() const;

  void readLefAfter(odb::dbLib* lib);
  void readDefAfter(odb::dbBlock* block);
  void readDbAfter(odb::dbDatabase* db);
  void readLibertyAfter(LibertyLibrary* lib) override;

  void addObserver(dbNetworkObserver* observer);
  void removeObserver(dbNetworkObserver* observer);

  odb::dbBlock* block() const { return block_; }
  utl::Logger* getLogger() const { return logger_; }
  void makeLibrary(odb::dbLib* lib);
  void makeCell(Library* library, odb::dbMaster* master);
  void makeVerilogCell(Library* library, odb::dbModInst*);
  void location(const Pin* pin,
                // Return values.
                double& x,
                double& y,
                bool& exists) const override;
  odb::Point location(const Pin* pin) const;
  bool isPlaced(const Pin* pin) const;

  LibertyCell* libertyCell(Cell* cell) const override;
  const LibertyCell* libertyCell(const Cell* cell) const override;
  LibertyCell* libertyCell(odb::dbInst* inst);
  LibertyPort* libertyPort(const Pin*) const override;
  odb::dbInst* staToDb(const Instance* instance) const;
  void staToDb(const Instance* instance,
               odb::dbInst*& db_inst,
               odb::dbModInst*& mod_inst) const;
  void staToDb(const Pin* pin,
               odb::dbITerm*& iterm,
               odb::dbBTerm*& bterm,
               odb::dbModITerm*& moditerm) const;
  odb::dbObject* staToDb(const Pin* pin) const;

  odb::dbNet* staToDb(const Net* net) const;
  void staToDb(const Net* net, odb::dbNet*& dnet, odb::dbModNet*& modnet) const;

  odb::dbBTerm* staToDb(const Term* term) const;
  void staToDb(const Term* term,
               odb::dbITerm*& iterm,
               odb::dbBTerm*& bterm,
               odb::dbModBTerm*& modbterm) const;
  odb::dbMaster* staToDb(const Cell* cell) const;
  void staToDb(const Cell* cell,
               odb::dbMaster*& master,
               odb::dbModule*& module) const;
  odb::dbMaster* staToDb(const LibertyCell* cell) const;
  odb::dbMTerm* staToDb(const Port* port) const;
  odb::dbMTerm* staToDb(const LibertyPort* port) const;
  void staToDb(const Port* port,
               odb::dbBTerm*& bterm,
               odb::dbMTerm*& mterm,
               odb::dbModBTerm*& modbterm) const;

  void staToDb(PortDirection* dir,
               odb::dbSigType& sig_type,
               odb::dbIoType& io_type) const;

  Pin* dbToSta(odb::dbBTerm* bterm) const;
  Term* dbToStaTerm(odb::dbBTerm* bterm) const;
  Pin* dbToSta(odb::dbITerm* iterm) const;

  ///
  /// Convert odb::dbObject* to Pin* if it's a terminal type (odb::dbITerm or
  /// odb::dbBTerm or odb::dbModITerm)
  ///
  Pin* dbToSta(odb::dbObject* term_obj) const;
  Instance* dbToSta(odb::dbInst* inst) const;
  Net* dbToSta(odb::dbNet* net) const;
  const Net* dbToSta(const odb::dbNet* net) const;
  const Net* dbToSta(const odb::dbModNet* net) const;
  Cell* dbToSta(odb::dbMaster* master) const;
  Port* dbToSta(odb::dbMTerm* mterm) const;

  Instance* dbToSta(odb::dbModInst* inst) const;
  Cell* dbToSta(odb::dbModule* master) const;
  Pin* dbToSta(odb::dbModITerm* mod_iterm) const;
  Net* dbToSta(odb::dbModNet* net) const;
  Port* dbToSta(odb::dbModBTerm* modbterm) const;
  Term* dbToStaTerm(odb::dbModBTerm* modbterm) const;

  PortDirection* dbToSta(const odb::dbSigType& sig_type,
                         const odb::dbIoType& io_type) const;

  bool isPGSupply(odb::dbITerm* iterm) const;
  bool isPGSupply(odb::dbBTerm* bterm) const;
  bool isPGSupply(odb::dbNet* net) const;

  // dbStaCbk::inDbBTermCreate
  Port* makeTopPort(odb::dbBTerm* bterm);
  odb::dbBTerm* isTopPort(const Port*) const;
  void setTopPortDirection(odb::dbBTerm* bterm, const odb::dbIoType& io_type);
  ObjectId id(const Port* port) const override;
  ObjectId id(const Cell* cell) const override;

  // generic connect pin -> net, supports all pin/net types
  void connectPin(Pin* pin, Net* net) override;
  // generic connect pin -> flat_net, hier_net.
  void connectPin(Pin* pin, Net* flat_net, Net* hier_net);
  // hierarchical support functions
  odb::dbModule* getNetDriverParentModule(Net* net,
                                          Pin*& driver_pin,
                                          bool hier = false);
  Instance* getOwningInstanceParent(Pin* pin);

  bool isSpecial(Net* net);
  void visitConnectedPins(const Net* net,
                          PinVisitor& visitor,
                          NetSet& visited_nets) const override;

  using Network::isConnected;
  bool isConnected(const Net* net, const Pin* pin) const override;
  bool isConnected(const Net* net1, const Net* net2) const override;
  bool isConnected(const Pin* source_pin, const Pin* dest_pin) const;
  // Get the flat net (odb::dbNet) with the Net*.
  // - Use odb::dbNet::hierarchicalConnect(odb::dbObject* driver, odb::dbObject*
  // load) instead.
  // - The new API can handle both odb::dbBTerm and dbIterm.
  void hierarchicalConnect(odb::dbITerm* source_pin,
                           odb::dbITerm* dest_pin,
                           const char* connection_name = "net");

  // This API is still needed if odb::dbModITerm connection is required.
  void hierarchicalConnect(odb::dbITerm* source_pin,
                           odb::dbModITerm* dest_pin,
                           const char* connection_name = "net");
  Instance* findHierInstance(const char* name);
  void replaceHierModule(odb::dbModInst* mod_inst, odb::dbModule* module);
  void removeUnusedPortsAndPinsOnModuleInstances();

  ////////////////////////////////////////////////////////////////
  //
  // Implement network API
  //
  ////////////////////////////////////////////////////////////////

  bool linkNetwork(const char* top_cell_name,
                   bool make_black_boxes,
                   Report* report) override;
  bool isLinked() const override;

  ////////////////////////////////////////////////////////////////
  // Instance functions
  // Top level instance of the design (defined after link).
  Instance* topInstance() const override;
  bool isTopInstanceOrNull(const Instance* instance) const;
  // Name local to containing cell/instance.
  const char* name(const Instance* instance) const override;
  const char* name(const Port* port) const override;
  // Path name functions needed hierarchical verilog netlists.
  using ConcreteNetwork::pathName;
  const char* pathName(const Net* net) const override;

  const char* busName(const Port* port) const override;
  ObjectId id(const Instance* instance) const override;
  Cell* cell(const Instance* instance) const override;
  Instance* parent(const Instance* instance) const override;
  using ConcreteNetwork::isLeaf;
  bool isLeaf(const Instance* instance) const override;
  bool isLeaf(const Pin* pin) const override;
  Port* findPort(const Cell* cell, const char* name) const override;
  Instance* findInstance(const char* path_name) const override;
  Instance* findChild(const Instance* parent, const char* name) const override;
  InstanceChildIterator* childIterator(const Instance* instance) const override;
  InstancePinIterator* pinIterator(const Instance* instance) const override;
  InstanceNetIterator* netIterator(const Instance* instance) const override;
  std::string getAttribute(const Instance* inst,
                           const std::string& key) const override;
  void setAttribute(Instance* instance,
                    const std::string& key,
                    const std::string& value) override;
  odb::dbNet* findRelatedDbNet(const odb::dbModNet*) const;
  odb::dbModNet* findModNetForPin(const Pin*);
  odb::dbModInst* getModInst(Instance* inst) const;

  ////////////////////////////////////////////////////////////////
  // Pin functions
  ObjectId id(const Pin* pin) const override;
  Pin* findPin(const Instance* instance, const char* port_name) const override;
  Pin* findPin(const Instance* instance, const Port* port) const override;
  Port* port(const Pin* pin) const override;
  Instance* instance(const Pin* pin) const override;
  Net* net(const Pin* pin) const override;
  void net(const Pin* pin,
           odb::dbNet*& db_net,
           odb::dbModNet*& db_modnet) const;

  ///
  /// Get a odb::dbNet connected to the input pin.
  /// - If both odb::dbNet and odb::dbModNet are connected to the input pin,
  ///   this function returns the odb::dbNet.
  /// - NOTE: If only odb::dbModNet is connected to the input pin, this
  ///   function returns nullptr. If you need to get the odb::dbNet
  ///   corresponding to the odb::dbModNet, use findFlatDbNet() instead.
  ///
  odb::dbNet* flatNet(const Pin* pin) const;

  ///
  /// Get a odb::dbModNet connected to the input pin.
  /// - If both odb::dbNet and odb::dbModNet are connected to the input pin,
  ///   this function returns the odb::dbModNet.
  /// - If only odb::dbNet is connected to the input pin, this function returns
  ///   nullptr.
  ///
  odb::dbModNet* hierNet(const Pin* pin) const;

  odb::dbITerm* flatPin(const Pin* pin) const;
  odb::dbModITerm* hierPin(const Pin* pin) const;
  odb::dbBlock* getBlockOf(const Pin* pin) const;

  bool isFlat(const Pin* pin) const;
  bool isFlat(const Net* net) const;

  Term* term(const Pin* pin) const override;
  PortDirection* direction(const Pin* pin) const override;
  VertexId vertexId(const Pin* pin) const override;
  void setVertexId(Pin* pin, VertexId id) override;
  // Find the connected odb::dbModITerm in the parent module of the input pin.
  odb::dbModITerm* findInputModITermInParent(const Pin* input_pin) const;

  ////////////////////////////////////////////////////////////////
  // Terminal functions
  Net* net(const Term* term) const override;
  odb::dbNet* flatNet(const Term* term) const;
  Pin* pin(const Term* term) const override;
  ObjectId id(const Term* term) const override;

  ////////////////////////////////////////////////////////////////
  // Cell functions
  const char* name(const Cell* cell) const override;
  std::string getAttribute(const Cell* cell,
                           const std::string& key) const override;
  void setAttribute(Cell* cell,
                    const std::string& key,
                    const std::string& value) override;

  bool isConcreteCell(const Cell*) const;
  void registerHierModule(const Cell* cell);
  void unregisterHierModule(const Cell* cell);

  ////////////////////////////////////////////////////////////////
  // Port functions
  Cell* cell(const Port* port) const override;
  void registerConcretePort(const Port*);

  bool isConcretePort(const Port*) const;
  bool isLibertyPort(const Port*) const;

  LibertyPort* libertyPort(const Port* port) const override;
  PortDirection* direction(const Port* port) const override;

  ////////////////////////////////////////////////////////////////
  // Net functions
  ObjectId id(const Net* net) const override;
  Net* findNet(const Instance* instance, const char* net_name) const override;
  Net* findNetAllScopes(const char* net_name) const;
  void findInstNetsMatching(const Instance* instance,
                            const PatternMatch* pattern,
                            // Return value.
                            NetSeq& nets) const override;
  const char* name(const Net* net) const override;
  Instance* instance(const Net* net) const override;
  bool isPower(const Net* net) const override;
  bool isGround(const Net* net) const override;
  NetPinIterator* pinIterator(const Net* net) const override;
  NetTermIterator* termIterator(const Net* net) const override;
  const Net* highestConnectedNet(Net* net) const override;

  // Get the flat net (odb::dbNet) with the Net*.
  // If the net is a hierarchical net (odb::dbModNet), return nullptr
  odb::dbNet* flatNet(const Net* net) const;

  // Given a net or pin that may be hierarchical, find the corresponding flat
  // odb::dbNet by traversing the netlist.
  // If the net is already a flat net (odb::dbNet), it is returned as is.
  // If the net is a hierarchical net (odb::dbModNet), find the associated
  // odb::dbNet.
  odb::dbNet* findFlatDbNet(const Net* net) const;
  odb::dbNet* findFlatDbNet(const Pin* pin) const;

  // Given a net that may be hierarchical, find the corresponding flat
  // odb::dbNet by traversing the netlist and return it as Net*. If the net is
  // already a flat net, it is returned as is. If the net is a hierarchical net
  // (odb::dbModNet), find the associated odb::dbNet and return it as Net*.
  Net* findFlatNet(const Net* net) const;
  Net* findFlatNet(const Pin* pin) const;

  bool hasPort(const Net* net) const;

  // Return the highest net above the given net.
  // - If the net is a flat net, return it.
  // - If the net is a hier net, return the modnet in the highest hierarchy.
  Net* highestNetAbove(Net* net) const override;

  ////////////////////////////////////////////////////////////////
  // Edit functions
  Instance* makeInstance(LibertyCell* cell,
                         const char* name,
                         Instance* parent) override;
  void makePins(Instance* inst) override;
  void replaceCell(Instance* inst, Cell* cell) override;
  // Deleting instance also deletes instance pins.
  void deleteInstance(Instance* inst) override;

  // Connect the port on an instance to a net.
  Pin* connect(Instance* inst, Port* port, Net* net) override;
  Pin* connect(Instance* inst, LibertyPort* port, Net* net) override;
  void connectPinAfter(Pin* pin);
  void disconnectPin(Pin* pin) override;
  void disconnectPin(Pin* pin, Net*);
  void disconnectPinBefore(const Pin* pin);
  void deletePin(Pin* pin) override;
  Net* makeNet(Instance* parent = nullptr);
  Net* makeNet(const char* name, Instance* parent) override;
  Net* makeNet(const char* base_name,
               Instance* parent,
               const odb::dbNameUniquifyType& uniquify);
  Pin* makePin(Instance* inst, Port* port, Net* net) override;
  Port* makePort(Cell* cell, const char* name) override;
  void deleteNet(Net* net) override;
  void deleteNetBefore(const Net* net);
  void mergeInto(Net* net, Net* into_net) override;
  Net* mergedInto(Net* net) override;
  double dbuToMeters(int dist) const;
  int metersToDbu(double dist) const;

  // hierarchy handler, set in openroad tested in network child traverserser

  void setHierarchy() { db_->setHierarchy(true); }
  void disableHierarchy() { db_->setHierarchy(false); }
  bool hasHierarchy() const { return db_->hasHierarchy(); }
  bool hasHierarchicalElements() const;
  void reassociateHierFlatNet(odb::dbModNet* mod_net,
                              odb::dbNet* new_flat_net,
                              odb::dbNet* orig_flat_net);

  void reassociateFromDbNetView(odb::dbNet* flat_net, odb::dbModNet* mod_net);
  void reassociatePinConnection(Pin* pin);

  void accumulateFlatLoadPinsOnNet(
      Net* net,
      Pin* drvr_pin,
      std::unordered_set<const Pin*>& accumulated_pins);

  int fromIndex(const Port* port) const override;
  int toIndex(const Port* port) const override;
  bool isBus(const Port*) const override;
  bool hasMembers(const Port* port) const override;
  Port* findMember(const Port* port, int index) const override;
  PortMemberIterator* memberIterator(const Port* port) const override;
  PinSet* drivers(const Pin* pin) override;
  PinSet* drivers(const Net* net) override;
  void removeDriverFromCache(const Net* net);
  void removeDriverFromCache(const Net* net, const Pin* drvr);

  using Network::cell;
  using Network::direction;
  using Network::findCellsMatching;
  using Network::findInstNetsMatching;
  using Network::findNet;
  using Network::findNetsMatching;
  using Network::findPin;
  using Network::findPortsMatching;
  using Network::libertyCell;
  using Network::libertyLibrary;
  using Network::libertyPort;
  using Network::name;
  using Network::netIterator;
  using NetworkReader::makeCell;
  using NetworkReader::makeLibrary;

 protected:
  void readDbNetlistAfter();
  void checkLibertyCellsWithoutLef() const;
  void makeTopCell();
  void findConstantNets();
  void makeAccessHashes();
  bool portMsbFirst(const char* port_name, const char* cell_name);
  ObjectId getDbNwkObjectId(const odb::dbObject* object) const;

  ////////////////////////////////////////////////////////////////
  // Debug functions
  void dumpNetDrvrPinMap() const;

  odb::dbDatabase* db_ = nullptr;
  utl::Logger* logger_ = nullptr;
  odb::dbBlock* block_ = nullptr;
  Instance* top_instance_;
  Cell* top_cell_ = nullptr;
  std::set<dbNetworkObserver*> observers_;

  // unique addresses for the db objects
  static constexpr unsigned DBITERM_ID = 0x0;
  static constexpr unsigned DBBTERM_ID = 0x1;
  static constexpr unsigned DBINST_ID = 0x2;
  static constexpr unsigned DBNET_ID = 0x3;
  static constexpr unsigned DBMODITERM_ID = 0x4;
  static constexpr unsigned DBMODBTERM_ID = 0x5;
  static constexpr unsigned DBMODINST_ID = 0x6;
  static constexpr unsigned DBMODNET_ID = 0x7;
  static constexpr unsigned DBMODULE_ID = 0x8;
  // Number of lower bits used
  static constexpr unsigned DBIDTAG_WIDTH = 0x4;

 private:
  std::set<const Cell*> hier_modules_;
  std::set<const Port*> concrete_ports_;
  std::unique_ptr<dbEditHierarchy> hierarchy_editor_;
};

}  // namespace sta
