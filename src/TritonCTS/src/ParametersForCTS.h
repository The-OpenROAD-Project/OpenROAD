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

#ifndef PARAMETERSFORCTS_H
#define PARAMETERSFORCTS_H

#include "Util.h"

#include <string>

namespace TritonCTS {

class ParametersForCTS {
public:
        ParametersForCTS() {
                _blockName       = "";
                _lutFile         = "";
                _clockNets       = "";
                _libertyFile     = "";
                _verilogFile     = "";
                _sdcFile         = "";
                _rootBuffer      = "";
                _numSinkRegions  = -1;
                _dbUnits         = -1;
                _wireSegmentUnit = 0;
                _dbId            = 0;
        }

        void setBlockName(const std::string& blockName) { _blockName = blockName; }
        std::string getBlockName() const { return _blockName; } 
        void setLutFile(const std::string& lutFile) { _lutFile = lutFile; }
        std::string getLutFile() const { return _lutFile; } 
        void setClockNets(const std::string& clockNets) { _clockNets = clockNets; }
        std::string getClockNets() const { return _clockNets; }
        void setLibertyFiles(const std::string& libertyFile) { _libertyFile = libertyFile; }
        std::string getLibertyFiles() const { return _libertyFile; }
        void setVerilogFile(const std::string& verilogFile) { _verilogFile = verilogFile; }
        std::string getVerilogFile() const { return _verilogFile; }
        void setSdcFile(const std::string& sdcFile) { _sdcFile = sdcFile; }
        std::string getSdcFile() const { return _sdcFile; }
        void setRootBuffer(const std::string& buffer) { _rootBuffer = buffer; }
        std::string getRootBuffer() const { return  _rootBuffer; }
        void setNumSinkRegions(unsigned numSinkRegions) { _numSinkRegions = numSinkRegions; }
        unsigned getNumSinkRegions() const { return _numSinkRegions; }
        void setDbUnits(DBU units) { _dbUnits = units; }
        DBU getDbUnits() const { return _dbUnits; }
        void setWireSegmentUnit(unsigned wireSegmentUnit) { _wireSegmentUnit = wireSegmentUnit; }
        unsigned getWireSegmentUnit() { return _wireSegmentUnit; }
        void setDbId(unsigned id) { _dbId = id; }
        unsigned getDbId() { return _dbId; }

private:
        std::string _blockName;
        std::string _lutFile;
        std::string _clockNets;
        std::string _libertyFile;
        std::string _verilogFile;
        std::string _sdcFile;
        std::string _rootBuffer;
        unsigned    _numSinkRegions;
        DBU         _dbUnits;
        unsigned    _wireSegmentUnit;
        unsigned    _dbId;
};

}

#endif
