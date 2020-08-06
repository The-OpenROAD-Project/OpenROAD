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


#include "DbWrapper.h"
#include "TritonCTSKernel.h"
#include "HTreeBuilder.h"
#include "db_sta/dbSta.hh"
#include "openroad/Error.hh"
#include "sta/Sdc.hh"
#include "sta/Clock.hh"
#include "sta/Set.hh"

// DB includes
#include "db.h"
#include "dbShape.h"
#include "dbTypes.h"

#include <iostream>
#include <sstream>

namespace TritonCTS {

using ord::error;

DbWrapper::DbWrapper(CtsOptions& options,
                     TritonCTSKernel& kernel) {
        _options  = &options;
        _kernel = &kernel;
}

void DbWrapper::populateTritonCTS() {
       initDB();
       initAllClocks(); 
}

void DbWrapper::initDB() {
        ord::OpenRoad* openRoad = ord::OpenRoad::openRoad();
        _openSta = openRoad->getSta();
        _db    = odb::dbDatabase::getDatabase(_options->getDbId());
        _chip  = _db->getChip();
        _block = _chip->getBlock();
        _options->setDbUnits(_block->getDbUnitsPerMicron());
}

void DbWrapper::initAllClocks() {
        std::cout << " Initializing clock nets\n";

        clearNumClocks();

        std::cout << " Looking for clock nets in the design\n";

        //Uses dbSta to find all clock nets in the design.

        std::set<odb::dbNet*> clockNets;

        //Checks the user input in case there are other nets that need to be added to the set.
        std::vector<odb::dbNet*> inputClkNets = _options->getClockNetsObjs();

        if (inputClkNets.size() != 0){
                for (odb::dbNet* net : inputClkNets) {
                        //Since a set is unique, only the nets not found by dbSta are added.
                        clockNets.insert(net);
                }
        } else {
                _openSta->findClkNets(clockNets);
        }

        //Iterate over all the nets found by the user-input and dbSta
        for (odb::dbNet* net : clockNets){
                if (net != nullptr) {
                        std::cout << " Net \"" << net->getName() << "\" found\n";
                        //Initializes the net in TritonCTS. If the number of sinks is less than 2, the net is discarded.
                        initClock(net);
                } else {
                        std::cout << " [WARNING] A net was not found in the design. Skipping...\n";
                }
        }

        if (getNumClocks() == 0) {
                error("No clock nets have been found.\n");
        }

        std::cout << " TritonCTS found " << getNumClocks() << " clock nets." << std::endl;
        _options->setNumClockRoots(getNumClocks());
}       

void DbWrapper::initClock(odb::dbNet* net) {
        std::string driver = "";
        odb::dbITerm* iterm = net->getFirstOutput();
        int xPin, yPin;
        if (iterm == nullptr) {
                odb::dbBTerm* bterm = net->get1stBTerm(); // Clock pin
                driver = bterm->getConstName();
                bterm->getFirstPinLocation(xPin, yPin);
         } else {
                odb::dbInst* inst = iterm->getInst();
                odb::dbMTerm* mterm = iterm->getMTerm();
                std::string driver = std::string(inst->getConstName()) + "/" +  
                                   std::string(mterm->getConstName());
                DBU xTmp, yTmp;
                computeITermPosition(iterm, xTmp, yTmp);
                xPin = xTmp; yPin = yTmp;
        }

        // Initialize clock net
        std::cout << " Initializing clock net for : \"" << net->getConstName() << "\"" << std::endl;
        
        Clock clockNet(net->getConstName(), 
                       driver,
                       xPin, yPin);
        
        for (odb::dbITerm* iterm : net->getITerms()) {
                
                if (!(iterm->isInputSignal())) {
                        continue;
                }

                odb::dbInst* inst = iterm->getInst();
                odb::dbMTerm* mterm = iterm->getMTerm();
                std::string name = std::string(inst->getConstName()) + "/" +  
                                   std::string(mterm->getConstName());
                DBU x, y;
                computeITermPosition(iterm, x, y);
                clockNet.addSink(name, x, y, iterm);
        }
        
        if (clockNet.getNumSinks() < 2) {
                std::cout << "    [WARNING] Net \"" << clockNet.getName() << "\""  
                          << " has " << clockNet.getNumSinks()  << " sinks. Skipping...\n";
                return;
        
        }

        std::cout << " Clock net \"" << net->getConstName() << "\" has " << clockNet.getNumSinks() << " sinks" << std::endl;

        long int currentTotalSinks = _options->getNumSinks() + clockNet.getNumSinks();
        _options->setNumSinks(currentTotalSinks);

        incrementNumClocks();

        clockNet.setNetObj(net);

        _kernel->addBuilder(new HTreeBuilder(*_options, clockNet));
}

void DbWrapper::parseClockNames(std::vector<std::string>& clockNetNames) const {
        std::stringstream allNames(_options->getClockNets());

        std::string tmpName = "";
        while (allNames >> tmpName) {
                clockNetNames.push_back(tmpName);
        } 
        
        unsigned numClocks = clockNetNames.size();
        std::cout << " Number of user-input clocks: " << numClocks << ".\n";

        if (numClocks > 0) {
                std::cout << " (";
                for (const std::string& name: clockNetNames) {
                        std::cout << " \"" << name << "\"";
                }
                std::cout << " )\n";
        } 
}

void DbWrapper::computeITermPosition(odb::dbITerm* term, DBU &x, DBU &y) const {
        odb::dbITermShapeItr itr;
        
        odb::dbShape shape;
        x = 0;
        y = 0;
        unsigned numShapes = 0;
        for (itr.begin(term); itr.next(shape); ) {
                if (shape.isVia()) {
                        continue;
                } else {
                        x += shape.xMin() + (shape.xMax() - shape.xMin()) / 2;
                        y += shape.yMin() + (shape.yMax() - shape.yMin()) / 2;
                        ++numShapes;                 
                }
        }
       if (numShapes > 0){
                x /= numShapes;
                y /= numShapes;
       }
};  

void DbWrapper::writeClockNetsToDb(Clock& clockNet) {
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

        std::map<int,uint> fanoutcount;

        // create subNets
        _numClkNets = 0; 
        _numFixedNets = 0;
        const Clock::SubNet* rootSubNet = nullptr;
        clockNet.forEachSubNet( [&] (const Clock::SubNet& subNet) {
                        bool outputPinFound = true;
                        bool inputPinFound = true;
                        bool leafLevelNet = subNet.isLeafLevel();
                        //std::cout << "    SubNet: " << subNet.getName() << "\n";
                        if (("clknet_0_" + clockNet.getName()) == subNet.getName()){
                                rootSubNet = &subNet;
                        }
                        odb::dbNet* clkSubNet = odb::dbNet::create(_block, subNet.getName().c_str());
                        ++_numClkNets;
                        clkSubNet->setSigType(odb::dbSigType::CLOCK);
                        
                        //std::cout << "      Driver: " << subNet.getDriver()->getName() << "\n";
                       
                        odb::dbInst* driver = subNet.getDriver()->getDbInst();
                        odb::dbITerm* driverInputPin = getFirstInput(driver);
                        odb::dbNet* inputNet = driverInputPin->getNet();
                        odb::dbITerm* outputPin = driver->getFirstOutput();
                        if (outputPin == nullptr) {
                                outputPinFound = false;
                        }
                        odb::dbITerm::connect(outputPin, clkSubNet); 

                        if (subNet.getNumSinks() == 0){
                                inputPinFound = false;
                        }

                        subNet.forEachSink( [&] (ClockInst *inst) {
                                //std::cout << "      " << inst->getName() << "\n";
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
                                        if (!inputPin->getInst()->isPlaced()){
                                                inputPinFound = false;
                                        }
                                }
                                odb::dbITerm::connect(inputPin, clkSubNet); 
                        });

                        if (leafLevelNet){
                                //Report fanout values only for sink nets
                                if ( fanoutcount.find(subNet.getNumSinks()) == fanoutcount.end() ) {
                                        fanoutcount[subNet.getNumSinks()] = 0;
                                }
                                fanoutcount[subNet.getNumSinks()] = fanoutcount[subNet.getNumSinks()] + 1;
                        }

                        if (inputPinFound == false || outputPinFound == false){
                                // Net not fully connected. Removing it.
                                disconnectAllPinsFromNet(clkSubNet);
                                odb::dbNet::destroy(clkSubNet);
                                _numFixedNets++;
                                --_numClkNets;
                                odb::dbInst::destroy(driver);
                                checkUpstreamConnections(inputNet);
                        }
                });       
        
        std::string fanoutDistString = "    Fanout distribution for the current clock = ";
        std::string currentFanout = "";
        for (auto const& x : fanoutcount){
                currentFanout = currentFanout + std::to_string(x.first) + ':' + std::to_string(x.second) + ", ";
                
        }

        fanoutDistString = fanoutDistString + currentFanout.substr(0,currentFanout.size() - 2) + ".";
        
        if (_options->writeOnlyClockNets()) {
                removeNonClockNets();
        }

        int minPath = std::numeric_limits<int>::max();
        int maxPath = std::numeric_limits<int>::min();
        rootSubNet->forEachSink( [&] (ClockInst *inst) {
                if (inst->isClockBuffer()){
                        std::pair<int,int> resultsForBranch = branchBufferCount(inst, 1, clockNet);
                        if (resultsForBranch.first < minPath){
                                minPath = resultsForBranch.first;
                        }
                        if (resultsForBranch.second > maxPath){
                                maxPath = resultsForBranch.second;
                        }
                }
        });

        std::cout << "    Minimum number of buffers in the clock path: " << minPath << ".\n";
        std::cout << "    Maximum number of buffers in the clock path: " << maxPath << ".\n";

        if (_numFixedNets > 0){
                std::cout << "    " << _numFixedNets << " clock nets were removed/fixed.\n";
        }

        std::cout << "    Created " << _numClkNets << " clock nets.\n";
        long int currentTotalNets = _options->getNumClockSubnets() + _numClkNets;
        _options->setNumClockSubnets(currentTotalNets);
        
        std::cout << fanoutDistString << std::endl;
        std::cout << "    Max level of the clock tree: " << clockNet.getMaxLevel() << ".\n";
}

std::pair<int,int> DbWrapper::branchBufferCount(ClockInst *inst, int bufCounter, Clock& clockNet){
        odb::dbInst* sink = inst->getDbInst();
        odb::dbITerm* outITerm = sink->getFirstOutput();
        int minPath = std::numeric_limits<int>::max();
        int maxPath = std::numeric_limits<int>::min();
        for (odb::dbITerm* sinkITerms : outITerm->getNet()->getITerms()){
                if (sinkITerms != outITerm){
                        ClockInst* clockInst = clockNet.findClockByName(sinkITerms->getInst()->getName());
                        if (clockInst == nullptr){
                                int newResult = bufCounter + 1;
                                if (newResult > maxPath){
                                        maxPath = newResult;
                                }
                                if (newResult < minPath){
                                        minPath = newResult;
                                }
                        } else {
                                std::pair<int,int> newResults = branchBufferCount(clockInst, bufCounter + 1, clockNet);
                                if (newResults.first < minPath){
                                        minPath = newResults.first;
                                }
                                if (newResults.second > maxPath){
                                        maxPath = newResults.second;
                                }
                        }
                }
        }
        std::pair<int,int> currentResults (minPath,maxPath);
        return currentResults;
}

void DbWrapper::disconnectAllSinksFromNet(odb::dbNet* net) {
        odb::dbSet<odb::dbITerm> iterms = net->getITerms(); 
        odb::dbSet<odb::dbITerm>::iterator itr;
        for (itr = iterms.begin(); itr != iterms.end(); ++itr) {
                odb::dbITerm* iterm = *itr;
                if (iterm->getIoType() != odb::dbIoType::INPUT) {
                        continue;
                }
                odb::dbITerm::disconnect(iterm);
        }
}

void DbWrapper::disconnectAllPinsFromNet(odb::dbNet* net) {
        odb::dbSet<odb::dbITerm> iterms = net->getITerms(); 
        odb::dbSet<odb::dbITerm>::iterator itr;
        for (itr = iterms.begin(); itr != iterms.end(); ++itr) {
                odb::dbITerm* iterm = *itr;
                odb::dbITerm::disconnect(iterm);
        }
}

void DbWrapper::checkUpstreamConnections(odb::dbNet* net) {
        odb::dbNet* currentNet = net;
        while (currentNet->getITermCount() <= 1){
                //Net is incomplete, only 1 pin.
                odb::dbITerm* firstITerm = currentNet->get1stITerm();
                if (firstITerm == nullptr) {
                        disconnectAllPinsFromNet(currentNet);
                        odb::dbNet::destroy(currentNet);
                        break;
                } else {
                        odb::dbInst* bufferInst = firstITerm->getInst();
                        odb::dbITerm* driverInputPin = getFirstInput(bufferInst);
                        disconnectAllPinsFromNet(currentNet);
                        odb::dbNet::destroy(currentNet);
                        currentNet = driverInputPin->getNet();
                        ++_numFixedNets;
                        --_numClkNets;
                        odb::dbInst::destroy(bufferInst);
                }
        }

}

void DbWrapper::createClockBuffers(Clock& clockNet) {
        unsigned numBuffers = 0;
        clockNet.forEachClockBuffer([&] (ClockInst& inst) {
                odb::dbMaster* master = _db->findMaster(inst.getMaster().c_str());
                odb::dbInst* newInst = odb::dbInst::create(_block, master, inst.getName().c_str());
                inst.setInstObj(newInst);
                inst.setInputPinObj(getFirstInput(newInst));
                newInst->setLocation(inst.getX(), inst.getY());        
                newInst->setPlacementStatus(odb::dbPlacementStatus::PLACED);
                ++numBuffers;
        });
        std::cout << "    Created " << numBuffers << " clock buffers.\n";
        long int currentTotalBuffers = _options->getNumBuffersInserted() + numBuffers;
        _options->setNumBuffersInserted(currentTotalBuffers);
}

odb::dbITerm* DbWrapper::getFirstInput(odb::dbInst* inst) const {
        odb::dbSet<odb::dbITerm> iterms = inst->getITerms();
        odb::dbSet<odb::dbITerm>::iterator itr;
        
        for (itr = iterms.begin(); itr != iterms.end(); ++itr) {
                odb::dbITerm* iterm = *itr;
                if (iterm->isInputSignal()) {
                        return iterm;
                } 
        }
        
        return nullptr;
}

bool DbWrapper::masterExists(const std::string& master) const {
        return _db->findMaster(master.c_str()); 
};

void DbWrapper::removeNonClockNets() {
        for (odb::dbNet* net : _block->getNets()) {
                if (net->getSigType() != odb::dbSigType::CLOCK) {
                        odb::dbNet::destroy(net);
                } 
        } 
}

}
