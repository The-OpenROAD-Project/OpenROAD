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
#include <tuple>

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

  Node* getNode0() const { return node0_; };
  Node* getNode1() const { return node1_; };

  Node* getOtherNode(const Node* node) const;

  void changeNode(Node* orgnode, Node* newnode);

  virtual Resistance getResistance(const ResistanceMap& res_map) const = 0;
  Conductance getConductance(const ResistanceMap& res_map) const;

  virtual bool isVia() const { return false; };
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

  bool isVia() const override { return true; };

  std::string describe() const override;

 private:
  int cuts_;
};

class TermConnection : public Connection
{
 public:
  TermConnection(Node* node0, Node* node1);

  Resistance getResistance(const ResistanceMap& res_map) const override;
  bool isValid() const override { return true; };

  void mergeWith(const Connection* other) override{};

  std::string describe() const override;

 private:
  static constexpr Resistance resistance_ = 0.001;
};

}  // namespace psm
