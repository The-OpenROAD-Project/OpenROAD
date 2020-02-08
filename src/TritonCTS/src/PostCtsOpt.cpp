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

#include "PostCtsOpt.h"

#include <iostream> 

namespace TritonCTS {

void PostCtsOpt::run() {
        initSourceSinkDists();
        fixSourceSinkDists();
}

void PostCtsOpt::initSourceSinkDists() {
        _clock->forEachSubNet([&] (const Clock::SubNet& subNet) {
                if (!subNet.isLeafLevel()) {
                        return;
                }

                computeNetSourceSinkDists(subNet);
        });

        _avgSourceSinkDist /= _clock->getNumSinks();
        std::cout << " Avg. source sink dist: " << _avgSourceSinkDist << " dbu.\n";
}

void PostCtsOpt::computeNetSourceSinkDists(const Clock::SubNet& subNet) {
        const ClockInst* driver = subNet.getDriver();
        Point<DBU> driverLoc = driver->getLocation();

        subNet.forEachSink([&] (const ClockInst* sink) {
                  Point<DBU> sinkLoc = sink->getLocation();
                        std::string sinkName = sink->getName();
                        DBU dist = driverLoc.computeDist(sinkLoc);
                        _avgSourceSinkDist += dist;
                        _sinkDistMap[sinkName] = dist;
        });                           
}

void PostCtsOpt::fixSourceSinkDists() {
        _clock->forEachSubNet([&] (Clock::SubNet& subNet) {
                if (!subNet.isLeafLevel()) {
                        return;
                }

                fixNetSourceSinkDists(subNet);
        });
        
        std::cout << " Num outlier sinks: " << _numViolatingSinks << "\n";
}

void PostCtsOpt::fixNetSourceSinkDists(Clock::SubNet& subNet) {
        ClockInst* driver = subNet.getDriver();

        subNet.forEachSink([&] (ClockInst* sink) {
                std::string sinkName = sink->getName();
                unsigned dist = _sinkDistMap[sinkName];        
                if (dist > 5 * _avgSourceSinkDist) {
                        createSubClockNet(subNet, driver, sink);
                        ++_numViolatingSinks;
                }
        });
}

void PostCtsOpt::createSubClockNet(Clock::SubNet& net, ClockInst* driver, ClockInst* sink) {
        std::string master = _options->getRootBuffer();
        
        Point<DBU> bufLoc = computeBufferLocation(driver, sink);

        ClockInst& clkBuffer = _clock->addClockBuffer("clkbuf_opt_" + std::to_string(_numInsertedBuffers),
                                                      master, bufLoc.getX(), bufLoc.getY());

        net.replaceSink(sink, &clkBuffer);
        
        Clock::SubNet& newSubNet = _clock->addSubNet("clknet_opt_" + std::to_string(_numInsertedBuffers));       
        newSubNet.addInst(clkBuffer);
        newSubNet.addInst(*sink);

        ++_numInsertedBuffers;
}

Point<DBU> PostCtsOpt::computeBufferLocation(ClockInst* driver, ClockInst* sink) const {
        Point<DBU> driverLoc = driver->getLocation();
        Point<DBU> sinkLoc = sink->getLocation();       
        unsigned xDist = driverLoc.computeDistX(sinkLoc); 
        unsigned yDist = driverLoc.computeDistY(sinkLoc); 
        
        double xBufDistRatio = _bufDistRatio;
        if (sinkLoc.getX() < driverLoc.getX()) {
                xBufDistRatio = - _bufDistRatio;
        }
        
        double yBufDistRatio = _bufDistRatio;
        if (sinkLoc.getY() < driverLoc.getY()) {
                yBufDistRatio = - _bufDistRatio;
        }
        
        Point<DBU> bufLoc(driverLoc.getX() + xBufDistRatio * xDist, 
                          driverLoc.getY() + yBufDistRatio * yDist);
        
        //std::cout << "DriverLoc: " << driverLoc << " SinkLoc: " << sinkLoc
        //          << " Dist: [(" << xDist << ", " << yDist << ")] "
        //          << " BufLoc: " << bufLoc << "\n";

        return bufLoc;
}

}
