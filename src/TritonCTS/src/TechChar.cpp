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

#include <fstream>
#include <ostream>
#include <iomanip>
#include <sstream>
#include <algorithm>

namespace TritonCTS {

void TechChar::parseLut(const std::string& file) {
        std::cout << " Reading LUT file \"" << file << "\"\n";
        std::ifstream lutFile(file.c_str());
        
        if (!lutFile.is_open()) {
                std::cout << "    [ERROR] Could not find LUT file. Exiting...\n";
                std::exit(1);
        }

        initLengthUnits();

        // First line of the LUT is a header with normalization values
        if (!(lutFile >> _minSegmentLength >> _maxSegmentLength >> _minCapacitance 
                      >> _maxCapacitance >> _minSlew >> _maxSlew)) {
                std::cout << "    [ERROR] Problem reading the LUT file\n";                 
        }
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
               std::cout << "    [ERROR] Normalized values in the LUT should be in the range ";
               std::cout << "[1, " << MAX_NORMALIZED_VAL << "]\n";
               std::cout << "    Check the table above to see the normalization ranges and check ";
               std::cout << "your characterization configuration.\n";
               std::exit(1);
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
                        std::cout << "    [ERROR] Number of buffers does not match on solution " 
                                  << solIdx << "\n";
                        std::cout << numBuffers << "\n";
                        std::exit(1);
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
             << _minSlew << " " << _maxSlew << "\n";

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

void TechChar::createFakeEntries(unsigned length, unsigned fakeLength) {
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

}
