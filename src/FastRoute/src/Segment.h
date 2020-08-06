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

#include <string>
#include <vector>

#include "Coordinate.h"
#include "Node.h"

namespace FastRoute {

class Segment
{
 private:
  int _index;
  Node _firstNode;
  Node _lastNode;
  int _parent;

 public:
  Segment() = default;

  Segment(int index, Node firstNode, Node lastNode, int parent)
      : _index(index),
        _firstNode(firstNode),
        _lastNode(lastNode),
        _parent(parent)
  {
  }

  bool operator==(const Segment& segment)
  {
    return (
        ((_firstNode == segment._firstNode && _lastNode == segment._lastNode)
         || (_firstNode == segment._lastNode
             && _lastNode == segment._firstNode))
        && _index == segment.getIndex());
  }

  int getIndex() const { return _index; }
  Node getFirstNode() const { return _firstNode; }
  Node getLastNode() const { return _lastNode; }
  int getParent() const { return _parent; }

  void setParent(int parent) { _parent = parent; }

  void printSegment()
  {
    std::cout << "----(" << _firstNode.getPosition().getX() << ", "
              << _firstNode.getPosition().getY() << ", "
              << _firstNode.getLayer() << "); ("
              << _lastNode.getPosition().getX() << ", "
              << _lastNode.getPosition().getY() << ", " << _lastNode.getLayer()
              << ")\n";
  };
};

}  // namespace FastRoute
