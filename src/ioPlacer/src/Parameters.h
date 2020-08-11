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


#ifndef __PARAMETERS_H_
#define __PARAMETERS_H_

#include <string>

#include "Coordinate.h"

namespace ioPlacer {

class Parameters {
       private:
        int _horizontalMetalLayer = 1;
        int _verticalMetalLayer = 2;
        std::string _inputLefFile;
        std::string _inputDefFile;
        std::string _outputDefFile;
        
        bool _reportHPWL = false;
        bool _forceSpread = true;
        int _numSlots = -1;
        int _randomMode = 0;
        float _slotsFactor = -1;
        float _usage = -1;
        float _usageFactor = -1;
        std::string _blockagesFile;
        float _horizontalThicknessMultiplier = 1;
        float _verticalThicknessMultiplier = 1;
        float _horizontalLengthExtend = -1;
        float _verticalLengthExtend = -1;
        float _horizontalLength = -1;
        float _verticalLength = -1;
        bool  _interactiveMode = false;
        int _numThreads = -1;
        double _randSeed = 42.0;
        unsigned _dbId;
        int _boundariesOffset;
        int _minDist;

       public:
        Parameters() = default;
        
        void setInputDefFile(const std::string& file) { _inputDefFile = file; }
        std::string getInputDefFile() const { return _inputDefFile; }
        std::string getInputLefFile() const { return _inputLefFile; }
        void setOutputDefFile(const std::string& file) { _outputDefFile = file; }
        std::string getOutputDefFile() const { return _outputDefFile; }
        void setHorizontalMetalLayer(int layer) { _horizontalMetalLayer = layer; }
        int getHorizontalMetalLayer() const { return _horizontalMetalLayer; }
        void setVerticalMetalLayer(int layer) { _verticalMetalLayer = layer; }
        int getVerticalMetalLayer() const { return _verticalMetalLayer; }
        void setReportHPWL(bool report) { _reportHPWL = report; }
        bool getReportHPWL() const { return _reportHPWL; }
        void setNumSlots(int numSlots) { _numSlots = numSlots; }
        int getNumSlots() const { return _numSlots; }
        void setRandomMode(int mode) { _randomMode = mode; }
        int getRandomMode() const { return _randomMode; }
        void setSlotsFactor(float factor) { _slotsFactor = factor; } 
        float getSlotsFactor() const { return _slotsFactor; }
        void setUsage(float usage) { _usage = usage; }
        float getUsage() const { return _usage; }
        void setUsageFactor(float factor) { _usageFactor = factor; }
        float getUsageFactor() const { return _usageFactor; }
        void setForceSpread(bool forceSpread) { _forceSpread = forceSpread; } 
        bool getForceSpread() const { return _forceSpread; }
        void setBlockagesFile(const std::string& file) { _blockagesFile = file; }
        std::string getBlockagesFile() const { return _blockagesFile; }
        void setHorizontalLengthExtend(float length) { _horizontalLengthExtend = length; }
        float getHorizontalLengthExtend() const { return _horizontalLengthExtend; }
        void setVerticalLengthExtend(float length) { _verticalLengthExtend = length; }
        float getVerticalLengthExtend() const { return _verticalLengthExtend; }
        void setHorizontalLength(float length) { _horizontalLength = length; }
        float getHorizontalLength() const { return _horizontalLength; }
        void setVerticalLength(float length) { _verticalLength = length; }
        float getVerticalLength() const { return _verticalLength; }
        void setInteractiveMode(bool enable) { _interactiveMode = enable; }
        bool isInteractiveMode() const { return _interactiveMode; }
        void setNumThreads(int numThreads) { _numThreads = numThreads; }
        int  getNumThreads() const { return _numThreads; }
        void setRandSeed(double seed) { _randSeed = seed; }
        double getRandSeed() const { return _randSeed; }
        void setHorizontalThicknessMultiplier(float length) { _horizontalThicknessMultiplier = length; }
        float getHorizontalThicknessMultiplier() const { return _horizontalThicknessMultiplier; }
        void setVerticalThicknessMultiplier(float length) { _verticalThicknessMultiplier = length; }
        float getVerticalThicknessMultiplier() const { return _verticalThicknessMultiplier; }
        void setBoundariesOffset(int offset) { _boundariesOffset = offset; }
        int getBoundariesOffset() const { return _boundariesOffset; }
        void setMinDistance(int minDist) { _minDist = minDist; }
        int getMinDistance() const { return _minDist; }

        void setDbId(unsigned idx) { _dbId = idx; }
        unsigned getDbId() const { return _dbId; }

        void printAll() const;
};

}
#endif /* __PARAMETERS_H_ */
