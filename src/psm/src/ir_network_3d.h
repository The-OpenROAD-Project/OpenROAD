// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#pragma once

#include <cstddef>
#include <cstdint>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "connection.h"
#include "node.h"
#include "odb/PtrSetMap.h"
#include "odb/db.h"

namespace utl {
class Logger;
}

namespace psm {

class IRNetwork;

class IRNetwork3D
{
 public:
  using Nodes = std::set<Node*>;

  IRNetwork3D(odb::dbChipNet* chip_net,
              utl::Logger* logger,
              bool floorplanning = false);
  ~IRNetwork3D();

  void construct();
  bool hasSameBonds() const;
  bool isConnected() const;
  Nodes getDisconnectedNodes() const;
  std::vector<std::pair<odb::dbChipInst*, odb::dbNet*>> getChipletNets() const;
  std::vector<std::pair<odb::dbChipInst*, ITermNode*>> getITermNodes() const;
  std::map<Node*, odb::dbChipInst*> getNodeChiplets() const;

  odb::dbChipNet* getChipNet() const { return chip_net_; }
  std::size_t getChipletNetworkCount() const { return networks_.size(); }
  std::size_t getInterDieConnectionCount() const
  {
    return interdie_connections_.size();
  }
  std::size_t getNodeCount() const;
  std::size_t getConnectionCount() const;

  Nodes getNodes() const;
  std::vector<Connection*> getConnections() const;
  odb::PtrSet<odb::dbTechLayer> getLayers() const;
  Node* findTerminalNode(odb::dbChipInst* chip_inst,
                         const std::string& terminal) const;
  std::vector<odb::dbNet*> getNets() const;
  odb::PtrMap<odb::dbInst, Node::NodeSet> getInstanceNodeMapping(
      odb::dbNet* net) const;

  const Connections& getInterDieConnections() const
  {
    return interdie_connections_;
  }

 private:
  struct BondState
  {
    uint32_t id;
    uint32_t source_id;
    uint32_t target_id;
    uint32_t source_bump_id;
    uint32_t target_bump_id;
    float resistance;

    bool operator==(const BondState&) const = default;
  };
  std::vector<BondState> getBondState() const;

  struct ChipletNetwork
  {
    odb::dbChipInst* chip_inst;
    odb::dbNet* net;
    std::unique_ptr<IRNetwork> network;
  };

  ChipletNetwork* findNetwork(odb::dbChipBumpInst* bump_inst);
  Node* findEndpoint(odb::dbChipCapNode* cap_node);

  odb::dbChipNet* chip_net_;
  utl::Logger* logger_;
  bool floorplanning_;
  std::vector<ChipletNetwork> networks_;
  Connections interdie_connections_;
  std::vector<BondState> bond_state_;
};

}  // namespace psm
