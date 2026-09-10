// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include "ClockTree.h"

#include <algorithm>
#include <functional>
#include <optional>
#include <unordered_set>
#include <vector>

#include "db_sta/dbNetwork.hh"
#include "odb/db.h"
#include "sta/Liberty.hh"

namespace wmk {

using odb::dbBlock;
using odb::dbInst;
using odb::dbITerm;
using odb::dbNet;

namespace {

struct BufferPorts
{
  dbITerm* input = nullptr;
  dbITerm* output = nullptr;
  bool inverted = false;
};

std::optional<BufferPorts> bufferPorts(dbInst* inst, sta::dbNetwork* network)
{
  const sta::LibertyCell* cell = network->libertyCell(inst);
  if (cell == nullptr || !cell->sequentials().empty() || cell->isClockGate()
      || (!cell->isBuffer() && !cell->isInverter())) {
    return std::nullopt;
  }
  BufferPorts ports;
  ports.inverted = cell->isInverter();
  for (dbITerm* iterm : inst->getITerms()) {
    if (iterm->getSigType().isSupply()) {
      continue;
    }
    if (iterm->getIoType() == odb::dbIoType::INPUT && ports.input == nullptr) {
      ports.input = iterm;
    } else if (iterm->getIoType() == odb::dbIoType::OUTPUT
               && ports.output == nullptr) {
      ports.output = iterm;
    } else {
      return std::nullopt;
    }
    const sta::LibertyPort* port
        = network->libertyPort(network->dbToSta(iterm));
    if (port == nullptr || port->tristateEnable() != nullptr) {
      return std::nullopt;
    }
  }
  if (ports.input == nullptr || ports.output == nullptr) {
    return std::nullopt;
  }
  return ports;
}

}  // namespace

odb::dbNet* singleOutputNet(dbInst* inst)
{
  dbITerm* output = nullptr;
  for (dbITerm* iterm : inst->getITerms()) {
    if (iterm->getIoType() == odb::dbIoType::OUTPUT) {
      if (output != nullptr) {
        return nullptr;
      }
      output = iterm;
    }
  }
  return output == nullptr ? nullptr : output->getNet();
}

bool isSequentialClockSink(dbITerm* iterm, sta::dbNetwork* network)
{
  return network->isRegClkPin(network->dbToSta(iterm));
}

std::optional<int> seqFanout(dbInst* lcb, sta::dbNetwork* network)
{
  const auto ports = bufferPorts(lcb, network);
  if (!ports || ports->output->getNet() == nullptr) {
    return std::nullopt;
  }
  int count = 0;
  for (dbITerm* iterm : ports->output->getNet()->getITerms()) {
    if (network->libertyPort(network->dbToSta(iterm)) == nullptr) {
      return std::nullopt;
    }
    if (isSequentialClockSink(iterm, network)) {
      ++count;
    }
  }
  return count;
}

std::vector<dbInst*> findLeafClockBuffers(dbBlock* block,
                                          sta::dbNetwork* network)
{
  std::vector<dbInst*> lcbs;
  for (dbInst* inst : block->getInsts()) {
    dbNet* out = singleOutputNet(inst);
    if (out == nullptr || out->getSigType() != odb::dbSigType::CLOCK) {
      continue;
    }
    const auto fanout = seqFanout(inst, network);
    if (fanout && *fanout > 0) {
      lcbs.push_back(inst);
    }
  }
  // Name order keeps pairing independent of database insertion order.
  std::ranges::sort(
      lcbs, [](dbInst* a, dbInst* b) { return a->getName() < b->getName(); });
  return lcbs;
}

bool canMoveClockSink(dbITerm* sink, dbNet* destination)
{
  dbNet* origin = sink->getNet();
  dbInst* inst = sink->getInst();
  return origin != nullptr && destination != nullptr && origin != destination
         && origin->getBlock() == destination->getBlock()
         && !origin->isDoNotTouch() && !destination->isDoNotTouch()
         && !inst->isDoNotTouch() && !inst->isFixed()
         && !sink->getDb()->hasHierarchy() && sink->getModNet() == nullptr;
}

bool tryClockSinkMove(dbITerm* sink,
                      dbNet* destination,
                      const std::function<void()>& refresh,
                      const std::function<bool()>& acceptable)
{
  if (!canMoveClockSink(sink, destination)) {
    return false;
  }
  dbNet* origin = sink->getNet();
  try {
    // connect validates the destination before disconnecting the original net.
    // Do not call disconnect(): it also discards hierarchical connectivity.
    sink->connect(destination);
    refresh();
    if (acceptable()) {
      return true;
    }
  } catch (...) {
    sink->connect(origin);
    refresh();
    throw;
  }
  sink->connect(origin);
  refresh();
  return false;
}

std::optional<ClockBranch> clockBranch(dbInst* lcb, sta::dbNetwork* network)
{
  if (!bufferPorts(lcb, network)) {
    return std::nullopt;
  }
  dbNet* net = singleOutputNet(lcb);
  bool inverted = false;
  std::unordered_set<dbNet*> visited;
  while (net != nullptr && visited.insert(net).second) {
    dbITerm* driver = nullptr;
    int drivers = 0;
    for (dbITerm* iterm : net->getITerms()) {
      if (iterm->getIoType() == odb::dbIoType::OUTPUT) {
        driver = iterm;
        ++drivers;
      } else if (iterm->getIoType() != odb::dbIoType::INPUT) {
        return std::nullopt;
      }
    }
    for (odb::dbBTerm* bterm : net->getBTerms()) {
      if (bterm->getIoType() == odb::dbIoType::INPUT) {
        ++drivers;
      } else if (bterm->getIoType() != odb::dbIoType::OUTPUT) {
        return std::nullopt;
      }
    }
    if (drivers != 1) {
      return std::nullopt;
    }
    if (driver == nullptr) {
      return ClockBranch{.source = net, .inverted = inverted};
    }
    const auto ports = bufferPorts(driver->getInst(), network);
    if (!ports) {
      return ClockBranch{.source = net, .inverted = inverted};
    }
    inverted ^= ports->inverted;
    net = ports->input->getNet();
  }
  return std::nullopt;
}

}  // namespace wmk
