// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024-2025, The OpenROAD Authors

#pragma once

#include <map>
#include <memory>
#include <set>
#include <string>
#include <tuple>
#include <vector>

#include "node.h"

namespace utl {
class Logger;
}

namespace odb {
class dbTechLayer;
}

namespace psm {

class Connection
{
 public:
  using Resistance = double;
  using Conductance = double;

  // For routing layers, resistance per square
  // For via layers, resistance per cut
  using ResistanceMap = std::map<odb::dbTechLayer*, Resistance>;

  struct Compare
  {
    bool operator()(const Connection* lhs, const Connection* rhs) const
    {
      return lhs->compare(rhs);
    }
  };

  using ConnectionSet = std::set<Connection*, Compare>;

  template <typename T>
  using ConnectionMap = std::map<Connection*, T, Compare>;

  Connection(Node* node0, Node* node1);
  virtual ~Connection() = default;

  void ensureNodeOrder();

  Node* getNode0() const { return node0_; }
  Node* getNode1() const { return node1_; }

  Node* getOtherNode(const Node* node) const;

  void changeNode(Node* orgnode, Node* newnode);

  virtual Resistance getResistance(const ResistanceMap& res_map) const = 0;
  Conductance getConductance(const ResistanceMap& res_map) const;

  virtual bool isVia() const { return false; }
  virtual bool isValid() const = 0;
  bool isLoop() const { return node0_ == node1_; }
  bool isStub() const { return node0_ == nullptr || node1_ == nullptr; }

  bool hasITermNode() const;
  bool hasBPinNode() const;

  void print(utl::Logger* logger) const;
  virtual std::string describe() const = 0;
  std::string describeWithNodes() const;

  virtual void mergeWith(const Connection* other) = 0;

  bool compare(const Connection* other) const;
  bool compare(const std::unique_ptr<Connection>& other) const;

 protected:
  int getDBUs() const;

  Node* node0_;
  Node* node1_;

 private:
  using CompareInformation
      = std::tuple<Node::CompareInformation, Node::CompareInformation>;
  CompareInformation compareTuple() const;

  template <typename T>
  bool hasNodeOfType() const;
};

using Connections = std::vector<std::unique_ptr<Connection>>;

class LayerConnection : public Connection
{
 public:
  LayerConnection(Node* node0, Node* node1, int length, int width);

  Resistance getResistance(const ResistanceMap& res_map) const override;
  bool isValid() const override;

  void mergeWith(const Connection* other) override;

  std::string describe() const override;

 private:
  int length_;
  int width_;
};

class ViaConnection : public Connection
{
 public:
  ViaConnection(Node* node0, Node* node1, int cuts);

  Resistance getResistance(const ResistanceMap& res_map) const override;
  bool isValid() const override;

  void mergeWith(const Connection* other) override;

  bool isVia() const override { return true; }

  std::string describe() const override;

 private:
  int cuts_;
};

class TermConnection : public Connection
{
 public:
  TermConnection(Node* node0, Node* node1);

  Resistance getResistance(const ResistanceMap& res_map) const override
  {
    return kResistance;
  }
  bool isValid() const override { return true; }

  void mergeWith(const Connection* other) override {}

  std::string describe() const override;

 private:
  static constexpr Resistance kResistance = 0.001;
};

class FixedResistanceConnection : public Connection
{
 public:
  FixedResistanceConnection(Node* node0, Node* node1, Resistance resistance);

  Resistance getResistance(const ResistanceMap& res_map) const override
  {
    return res_;
  };
  bool isValid() const override { return true; }

  void mergeWith(const Connection* other) override {}

  std::string describe() const override;

 private:
  Resistance res_;
};

}  // namespace psm
