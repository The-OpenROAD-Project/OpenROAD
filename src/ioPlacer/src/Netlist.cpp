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

#include "Netlist.h"
#include "Slots.h"
#include "ioplacer/IOPlacer.h"

namespace ioPlacer {

Netlist::Netlist()
{
  _netPointer.push_back(0);
}

void Netlist::addIONet(const IOPin& ioPin,
                       const std::vector<InstancePin>& instPins)
{
  _ioPins.push_back(ioPin);
  _instPins.insert(_instPins.end(), instPins.begin(), instPins.end());
  _netPointer.push_back(_instPins.size());
}

void Netlist::forEachIOPin(std::function<void(int idx, IOPin&)> func)
{
  for (int idx = 0; idx < _ioPins.size(); ++idx) {
    func(idx, _ioPins[idx]);
  }
}

void Netlist::forEachIOPin(
    std::function<void(int idx, const IOPin&)> func) const
{
  for (int idx = 0; idx < _ioPins.size(); ++idx) {
    func(idx, _ioPins[idx]);
  }
}

void Netlist::forEachSinkOfIO(int idx,
                              std::function<void(InstancePin&)> func)
{
  int netStart = _netPointer[idx];
  int netEnd = _netPointer[idx + 1];
  for (int idx = netStart; idx < netEnd; ++idx) {
    func(_instPins[idx]);
  }
}

void Netlist::forEachSinkOfIO(
    int idx,
    std::function<void(const InstancePin&)> func) const
{
  int netStart = _netPointer[idx];
  int netEnd = _netPointer[idx + 1];
  for (int idx = netStart; idx < netEnd; ++idx) {
    func(_instPins[idx]);
  }
}

int Netlist::numSinksOfIO(int idx)
{
  int netStart = _netPointer[idx];
  int netEnd = _netPointer[idx + 1];
  return netEnd - netStart;
}

int Netlist::numIOPins()
{
  return _ioPins.size();
}

Rect Netlist::getBB(int idx, Point slotPos)
{
  int netStart = _netPointer[idx];
  int netEnd = _netPointer[idx + 1];

  int minX = slotPos.x();
  int minY = slotPos.y();
  int maxX = slotPos.x();
  int maxY = slotPos.y();

  for (int idx = netStart; idx < netEnd; ++idx) {
    Point pos = _instPins[idx].getPos();
    minX = std::min(minX, pos.x());
    maxX = std::max(maxX, pos.x());
    minY = std::min(minY, pos.y());
    maxY = std::max(maxY, pos.y());
  }

  Point upperBounds = Point(maxX, maxY);
  Point lowerBounds = Point(minX, minY);

  Rect netBBox(lowerBounds, upperBounds);
  return netBBox;
}

int Netlist::computeIONetHPWL(int idx, Point slotPos)
{
  int netStart = _netPointer[idx];
  int netEnd = _netPointer[idx + 1];

  int minX = slotPos.x();
  int minY = slotPos.y();
  int maxX = slotPos.x();
  int maxY = slotPos.y();

  for (int idx = netStart; idx < netEnd; ++idx) {
    Point pos = _instPins[idx].getPos();
    minX = std::min(minX, pos.x());
    maxX = std::max(maxX, pos.x());
    minY = std::min(minY, pos.y());
    maxY = std::max(maxY, pos.y());
  }

  int x = maxX - minX;
  int y = maxY - minY;

  return (x + y);
}

int Netlist::computeIONetHPWL(int idx, Point slotPos, Edge edge, 
                              std::vector<Constraint>& constraints)
{
  int hpwl;

  if (checkSlotForPin(_ioPins[idx], edge, slotPos, constraints)) {
    hpwl = computeIONetHPWL(idx, slotPos);
  } else {
    hpwl = std::numeric_limits<int>::max();
  }

  return hpwl;
}

int Netlist::computeDstIOtoPins(int idx, Point slotPos)
{
  int netStart = _netPointer[idx];
  int netEnd = _netPointer[idx + 1];

  int totalDistance = 0;

  for (int idx = netStart; idx < netEnd; ++idx) {
    Point pinPos = _instPins[idx].getPos();
    totalDistance += std::abs(pinPos.x() - slotPos.x())
                     + std::abs(pinPos.y() - slotPos.y());
  }

  return totalDistance;
}

bool Netlist::checkSlotForPin(IOPin& pin, Edge edge, odb::Point& point,
                        std::vector<Constraint> constraints)
{
  for (Constraint constraint : constraints) {
    int pos = (edge == Edge::Top || edge == Edge::Bottom) ?
               point.x() :  point.y();

    if (pin.getDirection() == constraint.direction) {
      if (constraint.interval.getEdge() == edge &&
          pos >= constraint.interval.getBegin() &&
          pos <= constraint.interval.getEnd()) {
        return true;
      } else {
        return false;
      }
    }
  }

  return true;
}

}  // namespace ioPlacer
