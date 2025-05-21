// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "LatenciesBalancer.h"

#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "Clock.h"
#include "CtsOptions.h"
#include "TreeBuilder.h"
#include "cts/TritonCTS.h"
#include "odb/db.h"

namespace cts {

using utl::CTS;

void LatanciesBalancer::run()
{
  findAllBuilders(root_);
}

void LatanciesBalancer::findAllBuilders(TreeBuilder* builder)
{
  expandBuilderGraph(builder);
  for (const auto& child : builder->getChildren()) {
    findAllBuilders(child);
  }
}

void LatanciesBalancer::expandBuilderGraph(TreeBuilder* builder)
{
  std::string topBufferName = builder->getTopBufferName();
  odb::dbBlock* block = db_->getChip()->getBlock();
  odb::dbInst* topBuffer = block->findInst(topBufferName.c_str());
  if(topBuffer == nullptr) {
    logger_->error(CTS, 544, "Could not find top buffer {}", topBufferName);
  }

  // Building graph

  // Find node of the src of the tree. If there is none we create one,
  // should happend only for the root builder
  odb::dbNet* topBufInputNet = getFirstInputNet(topBuffer);
  odb::dbITerm* builderInputTerm = topBufInputNet->getFirstOutput();
  std::string builderSrcName;

  if(!builderInputTerm) {
    builderSrcName = topBufInputNet->getName();
  } else {
    builderSrcName = builderInputTerm->getInst()->getName();
  }

  int builderSrcId = getNodeIdByName(builderSrcName);
  if(builderSrcId == -1) {
    builderSrcId = graph_.size();
    GraphNode builderSrcNode = GraphNode(builderSrcId, builderSrcName, -1);
    graph_.push_back(builderSrcNode);
  }

  // Create the node for the root of the tree
  int topBufId = graph_.size();
  GraphNode topBufNode = GraphNode(topBufId, topBufferName, builderSrcId);
  graph_.push_back(topBufNode);
  graph_[builderSrcId].childrenIds.push_back(topBufId);

  // If the builder is leaf don't expand it, as we don't want to insert delay
  // inside this tree
  if(builder->isLeafTree()) {
    return;
  }

  // Expand clock tree to possibly insert delay
  Clock& clockNet = builder->getClock();
  clockNet.forEachSubNet([&](ClockSubNet& subNet) {
    odb::dbInst* driver = subNet.getDriver()->getDbInst();
    int driverId = getNodeIdByName(driver->getName());
    
    if(driverId == -1) {
      logger_->error(CTS, 545, "Could not find node for driver: {}", driver->getName());
    }

    subNet.forEachSink([&](ClockInst* inst) {
      int sinkId = graph_.size();
      std::string sinkName;
      if(!inst->isClockBuffer()) {
        odb::dbITerm* sinkIterm = inst->getDbInputPin();
        sinkName = sinkIterm->getInst()->getName();
      } else {
        sinkName = inst->getName();
      }
      GraphNode sinkNode = GraphNode(sinkId, sinkName, driverId);
      graph_.push_back(sinkNode);
      graph_[driverId].childrenIds.push_back(sinkId);
    });
  });
}

int LatanciesBalancer::getNodeIdByName(std::string name)
{
  for(GraphNode node : graph_) {
    if(node.name == name) {
      return node.id;
    }
  }
  return -1;
}

odb::dbNet* LatanciesBalancer::getFirstInputNet(odb::dbInst* inst) const
{
  odb::dbSet<odb::dbITerm> iterms = inst->getITerms();
  for (odb::dbITerm* iterm : iterms) {
    if (iterm->isInputSignal()) {
      return iterm->getNet();
    }
  }

  return nullptr;
}

}  // namespace cts
