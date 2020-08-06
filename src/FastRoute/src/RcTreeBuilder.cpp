/////////////////////////////////////////////////////////////////////////////
//
// BSD 3-Clause License
//
// Copyright (c) 2019, University of California, San Diego.
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
//
///////////////////////////////////////////////////////////////////////////////

#include "RcTreeBuilder.h"

#include "db_sta/dbNetwork.hh"
#include "db_sta/dbSta.hh"
#include "openroad/OpenRoad.hh"
#include "sta/Corner.hh"
#include "sta/Parasitics.hh"
#include "sta/ParasiticsClass.hh"
#include "sta/Sdc.hh"
#include "sta/Units.hh"

namespace FastRoute {

RcTreeBuilder::RcTreeBuilder(ord::OpenRoad* openroad, DBWrapper* dbWrapper)
{
  _dbWrapper = dbWrapper;
  sta::dbSta* dbSta = openroad->getSta();
  // Init analysis point
  _corner = dbSta->cmdCorner();
  sta::MinMax* minMax = sta::MinMax::max();
  _analysisPoint = _corner->findParasiticAnalysisPt(minMax);

  sta::Sdc* sdc = dbSta->sdc();
  _op_cond = sdc->operatingConditions(minMax);

  // Init parasitics
  _parasitics = dbSta->parasitics();

  // Init network
  _network = openroad->getDbNetwork();

  // Init units
  _units = dbSta->units();
}

void RcTreeBuilder::run(Net* net, SteinerTree* steinerTree, Grid* grid)
{
  _net = net;
  _steinerTree = steinerTree;
  _grid = grid;
  makeParasiticNetwork();
  computeGlobalParasitics();
  computeLocalParasitics();
  if (_debug)
    reportParasitics();
  reduceParasiticNetwork();
}

void RcTreeBuilder::makeParasiticNetwork()
{
  // This will definitely fail if the net name has escapes.
  // This SHOULD look more like this. No names! -cherry
  // _staNet = _network->dbToSta(_net);
  _staNet = _network->findNet(_net->getName().c_str());
  _parasitic
      = _parasitics->makeParasiticNetwork(_staNet, false, _analysisPoint);
}

void RcTreeBuilder::reduceParasiticNetwork()
{
  sta::ReduceParasiticsTo reduce_to = sta::ReduceParasiticsTo::pi_elmore;
  _parasitics->reduceTo(_parasitic,
                        _staNet,
                        reduce_to,
                        _op_cond,
                        _corner,
                        sta::MinMax::max(),
                        _analysisPoint);
  _parasitics->deleteParasiticNetwork(_staNet, _analysisPoint);
}

void RcTreeBuilder::createSteinerNodes()
{
  unsigned numSteinerPoints = _steinerTree->getNodes().size();
  for (unsigned snode = 0; snode < numSteinerPoints; ++snode) {
    _parasitics->ensureParasiticNode(_parasitic, _staNet, snode);
  }
}

void RcTreeBuilder::computeGlobalParasitics()
{
  const std::vector<Node>& nodes = _steinerTree->getNodes();

  if (_debug) {
    std::cout << "[DEBUG] Reporting global segments of net " << _net->getName()
              << "\n";
  }

  for (const Segment& segment : _steinerTree->getSegments()) {
    const Node& node1 = segment.getFirstNode();
    const Node& node2 = segment.getLastNode();
    unsigned idx1 = std::distance(nodes.begin(),
                                  std::find(nodes.begin(), nodes.end(), node1));
    unsigned idx2 = std::distance(nodes.begin(),
                                  std::find(nodes.begin(), nodes.end(), node2));
    sta::ParasiticNode* n1
        = _parasitics->ensureParasiticNode(_parasitic, _staNet, idx1);
    sta::ParasiticNode* n2
        = _parasitics->ensureParasiticNode(_parasitic, _staNet, idx2);

    float cap, res;
    if (node1.getPosition() == node2.getPosition()) {  // Via
      int lower_layer = std::min(node1.getLayer(), node2.getLayer());
      _dbWrapper->getCutLayerRes(lower_layer, res);
      cap = 0.0;
    } else {  // Wire
      // First compute wirelength
      unsigned dbuWirelength = computeDist(node1, node2);
      float wirelength = _dbWrapper->dbuToMeters(dbuWirelength);

      // Then get layer r/c-unit from the DB
      unsigned layerId = node1.getLayer();
      float rUnit, cUnit;
      _dbWrapper->getLayerRC(layerId, rUnit, cUnit);

      // Finally multiply
      res = rUnit * wirelength;
      cap = cUnit * wirelength;

      if (_debug) {
        std::cout << "[DEBUG] Layer ID: " << layerId
                  << " dbuWirelength: " << dbuWirelength;
        std::cout << std::scientific;
        std::cout << " rUnit: " << rUnit << " cUnit: " << cUnit << "\n";
        std::cout << "[DEBUG]    wirelength: " << wirelength << " res: " << res
                  << " cap: " << cap << "\n";
        std::cout << std::fixed;
      }
    }

    _parasitics->incrCap(n1, cap / 2.0, _analysisPoint);
    _parasitics->makeResistor(nullptr, n1, n2, res, _analysisPoint);
    _parasitics->incrCap(n2, cap / 2.0, _analysisPoint);
  }
}

unsigned RcTreeBuilder::computeDist(const Node& n1, const Node& n2) const
{
  return std::abs(n1.getPosition().getX() - n2.getPosition().getX())
         + std::abs(n1.getPosition().getY() - n2.getPosition().getY());
}

unsigned RcTreeBuilder::computeDist(const odb::Point& pt, const Node& n) const
{
  return std::abs(pt.getX() - n.getPosition().getX())
         + std::abs(pt.getY() - n.getPosition().getY());
}

void RcTreeBuilder::computeLocalParasitics()
{
  std::vector<Node> nodes = _steinerTree->getNodes();
  std::vector<unsigned> pinNodes;
  for (unsigned nodeId = 0; nodeId < nodes.size(); ++nodeId) {
    const Node& node = nodes[nodeId];
    if (node.getType() == NodeType::SINK
        || node.getType() == NodeType::SOURCE) {
      pinNodes.push_back(nodeId);
    }
  }

  if (_debug) {
    std::cout << "[DEBUG] Reporting local segments of net " << _net->getName()
              << "\n";
  }

  const std::vector<Pin>& pins = _net->getPins();
  for (const Pin& pin : pins) {
    // Sta pin
    odb::dbITerm* iterm = pin.getITerm();
    odb::dbBTerm* bterm = pin.getBTerm();
    sta::Pin* staPin
        = iterm ? _network->dbToSta(iterm) : _network->dbToSta(bterm);
    sta::ParasiticNode* n1
        = _parasitics->ensureParasiticNode(_parasitic, staPin);

    int nodeToConnect = findNodeToConnect(pin, pinNodes);
    sta::ParasiticNode* n2
        = _parasitics->ensureParasiticNode(_parasitic, _staNet, nodeToConnect);

    odb::Point pin_loc = _network->location(staPin);

    unsigned dbuWirelength = computeDist(pin_loc, nodes[nodeToConnect]);
    float wirelength = _dbWrapper->dbuToMeters(dbuWirelength);

    // Then get layer r/c-unit from the DB
    unsigned layerId = pin.getTopLayer();
    float rUnit = 0.0, cUnit = 0.0;
    _dbWrapper->getLayerRC(layerId, rUnit, cUnit);

    // Finally multiply
    float res = rUnit * wirelength;
    float cap = cUnit * wirelength;

    if (_debug) {
      std::cout << "[DEBUG] Layer ID: " << layerId
                << " dbuWirelength: " << dbuWirelength;
      std::cout << std::scientific;
      std::cout << " rUnit: " << rUnit << " cUnit: " << cUnit << "\n";
      std::cout << "[DEBUG]    wirelength: " << wirelength << " res: " << res
                << " cap: " << cap << "\n";
      std::cout << std::fixed;
    }

    _parasitics->incrCap(n1, cap / 2.0, _analysisPoint);
    _parasitics->makeResistor(nullptr, n1, n2, res, _analysisPoint);
    _parasitics->incrCap(n2, cap / 2.0, _analysisPoint);
  }
}

int RcTreeBuilder::findNodeToConnect(
    const Pin& pin,
    const std::vector<unsigned>& pinNodes) const
{
  std::vector<Node> nodes = _steinerTree->getNodes();
  const std::vector<Pin>& pins = _net->getPins();
  unsigned topLayer = pin.getTopLayer();
  std::vector<Box> pinBoxes = pin.getBoxes().at(topLayer);
  for (Box pinBox : pinBoxes) {
    Coordinate posOnGrid = _grid->getPositionOnGrid(pinBox.getMiddle());
    for (unsigned pinNode : pinNodes) {
      const Node& node = nodes[pinNode];
      if (posOnGrid == node.getPosition() && node.getLayer() == topLayer) {
        return pinNode;
      }
    }
  }

  return -1;
}

void RcTreeBuilder::reportParasitics()
{
  std::cout << "Net: " << _net->getName() << "\n";
  std::cout << "Num pins:  " << _net->getNumPins() << "\n";

  std::cout << "Nodes: \n";
  const std::vector<Node>& nodes = _steinerTree->getNodes();
  for (unsigned nodeId = 0; nodeId < nodes.size(); ++nodeId) {
    const Node& node = nodes[nodeId];
    std::cout << nodeId << " (" << node.getPosition().getX() << ", "
              << node.getPosition().getY() << ", " << node.getLayer() << ") ";
    if (node.getType() == NodeType::SINK) {
      std::cout << "sink\n";
    } else if (node.getType() == NodeType::SOURCE) {
      std::cout << "source\n";
    } else {
      std::cout << "steiner\n";
    }
  }

  std::cout << "Segments: \n";
  for (const Segment& segment : _steinerTree->getSegments()) {
    const Node& node1 = segment.getFirstNode();
    const Node& node2 = segment.getLastNode();
    unsigned idx1 = std::distance(nodes.begin(),
                                  std::find(nodes.begin(), nodes.end(), node1));
    unsigned idx2 = std::distance(nodes.begin(),
                                  std::find(nodes.begin(), nodes.end(), node2));
    std::cout << idx1 << " -> " << idx2 << "\n";
  }

  std::cout << std::scientific;
  std::cout << "Resistors: \n";
  sta::ParasiticDeviceIterator* deviceIt
      = _parasitics->deviceIterator(_parasitic);
  while (deviceIt->hasNext()) {
    sta::ParasiticDevice* device = deviceIt->next();
    if (!_parasitics->isResistor(device)) {
      continue;
    }

    sta::ParasiticNode* node1 = _parasitics->node1(device);
    sta::ParasiticNode* node2 = _parasitics->node2(device);
    std::cout << _parasitics->name(node1) << " " << _parasitics->name(node2)
              << " ";
    std::cout << _units->resistanceUnit()->asString(
        _parasitics->value(device, _analysisPoint))
              << "\n";
  }

  std::cout << "Capacitors: \n";
  sta::ParasiticNodeIterator* nodeIt = _parasitics->nodeIterator(_parasitic);
  while (nodeIt->hasNext()) {
    sta::ParasiticNode* node = nodeIt->next();

    std::cout << _parasitics->name(node) << " ";
    std::cout << _units->capacitanceUnit()->asString(
        _parasitics->nodeGndCap(node, _analysisPoint))
              << "\n";
  }

  std::cout << std::fixed;
}

}  // namespace FastRoute
