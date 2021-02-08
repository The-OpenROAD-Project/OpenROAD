///////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// Copyright (c) 2019-2020, The Regents of the University of California
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
// ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
// SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
// CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
// OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <iostream>
#include <string>
#include <vector>

namespace sta {
class Pin;
class Instance;
}  // namespace sta

namespace mpl {

class MacroCircuit;
class Vertex;

class Edge
{
 public:
  Vertex* from() const;
  Vertex* to() const;
  int weight() const;

  Edge();
  Edge(Vertex* from, Vertex* to, int weight);

 private:
  Vertex* from_;
  Vertex* to_;
  int weight_;
};

class PinGroup;

// pin, instMacro, instOther, nonInit
enum VertexType
{
  PinGroupType,
  MacroInstType,
  OtherInstType,
  Uninitialized
};

class Vertex
{
 public:
  Vertex();

  // PinGroup is input then VertexType must be PinGroupType
  Vertex(PinGroup* pinGroup);

  // need to specify between macro or other
  Vertex(sta::Instance* inst, VertexType vertexType);

  // Will be removed soon
  Vertex(void* ptr, VertexType vertexType);
  void* ptr() const;

  VertexType vertexType() const;
  const std::vector<int>& from();
  const std::vector<int>& to();

  PinGroup* pinGroup() const;
  sta::Instance* instance() const;

  bool isPinGroup() const;
  bool isInstance() const;

  void setPinGroup(PinGroup* pinGroup);
  void setInstance(sta::Instance* inst);

 private:
  VertexType vertexType_;
  std::vector<int> from_;
  std::vector<int> to_;

  // This can be either PinGroup / sta::Instance*,
  // based on vertexType
  void* ptr_;
};

enum PinGroupLocation
{
  West,
  East,
  North,
  South
};

class PinGroup
{
 public:
  PinGroupLocation pinGroupLocation() const;
  const std::vector<sta::Pin*>& pins();
  const std::string name() const;

  void setPinGroupLocation(PinGroupLocation);
  void addPin(sta::Pin* pin);

 private:
  PinGroupLocation pinGroupLocation_;
  std::vector<sta::Pin*> pins_;
};

}  // namespace mpl
