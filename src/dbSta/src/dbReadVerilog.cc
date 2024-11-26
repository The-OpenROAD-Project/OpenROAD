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

#include "db_sta/dbReadVerilog.hh"

#include <odb/dbSet.h>

#include <map>
#include <string>

#include "db_sta/dbNetwork.hh"
#include "db_sta/dbSta.hh"
#include "odb/db.h"
#include "ord/OpenRoad.hh"
#include "sta/ConcreteNetwork.hh"
#include "sta/NetworkCmp.hh"
#include "sta/PortDirection.hh"
#include "sta/Vector.hh"
#include "sta/VerilogReader.hh"
#include "utl/Logger.h"

namespace ord {

using odb::dbBlock;
using odb::dbBTerm;
using odb::dbBusPort;
using odb::dbChip;
using odb::dbDatabase;
using odb::dbInst;
using odb::dbIoType;
using odb::dbITerm;
using odb::dbMaster;
using odb::dbModBTerm;
using odb::dbModInst;
using odb::dbModITerm;
using odb::dbModNet;
using odb::dbModule;
using odb::dbMTerm;
using odb::dbNet;
using odb::dbTech;
using utl::ORD;

using sta::Cell;
using sta::CellPortBitIterator;
using sta::CellPortIterator;
using sta::ConnectedPinIterator;
using sta::dbNetwork;
using sta::deleteVerilogReader;
using sta::Instance;
using sta::InstanceChildIterator;
using sta::InstancePinIterator;
using sta::LeafInstanceIterator;
using sta::LibertyCell;
using sta::Net;
using sta::NetConnectedPinIterator;
using sta::NetIterator;
using sta::NetTermIterator;
using sta::Network;
using sta::Pin;
using sta::PinPathNameLess;
using sta::PinSeq;
using sta::Port;
using sta::PortDirection;
using sta::Term;
using utl::Logger;

dbVerilogNetwork::dbVerilogNetwork()
{
  report_ = nullptr;
  debug_ = nullptr;
}

void dbVerilogNetwork::init(dbNetwork* db_network)
{
  db_network_ = db_network;
  copyState(db_network_);
}

dbVerilogNetwork* makeDbVerilogNetwork()
{
  return new dbVerilogNetwork;
}

void initDbVerilogNetwork(ord::OpenRoad* openroad)
{
  sta::dbSta* sta = openroad->getSta();
  openroad->getVerilogNetwork()->init(sta->getDbNetwork());
}

void deleteDbVerilogNetwork(dbVerilogNetwork* verilog_network)
{
  delete verilog_network;
}

// Facade that looks in the db network for a liberty cell if
// there isn't one in the verilog network.
Cell* dbVerilogNetwork::findAnyCell(const char* name)
{
  Cell* cell = ConcreteNetwork::findAnyCell(name);
  if (cell == nullptr) {
    cell = db_network_->findAnyCell(name);
  }
  return cell;
}

void dbReadVerilog(const char* filename, dbVerilogNetwork* verilog_network)
{
  sta::readVerilogFile(filename, verilog_network);
}

////////////////////////////////////////////////////////////////

class Verilog2db
{
 public:
  Verilog2db(Network* verilog_network,
             dbDatabase* db,
             Logger* logger,
             bool hierarchy);
  void makeBlock();
  void makeDbNetlist();

 private:
  struct LineInfo
  {
    std::string file_name;
    int line_number;
  };
  using InstPair = std::pair<const Instance*, dbModInst*>;
  using InstPairs = std::vector<InstPair>;
  void makeDbModule(Instance* inst, dbModule* parent, InstPairs& inst_pairs);
  void makeChildInsts(Instance* inst, dbModule* module, InstPairs& inst_pairs);
  void makeModBTerms(Cell* cell, dbModule* module);
  void makeModITerms(Instance* inst, dbModInst* modinst);
  dbIoType staToDb(PortDirection* dir);
  bool staToDb(dbModule* module,
               const Pin* pin,
               dbBTerm*& bterm,
               dbITerm*& iterm,
               dbModBTerm*& mod_bterm,
               dbModITerm*& mod_iterm);
  void recordBusPortsOrder();
  void makeDbNets(const Instance* inst);

  void makeVModNets(const Instance* inst, dbModInst* mod_inst);
  void makeVModNets(InstPairs& inst_pairs);
  dbModNet* constructModNet(Net* inst_pin_net, dbModule* module);

  bool hasTerminals(Net* net) const;
  dbMaster* getMaster(Cell* cell);
  void storeLineInfo(const std::string& attribute, dbInst* db_inst);

  Network* network_;
  dbDatabase* db_;
  dbBlock* block_ = nullptr;
  Logger* logger_;
  std::map<Cell*, dbMaster*> master_map_;
  // Map file names to a unique id to avoid having to store the full file name
  // for each instance
  std::map<std::string, int> src_file_id_;
  // We have to store dont_touch instances and apply the attribute after
  // creating iterms; as iterms can't be added to a dont_touch inst
  std::vector<dbInst*> dont_touch_insts;
  bool hierarchy_ = false;
  static const std::regex line_info_re;
};

// Example: "./designs/src/gcd/gcd.v:571.3-577.6"
const std::regex Verilog2db::line_info_re("^(.*):(\\d+)\\.\\d+-\\d+\\.\\d+$");

void dbLinkDesign(const char* top_cell_name,
                  dbVerilogNetwork* verilog_network,
                  dbDatabase* db,
                  Logger* logger,
                  bool hierarchy)
{
  bool link_make_black_boxes = true;
  bool success = verilog_network->linkNetwork(
      top_cell_name, link_make_black_boxes, verilog_network->report());
  if (success) {
    Verilog2db v2db(verilog_network, db, logger, hierarchy);
    v2db.makeBlock();
    v2db.makeDbNetlist();
    deleteVerilogReader();
  }
}

Verilog2db::Verilog2db(Network* network,
                       dbDatabase* db,
                       Logger* logger,
                       bool hierarchy)
    : network_(network), db_(db), logger_(logger), hierarchy_(hierarchy)
{
}

void Verilog2db::makeBlock()
{
  dbChip* chip = db_->getChip();
  if (chip == nullptr) {
    chip = dbChip::create(db_);
  }
  block_ = chip->getBlock();
  if (block_) {
    // Delete existing db network objects.
    auto insts = block_->getInsts();
    for (auto iter = insts.begin(); iter != insts.end();) {
      iter = dbInst::destroy(iter);
    }
    auto nets = block_->getNets();
    for (auto iter = nets.begin(); iter != nets.end();) {
      iter = dbNet::destroy(iter);
    }
    auto bterms = block_->getBTerms();
    for (auto iter = bterms.begin(); iter != bterms.end();) {
      iter = dbBTerm::destroy(iter);
    }
    auto mod_insts = block_->getTopModule()->getChildren();
    for (auto iter = mod_insts.begin(); iter != mod_insts.end();) {
      iter = dbModInst::destroy(iter);
    }
  } else {
    const char* design
        = network_->name(network_->cell(network_->topInstance()));
    block_ = dbBlock::create(
        chip, design, db_->getTech(), network_->pathDivider());
  }
  dbTech* tech = db_->getTech();
  block_->setDefUnits(tech->getLefUnits());
  block_->setBusDelimeters('[', ']');
}

void Verilog2db::makeDbNetlist()
{
  recordBusPortsOrder();
  // As a side effect we accumulate the instance <-> modinst pairs
  InstPairs inst_pairs;
  makeDbModule(network_->topInstance(), /* parent */ nullptr, inst_pairs);
  makeDbNets(network_->topInstance());
  if (hierarchy_) {
    makeVModNets(inst_pairs);
  }
  for (auto inst : dont_touch_insts) {
    inst->setDoNotTouch(true);
  }
}

void Verilog2db::recordBusPortsOrder()
{
  // OpenDB does not have any concept of bus ports.
  // Use a property to annotate the bus names as msb or lsb first for writing
  // verilog.
  Cell* top_cell = network_->cell(network_->topInstance());
  std::unique_ptr<CellPortIterator> bus_iter{network_->portIterator(top_cell)};
  while (bus_iter->hasNext()) {
    Port* port = bus_iter->next();
    if (network_->isBus(port)) {
      const char* port_name = network_->name(port);
      const char* cell_name = network_->name(top_cell);
      int from = network_->fromIndex(port);
      int to = network_->toIndex(port);
      string key = std::string("bus_msb_first ") + port_name + " " + cell_name;
      odb::dbBoolProperty::create(block_, key.c_str(), from > to);
    }
  }
}

void Verilog2db::storeLineInfo(const std::string& attribute, dbInst* db_inst)
{
  if (attribute.empty()) {
    return;
  }

  std::smatch match;

  if (std::regex_match(attribute, match, line_info_re)) {
    const std::string file_name = match[1];
    const auto iter = src_file_id_.find(file_name);
    int file_id;
    if (iter != src_file_id_.end()) {
      file_id = iter->second;
    } else {
      file_id = src_file_id_.size();
      src_file_id_[file_name] = file_id;
      const auto id_string = fmt::format("src_file_{}", file_id);
      odb::dbStringProperty::create(
          block_, id_string.c_str(), file_name.c_str());
    }
    odb::dbIntProperty::create(db_inst, "src_file_id", file_id);
    odb::dbIntProperty::create(db_inst, "src_file_line", stoi(match[2]));
  }
}

// Recursively builds odb's dbModule/dbModInst hierarchy corresponding
// to the sta network rooted at inst.  parent is the dbModule to build
// the hierarchy under. If null the top module is used.

void Verilog2db::makeDbModule(
    Instance* inst,
    dbModule* parent,
    // harvest the hierarchical instances. Modnets connected to these
    InstPairs& inst_pairs)
{
  Cell* cell = network_->cell(inst);

  dbModule* module;
  if (parent == nullptr) {
    module = block_->getTopModule();
  } else {
    // This uniquifies the cell
    module = dbModule::makeUniqueDbModule(
        network_->name(cell), network_->name(inst), block_);

    std::string module_inst_name = network_->name(inst);

    dbModInst* modinst
        = dbModInst::create(parent, module, module_inst_name.c_str());

    inst_pairs.emplace_back(inst, modinst);

    debugPrint(logger_,
               utl::ODB,
               "dbReadVerilog",
               1,
               "Created module instance {} in parent {} ",
               module_inst_name.c_str(),
               parent->getName());

    if (modinst == nullptr) {
      logger_->error(ORD,
                     2014,
                     "hierachical instance creation failed for {} of {}",
                     network_->name(inst),
                     network_->name(cell));
    }
    if (hierarchy_) {
      makeModBTerms(cell, module);
      makeModITerms(inst, modinst);
    }
  }
  makeChildInsts(inst, module, inst_pairs);
}

void Verilog2db::makeModBTerms(Cell* cell, dbModule* module)
{
  dbBusPort* dbbusport = nullptr;
  // make the module ports
  std::unique_ptr<CellPortIterator> cp_iter{network_->portIterator(cell)};
  while (cp_iter->hasNext()) {
    Port* port = cp_iter->next();
    if (network_->isBus(port)) {
      // make the bus port as part of the port set for the cell.
      const char* port_name = network_->name(port);
      dbModBTerm* bmodterm = dbModBTerm::create(module, port_name);
      dbbusport = dbBusPort::create(module,
                                    bmodterm,  // the root of the bus port
                                    network_->fromIndex(port),
                                    network_->toIndex(port));
      bmodterm->setBusPort(dbbusport);
      const dbIoType io_type = staToDb(network_->direction(port));
      bmodterm->setIoType(io_type);

      //
      // Make a modbterm for each bus bit
      // Keep traversal in terms of bits
      // These modbterms are annotated as being
      // part of the port bus.
      //

      const int from_index = network_->fromIndex(port);
      const int to_index = network_->toIndex(port);
      const bool updown = (from_index <= to_index) ? true : false;
      const int size
          = updown ? to_index - from_index + 1 : from_index - to_index + 1;
      for (int i = 0; i < size; i++) {
        const int ix = updown ? from_index + i : from_index - i;
        const std::string bus_bit_port = port_name + std::string("[")
                                         + std::to_string(ix)
                                         + std::string("]");
        dbModBTerm* modbterm = dbModBTerm::create(module, bus_bit_port.c_str());
        if (i == 0) {
          dbbusport->setMembers(modbterm);
        }
        if (i == size - 1) {
          dbbusport->setLast(modbterm);
        }
        dbIoType io_type = staToDb(network_->direction(port));
        bmodterm->setIoType(io_type);
      }
    } else {
      const std::string port_name = network_->name(port);
      dbModBTerm* bmodterm = dbModBTerm::create(module, port_name.c_str());
      const dbIoType io_type = staToDb(network_->direction(port));
      bmodterm->setIoType(io_type);
      debugPrint(logger_,
                 utl::ODB,
                 "dbReadVerilog",
                 1,
                 "Created module bterm {} ",
                 bmodterm->getName());
    }
  }
  module->getModBTerms().reverse();
}

void Verilog2db::makeModITerms(Instance* inst, dbModInst* modinst)
{
  // make the instance iterms and set up their reference
  // to the child ports (dbModBTerms).

  InstancePinIterator* ip_iter = network_->pinIterator(inst);
  while (ip_iter->hasNext()) {
    Pin* cur_pin = ip_iter->next();
    const std::string pin_name_string = network_->portName(cur_pin);
    //
    // we do not need to store the pin names.. But they are
    // assumed to exist in the STA world.
    //
    dbModITerm* moditerm = dbModITerm::create(modinst, pin_name_string.c_str());
    dbModBTerm* modbterm;
    std::string port_name_str = std::move(pin_name_string);
    const size_t last_idx = port_name_str.find_last_of('/');
    if (last_idx != string::npos) {
      port_name_str = port_name_str.substr(last_idx + 1);
    }
    dbModule* module = modinst->getMaster();
    modbterm = module->findModBTerm(port_name_str.c_str());
    moditerm->setChildModBTerm(modbterm);
    modbterm->setParentModITerm(moditerm);

    debugPrint(logger_,
               utl::ODB,
               "dbReadVerilog",
               1,
               "Created module iterm {} for bterm {}",
               moditerm->getName(),
               modbterm->getName());
  }
}

void Verilog2db::makeChildInsts(Instance* inst,
                                dbModule* module,
                                InstPairs& inst_pairs)
{
  std::unique_ptr<InstanceChildIterator> child_iter{
      network_->childIterator(inst)};
  while (child_iter->hasNext()) {
    Instance* child = child_iter->next();
    if (network_->isHierarchical(child)) {
      makeDbModule(child, module, inst_pairs);
    } else {
      const char* child_name = network_->pathName(child);
      Instance* parent_instance = network_->parent(child);
      dbModule* parent_module = nullptr;
      Cell* parent_cell = nullptr;
      if (parent_instance == network_->topInstance() || hierarchy_ == false) {
        parent_module = block_->getTopModule();
        parent_cell = network_->cell(parent_instance);
      } else {
        parent_cell = network_->cell(parent_instance);
        parent_module = block_->findModule(network_->name(parent_cell));
      }
      (void) parent_module;
      (void) parent_cell;
      Cell* cell = network_->cell(child);
      dbMaster* master = getMaster(cell);
      if (master == nullptr) {
        logger_->warn(ORD,
                      2013,
                      "instance {} LEF master {} not found.",
                      child_name,
                      network_->name(cell));
        continue;
      }

      auto db_inst = dbInst::create(block_, master, child_name, false, module);

      // Yosys writes a src attribute on sequential instances to give the
      // Verilog source info.
      storeLineInfo(network_->getAttribute(child, "src"), db_inst);

      const auto dont_touch = network_->getAttribute(child, "dont_touch");
      if (!dont_touch.empty()) {
        if (std::stoi(dont_touch)) {
          dont_touch_insts.push_back(db_inst);
        }
      }

      if (db_inst == nullptr) {
        logger_->error(ORD,
                       2015,
                       "Leaf instance creation failed for {} of {}",
                       network_->name(child),
                       module->getName());
      }
    }
  }

  if (module->getChildren().reversible()
      && module->getChildren().orderReversed()) {
    module->getChildren().reverse();
  }

  if (module->getInsts().reversible() && module->getInsts().orderReversed()) {
    module->getInsts().reverse();
  }
}

bool Verilog2db::staToDb(dbModule* module,
                         const Pin* pin,
                         dbBTerm*& bterm,
                         dbITerm*& iterm,
                         dbModBTerm*& mod_bterm,
                         dbModITerm*& mod_iterm)
{
  mod_bterm = nullptr;
  mod_iterm = nullptr;
  bterm = nullptr;
  iterm = nullptr;

  const char* port_name = network_->portName(pin);
  Instance* cur_inst = network_->instance(pin);
  std::string pin_name = network_->portName(pin);

  //
  // cases: All the things a pin could be:
  //
  // 1. A pin on a module instance (moditerm)
  // 2. A port on the top level (bterm)
  // 3. A pin on a dbInst (iterm)
  // 4. A port on a module (modbterm).
  //

  if (module) {
    if (cur_inst) {
      std::string instance_name = network_->pathName(cur_inst);
      size_t last_idx = instance_name.find_last_of('/');
      if (last_idx != string::npos) {
        instance_name = instance_name.substr(last_idx + 1);
      }
      dbModInst* mod_inst = module->findModInst(instance_name.c_str());
      if (mod_inst) {
        mod_iterm = mod_inst->findModITerm(pin_name.c_str());
      }
    }
  }

  if (!mod_iterm) {
    // a pin on the top level. Use the port name
    if (cur_inst == network_->topInstance()) {
      bterm = block_->findBTerm(port_name);
    } else {
      // a pin on an instance
      // we store just the pin name on the db inst iterm
      std::string instance_name = network_->pathName(cur_inst);
      size_t last_idx = pin_name.find_last_of('/');
      if (last_idx != string::npos) {
        pin_name = pin_name.substr(last_idx + 1);
      }
      // we store the full instance name for db insts
      dbInst* db_inst = module->findDbInst(instance_name.c_str());
      if (db_inst) {
        iterm = db_inst->findITerm(pin_name.c_str());
      } else {
        // a port on the module itself (a mod bterm)
        mod_bterm = module->findModBTerm(pin_name.c_str());
      }
    }
  }
  if (bterm || iterm || mod_iterm || mod_bterm) {
    return true;
  }
  return false;
}

dbIoType Verilog2db::staToDb(PortDirection* dir)
{
  if (dir == PortDirection::input()) {
    return dbIoType::INPUT;
  }
  if (dir == PortDirection::output()) {
    return dbIoType::OUTPUT;
  }
  if (dir == PortDirection::bidirect()) {
    return dbIoType::INOUT;
  }
  if (dir == PortDirection::tristate()) {
    return dbIoType::OUTPUT;
  }
  if (dir == PortDirection::unknown()) {
    return dbIoType::INPUT;
  }
  return dbIoType::INOUT;
}

void Verilog2db::makeDbNets(const Instance* inst)
{
  bool is_top = (inst == network_->topInstance());
  std::unique_ptr<NetIterator> net_iter{network_->netIterator(inst)};
  // Todo, put dbnets in the module in case of hierarchy (not block)
  while (net_iter->hasNext()) {
    Net* net = net_iter->next();

    if (!is_top && hasTerminals(net)) {
      continue;
    }

    const char* net_name = network_->pathName(net);
    dbNet* db_net = dbNet::create(block_, net_name);
    if (network_->isPower(net)) {
      db_net->setSigType(odb::dbSigType::POWER);
    }
    if (network_->isGround(net)) {
      db_net->setSigType(odb::dbSigType::GROUND);
    }

    // Sort connected pins for regression stability.
    PinSeq net_pins;
    std::unique_ptr<NetConnectedPinIterator> pin_iter{
        network_->connectedPinIterator(net)};
    while (pin_iter->hasNext()) {
      const Pin* pin = pin_iter->next();
      net_pins.push_back(pin);
    }
    sort(net_pins, PinPathNameLess(network_));

    for (const Pin* pin : net_pins) {
      if (network_->isTopLevelPort(pin)) {
        const char* port_name = network_->portName(pin);
        if (block_->findBTerm(port_name) == nullptr) {
          dbBTerm* bterm = dbBTerm::create(db_net, port_name);
          dbIoType io_type = staToDb(network_->direction(pin));
          bterm->setIoType(io_type);
        }
      } else if (network_->isLeaf(pin)) {
        const char* port_name = network_->portName(pin);
        Instance* inst = network_->instance(pin);
        const char* inst_name = network_->pathName(inst);
        dbInst* db_inst = block_->findInst(inst_name);
        if (db_inst) {
          dbMaster* master = db_inst->getMaster();
          dbMTerm* mterm = master->findMTerm(block_, port_name);
          if (mterm) {
            db_inst->getITerm(mterm)->connect(db_net);
          }
        }
      }
    }
  }

  std::unique_ptr<InstanceChildIterator> child_iter{
      network_->childIterator(inst)};
  while (child_iter->hasNext()) {
    const Instance* child = child_iter->next();
    makeDbNets(child);
  }
}

void Verilog2db::makeVModNets(InstPairs& inst_pairs)
{
  for (auto& [inst, modinst] : inst_pairs) {
    makeVModNets(inst, modinst);
  }
}

void Verilog2db::makeVModNets(const Instance* inst, dbModInst* mod_inst)
{
  // Given a hierarchical instance, get the pins on the outside
  // and the inside of the instance and construct the modnets

  dbModule* parent_module = mod_inst->getParent();
  dbModule* child_module = mod_inst->getMaster();

  std::unique_ptr<InstancePinIterator> pinIter{network_->pinIterator(inst)};
  while (pinIter->hasNext()) {
    Pin* inst_pin = pinIter->next();
    Net* inst_pin_net = network_->net(inst_pin);

    if (!inst_pin_net) {
      continue;
    }

    dbModNet* upper_mod_net = constructModNet(inst_pin_net, parent_module);

    dbModITerm* mod_iterm = nullptr;
    dbModBTerm* mod_bterm = nullptr;
    dbBTerm* bterm = nullptr;
    dbITerm* iterm = nullptr;
    staToDb(child_module, inst_pin, bterm, iterm, mod_bterm, mod_iterm);
    if (mod_bterm) {
      mod_iterm = mod_bterm->getParentModITerm();
      if (mod_iterm) {
        mod_iterm->connect(upper_mod_net);
      }
    }

    // make sure any top level bterms are connected to this net too...
    if (parent_module == block_->getTopModule()) {
      NetConnectedPinIterator* pin_iter
          = network_->connectedPinIterator(inst_pin_net);
      while (pin_iter->hasNext()) {
        const Pin* pin = pin_iter->next();
        staToDb(parent_module, pin, bterm, iterm, mod_bterm, mod_iterm);
        if (bterm) {
          bterm->connect(upper_mod_net);
        }
      }
    }

    // push down inside the hierarchical instance to find any
    // modnets connected on the inside of the instance
    Net* below_pin_net;
    Term* below_term = network_->term(inst_pin);
    if (below_term) {
      below_pin_net = network_->net(below_term);
      const char* below_net_name = network_->name(below_pin_net);
      if (child_module->getModNet(below_net_name)) {
        continue;
      }
      std::string pin_name = network_->name(below_term);
      size_t last_idx = pin_name.find_last_of('/');
      if (last_idx != string::npos) {
        pin_name = pin_name.substr(last_idx + 1);
      }
      dbModBTerm* mod_bterm = child_module->findModBTerm(pin_name.c_str());
      dbModNet* lower_mod_net = constructModNet(below_pin_net, child_module);
      mod_bterm->connect(lower_mod_net);
    }
  }
}

dbModNet* Verilog2db::constructModNet(Net* inst_pin_net, dbModule* module)
{
  dbModNet* db_mod_net = nullptr;

  std::unique_ptr<sta::NetPinIterator> npi{network_->pinIterator(inst_pin_net)};
  std::map<std::string, const sta::Pin*> net_pin_map;
  while (npi->hasNext()) {
    const sta::Pin* net_pin = npi->next();
    net_pin_map[network_->name(net_pin)] = net_pin;
  }

  const char* net_name = network_->name(inst_pin_net);
  db_mod_net = module->getModNet(net_name);
  if (!db_mod_net) {
    db_mod_net = dbModNet::create(module, net_name);
  }
  for (auto& [name, pin] : net_pin_map) {
    dbITerm* iterm = nullptr;
    dbBTerm* bterm = nullptr;
    dbModITerm* mod_iterm = nullptr;
    dbModBTerm* mod_bterm = nullptr;
    // Make the connections to the mod net
    staToDb(module, pin, bterm, iterm, mod_bterm, mod_iterm);
    // leaf -> iterm
    // root -> bterm
    // instance -> moditerm
    // parent -> modbterm
    if (iterm) {
      iterm->connect(db_mod_net);
    } else if (bterm) {
      bterm->connect(db_mod_net);
    } else if (mod_bterm) {
      mod_bterm->connect(db_mod_net);
    } else if (mod_iterm) {
      mod_iterm->connect(db_mod_net);
    }
  }
  return db_mod_net;
}

bool Verilog2db::hasTerminals(Net* net) const
{
  std::unique_ptr<NetTermIterator> term_iter{network_->termIterator(net)};
  return term_iter->hasNext();
}

dbMaster* Verilog2db::getMaster(Cell* cell)
{
  auto miter = master_map_.find(cell);
  if (miter != master_map_.end()) {
    return miter->second;
  }
  const char* cell_name = network_->name(cell);
  dbMaster* master = db_->findMaster(cell_name);
  if (master) {
    master_map_[cell] = master;
    // Check for corresponding liberty cell.
    LibertyCell* lib_cell = network_->libertyCell(cell);
    if (lib_cell == nullptr) {
      logger_->warn(ORD, 2011, "LEF master {} has no liberty cell.", cell_name);
    }
    return master;
  }
  LibertyCell* lib_cell = network_->libertyCell(cell);
  if (lib_cell) {
    logger_->warn(ORD, 2012, "Liberty cell {} has no LEF master.", cell_name);
  }
  // OpenSTA read_verilog warns about missing cells.
  master_map_[cell] = nullptr;
  return nullptr;
}

}  // namespace ord
