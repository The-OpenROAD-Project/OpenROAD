////////////////////////////////////////////////////////////////////////////////
// Authors: Mateus Fogaça, Isadora Oliveira and Marcelo Danigno
// 
//          (Advisor: Ricardo Reis and Paulo Butzen)
//
// BSD 3-Clause License
//
// Copyright (c) 2020, Federal University of Rio Grande do Sul (UFRGS)
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
////////////////////////////////////////////////////////////////////////////////


#include "DBWrapper.h"
#include "Graph.h"
#include <iostream>

namespace PartClusManager {

class ChacoOptions {
public:
        ChacoOptions() = default;
       
        void setNumberStarts(unsigned numStarts) { _numStarts = numStarts; }
        unsigned getNumberStarts() const { return _numStarts; } 
        void setNumberPartitions(unsigned numParts) { _numPartitions = numParts; }
        unsigned getNumberPartitions() const { return _numPartitions; } 
        void setWeightedVertices(bool enable) { _weightedVertices = enable; } 
        bool getWeightedVertices() const { return _weightedVertices; }
        void setCoarRatio(double cratio) { _coarRatio = cratio; }
        double getCoarRatio() { return _coarRatio; }
        void setCoarVertices(unsigned coarVertices) { _coarVertices = coarVertices; }
        unsigned getCoarVertices() const { return _coarVertices; } 
        void setTermProp(bool enable) { _termProp = enable; } 
        bool getTermProp() const { return _termProp; }
        void setCutHopRatio(double ratio) { _cutHopRatio = ratio; }
        double getCutHopRatio() { return _cutHopRatio; }
        void setArchitecture(bool enable) { _architecture = enable; } 
        bool getArchitecture() const { return _architecture; }
        void setArchTopology(const std::string& archString) { _archTopology = archString; }
        std::string getArchTopology() const { return _archTopology; } 
                
private:
        unsigned        _numStarts              = 0;
        unsigned        _numPartitions          = 0;
        bool            _weightedVertices       = false;
        double          _coarRatio              = 0.8;
        unsigned        _coarVertices           = 2500;
        bool            _termProp               = true;
        double          _cutHopRatio            = 1.0;
        bool            _architecture           = false;
        std::string     _archTopology           = "";
};

class GPMetisOptions {
public:
        GPMetisOptions() = default;
       
        void setNumberStarts(unsigned numStarts) { _numStarts = numStarts; }
        unsigned getNumberStarts() const { return _numStarts; } 
        void setNumberPartitions(unsigned numParts) { _numPartitions = numParts; }
        unsigned getNumberPartitions() const { return _numPartitions; } 
        void setWeightedVertices(bool enable) { _weightedVertices = enable; } 
        bool getWeightedVertices() const { return _weightedVertices; }
                
private:
        unsigned        _numStarts              = 0;
        unsigned        _numPartitions          = 0;
        bool            _weightedVertices       = false;
};

class MLPartOptions {
public:
        MLPartOptions() = default;
       
        void setNumberStarts(unsigned numStarts) { _numStarts = numStarts; }
        unsigned getNumberStarts() const { return _numStarts; } 
        void setNumberPartitions(unsigned numParts) { _numPartitions = numParts; }
        unsigned getNumberPartitions() const { return _numPartitions; } 
                
private:
        unsigned        _numStarts              = 0;
        unsigned        _numPartitions          = 0;
};

class PartClusManagerKernel {
protected:

private:
        DBWrapper _dbWrapper;

public:
        PartClusManagerKernel() = default;
        void runChaco();
        void runChaco(const Graph& graph, const ChacoOptions& options);
        void runGpMetis();
        void runGpMetis(const Graph& graph, const GPMetisOptions& options);
        void runMlPart();
        void runMlPart(const Graph& graph, const MLPartOptions& options);
};

}
