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

#pragma once

#include "Util.h"

#include "db.h"

#include <cassert>
#include <deque>
#include <functional>
#include <unordered_map>
#include <vector>

namespace utl {
class Logger;
} // namespace utl

namespace cts {

enum InstType : uint8_t
{
  CLOCK_BUFFER,
  CLOCK_SINK,
  CLOCK_GATING_CELL
};

class ClockInst
{
 public:
  ClockInst(const std::string& name,
            const std::string& master,
            InstType type,
            DBU x,
            DBU y,
            odb::dbITerm* pinObj = nullptr,
            float inputCap = 0.0)
      : _name(name),
        _master(master),
        _type(type),
        _location(x, y),
        _inputPinObj(pinObj),
        _inputCap(inputCap)
  {
  }

  std::string getName() const { return _name; }
  std::string getMaster() const { return _master; }
  DBU getX() const { return _location.getX(); }
  DBU getY() const { return _location.getY(); }
  Point<DBU> getLocation() const { return _location; }

  void setInstObj(odb::dbInst* inst) { _instObj = inst; }
  odb::dbInst* getDbInst() const { return _instObj; }
  void setInputPinObj(odb::dbITerm* pin) { _inputPinObj = pin; }
  odb::dbITerm* getDbInputPin() const { return _inputPinObj; }
  void setInputCap(float cap) { _inputCap = cap; }
  float getInputCap() const { return _inputCap;}

  bool isClockBuffer() const { return _type == CLOCK_BUFFER; }

 protected:
  std::string _name;
  std::string _master;
  InstType _type;
  Point<DBU> _location;
  odb::dbInst* _instObj = nullptr;
  odb::dbITerm* _inputPinObj = nullptr;
  float _inputCap;
};

//-----------------------------------------------------------------------------

class Clock
{
 public:
  class SubNet
  {
   protected:
    std::string _name;
    std::deque<ClockInst*> _instances;
    std::unordered_map<ClockInst*, unsigned> _mapInstToIdx;
    bool _leafLevel;

   public:
    SubNet(const std::string& name) : _name(name), _leafLevel(false) {}

    void setLeafLevel(bool isLeaf) { _leafLevel = isLeaf; }
    bool isLeafLevel() const { return _leafLevel; }

    void addInst(ClockInst& inst)
    {
      _instances.push_back(&inst);
      _mapInstToIdx[&inst] = _instances.size() - 1;
    }

    unsigned findIndex(ClockInst* inst) const { return _mapInstToIdx.at(inst); }

    void replaceSink(ClockInst* curSink, ClockInst* newSink)
    {
      unsigned idx = findIndex(curSink);
      _instances[idx] = newSink;
      _mapInstToIdx.erase(curSink);
      _mapInstToIdx[newSink] = idx;
    }

    std::string getName() const { return _name; }
    int getNumSinks() const { return _instances.size() - 1; }

    ClockInst* getDriver() const
    {
      assert(_instances.size() > 0);
      return _instances[0];
    }

    void forEachSink(const std::function<void(ClockInst*)>& func) const
    {
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
        const std::string& sdcClockName,
        DBU clockPinX,
        DBU clockPinY)
      : _netName(netName),
        _clockPin(clockPin),
        _sdcClockName(sdcClockName),
        _clockPinX(clockPinX),
        _clockPinY(clockPinY){};

  ClockInst& addClockBuffer(const std::string& name,
                            const std::string& master,
                            DBU x,
                            DBU y)
  {
    _clockBuffers.emplace_back(
        name + "_" + getName(), master, CLOCK_BUFFER, x, y);
    _mapNameToInst[name + "_" + getName()] = &_clockBuffers.back();
    return _clockBuffers.back();
  }

  ClockInst* findClockByName(std::string name)
  {
    if (_mapNameToInst.find(name) == _mapNameToInst.end()) {
      return nullptr;
    } else {
      return _mapNameToInst.at(name);
    }
  }

  SubNet& addSubNet(const std::string& name)
  {
    _subNets.emplace_back(name + "_" + getName());
    return _subNets.back();
  }

  void addSink(const std::string& name, DBU x, DBU y)
  {
    _sinks.emplace_back(name, "", CLOCK_SINK, x, y);
  }

  void addSink(const std::string& name, DBU x, DBU y, odb::dbITerm* pinObj, float inputCap)
  {
    _sinks.emplace_back(name, "", CLOCK_SINK, x, y, pinObj, inputCap);
  }

  std::string getName() const { return _netName; }
  unsigned getNumSinks() const { return _sinks.size(); }

  Box<DBU> computeSinkRegion();
  Box<double> computeSinkRegionClustered(
      std::vector<std::pair<float, float>> sinks);
  Box<double> computeNormalizedSinkRegion(double factor);

  void report(utl::Logger* _logger) const;

  void forEachSink(const std::function<void(const ClockInst&)>& func) const;
  void forEachSink(const std::function<void(ClockInst&)>& func);
  void forEachClockBuffer(
      const std::function<void(const ClockInst&)>& func) const;
  void forEachClockBuffer(const std::function<void(ClockInst&)>& func);
  void forEachSubNet(const std::function<void(const SubNet&)>& func) const
  {
    for (const SubNet& subNet : _subNets) {
      func(subNet);
    }
  }

  void forEachSubNet(const std::function<void(SubNet&)>& func)
  {
    unsigned size = _subNets.size();
    // We want to use ranged for loops in here beacause
    // the user may add new items to _subNets.
    // C++11 "for each" loops will crash due to invalid
    // iterators.
    for (unsigned idx = 0; idx < size; ++idx) {
      func(_subNets[idx]);
    }
  }

  void setMaxLevel(unsigned level) { _numLevels = level; }
  unsigned getMaxLevel() const { return _numLevels; }

  void setNetObj(odb::dbNet* net) { _netObj = net; }
  odb::dbNet* getNetObj() { return _netObj; }

 private:
  std::string _netName;
  std::string _clockPin;
  std::string _sdcClockName;
  DBU _clockPinX;
  DBU _clockPinY;

  std::deque<ClockInst> _sinks;
  std::deque<ClockInst> _clockBuffers;
  std::deque<SubNet> _subNets;
  std::unordered_map<std::string, ClockInst*> _mapNameToInst;

  odb::dbNet* _netObj;

  unsigned _numLevels = 0;
};

}  // namespace cts
