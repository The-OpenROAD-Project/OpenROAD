#ifndef CLOCK_H
#define CLOCK_H

#include "Util.h"

#include <deque>
#include <functional>
#include <cassert>
#include <vector>
#include <unordered_map>

namespace TritonCTS {

enum InstType : uint8_t {
        CLOCK_BUFFER,
        CLOCK_SINK,
        CLOCK_GATING_CELL
};


class ClockInst {
public:
        ClockInst(const std::string& name,
                  const std::string& master,
                  InstType type,
                  DBU x, DBU y) : 
                  _name(name), 
                  _master(master),
                  _type(type),
                  _location(x, y) {}

        std::string getName() const { return _name; }
        std::string getMaster() const { return _master; }
        DBU getX() const { return _location.getX(); }
        DBU getY() const { return _location.getY(); }
        Point<DBU> getLocation() const { return _location; }

        bool isClockBuffer() const { return _type == CLOCK_BUFFER; }

protected:
        std::string _name;
        std::string _master;
        InstType    _type;
        Point<DBU>  _location;
};

//-----------------------------------------------------------------------------

class Clock {
public:
        class SubNet { 
        protected:
                std::string _name;
                std::deque<ClockInst*> _instances;
                std::unordered_map<ClockInst*, unsigned> _mapInstToIdx;
                bool _leafLevel;
        public:
                SubNet(const std::string& name) : _name(name), _leafLevel(false) {}
                
                void setLeafLevel(bool isLeaf) { _leafLevel = isLeaf; }
                bool isLeafLevel() const { return _leafLevel; }
                
                void addInst(ClockInst& inst) { 
                        _instances.push_back(&inst);
                        _mapInstToIdx[&inst] = _instances.size() - 1;
                }

                unsigned findIndex(ClockInst* inst) const {
                        return _mapInstToIdx.at(inst);
                }

                void replaceSink(ClockInst *curSink, ClockInst *newSink) {
                        unsigned idx = findIndex(curSink);
                        _instances[idx] = newSink;
                        _mapInstToIdx.erase(curSink);
                        _mapInstToIdx[newSink] = idx;
                }

                std::string getName() const { return _name; }
                int getNumSinks() const { return _instances.size() - 1; }

                ClockInst* getDriver() const { 
                        assert(_instances.size() > 0);
                        return _instances[0]; 
                }

                void forEachSink(const std::function<void(ClockInst*)>& func) const {
                        if (_instances.size() < 2) {
                                return;
                        }
                        for (unsigned inst = 1; inst < _instances.size(); ++inst) {
                                func(_instances[inst]);
                        }
                }
        };
        
        Clock(const std::string& netName, 
                    const std::string& clockPin, 
                    DBU clockPinX, 
                    DBU clockPinY) : 
                    _netName(netName),
                    _clockPin(clockPin), 
                    _clockPinX(clockPinX), 
                    _clockPinY(clockPinY) {};
        
        ClockInst& addClockBuffer(const std::string& name, 
                                  const std::string& master,
                                  DBU x, DBU y ) {
                _clockBuffers.emplace_back(name + "_" + getName(), master, CLOCK_BUFFER, x, y);
                return _clockBuffers.back();
        }

        SubNet& addSubNet(const std::string& name) {
                _subNets.emplace_back(name + "_" + getName());
                return _subNets.back();
        }

        void addSink(const std::string& name, DBU x, DBU y) {
                _sinks.emplace_back(name, "", CLOCK_SINK, x, y);
        }

        std::string getName() const { return _netName; }
        unsigned getNumSinks() const { return _sinks.size(); }

        Box<DBU> computeSinkRegion();
        Box<double> computeNormalizedSinkRegion(double factor);
        
        void report() const;
        
        void forEachSink(const std::function<void(const ClockInst&)>& func) const;
        void forEachSink(const std::function<void(ClockInst&)>& func);
        void forEachClockBuffer(const std::function<void(const ClockInst&)>& func) const;
        void forEachSubNet(const std::function<void(const SubNet&)>& func) const {
                for (const SubNet& subNet: _subNets) {
                        func(subNet);
                }
        }
        
        void forEachSubNet(const std::function<void(SubNet&)>& func) {
		unsigned size = _subNets.size();
		// We want to use ranged for loops in here beacause 
		// the user may add new items to _subNets.
		// C++11 "for each" loops will crash due to invalid
		// iterators.
		for (unsigned idx = 0; idx < size; ++idx) {                       
			func(_subNets[idx]);
                }
        }

private:
        std::string _netName;
        std::string _clockPin;
        DBU         _clockPinX;
        DBU         _clockPinY;
        
        std::deque<ClockInst> _sinks;
        std::deque<ClockInst> _clockBuffers;
        std::deque<SubNet>    _subNets;

};

}

#endif 
