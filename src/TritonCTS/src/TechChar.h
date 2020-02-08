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

#ifndef CHARACTERIZATION_H
#define CHARACTERIZATION_H

#include "CtsOptions.h"

#include <iostream>
#include <vector>
#include <unordered_map>
#include <functional>
#include <bitset>
#include <cassert> 
#include <string>
#include <deque>

namespace TritonCTS {

class WireSegment {
       uint8_t  _length;
       uint8_t  _load;
       uint8_t  _outputSlew;
       
       double   _power;
       unsigned _delay;
       uint8_t  _inputCap;
       uint8_t  _inputSlew;
       
       std::vector<double> _bufferLocations;
       std::vector<std::string> _bufferMasters;
       
public: 
       WireSegment(uint8_t length, uint8_t load, uint8_t outputSlew,
                   double power, unsigned delay, uint8_t inputCap,
                   uint8_t inputSlew) :
                   _length(length), _load(load), _outputSlew(outputSlew),
                   _power(power), _delay(delay), _inputCap(inputCap),
                   _inputSlew(inputSlew) {}
       
       void addBuffer(double location) { 
               _bufferLocations.push_back(location);
       }
       
       void addBufferMaster(std::string name) { 
               _bufferMasters.push_back(name);
       }
       
       double   getPower() const { return _power; }
       unsigned getDelay() const { return _delay; }
       uint8_t  getInputCap() const { return _inputCap; }
       uint8_t  getInputSlew() const { return _inputSlew; }
       uint8_t  getLength() const { return _length; }
       uint8_t  getLoad() const { return _load; }
       uint8_t  getOutputSlew() const { return _outputSlew; }
       bool     isBuffered() const { return _bufferLocations.size() > 0; }
       unsigned getNumBuffers() const { return _bufferLocations.size(); }
       std::vector<double>& getBufferLocations() { return _bufferLocations; }
       std::vector<std::string>& getBufferMasters() { return _bufferMasters; }
       
       double   getBufferLocation(unsigned idx) const {
               if (idx < 0 || idx >= _bufferLocations.size()) {
                       return -1.0;
               }
               return _bufferLocations[idx];
       }

        std::string getBufferMaster(unsigned idx) const {
               assert(idx >= 0 || idx < _bufferMasters.size());
               return _bufferMasters[idx];
        }
};

//-----------------------------------------------------------------------------

class TechChar {
public:
        typedef uint32_t Key;
        static const unsigned NUM_BITS_PER_FIELD = 10;
        static const unsigned MAX_NORMALIZED_VAL = 1023;
        static const unsigned LENGTH_UNIT_MICRON = 10;

public:
        TechChar(CtsOptions& options) : _options(&options) {}
        
        void parse(const std::string& lutFile, const std::string solListFile);
        void write(const std::string& file) const;
        
        void report() const;
        void reportSegment(unsigned key) const;
        void reportSegments(uint8_t length, uint8_t load, uint8_t outputSlew) const;
        
        void forEachWireSegment(const std::function<void(unsigned, const WireSegment&)> func) const;
        
        void forEachWireSegment(uint8_t length, 
                                uint8_t load, 
                                uint8_t outputSlew, 
                                const std::function<void(unsigned, const WireSegment&)> func) const;

        WireSegment& createWireSegment(uint8_t length, 
                                       uint8_t load, 
                                       uint8_t outputSlew,
                                       double power, 
                                       unsigned delay, 
                                       uint8_t inputCap,
                                       uint8_t inputSlew);

        const WireSegment& getWireSegment(unsigned idx) const { return _wireSegments[idx]; }
        
        unsigned getMinSegmentLength() const { return _minSegmentLength; }
        unsigned getMaxSegmentLength() const { return _maxSegmentLength; }
        unsigned getMaxCapacitance() const { return _maxCapacitance; }
        unsigned getMaxSlew() const { return _maxSlew; }
        void setActualMinInputCap(unsigned cap) { _actualMinInputCap = cap; }
        unsigned getActualMinInputCap() const { return _actualMinInputCap; }
        unsigned getLengthUnit() const { return _lengthUnit; }
       
        void createFakeEntries(unsigned length, unsigned fakeLength);

        unsigned computeKey(uint8_t length, uint8_t load, uint8_t outputSlew) const {
                return length | (load << NUM_BITS_PER_FIELD) | 
                       (outputSlew << 2 * NUM_BITS_PER_FIELD); 
        }

protected:
        void parseLut(const std::string& file);
        void parseSolList(const std::string& file);
        void initLengthUnits();
        void reportCharacterizationBounds() const;
        void checkCharacterizationBounds() const;

        unsigned toInternalLengthUnit(unsigned length) { 
                return length * _lengthUnitRatio; 
        }

        unsigned _lengthUnit         = 0;
        unsigned _charLengthUnit     = 0;
        unsigned _lengthUnitRatio    = 0;

        unsigned _minSegmentLength   = 0;
        unsigned _maxSegmentLength   = 0;
        unsigned _minCapacitance     = 0;
        unsigned _maxCapacitance     = 0;
        unsigned _minSlew            = 0;
        unsigned _maxSlew            = 0;

        unsigned _actualMinInputCap  = 0;
        unsigned _actualMinInputSlew = 0;

        std::deque<WireSegment> _wireSegments;
        std::unordered_map<Key, std::deque<unsigned>> _keyToWireSegments;

        CtsOptions* _options;
};

}

#endif
