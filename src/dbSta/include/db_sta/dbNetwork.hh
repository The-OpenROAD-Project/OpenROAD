/////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2019, The Regents of the University of California
// All rights reserved.
//
// BSD 3-Clause License
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// * Redistributions of source code must retain the above copyright notice, this
//   list of conditions and the following disclaimer.
//
// * Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
//
// * Neither the name of the copyright holder nor the names of its
//   contributors may be used to endorse or promote products derived from
//   this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <set>

#include "odb/db.h"
#include "sta/ConcreteNetwork.hh"
#include "sta/GraphClass.hh"

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
using odb::dbModInst;
using odb::dbMTerm;
using odb::dbNet;
using odb::dbObject;
using odb::dbSet;
using odb::dbSigType;
using odb::Point;

class dbNetwork;
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
 public:
  dbNetwork();
  ~dbNetwork() override;
  void init(dbDatabase* db, Logger* logger);
  void setBlock(dbBlock* block);
  void clear() override;

  void readLefAfter(dbLib* lib);
  void readDefAfter(dbBlock* block);
  void readDbAfter(dbDatabase* db);
  void readLibertyAfter(LibertyLibrary* lib) override;

  void addObserver(dbNetworkObserver* observer);
  void removeObserver(dbNetworkObserver* observer);

  dbBlock* block() const { return block_; }
  void makeLibrary(dbLib* lib);
  void makeCell(Library* library, dbMaster* master);

  void location(const Pin* pin,
                // Return values.
                double& x,
                double& y,
                bool& exists) const override;
  Point location(const Pin* pin) const;
  bool isPlaced(const Pin* pin) const;

  LibertyCell* libertyCell(dbInst* inst);

  // Use the this if you know you are dealing with a leaf instance
  dbInst* staToDb(const Instance* instance) const;
  // Use the this if you might have a hierarchical instance
  void staToDb(const Instance* instance,
               // Return values.
               dbInst*& db_inst,
               dbModInst*& mod_inst) const;
  dbNet* staToDb(const Net* net) const;
  void staToDb(const Pin* pin,
               // Return values.
               dbITerm*& iterm,
               dbBTerm*& bterm) const;
  dbBTerm* staToDb(const Term* term) const;
  dbMaster* staToDb(const Cell* cell) const;
  dbMaster* staToDb(const LibertyCell* cell) const;
  dbMTerm* staToDb(const Port* port) const;
  dbMTerm* staToDb(const LibertyPort* port) const;
  void staToDb(PortDirection* dir,
               // Return values.
               dbSigType& sig_type,
               dbIoType& io_type) const;

  Pin* dbToSta(dbBTerm* bterm) const;
  Term* dbToStaTerm(dbBTerm* bterm) const;
  Pin* dbToSta(dbITerm* iterm) const;
  Instance* dbToSta(dbInst* inst) const;
  Instance* dbToSta(dbModInst* inst) const;
  Net* dbToSta(dbNet* net) const;
  const Net* dbToSta(const dbNet* net) const;
  Cell* dbToSta(dbMaster* master) const;
  Port* dbToSta(dbMTerm* mterm) const;
  PortDirection* dbToSta(const dbSigType& sig_type,
                         const dbIoType& io_type) const;
  // dbStaCbk::inDbBTermCreate
  Port* makeTopPort(dbBTerm* bterm);
  void setTopPortDirection(dbBTerm* bterm, const dbIoType& io_type);
  ObjectId id(const Port* port) const override;

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
  // Name local to containing cell/instance.
  const char* name(const Instance* instance) const override;
  ObjectId id(const Instance* instance) const override;
  Cell* cell(const Instance* instance) const override;
  Instance* parent(const Instance* instance) const override;
  bool isLeaf(const Instance* instance) const override;
  Instance* findInstance(const char* path_name) const override;
  Instance* findChild(const Instance* parent, const char* name) const override;
  InstanceChildIterator* childIterator(const Instance* instance) const override;
  InstancePinIterator* pinIterator(const Instance* instance) const override;
  InstanceNetIterator* netIterator(const Instance* instance) const override;

  ////////////////////////////////////////////////////////////////
  // Pin functions
  ObjectId id(const Pin* pin) const override;
  Pin* findPin(const Instance* instance, const char* port_name) const override;
  Pin* findPin(const Instance* instance, const Port* port) const override;
  Port* port(const Pin* pin) const override;
  Instance* instance(const Pin* pin) const override;
  Net* net(const Pin* pin) const override;
  Term* term(const Pin* pin) const override;
  PortDirection* direction(const Pin* pin) const override;
  VertexId vertexId(const Pin* pin) const override;
  void setVertexId(Pin* pin, VertexId id) override;

  ////////////////////////////////////////////////////////////////
  // Terminal functions
  Net* net(const Term* term) const override;
  Pin* pin(const Term* term) const override;
  ObjectId id(const Term* term) const override;

  ////////////////////////////////////////////////////////////////
  // Net functions
  ObjectId id(const Net* net) const override;
  Net* findNet(const Instance* instance, const char* net_name) const override;
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
  bool isSpecial(Net* net);

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
  void disconnectPinBefore(const Pin* pin);
  void deletePin(Pin* pin) override;
  Net* makeNet(const char* name, Instance* parent) override;
  Pin* makePin(Instance* inst, Port* port, Net* net) override;
  Port* makePort(Cell* cell, const char* name) override;
  void deleteNet(Net* net) override;
  void deleteNetBefore(const Net* net);
  void mergeInto(Net* net, Net* into_net) override;
  Net* mergedInto(Net* net) override;
  double dbuToMeters(int dist) const;
  int metersToDbu(double dist) const;

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
  void makeTopCell();
  void findConstantNets();
  void visitConnectedPins(const Net* net,
                          PinVisitor& visitor,
                          NetSet& visited_nets) const override;
  bool portMsbFirst(const char* port_name);

  dbDatabase* db_ = nullptr;
  Logger* logger_ = nullptr;
  dbBlock* block_ = nullptr;
  Instance* top_instance_;
  Cell* top_cell_ = nullptr;

  std::set<dbNetworkObserver*> observers_;
};

}  // namespace sta
