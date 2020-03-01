// OpenStaDB, OpenSTA on OpenDB
// Copyright (c) 2019, Parallax Software, Inc.
// 
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.

#ifndef STA_DB_NETWORK_H
#define STA_DB_NETWORK_H

#include "ConcreteNetwork.hh"
#include "GraphClass.hh"
#include "opendb/db.h"

namespace sta {

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

// This adapter implements the network api for OpenDB.
// ConcreteNetwork is used for library/cell/port functions only.
class dbNetwork : public ConcreteNetwork
{
public:
  dbNetwork();
  virtual ~dbNetwork();
  void setDb(dbDatabase *db);
  void setBlock(dbBlock *block);
  virtual void clear();

  void readLefAfter(dbLib *lib);
  void readDefAfter();
  void readDbAfter();
  void readLibertyAfter(LibertyLibrary *lib);

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

  virtual ConstantPinIterator *constantPinIterator();

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
  virtual void disconnectPin(Pin *pin);
  virtual void deletePin(Pin *pin);
  virtual Net *makeNet(const char *name,
		       Instance *parent);
  virtual void deleteNet(Net *net);
  virtual void mergeInto(Net *net,
			 Net *into_net);
  virtual Net *mergedInto(Net *net);

  ////////////////////////////////////////////////////////////////
  dbBlock *block() const { return block_; }
  void makeLibrary(dbLib *lib);
  void makeCell(Library *library,
		dbMaster *master);
  void makeTopCell();

  dbInst *staToDb(const Instance *instance) const;
  dbNet *staToDb(const Net *net) const;
  void staToDb(const Pin *pin,
	       // Return values.
	       dbITerm *&iterm,
	       dbBTerm *&bterm) const;
  dbBTerm *staToDb(const Term *term) const;
  dbMaster *staToDb(const Cell *cell) const;
  dbMTerm *staToDb(const Port *port) const;
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
  void visitConnectedPins(const Net *net,
			  PinVisitor &visitor,
			  ConstNetSet &visited_nets) const;

  dbDatabase *db_;
  dbBlock *block_;
  Instance *top_instance_;
  Cell *top_cell_;
};

} // namespace
#endif
