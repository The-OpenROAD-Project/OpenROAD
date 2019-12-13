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

#include "ClockTreeBuilder.h"

#include <iostream>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <limits>
#include <algorithm>

namespace TritonCTS {

void ClockNet::report() const {
        std::cout << " ************************************\n";
        std::cout << " *         Clock net report         *\n";
        std::cout << " ************************************\n";
        std::cout << " Net name: " << _netName << "\n";
        std::cout << " Clock pin: " << _clockPin << " (" 
                  << _clockPinX << ", " << _clockPinY << ")\n";
        std::cout << " Number of sinks: " << _sinks.size() << "\n";
        std::cout << " ***********************************\n\n";
        
        std::cout << std::left << std::setw(45) << "Pin name" << "Pos" << "\n";
        forEachSink( [&] (const ClockInstance& sink) {
                        std::cout << std::left << std::setw(45) << sink.getName() 
                                  << "(" << sink.getX() << ", " 
                                  << sink.getY() << ")\n";
                });
        
}

Box<DBU> ClockNet::computeSinkRegion() {
        double percentile = 0.01;

        std::vector<DBU> allPositionsX;                
        std::vector<DBU> allPositionsY;                
        forEachSink( [&] (const ClockInstance& sink) {
                allPositionsX.push_back(sink.getX());
                allPositionsY.push_back(sink.getY());
        });                       

        std::sort(allPositionsX.begin(), allPositionsX.end());
        std::sort(allPositionsY.begin(), allPositionsY.end());

        unsigned numSinks = allPositionsX.size();
        unsigned numOutliers = percentile * numSinks;
        DBU xMin = allPositionsX[numOutliers];
        DBU xMax = allPositionsX[numSinks - numOutliers - 1]; 
        DBU yMin = allPositionsY[numOutliers];
        DBU yMax = allPositionsY[numSinks - numOutliers - 1]; 
        
        return Box<DBU>(xMin, yMin, xMax, yMax);
}

Box<double> ClockNet::computeNormalizedSinkRegion(double factor) {
        Box<DBU> sinkRegion = computeSinkRegion();
        return sinkRegion.normalize(factor);
}

void ClockNet::forEachSink(const std::function<void(const ClockInstance&)>& func) const {
        for (const ClockInstance& sink: _sinks) {
                func(sink);
        }
}

void ClockNet::forEachSink(const std::function<void(ClockInstance&)>& func) {
        for (ClockInstance& sink: _sinks) {
                func(sink);
        }
}

void ClockNet::forEachClockBuffer(const std::function<void(const ClockInstance&)>& func) const {
        for (const ClockInstance& clockBuffer: _clockBuffers) {
                func(clockBuffer);
        }
}

void GHTreeBuilder::run() {
        std::cout << " Generating GH-Tree topology for net " << _clockNet.getName() << "...\n";
        initSinkRegion();
}

void GHTreeBuilder::initSinkRegion() {
        unsigned wireSegmentUnitInMicron = _parms->getWireSegmentUnit(); 
        DBU dbUnits = _parms->getDbUnits();
        _wireSegmentUnit = wireSegmentUnitInMicron * dbUnits;

        std::cout << " Wire segment unit: " << _wireSegmentUnit << " dbu ("
                  << wireSegmentUnitInMicron << " um)\n";

        Box<DBU> sinkRegionDbu = _clockNet.computeSinkRegion();
        std::cout << " Original sink region: " << sinkRegionDbu << "\n";
        
        _sinkRegion = sinkRegionDbu.normalize(1.0/_wireSegmentUnit);
        std::cout << " Normalized sink region: " << _sinkRegion << "\n";
        std::cout << "  Width:  " << _sinkRegion.getWidth() << "\n";
        std::cout << "  Height: " << _sinkRegion.getHeight() << "\n";
}

}
