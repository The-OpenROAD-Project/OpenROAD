////////////////////////////////////////////////////////////////////////////////////
// Authors: Mateus Fogaca
//          (Ph.D. advisor: Ricardo Reis)
//          Jiajia Li
//          Andrew Kahng
// Based on:
//          K. Han, A. B. Kahng and J. Li, "Optimal Generalized H-Tree Topology and 
//          Buffering for High-Performance and Low-Power Clock Distribution", 
//          IEEE Trans. on CAD (2018), doi:10.1109/TCAD.2018.2889756.
//
//
// BSD 3-Clause License
//
// Copyright (c) 2018, The Regents of the University of California
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
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
// SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
// CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
// OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
////////////////////////////////////////////////////////////////////////////////////

#include "DbWrapper.h"
#include "TritonCTSKernel.h"
#include "HTreeBuilder.h"

// DB includes
#include "db.h"
#include "dbShape.h"
#include "dbTypes.h"

#include <iostream>
#include <sstream>

namespace TritonCTS {

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
        _db    = odb::dbDatabase::getDatabase(_options->getDbId());
        _chip  = _db->getChip();
        _block = _chip->getBlock();
        _options->setDbUnits(_block->getDbUnitsPerMicron());
}

void DbWrapper::initAllClocks() {
        std::cout << " Initializing clock nets\n";
        
        std::vector<std::string> clockNetNames;
        parseClockNames(clockNetNames);

        std::cout << " Looking for clock nets in the design\n";
        for (const std::string& name: clockNetNames) {
                odb::dbNet* net = _block->findNet(name.c_str());
                if (!net) {
                        odb::dbITerm* iterm = _block->findITerm(name.c_str());
                        net = iterm->getNet();
                }
                if (!net) {
                        std::cout << " [WARNING] Net \"" << name << "\" not in design. Skipping...\n";
                        continue;
                } 
                
                std::cout << " Net \"" << name << "\" found\n";
                initClock(net);
        }
}       

void DbWrapper::initClock(odb::dbNet* net) {
        std::string driver = "";
        odb::dbITerm* iterm = net->getFirstOutput();
        int xPin, yPin;
        if (!iterm) {
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
        std::cout << net->getConstName() << std::endl;
        
        Clock clockNet(net->getConstName(), 
                       driver,
                       xPin, yPin);
        
        odb::dbSet<odb::dbITerm> iterms = net->getITerms(); 
        odb::dbSet<odb::dbITerm>::iterator itr;
        for (itr = iterms.begin(); itr != iterms.end(); ++itr) {
                odb::dbITerm* iterm = *itr;
                
		if (iterm->getIoType() != odb::dbIoType::INPUT) {
                        continue;
                }
                
		odb::dbInst* inst = iterm->getInst();
                odb::dbMTerm* mterm = iterm->getMTerm();
                std::string name = std::string(inst->getConstName()) + "/" +  
                                   std::string(mterm->getConstName());
                DBU x, y;
                computeITermPosition(iterm, x, y);
                clockNet.addSink(name, x, y);
        }
        
        if (clockNet.getNumSinks() < 2) {
                std::cout << "    [WARNING] Net \"" << clockNet.getName() << "\""  
                          << " has " << clockNet.getNumSinks()  << " sinks. Skipping...\n";
                return;
        
        }

        _kernel->addBuilder(new HTreeBuilder(*_options, clockNet));
}

void DbWrapper::parseClockNames(std::vector<std::string>& clockNetNames) const {
        std::stringstream allNames(_options->getClockNets());

        std::string tmpName = "";
        while (allNames >> tmpName) {
                clockNetNames.push_back(tmpName);
        } 
        
        unsigned numClocks = clockNetNames.size();
        std::cout << " Number of user-input clocks: " << numClocks;

        if (numClocks > 0) {
                std::cout << " (";
                for (const std::string& name: clockNetNames) {
                        std::cout << " \"" << name << "\"";
                }
                std::cout << " )\n";
        } else {
                std::cout << "\n";
                std::cout << " [ERROR] No clock nets have been found.\n";
                std::exit(1);

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

        x /= numShapes;
        y /= numShapes;
};  

void DbWrapper::writeClockNetsToDb(const Clock& clockNet) {
        std::cout << " Writing clock net \"" << clockNet.getName() << "\" to DB\n";
                
        disconnectAllSinksFromNet(clockNet.getName());
        
        createClockBuffers(clockNet);   
        
        // connect top buffer on the clock pin
        odb::dbNet* topClockNet = _block->findNet(clockNet.getName().c_str());
        std::string topClockInstName = "clkbuf_0_" + clockNet.getName();
        odb::dbInst* topClockInst = _block->findInst(topClockInstName.c_str());
        odb::dbITerm* topClockInstInputPin = getFirstInput(topClockInst); 
        odb::dbITerm::connect(topClockInstInputPin, topClockNet); 
        topClockNet->setSigType(odb::dbSigType::CLOCK);

        // create subNets
        unsigned numClkNets = 0; 
        clockNet.forEachSubNet( [&] (const Clock::SubNet& subNet) {
                        //std::cout << "    SubNet: " << subNet.getName() << "\n";
                        odb::dbNet* clkSubNet = odb::dbNet::create(_block, subNet.getName().c_str());
                        ++numClkNets;
                        clkSubNet->setSigType(odb::dbSigType::CLOCK);
                        
                        //std::cout << "      Driver: " << subNet.getDriver()->getName() << "\n";
                       
                        odb::dbInst* driver = _block->findInst(subNet.getDriver()->getName().c_str());
                        odb::dbITerm* outputPin = driver->getFirstOutput();
                        odb::dbITerm::connect(outputPin, clkSubNet); 

                        subNet.forEachSink( [&] (ClockInst *inst) {
                                //std::cout << "      " << inst->getName() << "\n";
                                odb::dbITerm* inputPin = nullptr;
                                if (inst->isClockBuffer()) { 
                                        odb::dbInst* sink = _block->findInst(inst->getName().c_str());
                                        inputPin = getFirstInput(sink);
                                } else {
                                        inputPin = _block->findITerm(inst->getName().c_str());
                                }
                                odb::dbITerm::connect(inputPin, clkSubNet); 
                        });
                });       
        
        if (_options->writeOnlyClockNets()) {
                removeNonClockNets();
        }

        std::cout << "    Created " << numClkNets << " clock nets.\n";
}

void DbWrapper::disconnectAllSinksFromNet(std::string netName) {
        odb::dbNet* net = _block->findNet(netName.c_str());
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

void DbWrapper::createClockBuffers(const Clock& clockNet) {
        unsigned numBuffers = 0;
        clockNet.forEachClockBuffer([&] (const ClockInst& inst) {
                odb::dbMaster* master = _db->findMaster(inst.getMaster().c_str());
                odb::dbInst* newInst = odb::dbInst::create(_block, master, inst.getName().c_str());
                newInst->setLocation(inst.getX(), inst.getY());        
                newInst->setPlacementStatus(odb::dbPlacementStatus::PLACED);
                ++numBuffers;
        });
        std::cout << "    Created " << numBuffers << " clock buffers.\n";
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
