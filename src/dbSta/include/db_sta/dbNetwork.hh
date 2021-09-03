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

#include "sta/ConcreteNetwork.hh"
#include "sta/GraphClass.hh"
#include "odb/db.h"

namespace utl {
class Logger;
}

namespace sta {

using utl::Logger;

using odb::dbDatabase;
using odb::dbObject;
using odb::dbLib;
using odb::dbMaster;
using odb::dbBlock;
using odb::dbInst;
using odb::dbNet;
using odb::dbBTerm;
using odb::dbITerm;
using odb::dbMTerm;
using odb::dbSigType;
using odb::dbIoType;
using odb::dbSet;
using odb::Point;

// This adapter implements the network api for OpenDB.
// ConcreteNetwork is used for library/cell/port functions only.
class dbNetwork : public ConcreteNetwork
{
public:
  dbNetwork();
  virtual ~dbNetwork();
  void setDb(dbDatabase *db);
  void setBlock(dbBlock *block);
  void setLogger(Logger *logger);
  virtual void clear();

  void readLefAfter(dbLib *lib);
  void readDefAfter(dbBlock* block);
  void readDbAfter(dbDatabase* db);
  void readLibertyAfter(LibertyLibrary *lib);

  dbBlock *block() const { return block_; }
  void makeLibrary(dbLib *lib);
  void makeCell(Library *library,
		dbMaster *master);

  virtual void location(const Pin *pin,
			// Return values.
			double &x,
			double &y,
			bool &exists) const;
  virtual Point location(const Pin *pin) const;
  bool isPlaced(const Pin *pin) const;

  LibertyCell *libertyCell(dbInst *inst);

  dbInst *staToDb(const Instance *instance) const;
  dbNet *staToDb(const Net *net) const;
  void staToDb(const Pin *pin,
	       // Return values.
	       dbITerm *&iterm,
	       dbBTerm *&bterm) const;
  dbBTerm *staToDb(const Term *term) const;
  dbMaster *staToDb(const Cell *cell) const;
  dbMaster *staToDb(const LibertyCell *cell) const;
  dbMTerm *staToDb(const Port *port) const;
  dbMTerm *staToDb(const LibertyPort *port) const;
  void staToDb(PortDirection *dir,
	       // Return values.
	       dbSigType &sig_type,
	       dbIoType &io_type) const;

  Pin *dbToSta(dbBTerm *bterm) const;
  Term *dbToStaTerm(dbBTerm *bterm) const;
  Pin *dbToSta(dbITerm *iterm) const;
  Instance *dbToSta(dbInst *inst) const;
  Net *dbToSta(dbNet *net) const;
  const Net *dbToSta(const dbNet *net) const;
  Cell *dbToSta(dbMaster *master) const;
  Port *dbToSta(dbMTerm *mterm) const;
  PortDirection *dbToSta(dbSigType sig_type,
			 dbIoType io_type) const;

  ////////////////////////////////////////////////////////////////
  //
  // Implement network API
  //
  ////////////////////////////////////////////////////////////////
  
  virtual bool linkNetwork(const char *top_cell_name,
			   bool make_black_boxes,
			   Report *report);
  virtual bool isLinked() const;

  ////////////////////////////////////////////////////////////////
  // Instance functions
  // Top level instance of the design (defined after link).
  virtual Instance *topInstance() const;
  // Name local to containing cell/instance.
  virtual const char *name(const Instance *instance) const;
  virtual Cell *cell(const Instance *instance) const;
  virtual Instance *parent(const Instance *instance) const;
  virtual bool isLeaf(const Instance *instance) const;
  virtual Instance *findInstance(const char *path_name) const;
  virtual Instance *findChild(const Instance *parent,
			      const char *name) const;
  virtual InstanceChildIterator *
  childIterator(const Instance *instance) const;
  virtual InstancePinIterator *
  pinIterator(const Instance *instance) const;
  virtual InstanceNetIterator *
  netIterator(const Instance *instance) const;

  ////////////////////////////////////////////////////////////////
  // Pin functions
  virtual Pin *findPin(const Instance *instance,
		       const char *port_name) const;
  virtual Pin *findPin(const Instance *instance,
		       const Port *port) const;
  virtual Port *port(const Pin *pin) const;
  virtual Instance *instance(const Pin *pin) const;
  virtual Net *net(const Pin *pin) const;
  virtual Term *term(const Pin *pin) const;
  virtual PortDirection *direction(const Pin *pin) const;
  virtual VertexId vertexId(const Pin *pin) const;
  virtual void setVertexId(Pin *pin,
			   VertexId id);

  ////////////////////////////////////////////////////////////////
  // Terminal functions
  virtual Net *net(const Term *term) const;
  virtual Pin *pin(const Term *term) const;

  ////////////////////////////////////////////////////////////////
  // Net functions
  virtual Net *findNet(const Instance *instance,
		       const char *net_name) const;
  virtual void findInstNetsMatching(const Instance *instance,
				    const PatternMatch *pattern,
				    // Return value.
				    NetSeq *nets) const;
  virtual const char *name(const Net *net) const;
  virtual Instance *instance(const Net *net) const;
  virtual bool isPower(const Net *net) const;
  virtual bool isGround(const Net *net) const;
  virtual NetPinIterator *pinIterator(const Net *net) const;
  virtual NetTermIterator *termIterator(const Net *net) const;
  virtual Net *highestConnectedNet(Net *net) const;
  bool isSpecial(Net *net);

  ////////////////////////////////////////////////////////////////
  // Edit functions
  virtual Instance *makeInstance(LibertyCell *cell,
				 const char *name,
				 Instance *parent);
  virtual void makePins(Instance *inst);
  virtual void replaceCell(Instance *inst,
			   Cell *cell);
  // Deleting instance also deletes instance pins.
  virtual void deleteInstance(Instance *inst);
  // Connect the port on an instance to a net.
  virtual Pin *connect(Instance *inst,
		       Port *port,
		       Net *net);
  virtual Pin *connect(Instance *inst,
		       LibertyPort *port,
		       Net *net);
  void connectPinAfter(Pin *pin);
  virtual void disconnectPin(Pin *pin);
  void disconnectPinBefore(Pin *pin);
  virtual void deletePin(Pin *pin);
  virtual Net *makeNet(const char *name,
		       Instance *parent);
  virtual void deleteNet(Net *net);
  void deleteNetBefore(Net *net);
  virtual void mergeInto(Net *net,
			 Net *into_net);
  virtual Net *mergedInto(Net *net);
  double dbuToMeters(int dist) const;
  int metersToDbu(double dist) const;

  using Network::netIterator;
  using Network::findPin;
  using Network::findNet;
  using Network::findCellsMatching;
  using Network::findPortsMatching;
  using Network::findNetsMatching;
  using Network::findInstNetsMatching;
  using Network::libertyLibrary;
  using Network::libertyCell;
  using Network::libertyPort;
  using Network::direction;
  using Network::name;
  using Network::cell;
  using NetworkReader::makeLibrary;
  using NetworkReader::makeCell;

protected:
  void readDbNetlistAfter();
  void makeTopCell();
  void findConstantNets();
  virtual void visitConnectedPins(const Net *net,
                                  PinVisitor &visitor,
                                  ConstNetSet &visited_nets) const;
  bool portMsbFirst(const char *port_name);

  dbDatabase *db_;
  Logger *logger_;
  dbBlock *block_;
  Instance *top_instance_;
  Cell *top_cell_;
};

} // namespace

