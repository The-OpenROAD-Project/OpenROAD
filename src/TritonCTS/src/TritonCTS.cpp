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
#include "openroad/Error.hh"
#include "PostCtsOpt.h"
#include "CtsOptions.h"
#include "DbWrapper.h"
#include "StaEngine.h"
#include "TechChar.h"
#include "TreeBuilder.h"

#include <chrono>
#include <ctime>
#include <fstream>
#include <unordered_set>

namespace cts {

using ord::error;

void TritonCTS::init(ord::OpenRoad* openroad)
{
  _openroad = openroad;
  makeComponents();
}

void TritonCTS::makeComponents()
{
  _options = new CtsOptions;
  _techChar = new TechChar(_options);
  _staEngine = new StaEngine(*_options);
  _dbWrapper = new DbWrapper(*_options, *this, _openroad->getDb());
  _builders = new std::vector<TreeBuilder*>;
}

void TritonCTS::deleteComponents()
{
  delete _options;
  delete _techChar;
  delete _staEngine;
  delete _dbWrapper;
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
        if (_dbWrapper->masterExists(master)) {
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

  _dbWrapper->populateTritonCTS();

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
    PostCtsOpt opt(builder->getClock(), *_options);
    opt.run();
  }
}

void TritonCTS::writeDataToDb()
{
  std::cout << " ********************\n";
  std::cout << " * Write data to DB *\n";
  std::cout << " ********************\n";

  for (TreeBuilder* builder : *_builders) {
    _dbWrapper->writeClockNetsToDb(builder->getClock());
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

}  // namespace cts
