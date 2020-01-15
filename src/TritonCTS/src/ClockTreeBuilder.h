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

#ifndef CLOCKTREEBUILDER_H
#define CLOCKTREEBUILDER_H

#include <vector>
#include <string>
#include <unordered_map>
#include <functional>
#include <deque>

#include "Util.h"
#include "CtsOptions.h"
#include "Characterization.h"

namespace TritonCTS {

class ClockInstance {
public:
        enum InstType : uint8_t {
                CLOCK_BUFFER,
                CLOCK_SINK,
                CLOCK_GATING_CELL
        };
        
        ClockInstance(const std::string& name,
                      const std::string& master,
                      InstType type,
                      DBU x, DBU y) : 
                      _name(name), 
                      _master(master),
                      _type(type),
                      _x(x), _y(y) {}

        std::string getName() const { return _name; }
        std::string getMaster() const { return _master; }
        DBU getX() const { return _x; }
        DBU getY() const { return _y; }

        bool isClockBuffer() const { return _type == CLOCK_BUFFER; }

protected:
        std::string _name;
        std::string _master;
        InstType    _type;
        DBU _x;
        DBU _y;
};

//-----------------------------------------------------------------------------

class ClockNet {
public:
        class SubNet { 
        protected:
                std::string _name;
                std::deque<ClockInstance*> _instances;
        public:
                SubNet(const std::string& name) : _name(name) {}

                void addInstance(ClockInstance& inst) { 
                        _instances.push_back(&inst);
                }

                std::string getName() const { return _name; }

                ClockInstance* getDriver() const { 
                        assert(_instances.size() > 0);
                        return _instances[0]; 
                }

                void forEachSink(const std::function<void(ClockInstance*)>& func) const {
                        if (_instances.size() < 2) {
                                return;
                        }
                        for (unsigned inst = 1; inst < _instances.size(); ++inst) {
                                func(_instances[inst]);
                        }
                }
        };
        
        ClockNet(const std::string& netName, 
                 const std::string& clockPin, 
                 DBU clockPinX, 
                 DBU clockPinY) : 
                 _netName(netName),
                 _clockPin(clockPin), 
                 _clockPinX(clockPinX), 
                 _clockPinY(clockPinY) {};
        
        ClockInstance& addClockBuffer(const std::string& name, 
                            const std::string& master,
                            DBU x, DBU y ) {
                _clockBuffers.emplace_back(name, master, ClockInstance::CLOCK_BUFFER, x, y);
                return _clockBuffers.back();
        }

        SubNet& addSubNet(const std::string& name) {
                _subNets.emplace_back(name);
                return _subNets.back();
        }

        void addSink(const std::string& name, DBU x, DBU y) {
                _sinks.emplace_back(name, "", ClockInstance::CLOCK_SINK, x, y);
        }

        std::string getName() const { return _netName; }
        unsigned getNumSinks() const { return _sinks.size(); }

        Box<DBU> computeSinkRegion();
        Box<double> computeNormalizedSinkRegion(double factor);
        
        void report() const;
        
        void forEachSink(const std::function<void(const ClockInstance&)>& func) const;
        void forEachSink(const std::function<void(ClockInstance&)>& func);
        void forEachClockBuffer(const std::function<void(const ClockInstance&)>& func) const;
        void forEachSubNet(const std::function<void(const SubNet&)>& func) const {
                for (const SubNet& subNet: _subNets) {
                        func(subNet);
                }
        }

private:
        std::string _netName;
        std::string _clockPin;
        DBU         _clockPinX;
        DBU         _clockPinY;
        
        std::deque<ClockInstance> _sinks;
        std::deque<ClockInstance> _clockBuffers;
        std::deque<SubNet>        _subNets;

};

//-----------------------------------------------------------------------------

class ClockTreeBuilder {
public:
        ClockTreeBuilder(CtsOptions& options, ClockNet& net) : 
                         _options(&options), _clockNet(net) {};
        
        virtual void run() = 0;
        void setCharacterization(Characterization& techChar) { _techChar = &techChar; }
        const ClockNet& getClockNet() const { return _clockNet; }        

protected:
        CtsOptions*       _options = nullptr;
        ClockNet          _clockNet;
        Characterization* _techChar = nullptr;
};

//-----------------------------------------------------------------------------

class GHTreeBuilder : public ClockTreeBuilder {
public:
        GHTreeBuilder(CtsOptions& options, ClockNet& net) : 
                      ClockTreeBuilder(options, net) {};
        
        void run();
private:
        void initSinkRegion();

        Box<double> _sinkRegion;
        DBU _wireSegmentUnit = -1;
};

}
#endif
