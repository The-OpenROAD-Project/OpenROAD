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
#include "sta/Graph.hh"
#include "sta/GraphDelayCalc.hh"
#include "sta/Liberty.hh"
#include "sta/PathAnalysisPt.hh"
#include "sta/PathEnd.hh"
#include "sta/PathExpanded.hh"
#include "sta/Sdc.hh"


namespace cts {

using utl::CTS;

void LatenciesBalancer::run()
{
  initSta();
  computeTopBufferDelay(root_);
  findAllBuilders(root_);
  computeLeafsNumBufferToInsert(0);
}

void LatenciesBalancer::initSta() {
  openSta_->ensureGraph();
  openSta_->searchPreamble();
  openSta_->ensureClkNetwork();
  openSta_->ensureClkArrivals();
  timingGraph_ = openSta_->graph();
}

void LatenciesBalancer::findAllBuilders(TreeBuilder* builder)
{
  expandBuilderGraph(builder);
  for (const auto& child : builder->getChildren()) {
    findAllBuilders(child);
  }
}

void LatenciesBalancer::expandBuilderGraph(TreeBuilder* builder)
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
  odb::dbITerm* builderTopBufInputTerm = getFirstInput(topBuffer);
  odb::dbNet* topBufInputNet = builderTopBufInputTerm->getNet();
  odb::dbITerm* builderDriverTerm = topBufInputNet->getFirstOutput();
  std::string builderSrcName;

  if(!builderDriverTerm) {
    builderSrcName = topBufInputNet->getName();
  } else {
    builderSrcName = builderDriverTerm->getInst()->getName();
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
    computeAveSinkArrivals(builder);
    worseDelay_ = std::max(worseDelay_, builder->getAveSinkArrival());
    graph_[topBufId].delay = builder->getAveSinkArrival();
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

int LatenciesBalancer::getNodeIdByName(std::string name)
{
  for(GraphNode node : graph_) {
    if(node.name == name) {
      return node.id;
    }
  }
  return -1;
}

odb::dbITerm* LatenciesBalancer::getFirstInput(odb::dbInst* inst) const
{
  odb::dbSet<odb::dbITerm> iterms = inst->getITerms();
  for (odb::dbITerm* iterm : iterms) {
    if (iterm->isInputSignal()) {
      return iterm;
    }
  }

  return nullptr;
}

float LatenciesBalancer::getVertexClkArrival(sta::Vertex* sinkVertex,
                                     odb::dbNet* topNet,
                                     odb::dbITerm* iterm)
{
  sta::VertexPathIterator pathIter(sinkVertex, openSta_);
  float clkPathArrival = 0.0;
  while (pathIter.hasNext()) {
    sta::Path* path = pathIter.next();
    const sta::ClockEdge* clockEdge = path->clkEdge(openSta_);
    if (clockEdge == nullptr) {
      continue;
    }

    if (clockEdge->transition() != sta::RiseFall::rise()) {
      // only populate with rising edges
      continue;
    }

    if (path->dcalcAnalysisPt(openSta_)->delayMinMax() != sta::MinMax::max()) {
      continue;
      // only populate with max delay
    }

    const sta::Clock* clock = path->clock(openSta_);
    if (clock) {
      sta::PathExpanded expand(path, openSta_);
      const sta::Path* start = expand.startPath();

      odb::dbNet* pathStartNet = nullptr;

      odb::dbITerm* term;
      odb::dbBTerm* port;
      odb::dbModITerm* modIterm;
      network_->staToDb(start->pin(openSta_), term, port, modIterm);
      if (term) {
        pathStartNet = term->getNet();
      }
      if (port) {
        pathStartNet = port->getNet();
      }
      if (pathStartNet == topNet) {
        clkPathArrival = path->arrival();
        return clkPathArrival;
      }
    }
  }
  logger_->warn(CTS, 179, "No paths found for pin {}.", iterm->getName());
  return clkPathArrival;
}

void LatenciesBalancer::computeAveSinkArrivals(TreeBuilder* builder)
{
  Clock clock = builder->getClock();
  odb::dbNet* topInputClockNet = builder->getTopInputNet();
  // compute average input arrival at all sinks
  float sumArrivals = 0.0;
  unsigned numSinks = 0;
  clock.forEachSink([&](const ClockInst& sink) {
    odb::dbITerm* iterm = sink.getDbInputPin();
    computeSinkArrivalRecur(
        topInputClockNet, iterm, sumArrivals, numSinks);
  });
  float aveArrival = sumArrivals / (float) numSinks;
  builder->setAveSinkArrival(aveArrival);
  debugPrint(logger_,
             CTS,
             "insertion delay",
             1,
             "{} {}: average sink arrival is {:0.3e}",
             (builder->getTreeType() == TreeType::MacroTree) ? "macro tree"
                                                             : "register tree",
             clock.getName(),
             builder->getAveSinkArrival());
}

void LatenciesBalancer::computeSinkArrivalRecur(odb::dbNet* topClokcNet,
                                        odb::dbITerm* iterm,
                                        float& sumArrivals,
                                        unsigned& numSinks)
{
  if (iterm) {
    odb::dbInst* inst = iterm->getInst();
    if (inst) {
      if (isSink(iterm)) {
        // either register or macro input pin
        sta::Pin* pin = network_->dbToSta(iterm);
        if (pin) {
          sta::Vertex* sinkVertex = timingGraph_->pinDrvrVertex(pin);
          float arrival = getVertexClkArrival(sinkVertex, topClokcNet, iterm);
          // add insertion delay
          float insDelay = 0.0;
          sta::LibertyCell* libCell
              = network_->libertyCell(network_->dbToSta(inst));
          odb::dbMTerm* mterm = iterm->getMTerm();
          if (libCell && mterm) {
            sta::LibertyPort* libPort
                = libCell->findLibertyPort(mterm->getConstName());
            if (libPort) {
              const float rise = libPort->clkTreeDelay(
                  0.0, sta::RiseFall::rise(), sta::MinMax::max());
              const float fall = libPort->clkTreeDelay(
                  0.0, sta::RiseFall::fall(), sta::MinMax::max());

              if (rise != 0 || fall != 0) {
                insDelay = (rise + fall) / 2.0;
              }
            }
          }
          sumArrivals += (arrival + insDelay);
          numSinks++;
        }
      } else {
        // not a sink, but a clock gater
        odb::dbITerm* outTerm = inst->getFirstOutput();
        if (outTerm) {
          odb::dbNet* outNet = outTerm->getNet();
          bool propagate = propagateClock(iterm);
          if (outNet && propagate) {
            odb::dbSet<odb::dbITerm> iterms = outNet->getITerms();
            odb::dbSet<odb::dbITerm>::iterator iter;
            for (iter = iterms.begin(); iter != iterms.end(); ++iter) {
              odb::dbITerm* inTerm = *iter;
              if (inTerm->getIoType() == odb::dbIoType::INPUT) {
                computeSinkArrivalRecur(
                    topClokcNet, inTerm, sumArrivals, numSinks);
              }
            }
          }
        }
      }
    }
  }
}

void LatenciesBalancer::computeTopBufferDelay(TreeBuilder* builder)
{
  Clock clock = builder->getClock();
  odb::dbBlock* block = db_->getChip()->getBlock();
  odb::dbInst* topBuffer = block->findInst(builder->getTopBufferName().c_str());
  if (topBuffer) {
    builder->setTopBuffer(topBuffer);
    odb::dbITerm* inputTerm = getFirstInput(topBuffer);
    odb::dbITerm* outputTerm = topBuffer->getFirstOutput();
    sta::Pin* inputPin = network_->dbToSta(inputTerm);
    sta::Pin* outputPin = network_->dbToSta(outputTerm);

    float inputArrival = openSta_->pinArrival(
        inputPin, sta::RiseFall::rise(), sta::MinMax::max());
    float outputArrival = openSta_->pinArrival(
        outputPin, sta::RiseFall::rise(), sta::MinMax::max());
    float bufferDelay = outputArrival - inputArrival;
    builder->setTopBufferDelay(bufferDelay);
    debugPrint(logger_,
               CTS,
               "insertion delay",
               1,
               "top buffer delay for {} {} is {:0.3e}",
               (builder->getTreeType() == TreeType::MacroTree)
                   ? "macro tree"
                   : "register tree",
               topBuffer->getName(),
               builder->getTopBufferDelay());
  }
}

void LatenciesBalancer::computeLeafsNumBufferToInsert(int nodeId) {
  GraphNode* node = &graph_[nodeId];
  if(node->childrenIds.empty()) {
    node->nBuffInsert = (int) ((worseDelay_ - node->delay) / root_->getTopBufferDelay());
    return;
  }
  for(int child : node->childrenIds) {
    computeLeafsNumBufferToInsert(child);
  }
}

bool LatenciesBalancer::propagateClock(odb::dbITerm* input)
{
  odb::dbInst* inst = input->getInst();
  sta::Cell* masterCell = network_->dbToSta(inst->getMaster());
  sta::LibertyCell* libertyCell = network_->libertyCell(masterCell);

  if (!libertyCell) {
    return false;
  }
  // Clock tree buffers
  if (libertyCell->isInverter() || libertyCell->isBuffer()) {
    return true;
  }
  // Combinational components
  if (!libertyCell->hasSequentials()) {
    return true;
  }
  sta::LibertyPort* inputPort
      = libertyCell->findLibertyPort(input->getMTerm()->getConstName());

  // Clock Gater / Latch improvised as clock gater
  if (inputPort) {
    return inputPort->isClockGateClock() || libertyCell->isLatchData(inputPort);
  }

  return false;
}

bool LatenciesBalancer::isSink(odb::dbITerm* iterm)
{
  odb::dbInst* inst = iterm->getInst();
  sta::Cell* masterCell = network_->dbToSta(inst->getMaster());
  sta::LibertyCell* libertyCell = network_->libertyCell(masterCell);
  if (!libertyCell) {
    return true;
  }

  if (inst->isBlock()) {
    return true;
  }

  sta::LibertyPort* inputPort
      = libertyCell->findLibertyPort(iterm->getMTerm()->getConstName());
  if (inputPort) {
    return inputPort->isRegClk();
  }

  return false;
}

}  // namespace cts
