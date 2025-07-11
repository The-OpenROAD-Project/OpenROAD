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
  worseDelay_ = std::numeric_limits<float>::min();
  delayBufIndex_ = 0;
  initSta();
  findAllBuilders(root_);
  expandBuilderGraph(root_->getTopInputNet());
  computeLeafsNumBufferToInsert(0);
  debugPrint(logger_, CTS, "insertion delay", 1, "inserted {} delay buffers.", delayBufIndex_);
}

void LatenciesBalancer::initSta() {
  openSta_->ensureGraph();
  openSta_->ensureClkNetwork();
  openSta_->updateTiming(false);
  timingGraph_ = openSta_->graph();
}

void LatenciesBalancer::findAllBuilders(TreeBuilder* builder)
{
  std::string topBufferName = builder->getTopBufferName();
  inst2builder_[topBufferName] = builder;
  for (const auto& child : builder->getChildren()) {
    findAllBuilders(child);
  }
}

void LatenciesBalancer::expandBuilderGraph(odb::dbNet* clkInputNet)
{
  std::string builderSrcName;
  if(!clkInputNet->getFirstOutput()) {
    builderSrcName = clkInputNet->getName();
  } else {
    builderSrcName = clkInputNet->getFirstOutput()->getInst()->getName();
  }
  int builderSrcId = graph_.size();
  GraphNode builderSrcNode = GraphNode(builderSrcId, builderSrcName, clkInputNet->getFirstOutput());
  graph_.push_back(builderSrcNode);

  std::stack<int> visitNode;
  visitNode.push(builderSrcId);

  while(!visitNode.empty()) {
    int driverId = visitNode.top();
    visitNode.pop();

    odb::dbNet* driverNet;
    if(graph_[driverId].inputTerm != nullptr) {
      odb::dbInst* driverInst = graph_[driverId].inputTerm->getInst();
      odb::dbITerm* firstOutput = driverInst->getFirstOutput();
      driverNet = firstOutput->getNet();
      if(!driverNet) {
        continue;
      }
    } else {
      driverNet = clkInputNet;
    }

    for(odb::dbITerm* sinkIterm : driverNet->getITerms()) {
      if(sinkIterm->getIoType() == odb::dbIoType::INPUT) {
        odb::dbInst* sinkInst = sinkIterm->getInst();
        int sinkId = graph_.size();
        std::string sinkName = sinkInst->getName();
        GraphNode sinkNode = GraphNode(sinkId, sinkName, sinkIterm);
        graph_.push_back(sinkNode);
        graph_[driverId].childrenIds.push_back(sinkId);

        if(inst2builder_.find(sinkName) != inst2builder_.end()) {
          auto builder = inst2builder_[sinkName];
          if(builder->isLeafTree()) {
            float builerAvgArrival = computeAveSinkArrivals(builder);
            worseDelay_ = std::max(worseDelay_, builerAvgArrival);
            graph_[sinkId].delay = builerAvgArrival;
            continue;
          }
        }
        visitNode.push(sinkId);
      }
    }
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

  std::string builderSrcName;

  if(!topBufInputNet->getFirstOutput()) {
    builderSrcName = topBufInputNet->getName();
  } else {
    builderSrcName = topBufInputNet->getFirstOutput()->getInst()->getName();
  }

  int builderSrcId = getNodeIdByName(builderSrcName);
  if(builderSrcId == -1) {
    builderSrcId = graph_.size();
    GraphNode builderSrcNode = GraphNode(builderSrcId, builderSrcName, topBufInputNet->getFirstOutput());
    graph_.push_back(builderSrcNode);
  }

  // Create the node for the root of the tree
  int topBufId = graph_.size();
  GraphNode topBufNode = GraphNode(topBufId, topBufferName, builderTopBufInputTerm);
  graph_.push_back(topBufNode);
  graph_[builderSrcId].childrenIds.push_back(topBufId);

  // If the builder is leaf don't expand it, as we don't want to insert delay
  // inside this tree
  if(builder->isLeafTree()) {
    float builerAvgArrival = computeAveSinkArrivals(builder);
    worseDelay_ = std::max(worseDelay_, builerAvgArrival);
    graph_[topBufId].delay = builerAvgArrival;
    return;
  }
  std::map<std::string, std::vector<int>> not_created_nodes;
  // Expand clock tree to possibly insert delay
  Clock& clockNet = builder->getClock();
  clockNet.forEachSubNet([&](ClockSubNet& subNet) {
    odb::dbInst* driver = subNet.getDriver()->getDbInst();
    int driverId = getNodeIdByName(driver->getName());

    subNet.forEachSink([&](ClockInst* inst) {
      int sinkId = graph_.size();
      std::string sinkName;
      odb::dbITerm* sinkIterm = inst->getDbInputPin();
      if(!inst->isClockBuffer()) {
        sinkName = sinkIterm->getInst()->getName();
      } else {
        sinkName = inst->getName();
      }
      GraphNode sinkNode = GraphNode(sinkId, sinkName, sinkIterm);
      if(driverId == -1) {
        not_created_nodes[driver->getName()].push_back(sinkId);
      } else {
        graph_[driverId].childrenIds.push_back(sinkId);
      }

      if(not_created_nodes.find(sinkName) !=  not_created_nodes.end()) {
        sinkNode.childrenIds = not_created_nodes[sinkName];
      }
      graph_.push_back(sinkNode);
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

float LatenciesBalancer::computeAveSinkArrivals(TreeBuilder* builder)
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

  return aveArrival;
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

void LatenciesBalancer::computeLeafsNumBufferToInsert(int nodeId) {
  GraphNode* node = &graph_[nodeId];
  // Compute number of buffer needed for leaf node
  if(node->childrenIds.empty()) {
    if(node->delay != 0.0) {
      debugPrint(logger_,
                 CTS,
                 "insertion delay",
                 1,
                 "For node {}, isert {:2f} buffers",
                 node->name,
                 (worseDelay_ - node->delay) / root_->getTopBufferDelay());
      node->nBuffInsert = (int) ((worseDelay_ - node->delay) / root_->getTopBufferDelay());
    }
    return;
  }

  // If it is not a leaf node compute the amount of buffers needed for its children
  std::map<int, std::vector<odb::dbITerm*>> buffersNeeded2Childern;
  for(int child : node->childrenIds) {
    computeLeafsNumBufferToInsert(child);
    buffersNeeded2Childern[graph_[child].nBuffInsert].push_back(graph_[child].inputTerm);
  }

  // If the children need a different amount of buffers insert this difference
  std::vector<odb::dbITerm*> sinksInput;
  int previous_buf_to_insert = 0;
  int srcX, srcY;
  if(node->inputTerm == nullptr) {
    odb::dbNet* rootNet = root_->getTopInputNet();
    odb::dbBTerm* clkInput = rootNet->get1stBTerm();
    odb::Rect clkInputBBox = clkInput->getBBox();
    srcX = clkInputBBox.xCenter();
    srcY = clkInputBBox.yCenter();
  } else {
    node->inputTerm->getAvgXY(&srcX, &srcY);
  }

  for (auto it = buffersNeeded2Childern.rbegin(); it != buffersNeeded2Childern.rend(); ++it) {
    int buf_to_insert = it->first;
    std::vector<odb::dbITerm*> childs = it->second;
    if(!previous_buf_to_insert) {
      previous_buf_to_insert = buf_to_insert;
      sinksInput.clear();
      sinksInput = childs;      
      continue;
    } else if(buf_to_insert == -1) {
      continue;
    }

    int numBuffers = previous_buf_to_insert - buf_to_insert;
    odb::dbITerm* delauBuffInput = insertDelayBuffers(numBuffers, srcX, srcY, sinksInput);

    sinksInput.clear();
    sinksInput = childs;
    sinksInput.push_back(delauBuffInput);

    previous_buf_to_insert = buf_to_insert;
  }

  node->nBuffInsert = previous_buf_to_insert;
}

odb::dbITerm* LatenciesBalancer::insertDelayBuffers(int numBuffers, int srcX, int srcY, std::vector<odb::dbITerm*> sinksInput)
{
  // get bbox of current load pins without driver output pin
  int maxSinkX = 0, maxSinkY = 0;
  int minSinkX = std::numeric_limits<int>::max(), minSinkY = std::numeric_limits<int>::max();
  odb::dbNet* drivingNet = nullptr;
  debugPrint(logger_,
               CTS,
               "insertion delay",
               1,
               "Inserting {} buffers for sinks:",
               numBuffers);
  for (odb::dbITerm* sinkInput : sinksInput) {
    if (drivingNet == nullptr) {
      drivingNet = sinkInput->getNet();
    }

    sinkInput->disconnect();
    int sinkX, sinkY;
    sinkInput->getAvgXY(&sinkX, &sinkY);
    maxSinkX = std::max(maxSinkX, sinkX);
    maxSinkY = std::max(maxSinkY, sinkY);
    minSinkX = std::min(minSinkX, sinkX);
    minSinkY = std::min(minSinkY, sinkY);
  }

  int destX = (maxSinkX + minSinkX) / 2;
  int destY = (maxSinkY + minSinkY) / 2;
  float offsetX = (float) (destX - srcX) / (numBuffers + 1);
  float offsetY = (float) (destY - srcY) / (numBuffers + 1);
  odb::dbInst* returnBuffer = nullptr;
  for (int i = 0; i < numBuffers; i++) {
    double locX = (double) (srcX + offsetX * (i + 1)) / wireSegmentUnit_;
    double locY = (double) (srcY + offsetY * (i + 1)) / wireSegmentUnit_;
    Point<double> bufferLoc(locX, locY);
    Point<double> legalBufferLoc
        = root_->legalizeOneBuffer(bufferLoc, options_->getRootBuffer());
    odb::dbInst* lastBuffer
        = createDelayBuffer(drivingNet,
                            root_->getClock().getSdcName(),
                            legalBufferLoc.getX() * wireSegmentUnit_,
                            legalBufferLoc.getY() * wireSegmentUnit_);

    drivingNet = lastBuffer->getFirstOutput()->getNet();
    if(returnBuffer == nullptr) {
      returnBuffer = lastBuffer;
    }
  }

  for (odb::dbITerm* sinkInput : sinksInput) {
    sinkInput->connect(drivingNet);
  }
  return getFirstInput(returnBuffer);
}

// Create a new delay buffer and connect output pin of driver to input pin of
// new buffer. Output pin of new buffer will be connected later.
odb::dbInst* LatenciesBalancer::createDelayBuffer(odb::dbNet* driverNet,
                                          const std::string& clockName,
                                          int locX,
                                          int locY)
{
  odb::dbBlock* block = db_->getChip()->getBlock();
  // creat a new input net
  std::string newNetName
      = "delaynet_" + std::to_string(delayBufIndex_) + "_" + clockName;
  odb::dbNet* newNet = odb::dbNet::create(block, newNetName.c_str());
  newNet->setSigType(odb::dbSigType::CLOCK);

  // create a new delay buffer
  std::string newBufName
      = "delaybuf_" + std::to_string(delayBufIndex_++) + "_" + clockName;
  odb::dbMaster* master = db_->findMaster(options_->getRootBuffer().c_str());
  odb::dbInst* newBuf = odb::dbInst::create(block, master, newBufName.c_str());

  newBuf->setSourceType(odb::dbSourceType::TIMING);
  newBuf->setLocation(locX, locY);
  newBuf->setPlacementStatus(odb::dbPlacementStatus::PLACED);

  // connect driver output with new buffer input
  odb::dbITerm* newBufOutTerm = newBuf->getFirstOutput();
  odb::dbITerm* newBufInTerm = getFirstInput(newBuf);
  newBufInTerm->connect(driverNet);
  newBufOutTerm->connect(newNet);

  debugPrint(logger_,
             CTS,
             "insertion delay",
             1,
             "new delay buffer {} is inserted at ({} {})",
             newBuf->getName(),
             locX,
             locY);

  return newBuf;
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
    return inputPort->isClockGateClock() || inputPort->isLatchData();
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

void LatenciesBalancer::showGraph() {
  logger_->report("Graph builded:");
  for(auto node : graph_) {
    odb::dbITerm* inputTerm = node.inputTerm;
    logger_->report(" Node {}", node.name);
    logger_->report("   id       = {}", node.id);
    logger_->report("   delay    = {}", node.delay);
    logger_->report("   n buffer = {}", node.nBuffInsert);
    logger_->report("   in Term  = {}", inputTerm == nullptr ? "no dbITerm" : inputTerm->getName());
    logger_->report("   childern [");
    for(int cId : node.childrenIds) {
      logger_->report("              {},", cId);
    }
    logger_->report("            ]");
  }
}

}  // namespace cts
