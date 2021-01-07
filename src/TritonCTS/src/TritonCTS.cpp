/////////////////////////////////////////////////////////////////////////////
//
// BSD 3-Clause License
//
// Copyright (c) 2019, University of California, San Diego.
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

#include "tritoncts/TritonCTS.h"
#include "Clock.h"
#include "CtsOptions.h"
#include "HTreeBuilder.h"
#include "PostCtsOpt.h"
#include "StaEngine.h"
#include "TechChar.h"
#include "TreeBuilder.h"

#include "opendb/db.h"
#include "opendb/dbShape.h"
#include "openroad/Error.hh"
#include "db_sta/dbSta.hh"

#include <chrono>
#include <ctime>
#include <fstream>
#include <unordered_set>
#include <iterator>

namespace cts {

using ord::error;

void TritonCTS::init(ord::OpenRoad* openroad)
{
  _openroad = openroad;
  makeComponents();
}

void TritonCTS::makeComponents()
{
  _db = _openroad->getDb();
  _options = new CtsOptions;
  _techChar = new TechChar(_options);
  _staEngine = new StaEngine(_options);
  _builders = new std::vector<TreeBuilder*>;
}

void TritonCTS::deleteComponents()
{
  delete _options;
  delete _techChar;
  delete _staEngine;
  delete _builders;
}

TritonCTS::~TritonCTS()
{
  deleteComponents();
}

void TritonCTS::runTritonCts()
{
  printHeader();
  setupCharacterization();
  findClockRoots();
  populateTritonCts();
  checkCharacterization();
  if (_options->getOnlyCharacterization()) {
    return;
  }
  buildClockTrees();
  if (_options->runPostCtsOpt()) {
    runPostCtsOpt();
  }
  writeDataToDb();
  printFooter();
}

void TritonCTS::addBuilder(TreeBuilder* builder)
{
  _builders->push_back(builder);
}

void TritonCTS::printHeader() const
{
  std::cout << " *****************\n";
  std::cout << " * TritonCTS 2.0 *\n";
  std::cout << " *****************\n";
}

void TritonCTS::setupCharacterization()
{
  if (_options->runAutoLut()) {
    // A new characteriztion is created.
    createCharacterization();
  } else {
    // LUT files exists. Import the characterization results.
    importCharacterization();
  }
  // Also resets metrics everytime the setup is done
  _options->setNumSinks(0);
  _options->setNumBuffersInserted(0);
  _options->setNumClockRoots(0);
  _options->setNumClockSubnets(0);
}

void TritonCTS::importCharacterization()
{
  std::cout << " *****************************\n";
  std::cout << " *  Import characterization  *\n";
  std::cout << " *****************************\n";

  _techChar->parse(_options->getLutFile(), _options->getSolListFile());
}

void TritonCTS::createCharacterization()
{
  std::cout << " *****************************\n";
  std::cout << " *  Create characterization  *\n";
  std::cout << " *****************************\n";

  _techChar->create();
}

void TritonCTS::checkCharacterization()
{
  std::cout << " ****************************\n";
  std::cout << " *  Check characterization  *\n";
  std::cout << " ****************************\n";

  std::unordered_set<std::string> visitedMasters;
  _techChar->forEachWireSegment([&](unsigned idx, const WireSegment& wireSeg) {
    for (int buf = 0; buf < wireSeg.getNumBuffers(); ++buf) {
      std::string master = wireSeg.getBufferMaster(buf);
      if (visitedMasters.count(master) == 0) {
        if (masterExists(master)) {
          visitedMasters.insert(master);
        } else {
          error(("Buffer " + master + " is not in the loaded DB.\n").c_str());
        }
      }
    }
  });

  std::cout << "    The chacterization used " << visitedMasters.size()
            << " buffer(s) types."
            << " All of them are in the loaded DB.\n";
}

void TritonCTS::findClockRoots()
{
  std::cout << " **********************\n";
  std::cout << " *  Find clock roots  *\n";
  std::cout << " **********************\n";

  if (_options->getClockNets() != "") {
    std::cout << " Running TritonCTS with user-specified clock roots: ";
    std::cout << _options->getClockNets() << "\n";
    return;
  }

  std::cout << " User did not specify clock roots.\n";
  _staEngine->init();
}

void TritonCTS::populateTritonCts()
{
  std::cout << " ************************\n";
  std::cout << " *  Populate TritonCTS  *\n";
  std::cout << " ************************\n";

  populateTritonCTS();

  if (_builders->size() < 1) {
    error("No valid clock nets in the design.\n");
  }
}

void TritonCTS::buildClockTrees()
{
  std::cout << " ***********************\n";
  std::cout << " *  Build clock trees  *\n";
  std::cout << " ***********************\n";

  for (TreeBuilder* builder : *_builders) {
    builder->setTechChar(*_techChar);
    builder->run();
  }
}

void TritonCTS::runPostCtsOpt()
{
  if (!_options->runPostCtsOpt()) {
    return;
  }

  std::cout << " ****************\n";
  std::cout << " * Post CTS opt *\n";
  std::cout << " ****************\n";

  for (TreeBuilder* builder : *_builders) {
    PostCtsOpt opt(builder->getClock(), _options);
    opt.run();
  }
}

void TritonCTS::writeDataToDb()
{
  std::cout << " ********************\n";
  std::cout << " * Write data to DB *\n";
  std::cout << " ********************\n";

  for (TreeBuilder* builder : *_builders) {
    writeClockNetsToDb(builder->getClock());
  }
}

void TritonCTS::forEachBuilder(
    const std::function<void(const TreeBuilder*)> func) const
{
  for (const TreeBuilder* builder : *_builders) {
    func(builder);
  }
}

void TritonCTS::printFooter() const
{
  std::cout << " ... End of TritonCTS execution.\n";
}

void TritonCTS::reportCtsMetrics()
{
  std::string filename = _options->getMetricsFile();

  if (filename != "") {
    std::ofstream file(filename.c_str());

    if (!file.is_open()) {
      std::cout << "Could not open output metric file.\n";
      return;
    }

    file << "[TritonCTS Metrics] Total number of Clock Roots: "
         << _options->getNumClockRoots() << ".\n";
    file << "[TritonCTS Metrics] Total number of Buffers Inserted: "
         << _options->getNumBuffersInserted() << ".\n";
    file << "[TritonCTS Metrics] Total number of Clock Subnets: "
         << _options->getNumClockSubnets() << ".\n";
    file << "[TritonCTS Metrics] Total number of Sinks: "
         << _options->getNumSinks() << ".\n";

    file.close();
  } else {
    std::cout << "[TritonCTS Metrics] Total number of Clock Roots: "
              << _options->getNumClockRoots() << ".\n";
    std::cout << "[TritonCTS Metrics] Total number of Buffers Inserted: "
              << _options->getNumBuffersInserted() << ".\n";
    std::cout << "[TritonCTS Metrics] Total number of Clock Subnets: "
              << _options->getNumClockSubnets() << ".\n";
    std::cout << "[TritonCTS Metrics] Total number of Sinks: "
              << _options->getNumSinks() << ".\n";
  }
}

int TritonCTS::setClockNets(const char* names)
{
  odb::dbDatabase* db = _openroad->getDb();
  odb::dbChip* chip = db->getChip();
  odb::dbBlock* block = chip->getBlock();

  _options->setClockNets(names);
  std::stringstream ss(names);
  std::istream_iterator<std::string> begin(ss);
  std::istream_iterator<std::string> end;
  std::vector<std::string> nets(begin, end);

  std::vector<odb::dbNet*> netObjects;

  for (std::string name : nets) {
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
  _options->setClockNetsObjs(netObjects);
  return 0;
}

void TritonCTS::setBufferList(const char* buffers)
{
  std::stringstream ss(buffers);
  std::istream_iterator<std::string> begin(ss);
  std::istream_iterator<std::string> end;
  std::vector<std::string> bufferVector(begin, end);
  _options->setBufferList(bufferVector);
}

// db functions

void TritonCTS::populateTritonCTS()
{
  initDB();
  initAllClocks();
}

void TritonCTS::initDB()
{
  _openSta = _openroad->getSta();
  _block = _db->getChip()->getBlock();
  _options->setDbUnits(_block->getDbUnitsPerMicron());
}

void TritonCTS::initAllClocks()
{
  std::cout << " Initializing clock nets\n";

  clearNumClocks();

  std::cout << " Looking for clock nets in the design\n";

  // Uses dbSta to find all clock nets in the design.

  std::set<odb::dbNet*> clockNets;

  // Checks the user input in case there are other nets that need to be added to
  // the set.
  std::vector<odb::dbNet*> inputClkNets = _options->getClockNetsObjs();

  if (inputClkNets.size() != 0) {
    for (odb::dbNet* net : inputClkNets) {
      // Since a set is unique, only the nets not found by dbSta are added.
      clockNets.insert(net);
    }
  } else {
    clockNets = _openSta->findClkNets();
  }

  // Iterate over all the nets found by the user-input and dbSta
  for (odb::dbNet* net : clockNets) {
    if (net != nullptr) {
      std::cout << " Net \"" << net->getName() << "\" found\n";
      // Initializes the net in TritonCTS. If the number of sinks is less than
      // 2, the net is discarded.
      initClock(net);
    } else {
      std::cout
          << " [WARNING] A net was not found in the design. Skipping...\n";
    }
  }

  if (getNumClocks() == 0) {
    error("No clock nets have been found.\n");
  }

  std::cout << " TritonCTS found " << getNumClocks() << " clock nets."
            << std::endl;
  _options->setNumClockRoots(getNumClocks());
}

void TritonCTS::initClock(odb::dbNet* net)
{
  std::string driver = "";
  odb::dbITerm* iterm = net->getFirstOutput();
  int xPin, yPin;
  if (iterm == nullptr) {
    odb::dbBTerm* bterm = net->get1stBTerm();  // Clock pin
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
  std::cout << " Initializing clock net for : \"" << net->getConstName() << "\""
            << std::endl;

  Clock clockNet(net->getConstName(), driver, xPin, yPin);

  for (odb::dbITerm* iterm : net->getITerms()) {
    odb::dbInst* inst = iterm->getInst();

    if (iterm->isInputSignal() && inst->isPlaced()) {
      odb::dbMTerm* mterm = iterm->getMTerm();
      std::string name = std::string(inst->getConstName()) + "/"
                         + std::string(mterm->getConstName());
      int x, y;
      computeITermPosition(iterm, x, y);
      clockNet.addSink(name, x, y, iterm);
    }
  }

  if (clockNet.getNumSinks() < 2) {
    std::cout << "    [WARNING] Net \"" << clockNet.getName() << "\""
              << " has " << clockNet.getNumSinks() << " sinks. Skipping...\n";
    return;
  } else {
    if (clockNet.getNumSinks() == 0) {
      std::cout << "    [WARNING] Net \"" << clockNet.getName() << "\""
                << " has 0 sinks. Disconnected net or unplaced sink instances. "
                   "Skipping...\n";
      return;
    }
  }

  std::cout << " Clock net \"" << net->getConstName() << "\" has "
            << clockNet.getNumSinks() << " sinks" << std::endl;

  long int totalSinks = _options->getNumSinks() + clockNet.getNumSinks();
  _options->setNumSinks(totalSinks);

  incrementNumClocks();

  clockNet.setNetObj(net);

  addBuilder(new HTreeBuilder(_options, clockNet));
}

void TritonCTS::parseClockNames(std::vector<std::string>& clockNetNames) const
{
  std::stringstream allNames(_options->getClockNets());

  std::string tmpName = "";
  while (allNames >> tmpName) {
    clockNetNames.push_back(tmpName);
  }

  unsigned numClocks = clockNetNames.size();
  std::cout << " Number of user-input clocks: " << numClocks << ".\n";

  if (numClocks > 0) {
    std::cout << " (";
    for (const std::string& name : clockNetNames) {
      std::cout << " \"" << name << "\"";
    }
    std::cout << " )\n";
  }
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

void TritonCTS::writeClockNetsToDb(Clock& clockNet)
{
  std::cout << " Writing clock net \"" << clockNet.getName() << "\" to DB\n";
  odb::dbNet* topClockNet = clockNet.getNetObj();

  disconnectAllSinksFromNet(topClockNet);

  createClockBuffers(clockNet);

  // connect top buffer on the clock pin
  std::string topClockInstName = "clkbuf_0_" + clockNet.getName();
  odb::dbInst* topClockInst = _block->findInst(topClockInstName.c_str());
  odb::dbITerm* topClockInstInputPin = getFirstInput(topClockInst);
  odb::dbITerm::connect(topClockInstInputPin, topClockNet);
  topClockNet->setSigType(odb::dbSigType::CLOCK);

  std::map<int, uint> fanoutcount;

  // create subNets
  _numClkNets = 0;
  _numFixedNets = 0;
  const Clock::SubNet* rootSubNet = nullptr;
  clockNet.forEachSubNet([&](const Clock::SubNet& subNet) {
    bool outputPinFound = true;
    bool inputPinFound = true;
    bool leafLevelNet = subNet.isLeafLevel();
    // std::cout << "    SubNet: " << subNet.getName() << "\n";
    if (("clknet_0_" + clockNet.getName()) == subNet.getName()) {
      rootSubNet = &subNet;
    }
    odb::dbNet* clkSubNet
        = odb::dbNet::create(_block, subNet.getName().c_str());
    ++_numClkNets;
    clkSubNet->setSigType(odb::dbSigType::CLOCK);

    // std::cout << "      Driver: " << subNet.getDriver()->getName() << "\n";

    odb::dbInst* driver = subNet.getDriver()->getDbInst();
    odb::dbITerm* driverInputPin = getFirstInput(driver);
    odb::dbNet* inputNet = driverInputPin->getNet();
    odb::dbITerm* outputPin = driver->getFirstOutput();
    if (outputPin == nullptr) {
      outputPinFound = false;
    }
    odb::dbITerm::connect(outputPin, clkSubNet);

    if (subNet.getNumSinks() == 0) {
      inputPinFound = false;
    }

    subNet.forEachSink([&](ClockInst* inst) {
      // std::cout << "      " << inst->getName() << "\n";
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
      odb::dbITerm::connect(inputPin, clkSubNet);
    });

    if (leafLevelNet) {
      // Report fanout values only for sink nets
      if (fanoutcount.find(subNet.getNumSinks()) == fanoutcount.end()) {
        fanoutcount[subNet.getNumSinks()] = 0;
      }
      fanoutcount[subNet.getNumSinks()] = fanoutcount[subNet.getNumSinks()] + 1;
    }

    if (!inputPinFound || !outputPinFound) {
      // Net not fully connected. Removing it.
      disconnectAllPinsFromNet(clkSubNet);
      odb::dbNet::destroy(clkSubNet);
      _numFixedNets++;
      --_numClkNets;
      odb::dbInst::destroy(driver);
      checkUpstreamConnections(inputNet);
    }
  });

  std::string fanoutDistString
      = "    Fanout distribution for the current clock = ";
  std::string fanout = "";
  for (auto const& x : fanoutcount) {
    fanout = fanout + std::to_string(x.first) + ':' + std::to_string(x.second)
             + ", ";
  }

  fanoutDistString
      = fanoutDistString + fanout.substr(0, fanout.size() - 2) + ".";

  if (_options->writeOnlyClockNets()) {
    removeNonClockNets();
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

  std::cout << "    Minimum number of buffers in the clock path: " << minPath
            << ".\n";
  std::cout << "    Maximum number of buffers in the clock path: " << maxPath
            << ".\n";

  if (_numFixedNets > 0) {
    std::cout << "    " << _numFixedNets << " clock nets were removed/fixed.\n";
  }

  std::cout << "    Created " << _numClkNets << " clock nets.\n";
  long int totalNets = _options->getNumClockSubnets() + _numClkNets;
  _options->setNumClockSubnets(totalNets);

  std::cout << fanoutDistString << std::endl;
  std::cout << "    Max level of the clock tree: " << clockNet.getMaxLevel()
            << ".\n";
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
      odb::dbITerm::disconnect(iterm);
    }
  }
}

void TritonCTS::disconnectAllPinsFromNet(odb::dbNet* net)
{
  odb::dbSet<odb::dbITerm> iterms = net->getITerms();
  for (odb::dbITerm* iterm : iterms) {
    odb::dbITerm::disconnect(iterm);
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
    } else {
      odb::dbInst* bufferInst = firstITerm->getInst();
      odb::dbITerm* driverInputPin = getFirstInput(bufferInst);
      disconnectAllPinsFromNet(net);
      odb::dbNet::destroy(net);
      net = driverInputPin->getNet();
      ++_numFixedNets;
      --_numClkNets;
      odb::dbInst::destroy(bufferInst);
    }
  }
}

void TritonCTS::createClockBuffers(Clock& clockNet)
{
  unsigned numBuffers = 0;
  clockNet.forEachClockBuffer([&](ClockInst& inst) {
    odb::dbMaster* master = _db->findMaster(inst.getMaster().c_str());
    odb::dbInst* newInst
        = odb::dbInst::create(_block, master, inst.getName().c_str());
    inst.setInstObj(newInst);
    inst.setInputPinObj(getFirstInput(newInst));
    newInst->setLocation(inst.getX(), inst.getY());
    newInst->setPlacementStatus(odb::dbPlacementStatus::PLACED);
    ++numBuffers;
  });
  std::cout << "    Created " << numBuffers << " clock buffers.\n";
  long int totalBuffers = _options->getNumBuffersInserted() + numBuffers;
  _options->setNumBuffersInserted(totalBuffers);
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

bool TritonCTS::masterExists(const std::string& master) const
{
  return _db->findMaster(master.c_str());
};

void TritonCTS::removeNonClockNets()
{
  for (odb::dbNet* net : _block->getNets()) {
    if (net->getSigType() != odb::dbSigType::CLOCK) {
      odb::dbNet::destroy(net);
    }
  }
}

}  // namespace cts
