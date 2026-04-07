// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "LatencyBalancer.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <map>
#include <ranges>
#include <set>
#include <stack>
#include <string>
#include <utility>
#include <vector>

#include "Clock.h"
#include "CtsOptions.h"
#include "TreeBuilder.h"
#include "Util.h"
#include "cts/TritonCTS.h"
#include "odb/db.h"
#include "odb/dbObject.h"
#include "odb/dbSet.h"
#include "odb/geom.h"
#include "sta/Clock.hh"
#include "sta/Delay.hh"
#include "sta/Graph.hh"
#include "sta/GraphDelayCalc.hh"
#include "sta/Liberty.hh"
#include "sta/Mode.hh"
#include "sta/NetworkClass.hh"
#include "sta/Path.hh"
#include "sta/PathEnd.hh"
#include "sta/PathExpanded.hh"
#include "sta/Sdc.hh"
#include "sta/TimingArc.hh"
#include "sta/TimingModel.hh"
#include "utl/Logger.h"

namespace cts {

using utl::CTS;

int LatencyBalancer::run()
{
  logger_->info(CTS,
                33,
                "Balancing latency for clock {}",
                root_->getClock().getSdcName());
  wireSegmentUnit_ = techChar_->getLengthUnit();
  initSta();
  findLeafBuilders(root_);
  buildGraph(root_->getTopInputNet());
  computeBuffersDelay(buffersDelay_, 0);
  balanceLatencies(0);
  logger_->info(CTS,
                36,
                " inserted {} delay buffers",
                delayBufIndex_,
                root_->getClock().getSdcName());
  showGraph();
  return delayBufIndex_;
}

void LatencyBalancer::initSta()
{
  openSta_->ensureGraph();
  for (auto mode : openSta_->modes()) {
    openSta_->ensureClkNetwork(mode);
  }
  openSta_->updateTiming(false);
  timingGraph_ = openSta_->graph();
}

void LatencyBalancer::computeBuffersDelay(std::vector<int>& buffersDelay,
                                          double extra_out_cap)
{
  std::vector<std::string> dlyBuffers = options_->getDlyBufferList();
  debugPrint(logger_, CTS, "insertion delay", 3, "Buffer list = [");
  for (const std::string& buffer : dlyBuffers) {
    int bufDelay = techChar_->computeBufferDelay(buffer, buffer, extra_out_cap)
                   * dpUnit_;
    debugPrint(logger_, CTS, "insertion delay", 3, "{} : {}", buffer, bufDelay);
    buffersDelay.push_back(
        techChar_->computeBufferDelay(buffer, buffer, extra_out_cap)
        * dpUnit_);
  }
  debugPrint(logger_, CTS, "insertion delay", 3, "]");
}

int64_t LatencyBalancer::computeWireLumpedDelay(const std::string& load, double wl, double& wireCap)
{
  wireCap = wl * capPerDBU_;
  double totalCap = wireCap;
  double wireRes = wl * resPerDBU_;


  if(load != "") {
    odb::dbMaster* loadMaster = db_->findMaster(load.c_str());
    sta::Cell* loadMasterCell = network_->dbToSta(loadMaster);
    sta::LibertyCell* libertyLoadCell
        = network_->libertyCell(loadMasterCell);
    sta::LibertyPort *input, *output;
    libertyLoadCell->bufferPorts(input, output);
    totalCap += input->capacitance(sta::RiseFall::rise(), sta::MinMax::max());
  }




  return wireRes * totalCap * dpUnit_;
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
      = GraphNode(builderSrcId, std::move(rootSrcName), rootOutputITerm);
  graph_.push_back(std::move(builderSrcNode));

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
        odb::dbInst* sinkInst = sinkIterm->getInst();
        odb::dbNet* sinkOutNet = sinkInst->getFirstOutput()->getNet();
        if ((!isSink(sinkIterm) && !propagateClock(sinkIterm)) || !sinkOutNet) {
          continue;
        }
        int sinkId = graph_.size();
        std::string sinkName = sinkInst->getName();
        GraphNode sinkNode = GraphNode(sinkId, sinkName, sinkIterm);
        graph_.push_back(std::move(sinkNode));
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
            worseDelay_ = std::max(worseDelay_, (arrival + insDelay));
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

    if (path->minMax(openSta_) != sta::MinMax::max()) {
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
  float aveArrival = 0.0;
  if (numSinks) {
    aveArrival = sumArrivals / (float) numSinks;
  }
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

DPResult LatencyBalancer::solveDP(
    int64_t                    target,
    int64_t                    wireDly,
    const std::vector<odb::dbITerm*>& sinks,
    const std::vector<std::string>&   dlyBuffers,
    double                     extraOutCap,
    double                     loadPinsHwpl)
{
  const size_t nBuffers = dlyBuffers.size();

  int64_t maxBufDelay = 0;
  // Buffers delay when driving sinks
  std::vector<int64_t> sinkDelay(nBuffers);
  for (size_t j = 0; j < nBuffers; j++) {
    int64_t delay= static_cast<int64_t>(
                       techChar_->computeBufferDelay(
                           dlyBuffers[j], sinks,
                           extraOutCap + loadPinsHwpl * capPerDBU_)
                       * dpUnit_)
                   + wireDly;
    sinkDelay[j] = delay;
    maxBufDelay = std::max(maxBufDelay, delay);
  }

  // Buffer delay when buffer i drives buffer j
  std::vector<std::vector<int64_t>> pairDelay(nBuffers, std::vector<int64_t>(nBuffers));
  for (size_t i = 0; i < nBuffers; i++) {
    for (size_t j = 0; j < nBuffers; j++) {
      int64_t delay = static_cast<int64_t>(
                            techChar_->computeBufferDelay(
                                dlyBuffers[i], dlyBuffers[j], extraOutCap)
                            * dpUnit_)
                        + wireDly;
      pairDelay[i][j] = delay;
      maxBufDelay = std::max(maxBufDelay, delay);
    }
  }

  const int64_t maxW = target + maxBufDelay;
  constexpr int64_t UNSET = -1;
  std::vector<std::vector<int64_t>> dp (maxW + 1, std::vector<int64_t>(nBuffers, UNSET));
  std::vector<std::vector<int>> nxt(maxW + 1, std::vector<int>    (nBuffers, -2));
  std::vector<std::vector<int>> length(maxW + 1, std::vector<int>    (nBuffers, 0));

  // Base case: single buffer driving the sinks
  for (size_t j = 0; j < nBuffers; j++) {
    int64_t bufDelay = sinkDelay[j];
    if (bufDelay <= maxW) {
      dp[bufDelay][j] = bufDelay;
      nxt[bufDelay][j] = -1;  // -1 → drives sinks
      length[bufDelay][j] = 1;
    }
  }

  // Extend the chain one buffer to the left at a time
  for (int64_t w = 0; w <= maxW; w++) {
    for (size_t j = 0; j < nBuffers; j++) {       // j = current leftmost buffer
      if (dp[w][j] == UNSET) {
        continue;
      }

      for (size_t i = 0; i < nBuffers; i++) {     // i = candidate new leftmost buffer
        int64_t bufDelay = pairDelay[i][j];   // exact: i's load is j, which is known
        int64_t newWeight = w + bufDelay;
        if (newWeight > maxW) {
          continue;
        }

        int64_t new_total = dp[w][j] + bufDelay;
        if (dp[newWeight][i] == UNSET
            || length[w][j] + 1 < length[newWeight][i]) {
          dp[newWeight][i] = new_total;
          nxt[newWeight][i] = static_cast<int>(j);
          length[newWeight][j] = length[w][j] + 1;
        }
      }
    }
  }

  // ── 3. Pick best solution ─────────────────────────────────────────────────

  int64_t bestW = 0;
  int     bestJ = -1;
  for (int64_t w = 0; w <= maxW; w++) {
    for (size_t j = 0; j < nBuffers; j++) {
      if (dp[w][j] == UNSET) {
        continue;
      }
      int64_t dist = std::abs(w - target);
      int64_t bestDist = (bestJ == -1) ? std::numeric_limits<int64_t>::max()
                                     : std::abs(bestW - target);

      if (dist < bestDist
        || (dist == bestDist && length[w][j] < length[bestW][bestJ])) {
        bestW = w;
        bestJ = static_cast<int>(j);
      }
    }
  }

  if (bestJ == -1) {
    return {};
  }  // no solution found

  // ── 4. Backtrack left → right along nxt[] pointers ───────────────────────
  //
  // At state (w, cur): cur is leftmost, nxt[w][cur] is what cur drives.
  // Step back: w -= pairDelay[cur][next], then cur = next.

  DPResult result;
  result.achievedDelay = dp[bestW][bestJ];

  int64_t w   = bestW;
  int cur = bestJ;
  while (cur != -1) {
    result.buffers.push_back(dlyBuffers[cur]);
    int next = nxt[w][cur];
    if (next == -1) { // cur drives sinks; we're done
      break;
    }
    w  -= pairDelay[cur][next];
    cur = next;
  }

  return result;
}

int LatencyBalancer::backtrackCount(const std::vector<int>& dp_elements, const std::vector<int>& bufDelays, int64_t target)
{
  int n = 0;
  int64_t w = target;
  while (w > 0 && dp_elements[w] != -1) {
    n++;
    w -= bufDelays[dp_elements[w]];
  }
  return n;
}

std::vector<std::string> LatencyBalancer::computeNumberOfDelayBuffers(
    double delayNeeded,
    int srcX,
    int srcY,
    const std::vector<odb::dbITerm*>& sinks)
{
  int target = delayNeeded * dpUnit_;
  debugPrint(logger_, CTS, "insertion delay", 2, "  target delay: {}", target);
  std::vector<std::string> dlyBuffers = options_->getDlyBufferList();

  // Compute initial best combinations of buffers to insert the target delay
  std::vector<int64_t> dp(target + 1, 0);
  std::vector<int> dp_elements(target + 1, -1);
  for (int64_t w = 0; w <= target; w++) {
    for (size_t i = 0; i < buffersDelay_.size(); i++) {
      const int bufDelay = buffersDelay_[i];
      int bestPrevWeight;
      if (bufDelay > w) {
        bestPrevWeight = 0;
      } else {
        bestPrevWeight = dp[w - bufDelay];
      }

      if (std::abs(dp[w] - w) >= std::abs(bestPrevWeight + bufDelay - w)) {
        dp_elements[w] = static_cast<int>(i);
        dp[w] = bestPrevWeight + bufDelay;
      }
    }
  }

  // No buffers to insert
  if (!dp[target]) {
    return {};
  }

  // Backtrack to find number of buffers that will be needed
  int nBufs = backtrackCount(dp_elements, buffersDelay_, target);
  debugPrint(
      logger_, CTS, "insertion delay", 4, "Initial best = {}", dp[target]);
  debugPrint(logger_, CTS, "insertion delay", 4, "Initial n bufs = {}", nBufs);

  // Compute wiredelay and adjust buffers delay for wire cap
  odb::Rect loadPinsBbox = odb::Rect();
  loadPinsBbox.mergeInit();
  for (odb::dbITerm* sinkInput : sinks) {
    int x, y;
    sinkInput->getAvgXY(&x, &y);
    loadPinsBbox.merge({x, y});
  }
  double loadPinsHwpl = (loadPinsBbox.dx()+loadPinsBbox.dy()) / 2.0;

  int prevNBufs = std::numeric_limits<int>::max();
  int pass = 2;
  DPResult dpResult;
  std::vector<int> adjustedBuffersDelay;
  while(nBufs < prevNBufs) {
    double offsetX = (double) (loadPinsBbox.xCenter() - srcX) / (double) (nBufs + 1);
    double offsetY = (double) (loadPinsBbox.yCenter() - srcY) / (double) (nBufs + 1);

    double extraOutCap = 0.0;
    int64_t wireDly = computeWireLumpedDelay(options_->getRootBuffer(), std::abs(offsetX) + std::abs(offsetY), extraOutCap);
    debugPrint(
        logger_, CTS, "insertion delay", 4, "Estimated wire delay: {}", wireDly);
    
    
    // Compute best buffer combination with more accurate values
    dpResult = solveDP(target, wireDly, sinks, dlyBuffers, extraOutCap, loadPinsHwpl);

    debugPrint(logger_, CTS, "insertion delay", 4,
               "Pass {} best = {}", pass, dpResult.achievedDelay);

    prevNBufs = nBufs;
    nBufs = static_cast<int>(dpResult.buffers.size());
    debugPrint(logger_, CTS, "insertion delay", 4, "Pass {} n bufs = {}", pass + 2, nBufs);
    if(!nBufs) {
      break;
    }
    pass++;
  }

  debugPrint(logger_,
             CTS,
             "insertion delay",
             2,
             "  Max achievable delay {}",
             dpResult.achievedDelay);

  std::stringstream tmp;
  tmp << "[";
  for(size_t i = 0; i < dpResult.buffers.size(); i++) {
    if (i == 0) {
      tmp <<dpResult.buffers[i];
    } else {
      tmp << ", " << dpResult.buffers[i];
    }
  }
  tmp << "]";


  debugPrint(
      logger_, CTS, "insertion delay", 2, "  using buffers {}", tmp.str());
  return dpResult.buffers;
}

void LatencyBalancer::balanceLatencies(int nodeId)
{
  GraphNode* node = &graph_[nodeId];

  // Compute number of buffer needed for leaf node
  if (node->childrenIds.empty()) {
    node->dlyNeeded = worseDelay_ - node->arrival;
    return;
  }

  // If it is not a leaf node compute the amount of buffers needed for its
  // children
  std::vector<odb::dbITerm*> sinksInput;
  double previouDlyNeeded = 0;
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

  std::map<double, std::vector<odb::dbITerm*>> delayNeeded2Childern;
  for (int child : node->childrenIds) {
    balanceLatencies(child);
    if (graph_[child].dlyNeeded == -1) {
      continue;
    }

    delayNeeded2Childern[graph_[child].dlyNeeded].push_back(
        graph_[child].inputTerm);
  }

  // If the children need a different amount of buffers insert this difference
  debugPrint(logger_, CTS, "insertion delay", 1, "at node {}", node->name);
  for (auto& [dlyNeeded, children] :
       std::ranges::reverse_view(delayNeeded2Childern)) {
    if (logger_->debugCheck(CTS, "insertion delay", 2)) {
      debugPrint(logger_, CTS, "insertion delay", 2, " sinks [");
      for (auto c : children) {
        debugPrint(logger_,
                   CTS,
                   "insertion delay",
                   2,
                   "{}, ",
                   c->getInst()->getName());
      }
      debugPrint(logger_, CTS, "insertion delay", 2, "]");
      ;
      debugPrint(
          logger_, CTS, "insertion delay", 2, " need {} delay", dlyNeeded);
    }
    if (!previouDlyNeeded) {
      previouDlyNeeded = dlyNeeded;
      sinksInput.clear();
      sinksInput = std::move(children);
      continue;
    }

    double dlyDiff = previouDlyNeeded - dlyNeeded;
    debugPrint(logger_,
               CTS,
               "insertion delay",
               3,
               " previous delay = {}",
               previouDlyNeeded);
    debugPrint(logger_,
               CTS,
               "insertion delay",
               3,
               " Has a {} dly diff with previous",
               dlyDiff);
    std::vector<std::string> buffersMaster
        = computeNumberOfDelayBuffers(dlyDiff, srcX, srcY, sinksInput);
    if (!buffersMaster.size()) {
      sinksInput.insert(sinksInput.end(), children.begin(), children.end());
      debugPrint(logger_,
                 CTS,
                 "insertion delay",
                 1,
                 " Not possible to insert buffers");
      continue;
    }
    debugPrint(logger_,
               CTS,
               "insertion delay",
               2,
               " dly buffers needed: {}",
               buffersMaster.size());
    odb::dbITerm* delauBuffInput
        = insertDelayBuffers(srcX, srcY, buffersMaster, sinksInput);

    sinksInput.clear();
    sinksInput = std::move(children);
    sinksInput.push_back(delauBuffInput);

    previouDlyNeeded = dlyNeeded;
  }

  node->dlyNeeded = previouDlyNeeded;
}

odb::dbITerm* LatencyBalancer::insertDelayBuffers(
    int srcX,
    int srcY,
    const std::vector<std::string>& buffersMaster,
    const std::vector<odb::dbITerm*>& sinksInput)
{
  int numBuffers = buffersMaster.size();
  debugPrint(logger_,
             CTS,
             "insertion delay",
             3,
             "Inserting {} buffers for sinks:",
             numBuffers);
  // get bbox of current load pins without driver output pin
  odb::dbNet* drivingNet = nullptr;
  odb::Rect loadPinsBbox = odb::Rect();
  loadPinsBbox.mergeInit();
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

  for (int i = 0; i < buffersMaster.size(); i++) {
    odb::dbMaster* bufMaster = db_->findMaster(buffersMaster[i].c_str());
    // Set the location
    double locX = (double) (srcX + (offsetX * (i + 1))) / wireSegmentUnit_;
    double locY = (double) (srcY + (offsetY * (i + 1))) / wireSegmentUnit_;
    Point<double> bufferLoc(locX, locY);
    Point<double> legalBufferLoc
        = root_->legalizeOneBuffer(bufferLoc, bufMaster->getName());

    odb::Point loc{static_cast<int>(legalBufferLoc.getX() * wireSegmentUnit_),
                   static_cast<int>(legalBufferLoc.getY() * wireSegmentUnit_)};

    // Insert buffer
    std::string clkName = root_->getClock().getSdcName();
    std::string newNetName
        = fmt::format("delaynet_{}_{}", delayBufIndex_, clkName);
    std::string newBufferName
        = fmt::format("delaybuf_{}_{}", delayBufIndex_++, clkName);

    odb::dbInst* lastBuffer = nullptr;

    // Use load pins buffering at the end
    std::set<odb::dbObject*> load_pins;
    for (odb::dbITerm* sinkInput : sinksInput) {
      load_pins.insert(sinkInput);
    }

    // load_pins are not connected yet. So this option is required.
    bool loads_on_different_nets = true;
    lastBuffer = drivingNet->insertBufferBeforeLoads(
        load_pins,
        bufMaster,
        &loc,
        newBufferName.c_str(),
        newNetName.c_str(),
        odb::dbNameUniquifyType::IF_NEEDED,
        loads_on_different_nets);

    debugPrint(logger_,
               CTS,
               "insertion delay",
               1,
               "new delay buffer {} inserted at ({} {})",
               lastBuffer->getName(),
               loc.getX(),
               loc.getY());

    // Update the driving iterm & net to insert a next buffer on it
    odb::dbITerm* drvrPin = lastBuffer->getFirstOutput();
    drivingNet = drvrPin->getNet();

    // Update return buffer (the first buffer inserted)
    if (returnBuffer == nullptr) {
      returnBuffer = lastBuffer;
    }
  }

  return getFirstInput(returnBuffer);
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
