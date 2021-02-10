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

#include "graph.h"
#include "sta/Network.hh"

namespace mpl {

////////////////////////////////////////////////
// Vertex

Vertex::Vertex() : vertexType_(VertexType::Uninitialized), ptr_(nullptr)
{
}

std::string Vertex::name(sta::Network *network)
{
  if (isInstance())
    return network->pathName(static_cast<sta::Instance*>(ptr_));
  else
    return static_cast<PinGroup*>(ptr_)->name();
}

Vertex::Vertex(PinGroup* pinGroup)
{
  ptr_ = static_cast<void*>(pinGroup);
  vertexType_ = VertexType::PinGroupType;
}

Vertex::Vertex(sta::Instance* inst, VertexType vertexType)
{
  ptr_ = static_cast<void*>(inst);
  vertexType_ = vertexType;
}

// will be removed soon
Vertex::Vertex(void* ptr, VertexType vertexType) : Vertex()
{
  ptr_ = ptr;
  vertexType_ = vertexType;
}

void* Vertex::ptr() const
{
  return ptr_;
}

VertexType Vertex::vertexType() const
{
  return vertexType_;
}

PinGroup* Vertex::pinGroup() const
{
  if (isPinGroup()) {
    return static_cast<PinGroup*>(ptr_);
  }
  return nullptr;
}

sta::Instance* Vertex::instance() const
{
  if (isInstance()) {
    return static_cast<sta::Instance*>(ptr_);
  }
  return nullptr;
}

bool Vertex::isPinGroup() const
{
  return vertexType_ == VertexType::PinGroupType;
}

bool Vertex::isInstance() const
{
  return vertexType_ != VertexType::PinGroupType;
}

////////////////////////////////////////////////
// PinGroup

PinGroupLocation PinGroup::pinGroupLocation() const
{
  return pinGroupLocation_;
}

const std::vector<sta::Pin*>& PinGroup::pins()
{
  return pins_;
}

const std::string PinGroup::name() const
{
  if (pinGroupLocation_ == PinGroupLocation::West) {
    return "West";
  } else if (pinGroupLocation_ == PinGroupLocation::East) {
    return "East";
  } else if (pinGroupLocation_ == PinGroupLocation::North) {
    return "North";
  } else if (pinGroupLocation_ == PinGroupLocation::South) {
    return "South";
  }
  return "??";
}

void PinGroup::setPinGroupLocation(PinGroupLocation pgl)
{
  pinGroupLocation_ = pgl;
}

void PinGroup::addPin(sta::Pin* pin)
{
  pins_.push_back(pin);
}

}  // namespace mpl
