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

class CtsOptions {
public:
        CtsOptions() = default;
       
        void setBlockName(const std::string& blockName) { _blockName = blockName; }
        std::string getBlockName() const { return _blockName; } 
        void setLutFile(const std::string& lutFile) { _lutFile = lutFile; }
        std::string getLutFile() const { return _lutFile; } 
        void setSolListFile(const std::string& solListFile) { _solListFile = solListFile; }
        std::string getSolListFile() const { return _solListFile; } 
        void setClockNets(const std::string& clockNets) { _clockNets = clockNets; }
        std::string getClockNets() const { return _clockNets; }
        void setRootBuffer(const std::string& buffer) { _rootBuffer = buffer; }
        std::string getRootBuffer() const { return  _rootBuffer; }
        void setDbUnits(DBU units) { _dbUnits = units; }
        DBU getDbUnits() const { return _dbUnits; }
        void setWireSegmentUnit(unsigned wireSegmentUnit) { _wireSegmentUnit = wireSegmentUnit; }
        unsigned getWireSegmentUnit() const { return _wireSegmentUnit; }
        void setDbId(unsigned id) { _dbId = id; }
        unsigned getDbId() const { return _dbId; }
        void setPlotSolution(bool plot) { _plotSolution = plot; } 
        bool getPlotSolution() const { return _plotSolution; }
        void setNumMaxLeafSinks(unsigned numSinks) { _numMaxLeafSinks = numSinks; }
        unsigned getNumMaxLeafSinks() const { return _numMaxLeafSinks; }        
        void setMaxSlew(unsigned slew) { _maxSlew = slew; }
        unsigned getMaxSlew() const { return _maxSlew; }
        void setClockTreeMaxDepth(unsigned depth) { _clockTreeMaxDepth = depth; }
        unsigned getClockTreeMaxDepth() const { return _clockTreeMaxDepth; }
        void setEnableFakeLutEntries(bool enable) { _enableFakeLutEntries = enable; }
        unsigned isFakeLutEntriesEnabled() const { return _enableFakeLutEntries; }
        void setForceBuffersOnLeafLevel(bool force) { _forceBuffersOnLeafLevel = force; }
        bool forceBuffersOnLeafLevel() const { return _forceBuffersOnLeafLevel; }
        void setWriteOnlyClockNets(bool writeOnlyClk) { _writeOnlyClockNets = writeOnlyClk; }
        bool writeOnlyClockNets() const { return _writeOnlyClockNets; }
        void setRunPostCtsOpt(bool run) { _runPostCtsOpt = run; }
        bool runPostCtsOpt() { return _runPostCtsOpt; }
        void setBufDistRatio(double ratio) { _bufDistRatio = ratio; }
        double getBufDistRatio() { return _bufDistRatio; }
                
private:
        std::string _blockName               = "";
        std::string _lutFile                 = "";
        std::string _solListFile             = "";
        std::string _clockNets               = "";
        std::string _rootBuffer              = "";
        DBU         _dbUnits                 = -1;
        unsigned    _wireSegmentUnit         = 0;
        unsigned    _dbId                    = 0;
        bool        _plotSolution            = false;
        unsigned    _numMaxLeafSinks         = 15;
        unsigned    _maxSlew                 = 4;
        unsigned    _clockTreeMaxDepth       = 100;
        bool        _enableFakeLutEntries    = true;
        bool        _forceBuffersOnLeafLevel = true;
        bool        _writeOnlyClockNets      = false;
        bool        _runPostCtsOpt           = true;
        double      _bufDistRatio            = 0.1;
};

}

#endif
