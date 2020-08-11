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

#include "DBWrapper.h"

#include <iostream>
#include <vector>
#include <cassert> 

#include "db.h"
#include "lefin.h"
#include "defin.h"
#include "defout.h"
#include "dbShape.h"
#include "Coordinate.h"

namespace ioPlacer {

DBWrapper::DBWrapper(Netlist& netlist, Core& core, Parameters& parms) : 
                _netlist(&netlist), _core(&core), _parms(&parms) {
        _db = odb::dbDatabase::create();
}

void DBWrapper::parseLEF(const std::string &filename) {
        odb::lefin lefReader(_db, false);
        lefReader.createTechAndLib("testlib", filename.c_str());
}

void DBWrapper::parseDEF(const std::string &filename) {
        odb::defin defReader(_db);

        std::vector<odb::dbLib *> searchLibs;
        odb::dbSet<odb::dbLib> libs = _db->getLibs();
        odb::dbSet<odb::dbLib>::iterator itr;
        for(itr = libs.begin(); itr != libs.end(); ++itr) {
                searchLibs.push_back(*itr);
        }
        
        _chip = defReader.createChip(searchLibs, filename.c_str());
}

void DBWrapper::populateIOPlacer() {
	_db = odb::dbDatabase::getDatabase(_parms->getDbId());
	_chip = _db->getChip();
        initNetlist();
        initCore();
}


void DBWrapper::initCore() {
        odb::dbTech* tech = _db->getTech();
        if (!tech) {
                std::cout << "[ERROR] odb::dbTech not initialized! Exiting...\n";
                std::exit(1);
        }
        
        int databaseUnit = tech->getLefUnits(); 

        odb::dbBlock* block = _chip->getBlock();
        if (!block) {
                std::cout << "[ERROR] odb::dbBlock not found! Exiting...\n";
                std::exit(1);
        }

        odb::Rect rect;
        block->getDieArea(rect);
	
        Coordinate lowerBound(rect.xMin(), rect.yMin());
        Coordinate upperBound(rect.xMax(), rect.yMax());
        
        int horLayerIdx = _parms->getHorizontalMetalLayer();
        int verLayerIdx = _parms->getVerticalMetalLayer();

        odb::dbTechLayer* horLayer = tech->findRoutingLayer(horLayerIdx);
        if (!horLayer) {
                std::cout << "[ERROR] Layer" << horLayerIdx << " not found! Exiting...\n";
                std::exit(1);
        }

        odb::dbTechLayer* verLayer = tech->findRoutingLayer(verLayerIdx);
        if (!horLayer) {
                std::cout << "[ERROR] Layer" << verLayerIdx << " not found! Exiting...\n";
                std::exit(1);
        }

        odb::dbTrackGrid* horTrackGrid = block->findTrackGrid( horLayer );        
        odb::dbTrackGrid* verTrackGrid = block->findTrackGrid( verLayer );
        if (!horTrackGrid || !verTrackGrid ||
            horTrackGrid->getNumGridPatternsY() == 0 || 
            verTrackGrid->getNumGridPatternsX() == 0) {
                std::cout << "[ERROR] No track grid! Exiting...\n";
                std::exit(1);
        }

        int minSpacingX = 0;
        int minSpacingY = 0;
        int initTrackX = 0;
        int initTrackY = 0;
        int minAreaX = 0;
        int minAreaY = 0;
        int minWidthX = 0;
        int minWidthY = 0;
        
        int numTracksX = 0;
        int numTracksY = 0;
        verTrackGrid->getGridPatternX(0, initTrackX, numTracksX, minSpacingX);
        horTrackGrid->getGridPatternY(0, initTrackY, numTracksY, minSpacingY);

        minAreaX =  verLayer->getArea() * databaseUnit * databaseUnit;
        minWidthX = verLayer->getWidth();
        minAreaY =  horLayer->getArea() * databaseUnit * databaseUnit;
        minWidthY = horLayer->getWidth();

        *_core = Core(lowerBound, upperBound, minSpacingX, minSpacingY,
                      initTrackX, initTrackY, numTracksX, numTracksY, minAreaX, minAreaY,
                      minWidthX, minWidthY, databaseUnit);
        if(_verbose) {
                std::cout << "lowerBound: " << lowerBound.getX() << " " << lowerBound.getY() << "\n";
                std::cout << "upperBound: " << upperBound.getX() << " " << upperBound.getY() << "\n";
                std::cout << "minSpacingX: " << minSpacingX << "\n";
                std::cout << "minSpacingY: " << minSpacingY << "\n";
                std::cout << "initTrackX: " << initTrackX << "\n";
                std::cout << "initTrackY: " << initTrackY << "\n";
                std::cout << "numTracksX: " << numTracksX << "\n";
                std::cout << "numTracksY: " << numTracksY << "\n";
                std::cout << "minAreaX: " << minAreaX << "\n";
                std::cout << "minAreaY: " << minAreaY << "\n";
                std::cout << "minWidthX: " << minWidthX << "\n";
                std::cout << "minWidthY: " << minWidthY << "\n";
                std::cout << "databaseUnit: " << databaseUnit << "\n";
        }
}

void DBWrapper::initNetlist() {
        odb::dbBlock* block = _chip->getBlock();
        if (!block) {
                std::cout << "[ERROR] odb::dbBlock not found! Exiting...\n";
                std::exit(1);
        }

        odb::dbSet<odb::dbBTerm> bterms = block->getBTerms();

        if(bterms.size() == 0) {
                std::cout << "[ERROR] Design without pins. Exiting...\n";
                std::exit(1);
        }

        odb::dbSet<odb::dbBTerm>::iterator btIter;
        
        for(btIter = bterms.begin(); btIter != bterms.end(); ++btIter) {
                odb::dbBTerm* curBTerm = *btIter;
                odb::dbNet* net =  curBTerm->getNet();
                if (!net) {
                        std::cout << "[WARNING] Pin " << curBTerm->getConstName()
                                  << " without net!\n";
                }

                Direction dir = DIR_INOUT;
                switch( curBTerm->getIoType() ) {
                        case odb::dbIoType::INPUT:
                                dir = DIR_IN;
                                break;
                        case odb::dbIoType::OUTPUT:
                                dir = DIR_OUT;
                                break;
                        default:
                                dir = DIR_INOUT;
                }

                int xPos = 0;
                int yPos = 0;
                curBTerm->getFirstPinLocation( xPos, yPos );
               
                Coordinate bounds(0, 0);
                IOPin ioPin( curBTerm->getConstName(), 
                             Coordinate(xPos, yPos), 
                             dir, bounds, bounds, net->getConstName(),
                             "FIXED" );

                std::vector<InstancePin> instPins;
                odb::dbSet<odb::dbITerm> iterms = net->getITerms();
                odb::dbSet<odb::dbITerm>::iterator iIter;
                for(iIter = iterms.begin(); iIter != iterms.end(); ++iIter) {
                        odb::dbITerm* curITerm = *iIter;
                        odb::dbInst* inst = curITerm->getInst();
                        int instX = 0, instY = 0;
                        inst->getLocation(instX, instY);
                        
                        instPins.push_back(InstancePin(inst->getConstName(), 
                                                       Coordinate(instX, instY)));
                }

                _netlist->addIONet(ioPin, instPins);
        }
}

void DBWrapper::commitIOPlacementToDB(std::vector<IOPin>& assignment) {
        odb::dbBlock* block = _chip->getBlock();
        if (!block) {
                std::cout << "[ERROR] odb::dbBlock not found! Exiting...\n";
                std::exit(1);
        }
       
        odb::dbTech* tech = _db->getTech();
        if (!tech) {
                std::cout << "[ERROR] odb::dbTech not initialized! Exiting...\n";
                std::exit(1);
        }
 
        int horLayerIdx = _parms->getHorizontalMetalLayer();
        int verLayerIdx = _parms->getVerticalMetalLayer();

        odb::dbTechLayer* horLayer = tech->findRoutingLayer(horLayerIdx);
        if (!horLayer) {
                std::cout << "[ERROR] Layer" << horLayerIdx << " not found! Exiting...\n";
                std::exit(1);
        }

        odb::dbTechLayer* verLayer = tech->findRoutingLayer(verLayerIdx);
        if (!horLayer) {
                std::cout << "[ERROR] Layer" << verLayerIdx << " not found! Exiting...\n";
                std::exit(1);
        }


        for (IOPin& pin: assignment) {
                odb::dbBTerm* bterm = block->findBTerm(pin.getName().c_str());
                odb::dbSet<odb::dbBPin> bpins = bterm->getBPins();
                odb::dbSet<odb::dbBPin>::iterator bpinIter;
                std::vector<odb::dbBPin*> allBPins;
                for(bpinIter = bpins.begin(); bpinIter != bpins.end(); ++bpinIter) {
                        odb::dbBPin* curBPin = *bpinIter;
                        allBPins.push_back(curBPin);
                } 
                               
                for(odb::dbBPin* bpin: allBPins) {
                        odb::dbBPin::destroy(bpin);
                } 
                       
                Coordinate lowerBound = pin.getLowerBound();
                Coordinate upperBound = pin.getUpperBound();

                odb::dbBPin* bpin = odb::dbBPin::create(bterm);
                
                int size = upperBound.getX() - lowerBound.getX();

                int xMin = lowerBound.getX();
                int yMin = lowerBound.getY();
                int xMax = upperBound.getX();
                int yMax = upperBound.getY();
                odb::dbTechLayer* layer = verLayer;
                if (pin.getOrientation() == Orientation::ORIENT_EAST ||
                    pin.getOrientation() == Orientation::ORIENT_WEST) {
                        layer = horLayer;
                }
                
                odb::dbBox::create(bpin, layer, xMin, yMin, xMax, yMax);
                bpin->setPlacementStatus(odb::dbPlacementStatus::PLACED);
        };
}

void DBWrapper::writeDEF() {
        odb::dbBlock* block = _chip->getBlock();
        
        odb::defout writer;
        
        std::string defFileName = _parms->getOutputDefFile();
        
        writer.setVersion( odb::defout::DEF_5_6 );
        writer.writeBlock( block, defFileName.c_str() );
}

}
