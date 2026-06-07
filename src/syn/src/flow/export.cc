// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// Export the mapped gate-level netlist to ODB.

#include "flow/export.h"

#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <string>
#include <vector>

#include "db_sta/dbNetwork.hh"
#include "db_sta/dbSta.hh"
#include "odb/db.h"
#include "sta/Liberty.hh"
#include "sta/PortDirection.hh"
#include "syn/ir/Bundle.h"
#include "syn/ir/Graph.h"
#include "syn/ir/Instance.h"
#include "syn/ir/Net.h"
#include "utl/Logger.h"

namespace syn {

void exportToOdb(Graph& g,
                 odb::dbDatabase* db,
                 sta::dbSta* sta,
                 utl::Logger* logger)
{
  g.normalize();

  auto* tech = db->getTech();
  if (!tech) {
    logger->error(utl::SYN,
                  33,
                  "Export to ODB: no tech in database;"
                  " read LEF files first");
    return;
  }
  auto* chip = db->getChip();
  if (!chip) {
    chip = odb::dbChip::create(db, tech);
  }
  const char* block_name = g.name().empty() ? "syn" : g.name().c_str();
  auto* block = chip->getBlock();
  if (block) {
    odb::dbBlock::destroy(block);
  }
  block = odb::dbBlock::create(chip, block_name);
  auto* network = sta->getDbNetwork();

  // Collect net names from Name instances
  std::vector<std::string> net_names(g.tableSize());
  g.forEachInstance([&](const Instance* inst) {
    auto* name_inst = inst->try_as<Name>();
    if (!name_inst) {
      return;
    }
    const Bundle& val = name_inst->value();
    for (uint32_t i = 0; i < val.width(); i++) {
      Net net = val[i];
      if (net.isConst()) {
        continue;
      }

      std::string suffix;
      // If the Name looks through an inverter, name the inverter's input
      // with an asterisk to signify complementation.
      auto [producer, off] = g.resolve(net);
      if (auto* not1 = producer->try_as<Not>()) {
        net = not1->a()[off];
        if (net.isConst()) {
          continue;
        }
        suffix = "*";
      }

      uint32_t id = Graph::netId(net);
      if (!net_names[id].empty()) {
        continue;  // first name wins
      }
      if (!name_inst->isVector()) {
        net_names[id] = name_inst->nameStr() + suffix;
      } else {
        net_names[id] = name_inst->nameStr() + "["
                        + std::to_string(name_inst->from() + i) + "]" + suffix;
      }
    }
  });

  // Map graph net IDs to dbNets (lazily created)
  std::vector<odb::dbNet*> net2db(g.tableSize(), nullptr);
  int net_counter = 0;

  auto getOrCreateNet = [&](Net net) -> odb::dbNet* {
    uint32_t id = Graph::netId(net);
    if (net2db[id]) {
      return net2db[id];
    }
    if (net.isConst()) {
      logger->error(utl::SYN,
                    26,
                    "Export to ODB: constant net (id={}) used as instance pin;"
                    " tie cells should have been inserted",
                    id);
    }
    std::string name = net_names[id].empty()
                           ? "n" + std::to_string(net_counter++)
                           : net_names[id];
    auto* dbnet = odb::dbNet::create(block, name.c_str());
    if (!dbnet) {
      // Name collision — fall back to a unique name
      name = "n" + std::to_string(net_counter++);
      dbnet = odb::dbNet::create(block, name.c_str());
    }
    net2db[id] = dbnet;
    return net2db[id];
  };

  int inst_counter = 0;

  // First pass: create dbNets for Input port outputs and Target outputs
  // so they exist before we try to connect to them.
  g.forEachInstance([&](const Instance* inst) {
    if (inst->is<TieLow>() || inst->is<TieHigh>() || inst->is<TieX>()) {
      return;
    }
    if (inst->is<Name>()) {
      return;
    }
    if (inst->is<Not>()) {
      // Ignore inverters, they should exist for naming only.
      return;
    }

    if (auto* breaker = inst->try_as<LoopBreaker>()) {
      BundleView out = g.output(inst);
      for (uint32_t i = 0; i < out.width(); i++) {
        odb::dbNet* dbOut = getOrCreateNet(out[i]);
        odb::dbNet* dbIn = getOrCreateNet(breaker->a()[i]);
        dbIn->mergeNet(dbOut);
        // dbIn survives
        net2db[Graph::netId(out[i])] = dbIn;
      }
      return;
    }

    if (auto* input = inst->try_as<Input>()) {
      BundleView out = g.output(inst);
      for (uint32_t i = 0; i < out.width(); i++) {
        std::string name = input->name();
        if (out.width() > 1) {
          name += "[" + std::to_string(i) + "]";
        }
        auto* net = odb::dbNet::create(block, name.c_str());
        net2db[Graph::netId(out[i])] = net;
        auto* bterm = odb::dbBTerm::create(net, name.c_str());
        bterm->setIoType(odb::dbIoType::INPUT);
      }
      return;
    }

    if (auto* output = inst->try_as<Output>()) {
      const Bundle& value = output->value();
      for (uint32_t i = 0; i < value.width(); i++) {
        std::string name = output->name();
        if (value.width() > 1) {
          name += "[" + std::to_string(i) + "]";
        }
        auto* net = getOrCreateNet(value[i]);
        auto* bterm = odb::dbBTerm::create(net, name.c_str());
        bterm->setIoType(odb::dbIoType::OUTPUT);
      }
      return;
    }

    if (auto* target = inst->try_as<Target>()) {
      odb::dbMaster* master = network->staToDb(target->cell());
      if (!master) {
        logger->error(utl::SYN,
                      27,
                      "Export to ODB: no dbMaster for cell '{}'",
                      target->cell()->name());
        return;
      }

      std::string inst_name = "u" + std::to_string(inst_counter++);
      auto* dbinst = odb::dbInst::create(block, master, inst_name.c_str());

      // Connect input pins
      int inputIdx = 0;
      BundleView out = g.output(inst);
      int outputIdx = 0;

      sta::LibertyCellPortIterator pit(target->cell());
      while (pit.hasNext()) {
        sta::LibertyPort* port = pit.next();
        if (port->isPwrGnd()) {
          continue;
        }

        if (port->direction()->isInput()) {
          for (int j = 0; j < port->size(); j++) {
            auto* net = getOrCreateNet(target->inputs()[inputIdx++]);

            std::string pin_name = port->name();
            if (port->isBus()) {
              int bit = port->fromIndex() < port->toIndex()
                            ? port->fromIndex() + j
                            : port->fromIndex() - j;
              pin_name = port->findBusBit(bit)->name();
            }
            auto* mterm = master->findMTerm(pin_name.c_str());
            if (!mterm) {
              logger->error(
                  utl::SYN,
                  28,
                  "Export to ODB: cannot find mterm '{}' on master '{}'",
                  pin_name,
                  master->getName());
            }
            dbinst->getITerm(mterm)->connect(net);
          }
        } else if (port->direction()->isOutput()) {
          for (int j = 0; j < port->size(); j++) {
            if (outputIdx >= out.width()) {
              logger->error(
                  utl::SYN,
                  34,
                  "Export to ODB: output index {} exceeds output width {}"
                  " for cell '{}' instance '{}'",
                  outputIdx,
                  out.width(),
                  target->cell()->name(),
                  inst_name);
            }
            auto* net = getOrCreateNet(out[outputIdx++]);
            assert(net);

            std::string pin_name = port->name();
            if (port->isBus()) {
              int bit = port->fromIndex() < port->toIndex()
                            ? port->fromIndex() + j
                            : port->fromIndex() - j;
              pin_name = port->findBusBit(bit)->name();
            }
            auto* mterm = master->findMTerm(pin_name.c_str());
            if (!mterm) {
              logger->error(
                  utl::SYN,
                  29,
                  "Export to ODB: cannot find mterm '{}' on master '{}'",
                  pin_name,
                  master->getName());
            }
            dbinst->getITerm(mterm)->connect(net);
          }
        }
      }
      return;
    }

    // Anything else is unexpected at this point
    logger->warn(utl::SYN,
                 30,
                 "Export to ODB: skipping unexpected instance type '{}'",
                 cellKeyword(inst->entryType()));
  });

  logger->info(utl::SYN,
               31,
               "Export to ODB: created {} instances, {} nets, {} ports",
               inst_counter,
               net_counter,
               block->getBTerms().size());

  db->triggerPostReadDb();
}

}  // namespace syn
