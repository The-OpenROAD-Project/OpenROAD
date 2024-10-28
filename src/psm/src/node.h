///////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// Copyright (c) 2024, The Regents of the University of California
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

#pragma once

#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

#include "odb/geom_boost.h"

namespace odb {
class dbTechLayer;
class dbBPin;
class dbITerm;
}  // namespace odb

namespace utl {
class Logger;
}

namespace psm {
class Shape;

class Node
{
 public:
  enum class NodeType
  {
    Node,
    Source,
    ITerm,
    BPin,
    Unknown
  };

  struct Compare
  {
    bool operator()(const Node* lhs, const Node* rhs) const
    {
      return lhs->compare(rhs);
    }
  };

  using NodeSet = std::set<Node*, Compare>;

  Node(const odb::Point& pt, odb::dbTechLayer* layer);
  virtual ~Node() = default;

  bool compare(const Node* other) const;
  bool compare(const std::unique_ptr<Node>& other) const;

  const odb::Point& getPoint() const { return pt_; };
  odb::dbTechLayer* getLayer() const { return layer_; };

  void print(utl::Logger* logger, const std::string& prefix = "") const;
  virtual std::string describe(const std::string& prefix) const;

  std::string getName() const;
  std::string getTypeName() const;

  using CompareInformation = std::tuple<int, int, int, NodeType, int>;
  CompareInformation compareTuple() const;
  static CompareInformation dummyCompareTuple();

 protected:
  virtual NodeType getType() const { return NodeType::Node; }

  virtual int getTypeCompareInfo() const { return 0; };

 private:
  double getDBUs() const;

  odb::Point pt_;
  odb::dbTechLayer* layer_;
};

class SourceNode : public Node
{
 public:
  SourceNode(Node* node);

  Node* getSource() const { return source_; }

 protected:
  NodeType getType() const override { return NodeType::Source; }

 private:
  Node* source_;
};

class TerminalNode : public Node
{
 public:
  TerminalNode(const odb::Rect& shape, odb::dbTechLayer* layer);

  const odb::Rect& getShape() const { return shape_; }

 private:
  odb::Rect shape_;
};

class ITermNode : public TerminalNode
{
 public:
  ITermNode(odb::dbITerm* iterm, const odb::Point& pt, odb::dbTechLayer* layer);

  odb::dbITerm* getITerm() const { return iterm_; }

  std::string describe(const std::string& prefix) const override;

 protected:
  NodeType getType() const override { return NodeType::ITerm; }

  int getTypeCompareInfo() const override;

 private:
  odb::dbITerm* iterm_;
};

class BPinNode : public TerminalNode
{
 public:
  BPinNode(odb::dbBPin* pin, const odb::Rect& shape, odb::dbTechLayer* layer);

  const odb::dbBPin* getBPin() const { return pin_; }

 protected:
  NodeType getType() const override { return NodeType::BPin; }

  int getTypeCompareInfo() const override;

 private:
  odb::dbBPin* pin_;
};

}  // namespace psm
