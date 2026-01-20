// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024-2025, The OpenROAD Authors

#pragma once

#include <map>
#include <memory>
#include <set>
#include <string>
#include <tuple>
#include <vector>

#include "odb/geom.h"
#include "odb/geom_boost.h"

namespace odb {
class dbBox;
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
    kNode,
    kSource,
    kITerm,
    kBPin,
    kUnknown
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
  virtual NodeType getType() const { return NodeType::kNode; }

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
  NodeType getType() const override { return NodeType::kSource; }

 private:
  Node* source_;
};

using SourceNodes = std::vector<std::unique_ptr<SourceNode>>;

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
  NodeType getType() const override { return NodeType::kITerm; }

  int getTypeCompareInfo() const override;

 private:
  odb::dbITerm* iterm_;
};

class BPinNode : public TerminalNode
{
 public:
  BPinNode(odb::dbBPin* pin, odb::dbBox* box, odb::dbTechLayer* layer);

  const odb::dbBPin* getBPin() const { return pin_; }
  bool shouldConnect() const;

 protected:
  NodeType getType() const override { return NodeType::kBPin; }

  int getTypeCompareInfo() const override;

 private:
  odb::dbBPin* pin_;
  odb::dbBox* box_;

  static constexpr const char* kDisconnectProperty = "PSM_DISCONNECT";
};

}  // namespace psm
