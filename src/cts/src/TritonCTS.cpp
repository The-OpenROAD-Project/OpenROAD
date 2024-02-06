/////////////////////////////////////////////////////////////////////////////
//
// BSD 3-Clause License
//
// Copyright (c) 2019, The Regents of the University of California
// All rights reserved.
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

#include "cts/TritonCTS.h"

#include <cctype>
#include <chrono>
#include <ctime>
#include <fstream>
#include <iterator>
#include <unordered_set>

#include "Clock.h"
#include "CtsOptions.h"
#include "HTreeBuilder.h"
#include "LevelBalancer.h"
#include "TechChar.h"
#include "TreeBuilder.h"
#include "db_sta/dbNetwork.hh"
#include "db_sta/dbSta.hh"
#include "odb/db.h"
#include "odb/dbShape.h"
#include "ord/OpenRoad.hh"
#include "rsz/Resizer.hh"
#include "sta/Fuzzy.hh"
#include "sta/Liberty.hh"
#include "sta/PatternMatch.hh"
#include "sta/Sdc.hh"
#include "utl/Logger.h"

namespace cts {

using utl::CTS;

void TritonCTS::init(utl::Logger* logger,
                     odb::dbDatabase* db,
                     sta::dbNetwork* network,
                     sta::dbSta* sta,
                     stt::SteinerTreeBuilder* st_builder,
                     rsz::Resizer* resizer)
{
  logger_ = logger;
  db_ = db;
  network_ = network;
  openSta_ = sta;
  resizer_ = resizer;

  options_ = new CtsOptions(logger_, st_builder);
  techChar_ = new TechChar(options_, db_, openSta_, resizer, network_, logger_);
  builders_ = new std::vector<TreeBuilder*>;
}

TritonCTS::~TritonCTS()
{
  delete options_;
  delete techChar_;
  delete builders_;
}

void TritonCTS::runTritonCts()
{
  setupCharacterization();
  findClockRoots();
  populateTritonCTS();
  if (builders_->empty()) {
    logger_->warn(CTS, 82, "No valid clock nets in the design.");
  } else {
    checkCharacterization();
    buildClockTrees();
    writeDataToDb();
  }
}

void TritonCTS::addBuilder(TreeBuilder* builder)
{
  builders_->push_back(builder);
}

void TritonCTS::setupCharacterization()
{
  // Finalize root/sink buffers
  std::string rootBuffer = selectRootBuffer(rootBuffers_);
  options_->setRootBuffer(rootBuffer);
  std::string sinkBuffer = selectSinkBuffer(sinkBuffers_);
  options_->setSinkBuffer(sinkBuffer);

  // A new characteriztion is always created.
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
      if (visitedMasters.count(master) == 0) {
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
  for (TreeBuilder* builder : *builders_) {
    builder->setTechChar(*techChar_);
    builder->setDb(db_);
    builder->setLogger(logger_);
    builder->initBlockages();
    builder->run();
  }

  if (options_->getBalanceLevels()) {
    for (TreeBuilder* builder : *builders_) {
      if (!builder->getParent() && !builder->getChildren().empty()) {
        LevelBalancer balancer(
            builder, options_, logger_, techChar_->getLengthUnit());
        balancer.run();
      }
    }
  }
}

void TritonCTS::initOneClockTree(odb::dbNet* driverNet,
                                 const std::string& sdcClockName,
                                 TreeBuilder* parent)
{
  TreeBuilder* clockBuilder = nullptr;
  if (driverNet->isSpecial()) {
    logger_->info(
        CTS, 116, "Special net \"{}\" skipped.", driverNet->getName());
  } else {
    clockBuilder = initClock(driverNet, sdcClockName, parent);
  }
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
            initOneClockTree(outputNet, sdcClockName, clockBuilder);
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
    const std::unordered_set<odb::dbITerm*>& sinks)
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
      bool terminate
          = fullTree
                ? (sinks.find(iterm) != sinks.end())
                : !builder->isAnyTreeBuffer(getClockFromInst(iterm->getInst()));
      if (!terminate) {
        odb::dbITerm* outputPin = iterm->getInst()->getFirstOutput();
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
                                sinks);
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
          logger_->info(
              CTS, 121, "{} '{}' has unconnected output pin.", cellType, name);
        }
        if (builder->isLeafBuffer(getClockFromInst(iterm->getInst()))) {
          leafSinks++;
        }
      } else {
        sinks_cnt++;
        double currSinkWl
            = (dist + currWireLength) / double(options_->getDbUnits());
        sinkWireLength += currSinkWl;
        if (depth > maxDepth) {
          maxDepth = depth;
        }
        if ((minDepth > 0 && depth < minDepth) || (minDepth == 0)) {
          minDepth = depth;
        }
      }
    }
  }  // ignoring block pins/feedthrus
}

ClockInst* TritonCTS::getClockFromInst(odb::dbInst* inst)
{
  return (inst2clkbuf_.find(inst) != inst2clkbuf_.end()) ? inst2clkbuf_[inst]
                                                         : nullptr;
}

void TritonCTS::writeDataToDb()
{
  std::set<odb::dbNet*> clkLeafNets;
  for (TreeBuilder* builder : *builders_) {
    writeClockNetsToDb(builder->getClock(), clkLeafNets);
    if (options_->applyNDR()) {
      writeClockNDRsToDb(clkLeafNets);
    }
    if (options_->dummyLoadEnabled()) {
      writeDummyLoadsToDb(builder->getClock());
    }
  }

  for (TreeBuilder* builder : *builders_) {
    odb::dbNet* topClockNet = builder->getClock().getNetObj();
    unsigned sinkCount = 0;
    unsigned leafSinks = 0;
    double allSinkDistance = 0.0;
    int minDepth = 0;
    int maxDepth = 0;
    bool reportFullTree = !builder->getParent()
                          && !builder->getChildren().empty()
                          && options_->getBalanceLevels();

    std::unordered_set<odb::dbITerm*> sinks;
    builder->getClock().forEachSink([&sinks](const ClockInst& inst) {
      sinks.insert(inst.getDbInputPin());
    });
    countSinksPostDbWrite(builder,
                          topClockNet,
                          sinkCount,
                          leafSinks,
                          0,
                          allSinkDistance,
                          minDepth,
                          maxDepth,
                          0,
                          reportFullTree,
                          sinks);
    logger_->info(CTS, 98, "Clock net \"{}\"", builder->getClock().getName());
    logger_->info(CTS, 99, " Sinks {}", sinkCount);
    logger_->info(CTS, 100, " Leaf buffers {}", leafSinks);
    double avgWL = allSinkDistance / sinkCount;
    logger_->info(CTS, 101, " Average sink wire length {:.2f} um", avgWL);
    logger_->info(CTS, 102, " Path depth {} - {}", minDepth, maxDepth);
    if (options_->dummyLoadEnabled()) {
      logger_->info(CTS, 207, " Leaf load cells {}", dummyLoadIndex_);
    }
  }
}

void TritonCTS::forEachBuilder(
    const std::function<void(const TreeBuilder*)>& func) const
{
  for (const TreeBuilder* builder : *builders_) {
    func(builder);
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
    file.close();

  } else {
    logger_->info(CTS,
                  3,
                  "Total number of Clock Roots: {}.",
                  options_->getNumClockRoots());
    logger_->info(CTS,
                  4,
                  "Total number of Buffers Inserted: {}.",
                  options_->getNumBuffersInserted());
    logger_->info(CTS,
                  5,
                  "Total number of Clock Subnets: {}.",
                  options_->getNumClockSubnets());
    logger_->info(
        CTS, 6, "Total number of Sinks: {}.", options_->getNumSinks());
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
  std::stringstream ss(buffers);
  std::istream_iterator<std::string> begin(ss);
  std::istream_iterator<std::string> end;
  std::vector<std::string> bufferList(begin, end);
  if (bufferList.empty()) {
    inferBufferList(bufferList);
  }
  options_->setBufferList(bufferList);
}

void TritonCTS::inferBufferList(std::vector<std::string>& buffers)
{
  // first, look for buffers with "is_clock_cell: true" cell attribute
  sta::LibertyLibraryIterator* lib_iter = network_->libertyLibraryIterator();
  while (lib_iter->hasNext()) {
    sta::LibertyLibrary* lib = lib_iter->next();
    for (sta::LibertyCell* buffer : *lib->buffers()) {
      if (buffer->isClockCell() && isClockCellCandidate(buffer)) {
        buffers.emplace_back(buffer->name());
        // clang-format off
        debugPrint(logger_, CTS, "buffering", 1, "{} has clock cell attribute",
                   buffer->name());
        // clang-format on
      }
    }
  }

  // second, look for all buffers with name CLKBUF or clkbuf
  if (buffers.empty()) {
    sta::PatternMatch patternClkBuf("*CLKBUF*",
                                    /* is_regexp */ true,
                                    /* nocase */ true,
                                    /* Tcl_interp* */ nullptr);
    sta::LibertyLibraryIterator* lib_iter = network_->libertyLibraryIterator();
    while (lib_iter->hasNext()) {
      sta::LibertyLibrary* lib = lib_iter->next();
      for (sta::LibertyCell* buffer :
           lib->findLibertyCellsMatching(&patternClkBuf)) {
        if (buffer->isBuffer() && isClockCellCandidate(buffer)) {
          buffers.emplace_back(buffer->name());
        }
      }
    }
  }

  // third, look for all buffers with name BUF or buf
  if (buffers.empty()) {
    sta::PatternMatch patternBuf("*BUF*",
                                 /* is_regexp */ true,
                                 /* nocase */ true,
                                 /* Tcl_interp* */ nullptr);
    lib_iter = network_->libertyLibraryIterator();
    while (lib_iter->hasNext()) {
      sta::LibertyLibrary* lib = lib_iter->next();
      for (sta::LibertyCell* buffer :
           lib->findLibertyCellsMatching(&patternBuf)) {
        if (buffer->isBuffer() && isClockCellCandidate(buffer)) {
          buffers.emplace_back(buffer->name());
        }
      }
    }
  }

  // abandon attributes & name patterns, just look for all buffers
  if (buffers.empty()) {
    lib_iter = network_->libertyLibraryIterator();
    while (lib_iter->hasNext()) {
      sta::LibertyLibrary* lib = lib_iter->next();
      for (sta::LibertyCell* buffer : *lib->buffers()) {
        if (isClockCellCandidate(buffer)) {
          buffers.emplace_back(buffer->name());
        }
      }
    }

    if (buffers.empty()) {
      logger_->error(
          CTS,
          110,
          "No clock buffer candidates could be found from any libraries.");
    }

    // it's possible that pattern-based lib cell search missed
    // clock buffers (because they are not loaded or linked?)
    std::string pattern("clkbuf");
    std::vector<std::string> clockBuffers
        = findMatchingSubset(pattern, buffers);
    // clang-format off
    debugPrint(logger_, CTS, "buffering", 1, "{} buffers with 'clkbuf' "
               "have been found", clockBuffers.size());
    // clang-format on
    if (!clockBuffers.empty()) {
      buffers = clockBuffers;
    } else {
      pattern = std::string("buf");
      clockBuffers = findMatchingSubset(pattern, buffers);
      // clang-format off
      debugPrint(logger_, CTS, "buffering", 1, "{} buffers with 'buf' "
                 "have been found", clockBuffers.size());
      // clang-format on
      if (!clockBuffers.empty()) {
        buffers = std::move(clockBuffers);
      }
    }
  }

  options_->setBufferListInferred(true);
  if (logger_->debugCheck(utl::CTS, "buffering", 1)) {
    for (const std::string& bufName : buffers) {
      logger_->report("{} has been inferred as clock buffer", bufName);
    }
  }
}

std::string toLowerCase(std::string str)
{
  std::transform(str.begin(), str.end(), str.begin(), [](unsigned char c) {
    return std::tolower(c);
  });
  return str;
}

std::vector<std::string> TritonCTS::findMatchingSubset(
    const std::string& pattern,
    const std::vector<std::string>& buffers)
{
  std::vector<std::string> subset;
  std::copy_if(buffers.begin(),
               buffers.end(),
               std::back_inserter(subset),
               [&pattern](const std::string& str) {
                 std::string lowerCaseStr = toLowerCase(str);
                 return lowerCaseStr.find(pattern) != std::string::npos;
               });
  return subset;
}

bool TritonCTS::isClockCellCandidate(sta::LibertyCell* cell)
{
  return (!cell->dontUse() && !resizer_->dontUse(cell) && !cell->alwaysOn()
          && !cell->isIsolationCell() && !cell->isLevelShifter());
}

void TritonCTS::setRootBuffer(const char* buffers)
{
  std::stringstream ss(buffers);
  std::istream_iterator<std::string> begin(ss);
  std::istream_iterator<std::string> end;
  std::vector<std::string> bufferList(begin, end);
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
  float rootWireCap
      = resizer_->wireSignalCapacitance(corner) * 1e-6 * sinkWireLength / 2.0;
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
  float sinkWireCap
      = resizer_->wireSignalCapacitance(corner) * 1e-6 * sinkWireLength;

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
               maxCap * options_->getSinkBufferMaxCapDerate(), totalCap,
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

void TritonCTS::populateTritonCTS()
{
  block_ = db_->getChip()->getBlock();
  options_->setDbUnits(block_->getDbUnitsPerMicron());

  clearNumClocks();

  // Use dbSta to find all clock nets in the design.
  std::vector<std::pair<std::set<odb::dbNet*>, std::string>> clockNetsInfo;

  // Checks the user input in case there are other nets that need to be added to
  // the set.
  std::vector<odb::dbNet*> inputClkNets = options_->getClockNetsObjs();

  if (!inputClkNets.empty()) {
    std::set<odb::dbNet*> clockNets;
    for (odb::dbNet* net : inputClkNets) {
      // Since a set is unique, only the nets not found by dbSta are added.
      clockNets.insert(net);
    }
    clockNetsInfo.emplace_back(std::make_pair(clockNets, std::string("")));
  } else {
    std::set<odb::dbNet*> allClkNets;
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
      clockNetsInfo.emplace_back(std::make_pair(clkNets, clkName));
      allClkNets.insert(clkNets.begin(), clkNets.end());
    }
  }

  // Iterate over all the nets found by the user-input and dbSta
  for (const auto& clockInfo : clockNetsInfo) {
    std::set<odb::dbNet*> clockNets = clockInfo.first;
    std::string clkName = clockInfo.second;
    for (odb::dbNet* net : clockNets) {
      if (net != nullptr) {
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
          initOneClockTree(net, clkName, nullptr);
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

TreeBuilder* TritonCTS::initClock(odb::dbNet* net,
                                  const std::string& sdcClock,
                                  TreeBuilder* parentBuilder)
{
  std::string driver;
  odb::dbITerm* iterm = net->getFirstOutput();
  int xPin, yPin;
  if (iterm == nullptr) {
    odb::dbBTerm* bterm = net->get1stBTerm();  // Clock pin
    if (bterm == nullptr) {
      logger_->info(
          CTS,
          122,
          "Clock net \"{}\" is skipped for CTS because it is not "
          "connected to any output instance pin or input block terminal.",
          net->getName());
      return nullptr;
    }
    driver = bterm->getConstName();
    bterm->getFirstPinLocation(xPin, yPin);
  } else {
    odb::dbInst* inst = iterm->getInst();
    odb::dbMTerm* mterm = iterm->getMTerm();
    std::string driver = std::string(inst->getConstName()) + "/"
                         + std::string(mterm->getConstName());
    int xTmp, yTmp;
    computeITermPosition(iterm, xTmp, yTmp);
    xPin = xTmp;
    yPin = yTmp;
  }

  // Initialize clock net
  Clock clockNet(net->getConstName(), driver, sdcClock, xPin, yPin);
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

  for (odb::dbITerm* iterm : net->getITerms()) {
    odb::dbInst* inst = iterm->getInst();

    if (buffer_masters.find(inst->getMaster()) != buffer_masters.end()) {
      logger_->warn(CTS,
                    105,
                    "Net \"{}\" already has clock buffer {}. Skipping...",
                    clockNet.getName(),
                    inst->getName());
      return nullptr;
    }

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
                net->getConstName(),
                clockNet.getNumSinks());

  int totalSinks = options_->getNumSinks() + clockNet.getNumSinks();
  options_->setNumSinks(totalSinks);

  incrementNumClocks();

  clockNet.setNetObj(net);
  HTreeBuilder* builder
      = new HTreeBuilder(options_, clockNet, parentBuilder, logger_, db_);
  addBuilder(builder);
  return builder;
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

void TritonCTS::writeClockNetsToDb(Clock& clockNet,
                                   std::set<odb::dbNet*>& clkLeafNets)
{
  odb::dbNet* topClockNet = clockNet.getNetObj();

  disconnectAllSinksFromNet(topClockNet);

  createClockBuffers(clockNet);

  // connect top buffer on the clock pin
  std::string topClockInstName = "clkbuf_0_" + clockNet.getName();
  odb::dbInst* topClockInst = block_->findInst(topClockInstName.c_str());
  odb::dbITerm* topClockInstInputPin = getFirstInput(topClockInst);
  topClockInstInputPin->connect(topClockNet);
  topClockNet->setSigType(odb::dbSigType::CLOCK);

  std::map<int, odb::uint> fanoutcount;

  // create subNets
  numClkNets_ = 0;
  numFixedNets_ = 0;
  const ClockSubNet* rootSubNet = nullptr;
  clockNet.forEachSubNet([&](const ClockSubNet& subNet) {
    bool outputPinFound = true;
    bool inputPinFound = true;
    bool leafLevelNet = subNet.isLeafLevel();
    if (("clknet_0_" + clockNet.getName()) == subNet.getName()) {
      rootSubNet = &subNet;
    }
    odb::dbNet* clkSubNet
        = odb::dbNet::create(block_, subNet.getName().c_str());
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
    if (inst->isClockBuffer()) {
      std::pair<int, int> resultsForBranch
          = branchBufferCount(inst, 1, clockNet);
      if (resultsForBranch.first < minPath) {
        minPath = resultsForBranch.first;
      }
      if (resultsForBranch.second > maxPath) {
        maxPath = resultsForBranch.second;
      }
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

void TritonCTS::writeClockNDRsToDb(const std::set<odb::dbNet*>& clkLeafNets)
{
  char ruleName[64];
  int ruleIndex = 0;
  odb::dbTechNonDefaultRule* clockNDR;

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
    int defaultSpace = layer->getSpacing();
    int defaultWidth = layer->getWidth();
    layerRule->setSpacing(defaultSpace * 2);
    layerRule->setWidth(defaultWidth);
    // clang-format off
    debugPrint(logger_, CTS, "clustering", 1, "  NDR rule set to layer {} {} as "
	       "space={} width={} vs. default space={} width={}",
	       i, layer->getName(),
	       layerRule->getSpacing(), layerRule->getWidth(),
	       defaultSpace, defaultWidth);
    // clang-format on
  }

  // apply NDR to all non-leaf clock nets
  int clkNets = 0;
  for (odb::dbNet* net : block_->getNets()) {
    if (net->getSigType() == odb::dbSigType::CLOCK
        && (clkLeafNets.find(net) == clkLeafNets.end())) {
      net->setNonDefaultRule(clockNDR);
      clkNets++;
    }
  }

  logger_->info(CTS,
                202,
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
        if (newResult > maxPath) {
          maxPath = newResult;
        }
        if (newResult < minPath) {
          minPath = newResult;
        }
      } else {
        std::pair<int, int> newResults
            = branchBufferCount(clockInst, bufCounter + 1, clockNet);
        if (newResults.first < minPath) {
          minPath = newResults.first;
        }
        if (newResults.second > maxPath) {
          maxPath = newResults.second;
        }
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

void TritonCTS::createClockBuffers(Clock& clockNet)
{
  unsigned numBuffers = 0;
  clockNet.forEachClockBuffer([&](ClockInst& inst) {
    odb::dbMaster* master = db_->findMaster(inst.getMaster().c_str());
    odb::dbInst* newInst
        = odb::dbInst::create(block_, master, inst.getName().c_str());
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
  for (const sta::Pin* pin : clk->leafPins()) {
    odb::dbITerm* instTerm;
    odb::dbBTerm* port;
    network_->staToDb(pin, instTerm, port);
    odb::dbNet* net = instTerm ? instTerm->getNet() : port->getNet();
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

double TritonCTS::computeInsertionDelay(const std::string& name,
                                        odb::dbInst* inst,
                                        odb::dbMTerm* mterm)
{
  double insDelayPerMicron = 0.0;

  if (options_->insertionDelayEnabled()) {
    sta::LibertyCell* libCell = network_->libertyCell(network_->dbToSta(inst));
    sta::LibertyPort* libPort = libCell->findLibertyPort(mterm->getConstName());
    sta::RiseFallMinMax insDelays = libPort->clockTreePathDelays();
    if (insDelays.hasValue()) {
      // use average of max rise and max fall
      // TODO: do we need to look at min insertion delays?
      double delayPerSec
          = (insDelays.value(sta::RiseFall::rise(), sta::MinMax::max())
             + insDelays.value(sta::RiseFall::fall(), sta::MinMax::max()))
            / 2.0;
      // convert delay to length because HTree uses lengths
      sta::Corner* corner = openSta_->cmdCorner();
      double capPerMicron = resizer_->wireSignalCapacitance(corner) * 1e-6;
      double resPerMicron = resizer_->wireSignalResistance(corner) * 1e-6;
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
      debugPrint(logger_, CTS, "clustering", 1, "sink {} has ins delay={:.2e} and "
		 "micron leng={:0.1f} dbUnits/um={}", name, delayPerSec,
		 insDelayPerMicron, block_->getDbUnitsPerMicron());
      debugPrint(logger_, CTS, "clustering", 1, "capPerMicron={:.2e} resPerMicron={:.2e}",
		 capPerMicron, resPerMicron);
      // clang-format on
    }
  }

  return insDelayPerMicron;
}

float getInputCap(const sta::LibertyCell* cell)
{
  sta::LibertyPort *in, *out;
  cell->bufferPorts(in, out);
  if (in != nullptr) {
    return in->capacitance();
  }
  return 0.0;
}

sta::LibertyCell* findBestDummyCell(
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

void TritonCTS::writeDummyLoadsToDb(Clock& clockNet)
{
  // Traverse clock tree and compute ideal output caps for clock
  // buffers in the same level
  if (!computeIdealOutputCaps(clockNet)) {
    // No cap adjustment is needed
    return;
  }

  // Find suitable candidate cells for dummy loads
  std::vector<sta::LibertyCell*> dummyCandidates;
  findCandidateDummyCells(dummyCandidates);

  clockNet.forEachSubNet([&](ClockSubNet& subNet) {
    subNet.forEachSink([&](ClockInst* inst) {
      if (inst->isClockBuffer()
          && !sta::fuzzyEqual(inst->getOutputCap(),
                              inst->getIdealOutputCap())) {
        insertDummyCell(clockNet, inst, dummyCandidates);
      }
    });
  });

  if (logger_->debugCheck(utl::CTS, "dummy load", 1)) {
    printClockNetwork(clockNet);
  }
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
      if (inv->isClockCell() && isClockCellCandidate(inv)) {
        inverters.emplace_back(inv);
        dummyCandidates.emplace_back(inv);
      }
    }
  }

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
        if (inv->isInverter() && isClockCellCandidate(inv)) {
          inverters.emplace_back(inv);
          dummyCandidates.emplace_back(inv);
        }
      }
    }
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
        if (inv->isInverter() && isClockCellCandidate(inv)) {
          inverters.emplace_back(inv);
          dummyCandidates.emplace_back(inv);
        }
      }
    }
  }

  // abandon attributes & name patterns, just look for all inverters
  if (inverters.empty()) {
    lib_iter = network_->libertyLibraryIterator();
    while (lib_iter->hasNext()) {
      sta::LibertyLibrary* lib = lib_iter->next();
      for (sta::LibertyCell* inv : *lib->inverters()) {
        if (isClockCellCandidate(inv)) {
          inverters.emplace_back(inv);
          dummyCandidates.emplace_back(inv);
        }
      }
    }
  }

  // Sort cells in ascending order of input cap
  std::sort(dummyCandidates.begin(),
            dummyCandidates.end(),
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

void TritonCTS::insertDummyCell(
    Clock& clockNet,
    ClockInst* inst,
    const std::vector<sta::LibertyCell*>& dummyCandidates)
{
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
    return;
  }
  ClockSubNet* subNet = driver2subnet_[inst];
  connectDummyCell(inst, dummyInst, *subNet, dummyClock);
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
      = std::string("clkload") + std::to_string(dummyLoadIndex_++);
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

}  // namespace cts
