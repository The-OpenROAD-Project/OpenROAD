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

#ifndef __NETLIST_H_
#define __NETLIST_H_

#include <functional>
#include <iostream>
#include <limits>
#include <string>
#include <vector>

#include "opendb/db.h"

namespace pin_placer {

using odb::Point;
using odb::Rect;

struct Constraint;
enum class Edge;

enum class Orientation
{
  North,
  South,
  East,
  West
};
enum class Direction
{
  Input,
  Output,
  Inout,
  Feedthru,
  Invalid
};

class InstancePin
{
 protected:
  std::string _name;
  odb::Point _pos;

 public:
  InstancePin(const std::string& name, const odb::Point& pos)
      : _name(name), _pos(pos)
  {
  }
  std::string getName() const { return _name; }
  odb::Point getPos() const { return _pos; }
  int getX() const { return _pos.getX(); }
  int getY() const { return _pos.getY(); }
};

class IOPin : public InstancePin
{
 private:
  Orientation _orientation;
  Direction _direction;
  odb::Point _lowerBound;
  odb::Point _upperBound;
  std::string _netName;
  std::string _locationType;

 public:
  IOPin(const std::string& name,
        const odb::Point& pos,
        Direction dir,
        odb::Point lowerBound,
        odb::Point upperBound,
        std::string netName,
        std::string locationType)
      : InstancePin(name, pos),
        _orientation(Orientation::North),
        _direction(dir),
        _lowerBound(lowerBound),
        _upperBound(upperBound),
        _netName(netName),
        _locationType(locationType)
  {
  }

  void setOrientation(const Orientation o) { _orientation = o; }
  Orientation getOrientation() const { return _orientation; }
  odb::Point getPosition() const { return _pos; }
  void setX(const int x) { _pos.setX(x); }
  void setY(const int y) { _pos.setY(y); }
  void setPos(const odb::Point pos) { _pos = pos; }
  void setPos(const int x, const int y) { _pos = odb::Point(x, y); }
  void setLowerBound(const int x, const int y) { _lowerBound = odb::Point(x, y); };
  void setUpperBound(const int x, const int y) { _upperBound = odb::Point(x, y); };
  Direction getDirection() const { return _direction; }
  odb::Point getLowerBound() const { return _lowerBound; };
  odb::Point getUpperBound() const { return _upperBound; };
  std::string getNetName() const { return _netName; }
  std::string getLocationType() const { return _locationType; };
};

class Netlist
{
 private:
  std::vector<InstancePin> _instPins;
  std::vector<int> _netPointer;
  std::vector<IOPin> _ioPins;

  bool checkSlotForPin(IOPin& pin, Edge edge, odb::Point& point,
                       std::vector<Constraint> restrictions);
  bool checkInterval(Constraint constraint, Edge edge, int pos);

 public:
  Netlist();

  void addIONet(const IOPin&, const std::vector<InstancePin>&);

  void forEachIOPin(std::function<void(int, IOPin&)>);
  void forEachIOPin(std::function<void(int, const IOPin&)>) const;
  void forEachSinkOfIO(int, std::function<void(InstancePin&)>);
  void forEachSinkOfIO(int, std::function<void(const InstancePin&)>) const;
  int numSinksOfIO(int);
  int numIOPins();

  int computeIONetHPWL(int, odb::Point);
  int computeIONetHPWL(int, odb::Point, Edge, std::vector<Constraint>&);
  int computeDstIOtoPins(int, odb::Point);
  odb::Rect getBB(int, odb::Point);
};

}  // namespace pin_placer
#endif /* __NETLIST_H_ */
