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
#include "db.h"

#include <string>
#include <iostream>
#include <sstream>
#include <vector>

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
        void setBufferList(std::vector<std::string> buffers) { _bufferList = buffers; }
        std::vector<std::string> getBufferList() const { return  _bufferList; }
        void setDbUnits(DBU units) { _dbUnits = units; }
        DBU getDbUnits() const { return _dbUnits; }
        void setWireSegmentUnit(unsigned wireSegmentUnit) { _wireSegmentUnit = wireSegmentUnit; }
        unsigned getWireSegmentUnit() const { return _wireSegmentUnit; }
        void setDbId(unsigned id) { _dbId = id; }
        unsigned getDbId() const { return _dbId; }
        void setPlotSolution(bool plot) { _plotSolution = plot; } 
        bool getPlotSolution() const { return _plotSolution; }
        void setOnlyCharacterization(bool enable) { _onlyCharacterization = enable; } 
        bool getOnlyCharacterization() const { return _onlyCharacterization; }
        void setNumMaxLeafSinks(unsigned numSinks) { _numMaxLeafSinks = numSinks; }
        unsigned getNumMaxLeafSinks() const { return _numMaxLeafSinks; }        
        void setMaxSlew(unsigned slew) { _maxSlew = slew; }
        unsigned getMaxSlew() const { return _maxSlew; }
        void setMaxCharSlew(double slew) { _maxCharSlew = slew; }
        double getMaxCharSlew() const { return _maxCharSlew; }
        void setMaxCharCap(double cap) { _maxCharCap = cap; }
        double getMaxCharCap() const { return _maxCharCap; }
        void setCharLoadIterations(unsigned loadIterations) { _charLoadIterations = loadIterations; }
        unsigned getCharLoadIterations() const { return _charLoadIterations; }
        void setCharSlewIterations(unsigned slewIterations) { _charSlewIterations = slewIterations; }
        unsigned getCharSlewIterations() const { return _charSlewIterations; }
        void setCharWirelengthIterations(unsigned wirelengthIterations) { _charWirelengthIterations = wirelengthIterations; }
        unsigned getCharWirelengthIterations() const { return _charWirelengthIterations; }
        void setOutputPath(const std::string& path) { _outputPath = path; }
        std::string getOutputPath() const { return _outputPath; } 
        void setCapPerSqr(double cap) { _capPerSqr = cap; }
        double getCapPerSqr() const { return _capPerSqr; }
        void setResPerSqr(double res) { _resPerSqr = res; }
        double getResPerSqr() const { return _resPerSqr; }
        void setCapInter(double cap) { _capInter = cap; }
        double getCapInter() const { return _capInter; }
        void setSlewInter(double slew) { _slewInter = slew; }
        double getSlewInter() const { return _slewInter; }
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
        void setClockNetsObjs(std::vector<odb::dbNet*> nets) { _clockNetsObjs = nets; }
        std::vector<odb::dbNet*> getClockNetsObjs() const { return  _clockNetsObjs; }
        void setMetricsFile(const std::string& metricFile) { _metricFile = metricFile; }
        std::string getMetricsFile() const { return _metricFile; } 
        void setNumClockRoots(unsigned roots) { _clockRoots = roots; }
        long int getNumClockRoots() const { return _clockRoots; }
        void setNumClockSubnets(long int nets) { _clockSubnets = nets; }
        long int getNumClockSubnets() const { return _clockSubnets; }
        void setNumBuffersInserted(long int buffers) { _buffersInserted = buffers; }
        long int getNumBuffersInserted() const { return _buffersInserted; }
        void setNumSinks(long int sinks) { _sinks = sinks; }
        long int getNumSinks() const { return _sinks; }
                
private:
        std::string _blockName                  = "";
        std::string _lutFile                    = "";
        std::string _solListFile                = "";
        std::string _outputPath                 = "";
        std::string _clockNets                  = "";
        std::string _rootBuffer                 = "";
        std::string _metricFile                 = "";
        DBU         _dbUnits                    = -1;
        unsigned    _wireSegmentUnit            = 0;
        unsigned    _dbId                       = 0;
        bool        _plotSolution               = false;
        bool        _onlyCharacterization       = false;
        unsigned    _numMaxLeafSinks            = 15;
        unsigned    _maxSlew                    = 4;
        double      _maxCharSlew                = 0;
        double      _maxCharCap                 = 0;
        double      _capPerSqr                  = 0;
        double      _resPerSqr                  = 0;
        double      _capInter                   = 0;
        double      _slewInter                  = 0;
        unsigned    _charWirelengthIterations   = 4;
        unsigned    _charLoadIterations         = 34;
        unsigned    _charSlewIterations         = 12;
        unsigned    _clockTreeMaxDepth          = 100;
        bool        _enableFakeLutEntries       = true;
        bool        _forceBuffersOnLeafLevel    = true;
        bool        _writeOnlyClockNets         = false;
        bool        _runPostCtsOpt              = true;
        double      _bufDistRatio               = 0.1;
        long int    _clockRoots                 = 0;
        long int    _clockSubnets               = 0;
        long int    _buffersInserted            = 0;
        long int    _sinks                      = 0;
        std::vector<std::string> _bufferList;
        std::vector<odb::dbNet*> _clockNetsObjs;
};

}

#endif
