// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include <cstdint>
#include <memory>
#include <set>
#include <string>
#include <string_view>
#include <unordered_set>

#include "odb/PtrSetMap.h"
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
  odb::dbNet* checkRelatedDbNet(const odb::dbModNet*) const;
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
  const LibertyCell* testCell(const Cell* cell) const;
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

  // ---- 3DIC (3DBlox) cross-chiplet STA ----
  // Top dbChip when a 3Dbx design is active; null otherwise.
  odb::dbChip* topChip() const { return top_chip_; }
  void setTopChip(odb::dbChip* chip);
  // True when top_chip_ is a hierarchical chip (no own dbBlock, owns
  // dbChipInsts). Chip-aware iterators/accessors gate on this first.
  bool has3DicChip() const;
  // Master block of a chiplet instance; null if the master chip is itself
  // hierarchical (no own dbBlock).
  odb::dbBlock* blockOf(odb::dbChipInst* chip_inst) const;
  // True when chip_inst's master block is placed by exactly one chip-inst.
  // Gates descent into the master body — shared masters alias inner dbInsts
  // and would break STA's per-pin Vertex assumption (duplicated masters are
  // rejected up front with STA-3004).
  bool blockOwnedUniquelyBy(odb::dbChipInst* chip_inst) const;

  // Encode/decode chip db objects as STA handles. A chip-bump Pin is the
  // per-unfold-path dbUnfoldedChipBumpInst (so duplicated chiplet masters get
  // distinct bump vertices), tagged in the low-3-bit pointer tag
  // (kDbChipBumpInst — needs 8-byte alignment). A dbChipInst (Instance) /
  // dbChipNet (Net) is a plain reinterpret_cast, discriminated at decode by
  // dbObject::getObjectType().
  Pin* dbToSta(odb::dbUnfoldedChipBumpInst* bump_inst) const;
  Instance* dbToSta(odb::dbChipInst* chip_inst) const;
  Net* dbToSta(odb::dbChipNet* chip_net) const;
  odb::dbUnfoldedChipBumpInst* staToUnfoldedBump(const Pin* pin) const;
  odb::dbChipInst* staToDbChipInst(const Instance* instance) const;
  odb::dbChipNet* staToDbChipNet(const Net* net) const;
  // Raw chip-inst / chip-net -> their per-unfold-path objects (built in
  // setTopChip). Used by the chip-aware iterators to enumerate unfolded bumps.
  odb::dbUnfoldedChipInst* unfoldedChipInst(odb::dbChipInst* chip_inst) const;
  odb::dbUnfoldedChipNet* unfoldedChipNet(odb::dbChipNet* chip_net) const;

  // Synthesize the top Cell for a hierarchical chip plus one plain Cell per
  // chiplet master (a Port per chip-bump bterm). No LibertyCell binding, so
  // chip-inst/chip-bump property queries have a non-null Cell*/Port* and
  // chip-bump pins read as BIDIRECT (clock propagates via the fat-net model).
  void makeTopCellForChip(odb::dbChip* chip);

  // dbStaCbk::inDbBTermCreate
  Port* makeTopPort(odb::dbBTerm* bterm);
  odb::dbBTerm* isTopPort(const Port*) const;
  void setTopPortDirection(odb::dbBTerm* bterm, const odb::dbIoType& io_type);
  ObjectId id(const Port* port) const override;
  ObjectId id(const Cell* cell) const override;

  // generic connect pin -> net, supports all pin/net types
  void connectPin(Pin* pin, Net* net);
  // generic connect pin -> flat_net, hier_net.
  void connectPin(Pin* pin,
                  Net* flat_net,
                  Net* hier_net,
                  bool reassociate_hier_flat = true);
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

  bool linkNetwork(std::string_view top_cell_name,
                   bool make_black_boxes,
                   Report* report) override;
  bool isLinked() const override;

  ////////////////////////////////////////////////////////////////
  // Instance functions
  // Top level instance of the design (defined after link).
  Instance* topInstance() const override;
  bool isTopInstanceOrNull(const Instance* instance) const;
  // Name local to containing cell/instance.
  std::string name(const Instance* instance) const override;
  std::string name(const Port* port) const override;
  // Path name functions needed hierarchical verilog netlists.
  using ConcreteNetwork::pathName;
  std::string pathName(const Net* net) const override;

  std::string busName(const Port* port) const override;
  ObjectId id(const Instance* instance) const override;
  Cell* cell(const Instance* instance) const override;
  Instance* parent(const Instance* instance) const override;
  using ConcreteNetwork::isLeaf;
  bool isLeaf(const Instance* instance) const override;
  bool isLeaf(const Pin* pin) const override;
  Port* findPort(const Cell* cell, std::string_view name) const override;
  Instance* findInstance(std::string_view path_name) const override;
  Instance* findChild(const Instance* parent,
                      std::string_view name) const override;
  InstanceChildIterator* childIterator(const Instance* instance) const override;
  InstancePinIterator* pinIterator(const Instance* instance) const override;
  InstanceNetIterator* netIterator(const Instance* instance) const override;
  std::string getAttribute(const Instance* inst,
                           std::string_view key) const override;
  void setAttribute(Instance* instance,
                    std::string_view key,
                    std::string_view value) override;
  odb::dbModNet* findModNetForPin(const Pin*);
  odb::dbModInst* getModInst(Instance* inst) const;

  ////////////////////////////////////////////////////////////////
  // Pin functions
  ObjectId id(const Pin* pin) const override;
  Pin* findPin(const Instance* instance,
               std::string_view port_name) const override;
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
  std::string name(const Cell* cell) const override;
  std::string getAttribute(const Cell* cell,
                           std::string_view key) const override;
  void setAttribute(Cell* cell,
                    std::string_view key,
                    std::string_view value) override;

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
  Net* findNet(const Instance* instance,
               std::string_view net_name) const override;
  Net* findNetAllScopes(std::string_view net_name) const;
  void findInstNetsMatching(const Instance* instance,
                            const PatternMatch* pattern,
                            // Return value.
                            NetSeq& nets) const override;
  std::string name(const Net* net) const override;
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
  // - If the net is a hier net (dbModNet), return the associated flat dbNet.
  // This ensures parasitic externality checks in ensureParasiticNode work
  // correctly: net_ is always a flat net, so the comparison net != net_
  // must also operate on flat nets.
  Net* highestNetAbove(Net* net) const override;

  ////////////////////////////////////////////////////////////////
  // Edit functions
  Instance* makeInstance(LibertyCell* cell,
                         std::string_view name,
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
  Net* makeNet(std::string_view name, Instance* parent) override;
  Net* makeNet(std::string_view base_name,
               Instance* parent,
               const odb::dbNameUniquifyType& uniquify);
  Pin* makePin(Instance* inst, Port* port, Net* net) override;
  Port* makePort(Cell* cell, std::string_view name) override;
  void deleteNet(Net* net) override;
  void deleteNetBefore(const Net* net);
  void mergeInto(Net* net, Net* into_net) override;
  Net* mergedInto(Net* net) override;
  double dbuToMeters(int dist) const;
  int metersToDbu(double dist) const;

  ////////////////////////////////////////////////////////////////
  // Sequential / Flop / Scan flop utility functions
  // clock pin functions
  bool isClockPin(odb::dbITerm* iterm) const;
  bool clockOn(odb::dbInst* inst) const;

  // d pin functions
  bool isDPin(odb::dbITerm* iterm) const;
  int getNumD(odb::dbInst* inst) const;

  // q(n) pin functions
  bool isQPin(odb::dbITerm* iterm) const;
  bool isInvertingQPin(odb::dbITerm* iterm) const;
  int getNumQ(odb::dbInst* inst) const;

  // clear/preset pin functions
  bool hasClear(odb::dbInst* inst) const;
  bool isClearPin(odb::dbITerm* iterm) const;
  bool hasPreset(odb::dbInst* inst) const;
  bool isPresetPin(odb::dbITerm* iterm) const;

  // scan cell/pin functions
  bool isScanCell(odb::dbInst* inst) const;
  bool isScanIn(odb::dbITerm* iterm) const;
  odb::dbITerm* getScanIn(odb::dbInst* inst) const;
  bool isScanEnable(odb::dbITerm* iterm) const;
  odb::dbITerm* getScanEnable(odb::dbInst* inst) const;
  LibertyPort* getLibertyScanEnable(const LibertyCell* lib_cell) const;
  LibertyPort* getLibertyScanIn(const LibertyCell* lib_cell) const;
  LibertyPort* getLibertyScanOut(const LibertyCell* lib_cell) const;

  // supply pin functions
  bool isSupplyPin(odb::dbITerm* iterm) const;
  bool isValidFlop(odb::dbInst* FF) const;

  ////////////////////////////////////////////////////////////////

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
  bool portMsbFirst(std::string_view port_name, std::string_view cell_name);
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
  static constexpr unsigned DBCHIPINST_ID = 0x9;
  static constexpr unsigned DBCHIPBUMP_INST_ID = 0xA;
  static constexpr unsigned DBCHIPNET_ID = 0xB;
  static constexpr unsigned DBUNFOLDEDCHIPBUMP_INST_ID = 0xC;
  static constexpr unsigned CONCRETE_OBJECT_ID = 0xF;
  // Number of lower bits used
  static constexpr unsigned DBIDTAG_WIDTH = 0x4;
  // 3DIC: width of the per-chiplet-block discriminator stamped into the upper
  // bits of an encoded ObjectId. With DBIDTAG_WIDTH=4 tag bits and a 20-bit
  // per-block db_id, this leaves 8 bits -> up to 256 unique chiplet blocks
  // (discriminators 0..255).
  static constexpr unsigned kBlockTagWidth = 8;
  static constexpr unsigned kBlockTagShift = 32 - kBlockTagWidth;        // 24
  static constexpr uint32_t kBlockTagMask = (1U << kBlockTagWidth) - 1;  // 0xFF
  // Stamp the per-block discriminator into the top bits of an ObjectId for
  // iterm/bterm/inst/net so identically-numbered objects in different chiplet
  // blocks don't collide as NetSet/PinSet keys. 0 when not in 3DIC mode.
  ObjectId blockDiscBits(const odb::dbObject* obj, odb::dbObjectType typ) const;

 private:
  void addDriverToCacheIfPresent(const Net* net, const Pin* drvr);
  void removeDriverFromCache(const Net* net);
  void removeDriverFromCache(const Net* net, const Pin* drvr);

  // Strip the parent-instance prefix from a hierarchical name, treating
  // backslash-escaped slashes (\/) as literal name characters rather than
  // hierarchy separators.  Used to recover an in-module name from a
  // path-qualified one stored in ODB.
  static std::string stripParentPrefix(const std::string& name);

  std::set<const Cell*> hier_modules_;
  std::set<const Port*> concrete_ports_;
  std::unique_ptr<dbEditHierarchy> hierarchy_editor_;

  // ---- 3DIC (3DBlox) state ----
  // Top dbChip of a 3Dbx design (hierarchical chip, no own dbBlock).
  odb::dbChip* top_chip_ = nullptr;
  // Chiplet master dbBlock -> the chip-inst that placed it. Duplicated
  // masters (a block placed by >1 chip-inst) are rejected in setTopChip
  // (STA-3004), so every entry here is a uniquely-placed master.
  odb::PtrMap<odb::dbBlock, odb::dbChipInst*> block_to_chip_inst_;
  // Per-block 0..N-1 discriminator stamped into upper ObjectId bits so
  // iterms/bterms/insts/nets from different chiplet blocks (each numbered
  // from 1) don't collide in NetSet/PinSet keys. Starts at 0 so a single
  // chiplet block stamps nothing (no-op, same as 2D).
  odb::PtrMap<odb::dbBlock, uint32_t> block_disc_;
  // One synthetic (non-Liberty) Cell per chiplet master, backing
  // cell()/port()/name() for chip-inst and chip-bump pins. Owned by
  // chip_master_lib_. Reset in clear() (ConcreteNetwork::clear destroys the
  // owning libraries).
  odb::PtrMap<odb::dbChip, Cell*> chip_master_cells_;
  Library* chip_master_lib_ = nullptr;
  // STA vertex-id for chip-bump pins, keyed by the per-unfold-path
  // dbUnfoldedChipBumpInst (no sta_vertex_id_ field on the odb object yet).
  // Reset on clear() and on an unfolded-model rebuild (keys change).
  mutable odb::PtrMap<odb::dbUnfoldedChipBumpInst, VertexId>
      chip_bump_vertex_ids_;
  // Reverse lookups derived from the unfolded model (lazily refreshed by
  // ensureUnfoldedMapsFresh when the top chip-net count changes — picks up
  // Tcl-created chip-nets after read_3dbx):
  //   unfolded bump -> owning chip-net (for net(pin))
  //   raw chip-inst -> its unfolded chip-inst (for the pin iterator)
  //   raw chip-net  -> its unfolded chip-net  (for the net-pin iterator)
  mutable odb::PtrMap<odb::dbUnfoldedChipBumpInst, odb::dbChipNet*>
      bump_to_chip_net_;
  mutable odb::PtrMap<odb::dbChipInst, odb::dbUnfoldedChipInst*>
      chip_inst_to_unfolded_;
  mutable odb::PtrMap<odb::dbChipNet, odb::dbUnfoldedChipNet*>
      chip_net_to_unfolded_;
  mutable size_t unfolded_cache_net_count_ = 0;
  // Whether the maps above have been built at least once. Distinct from
  // (count == 0): a design legitimately with zero chip-nets is "built" with
  // empty maps and must not rebuild on every accessor call.
  mutable bool unfolded_built_ = false;
  // (Re)build the unfolded model if the chip-net count changed, then rebuild
  // the lookup maps above. const so the chip-aware accessors can call it.
  void ensureUnfoldedMapsFresh() const;
  void buildUnfoldedMaps() const;
};

}  // namespace sta
