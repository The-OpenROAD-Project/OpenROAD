// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "LatencyBalancer.h"

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

int LatencyBalancer::run()
{
  logger_->info(CTS,
                33,
                "Balancing latency for clock {}",
                root_->getClock().getSdcName());
  worseDelay_ = std::numeric_limits<float>::min();
  delayBufIndex_ = 0;
  initSta();
  findLeafBuilders(root_);
  buildGraph(root_->getTopInputNet());
  bufferDelay_ = computeBufferDelay(0);
  balanceLatencies(0);
  logger_->info(CTS,
                36,
                " inserted {} delay buffers",
                delayBufIndex_,
                root_->getClock().getSdcName());
  return delayBufIndex_;
}

void LatencyBalancer::initSta()
{
  openSta_->ensureGraph();
  openSta_->ensureClkNetwork();
  openSta_->updateTiming(false);
  timingGraph_ = openSta_->graph();
}

sta::ArcDelay LatencyBalancer::computeBufferDelay(double extra_out_cap)
{
  odb::dbMaster* bufferMaster
      = db_->findMaster(options_->getRootBuffer().c_str());
  sta::Cell* bufferMasterCell = network_->dbToSta(bufferMaster);
  sta::LibertyCell* buffer_cell = network_->libertyCell(bufferMasterCell);
  sta::ArcDelay max_rise_delay = 0;

  sta::LibertyPort *input, *output;
  buffer_cell->bufferPorts(input, output);
  for (sta::Corner* corner : *openSta_->corners()) {
    const sta::DcalcAnalysisPt* dcalc_ap
        = corner->findDcalcAnalysisPt(sta::MinMax::max());
    const sta::Pvt* pvt = dcalc_ap->operatingConditions();

    for (sta::TimingArcSet* arc_set :
         buffer_cell->timingArcSets(input, output)) {
      for (sta::TimingArc* arc : arc_set->arcs()) {
        sta::GateTimingModel* model
            = dynamic_cast<sta::GateTimingModel*>(arc->model());
        const sta::RiseFall* in_rf = arc->fromEdge()->asRiseFall();
        const sta::RiseFall* out_rf = arc->toEdge()->asRiseFall();
        // Only look at rise-rise arcs
        if (model != nullptr && in_rf == sta::RiseFall::rise()
            && out_rf == sta::RiseFall::rise()) {
          double in_cap = input->capacitance(in_rf, sta::MinMax::max());
          double load_cap = in_cap + extra_out_cap;
          sta::ArcDelay arc_delay;
          sta::Slew arc_slew;
          model->gateDelay(pvt, 0.0, load_cap, false, arc_delay, arc_slew);
          // Cycle the arc_slew through the gate delay calculator once more
          model->gateDelay(pvt, arc_slew, load_cap, false, arc_delay, arc_slew);
          // and once more
          model->gateDelay(pvt, arc_slew, load_cap, false, arc_delay, arc_slew);

          if (arc_delay > max_rise_delay) {
            max_rise_delay = arc_delay;
          }
        }
      }
    }
  }

  return max_rise_delay;
}

void LatencyBalancer::findLeafBuilders(TreeBuilder* builder)
{
  if (builder->isLeafTree()) {
    std::string topBufferName = builder->getTopBufferName();
    inst2builder_[topBufferName] = builder;
  }
  for (const auto& child : builder->getChildren()) {
    findLeafBuilders(child);
  }
}

void LatencyBalancer::buildGraph(odb::dbNet* clkInputNet)
{
  std::string rootSrcName;
  odb::dbITerm* rootOutputITerm = clkInputNet->getFirstOutput();
  if (!rootOutputITerm) {
    rootSrcName = clkInputNet->getName();
  } else {
    rootSrcName = rootOutputITerm->getInst()->getName();
  }
  int builderSrcId = graph_.size();
  GraphNode builderSrcNode
      = GraphNode(builderSrcId, rootSrcName, rootOutputITerm);
  graph_.push_back(builderSrcNode);

  std::stack<int> visitNode;
  visitNode.push(builderSrcId);

  while (!visitNode.empty()) {
    int driverId = visitNode.top();
    visitNode.pop();

    odb::dbNet* driverNet;
    if (graph_[driverId].inputTerm != nullptr) {
      odb::dbInst* driverInst = graph_[driverId].inputTerm->getInst();
      odb::dbITerm* firstOutput = driverInst->getFirstOutput();
      driverNet = firstOutput->getNet();
      if (!driverNet) {
        continue;
      }
    } else {
      driverNet = clkInputNet;
    }

    for (odb::dbITerm* sinkIterm : driverNet->getITerms()) {
      if (sinkIterm->getIoType() == odb::dbIoType::INPUT) {
        int sinkId = graph_.size();
        odb::dbInst* sinkInst = sinkIterm->getInst();
        std::string sinkName = sinkInst->getName();
        GraphNode sinkNode = GraphNode(sinkId, sinkName, sinkIterm);
        graph_.push_back(sinkNode);
        graph_[driverId].childrenIds.push_back(sinkId);

        if (inst2builder_.find(sinkName) != inst2builder_.end()) {
          auto builder = inst2builder_[sinkName];
          float builerAvgArrival = computeAveSinkArrivals(builder);
          worseDelay_ = std::max(worseDelay_, builerAvgArrival);
          graph_[sinkId].arrival = builerAvgArrival;
          continue;
        }

        if (isSink(sinkIterm)) {
          sta::Pin* pin = network_->dbToSta(sinkIterm);
          if (pin) {
            sta::Vertex* sinkVertex = timingGraph_->pinDrvrVertex(pin);
            float arrival
                = getVertexClkArrival(sinkVertex, clkInputNet, sinkIterm);
            float insDelay = 0.0;
            sta::LibertyCell* libCell
                = network_->libertyCell(network_->dbToSta(sinkInst));
            odb::dbMTerm* mterm = sinkIterm->getMTerm();
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

            graph_[sinkId].arrival = arrival + insDelay;
            debugPrint(logger_,
                       CTS,
                       "insertion delay",
                       2,
                       "Sink {}: average sink arrival is {:0.3e}",
                       sinkIterm->getName(),
                       arrival);
          }
          continue;
        }
        visitNode.push(sinkId);
      }
    }
  }
}

odb::dbITerm* LatencyBalancer::getFirstInput(odb::dbInst* inst) const
{
  odb::dbSet<odb::dbITerm> iterms = inst->getITerms();
  for (odb::dbITerm* iterm : iterms) {
    if (iterm->isInputSignal()) {
      return iterm;
    }
  }

  return nullptr;
}

float LatencyBalancer::getVertexClkArrival(sta::Vertex* sinkVertex,
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

float LatencyBalancer::computeAveSinkArrivals(TreeBuilder* builder)
{
  Clock clock = builder->getClock();
  odb::dbNet* topInputClockNet = builder->getTopInputNet();
  // compute average input arrival at all sinks
  float sumArrivals = 0.0;
  unsigned numSinks = 0;
  clock.forEachSink([&](const ClockInst& sink) {
    odb::dbITerm* iterm = sink.getDbInputPin();
    computeSinkArrivalRecur(topInputClockNet, iterm, sumArrivals, numSinks);
  });
  float aveArrival = sumArrivals / (float) numSinks;
  builder->setAveSinkArrival(aveArrival);
  debugPrint(logger_,
             CTS,
             "insertion delay",
             2,
             "{} {}: average sink arrival is {:0.3e}",
             (builder->getTreeType() == TreeType::MacroTree) ? "macro tree"
                                                             : "register tree",
             clock.getName(),
             builder->getAveSinkArrival());

  return aveArrival;
}

void LatencyBalancer::computeSinkArrivalRecur(odb::dbNet* topClokcNet,
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

void LatencyBalancer::computeNumberOfDelayBuffers(int nodeId,
                                                  int srcX,
                                                  int srcY)
{
  GraphNode* node = &graph_[nodeId];
  if (node->arrival != 0.0) {
    int numBuffers = (int) ((worseDelay_ - node->arrival) / bufferDelay_);

    // adjust buffer delay for wire cap
    int sinkX, sinkY;
    graph_[nodeId].inputTerm->getAvgXY(&sinkX, &sinkY);
    float offsetX = (float) (sinkX - srcX) / (numBuffers + 1);
    float offsetY = (float) (sinkY - srcY) / (numBuffers + 1);
    auto newDelay = computeBufferDelay((std::abs(offsetX) + std::abs(offsetY))
                                       * capPerDBU_);
    numBuffers = (int) ((worseDelay_ - node->arrival) / newDelay);
    if (node->childrenIds.empty()) {
      debugPrint(logger_,
                 CTS,
                 "insertion delay",
                 3,
                 "For node {}, isert {:2f} buffers",
                 node->name,
                 numBuffers);
    }
    node->nBuffInsert = numBuffers;
  }
}

void LatencyBalancer::balanceLatencies(int nodeId)
{
  GraphNode* node = &graph_[nodeId];

  // Compute number of buffer needed for leaf node
  if (node->childrenIds.empty()) {
    return;
  }

  // If it is not a leaf node compute the amount of buffers needed for its
  // children
  std::vector<odb::dbITerm*> sinksInput;
  int previouBufToInsert = 0;
  int srcX, srcY;
  if (node->inputTerm == nullptr) {
    odb::dbNet* rootNet = root_->getTopInputNet();
    odb::dbBTerm* clkInput = rootNet->get1stBTerm();
    odb::Rect clkInputBBox = clkInput->getBBox();
    srcX = clkInputBBox.xCenter();
    srcY = clkInputBBox.yCenter();
  } else {
    node->inputTerm->getAvgXY(&srcX, &srcY);
  }

  double maxArrival = std::numeric_limits<double>::min();
  std::map<int, std::vector<odb::dbITerm*>> buffersNeeded2Childern;
  for (int child : node->childrenIds) {
    balanceLatencies(child);
    computeNumberOfDelayBuffers(child, srcX, srcY);
    maxArrival = std::max(graph_[child].arrival, maxArrival);
    buffersNeeded2Childern[graph_[child].nBuffInsert].push_back(
        graph_[child].inputTerm);
  }

  // If the children need a different amount of buffers insert this difference
  for (auto it = buffersNeeded2Childern.rbegin();
       it != buffersNeeded2Childern.rend();
       ++it) {
    int bufToInsert = it->first;
    std::vector<odb::dbITerm*> children = it->second;
    if (bufToInsert == -1) {
      continue;
    }

    if (!previouBufToInsert) {
      previouBufToInsert = bufToInsert;
      sinksInput.clear();
      sinksInput = children;
      continue;
    }

    int numBuffers = previouBufToInsert - bufToInsert;
    odb::dbITerm* delauBuffInput
        = insertDelayBuffers(numBuffers, srcX, srcY, sinksInput);

    sinksInput.clear();
    sinksInput = children;
    sinksInput.push_back(delauBuffInput);

    previouBufToInsert = bufToInsert;
  }

  node->nBuffInsert = previouBufToInsert;
  node->arrival = maxArrival;
}

odb::dbITerm* LatencyBalancer::insertDelayBuffers(
    int numBuffers,
    int srcX,
    int srcY,
    const std::vector<odb::dbITerm*>& sinksInput)
{
  // get bbox of current load pins without driver output pin
  odb::Rect loadPinsBbox = odb::Rect();
  loadPinsBbox.mergeInit();
  odb::dbNet* drivingNet = nullptr;
  debugPrint(logger_,
             CTS,
             "insertion delay",
             3,
             "Inserting {} buffers for sinks:",
             numBuffers);
  for (odb::dbITerm* sinkInput : sinksInput) {
    if (drivingNet == nullptr) {
      drivingNet = sinkInput->getNet();
    }
    debugPrint(
        logger_, CTS, "insertion delay", 3, "  {}", sinkInput->getName());
    sinkInput->disconnect();
    int sinkX, sinkY;
    sinkInput->getAvgXY(&sinkX, &sinkY);
    loadPinsBbox.merge({sinkX, sinkY});
  }

  float offsetX = (float) (loadPinsBbox.xCenter() - srcX) / (numBuffers + 1);
  float offsetY = (float) (loadPinsBbox.yCenter() - srcY) / (numBuffers + 1);

  odb::dbInst* returnBuffer = nullptr;
  odb::dbInst* lastBuffer = nullptr;
  for (int i = 0; i < numBuffers; i++) {
    double locX = (double) (srcX + offsetX * (i + 1)) / wireSegmentUnit_;
    double locY = (double) (srcY + offsetY * (i + 1)) / wireSegmentUnit_;
    Point<double> bufferLoc(locX, locY);
    Point<double> legalBufferLoc
        = root_->legalizeOneBuffer(bufferLoc, options_->getRootBuffer());
    lastBuffer
        = createDelayBuffer(drivingNet,
                            root_->getClock().getSdcName(),
                            legalBufferLoc.getX() * wireSegmentUnit_,
                            legalBufferLoc.getY() * wireSegmentUnit_);

    drivingNet = lastBuffer->getFirstOutput()->getNet();
    if (returnBuffer == nullptr) {
      returnBuffer = lastBuffer;
    }
  }

  for (odb::dbITerm* sinkInput : sinksInput) {
    sinkInput->connect(drivingNet);
    if (network_->hasHierarchy()) {
      network_->hierarchicalConnect(
          lastBuffer->getFirstOutput(), sinkInput, drivingNet->getName().c_str());
    }
  }
  return getFirstInput(returnBuffer);
}

// Create a new delay buffer and connect output pin of driver to input pin of
// new buffer. Output pin of new buffer will be connected later.
odb::dbInst* LatencyBalancer::createDelayBuffer(odb::dbNet* driverNet,
                                                const std::string& clockName,
                                                int locX,
                                                int locY)
{
  odb::dbBlock* block = db_->getChip()->getBlock();
  // creat a new input net
  std::string newNetName
      = "delaynet_" + std::to_string(delayBufIndex_) + "_" + clockName;

  // hierarchy fix, make the net in the right scope
  sta::Pin* driver = nullptr;
  odb::dbModule* module = network_->getNetDriverParentModule(
      network_->dbToSta(driverNet), driver);
  if (module == nullptr) {
    // if none put in top level
    module = block->getTopModule();
  }
  sta::Instance* scope
      = (module == nullptr || (module == block->getTopModule()))
            ? network_->topInstance()
            : (sta::Instance*) (module->getModInst());
  odb::dbNet* newNet = network_->staToDb(network_->makeNet(
      newNetName.c_str(), scope, odb::dbNameUniquifyType::IF_NEEDED));

  newNet->setSigType(odb::dbSigType::CLOCK);

  // create a new delay buffer
  std::string newBufName
      = "delaybuf_" + std::to_string(delayBufIndex_++) + "_" + clockName;
  odb::dbMaster* master = db_->findMaster(options_->getRootBuffer().c_str());

  // fix: make buffer in same hierarchical module as driver

  odb::dbInst* newBuf
      = odb::dbInst::create(block, master, newBufName.c_str(), false, module);

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

bool LatencyBalancer::propagateClock(odb::dbITerm* input)
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

bool LatencyBalancer::isSink(odb::dbITerm* iterm)
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

void LatencyBalancer::showGraph()
{
  logger_->report("Graph built:");
  for (const auto& node : graph_) {
    odb::dbITerm* inputTerm = node.inputTerm;
    logger_->report(" Node {}", node.name);
    logger_->report("   id       = {}", node.id);
    logger_->report("   delay    = {}", node.arrival);
    logger_->report("   n buffer = {}", node.nBuffInsert);
    logger_->report("   in Term  = {}",
                    inputTerm == nullptr ? "no dbITerm" : inputTerm->getName());
    logger_->report("   childern [");
    for (int cId : node.childrenIds) {
      logger_->report("              {},", cId);
    }
    logger_->report("            ]");
  }
}

}  // namespace cts
