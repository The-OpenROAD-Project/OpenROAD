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

#include "TechChar.h"
#include "openroad/Error.hh"

#include "sta/Sdc.hh"
#include "sta/Liberty.hh"
#include "sta/PathAnalysisPt.hh"
#include "sta/Units.hh"
#include "sta/Search.hh"
#include "sta/Power.hh"
#include "sta/Graph.hh"
#include "db_sta/dbSta.hh"

#include <fstream>
#include <ostream>
#include <iomanip>
#include <sstream>
#include <algorithm>

namespace TritonCTS {

using ord::error;

void TechChar::compileLut(std::vector<TechChar::ResultData> lutSols) {
        std::cout << " Compiling LUT\n";
        initLengthUnits();
        
        _minSegmentLength = toInternalLengthUnit(_minSegmentLength); 
        _maxSegmentLength = toInternalLengthUnit(_maxSegmentLength); 
        
        reportCharacterizationBounds(); //min and max values already set
        checkCharacterizationBounds();

        unsigned noSlewDegradationCount = 0;
        _actualMinInputCap = std::numeric_limits<unsigned>::max();
        //For the results in each wire segment...
        for (ResultData lutLine : lutSols){

                _actualMinInputCap = std::min(static_cast<unsigned> (lutLine.totalcap) , _actualMinInputCap);
                //Checks the output slew of the wiresegment.
                if (lutLine.isPureWire && lutLine.pinSlew <= lutLine.inSlew) {
                        ++noSlewDegradationCount;
                        ++lutLine.pinSlew;
                }

                unsigned length = toInternalLengthUnit(lutLine.wirelength);

                WireSegment& segment = createWireSegment((unsigned) (length), (unsigned) (lutLine.load), 
                                                         (unsigned) (lutLine.pinSlew), lutLine.totalPower,
                                                         (unsigned) (lutLine.pinArrival), (unsigned) (lutLine.totalcap), (unsigned) (lutLine.inSlew));
                
                if (lutLine.isPureWire) {
                        continue;         
                }
                //Goes through the topology of the wiresegment and defines the buffer locations and masters.
                int maxIndex = 0;
                if (lutLine.topology.size() % 2 == 0) {
                        maxIndex = lutLine.topology.size();
                } else {
                        maxIndex = lutLine.topology.size() - 1;
                }
                for (int topologyIndex = 0 ; topologyIndex < maxIndex ; topologyIndex++) {
                        std::string topologyS = lutLine.topology[topologyIndex];
                        //Each buffered topology always has a wire segment followed by a buffer.
                        if (_masterNames.find(topologyS) == _masterNames.end()){
                                //Is a number (i.e. a wire segment).
                                segment.addBuffer(std::stod(topologyS)); 
                        } else {
                                segment.addBufferMaster(topologyS);
                        }
                }
        }

        if (noSlewDegradationCount > 0) {
                std::cout << "    [WARNING] " << noSlewDegradationCount 
                          << " wires are pure wire and no slew degration.\n" 
                          << "    TritonCTS forced slew degradation on these wires.\n";
        }

        std::cout << "    Num wire segments: " << _wireSegments.size() << "\n";
        std::cout << "    Num keys in characterization LUT: " 
                  << _keyToWireSegments.size() << "\n";

        std::cout << "    Actual min input cap: " << _actualMinInputCap << "\n";

}

void TechChar::parseLut(const std::string& file) {
        std::cout << " Reading LUT file \"" << file << "\"\n";
        std::ifstream lutFile(file.c_str());
        
        if (!lutFile.is_open()) {
                error("Could not find LUT file.\n");
        }

        // First line of the LUT is a header with normalization values
        if (!(lutFile >> _minSegmentLength >> _maxSegmentLength >> _minCapacitance 
                      >> _maxCapacitance >> _minSlew >> _maxSlew)) {
                error("Problem reading the LUT file.\n");
        }
        
        if (_options->getWireSegmentUnit() == 0){
                unsigned presetWireUnit = 0;
                if (!(lutFile >> presetWireUnit)) {
                        error("Problem reading the LUT file.\n");
                }
                if (presetWireUnit == 0) {
                        error("Problem reading the LUT file.\n");
                }
                _options->setWireSegmentUnit(presetWireUnit);
                setLenghthUnit(static_cast<unsigned> (presetWireUnit)/2);
        }
        
        initLengthUnits();
        
        _minSegmentLength = toInternalLengthUnit(_minSegmentLength); 
        _maxSegmentLength = toInternalLengthUnit(_maxSegmentLength); 

        reportCharacterizationBounds();
        checkCharacterizationBounds();

        std::string line;
        std::getline(lutFile, line); // Ignore first line
        unsigned noSlewDegradationCount = 0;
        _actualMinInputCap = std::numeric_limits<unsigned>::max();
        while (std::getline(lutFile, line)) {
                unsigned idx, delay;
                double power;
                unsigned  length, load, outputSlew, inputCap, inputSlew;
                bool isPureWire;
                
                std::stringstream ss(line);
                ss >> idx >> length >> load >> outputSlew >> power >> delay 
                   >> inputCap >> inputSlew >> isPureWire;
              
                length = toInternalLengthUnit(length);

                _actualMinInputCap = std::min(inputCap, _actualMinInputCap);                 

                if (isPureWire && outputSlew <= inputSlew) {
                        ++noSlewDegradationCount;
                        ++outputSlew;
                }

                WireSegment& segment = createWireSegment(length, load, outputSlew, 
                                                         power, delay, inputCap, inputSlew);
                
                if (isPureWire) {
                        continue;         
                }
                
                double bufferLocation;
                while (ss >> bufferLocation) {
                        segment.addBuffer(bufferLocation); 
                }
        }

        if (noSlewDegradationCount > 0) {
                std::cout << "    [WARNING] " << noSlewDegradationCount 
                          << " wires are pure wire and no slew degration.\n" 
                          << "    TritonCTS forced slew degradation on these wires.\n";
        }

        std::cout << "    Num wire segments: " << _wireSegments.size() << "\n";
        std::cout << "    Num keys in characterization LUT: " 
                  << _keyToWireSegments.size() << "\n";

        std::cout << "    Actual min input cap: " << _actualMinInputCap << "\n";
}

void TechChar::parse(const std::string& lutFile, const std::string solListFile) {
        parseLut(lutFile);
        parseSolList(solListFile);
}

void TechChar::initLengthUnits() {
        _charLengthUnit = _options->getWireSegmentUnit();
        _lengthUnit = LENGTH_UNIT_MICRON;
        _lengthUnitRatio = _charLengthUnit / _lengthUnit;
}

inline
void TechChar::reportCharacterizationBounds() const {
        std::cout << std::setw(12) << "Min. len" << std::setw(12) << "Max. len" 
                  << std::setw(12) << "Min. cap"  << std::setw(12) << "Max. cap"
                  << std::setw(12) << "Min. slew" << std::setw(12) << "Max. slew" << "\n"; 
        
        std::cout << std::setw(12) << _minSegmentLength << std::setw(12) << _maxSegmentLength 
                  << std::setw(12) << _minCapacitance  << std::setw(12) << _maxCapacitance
                  << std::setw(12) << _minSlew << std::setw(12) << _maxSlew << "\n";
}

inline
void TechChar::checkCharacterizationBounds() const {
        if (_minSegmentLength > MAX_NORMALIZED_VAL || _maxSegmentLength > MAX_NORMALIZED_VAL ||
            _minCapacitance > MAX_NORMALIZED_VAL || _maxCapacitance > MAX_NORMALIZED_VAL ||
            _minSlew > MAX_NORMALIZED_VAL || _maxSlew > MAX_NORMALIZED_VAL) {
               error(("Normalized values in the LUT should be in the range [1, " +
                      std::to_string(MAX_NORMALIZED_VAL) + "\n Check the table " +
                      "above to see the normalization ranges and check " +
                      "your characterization configuration.\n").c_str());
        } 
}

inline
WireSegment& TechChar::createWireSegment(uint8_t length, uint8_t load, uint8_t outputSlew, 
                                                 double power, unsigned delay, uint8_t inputCap, 
                                                 uint8_t inputSlew) {
        _wireSegments.emplace_back(length, load, outputSlew, power, 
                                   delay, inputCap, inputSlew);
                
        unsigned segmentIdx = _wireSegments.size() - 1;
        unsigned key = computeKey(length, load, outputSlew);

        if (_keyToWireSegments.find(key) == _keyToWireSegments.end()) {
            _keyToWireSegments[key] = std::deque<unsigned>();        
        }              
                
        _keyToWireSegments[key].push_back(segmentIdx);
                
        return _wireSegments.back(); 
}

void TechChar::parseSolList(const std::string& file) {
        std::cout << " Reading solution list file \"" << file << "\"\n";
        std::ifstream solFile(file.c_str());

        unsigned solIdx = 0;
        std::string line;
        
        while (getline(solFile, line)) {
                std::stringstream ss(line);
                std::string token;
                unsigned numBuffers = 0;
                while (getline(ss, token, ',')) {
                        if (std::any_of(std::begin(token), std::end(token), ::isalpha)) {
				_wireSegments[solIdx].addBufferMaster(token);
                                ++numBuffers;
                        }
                }
                
                // Sanity check
                if (_wireSegments[solIdx].getNumBuffers() != numBuffers) {
                        error(("Number of buffers does not match on solution.\n" +
                               std::to_string(solIdx) + " " + std::to_string(numBuffers) + ".\n").c_str());
                }
               ++solIdx; 
        }
}


void TechChar::forEachWireSegment(const std::function<void(unsigned, const WireSegment&)> func) const {
        for (unsigned idx = 0; idx < _wireSegments.size(); ++idx) {
                func(idx, _wireSegments[idx]);
        }
};

void TechChar::forEachWireSegment(uint8_t length, uint8_t load, uint8_t outputSlew,
                                          const std::function<void(unsigned, const WireSegment&)> func) const {
        unsigned key = computeKey(length, load, outputSlew);
        
        if (_keyToWireSegments.find(key) == _keyToWireSegments.end()) {
               //std::cout << "    [WARNING] No wire with the following characterization: [" 
               //          << (unsigned) length << ", " << (unsigned) load << ", " << (unsigned) outputSlew << "]\n";
               return; 
        }

        const std::deque<unsigned> &wireSegmentsIdx = _keyToWireSegments.at(key);
        for (unsigned idx : wireSegmentsIdx) {
                func(idx, _wireSegments[idx]);
        }
};

void TechChar::report() const {
        std::cout << "\n";
        std::cout << "*********************************************************************\n";
        std::cout << "*                     Report Characterization                       *\n";
        std::cout << "*********************************************************************\n";
        std::cout << std::setw(5) << "Idx" 
                  << std::setw(5) << "Len"
                  << std::setw(5) << "Load"
                  << std::setw(10) << "Out slew"
                  << std::setw(12) << "Power" 
                  << std::setw(8) << "Delay"
                  << std::setw(8) << "In cap"
                  << std::setw(8) << "In slew"
                  << std::setw(8) << "Buf"
                  << std::setw(10) << "Buf Locs"
                  << "\n";  
        
        std::cout.precision(2);
        std::cout << std::boolalpha;        
        forEachWireSegment( [&] (unsigned idx, const WireSegment& segment) {
                std::cout << std::scientific;
                std::cout << std::setw(5) << idx 
                          << std::setw(5) << (unsigned) segment.getLength()
                          << std::setw(5) << (unsigned) segment.getLoad()
                          << std::setw(10) << (unsigned) segment.getOutputSlew()
                          << std::setw(12) << segment.getPower()
                          << std::setw(8) << segment.getDelay()
                          << std::setw(8) << (unsigned) segment.getInputCap()
                          << std::setw(8) << (unsigned) segment.getInputSlew()
                          << std::setw(8) << segment.isBuffered();
                
                std::cout << std::fixed; 
                for (unsigned idx = 0; idx < segment.getNumBuffers(); ++idx) {
                        std::cout << std::setw(6) << segment.getBufferLocation(idx) << " ";        
                }
                
                std::cout << "\n";                
        });

        std::cout << "*************************************************************\n\n";
}

void TechChar::reportSegments(uint8_t length, uint8_t load, uint8_t outputSlew) const {
        std::cout << "\n";
        std::cout << "*********************************************************************\n";
        std::cout << "*                     Report Characterization                       *\n";
        std::cout << "*********************************************************************\n\n";
        
        std::cout << " Reporting wire segments with length: " << (unsigned) length
                  << " load: " << (unsigned) load << " out slew: " << (unsigned) outputSlew << "\n\n";
        
        std::cout << std::setw(5) << "Idx" 
                  << std::setw(5) << "Len"
                  << std::setw(5) << "Load"
                  << std::setw(10) << "Out slew"
                  << std::setw(12) << "Power" 
                  << std::setw(8) << "Delay"
                  << std::setw(8) << "In cap"
                  << std::setw(8) << "In slew"
                  << std::setw(8) << "Buf"
                  << std::setw(10) << "Buf Locs"
                  << "\n";  
        
        std::cout.precision(2);
        std::cout << std::boolalpha;  
        forEachWireSegment(length, load, outputSlew, 
                [&] (unsigned idx, const WireSegment& segment) {
                        std::cout << std::scientific;
                        std::cout << std::setw(5) << idx 
                                  << std::setw(5) << (unsigned) segment.getLength()
                                  << std::setw(5) << (unsigned) segment.getLoad()
                                  << std::setw(10) << (unsigned) segment.getOutputSlew()
                                  << std::setw(12) << segment.getPower()
                                  << std::setw(8) << segment.getDelay()
                                  << std::setw(8) << (unsigned) segment.getInputCap()
                                  << std::setw(8) << (unsigned) segment.getInputSlew()
                                  << std::setw(8) << segment.isBuffered();
                
                        std::cout << std::fixed; 
                        for (unsigned idx = 0; idx < segment.getNumBuffers(); ++idx) {
                                std::cout << std::setw(6) << segment.getBufferLocation(idx) << " ";        
                        }
                        
                        std::cout << "\n";                    
                });   
}

void TechChar::write(const std::string& filename) const {
        std::ofstream file(filename.c_str());

        if(!file.is_open()) {
                std::cout << " [ERROR] Could not open characterization file.\n";
        }

        file << _minSegmentLength << " " << _maxSegmentLength << " " 
             << _minCapacitance << " " << _maxCapacitance << " "
             << _minSlew << " " << _maxSlew << " " << _options->getWireSegmentUnit() << "\n";

        file.precision(15);
        forEachWireSegment( [&] (unsigned idx, const WireSegment& segment) {
                        file << idx << " " 
                             << (unsigned) segment.getLength() << " "
                             << (unsigned) segment.getLoad() << " "
                             << (unsigned) segment.getOutputSlew() << " "
                             << segment.getPower() << " "
                             << segment.getDelay() << " "
                             << (unsigned) segment.getInputCap() << " "
                             << (unsigned) segment.getInputSlew() << " ";
                        
                        file << !segment.isBuffered() << " ";

                        for (unsigned idx = 0; idx < segment.getNumBuffers(); ++idx) {
                                file << segment.getBufferLocation(idx) << " ";        
                        }

                        file << "\n";
                });
        
        file.close();
}

void TechChar::writeSol(const std::string& filename) const {
        std::ofstream file(filename.c_str());

        if(!file.is_open()) {
                std::cout << " [ERROR] Could not open characterization file.\n";
        }
        
        file.precision(15);
        forEachWireSegment( [&] (unsigned idx, const WireSegment& segment) {
                        file << idx << " ";
                        
                        if (segment.isBuffered()) {
                                for (unsigned idx = 0; idx < segment.getNumBuffers(); ++idx) {
                                        float wirelengthValue = segment.getBufferLocation(idx) * ((float) (segment.getLength()) * (float) (_options->getWireSegmentUnit()));
                                        file << (unsigned long) (wirelengthValue);
                                        file << "," << segment.getBufferMaster(idx);
                                        if (!(idx + 1 >= segment.getNumBuffers())) {
                                                file << ",";
                                        }
                                }
                        } else {
                                float wirelengthValue = (float) (segment.getLength()) * (float) (_options->getWireSegmentUnit());
                                file << (unsigned long) (wirelengthValue);
                        }

                        file << "\n";
                });
        
        file.close();
}

void TechChar::createFakeEntries(unsigned length, unsigned fakeLength) {
	// This condition would just duplicate wires that already exist
	if (length == fakeLength) {
		return;
	}

        std::cout << " [WARNING] Creating fake entries in the LUT.\n";
        for (unsigned load = 1; load <= getMaxCapacitance(); ++load) {
                for (unsigned outSlew = 1; outSlew <= getMaxSlew(); ++outSlew) {
                        forEachWireSegment(length, load, outSlew,
                                [&] (unsigned key, const WireSegment& seg) {
                                        unsigned power = seg.getPower();
                                        unsigned delay = seg.getDelay();
                                        unsigned inputCap = seg.getInputCap();
                                        unsigned inputSlew = seg.getInputSlew();
						
                                        WireSegment& fakeSeg = createWireSegment(fakeLength,
                                                                                 load, 
                                                                                 outSlew,
                                                                                 power,
                                                                                 delay,
                                                                                 inputCap,
                                                                                 inputSlew
                                                                                );

                                        for (unsigned buf = 0; buf < seg.getNumBuffers(); ++buf) {
                                                fakeSeg.addBuffer(seg.getBufferLocation(buf));
                                                fakeSeg.addBufferMaster(seg.getBufferMaster(buf));
                                        }                                   
                                });
                }
        }
}

void TechChar::reportSegment(unsigned key) const {
        const WireSegment& seg = getWireSegment(key);

        std::cout << "    Key: "     << key 
                  << " outSlew: "    << (unsigned) seg.getOutputSlew() 
                  << " load: "       << (unsigned) seg.getLoad()
                  << " length: "     << (unsigned) seg.getLength() 
                  << " isBuffered: " << seg.isBuffered() << "\n";
}

// Characterization Methods

void TechChar::initCharacterization() {
        //Sets up most of the attributes that the characterization uses.
        //Gets the chip, openSta and networks.
        ord::OpenRoad* openRoad = ord::OpenRoad::openRoad();
        _dbNetworkChar = openRoad->getDbNetwork();
        if (_dbNetworkChar == nullptr){
                error("Network not found. Check your lef/def/verilog file.\n");
        }
        _db = odb::dbDatabase::getDatabase(_options->getDbId());
        if (_db == nullptr){
                error("Database not found. Check your lef/def/verilog file.\n");
        }
        odb::dbChip* chip  = _db->getChip();
        if (chip == nullptr){
                error("Chip not found. Check your lef/def/verilog file.\n");
        }
        odb::dbBlock* block = chip->getBlock();
        if (block == nullptr){
                error("Block not found. Check your lef/def/verilog file.\n");
        }
        _openSta = openRoad->getSta();
        sta::Network* networkChar = _openSta->network();
        if (networkChar == nullptr){
                error("Network not found. Check your lef/def/verilog file.\n");
        }
        float dbUnitsPerMicron = static_cast<float> (block->getDbUnitsPerMicron());

        //Change resPerSqr and capPerSqr to DBU units.
        double newCapPerSqr = (_options->getCapPerSqr() * std::pow(10.0,-12))/dbUnitsPerMicron;
        double newResPerSqr = (_options->getResPerSqr())/dbUnitsPerMicron;
        _options->setCapPerSqr(newCapPerSqr); // picofarad/micron to farad/DBU
        _options->setResPerSqr(newResPerSqr); // ohm/micron to ohm/DBU

        //Change intervals if needed
        if (_options->getSlewInter() != 0) {
                _charSlewInter = _options->getSlewInter();
        }
        if (_options->getCapInter() != 0) {
                _charCapInter = _options->getCapInter();
        }

        //Gets the buffer masters and its in/out pins.
        std::vector<std::string> masterVector = _options->getBufferList();
        if (masterVector.size() < 1){
                error("Buffer not found. Check your -buf_list input.\n");
        }
        odb::dbMaster* testBuf = nullptr;
        for (std::string masterString : masterVector) {
                testBuf = _db->findMaster(masterString.c_str());
                if (testBuf == NULL){
                        error("Buffer not found. Check your -buf_list input.\n");
                }
                _masterNames.insert(masterString);
        }
        
        std::string bufMasterName =  masterVector[0];
        _charBuf = _db->findMaster(bufMasterName.c_str());

        for (odb::dbMTerm* masterTerminal : _charBuf->getMTerms()){
                if( masterTerminal->getIoType() == odb::dbIoType::INPUT && 
                    masterTerminal->getSigType() == odb::dbSigType::SIGNAL){
                        _charBufIn = masterTerminal->getName();
                } else if ( masterTerminal->getIoType() == odb::dbIoType::OUTPUT && 
                            masterTerminal->getSigType() == odb::dbSigType::SIGNAL ){
                        _charBufOut = masterTerminal->getName();
                }
        }
        //Creates the new characterization block. (Wiresegments are created here instead of the main block)
        std::string characterizationBlockName = "CharacterizationBlock";
        _charBlock = odb::dbBlock::create(block, characterizationBlockName.c_str());

        //Gets the capacitance and resistance per DBU. User input.
        _resPerDBU = _options->getResPerSqr(); 
        _capPerDBU = _options->getCapPerSqr(); 

        //Defines the different wirelengths to test and the characterization unit.
        unsigned wirelengthIterations = _options->getCharWirelengthIterations();
        unsigned maxWirelength = (_charBuf->getHeight() * 10) * wirelengthIterations; //Hard-coded limit
        if (_options->getWireSegmentUnit() == 0){
                unsigned charaunit = _charBuf->getHeight() * 10;
                _options->setWireSegmentUnit(static_cast<unsigned> (charaunit));
        } else {
                //Updates the units to DBU.
                int dbUnitsPerMicron = block->getDbUnitsPerMicron();
                unsigned currentSegmentDistance = _options->getWireSegmentUnit();
                _options->setWireSegmentUnit(currentSegmentDistance * dbUnitsPerMicron);
        }
        
        for (unsigned wirelengthInter = _options->getWireSegmentUnit(); 
             (wirelengthInter <= maxWirelength) && (wirelengthInter <= wirelengthIterations * _options->getWireSegmentUnit()); 
             wirelengthInter += _options->getWireSegmentUnit()){
                _wirelengthsToTest.push_back(wirelengthInter);
        }

        if (_wirelengthsToTest.size() < 1) {
                error("Error generating the wirelengths to test. Check your -wire_unit parameter or technology files.\n");
        }

        setLenghthUnit(static_cast<unsigned> ( ((_charBuf->getHeight() * 10)/2) / dbUnitsPerMicron));

        //Gets the max slew and max cap if they weren't added as parameters.
        float maxSlew = 0.0;
        float maxCap = 0.0;
        if ( _options->getMaxCharSlew() == 0 || _options->getMaxCharCap() == 0 ){
                sta::Cell* masterCell = _dbNetworkChar->dbToSta(_charBuf);
                sta::LibertyCell* libertyCell = networkChar->libertyCell(masterCell);
                if (!libertyCell) {
                        error("No Liberty cell found for %s.\n", bufMasterName.c_str());
                }

                sta::LibertyLibrary* staLib = libertyCell->libertyLibrary();
                bool maxSlewExist = false;
                bool maxCapExist = false;
                staLib->defaultMaxSlew(maxSlew, maxSlewExist);
                staLib->defaultMaxCapacitance(maxCap, maxCapExist);
                if ( !maxSlewExist || !maxCapExist ){
                        error("Liberty Library does not have Max Slew or Max Cap values.\n");
                } else {
                        _charMaxSlew = maxSlew;
                        _charMaxCap = maxCap;
                }
        } else {
                _charMaxSlew = _options->getMaxCharSlew();
                _charMaxCap = _options->getMaxCharCap();
        }

        //Creates the different slews and loads to test.
        unsigned slewIterations = _options->getCharSlewIterations();
        unsigned loadIterations = _options->getCharLoadIterations();
        for (float currentSlewInter = _charSlewInter; 
             (currentSlewInter <= _charMaxSlew) && (currentSlewInter <= slewIterations * _charSlewInter); 
             currentSlewInter += _charSlewInter){
                _slewsToTest.push_back(currentSlewInter);
        }
        for (float currentCapInter = _charCapInter; 
             ( (currentCapInter <= _charMaxCap) && (currentCapInter <= loadIterations * _charCapInter) ); 
             currentCapInter += _charCapInter){
                _loadsToTest.push_back(currentCapInter); 
        }

        if ((_loadsToTest.size() < 1) || (_slewsToTest.size() < 1)) {
                error("Error generating the wirelengths to test. Check your -max_cap / -max_slew / -cap_inter / -slew_inter parameter or technology files.\n");
        }
}

std::vector<TechChar::SolutionData> TechChar::createPatterns(unsigned setupWirelength) {
        //Sets the number of nodes (wirelength/characterization unit) that a buffer can be placed and...
        //...the number of topologies (combinations of buffers, considering only 1 drive) that can exist.
        const unsigned numberOfNodes = setupWirelength/_options->getWireSegmentUnit();
        unsigned numberOfTopologies = std::pow(2,numberOfNodes);
        std::vector<SolutionData> topologiesVector;
        odb::dbNet* currentNet = nullptr;
        odb::dbWire* currentWire = nullptr;

        //For each possible topology...
        for (unsigned solutionCounterInt = 0; 
        solutionCounterInt < numberOfTopologies; 
        solutionCounterInt++) {
                //Creates a bitset that represents the buffer locations.
                std::bitset<5> solutionCounter(solutionCounterInt);
                unsigned short int wireCounter = 0;
                SolutionData currentTopology;
                //Creates the starting net.
                std::string netName = "net_" + std::to_string(setupWirelength) + 
                                        "_" + solutionCounter.to_string() + 
                                        "_" + std::to_string(wireCounter);
                currentNet = odb::dbNet::create(_charBlock, netName.c_str());
                currentWire = odb::dbWire::create(currentNet);
                currentNet->setSigType(odb::dbSigType::SIGNAL);
                //Creates the input port.
                std::string inPortName = "in_" + std::to_string(setupWirelength) + 
                                                solutionCounter.to_string();
                odb::dbBTerm* inPort = odb::dbBTerm::create(currentNet, inPortName.c_str()); //sig type is signal by default
                inPort->setIoType(odb::dbIoType::INPUT);
                odb::dbBPin* inPortPin = odb::dbBPin::create(inPort);
                //Updates the topology with the new port.
                currentTopology.inPort = inPortPin;
                //Iterating through possible buffers...
                unsigned nodesWithoutBuf = 0;
                bool isCurrentlyPureWire = true;
                for (unsigned nodeIndex = 0;
                nodeIndex < numberOfNodes; nodeIndex++){
                        if (solutionCounter[nodeIndex] == 0)  {
                                //Not a buffer, only a wire segment.
                                nodesWithoutBuf++;
                        } else {
                                //Buffer, need to create the instance and a new net.
                                nodesWithoutBuf++;
                                //Creates a new buffer instance.
                                std::string bufName = "buf_" + std::to_string(setupWirelength) + 
                                                        solutionCounter.to_string() + "_" + 
                                                        std::to_string(wireCounter);
                                odb::dbInst* bufInstance = odb::dbInst::create(_charBlock, _charBuf, bufName.c_str());
                                odb::dbITerm* bufInstanceInPin = bufInstance->findITerm(_charBufIn.c_str());
                                odb::dbITerm* bufInstanceOutPin = bufInstance->findITerm(_charBufOut.c_str());
                                odb::dbITerm::connect(bufInstanceInPin, currentNet);
                                //Updates the topology with the old net and number of nodes that didn't have buffers until now.
                                currentTopology.netVector.push_back(currentNet);
                                currentTopology.nodesWithoutBufVector.push_back(nodesWithoutBuf);
                                //Creates a new net.
                                wireCounter++;
                                std::string netName = "net_" + std::to_string(setupWirelength) + 
                                                        solutionCounter.to_string() + "_" + 
                                                        std::to_string(wireCounter);
                                currentNet = odb::dbNet::create(_charBlock, netName.c_str());
                                currentWire = odb::dbWire::create(currentNet);
                                odb::dbITerm::connect(bufInstanceOutPin, currentNet);
                                currentNet->setSigType(odb::dbSigType::SIGNAL);
                                //Updates the topology wih the new instance and the current topology (as a vector of strings).
                                currentTopology.instVector.push_back(bufInstance);
                                currentTopology.topologyDescriptor.push_back(std::to_string(nodesWithoutBuf*_options->getWireSegmentUnit()));
                                currentTopology.topologyDescriptor.push_back(_charBuf->getName());
                                nodesWithoutBuf = 0;
                                isCurrentlyPureWire = false;
                        }
                }
                //Finishing up the topology with the output port.
                std::string outPortName = "out_" + std::to_string(setupWirelength) + solutionCounter.to_string();
                odb::dbBTerm* outPort = odb::dbBTerm::create(currentNet, outPortName.c_str()); //sig type is signal by default
                outPort->setIoType(odb::dbIoType::OUTPUT);
                odb::dbBPin* outPortPin = odb::dbBPin::create(outPort);
                //Updates the topology with the output port, old new, possible instances and other attributes.
                currentTopology.outPort = outPortPin;
                if (isCurrentlyPureWire) {
                        currentTopology.instVector.push_back(nullptr);
                }
                currentTopology.isPureWire = isCurrentlyPureWire;
                currentTopology.netVector.push_back(currentNet);
                currentTopology.nodesWithoutBufVector.push_back(nodesWithoutBuf);
                if (nodesWithoutBuf != 0){
                        currentTopology.topologyDescriptor.push_back(std::to_string(nodesWithoutBuf*_options->getWireSegmentUnit()));
                }
                //Go to the next topology.
                topologiesVector.push_back(currentTopology);
        }
        return topologiesVector;
}

void TechChar::createStaInstance() {
        //Creates a new OpenSTA instance that is used only for the characterization.
        ord::OpenRoad* openRoad = ord::OpenRoad::openRoad();
        //Creates the new instance based on the charcterization block.
        if (_openStaChar != nullptr){
                _openStaChar->clear();
        }
        _openStaChar = sta::makeBlockSta(_charBlock);
        //Sets the current OpenSTA instance as the new one just created.
        sta::Sta::setSta(_openStaChar);
        _openStaChar->clear();
        //Gets the new components from the new OpenSTA.
        _dbNetworkChar = openRoad->getDbNetwork();
        //Set some attributes for the new instance.
        _openStaChar->units()->timeUnit()->setScale(1);
        _openStaChar->units()->capacitanceUnit()->setScale(1);
        _openStaChar->units()->resistanceUnit()->setScale(1);
        _openStaChar->units()->powerUnit()->setScale(1);
        //Gets the corner and other analysis attributes from the new instance.
        _charCorner = _openStaChar->corners()->findCorner(0);
        sta::PathAPIndex path_ap_index = _charCorner->findPathAnalysisPt(sta::MinMax::max())->index();
        sta::Corners *corners = _openStaChar->search()->corners();
        _charPathAnalysis = corners->findPathAnalysisPt(path_ap_index);
}

void TechChar::setParasitics(std::vector<TechChar::SolutionData> topologiesVector, 
                                              unsigned setupWirelength) {
        //For each topology...
        for (SolutionData currentSolution : topologiesVector){
                //For each net in the topolgy -> set the parasitics.
                for (unsigned netIndex = 0 ; netIndex < currentSolution.netVector.size(); ++netIndex ){
                        //Gets the ITerms (instance pins) and BTerms (other high-level pins) from the current net.
                        odb::dbNet* currentNet = currentSolution.netVector[netIndex];
                        unsigned nodesWithoutBuf = currentSolution.nodesWithoutBufVector[netIndex];
                        odb::dbBTerm* inBTerm = currentSolution.inPort->getBTerm();
                        odb::dbBTerm* outBTerm = currentSolution.outPort->getBTerm();
                        odb::dbSet<odb::dbBTerm> netBTerms = currentNet->getBTerms();
                        odb::dbSet<odb::dbITerm> netITerms = currentNet->getITerms();
                        sta::Pin* firstPin = nullptr;
                        sta::Pin* lastPin = nullptr;
                        //Gets the sta::Pin from the beginning and end of the net.
                        if (netBTerms.size() > 1) { // Parasitics for a purewire segment. First and last pin are already available.
                                firstPin = _dbNetworkChar->dbToSta(inBTerm);
                                lastPin = _dbNetworkChar->dbToSta(outBTerm);
                        }  else if (netBTerms.size() == 1) { // Parasitics for the end/start of a net. One Port and one instance pin. 
                                odb::dbBTerm* netBTerm = currentNet->get1stBTerm();
                                odb::dbITerm* netITerm = currentNet->get1stITerm();
                                if (netBTerm == inBTerm) {     
                                        firstPin = _dbNetworkChar->dbToSta(netBTerm);
                                        lastPin = _dbNetworkChar->dbToSta(netITerm);
                                } else {
                                        firstPin = _dbNetworkChar->dbToSta(netITerm);
                                        lastPin = _dbNetworkChar->dbToSta(netBTerm);
                                }
                        } else { // Parasitics for a net that is between two buffers. Need to iterate over the net ITerms.
                                for (odb::dbITerm* iterm : netITerms) {
                                        if (iterm == nullptr) {
                                                continue;
                                        }

                                        if (iterm->getIoType() == odb::dbIoType::INPUT) {
                                                lastPin = _dbNetworkChar->dbToSta(iterm);
                                        }

                                        if (iterm->getIoType() == odb::dbIoType::OUTPUT) {
                                                firstPin = _dbNetworkChar->dbToSta(iterm);
                                        }

                                        if (firstPin != nullptr && lastPin != nullptr) {
                                                break;
                                        }
                                }
                        }
                        //Sets the Pi and Elmore information. 
                        unsigned charUnit = _options->getWireSegmentUnit();
                        _openStaChar->makePiElmore(firstPin, sta::RiseFall::rise(), sta::MinMaxAll::all(), 
                                                   (nodesWithoutBuf * charUnit * _capPerDBU)/2, 
                                                   nodesWithoutBuf * charUnit * _resPerDBU, 
                                                   (nodesWithoutBuf * charUnit * _capPerDBU)/2);
                        _openStaChar->makePiElmore(firstPin, sta::RiseFall::fall(), sta::MinMaxAll::all(), 
                                                   (nodesWithoutBuf * charUnit * _capPerDBU)/2, 
                                                   nodesWithoutBuf * charUnit * _resPerDBU, 
                                                   (nodesWithoutBuf * charUnit * _capPerDBU)/2);
                        _openStaChar->setElmore(firstPin, lastPin, sta::RiseFall::rise(), sta::MinMaxAll::all(), 
                                                nodesWithoutBuf * charUnit * _capPerDBU);
                        _openStaChar->setElmore(firstPin, lastPin, sta::RiseFall::fall(), sta::MinMaxAll::all(), 
                                                nodesWithoutBuf * charUnit * _capPerDBU);
                }
        }
}

void TechChar::setSdc(std::vector<TechChar::SolutionData> topologiesVector, 
                                              unsigned setupWirelength) {
        //Creates a clock to set input and output delay.
        sta::Sdc* sdcChar = _openStaChar->sdc();
        sta::FloatSeq* characterizationClockWave = new sta::FloatSeq;
        characterizationClockWave->push_back(0);
        characterizationClockWave->push_back(1);
        const std::string characterizationClockName = "characlock" + std::to_string(setupWirelength);
        _openStaChar->makeClock(characterizationClockName.c_str(),
                        NULL,
                        false,
                        1.0,
                        characterizationClockWave,
                        NULL);
        sta::Clock* characterizationClock = sdcChar->findClock(characterizationClockName.c_str());
        //For each topology...
        for (SolutionData currentSolution : topologiesVector){
                //Gets the input and output ports.
                odb::dbBTerm* inBTerm = currentSolution.inPort->getBTerm();
                odb::dbBTerm* outBTerm = currentSolution.outPort->getBTerm();
                sta::Pin *inPin = _dbNetworkChar->dbToSta(inBTerm);
                sta::Pin *outPin = _dbNetworkChar->dbToSta(outBTerm);
                //Set the input delay and output delay on each one.
                _openStaChar->setInputDelay(inPin,
                                        sta::RiseFallBoth::riseFall(),
                                        characterizationClock,
                                        sta::RiseFall::rise(),
                                        NULL,
                                        false,
                                        false,
                                        sta::MinMaxAll::max(),
                                        true,
                                        0.0);
                _openStaChar->setOutputDelay(outPin,
                                        sta::RiseFallBoth::riseFall(),
                                        characterizationClock,
                                        sta::RiseFall::rise(),
                                        NULL,
                                        false,
                                        false,
                                        sta::MinMaxAll::max(),
                                        true,
                                        0.0);
        }
}

TechChar::ResultData TechChar::computeTopologyResults(TechChar::SolutionData currentSolution, 
                                                      sta::Vertex* outPinVert, 
                                                      float currentLoad, 
                                                      unsigned setupWirelength) {
        ResultData currentResults;
        //Computations for power, requires the PowerResults class from OpenSTA.
        float totalPower = 0;
        if (currentSolution.isPureWire == false){
                //If it isn't a pure wire solution, get the sum of the total power of each buffer.
                for (odb::dbInst* bufferInst : currentSolution.instVector){
                        sta::Instance* bufferInstSta = _dbNetworkChar->dbToSta(bufferInst);  
                        sta::PowerResult instResults; 
                        instResults.clear(); 
                        _openStaChar->power(bufferInstSta, _charCorner, instResults);
                        totalPower = totalPower + instResults.total();
                }
        }
        currentResults.totalPower = totalPower;
        //Computations for input capacitance.
        float incap = 0;
        if (currentSolution.isPureWire == true){
                //For pure-wire, sum of the current load with the capacitance of the net.
                incap = currentLoad + (setupWirelength * _capPerDBU);
        } else {
                //For buffered solutions, add the cap of the input of the first buffer with the capacitance of the left-most net.
                float length = std::stod(currentSolution.topologyDescriptor[0]);
                sta::LibertyCell* firstInstLiberty = _dbNetworkChar->libertyCell(
                                                        _dbNetworkChar->dbToSta(currentSolution.instVector[0]));
                sta::LibertyPort* firstPinLiberty = firstInstLiberty->findLibertyPort(_charBufIn.c_str());
                float firstPinCapRise = firstPinLiberty->capacitance(sta::RiseFall::rise(), sta::MinMax::max());
                float firstPinCapFall = firstPinLiberty->capacitance(sta::RiseFall::fall(), sta::MinMax::max());
                float firstPinCap = 0;
                if (firstPinCapRise > firstPinCapFall) {
                        firstPinCap = firstPinCapRise;
                } else {
                        firstPinCap = firstPinCapFall;
                }
                incap = firstPinCap + (length * _capPerDBU);
        }
        float totalcap = std::floor((incap + (_charCapInter/2))/_charCapInter) * _charCapInter;
        currentResults.totalcap = totalcap;
        //Computations for delay. 
        float pinArrival = _openStaChar->vertexArrival(outPinVert, sta::RiseFall::fall(), _charPathAnalysis);
        currentResults.pinArrival = pinArrival;
        //Computations for output slew.
        float pinRise = _openStaChar->vertexSlew(outPinVert, sta::RiseFall::rise(), sta::MinMax::max());
        float pinFall = _openStaChar->vertexSlew(outPinVert, sta::RiseFall::fall(), sta::MinMax::max());
        float pinSlew = std::floor((((pinRise + pinFall)/2) + (_charSlewInter/2))/_charSlewInter) * _charSlewInter;
        currentResults.pinSlew = pinSlew;

        return currentResults;
}

TechChar::SolutionData TechChar::updateBufferTopologies(TechChar::SolutionData currentSolution) {
        unsigned currentindex = 0;
        //Change the buffer topology by increasing the size of the buffers.
        //After testing all the sizes for the current buffer, increment the size of the next one (works like a carry mechanism).
        //Ex for 4 different buffers: 103-> 110 -> 111 -> 112 -> 113 -> 120 ... 
        bool done = false;
        while (!done) {
                //Gets the iterator to the beggining of the _masterNames set.
                std::set<std::string>::iterator masterItr = _masterNames.find(currentSolution.instVector[currentindex]->getMaster()->getName());
                //Gets the iterator to the end of the _masterNames set.
                std::set<std::string>::iterator masterFinalItr = _masterNames.end();
                masterFinalItr--;
                if (masterItr == masterFinalItr) {
                        //If the iterator can't increment past the final iterator, change the current buf master to the _charBuf and try to go to next instance. 
                        odb::dbInst* currentInst = currentSolution.instVector[currentindex];
                        currentInst->swapMaster(_charBuf);
                        unsigned topologyCounter = 0;
                        for (unsigned topologyIndex = 0; 
                                topologyIndex < currentSolution.topologyDescriptor.size(); 
                                topologyIndex++){
                                //Iterates through the topologyDescriptor to set the new information(string representing the current buffer).
                                std::string topologyS = currentSolution.topologyDescriptor[topologyIndex];
                                if (_masterNames.find(topologyS) == _masterNames.end()){
                                        continue; //Is a number (i.e. a wire segment). Ignore and go to the next one.
                                } else {
                                        if (topologyCounter == currentindex){
                                                std::set<std::string>::iterator firstMaster = _masterNames.begin();
                                                currentSolution.topologyDescriptor[topologyIndex] = *firstMaster;
                                                break;         
                                        }
                                        topologyCounter++;
                                }
                        }
                        currentindex++;
                } else {
                        //Increment the iterator and change the current buffer to the new size.
                        masterItr++;
                        std::string masterString = *masterItr;
                        odb::dbMaster* newBufMaster = _db->findMaster(masterString.c_str());
                        odb::dbInst* currentInst = currentSolution.instVector[currentindex];
                        currentInst->swapMaster(newBufMaster);
                        unsigned topologyCounter = 0;
                        for (unsigned topologyIndex = 0; 
                                topologyIndex < currentSolution.topologyDescriptor.size(); 
                                topologyIndex++){
                                std::string topologyS = currentSolution.topologyDescriptor[topologyIndex];
                                if (_masterNames.find(topologyS) == _masterNames.end()){
                                        continue; //Is a number (i.e. a wire segment). Ignore and go to the next one.
                                } else {
                                        if (topologyCounter == currentindex){
                                                currentSolution.topologyDescriptor[topologyIndex] = masterString;
                                                break;         
                                        }
                                        topologyCounter++;
                                }
                        }
                        done = true;
                }
                if (currentindex >= currentSolution.instVector.size()){
                        //If the next instance doesn't exist, all the topologies were tested -> exit the function.
                        done = true;
                }
        }
        return currentSolution;
}

std::vector<TechChar::ResultData> TechChar::characterizationPostProcess() {
        //Post-process of the characterization results.
        std::vector <ResultData> selectedSolutions;
        //Select only a subset of the total results. If, for a combination of input cap, wirelength, load and output slew, more than 3 results exists -> select only 3 of them.
        for (auto& keyResults : _solutionMap) {
                CharKey currentKey = keyResults.first;
                std::vector<ResultData> resultVector = keyResults.second;
                /*if (resultVector.size() <= 3){
                        for (ResultData selectedResults : resultVector){
                                selectedSolutions.push_back(selectedResults);
                        }
                } else {
                        int indexLimit = resultVector.size() - 1;
                        selectedSolutions.push_back(
                                        resultVector[ static_cast<unsigned> (std::floor(0.1 * static_cast<float> (indexLimit))) ]);
                        selectedSolutions.push_back(
                                        resultVector[ static_cast<unsigned> (std::floor(0.5 * static_cast<float> (indexLimit))) ]);
                        selectedSolutions.push_back(
                                        resultVector[ static_cast<unsigned> (std::floor(0.9 * static_cast<float> (indexLimit))) ]);
                }*/
                for (ResultData selectedResults : resultVector){
                        selectedSolutions.push_back(selectedResults);
                }
        }

        //Creates variables to set the max and min values. These are normalized.
        unsigned minResultWirelength = std::numeric_limits<unsigned>::max();
        unsigned maxResultWirelength = 0;
        unsigned minResultCapacitance = std::numeric_limits<unsigned>::max();
        unsigned maxResultCapacitance = 0;
        unsigned minResultSlew = std::numeric_limits<unsigned>::max();
        unsigned maxResultSlew = 0;
        std::vector <ResultData> convertedSolutions;
        for (ResultData currentSolution : selectedSolutions){
                if (currentSolution.pinSlew > _charMaxSlew) {
                        continue;
                } 
                ResultData convertedResult;
                //Processing and normalizing of output slew.
                convertedResult.pinSlew = normalizeCharResults(currentSolution.pinSlew, _charSlewInter, &minResultSlew, &maxResultSlew);
                //Processing and normalizing of input slew.
                convertedResult.inSlew = normalizeCharResults(currentSolution.inSlew, _charSlewInter, &minResultSlew, &maxResultSlew);
                //Processing and normalizing of input cap.
                convertedResult.totalcap = normalizeCharResults(currentSolution.totalcap, _charCapInter, &minResultCapacitance, &maxResultCapacitance);
                //Processing and normalizing of load.
                convertedResult.load = normalizeCharResults(currentSolution.load, _charCapInter, &minResultCapacitance, &maxResultCapacitance);
                //Processing and normalizing of the wirelength.
                convertedResult.wirelength = normalizeCharResults(currentSolution.wirelength, _options->getWireSegmentUnit(), &minResultWirelength, &maxResultWirelength);
                //Processing and normalizing of delay.
                convertedResult.pinArrival = static_cast<unsigned> (std::ceil(currentSolution.pinArrival / (_charSlewInter/5)));
                //Add missing information.
                convertedResult.totalPower = currentSolution.totalPower;
                convertedResult.isPureWire = currentSolution.isPureWire;
                std::vector<std::string> topologyResult;
                for (int topologyIndex = 0 ; topologyIndex < currentSolution.topology.size() ; topologyIndex ++) {
                        std::string topologyS = currentSolution.topology[topologyIndex];
                        //Normalizes the strings that represents the topology too.
                        if (_masterNames.find(topologyS) == _masterNames.end()){
                                //Is a number (i.e. a wire segment).
                                topologyResult.push_back(std::to_string(std::stod(topologyS) / static_cast<float> (currentSolution.wirelength) ));
                        } else {
                                topologyResult.push_back(topologyS);
                        }
                }
                convertedResult.topology = topologyResult;
                //Send the results to a vector. This will be used to create the wiresegments for CTS.
                convertedSolutions.push_back(convertedResult);
        }
        //Sets the min and max values and returns the result vector.
        _minSlew = minResultSlew;
        _maxSlew = maxResultSlew;
        _minCapacitance = minResultCapacitance;
        _maxCapacitance = maxResultCapacitance;
        _minSegmentLength = minResultWirelength;
        _maxSegmentLength = maxResultWirelength;
        return convertedSolutions;
}

unsigned TechChar::normalizeCharResults(float value, float iter, unsigned* min, unsigned* max) {
	unsigned normVal = static_cast<unsigned>(std::ceil(value / iter));
        if (normVal == 0 ){
                normVal = 1;
        }
	*min = std::min(*min, normVal);
	*max = std::max(*max, normVal);
	return normVal;
}

void TechChar::create() {
        //Setup of the attributes required to run the characterization.
        initCharacterization();
        long unsigned int topologiesCreated = 0;
        for (unsigned setupWirelength : _wirelengthsToTest){
                //Creates the topologies for the current wirelength.
                std::vector<SolutionData> topologiesVector = createPatterns(setupWirelength);
                //Creates the new openSTA instance and setup its components with updateTiming(true);
                createStaInstance();
                _openStaChar->updateTiming(true);
                //Setup of the parasitics for each net and the input/output delay.
                setParasitics(topologiesVector, setupWirelength);
                setSdc(topologiesVector, setupWirelength);
                //For each topology...
                for (SolutionData currentSolution : topologiesVector){
                        //Gets the input and output port (as terms, pins and vertices).
                        odb::dbBTerm* inBTerm = currentSolution.inPort->getBTerm();
                        odb::dbBTerm* outBTerm = currentSolution.outPort->getBTerm();
                        odb::dbNet* lastNet = currentSolution.netVector.back();
                        sta::Pin* inPin = _dbNetworkChar->dbToSta(inBTerm);
                        sta::Pin* outPin = _dbNetworkChar->dbToSta(outBTerm);
                        sta::Port* inPort = _dbNetworkChar->port(inPin);
                        sta::Port* outPort = _dbNetworkChar->port(outPin);
                        sta::Vertex* outPinVert = _openStaChar->graph()->vertex(outBTerm->staVertexId());
                        sta::Vertex* inPinVert = _openStaChar->graph()->vertex(inBTerm->staVertexId());

                        //Gets the first pin of the last net. Needed to set a new parasitic (load) value.
                        sta::Pin* firstPinLastNet = nullptr;
                        if (lastNet->getBTerms().size() > 1) { // Parasitics for purewire segment. First and last pin are already available.
                                firstPinLastNet = inPin;
                        }  else { // Parasitics for the end/start of a net. One Port and one instance pin. 
                                odb::dbITerm* netITerm = lastNet->get1stITerm();
                                firstPinLastNet = _dbNetworkChar->dbToSta(netITerm);
                        }

                        float c1 = 0;
                        float c2 = 0;
                        float r1 = 0;
                        bool piExists = false;
                        //Gets the parasitics that are currently used for the last net.
                        _openStaChar->findPiElmore(firstPinLastNet, sta::RiseFall::rise(), sta::MinMax::max(), c1, r1, c2, piExists);  
                        //For each possible buffer combination (different sizes).
                        unsigned buffersUpdate = std::pow(_masterNames.size(), currentSolution.instVector.size());
                        do {
                                //For each possible load.
                                for (float currentLoad : _loadsToTest){
                                        //sta2->setPortExtPinCap(outPort, sta::RiseFallBoth::riseFall(), sta::MinMaxAll::all(), currentLoad );

                                        //Sets the new parasitic of the last net (load added to last pin).
                                        _openStaChar->makePiElmore(firstPinLastNet, sta::RiseFall::rise(), sta::MinMaxAll::all(), 
                                                                   c1, r1, c2 + currentLoad);
                                        _openStaChar->makePiElmore(firstPinLastNet, sta::RiseFall::fall(), sta::MinMaxAll::all(), 
                                                                   c1, r1, c2 + currentLoad);
                                        _openStaChar->setElmore(firstPinLastNet, outPin, sta::RiseFall::rise(), sta::MinMaxAll::all(), 
                                                                c1 + c2 + currentLoad);
                                        _openStaChar->setElmore(firstPinLastNet, outPin, sta::RiseFall::fall(), sta::MinMaxAll::all(), 
                                                                c1 + c2 + currentLoad);
                                        //For each possible input slew.
                                        for (float currentInputSlew : _slewsToTest){
                                                //sta2->setInputSlew(inPort, sta::RiseFallBoth::riseFall(), sta::MinMaxAll::all(), currentInputSlew);
                                                //Sets the slew on the input vertex. Here the new pattern is created (combination of load, buffers and slew values).
                                                _openStaChar->setAnnotatedSlew(inPinVert, _charCorner, 
                                                                       sta::MinMaxAll::all(), sta::RiseFallBoth::riseFall(), 
                                                                       currentInputSlew);
                                                //Updates timing for the new pattern. 
                                                _openStaChar->updateTiming(true);

                                                //Gets the results (delay, slew, power...) for the pattern.
                                                ResultData currentResults = computeTopologyResults(currentSolution, outPinVert, currentLoad, setupWirelength);

                                                //Appends the results to a map, grouping each result by wirelength, load, output slew and input cap.
                                                currentResults.wirelength = setupWirelength;
                                                currentResults.load = currentLoad;
                                                currentResults.inSlew = currentInputSlew;
                                                currentResults.topology = currentSolution.topologyDescriptor;
                                                currentResults.isPureWire = currentSolution.isPureWire;
                                                CharKey solutionKey;
                                                solutionKey.wirelength = currentResults.wirelength;
                                                solutionKey.pinSlew = currentResults.pinSlew;
                                                solutionKey.load = currentResults.load;
                                                solutionKey.totalcap = currentResults.totalcap;
                                                if (_solutionMap.find(solutionKey) != _solutionMap.end()){
                                                        _solutionMap[solutionKey].push_back(currentResults);
                                                } else {
                                                        std::vector<ResultData> resultGroup;
                                                        resultGroup.push_back(currentResults);
                                                        _solutionMap[solutionKey] = resultGroup;
                                                }
                                                topologiesCreated++;
                                                if (topologiesCreated % 50000 == 0){
                                                        std::cout << "Number of created patterns = " << topologiesCreated << ".\n";
                                                }
                                        }
                                }
                                //If the currentSolution is not a pure-wire, update the buffer topologies.
                                if (currentSolution.isPureWire == false) {
                                        currentSolution = updateBufferTopologies(currentSolution);
                                }                        
                                //For pure-wire solution buffersUpdate == 1, so it only runs once.
                                buffersUpdate--;
                        } while (buffersUpdate != 0);
                }
        }
        std::cout << "Number of created patterns = " << topologiesCreated << ".\n";
        //Returns the OpenSTA instance to the old one.
        sta::Sta::setSta(_openSta);
        //Post-processing of the results.
        std::vector<ResultData> convertedSolutions = characterizationPostProcess();
        //Changes the segment units back to micron and creates the wire segments.
        float dbUnitsPerMicron = static_cast<float> (_charBlock->getDbUnitsPerMicron());
        float currentSegmentDistance = static_cast<float> (_options->getWireSegmentUnit());
        _options->setWireSegmentUnit(static_cast<unsigned> (currentSegmentDistance / dbUnitsPerMicron));
        compileLut(convertedSolutions);
        //Saves the characterization file if needed.
        if (_options->getOutputPath().length() > 0) {
                std::string lutFile = _options->getOutputPath() + "lut.txt";
                std::string solFile = _options->getOutputPath() + "sol_list.txt";
                write(lutFile);
                writeSol(solFile);
        }
        if (_openStaChar != nullptr){
                _openStaChar->clear();
                delete _openStaChar;
                _openStaChar = nullptr;
        }
}

}
