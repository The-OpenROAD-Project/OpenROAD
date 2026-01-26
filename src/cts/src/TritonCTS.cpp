// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "cts/TritonCTS.h"

#include <algorithm>
#include <cassert>
#include <cctype>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <fstream>
#include <functional>
#include <iostream>
#include <iterator>
#include <limits>
#include <map>
#include <memory>
#include <ranges>
#include <set>
#include <sstream>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

#include "Clock.h"
#include "CtsOptions.h"
#include "HTreeBuilder.h"
#include "LatencyBalancer.h"
#include "TechChar.h"
#include "TreeBuilder.h"
#include "db_sta/dbNetwork.hh"
#include "db_sta/dbSta.hh"
#include "est/EstimateParasitics.h"
#include "odb/db.h"
#include "odb/dbSet.h"
#include "odb/dbShape.h"
#include "odb/dbTypes.h"
#include "odb/geom.h"
#include "rsz/Resizer.hh"
#include "sta/Clock.hh"
#include "sta/Fuzzy.hh"
#include "sta/Graph.hh"
#include "sta/GraphDelayCalc.hh"
#include "sta/Liberty.hh"
#include "sta/Network.hh"
#include "sta/NetworkClass.hh"
#include "sta/PathAnalysisPt.hh"
#include "sta/PathEnd.hh"
#include "sta/PatternMatch.hh"
#include "sta/Sdc.hh"
#include "sta/Vector.hh"
#include "stt/SteinerTreeBuilder.h"
#include "utl/Logger.h"

namespace cts {

using utl::CTS;

TritonCTS::TritonCTS(utl::Logger* logger,
                     odb::dbDatabase* db,
                     sta::dbNetwork* network,
                     sta::dbSta* sta,
                     stt::SteinerTreeBuilder* st_builder,
                     rsz::Resizer* resizer,
                     est::EstimateParasitics* estimate_parasitics)
{
  logger_ = logger;
  db_ = db;
  network_ = network;
  openSta_ = sta;
  resizer_ = resizer;
  estimate_parasitics_ = estimate_parasitics;

  options_ = new CtsOptions(logger_, st_builder);
}

TritonCTS::~TritonCTS()
{
  delete options_;
}

void TritonCTS::runTritonCts()
{
  odb::dbChip* chip = db_->getChip();
  odb::dbBlock* block = chip->getBlock();
  options_->addOwner(block);

  setupCharacterization();
  findClockRoots();
  populateTritonCTS();
  if (builders_.empty()) {
    logger_->warn(CTS, 82, "No valid clock nets in the design.");
  } else {
    checkCharacterization();
    buildClockTrees();
    writeDataToDb();
    setAllClocksPropagated();
    if (options_->getRepairClockNets()) {
      repairClockNets();
    }
    balanceMacroRegisterLatencies();
  }

  // reset
  techChar_.reset();
  builders_.clear();
  staClockNets_.clear();
  visitedClockNets_.clear();
  inst2clkbuf_.clear();
  driver2subnet_.clear();
  numberOfClocks_ = 0;
  numClkNets_ = 0;
  numFixedNets_ = 0;
  dummyLoadIndex_ = 0;
  rootBuffers_.clear();
  sinkBuffers_.clear();
  regTreeRootBufIndex_ = 0;
  delayBufIndex_ = 0;
  options_->removeOwner();
}

TreeBuilder* TritonCTS::addBuilder(CtsOptions* options,
                                   Clock& net,
                                   odb::dbNet* topInputNet,
                                   TreeBuilder* parent,
                                   utl::Logger* logger,
                                   odb::dbDatabase* db)
{
  auto builder
      = std::make_unique<HTreeBuilder>(options, net, parent, logger, db);

  builder->setTopInputNet(topInputNet);
  builders_.emplace_back(std::move(builder));
  return builders_.back().get();
}

int TritonCTS::getBufferFanoutLimit(const std::string& bufferName)
{
  int fanout = std::numeric_limits<int>::max();
  float tempFanout;
  bool existMaxFanout;

  // Check if top instance has fanout limit
  sta::Cell* top_cell = network_->cell(network_->topInstance());
  openSta_->sdc()->fanoutLimit(
      top_cell, sta::MinMax::max(), tempFanout, existMaxFanout);
  if (existMaxFanout) {
    fanout = std::min(fanout, (int) tempFanout);
  }

  odb::dbMaster* bufferMaster = db_->findMaster(bufferName.c_str());
  sta::Cell* bufferCell = network_->dbToSta(bufferMaster);
  sta::Port* buffer_port = nullptr;
  for (odb::dbMTerm* mterm : bufferMaster->getMTerms()) {
    odb::dbSigType sig_type = mterm->getSigType();
    if (sig_type == odb::dbSigType::GROUND
        || sig_type == odb::dbSigType::POWER) {
      continue;
    }
    odb::dbIoType io_type = mterm->getIoType();
    if (io_type == odb::dbIoType::OUTPUT) {
      buffer_port = network_->dbToSta(mterm);
      break;
    }
  }
  if (buffer_port == nullptr) {
    return (existMaxFanout) ? fanout : 0;
  }

  openSta_->sdc()->fanoutLimit(
      buffer_port, sta::MinMax::max(), tempFanout, existMaxFanout);
  if (existMaxFanout) {
    fanout = std::min(fanout, (int) tempFanout);
  }

  openSta_->sdc()->fanoutLimit(
      bufferCell, sta::MinMax::max(), tempFanout, existMaxFanout);
  if (existMaxFanout) {
    fanout = std::min(fanout, (int) tempFanout);
  }

  sta::LibertyPort* port = network_->libertyPort(buffer_port);
  port->fanoutLimit(sta::MinMax::max(), tempFanout, existMaxFanout);
  if (existMaxFanout) {
    fanout = std::min(fanout, (int) tempFanout);
  } else {
    port->libertyLibrary()->defaultMaxFanout(tempFanout, existMaxFanout);
    if ((existMaxFanout)) {
      fanout = std::min(fanout, (int) tempFanout);
    }
  }
  return fanout == std::numeric_limits<int>::max() ? 0 : fanout;
}

void TritonCTS::setupCharacterization()
{
  // Check if CTS library is valid
  if (options_->isCtsLibrarySet()) {
    sta::Library* lib = network_->findLibrary(options_->getCtsLibrary());
    if (lib == nullptr) {
      logger_->error(CTS,
                     209,
                     "Library {} cannot be found because it is not "
                     "loaded or name is incorrect",
                     options_->getCtsLibrary());
    } else {
      logger_->info(CTS,
                    210,
                    "Clock buffers will be chosen from library {}",
                    options_->getCtsLibrary());
    }
  }

  block_ = db_->getChip()->getBlock();
  options_->setDbUnits(block_->getDbUnitsPerMicron());

  openSta_->checkFanoutLimitPreamble();
  // Finalize root/sink buffers
  std::string rootBuffer = selectRootBuffer(rootBuffers_);
  options_->setRootBuffer(rootBuffer);
  std::string sinkBuffer = selectSinkBuffer(sinkBuffers_);
  options_->setSinkBuffer(sinkBuffer);

  int sinkMaxFanout = getBufferFanoutLimit(sinkBuffer);
  int rootMaxFanout = getBufferFanoutLimit(rootBuffer);

  if (rootMaxFanout && (options_->getNumMaxLeafSinks() > rootMaxFanout)) {
    options_->setMaxFanout(rootMaxFanout);
  }

  if (sinkMaxFanout) {
    options_->limitSinkClusteringSizes(sinkMaxFanout);
    if (sinkMaxFanout < options_->getMaxFanout()) {
      options_->setMaxFanout(sinkMaxFanout);
    }
  }

  double maxWlMicrons
      = resizer_->findMaxWireLength(/* don't issue error */ false) * 1e+6;
  options_->setMaxWl(block_->micronsToDbu(maxWlMicrons));

  // A new characteriztion is always created.
  techChar_ = std::make_unique<TechChar>(options_,
                                         db_,
                                         openSta_,
                                         resizer_,
                                         estimate_parasitics_,
                                         network_,
                                         logger_);
  techChar_->create();

  // Also resets metrics everytime the setup is done
  options_->setNumSinks(0);
  options_->setNumBuffersInserted(0);
  options_->setNumClockRoots(0);
  options_->setNumClockSubnets(0);
}

void TritonCTS::checkCharacterization()
{
  std::unordered_set<std::string> visitedMasters;
  techChar_->forEachWireSegment([&](unsigned idx, const WireSegment& wireSeg) {
    for (int buf = 0; buf < wireSeg.getNumBuffers(); ++buf) {
      const std::string& master = wireSeg.getBufferMaster(buf);
      if (!visitedMasters.contains(master)) {
        if (masterExists(master)) {
          visitedMasters.insert(master);
        } else {
          logger_->error(CTS, 81, "Buffer {} is not in the loaded DB.", master);
        }
      }
    }
  });

  logger_->info(CTS,
                97,
                "Characterization used {} buffer(s) types.",
                visitedMasters.size());
}

void TritonCTS::findClockRoots()
{
  if (!options_->getClockNets().empty()) {
    logger_->info(CTS,
                  1,
                  "Running TritonCTS with user-specified clock roots: {}.",
                  options_->getClockNets());
  }
}

void TritonCTS::buildClockTrees()
{
  for (auto& builder : builders_) {
    builder->setTechChar(*techChar_);
    builder->setDb(db_);
    builder->setLogger(logger_);
    builder->initBlockages();
    builder->run();
  }
}

void TritonCTS::initOneClockTree(odb::dbNet* driverNet,
                                 odb::dbNet* clkInputNet,
                                 const std::string& sdcClockName,
                                 TreeBuilder* parent)
{
  TreeBuilder* clockBuilder = nullptr;
  std::vector<odb::dbNet*> skipNets = options_->getSkipNets();
  if (driverNet->isSpecial()) {
    logger_->info(
        CTS, 116, "Special net \"{}\" skipped.", driverNet->getName());
  } else if (std::ranges::find(skipNets, driverNet) != skipNets.end()) {
    logger_->warn(CTS,
                  44,
                  "Skipping net {}, specified by the user...",
                  driverNet->getName());
  } else {
    clockBuilder = initClock(driverNet, clkInputNet, sdcClockName, parent);
  }
  if (clockBuilder != nullptr && net2builder_[clkInputNet] == nullptr) {
    net2builder_[clkInputNet] = clockBuilder;
  }
  // Treat gated clocks as separate clock trees
  // TODO: include sinks from gated clocks together with other sinks and build
  // one clock tree
  visitedClockNets_.insert(driverNet);
  odb::dbITerm* driver = driverNet->getFirstOutput();
  odb::dbSet<odb::dbITerm> iterms = driverNet->getITerms();
  for (odb::dbITerm* iterm : iterms) {
    if (iterm != driver && iterm->isInputSignal()) {
      if (!isSink(iterm)) {
        odb::dbITerm* outputPin = getSingleOutput(iterm->getInst(), iterm);
        if (outputPin && outputPin->getNet()) {
          odb::dbNet* outputNet = outputPin->getNet();
          if (visitedClockNets_.find(outputNet) == visitedClockNets_.end()
              && !openSta_->sdc()->isLeafPinClock(
                  network_->dbToSta(outputPin))) {
            if (clockBuilder == nullptr
                && net2builder_[clkInputNet] != nullptr) {
              initOneClockTree(outputNet,
                               clkInputNet,
                               sdcClockName,
                               net2builder_[clkInputNet]);
            } else {
              initOneClockTree(
                  outputNet, clkInputNet, sdcClockName, clockBuilder);
            }
          }
        }
      }
    }
  }
}

void TritonCTS::countSinksPostDbWrite(
    TreeBuilder* builder,
    odb::dbNet* net,
    unsigned& sinks_cnt,
    unsigned& leafSinks,
    unsigned currWireLength,
    double& sinkWireLength,
    int& minDepth,
    int& maxDepth,
    int depth,
    bool fullTree,
    const std::unordered_set<odb::dbITerm*>& sinks,
    const std::unordered_set<odb::dbInst*>& dummies)
{
  odb::dbSet<odb::dbITerm> iterms = net->getITerms();
  int driverX = 0;
  int driverY = 0;
  for (odb::dbITerm* iterm : iterms) {
    if (iterm->getIoType() != odb::dbIoType::INPUT) {
      iterm->getAvgXY(&driverX, &driverY);
      break;
    }
  }
  odb::dbSet<odb::dbBTerm> bterms = net->getBTerms();
  for (odb::dbBTerm* bterm : bterms) {
    if (bterm->getIoType() == odb::dbIoType::INPUT) {
      for (odb::dbBPin* pin : bterm->getBPins()) {
        odb::dbPlacementStatus status = pin->getPlacementStatus();
        if (status == odb::dbPlacementStatus::NONE
            || status == odb::dbPlacementStatus::UNPLACED) {
          continue;
        }
        for (odb::dbBox* box : pin->getBoxes()) {
          if (box) {
            driverX = box->xMin();
            driverY = box->yMin();
            break;
          }
        }
        break;
      }
    }
  }
  for (odb::dbITerm* iterm : iterms) {
    if (iterm->getIoType() == odb::dbIoType::INPUT) {
      std::string name = iterm->getInst()->getName();
      int receiverX, receiverY;
      iterm->getAvgXY(&receiverX, &receiverY);
      unsigned dist = abs(driverX - receiverX) + abs(driverY - receiverY);
      odb::dbInst* inst = iterm->getInst();
      bool terminate = fullTree
                           ? (sinks.find(iterm) != sinks.end())
                           : !builder->isAnyTreeBuffer(getClockFromInst(inst));
      odb::dbITerm* outputPin = iterm->getInst()->getFirstOutput();
      bool trueSink = true;
      if (outputPin && outputPin->getNet() == net) {
        // Skip feedback loop.  When input pin and output pin are
        // connected to the same net this can lead to infinite recursion. For
        // example, some designs have Q pin connected to SI pin.
        terminate = true;
        trueSink = false;
      }

      if (!terminate && inst) {
        if (inst->isBlock()) {
          // Skip non-sink macro blocks
          terminate = true;
          trueSink = false;
        } else {
          sta::Cell* masterCell = network_->dbToSta(inst->getMaster());
          if (masterCell) {
            sta::LibertyCell* libCell = network_->libertyCell(masterCell);
            if (libCell) {
              if (libCell->hasSequentials()) {
                // Skip non-sink registers
                terminate = true;
                trueSink = false;
              }
            }
          }
        }
      }

      if (!terminate) {
        // ignore dummy buffer and inverters added to balance loads
        if (outputPin && outputPin->getNet() != nullptr) {
          countSinksPostDbWrite(builder,
                                outputPin->getNet(),
                                sinks_cnt,
                                leafSinks,
                                (currWireLength + dist),
                                sinkWireLength,
                                minDepth,
                                maxDepth,
                                depth + 1,
                                fullTree,
                                sinks,
                                dummies);
        } else {
          std::string cellType = "Complex cell";
          odb::dbInst* inst = iterm->getInst();
          sta::Cell* masterCell = network_->dbToSta(inst->getMaster());
          if (masterCell) {
            sta::LibertyCell* libCell = network_->libertyCell(masterCell);
            if (libCell) {
              if (libCell->isInverter()) {
                cellType = "Inverter";
              } else if (libCell->isBuffer()) {
                cellType = "Buffer";
              }
            }
          }

          if (dummies.find(inst) == dummies.end()) {
            logger_->info(CTS,
                          121,
                          "{} '{}' has unconnected output pin.",
                          cellType,
                          name);
          }
        }
        if (builder->isLeafBuffer(getClockFromInst(iterm->getInst()))) {
          leafSinks++;
        }
      } else if (trueSink) {
        sinks_cnt++;
        double currSinkWl
            = (dist + currWireLength) / double(options_->getDbUnits());
        sinkWireLength += currSinkWl;
        maxDepth = std::max(depth, maxDepth);
        if ((minDepth > 0 && depth < minDepth) || (minDepth == 0)) {
          minDepth = depth;
        }
      }
    }
  }  // ignoring block pins/feedthrus
}

ClockInst* TritonCTS::getClockFromInst(odb::dbInst* inst)
{
  auto it = inst2clkbuf_.find(inst);
  return it != inst2clkbuf_.end() ? it->second : nullptr;
}

void TritonCTS::writeDataToDb()
{
  std::set<odb::dbNet*> clkLeafNets;
  std::unordered_set<odb::dbInst*> clkDummies;

  for (auto& builder : builders_) {
    writeClockNetsToDb(builder.get(), clkLeafNets);
    if (options_->getApplyNdr() != CtsOptions::NdrStrategy::NONE) {
      writeClockNDRsToDb(builder.get());
    }
    if (options_->dummyLoadEnabled()) {
      int nDummies = writeDummyLoadsToDb(builder->getClock(), clkDummies);
      builder->setNDummies(nDummies);
    }
  }

  for (auto& builder : builders_) {
    odb::dbNet* topClockNet = builder->getClock().getNetObj();
    unsigned sinkCount = 0;
    unsigned leafSinks = 0;
    double allSinkDistance = 0.0;
    int minDepth = 0;
    int maxDepth = 0;
    bool reportFullTree
        = !builder->getParent() && !builder->getChildren().empty();

    std::unordered_set<odb::dbITerm*> sinks;
    builder->getClock().forEachSink([&sinks](const ClockInst& inst) {
      sinks.insert(inst.getDbInputPin());
    });
    if (sinks.size() < 2) {
      logger_->info(
          CTS, 124, "Clock net \"{}\"", builder->getClock().getName());
      logger_->info(CTS, 125, " Sinks {}", sinks.size());
    } else {
      countSinksPostDbWrite(builder.get(),
                            topClockNet,
                            sinkCount,
                            leafSinks,
                            0,
                            allSinkDistance,
                            minDepth,
                            maxDepth,
                            0,
                            reportFullTree,
                            sinks,
                            clkDummies);
      logger_->info(CTS, 98, "Clock net \"{}\"", builder->getClock().getName());
      logger_->info(CTS, 99, " Sinks {}", sinkCount);
      logger_->info(CTS, 100, " Leaf buffers {}", leafSinks);
      if (sinkCount > 0) {
        double avgWL = allSinkDistance / sinkCount;
        logger_->info(CTS, 101, " Average sink wire length {:.2f} um", avgWL);
      }
      logger_->info(CTS, 102, " Path depth {} - {}", minDepth, maxDepth);
      if (options_->dummyLoadEnabled()) {
        logger_->info(
            CTS, 207, " Dummy loads inserted {}", builder->getNDummies());
      }
    }
  }
}

void TritonCTS::forEachBuilder(
    const std::function<void(const TreeBuilder*)>& func) const
{
  for (const auto& builder : builders_) {
    func(builder.get());
  }
}

void TritonCTS::reportCtsMetrics()
{
  std::string filename = options_->getMetricsFile();

  if (!filename.empty()) {
    std::ofstream file(filename.c_str());

    if (!file.is_open()) {
      logger_->error(
          CTS, 87, "Could not open output metric file {}.", filename.c_str());
    }

    file << "Total number of Clock Roots: " << options_->getNumClockRoots()
         << ".\n";
    file << "Total number of Buffers Inserted: "
         << options_->getNumBuffersInserted() << ".\n";
    file << "Total number of Clock Subnets: " << options_->getNumClockSubnets()
         << ".\n";
    file << "Total number of Sinks: " << options_->getNumSinks() << ".\n";

    file << "Buffers used:\n";
    for (const auto& [master, count] : options_->getBufferCount()) {
      file << "  " << master->getName() << ": " << count << "\n";
    }
    if (!options_->getDummyCount().empty()) {
      file << "Dummys used:\n";
      for (const auto& [master, count] : options_->getDummyCount()) {
        file << "  " << master->getName() << ": " << count << "\n";
      }
    }
    file.close();

  } else {
    logger_->report("Total number of Clock Roots: {}.",
                    options_->getNumClockRoots());
    logger_->report("Total number of Buffers Inserted: {}.",
                    options_->getNumBuffersInserted());
    logger_->report("Total number of Clock Subnets: {}.",
                    options_->getNumClockSubnets());
    logger_->report("Total number of Sinks: {}.", options_->getNumSinks());

    logger_->report("Cells used:");
    for (const auto& [master, count] : options_->getBufferCount()) {
      logger_->report("  {}: {}", master->getName(), count);
    }
    if (!options_->getDummyCount().empty()) {
      logger_->report("Dummys used:");
      for (const auto& [master, count] : options_->getDummyCount()) {
        logger_->report("  {}: {}", master->getName(), count);
      }
    }
  }
}

int TritonCTS::setClockNets(const char* names)
{
  odb::dbChip* chip = db_->getChip();
  odb::dbBlock* block = chip->getBlock();

  options_->setClockNets(names);
  std::stringstream ss(names);
  std::istream_iterator<std::string> begin(ss);
  std::istream_iterator<std::string> end;
  std::vector<std::string> nets(begin, end);

  std::vector<odb::dbNet*> netObjects;

  for (const std::string& name : nets) {
    odb::dbNet* net = block->findNet(name.c_str());
    bool netFound = false;
    if (net != nullptr) {
      // Since a set is unique, only the nets not found by dbSta are added.
      netObjects.push_back(net);
      netFound = true;
    } else {
      // User input was a pin, transform it into an iterm if possible
      odb::dbITerm* iterm = block->findITerm(name.c_str());
      if (iterm != nullptr) {
        net = iterm->getNet();
        if (net != nullptr) {
          // Since a set is unique, only the nets not found by dbSta are added.
          netObjects.push_back(net);
          netFound = true;
        }
      }
    }
    if (!netFound) {
      return 1;
    }
  }
  options_->setClockNetsObjs(netObjects);
  return 0;
}

void TritonCTS::setBufferList(const char* buffers)
{
  // Put the buffer list into a string vector
  std::stringstream ss(buffers);
  std::istream_iterator<std::string> begin(ss);
  std::istream_iterator<std::string> end;
  std::vector<std::string> bufferList(begin, end);
  // If the vector is empty, then the buffers are inferred
  if (bufferList.empty()) {
    const char* lib_name
        = options_->isCtsLibrarySet() ? options_->getCtsLibrary() : nullptr;
    resizer_->inferClockBufferList(lib_name, bufferList);
    options_->setBufferListInferred(true);
  } else {
    // Iterate the user-defined buffer list
    sta::Vector<sta::LibertyCell*> selected_buffers;
    for (const std::string& buffer : bufferList) {
      odb::dbMaster* buffer_master = db_->findMaster(buffer.c_str());
      if (buffer_master == nullptr) {
        logger_->error(
            CTS, 126, "No physical master cell found for buffer {}.", buffer);
      } else {
        // Get the buffer and add to the vector
        sta::Cell* master_cell = network_->dbToSta(buffer_master);
        if (master_cell) {
          sta::LibertyCell* lib_cell = network_->libertyCell(master_cell);
          selected_buffers.push_back(lib_cell);
        }
      }
    }
    // Add found buffer to RSZ
    resizer_->setClockBuffersList(selected_buffers);
  }
  options_->setBufferList(bufferList);
}

std::string TritonCTS::getRootBufferToString()
{
  std::ostringstream buffer_names;
  for (const auto& buf : rootBuffers_) {
    buffer_names << buf << " ";
  }
  return buffer_names.str();
}

void TritonCTS::setRootBuffer(const char* buffers)
{
  std::stringstream ss(buffers);
  std::istream_iterator<std::string> begin(ss);
  std::istream_iterator<std::string> end;
  std::vector<std::string> bufferList(begin, end);
  for (const std::string& buffer : bufferList) {
    if (db_->findMaster(buffer.c_str()) == nullptr) {
      logger_->error(
          CTS, 127, "No physical master cell found for buffer {}.", buffer);
    }
  }
  rootBuffers_ = std::move(bufferList);
}

std::string TritonCTS::selectRootBuffer(std::vector<std::string>& buffers)
{
  // if -root_buf is not specified, choose from the buffer list
  if (buffers.empty()) {
    buffers = options_->getBufferList();
  }

  if (buffers.size() == 1) {
    return buffers.front();
  }

  options_->setRootBufferInferred(true);
  // estimate wire cap for root buffer
  // assume sink buffer needs to drive clk buffers at two far ends of chip
  // at midpoint
  //
  //  --------------
  //  |      .     |
  //  |   ===x===  |
  //  |      .     |
  //  --------------
  odb::dbBlock* block = db_->getChip()->getBlock();
  odb::Rect coreArea = block->getCoreArea();
  float sinkWireLength
      = static_cast<float>(std::max(coreArea.dx(), coreArea.dy()))
        / block->getDbUnitsPerMicron();
  sta::Corner* corner = openSta_->cmdCorner();
  float rootWireCap = estimate_parasitics_->wireSignalCapacitance(corner) * 1e-6
                      * sinkWireLength / 2.0;
  std::string rootBuf = selectBestMaxCapBuffer(buffers, rootWireCap);
  return rootBuf;
}

void TritonCTS::setSinkBuffer(const char* buffers)
{
  std::stringstream ss(buffers);
  std::istream_iterator<std::string> begin(ss);
  std::istream_iterator<std::string> end;
  std::vector<std::string> bufferList(begin, end);
  sinkBuffers_ = std::move(bufferList);
}

std::string TritonCTS::selectSinkBuffer(std::vector<std::string>& buffers)
{
  // if -sink_clustering_buf is not specified, choose from the buffer list
  if (buffers.empty()) {
    buffers = options_->getBufferList();
  }

  if (buffers.size() == 1) {
    return buffers.front();
  }

  options_->setSinkBufferInferred(true);
  // estimate wire cap for sink buffer
  // assume sink buffer needs to drive clk buffers at two far ends of chip
  // to account for unknown pin caps
  //
  //  --------------
  //  |======x=====|
  //  |      .     |
  //  |----- .-----|
  //  |      .     |
  //  --------------
  odb::dbBlock* block = db_->getChip()->getBlock();
  odb::Rect coreArea = block->getCoreArea();
  float sinkWireLength
      = static_cast<float>(std::max(coreArea.dx(), coreArea.dy()))
        / block->getDbUnitsPerMicron();
  sta::Corner* corner = openSta_->cmdCorner();
  float sinkWireCap = estimate_parasitics_->wireSignalCapacitance(corner) * 1e-6
                      * sinkWireLength;

  std::string sinkBuf = selectBestMaxCapBuffer(buffers, sinkWireCap);
  // clang-format off
  debugPrint(logger_, CTS, "buffering", 1, "{} has been selected as sink "
             "buffer to drive sink wire cap of {:0.2e}", sinkBuf, sinkWireCap);
  // clang-format on
  return sinkBuf;
}

// pick the smallest buffer that can drive total cap
// if no such buffer exists, pick one that has the largest max cap
std::string TritonCTS::selectBestMaxCapBuffer(
    const std::vector<std::string>& buffers,
    float totalCap)
{
  std::string bestBuf, nextBestBuf;
  float bestArea = std::numeric_limits<float>::max();
  float bestCap = 0.0;

  for (const std::string& name : buffers) {
    odb::dbMaster* master = db_->findMaster(name.c_str());
    if (master == nullptr) {
      logger_->error(
          CTS, 117, "Physical master could not be found for cell '{}'", name);
    }
    sta::Cell* masterCell = network_->dbToSta(master);
    sta::LibertyCell* libCell = network_->libertyCell(masterCell);
    if (libCell == nullptr) {
      logger_->error(
          CTS, 112, "Liberty cell could not be found for cell '{}'", name);
    }
    sta::LibertyPort *in, *out;
    libCell->bufferPorts(in, out);
    float area = libCell->area();
    float maxCap = 0.0;
    bool maxCapExists = false;
    out->capacitanceLimit(sta::MinMax::max(), maxCap, maxCapExists);
    // clang-format off
    debugPrint(logger_, CTS, "buffering", 1, "{} has cap limit:{}"
               " vs. total cap:{}, derate:{}", name,
               maxCap * (float) options_->getSinkBufferMaxCapDerate(), totalCap,
               options_->getSinkBufferMaxCapDerate());
    // clang-format on
    if (maxCapExists
        && ((maxCap * options_->getSinkBufferMaxCapDerate()) > totalCap)
        && area < bestArea) {
      bestBuf = name;
      bestArea = area;
    }
    if (maxCap > bestCap) {
      nextBestBuf = name;
      bestCap = maxCap;
    }
  }

  if (bestBuf.empty()) {
    bestBuf = std::move(nextBestBuf);
  }

  return bestBuf;
}

// db functions

void TritonCTS::cloneClockGaters(odb::dbNet* clkNet)
{
  odb::dbITerm* driver = clkNet->getFirstOutput();
  std::vector<int> xs;
  std::vector<int> ys;
  std::map<odb::Point, std::vector<odb::dbITerm*>> point2pin;
  odb::dbSet<odb::dbITerm> iterms = clkNet->getITerms();
  for (odb::dbITerm* iterm : iterms) {
    if (iterm != driver && iterm->isInputSignal()) {
      int TestX, TestY;
      iterm->getAvgXY(&TestX, &TestY);
      xs.push_back(TestX);
      ys.push_back(TestY);
      point2pin[{TestX, TestY}].push_back(iterm);
      if (isSink(iterm)) {
        continue;
      }
      odb::dbITerm* outputPin = getSingleOutput(iterm->getInst(), iterm);
      if (!outputPin || !outputPin->getNet()) {
        continue;
      }
      odb::dbInst* icg = iterm->getInst();
      odb::dbNet* outputNet = outputPin->getNet();
      sta::Cell* masterCell = network_->dbToSta(icg->getMaster());
      sta::LibertyCell* libertyCell = network_->libertyCell(masterCell);

      if (!libertyCell) {
        continue;
      }
      // Clock tree buffers or inverters
      if (libertyCell->isInverter() || libertyCell->isBuffer()) {
        continue;
      }
      cloneClockGaters(outputNet);
    }
  }
  if (!driver) {
    return;
  }

  // xs is empty if the fanout is a bterm
  if (isSink(driver) || driver->getInst()->isFixed()
      || driver->getInst()->isPad() || xs.empty()) {
    return;
  }

  int drvrX, drvrY;
  driver->getAvgXY(&drvrX, &drvrY);
  point2pin[{drvrX, drvrY}].push_back(driver);
  stt::Tree ftree
      = options_->getSttBuilder()->makeSteinerTree(clkNet, xs, ys, 0);
  findLongEdges(ftree, {drvrX, drvrY}, point2pin);
}

void TritonCTS::findLongEdges(
    stt::Tree& clkSteiner,
    odb::Point driverPt,
    std::map<odb::Point, std::vector<odb::dbITerm*>>& point2pin)
{
  const int threshold = options_->getMaxWl();
  debugPrint(
      logger_, CTS, "clock gate cloning", 1, "Threshold = {}", threshold);

  std::map<int, int> iterm2cluster;
  std::vector<std::vector<int>> clusters;
  odb::dbNet* icgNet = point2pin[driverPt][0]->getNet();
  odb::dbITerm* icgTerm = icgNet->getFirstOutput();
  std::string icgName = icgTerm->getInst()->getName();

  for (int b = 0; b < clkSteiner.branchCount(); b++) {
    const stt::Branch branch = clkSteiner.branch[b];
    const stt::Branch* neighbor = &clkSteiner.branch[branch.n];
    odb::Point branchPt = {branch.x, branch.y};

    odb::Point neighborPt = {neighbor->x, neighbor->y};
    int64_t dist = odb::Point::manhattanDistance(branchPt, neighborPt);
    const int clusterFrom
        = iterm2cluster.find(b) == iterm2cluster.end() ? -1 : iterm2cluster[b];
    const int clusterTo = iterm2cluster.find(branch.n) == iterm2cluster.end()
                              ? -1
                              : iterm2cluster[branch.n];

    if (b == branch.n) {
      continue;
    }

    if (dist >= threshold) {
      if (clusterFrom == -1) {
        int newClusterID = clusters.size();
        iterm2cluster[b] = newClusterID;
        clusters.push_back({b});
      }
      if (clusterTo == -1) {
        int newClusterID = clusters.size();
        iterm2cluster[branch.n] = newClusterID;
        clusters.push_back({branch.n});
      }
      continue;
    }

    if (clusterFrom != -1 && clusterTo != -1) {
      int mantainedCLuster
          = (clusters[clusterFrom].size() >= clusters[clusterTo].size())
                ? clusterFrom
                : clusterTo;
      int removedCLuster
          = (clusters[clusterFrom].size() < clusters[clusterTo].size())
                ? clusterFrom
                : clusterTo;

      clusters[mantainedCLuster].insert(clusters[mantainedCLuster].end(),
                                        clusters[removedCLuster].begin(),
                                        clusters[removedCLuster].end());
      for (int point : clusters[removedCLuster]) {
        iterm2cluster[point] = mantainedCLuster;
      }
      clusters[removedCLuster].clear();

    } else if (clusterFrom != -1) {
      iterm2cluster[branch.n] = clusterFrom;
      clusters[clusterFrom].push_back(branch.n);
    } else if (clusterTo != -1) {
      iterm2cluster[b] = clusterTo;
      clusters[clusterTo].push_back(b);
    } else {
      int newClusterID = clusters.size();
      iterm2cluster[b] = newClusterID;
      iterm2cluster[branch.n] = newClusterID;
      clusters.push_back({b, branch.n});
    }
  }

  // Find closest cluster to original ICG
  int driverClusterID = -1;
  int64_t minDist2Driver = std::numeric_limits<int64_t>::max();
  int validClusters = 0;
  for (int n = 0; n < clusters.size(); n++) {
    const std::vector<int>& cluster = clusters[n];
    if (cluster.empty()) {
      continue;
    }
    bool validCluster = false;
    odb::Rect sinksBbox = odb::Rect();
    sinksBbox.mergeInit();
    for (int branch : cluster) {
      odb::Point branchPt
          = {clkSteiner.branch[branch].x, clkSteiner.branch[branch].y};
      for (auto sink : point2pin[branchPt]) {
        if (!sink->isInputSignal()) {
          continue;
        }
        validCluster = true;
        int sinkX, sinkY;
        sink->getAvgXY(&sinkX, &sinkY);
        sinksBbox.merge({sinkX, sinkY});
      }
    }
    if (validCluster) {
      validClusters += 1;
      int64_t dist2Driver
          = odb::Point::manhattanDistance(sinksBbox.center(), driverPt);
      if (dist2Driver < minDist2Driver) {
        driverClusterID = n;
        minDist2Driver = dist2Driver;
      }
    }
  }

  debugPrint(logger_,
             CTS,
             "clock gate cloning",
             1,
             "Found {} clusters",
             validClusters);

  // Insert original ICG to its closest cluster, create clones to drive the
  // other clusters
  int nClones = 0;
  // hierarchy fix, make the clone net in the right scope
  sta::Pin* driver = nullptr;
  odb::dbModule* module
      = network_->getNetDriverParentModule(network_->dbToSta(icgNet), driver);
  if (module == nullptr) {
    // if none put in top level
    module = block_->getTopModule();
  }
  sta::Instance* scope
      = (module == nullptr || (module == block_->getTopModule()))
            ? network_->topInstance()
            : (sta::Instance*) (module->getModInst());

  for (int n = 0; n < clusters.size(); n++) {
    const std::vector<int>& cluster = clusters[n];
    if (cluster.empty()) {
      continue;
    }
    odb::dbInst* clone = nullptr;
    odb::dbNet* cloneNet = nullptr;
    bool disconectNets = true;
    odb::Rect sinksBbox = odb::Rect();
    sinksBbox.mergeInit();
    if (driverClusterID == n) {
      cloneNet = icgNet;
      clone = icgTerm->getInst();
      disconectNets = false;
      debugPrint(logger_,
                 CTS,
                 "clock gate cloning",
                 1,
                 "Original cell {}",
                 clone->getName());
      debugPrint(logger_,
                 CTS,
                 "clock gate cloning",
                 2,
                 " Original net {}",
                 cloneNet->getName());
    } else {
      // Create the ICG clone
      // Create a new input net
      std::string newNetName
          = "clonenet_" + std::to_string(++nClones) + "_" + icgNet->getName();

      cloneNet = network_->staToDb(network_->makeNet(
          newNetName.c_str(), scope, odb::dbNameUniquifyType::IF_NEEDED));

      cloneNet->setSigType(odb::dbSigType::CLOCK);

      staClockNets_.insert(cloneNet);

      // Create a new clone instance
      std::string newBufName
          = "clone_" + std::to_string(nClones) + "_" + icgName;
      odb::dbMaster* master = icgTerm->getInst()->getMaster();

      // fix: make buffer in same hierarchical module as driver
      clone = odb::dbInst::create(
          block_, master, newBufName.c_str(), false, module);

      clone->setSourceType(odb::dbSourceType::TIMING);

      debugPrint(logger_,
                 CTS,
                 "clock gate cloning",
                 1,
                 "Creating clone {} from {}",
                 newBufName,
                 icgName);
      debugPrint(logger_,
                 CTS,
                 "clock gate cloning",
                 2,
                 " New clone net {}",
                 cloneNet->getName());

      // Connect clone pins to same input nets as parent and new output net
      for (odb::dbITerm* iterm : clone->getITerms()) {
        if (iterm->isInputSignal()) {
          odb::dbITerm* parentITerm = icgTerm->getInst()->findITerm(
              iterm->getMTerm()->getName().c_str());
          odb::dbNet* parentNet = parentITerm->getNet();
          odb::dbModNet* parentModNet
              = network_->hierNet(network_->dbToSta(parentITerm));
          if (parentNet) {
            iterm->connect(parentNet);
            if (parentModNet) {
              iterm->connect(parentModNet);
            }
          }
        } else if (iterm->isOutputSignal()) {
          iterm->connect(cloneNet);
        }
      }
    }

    // Compute cluster center
    for (int branch : cluster) {
      odb::Point branchPt
          = {clkSteiner.branch[branch].x, clkSteiner.branch[branch].y};
      for (auto sink : point2pin[branchPt]) {
        if (!sink->isInputSignal()) {
          continue;
        }
        debugPrint(logger_,
                   CTS,
                   "clock gate cloning",
                   2,
                   "  Connects sink {}",
                   sink->getName());
        int sinkX, sinkY;
        sink->getAvgXY(&sinkX, &sinkY);
        sinksBbox.merge({sinkX, sinkY});
        if (disconectNets) {
          // Connect sinks to new clone instance
          sink->disconnect();
          sink->connect(cloneNet);
          sta::Pin* sinkPin = network_->dbToSta(sink);
          sta::Instance* sinkParentInst
              = network_->getOwningInstanceParent(sinkPin);
          if (sinkParentInst != scope) {
            network_->hierarchicalConnect(
                clone->getFirstOutput(), sink, cloneNet->getName().c_str());
          }
        }
      }
    }

    // Move ICG (clone or original) to the center of its sinks
    clone->setLocation(sinksBbox.xCenter(), sinksBbox.yCenter());
    clone->setPlacementStatus(odb::dbPlacementStatus::PLACED);
  }
  debugPrint(
      logger_, CTS, "clock gate cloning", 1, "Created {} clones", nClones);
}

void TritonCTS::populateTritonCTS()
{
  clearNumClocks();

  // Use dbSta to find all clock nets in the design.
  std::vector<std::pair<std::set<odb::dbNet*>, std::string>> clockNetsInfo;

  // Checks the user input in case there are other nets that need to be added to
  // the set.
  std::vector<odb::dbNet*> inputClkNets = options_->getClockNetsObjs();

  std::set<odb::dbNet*> allClkNets;
  if (!inputClkNets.empty()) {
    std::set<odb::dbNet*> clockNets;
    for (odb::dbNet* net : inputClkNets) {
      // Since a set is unique, only the nets not found by dbSta are added.
      clockNets.insert(net);
    }
    allClkNets.insert(clockNets.begin(), clockNets.end());
    clockNetsInfo.emplace_back(clockNets, "");
  } else {
    staClockNets_ = openSta_->findClkNets();
    sta::Sdc* sdc = openSta_->sdc();
    for (auto clk : *sdc->clocks()) {
      std::string clkName = clk->name();
      std::set<odb::dbNet*> clkNets;
      findClockRoots(clk, clkNets);
      for (auto net : clkNets) {
        if (allClkNets.find(net) != allClkNets.end()) {
          logger_->error(
              CTS, 114, "Clock {} overlaps a previous clock.", clkName);
        }
      }
      clockNetsInfo.emplace_back(clkNets, clkName);
      allClkNets.insert(clkNets.begin(), clkNets.end());
    }
  }
  // Iterate over all the nets found by the user-input and dbSta
  for (const auto& clockInfo : clockNetsInfo) {
    std::set<odb::dbNet*> clockNets = clockInfo.first;
    std::string clkName = clockInfo.second;
    for (odb::dbNet* net : clockNets) {
      if (net != nullptr) {
        cloneClockGaters(net);
        if (clkName.empty()) {
          logger_->info(CTS, 95, "Net \"{}\" found.", net->getName());
        } else {
          logger_->info(CTS,
                        7,
                        "Net \"{}\" found for clock \"{}\".",
                        net->getName(),
                        clkName);
        }
        // Initializes the net in TritonCTS. If the number of sinks is less than
        // 2, the net is discarded.
        if (visitedClockNets_.find(net) == visitedClockNets_.end()) {
          initOneClockTree(net, net, clkName, nullptr);
        }
      } else {
        logger_->warn(
            CTS,
            40,
            "Net was not found in the design for {}, please check. Skipping...",
            clkName);
      }
    }
  }

  if (getNumClocks() == 0) {
    logger_->warn(CTS, 83, "No clock nets have been found.");
  }

  logger_->info(CTS, 8, "TritonCTS found {} clock nets.", getNumClocks());
  options_->setNumClockRoots(getNumClocks());
}

TreeBuilder* TritonCTS::initClock(odb::dbNet* firstNet,
                                  odb::dbNet* clkInputNet,
                                  const std::string& sdcClock,
                                  TreeBuilder* parentBuilder)
{
  std::string driver;
  odb::dbITerm* iterm = firstNet->getFirstOutput();
  int xPin, yPin;
  if (iterm == nullptr) {
    odb::dbBTerm* bterm = firstNet->get1stBTerm();  // Clock pin
    if (bterm == nullptr) {
      logger_->info(
          CTS,
          122,
          "Clock net \"{}\" is skipped for CTS because it is not "
          "connected to any output instance pin or input block terminal.",
          firstNet->getName());
      return nullptr;
    }
    driver = bterm->getConstName();
    bterm->getFirstPinLocation(xPin, yPin);
  } else {
    odb::dbInst* inst = iterm->getInst();
    odb::dbMTerm* mterm = iterm->getMTerm();
    driver = std::string(inst->getConstName()) + "/"
             + std::string(mterm->getConstName());
    int xTmp, yTmp;
    computeITermPosition(iterm, xTmp, yTmp);
    xPin = xTmp;
    yPin = yTmp;
  }

  // Initialize clock net
  Clock clockNet(firstNet->getConstName(), driver, sdcClock, xPin, yPin);
  clockNet.setDriverPin(iterm);

  // Build a set of all the clock buffers' masters
  std::unordered_set<odb::dbMaster*> buffer_masters;
  for (const std::string& name : options_->getBufferList()) {
    auto master = db_->findMaster(name.c_str());
    if (master) {
      buffer_masters.insert(master);
    }
  }

  // Add the root buffer
  {
    const std::string& name = options_->getRootBuffer();
    auto master = db_->findMaster(name.c_str());
    if (master) {
      buffer_masters.insert(master);
    }
  }

  // Build a clock tree to drive macro cells with insertion delays
  // separated from registers or leaves without insertion delays
  TreeBuilder* builder = initClockTreeForMacrosAndRegs(
      firstNet, clkInputNet, buffer_masters, clockNet, parentBuilder);
  return builder;
}

// Build a separate clock tree to pull macro cells with insertion delays
// ahead of cells without insertion delays.  If sinks consist of
// both macros and FFs, clock tree for macros is built first.  A new net and a
// new buffer are created to drive cells without insertion delays.   New
// buffer will be sized later based on macro cell insertion delays.
//
//                |----|>----[] cells with insertion delays
//       firstNet |
//            |   |----|>----[]
//            v   |
//   [root]-------|                  |---|>----[] cells without insertion
//                |
//                |----|>------------|
//                      ^        ^   |
//                      |        |   |
//               new buffer secondNet|---|>----[]
//
TreeBuilder* TritonCTS::initClockTreeForMacrosAndRegs(
    odb::dbNet*& firstNet,
    odb::dbNet* clkInputNet,
    const std::unordered_set<odb::dbMaster*>& buffer_masters,
    Clock& clockNet,
    TreeBuilder* parentBuilder)
{
  // Separate sinks into two buckets: one with insertion delays and another
  // without
  std::vector<std::pair<odb::dbInst*, odb::dbMTerm*>> macroSinks;
  std::vector<std::pair<odb::dbInst*, odb::dbMTerm*>> registerSinks;
  if (!separateMacroRegSinks(
          firstNet, clockNet, buffer_masters, registerSinks, macroSinks)) {
    return nullptr;
  }

  if (!options_->insertionDelayEnabled() || macroSinks.empty()
      || registerSinks.empty()) {
    // There is no need for separate clock trees
    for (odb::dbITerm* iterm : firstNet->getITerms()) {
      odb::dbInst* inst = iterm->getInst();
      if (iterm->isInputSignal() && inst->isPlaced()) {
        odb::dbMTerm* mterm = iterm->getMTerm();
        std::string name = std::string(inst->getConstName()) + "/"
                           + std::string(mterm->getConstName());
        int x, y;
        computeITermPosition(iterm, x, y);
        float insDelay = computeInsertionDelay(name, inst, mterm);
        clockNet.addSink(name, x, y, iterm, getInputPinCap(iterm), insDelay);
      }
    }
    if (clockNet.getNumSinks() < 2) {
      logger_->warn(CTS,
                    41,
                    "Net \"{}\" has {} sinks. Skipping...",
                    clockNet.getName(),
                    clockNet.getNumSinks());
      return nullptr;
    }
    logger_->info(CTS,
                  10,
                  " Clock net \"{}\" has {} sinks.",
                  firstNet->getConstName(),
                  clockNet.getNumSinks());
    int totalSinks = options_->getNumSinks() + clockNet.getNumSinks();
    options_->setNumSinks(totalSinks);
    incrementNumClocks();
    clockNet.setNetObj(firstNet);
    return addBuilder(
        options_, clockNet, clkInputNet, parentBuilder, logger_, db_);
  }

  // add macro sinks to existing firstNet
  TreeBuilder* firstBuilder = addClockSinks(
      clockNet, clkInputNet, firstNet, macroSinks, parentBuilder, "macros");
  if (firstBuilder) {
    firstBuilder->setTreeType(TreeType::MacroTree);
  }

  // create a new net 'secondNet' to drive register sinks
  odb::dbNet* secondNet;
  std::string topBufferName;
  Clock clockNet2 = forkRegisterClockNetwork(
      clockNet, registerSinks, firstNet, secondNet, topBufferName);

  // add register sinks to secondNet
  TreeBuilder* secondBuilder
      = addClockSinks(clockNet2,
                      clkInputNet,
                      secondNet,
                      registerSinks,
                      firstBuilder ? firstBuilder : parentBuilder,
                      "registers");
  if (secondBuilder) {
    secondBuilder->setTreeType(TreeType::RegisterTree);
    secondBuilder->setTopBufferName(std::move(topBufferName));
    secondBuilder->setDrivingNet(firstNet);
  }

  return firstBuilder;
}

// Separate sinks into registers (no insertion delay) and macros (insertion
// delay)
bool TritonCTS::separateMacroRegSinks(
    odb::dbNet*& net,
    Clock& clockNet,
    const std::unordered_set<odb::dbMaster*>& buffer_masters,
    std::vector<std::pair<odb::dbInst*, odb::dbMTerm*>>& registerSinks,
    std::vector<std::pair<odb::dbInst*, odb::dbMTerm*>>& macroSinks)
{
  for (odb::dbITerm* iterm : net->getITerms()) {
    odb::dbInst* inst = iterm->getInst();

    if (buffer_masters.find(inst->getMaster()) != buffer_masters.end()
        && inst->getSourceType() == odb::dbSourceType::TIMING) {
      logger_->warn(CTS,
                    105,
                    "Net \"{}\" already has clock buffer {}. Skipping...",
                    clockNet.getName(),
                    inst->getName());
      return false;
    }

    if (iterm->isInputSignal() && inst->isPlaced()) {
      // Cells with insertion delay, macros, clock gaters and inverters that
      // drive macros are put in the macro sinks.
      odb::dbMTerm* mterm = iterm->getMTerm();

      bool nonSinkMacro = !isSink(iterm);
      sta::Cell* masterCell = network_->dbToSta(mterm->getMaster());
      sta::LibertyCell* libertyCell = network_->libertyCell(masterCell);
      if (libertyCell && libertyCell->isInverter()) {
        odb::dbITerm* invertedTerm
            = inst->getFirstOutput()->getNet()->get1stSignalInput(false);
        nonSinkMacro &= invertedTerm->getInst()->isBlock();
      }

      if (hasInsertionDelay(inst, mterm) || nonSinkMacro || inst->isBlock()) {
        macroSinks.emplace_back(inst, mterm);
      } else {
        registerSinks.emplace_back(inst, mterm);
      }
    }
  }

  return true;
}

TreeBuilder* TritonCTS::addClockSinks(
    Clock& clockNet,
    odb::dbNet* topInputNet,
    odb::dbNet* physicalNet,
    const std::vector<std::pair<odb::dbInst*, odb::dbMTerm*>>& sinks,
    TreeBuilder* parentBuilder,
    const std::string& macrosOrRegs)
{
  for (auto elem : sinks) {
    odb::dbInst* inst = elem.first;
    odb::dbMTerm* mterm = elem.second;
    std::string name = std::string(inst->getConstName()) + "/"
                       + std::string(mterm->getConstName());
    int x, y;
    odb::dbITerm* iterm = inst->getITerm(mterm);
    computeITermPosition(iterm, x, y);
    float insDelay = computeInsertionDelay(name, inst, mterm);
    clockNet.addSink(name, x, y, iterm, getInputPinCap(iterm), insDelay);
  }
  logger_->info(CTS,
                11,
                " Clock net \"{}\" for {} has {} sinks.",
                physicalNet->getConstName(),
                macrosOrRegs,
                clockNet.getNumSinks());
  int totalSinks = options_->getNumSinks() + clockNet.getNumSinks();
  options_->setNumSinks(totalSinks);
  incrementNumClocks();
  clockNet.setNetObj(physicalNet);
  return addBuilder(
      options_, clockNet, topInputNet, parentBuilder, logger_, db_);
}

Clock TritonCTS::forkRegisterClockNetwork(
    Clock& clockNet,
    const std::vector<std::pair<odb::dbInst*, odb::dbMTerm*>>& registerSinks,
    odb::dbNet*& firstNet,
    odb::dbNet*& secondNet,
    std::string& topBufferName)
{
  // create a new clock net to drive register sinks
  std::string newClockName = clockNet.getName() + "_regs";
  secondNet = odb::dbNet::create(block_, newClockName.c_str());
  secondNet->setSigType(odb::dbSigType::CLOCK);

  sta::Pin* first_pin_driver = nullptr;
  odb::dbModule* first_net_module = network_->getNetDriverParentModule(
      network_->dbToSta(firstNet), first_pin_driver);
  (void) first_pin_driver;
  sta::Pin* second_pin_driver = nullptr;
  odb::dbModule* second_net_module = network_->getNetDriverParentModule(
      network_->dbToSta(secondNet), second_pin_driver);
  (void) second_pin_driver;
  odb::dbModule* target_module = nullptr;
  if ((first_net_module != nullptr)
      && (first_net_module == second_net_module)) {
    target_module = first_net_module;
  }

  // move register sinks from previous clock net to new clock net
  for (auto elem : registerSinks) {
    odb::dbInst* inst = elem.first;
    odb::dbMTerm* mterm = elem.second;
    odb::dbITerm* iterm = inst->getITerm(mterm);
    iterm->disconnect();
    iterm->connect(secondNet);
  }

  // create a new clock buffer
  odb::dbMaster* master = db_->findMaster(options_->getRootBuffer().c_str());
  topBufferName = "clkbuf_regs_" + std::to_string(regTreeRootBufIndex_++) + "_"
                  + clockNet.getSdcName();
  odb::dbInst* clockBuf = odb::dbInst::create(
      block_, master, topBufferName.c_str(), false, target_module);

  // place new clock buffer near center of mass for registers
  odb::Rect bbox = secondNet->getTermBBox();
  clockBuf->setSourceType(odb::dbSourceType::TIMING);
  clockBuf->setLocation(bbox.xCenter(), bbox.yCenter());
  clockBuf->setPlacementStatus(odb::dbPlacementStatus::PLACED);

  // connect root buffer to clock net
  odb::dbITerm* inputTerm = getFirstInput(clockBuf);
  odb::dbITerm* outputTerm = clockBuf->getFirstOutput();
  inputTerm->connect(firstNet);
  outputTerm->connect(secondNet);

  // initialize new clock net
  std::string driver = std::string(clockBuf->getConstName()) + "/"
                       + std::string(outputTerm->getMTerm()->getConstName());
  int xPin, yPin;
  computeITermPosition(outputTerm, xPin, yPin);
  Clock clockNet2(
      secondNet->getConstName(), driver, clockNet.getSdcName(), xPin, yPin);
  clockNet2.setDriverPin(outputTerm);

  return clockNet2;
}

void TritonCTS::computeITermPosition(odb::dbITerm* term, int& x, int& y) const
{
  odb::dbITermShapeItr itr;

  odb::dbShape shape;
  x = 0;
  y = 0;
  unsigned numShapes = 0;
  for (itr.begin(term); itr.next(shape);) {
    if (!shape.isVia()) {
      x += shape.xMin() + (shape.xMax() - shape.xMin()) / 2;
      y += shape.yMin() + (shape.yMax() - shape.yMin()) / 2;
      ++numShapes;
    }
  }
  if (numShapes > 0) {
    x /= numShapes;
    y /= numShapes;
  }
};

void TritonCTS::destroyClockModNet(sta::Pin* pin_driver)
{
  if (pin_driver == nullptr || network_->hasHierarchy() == false) {
    return;
  }

  odb::dbModNet* mod_net = network_->hierNet(pin_driver);
  if (mod_net) {
    odb::dbModNet::destroy(mod_net);
  }
}

void TritonCTS::writeClockNetsToDb(TreeBuilder* builder,
                                   std::set<odb::dbNet*>& clkLeafNets)
{
  Clock& clockNet = builder->getClock();
  odb::dbNet* topClockNet = clockNet.getNetObj();
  // gets the module for the driver for the net
  sta::Pin* pin_driver = nullptr;
  odb::dbModule* top_module = network_->getNetDriverParentModule(
      network_->dbToSta(topClockNet), pin_driver);
  (void) pin_driver;

  disconnectAllSinksFromNet(topClockNet);

  // If exists, remove the dangling dbModNet related to the topClockNet because
  // topClockNet has no load pin now.
  // After CTS, the driver pin will drive only a few of root clock buffers.
  // So the hierarchical net (dbModNet) is not needed any more.
  destroyClockModNet(pin_driver);

  // re-connect top buffer that separates macros from registers
  if (builder->getTreeType() == TreeType::RegisterTree) {
    odb::dbInst* topRegBuffer
        = block_->findInst(builder->getTopBufferName().c_str());
    if (topRegBuffer) {
      odb::dbITerm* topRegBufferInputPin = getFirstInput(topRegBuffer);
      topRegBufferInputPin->connect(builder->getDrivingNet());
    }
  }

  createClockBuffers(clockNet, top_module);

  // connect top buffer on the clock pin
  std::string topClockInstName = "clkbuf_0_" + clockNet.getName();
  odb::dbInst* topClockInst = block_->findInst(topClockInstName.c_str());
  odb::dbITerm* topClockInstInputPin = getFirstInput(topClockInst);
  topClockInstInputPin->connect(topClockNet);
  topClockNet->setSigType(odb::dbSigType::CLOCK);

  std::map<int, int> fanoutcount;

  // create subNets
  numClkNets_ = 0;
  numFixedNets_ = 0;
  ClockSubNet* rootSubNet = nullptr;
  std::set<ClockInst*> removedSinks;
  clockNet.forEachSubNet([&](ClockSubNet& subNet) {
    bool outputPinFound = true;
    bool inputPinFound = true;
    bool leafLevelNet = subNet.isLeafLevel();
    if (("clknet_0_" + clockNet.getName()) == subNet.getName()) {
      rootSubNet = &subNet;
    }
    odb::dbNet* clkSubNet
        = odb::dbNet::create(block_, subNet.getName().c_str());
    subNet.setNetObj(clkSubNet);

    ++numClkNets_;
    clkSubNet->setSigType(odb::dbSigType::CLOCK);

    odb::dbInst* driver = subNet.getDriver()->getDbInst();
    odb::dbITerm* driverInputPin = getFirstInput(driver);
    odb::dbNet* inputNet = driverInputPin->getNet();
    odb::dbITerm* outputPin = driver->getFirstOutput();
    if (outputPin == nullptr) {
      outputPinFound = false;
    }
    if (outputPinFound) {
      outputPin->connect(clkSubNet);
    }

    if (subNet.getNumSinks() == 0) {
      inputPinFound = false;
    }

    subNet.forEachSink([&](ClockInst* inst) {
      odb::dbITerm* inputPin = nullptr;
      if (inst->isClockBuffer()) {
        odb::dbInst* sink = inst->getDbInst();
        inputPin = getFirstInput(sink);
      } else {
        inputPin = inst->getDbInputPin();
      }
      if (inputPin == nullptr) {
        inputPinFound = false;
      } else {
        if (!inputPin->getInst()->isPlaced()) {
          inputPinFound = false;
        }
      }

      if (inputPinFound) {
        inputPin->connect(clkSubNet);
        // get module for input pin
        // resolve connection in hierarchy
        if (network_->hasHierarchy()) {
          network_->hierarchicalConnect(
              outputPin, inputPin, clkSubNet->getName().c_str());
        }
      }
    });

    if (leafLevelNet) {
      // Report fanout values only for sink nets
      if (fanoutcount.find(subNet.getNumSinks()) == fanoutcount.end()) {
        fanoutcount[subNet.getNumSinks()] = 0;
      }
      fanoutcount[subNet.getNumSinks()] = fanoutcount[subNet.getNumSinks()] + 1;
      clkLeafNets.insert(clkSubNet);
    }

    if (!inputPinFound || !outputPinFound) {
      // Net not fully connected. Removing it.
      disconnectAllPinsFromNet(clkSubNet);
      odb::dbNet::destroy(clkSubNet);
      ++numFixedNets_;
      --numClkNets_;
      odb::dbInst::destroy(driver);
      removedSinks.insert(subNet.getDriver());
      checkUpstreamConnections(inputNet);
    }
  });

  if (!rootSubNet) {
    logger_->error(
        CTS, 85, "Could not find the root of {}", clockNet.getName());
  }

  int minPath = std::numeric_limits<int>::max();
  int maxPath = std::numeric_limits<int>::min();
  rootSubNet->forEachSink([&](ClockInst* inst) {
    // skip removed sinks
    if (removedSinks.find(inst) == removedSinks.end()) {
      if (inst->isClockBuffer()) {
        std::pair<int, int> resultsForBranch
            = branchBufferCount(inst, 1, clockNet);
        minPath = std::min(resultsForBranch.first, minPath);
        maxPath = std::max(resultsForBranch.second, maxPath);
      }
    } else {
      rootSubNet->removeSinks(removedSinks);
    }
  });

  logger_->info(
      CTS, 12, "    Minimum number of buffers in the clock path: {}.", minPath);
  logger_->info(
      CTS, 13, "    Maximum number of buffers in the clock path: {}.", maxPath);

  if (numFixedNets_ > 0) {
    logger_->info(
        CTS, 14, "    {} clock nets were removed/fixed.", numFixedNets_);
  }

  logger_->info(CTS, 15, "    Created {} clock nets.", numClkNets_);
  int totalNets = options_->getNumClockSubnets() + numClkNets_;
  options_->setNumClockSubnets(totalNets);

  std::string fanout;
  for (auto const& x : fanoutcount) {
    fanout += std::to_string(x.first) + ':' + std::to_string(x.second) + ", ";
  }

  logger_->info(CTS,
                16,
                "    Fanout distribution for the current clock = {}.",
                fanout.substr(0, fanout.size() - 2) + ".");
  logger_->info(
      CTS, 17, "    Max level of the clock tree: {}.", clockNet.getMaxLevel());
}

// Utility function to get all unique clock tree levels
std::vector<int> TritonCTS::getAllClockTreeLevels(Clock& clockNet)
{
  std::set<int> uniqueLevels;

  clockNet.forEachSubNet([&](ClockSubNet& subNet) {
    if (!subNet.isLeafLevel() && subNet.getTreeLevel() != -1) {
      uniqueLevels.insert(subNet.getTreeLevel());
    }
  });

  return std::vector<int>(uniqueLevels.begin(), uniqueLevels.end());
}

// Function to apply NDR to specific clock tree levels and return the number of
// NDR applied nets
int TritonCTS::applyNDRToClockLevels(Clock& clockNet,
                                     odb::dbTechNonDefaultRule* clockNDR,
                                     const std::vector<int>& targetLevels)
{
  int ndrAppliedNets = 0;

  debugPrint(
      logger_, CTS, "clustering", 1, "Applying NDR to clock tree levels: ");
  for (int level : targetLevels) {
    debugPrint(logger_, CTS, "clustering", 1, "{} ", level);
  }

  // Check if the main clock net (level 0) is in the level list
  if (std::ranges::find(targetLevels, 0) != targetLevels.end()) {
    odb::dbNet* clk_net = clockNet.getNetObj();
    clk_net->setNonDefaultRule(clockNDR);
    ndrAppliedNets++;
    // clang-format off
    debugPrint(logger_, CTS, "clustering", 1,
        "Applied NDR to: {} (level {})", clockNet.getName(), 0);
    // clang-format on
  }

  // Check clock sub nets list and apply NDR if level matches
  clockNet.forEachSubNet([&](ClockSubNet& subNet) {
    int level = subNet.getTreeLevel();
    if (std::ranges::find(targetLevels, level) != targetLevels.end()) {
      odb::dbNet* net = subNet.getNetObj();
      if (!subNet.isLeafLevel()) {
        net->setNonDefaultRule(clockNDR);
        ndrAppliedNets++;
        std::string net_name = net->getName();
        // clang-format off
        debugPrint(logger_, CTS, "clustering", 1,
            "Applied NDR to: {} (level {})", net_name, level);
        // clang-format on
      }
    }
  });

  return ndrAppliedNets;
}

// Alternative function to apply NDR to a range of clock tree levels
int TritonCTS::applyNDRToClockLevelRange(Clock& clockNet,
                                         odb::dbTechNonDefaultRule* clockNDR,
                                         const int minLevel,
                                         const int maxLevel)
{
  std::vector<int> targetLevels;
  for (int i = minLevel; i <= maxLevel; i++) {
    targetLevels.push_back(i);
  }

  return applyNDRToClockLevels(clockNet, clockNDR, targetLevels);
}

// Function to apply NDR to the first half of clock tree levels
int TritonCTS::applyNDRToFirstHalfLevels(Clock& clockNet,
                                         odb::dbTechNonDefaultRule* clockNDR)
{
  // Get all unique levels in the design
  const std::vector<int> allLevels = getAllClockTreeLevels(clockNet);

  // Calculate first half (rounding up if odd number of levels)
  const int halfCount = (allLevels.size() + 1) / 2;

  // Create vector with first half of levels
  std::vector<int> firstHalfLevels(allLevels.begin(),
                                   allLevels.begin() + halfCount);

  // clang-format off
  debugPrint(logger_, CTS, "clustering", 1, "Total clock tree levels found: {}"
        " Applying NDR to first {} levels", allLevels.size(), halfCount);
  // clang-format on

  // Apply NDR to the first half
  return applyNDRToClockLevels(clockNet, clockNDR, firstHalfLevels);
}

// Priority for minSpc rule is SPACINGTABLE TWOWIDTHS > SPACINGTABLE PRL >
// SPACING
int TritonCTS::getNetSpacing(odb::dbTechLayer* layer,
                             const int width1,
                             const int width2)
{
  int min_spc = 0;
  if (layer->hasTwoWidthsSpacingRules()) {
    min_spc = layer->findTwSpacing(width1, width2, 0);
  } else if (layer->hasV55SpacingRules()) {
    min_spc = layer->findV55Spacing(std::max(width1, width2), 0);
  } else if (!layer->getV54SpacingRules().empty()) {
    for (auto rule : layer->getV54SpacingRules()) {
      if (rule->hasRange()) {
        uint32_t rmin;
        uint32_t rmax;
        rule->getRange(rmin, rmax);
        if (width1 < rmin || width2 > rmax) {
          continue;
        }
      }
      min_spc = std::max<int>(min_spc, rule->getSpacing());
    }
  } else {
    min_spc = layer->getSpacing();
  }

  // Last resort, get pitch - minWidth
  if (min_spc == 0) {
    min_spc = layer->getPitch() - layer->getMinWidth();
  }

  return min_spc;
}

void TritonCTS::writeClockNDRsToDb(TreeBuilder* builder)
{
  char ruleName[64];
  int ruleIndex = 0;
  odb::dbTechNonDefaultRule* clockNDR;
  Clock& clockNet = builder->getClock();

  // create a new non-default rule in *block* not tech
  while (ruleIndex >= 0) {
    snprintf(ruleName, 64, "CTS_NDR_%d", ruleIndex++);
    clockNDR = odb::dbTechNonDefaultRule::create(block_, ruleName);
    if (clockNDR) {
      break;
    }
  }
  assert(clockNDR != nullptr);

  // define NDR for all routing layers
  odb::dbTech* tech = db_->getTech();
  for (int i = 1; i <= tech->getRoutingLayerCount(); i++) {
    odb::dbTechLayer* layer = tech->findRoutingLayer(i);
    odb::dbTechLayerRule* layerRule = clockNDR->getLayerRule(layer);
    if (!layerRule) {
      layerRule = odb::dbTechLayerRule::create(clockNDR, layer);
    }
    assert(layerRule != nullptr);

    int defaultWidth = layer->getWidth();
    int defaultSpace = getNetSpacing(layer, defaultWidth, defaultWidth);

    // If width or space is 0, something is not right
    if (defaultWidth == 0 || defaultSpace == 0) {
      logger_->warn(CTS,
                    208,
                    "Clock NDR settings for layer {}: defaultSpace: {} - "
                    "defaultWidth: {}",
                    layer->getName(),
                    defaultSpace,
                    defaultWidth);
    }

    // Set NDR settings
    int ndr_width = defaultWidth;
    layerRule->setWidth(ndr_width);
    int ndr_space = 2 * getNetSpacing(layer, ndr_width, ndr_width);
    layerRule->setSpacing(ndr_space);

    debugPrint(logger_,
               CTS,
               "clustering",
               1,
               "  NDR rule set to layer {} {} as space={} width={} vs. default "
               "space={} width={}",
               i,
               layer->getName(),
               layerRule->getSpacing(),
               layerRule->getWidth(),
               defaultSpace,
               defaultWidth);
  }

  int clkNets = 0;

  // Apply NDR following the selected strategy (root_only, half, full)
  switch (options_->getApplyNdr()) {
    case CtsOptions::NdrStrategy::ROOT_ONLY:
      clkNets = applyNDRToClockLevels(clockNet, clockNDR, {0});
      break;
    case CtsOptions::NdrStrategy::HALF:
      clkNets = applyNDRToFirstHalfLevels(clockNet, clockNDR);
      break;
    case CtsOptions::NdrStrategy::FULL:
      clkNets = applyNDRToClockLevels(
          clockNet, clockNDR, getAllClockTreeLevels(clockNet));
      break;
    case CtsOptions::NdrStrategy::NONE:
      // Should not be called
      break;
  }

  debugPrint(logger_,
             CTS,
             "clustering",
             1,
             "Non-default rule {} for double spacing has been applied to {} "
             "clock nets",
             ruleName,
             clkNets);
}

std::pair<int, int> TritonCTS::branchBufferCount(ClockInst* inst,
                                                 int bufCounter,
                                                 Clock& clockNet)
{
  odb::dbInst* sink = inst->getDbInst();
  odb::dbITerm* outITerm = sink->getFirstOutput();
  int minPath = std::numeric_limits<int>::max();
  int maxPath = std::numeric_limits<int>::min();
  for (odb::dbITerm* sinkITerms : outITerm->getNet()->getITerms()) {
    if (sinkITerms != outITerm) {
      ClockInst* clockInst
          = clockNet.findClockByName(sinkITerms->getInst()->getName());
      if (clockInst == nullptr) {
        int newResult = bufCounter + 1;
        maxPath = std::max(newResult, maxPath);
        minPath = std::min(newResult, minPath);
      } else {
        std::pair<int, int> newResults
            = branchBufferCount(clockInst, bufCounter + 1, clockNet);
        minPath = std::min(newResults.first, minPath);
        maxPath = std::max(newResults.second, maxPath);
      }
    }
  }
  std::pair<int, int> results(minPath, maxPath);
  return results;
}

void TritonCTS::disconnectAllSinksFromNet(odb::dbNet* net)
{
  odb::dbSet<odb::dbITerm> iterms = net->getITerms();
  for (odb::dbITerm* iterm : iterms) {
    if (iterm->getIoType() == odb::dbIoType::INPUT) {
      iterm->disconnect();
    }
  }
}

void TritonCTS::disconnectAllPinsFromNet(odb::dbNet* net)
{
  odb::dbSet<odb::dbITerm> iterms = net->getITerms();
  for (odb::dbITerm* iterm : iterms) {
    iterm->disconnect();
  }
}

void TritonCTS::checkUpstreamConnections(odb::dbNet* net)
{
  while (net->getITermCount() <= 1) {
    // Net is incomplete, only 1 pin.
    odb::dbITerm* firstITerm = net->get1stITerm();
    if (firstITerm == nullptr) {
      disconnectAllPinsFromNet(net);
      odb::dbNet::destroy(net);
      break;
    }
    odb::dbInst* bufferInst = firstITerm->getInst();
    odb::dbITerm* driverInputPin = getFirstInput(bufferInst);
    disconnectAllPinsFromNet(net);
    odb::dbNet::destroy(net);
    net = driverInputPin->getNet();
    ++numFixedNets_;
    --numClkNets_;
    odb::dbInst::destroy(bufferInst);
  }
}

void TritonCTS::createClockBuffers(Clock& clockNet, odb::dbModule* parent)
{
  unsigned numBuffers = 0;
  clockNet.forEachClockBuffer([&](ClockInst& inst) {
    odb::dbMaster* master = db_->findMaster(inst.getMaster().c_str());
    odb::dbInst* newInst = odb::dbInst::create(
        block_, master, inst.getName().c_str(), false, parent);
    newInst->setSourceType(odb::dbSourceType::TIMING);
    inst.setInstObj(newInst);
    inst2clkbuf_[newInst] = &inst;
    inst.setInputPinObj(getFirstInput(newInst));
    newInst->setLocation(inst.getX(), inst.getY());
    newInst->setPlacementStatus(odb::dbPlacementStatus::PLACED);
    ++numBuffers;
  });
  logger_->info(CTS, 18, "    Created {} clock buffers.", numBuffers);
  int totalBuffers = options_->getNumBuffersInserted() + numBuffers;
  options_->setNumBuffersInserted(totalBuffers);
}

odb::dbITerm* TritonCTS::getFirstInput(odb::dbInst* inst) const
{
  odb::dbSet<odb::dbITerm> iterms = inst->getITerms();
  for (odb::dbITerm* iterm : iterms) {
    if (iterm->isInputSignal()) {
      return iterm;
    }
  }

  return nullptr;
}

odb::dbITerm* TritonCTS::getSingleOutput(odb::dbInst* inst,
                                         odb::dbITerm* input) const
{
  odb::dbSet<odb::dbITerm> iterms = inst->getITerms();
  odb::dbITerm* output = nullptr;
  for (odb::dbITerm* iterm : iterms) {
    if (iterm != input && iterm->isOutputSignal()) {
      odb::dbNet* net = iterm->getNet();
      if (net) {
        if (staClockNets_.find(net) != staClockNets_.end()) {
          output = iterm;
          break;
        }
      }
    }
  }
  return output;
}
bool TritonCTS::masterExists(const std::string& master) const
{
  return db_->findMaster(master.c_str());
};

void TritonCTS::findClockRoots(sta::Clock* clk,
                               std::set<odb::dbNet*>& clockNets)
{
  std::vector<odb::dbNet*> skipNets = options_->getSkipNets();
  for (const sta::Pin* pin : clk->leafPins()) {
    odb::dbITerm* instTerm;
    odb::dbBTerm* port;
    odb::dbModITerm* moditerm;
    network_->staToDb(pin, instTerm, port, moditerm);
    odb::dbNet* net = instTerm ? instTerm->getNet() : port->getNet();
    if (std::ranges::find(skipNets, net) != skipNets.end()) {
      logger_->warn(CTS,
                    42,
                    "Skipping root net {}, specified by the user...",
                    net->getName());
      continue;
    }
    clockNets.insert(net);
  }
}

float TritonCTS::getInputPinCap(odb::dbITerm* iterm)
{
  odb::dbInst* inst = iterm->getInst();
  sta::Cell* masterCell = network_->dbToSta(inst->getMaster());
  sta::LibertyCell* libertyCell = network_->libertyCell(masterCell);
  if (!libertyCell) {
    return 0.0;
  }

  sta::LibertyPort* inputPort
      = libertyCell->findLibertyPort(iterm->getMTerm()->getConstName());
  if (inputPort) {
    return inputPort->capacitance();
  }

  return 0.0;
}

bool TritonCTS::isSink(odb::dbITerm* iterm)
{
  odb::dbInst* inst = iterm->getInst();
  sta::Cell* masterCell = network_->dbToSta(inst->getMaster());
  sta::LibertyCell* libertyCell = network_->libertyCell(masterCell);
  if (!libertyCell) {
    return true;
  }

  if (inst->isBlock()) {
    return true;
  }

  sta::LibertyPort* inputPort
      = libertyCell->findLibertyPort(iterm->getMTerm()->getConstName());
  if (inputPort) {
    return inputPort->isRegClk();
  }

  return false;
}

bool TritonCTS::hasInsertionDelay(odb::dbInst* inst, odb::dbMTerm* mterm)
{
  if (options_->insertionDelayEnabled()) {
    sta::LibertyCell* libCell = network_->libertyCell(network_->dbToSta(inst));
    if (libCell) {
      sta::LibertyPort* libPort
          = libCell->findLibertyPort(mterm->getConstName());
      if (libPort) {
        const float rise = libPort->clkTreeDelay(
            0.0, sta::RiseFall::rise(), sta::MinMax::max());
        const float fall = libPort->clkTreeDelay(
            0.0, sta::RiseFall::fall(), sta::MinMax::max());

        if (rise != 0 || fall != 0) {
          return true;
        }
      }
    }
  }
  return false;
}

double TritonCTS::computeInsertionDelay(const std::string& name,
                                        odb::dbInst* inst,
                                        odb::dbMTerm* mterm)
{
  double insDelayPerMicron = 0.0;

  if (!options_->insertionDelayEnabled()) {
    return insDelayPerMicron;
  }

  sta::LibertyCell* libCell = network_->libertyCell(network_->dbToSta(inst));
  if (libCell) {
    sta::LibertyPort* libPort = libCell->findLibertyPort(mterm->getConstName());
    if (libPort) {
      const float rise = libPort->clkTreeDelay(
          0.0, sta::RiseFall::rise(), sta::MinMax::max());
      const float fall = libPort->clkTreeDelay(
          0.0, sta::RiseFall::fall(), sta::MinMax::max());

      if (rise != 0 || fall != 0) {
        // use average of max rise and max fall
        // TODO: do we need to look at min insertion delays?
        double delayPerSec = (rise + fall) / 2.0;
        // convert delay to length because HTree uses lengths
        sta::Corner* corner = openSta_->cmdCorner();
        double capPerMicron
            = estimate_parasitics_->wireSignalCapacitance(corner) * 1e-6;
        double resPerMicron
            = estimate_parasitics_->wireSignalResistance(corner) * 1e-6;
        if (sta::fuzzyEqual(capPerMicron, 1e-18)
            || sta::fuzzyEqual(resPerMicron, 1e-18)) {
          logger_->warn(CTS,
                        203,
                        "Insertion delay cannot be used because unit "
                        "capacitance or unit resistance is zero.  Check "
                        "layer RC settings.");
          return 0.0;
        }
        insDelayPerMicron = delayPerSec / (capPerMicron * resPerMicron);
        // clang-format off
          debugPrint(logger_, CTS, "clustering", 1, "sink {} has ins "
                     "delay={:.2e} and micron leng={:0.1f} dbUnits/um={}",
                     name, delayPerSec, insDelayPerMicron,
                     block_->getDbUnitsPerMicron());
          debugPrint(logger_, CTS, "clustering", 1, "capPerMicron={:.2e} "
                     "resPerMicron={:.2e}", capPerMicron, resPerMicron);
        // clang-format on
      }
    }
  }
  return insDelayPerMicron;
}

static float getInputCap(const sta::LibertyCell* cell)
{
  sta::LibertyPort *in, *out;
  cell->bufferPorts(in, out);
  if (in != nullptr) {
    return in->capacitance();
  }
  return 0.0;
}

static sta::LibertyCell* findBestDummyCell(
    const std::vector<sta::LibertyCell*>& dummyCandidates,
    float deltaCap)
{
  float minDiff = std::numeric_limits<float>::max();
  sta::LibertyCell* bestCell = nullptr;
  for (sta::LibertyCell* cell : dummyCandidates) {
    float diff = std::abs(getInputCap(cell) - deltaCap);
    if (diff < minDiff) {
      minDiff = diff;
      bestCell = cell;
    }
  }
  return bestCell;
}

int TritonCTS::writeDummyLoadsToDb(Clock& clockNet,
                                   std::unordered_set<odb::dbInst*>& dummies)
{
  // Traverse clock tree and compute ideal output caps for clock
  // buffers in the same level
  if (!computeIdealOutputCaps(clockNet)) {
    // No cap adjustment is needed
    return 0;
  }

  // Find suitable candidate cells for dummy loads
  std::vector<sta::LibertyCell*> dummyCandidates;
  findCandidateDummyCells(dummyCandidates);

  int nDummies = 0;
  clockNet.forEachSubNet([&](ClockSubNet& subNet) {
    subNet.forEachSink([&](ClockInst* inst) {
      if (inst->isClockBuffer()
          && !sta::fuzzyEqual(inst->getOutputCap(),
                              inst->getIdealOutputCap())) {
        odb::dbInst* dummyInst
            = insertDummyCell(clockNet, inst, dummyCandidates);
        if (dummyInst != nullptr) {
          dummies.insert(dummyInst);
          nDummies++;
        }
      }
    });
  });

  if (logger_->debugCheck(utl::CTS, "dummy load", 1)) {
    printClockNetwork(clockNet);
  }
  return nDummies;
}

// Return true if any clock buffers need cap adjustment; false otherwise
bool TritonCTS::computeIdealOutputCaps(Clock& clockNet)
{
  bool needAdjust = false;

  // pass 1: compute actual output caps seen by each clock instance
  clockNet.forEachSubNet([&](ClockSubNet& subNet) {
    // build driver -> subNet map
    ClockInst* driver = subNet.getDriver();
    driver2subnet_[driver] = &subNet;
    float sinkCapTotal = 0.0;
    subNet.forEachSink([&](ClockInst* inst) {
      odb::dbITerm* inputPin = inst->isClockBuffer()
                                   ? getFirstInput(inst->getDbInst())
                                   : inst->getDbInputPin();
      float cap = getInputPinCap(inputPin);
      // TODO: include wire caps?
      sinkCapTotal += cap;
    });
    driver->setOutputCap(sinkCapTotal);
  });

  // pass 2: compute ideal output caps for perfectly balanced tree
  clockNet.forEachSubNet([&](const ClockSubNet& subNet) {
    ClockInst* driver = subNet.getDriver();
    float maxCap = std::numeric_limits<float>::min();
    subNet.forEachSink([&](ClockInst* inst) {
      if (inst->isClockBuffer() && inst->getOutputCap() > maxCap) {
        maxCap = inst->getOutputCap();
      }
    });
    subNet.forEachSink([&](ClockInst* inst) {
      if (inst->isClockBuffer()) {
        inst->setIdealOutputCap(maxCap);
        float cap = inst->getOutputCap();
        if (!sta::fuzzyEqual(cap, maxCap)) {
          needAdjust = true;
          // clang-format off
          debugPrint(logger_, CTS, "dummy load", 1, "{} => {} "
                     "cap:{:0.2e} idealCap:{:0.2e} delCap:{:0.2e}",
                     driver->getName(), inst->getName(), cap, maxCap,
                     maxCap-cap);
          // clang-format on
        }
      }
    });
  });

  return needAdjust;
}

// Find clock buffers and inverters to use as dummy loads
void TritonCTS::findCandidateDummyCells(
    std::vector<sta::LibertyCell*>& dummyCandidates)
{
  // Add existing buffer list
  for (const std::string& buffer : options_->getBufferList()) {
    odb::dbMaster* master = db_->findMaster(buffer.c_str());

    if (master) {
      sta::Cell* masterCell = network_->dbToSta(master);
      if (masterCell) {
        sta::LibertyCell* libCell = network_->libertyCell(masterCell);
        if (libCell) {
          dummyCandidates.emplace_back(libCell);
        }
      }
    }
  }

  // Add additional inverter cells
  // first, look for inverters with "is_clock_cell: true" cell attribute
  std::vector<sta::LibertyCell*> inverters;
  sta::LibertyLibraryIterator* lib_iter = network_->libertyLibraryIterator();
  while (lib_iter->hasNext()) {
    sta::LibertyLibrary* lib = lib_iter->next();
    for (sta::LibertyCell* inv : *lib->inverters()) {
      if (inv->isClockCell() && resizer_->isClockCellCandidate(inv)) {
        inverters.emplace_back(inv);
        dummyCandidates.emplace_back(inv);
      }
    }
  }
  delete lib_iter;

  // second, look for all inverters with name CLKINV or clkinv
  if (inverters.empty()) {
    sta::PatternMatch patternClkInv("*CLKINV*",
                                    /* is_regexp */ true,
                                    /* nocase */ true,
                                    /* Tcl_interp* */ nullptr);
    lib_iter = network_->libertyLibraryIterator();
    while (lib_iter->hasNext()) {
      sta::LibertyLibrary* lib = lib_iter->next();
      for (sta::LibertyCell* inv :
           lib->findLibertyCellsMatching(&patternClkInv)) {
        if (inv->isInverter() && resizer_->isClockCellCandidate(inv)) {
          inverters.emplace_back(inv);
          dummyCandidates.emplace_back(inv);
        }
      }
    }
    delete lib_iter;
  }

  // third, look for all inverters with name INV or inv
  if (inverters.empty()) {
    sta::PatternMatch patternInv("*INV*",
                                 /* is_regexp */ true,
                                 /* nocase */ true,
                                 /* Tcl_interp* */ nullptr);
    lib_iter = network_->libertyLibraryIterator();
    while (lib_iter->hasNext()) {
      sta::LibertyLibrary* lib = lib_iter->next();
      for (sta::LibertyCell* inv : lib->findLibertyCellsMatching(&patternInv)) {
        if (inv->isInverter() && resizer_->isClockCellCandidate(inv)) {
          inverters.emplace_back(inv);
          dummyCandidates.emplace_back(inv);
        }
      }
    }
    delete lib_iter;
  }

  // abandon attributes & name patterns, just look for all inverters
  if (inverters.empty()) {
    lib_iter = network_->libertyLibraryIterator();
    while (lib_iter->hasNext()) {
      sta::LibertyLibrary* lib = lib_iter->next();
      for (sta::LibertyCell* inv : *lib->inverters()) {
        if (resizer_->isClockCellCandidate(inv)) {
          inverters.emplace_back(inv);
          dummyCandidates.emplace_back(inv);
        }
      }
    }
    delete lib_iter;
  }

  // Sort cells in ascending order of input cap
  std::ranges::sort(
      dummyCandidates,
      [](const sta::LibertyCell* cell1, const sta::LibertyCell* cell2) {
        return (getInputCap(cell1) < getInputCap(cell2));
      });

  if (logger_->debugCheck(utl::CTS, "dummy load", 1)) {
    for (const sta::LibertyCell* libCell : dummyCandidates) {
      // clang-format off
      logger_->debug(CTS, "dummy load",
                     "  {} is a dummy cell candidate with input cap={:0.3e}",
                     libCell->name(), getInputCap(libCell));
      // clang-format on
    }
  }
}

odb::dbInst* TritonCTS::insertDummyCell(
    Clock& clockNet,
    ClockInst* inst,
    const std::vector<sta::LibertyCell*>& dummyCandidates)
{
  ClockSubNet* subNet = driver2subnet_[inst];
  if (subNet->getNumSinks() == options_->getMaxFanout()) {
    return nullptr;
  }
  float deltaCap = inst->getIdealOutputCap() - inst->getOutputCap();
  sta::LibertyCell* dummyCell = findBestDummyCell(dummyCandidates, deltaCap);
  // clang-format off
  debugPrint(logger_, CTS, "dummy load", 1, "insertDummyCell {} at {}",
             inst->getName(), dummyCell->name());
  // clang-format on
  odb::dbInst* dummyInst = nullptr;
  ClockInst& dummyClock = placeDummyCell(clockNet, inst, dummyCell, dummyInst);
  if (driver2subnet_.find(inst) == driver2subnet_.end()) {
    logger_->error(
        CTS, 120, "Subnet was not found for clock buffer {}.", inst->getName());
    return nullptr;
  }
  connectDummyCell(inst, dummyInst, *subNet, dummyClock);
  return dummyInst;
}

ClockInst& TritonCTS::placeDummyCell(Clock& clockNet,
                                     const ClockInst* inst,
                                     const sta::LibertyCell* dummyCell,
                                     odb::dbInst*& dummyInst)
{
  odb::dbMaster* master = network_->staToDb(dummyCell);
  if (master == nullptr) {
    logger_->error(CTS,
                   118,
                   "No phyiscal master cell found for dummy cell {}.",
                   dummyCell->name());
  }
  std::string cellName
      = options_->getDummyLoadPrefix() + std::to_string(dummyLoadIndex_++);
  dummyInst = odb::dbInst::create(block_, master, cellName.c_str());
  dummyInst->setSourceType(odb::dbSourceType::TIMING);
  dummyInst->setLocation(inst->getX(), inst->getY());
  dummyInst->setPlacementStatus(odb::dbPlacementStatus::PLACED);
  ClockInst& dummyClock = clockNet.addClockBuffer(
      cellName, master->getName(), inst->getX(), inst->getY());
  // clang-format off
  debugPrint(logger_, CTS, "dummy load", 1, "  placed dummy instance {} at {}",
             dummyInst->getName(), dummyInst->getLocation());
  return dummyClock;
  // clang-format on
}

void TritonCTS::connectDummyCell(const ClockInst* inst,
                                 odb::dbInst* dummyInst,
                                 ClockSubNet& subNet,
                                 ClockInst& dummyClock)
{
  odb::dbInst* sinkInst = inst->getDbInst();
  if (sinkInst == nullptr) {
    logger_->error(
        CTS, 119, "Phyiscal instance {} is not found.", inst->getName());
  }
  odb::dbITerm* iTerm = sinkInst->getFirstOutput();
  odb::dbNet* sinkNet = iTerm->getNet();
  odb::dbITerm* dummyInputPin = getFirstInput(dummyInst);
  dummyInputPin->connect(sinkNet);
  dummyClock.setInputPinObj(dummyInputPin);
  subNet.addInst(dummyClock);
}

void TritonCTS::printClockNetwork(const Clock& clockNet) const
{
  clockNet.forEachSubNet([&](const ClockSubNet& subNet) {
    ClockInst* driver = subNet.getDriver();
    logger_->report("{} has {} sinks", driver->getName(), subNet.getNumSinks());
    subNet.forEachSink([&](const ClockInst* inst) {
      logger_->report("{} -> {}", driver->getName(), inst->getName());
    });
  });
}

void TritonCTS::setAllClocksPropagated()
{
  sta::Sdc* sdc = openSta_->sdc();
  for (sta::Clock* clk : *sdc->clocks()) {
    openSta_->setPropagatedClock(clk);
  }
  estimate_parasitics_->estimateParasitics(est::ParasiticsSrc::placement);
}

void TritonCTS::repairClockNets()
{
  double max_wire_length
      = resizer_->findMaxWireLength(/* don't issue error */ false);
  if (max_wire_length > 0.0) {
    resizer_->repairClkNets(max_wire_length);
  }
}

// Balance macro cell latencies with register latencies.
// This is needed only if special insertion delay handling
// is invoked.
void TritonCTS::balanceMacroRegisterLatencies()
{
  if (!options_->insertionDelayEnabled()) {
    return;
  }

  // Visit builders from bottom up such that latencies are adjusted near bottom
  // trees first
  int totalDelayBuff = 0;

  sta::Corner* corner = openSta_->cmdCorner();
  // convert from per meter to per dbu
  double capPerDBU = estimate_parasitics_->wireClkCapacitance(corner) * 1e-6
                     / block_->getDbUnitsPerMicron();

  for (auto& builder : std::ranges::reverse_view(builders_)) {
    if (builder->getParent() == nullptr && !builder->getChildren().empty()) {
      est::IncrementalParasiticsGuard parasitics_guard(estimate_parasitics_);
      LatencyBalancer balancer = LatencyBalancer(builder.get(),
                                                 options_,
                                                 logger_,
                                                 db_,
                                                 network_,
                                                 openSta_,
                                                 techChar_->getLengthUnit(),
                                                 capPerDBU);
      totalDelayBuff += balancer.run();
    }
  }
  if (totalDelayBuff) {
    logger_->info(CTS, 37, "Total number of delay buffers: {}", totalDelayBuff);
  }
}

}  // namespace cts
