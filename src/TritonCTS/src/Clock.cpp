#include "Clock.h"

#include <iostream>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <limits>
#include <algorithm>

namespace TritonCTS {

void Clock::report() const {
        std::cout << " ************************************\n";
        std::cout << " *         Clock net report         *\n";
        std::cout << " ************************************\n";
        std::cout << " Net name: " << _netName << "\n";
        std::cout << " Clock pin: " << _clockPin << " (" 
                  << _clockPinX << ", " << _clockPinY << ")\n";
        std::cout << " Number of sinks: " << _sinks.size() << "\n";
        std::cout << " ***********************************\n\n";
        
        std::cout << std::left << std::setw(45) << "Pin name" << "Pos" << "\n";
        forEachSink( [&] (const ClockInst& sink) {
                        std::cout << std::left << std::setw(45) << sink.getName() 
                                  << "(" << sink.getX() << ", " 
                                  << sink.getY() << ")\n";
                });
        
}

Box<DBU> Clock::computeSinkRegion() {
        double percentile = 0.01;

        std::vector<DBU> allPositionsX;                
        std::vector<DBU> allPositionsY;                
        forEachSink( [&] (const ClockInst& sink) {
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

Box<double> Clock::computeNormalizedSinkRegion(double factor) {
        Box<DBU> sinkRegion = computeSinkRegion();
        return sinkRegion.normalize(factor);
}

void Clock::forEachSink(const std::function<void(const ClockInst&)>& func) const {
        for (const ClockInst& sink: _sinks) {
                func(sink);
        }
}

void Clock::forEachSink(const std::function<void(ClockInst&)>& func) {
        for (ClockInst& sink: _sinks) {
                func(sink);
        }
}

void Clock::forEachClockBuffer(const std::function<void(const ClockInst&)>& func) const {
        for (const ClockInst& clockBuffer: _clockBuffers) {
                func(clockBuffer);
        }
}

}
