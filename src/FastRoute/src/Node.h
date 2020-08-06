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

namespace FastRoute {

enum class NodeType
{
  SOURCE,
  SINK,
  STEINER,
  INVALID
};

const char* nodeTypeString(NodeType type);

class Node
{
 private:
  Coordinate _position;
  int _layer;
  NodeType _type;

 public:
  Node() : _position(Coordinate(0, 0)), _layer(-1), _type(NodeType::INVALID) {}

  Node(const Coordinate& position, const int layer, const NodeType type)
      : _position(position), _layer(layer), _type(type)
  {
  }

  Node(const DBU x, const DBU y, const int layer, const NodeType type)
      : _position(Coordinate(x, y)), _layer(layer), _type(type)
  {
  }

  bool operator==(const Node& node) const
  {
    return (_position == node._position && _layer == node._layer);
  }

  bool operator!=(const Node& node) const
  {
    return (_position != node._position || _layer != node._layer);
  }

  Coordinate getPosition() const { return _position; }
  int getLayer() const { return _layer; }
  NodeType getType() const { return _type; }

  void setType(NodeType type) { _type = type; }
};

}  // namespace FastRoute
