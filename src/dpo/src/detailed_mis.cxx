////////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// Copyright (c) 2021, Andrew Kennings
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

////////////////////////////////////////////////////////////////////////////////
// Description:
// An implementation of maximum independent set matching to reduce either
// wirelength of displacement.
//
// The idea is to color the nodes (for wirelength).  Then, one can solve
// a matching problem to optimize for wirelength.  Nodes with the same
// color do not share nets so they can be repositioned without worsening
// wirelength.  Note that for displacement optimization, colors are not
// needed.
//
// There are likely many improvements which can be made to this code
// regarding the selection of nodes, etc.

#include "detailed_mis.h"

#include <lemon/cost_scaling.h>
#include <lemon/cycle_canceling.h>
#include <lemon/list_graph.h>
#include <lemon/network_simplex.h>
#include <lemon/preflow.h>
#include <lemon/smart_graph.h>

#include <boost/tokenizer.hpp>
#include <deque>
#include <vector>

#include "architecture.h"
#include "color.h"
#include "detailed_manager.h"
#include "detailed_segment.h"
#include "network.h"
#include "rectangle.h"
#include "router.h"
#include "utl/Logger.h"

using utl::DPO;

namespace dpo {

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
struct DetailedMis::Bucket
{
  void clear() { nodes_.clear(); }

  std::deque<Node*> nodes_;
  double xmin_ = 0.0;
  double xmax_ = 0.0;
  double ymin_ = 0.0;
  double ymax_ = 0.0;
  int i_ = 0;
  int j_ = 0;
  int travId_ = 0;
};

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
DetailedMis::DetailedMis(Architecture* arch,
                         Network* network,
                         RoutingParams* rt)
    : mgrPtr_(nullptr),
      arch_(arch),
      network_(network),
      rt_(rt),
      dimW_(0),
      dimH_(0),
      stepX_(0.0),
      stepY_(0.0),
      skipEdgesLargerThanThis_(100),
      maxProblemSize_(25),
      traversal_(0),
      useSameSize_(true),
      useSameColor_(true),
      maxTimesUsed_(2),
      obj_(DetailedMis::Hpwl)
{
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
DetailedMis::~DetailedMis()
{
  clearGrid();
}

//////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////
void DetailedMis::run(DetailedMgr* mgrPtr, const std::string& command)
{
  // A temporary interface to allow for a string which we will decode to create
  // the arguments.
  boost::char_separator<char> separators(" \r\t\n;");
  boost::tokenizer<boost::char_separator<char>> tokens(command, separators);
  std::vector<std::string> args;
  for (boost::tokenizer<boost::char_separator<char>>::iterator it
       = tokens.begin();
       it != tokens.end();
       it++) {
    args.push_back(*it);
  }
  run(mgrPtr, args);
}

//////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////
void DetailedMis::run(DetailedMgr* mgrPtr, std::vector<std::string>& args)
{
  // Given the arguments, figure out which routine to run to do the reordering.

  mgrPtr_ = mgrPtr;

  // Defaults.
  obj_ = DetailedMis::Hpwl;

  int passes = 1;
  double tol = 0.01;
  for (size_t i = 1; i < args.size(); i++) {
    if (args[i] == "-p" && i + 1 < args.size()) {
      passes = std::atoi(args[++i].c_str());
    } else if (args[i] == "-t" && i + 1 < args.size()) {
      tol = std::atof(args[++i].c_str());
    } else if (args[i] == "-d") {
      obj_ = DetailedMis::Disp;
    }
  }
  tol = std::max(tol, 0.01);
  passes = std::max(passes, 1);

  const char* obj_str
      = (obj_ == DetailedMis::Hpwl) ? "wirelength" : "displacement";
  mgrPtr_->getLogger()->info(
      DPO, 300, "Set matching objective is {:s}.", obj_str);

  // If using displacement objective, then it isn't required to use colors.
  if (obj_ == DetailedMis::Disp) {
    useSameColor_ = false;
  }

  mgrPtr_->resortSegments();
  double hpwl_x, hpwl_y;
  double curr_hpwl = Utility::hpwl(network_, hpwl_x, hpwl_y);
  const double init_hpwl = curr_hpwl;

  double tot_disp, max_disp, avg_disp;
  double curr_disp = Utility::disp_l1(network_, tot_disp, max_disp, avg_disp);
  const double init_disp = curr_disp;

  // Do some things that only need to be done once regardless
  // of the number of passes.
  collectMovableCells();  // Movable cells.
  if (candidates_.empty()) {
    mgrPtr_->getLogger()->info(DPO, 202, "No movable cells found");
    return;
  }
  colorCells();  // Color the cells.
  buildGrid();   // Grid for searching for neigbours.

  for (int p = 1; p <= passes; p++) {
    const double curr_obj = (obj_ == DetailedMis::Hpwl) ? curr_hpwl : curr_disp;

    mgrPtr_->getLogger()->info(
        DPO, 301, "Pass {:3d} of matching; objective is {:.6e}.", p, curr_obj);

    // Run the algo here...
    place();

    const double last_hpwl = curr_hpwl;
    curr_hpwl = Utility::hpwl(network_, hpwl_x, hpwl_y);
    if (obj_ == DetailedMis::Hpwl
        && std::fabs(curr_hpwl - last_hpwl) / last_hpwl <= tol) {
      break;
    }
    const double last_disp = curr_disp;
    curr_disp = Utility::disp_l1(network_, tot_disp, max_disp, avg_disp);
    if (obj_ == DetailedMis::Disp
        && std::fabs(curr_disp - last_disp) / last_disp <= tol) {
      break;
    }
  }
  mgrPtr_->resortSegments();

  double curr_imp;
  double curr_obj;
  if (obj_ == DetailedMis::Hpwl) {
    const double hpwl_imp = (((init_hpwl - curr_hpwl) / init_hpwl) * 100.);
    curr_imp = hpwl_imp;
    curr_obj = curr_hpwl;
  } else {
    const double disp_imp = (((init_disp - curr_disp) / init_disp) * 100.);
    curr_imp = disp_imp;
    curr_obj = curr_disp;
  }
  mgrPtr_->getLogger()->info(
      DPO,
      302,
      "End of matching; objective is {:.6e}, improvement is {:.2f} percent.",
      curr_obj,
      curr_imp);
}

//////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////
void DetailedMis::place()
{
  // Populate the grid.  Used for searching.
  populateGrid();

  timesUsed_.resize(network_->getNumNodes());
  std::fill(timesUsed_.begin(), timesUsed_.end(), 0);

  // Select candidates and solve matching problem.  Note that we need to do
  // something to make this more efficient, otherwise we will solve way too
  // many problems for larger circuits.  I think one effective idea is to
  // keep track of how many problems a candidate cell has been involved in;
  // if it has been involved is >= a certain number of problems, it has "had
  // some chance" to be moved, so skip it.
  Utility::random_shuffle(
      candidates_.begin(), candidates_.end(), mgrPtr_->getRng());
  for (Node* ndi : candidates_) {  // Pick a candidate as a seed.
    // Skip seed if it has been used already.
    if (timesUsed_[ndi->getId()] >= maxTimesUsed_) {
      continue;
    }

    // Get other cells within the vicinity.
    if (!gatherNeighbours(ndi)) {
      continue;
    }

    // Solve the flow.
    solveMatch();

    // Increment times each node has been used.
    for (const Node* ndj : neighbours_) {
      ++timesUsed_[ndj->getId()];
    }

    // Update grid?  Or, do we need to even bother?
    ;
  }
}

//////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////
void DetailedMis::collectMovableCells()
{
  candidates_.clear();
  candidates_.insert(candidates_.end(),
                     mgrPtr_->getSingleHeightCells().begin(),
                     mgrPtr_->getSingleHeightCells().end());
  for (size_t i = 2; i < mgrPtr_->getNumMultiHeights(); i++) {
    candidates_.insert(candidates_.end(),
                       mgrPtr_->getMultiHeightCells(i).begin(),
                       mgrPtr_->getMultiHeightCells(i).end());
  }
}

//////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////
void DetailedMis::colorCells()
{
  colors_.resize(network_->getNumNodes());
  std::fill(colors_.begin(), colors_.end(), -1);

  movable_.resize(network_->getNumNodes());
  std::fill(movable_.begin(), movable_.end(), false);
  for (const Node* ndi : candidates_) {
    movable_[ndi->getId()] = true;
  }

  Graph gr(network_->getNumNodes());
  for (int e = 0; e < network_->getNumEdges(); e++) {
    const Edge* edi = network_->getEdge(e);

    const int numPins = edi->getNumPins();
    if (numPins <= 1 || numPins > skipEdgesLargerThanThis_) {
      continue;
    }

    for (int pi = 0; pi < edi->getNumPins(); pi++) {
      const Pin* pini = edi->getPins()[pi];
      const Node* ndi = pini->getNode();
      if (!movable_[ndi->getId()]) {
        continue;
      }

      for (int pj = pi + 1; pj < edi->getNumPins(); pj++) {
        const Pin* pinj = edi->getPins()[pj];
        const Node* ndj = pinj->getNode();
        if (!movable_[ndj->getId()]) {
          continue;
        }
        if (ndj == ndi) {
          continue;
        }

        gr.addEdge(ndi->getId(), ndj->getId());
      }
    }
  }

  // The actual coloring.
  gr.removeDuplicates();
  gr.greedyColoring();

  std::vector<int> hist;
  for (int i = 0; i < network_->getNumNodes(); i++) {
    const Node* ndi = network_->getNode(i);

    const int color = gr.getColor(i);
    if (color < 0 || color >= gr.getNColors()) {
      mgrPtr_->internalError("Unable to color cells during matching");
    }
    if (movable_[ndi->getId()]) {
      colors_[ndi->getId()] = color;

      if (color >= hist.size()) {
        hist.resize(color + 1, 0);
      }
      ++hist[color];
    }
  }
}

//////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////
void DetailedMis::buildGrid()
{
  // Builds a coarse grid over the placement region for locating cells.
  traversal_ = 0;

  const double xmin = arch_->getMinX();
  const double xmax = arch_->getMaxX();
  const double ymin = arch_->getMinY();
  const double ymax = arch_->getMaxY();

  // Design each grid bin to hold a few hundred cells.  Do this based on the
  // average width and height of the cells.
  double avgH = 0.;
  double avgW = 0.;
  for (const Node* ndi : candidates_) {
    avgH += ndi->getHeight();
    avgW += ndi->getWidth();
  }
  avgH /= (double) candidates_.size();
  avgW /= (double) candidates_.size();

  stepX_ = avgW * std::sqrt(maxProblemSize_);
  stepY_ = avgH * std::sqrt(maxProblemSize_);

  dimW_ = (int) std::ceil((xmax - xmin) / stepX_);
  dimH_ = (int) std::ceil((ymax - ymin) / stepY_);

  clearGrid();
  grid_.resize(dimW_);
  for (int i = 0; i < dimW_; i++) {
    grid_[i].resize(dimH_);
    for (int j = 0; j < dimH_; j++) {
      auto bucket = new Bucket;
      bucket->xmin_ = xmin + (i) *stepX_;
      bucket->xmax_ = xmin + (i + 1) * stepX_;
      bucket->ymin_ = ymin + (j) *stepY_;
      bucket->ymax_ = ymin + (j + 1) * stepY_;
      bucket->i_ = i;
      bucket->j_ = j;
      bucket->travId_ = traversal_;
      grid_[i][j] = bucket;
    }
  }
}

//////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////
void DetailedMis::populateGrid()
{
  // Inserts movable cells into the grid.

  for (size_t i = 0; i < grid_.size(); i++) {
    for (size_t j = 0; j < grid_[i].size(); j++) {
      grid_[i][j]->clear();
    }
  }

  const double xmin = arch_->getMinX();
  const double ymin = arch_->getMinY();

  // Insert cells into the constructed grid.
  cellToBinMap_.clear();
  for (Node* ndi : candidates_) {
    const double y = ndi->getBottom() + 0.5 * ndi->getHeight();
    const double x = ndi->getLeft() + 0.5 * ndi->getWidth();

    const int j = std::max(std::min((int) ((y - ymin) / stepY_), dimH_ - 1), 0);
    const int i = std::max(std::min((int) ((x - xmin) / stepX_), dimW_ - 1), 0);

    grid_[i][j]->nodes_.push_back(ndi);
    cellToBinMap_[ndi] = grid_[i][j];
  }
}

//////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////
void DetailedMis::clearGrid()
// Clear out any old grid.  The dimensions of the grid are also stored in the
// class...
{
  for (size_t i = 0; i < grid_.size(); i++) {
    for (size_t j = 0; j < grid_[i].size(); j++) {
      delete grid_[i][j];
    }
    grid_[i].clear();
  }
  grid_.clear();
}

//////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////
bool DetailedMis::gatherNeighbours(Node* ndi)
{
  const double singleRowHeight = arch_->getRow(0)->getHeight();

  neighbours_.clear();
  neighbours_.push_back(ndi);

  // Scan the grid structure gathering up cells which are compatible with the
  // current cell.

  auto it = cellToBinMap_.find(ndi);
  if (it == cellToBinMap_.end()) {
    return false;
  }

  const int spanned_i = std::lround(ndi->getHeight() / singleRowHeight);

  std::deque<Bucket*> Q;
  Q.push_back(it->second);
  ++traversal_;
  while (!Q.empty()) {
    Bucket* currPtr = Q.front();
    Q.pop_front();

    if (currPtr->travId_ == traversal_) {
      continue;
    }
    currPtr->travId_ = traversal_;

    // Scan all the cells in this bucket.  If they are compatible with the
    // original cell, then add them to the neighbour list.
    for (Node* ndj : currPtr->nodes_) {
      const int spanned_j = std::lround(ndj->getHeight() / singleRowHeight);

      // Check to make sure the cell is not the original, that they have
      // the same region, that they have the same size (if applicable),
      // and that they have the same color (if applicable).
      bool compat = ndj != ndi;  // diff nodes
      if (compat) {
        // Must be the same color to avoid sharing nets.
        compat
            = !useSameColor_ || colors_[ndi->getId()] == colors_[ndj->getId()];
      }
      if (compat) {
        // Must be the same size.
        compat = !useSameSize_
                 || (ndi->getWidth() == ndj->getWidth()
                     && ndi->getHeight() == ndj->getHeight());
      }
      if (compat) {
        // Must span the same number of rows and also be voltage compatible.
        compat = spanned_i == spanned_j
                 && ndi->getBottomPower() == ndj->getBottomPower()
                 && ndi->getTopPower() == ndj->getTopPower();
      }
      if (compat) {
        // Must be in the same region.
        compat = ndj->getRegionId() == ndi->getRegionId();
      }

      // If compatible, include this current cell.
      if (compat) {
        neighbours_.push_back(ndj);
      }
    }

    if (neighbours_.size() >= maxProblemSize_) {
      break;
    }

    // Add more bins to the queue if we have not yet collected enough cells.
    if (currPtr->i_ - 1 >= 0) {
      Q.push_back(grid_[currPtr->i_ - 1][currPtr->j_]);
    }
    if (currPtr->i_ + 1 <= dimW_ - 1) {
      Q.push_back(grid_[currPtr->i_ + 1][currPtr->j_]);
    }
    if (currPtr->j_ - 1 >= 0) {
      Q.push_back(grid_[currPtr->i_][currPtr->j_ - 1]);
    }
    if (currPtr->j_ + 1 <= dimH_ - 1) {
      Q.push_back(grid_[currPtr->i_][currPtr->j_ + 1]);
    }
  }
  return true;
}

//////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////
void DetailedMis::solveMatch()
{
  if (neighbours_.size() <= 1) {
    return;
  }
  const std::vector<Node*>& nodes = neighbours_;

  const int nNodes = (int) nodes.size();
  const int nSpots = (int) nodes.size();

  // Original position of cells.
  std::vector<std::pair<int, int>> pos(nNodes);
  // Original segment assignment of cells.
  std::vector<std::vector<DetailedSeg*>> seg(nNodes);
  for (size_t i = 0; i < nodes.size(); i++) {
    Node* ndi = nodes[i];

    pos[i] = std::make_pair(ndi->getLeft(), ndi->getBottom());
    seg[i] = mgrPtr_->getReverseCellToSegs(ndi->getId());  // copy!
  }

  lemon::ListDigraph g;
  std::vector<lemon::ListDigraph::Node> nodeForCell;
  std::vector<lemon::ListDigraph::Node> nodeForSpot;
  nodeForCell.resize(nNodes);
  nodeForSpot.resize(nNodes);
  for (size_t i = 0; i < nodes.size(); i++) {
    nodeForCell[i] = g.addNode();
    nodeForSpot[i] = g.addNode();
  }
  lemon::ListDigraph::Node supplyNode = g.addNode();
  lemon::ListDigraph::Node demandNode = g.addNode();

  // Hook up the graph.
  lemon::ListDigraph::ArcMap<int> l_i(g);  // Lower bound on flow.
  lemon::ListDigraph::ArcMap<int> u_i(g);  // Upper bound on flow.
  lemon::ListDigraph::ArcMap<int> c_i(g);  // Cost of flow.

  std::map<lemon::ListDigraph::Arc, std::pair<int, int>> reverseMap;

  double icost;
  for (int i = 0; i < nNodes; i++) {
    // Supply to node.
    lemon::ListDigraph::Arc arc_sv = g.addArc(supplyNode, nodeForCell[i]);
    l_i[arc_sv] = 0;
    u_i[arc_sv] = 1;
    c_i[arc_sv] = 0;

    // Spot to demand.
    lemon::ListDigraph::Arc arc_vt = g.addArc(nodeForSpot[i], demandNode);
    l_i[arc_vt] = 0;
    u_i[arc_vt] = 1;
    c_i[arc_vt] = 0;

    // Nodes to spots.
    const Node* ndi = nodes[i];
    for (int j = 0; j < nSpots; j++) {
      // Determine the cost of assigning cell "ndi" to the
      // current position.  Note that we might want to
      // skip this location if it violates the maximum
      // displacement limit.  We _never_ prevent a cell
      // from being assigned to its original position as
      // this guarantees a solution!
      if (i != j) {
        double dx = std::fabs(pos[j].first - ndi->getOrigLeft());
        if ((int) std::ceil(dx) > mgrPtr_->getMaxDisplacementX()) {
          continue;
        }
        double dy = std::fabs(pos[j].second - ndi->getOrigBottom());
        if ((int) std::ceil(dy) > mgrPtr_->getMaxDisplacementY()) {
          continue;
        }
      }

      // Okay to assign the cell to this location.
      if (obj_ == DetailedMis::Hpwl) {
        icost = getHpwl(ndi,
                        pos[j].first + 0.5 * ndi->getWidth(),
                        pos[j].second + 0.5 * ndi->getHeight());
      } else {
        icost = getDisp(ndi,
                        pos[j].first + 0.5 * ndi->getWidth(),
                        pos[j].second + 0.5 * ndi->getHeight());
      }

      // Node to spot.
      lemon::ListDigraph::Arc arc_vu = g.addArc(nodeForCell[i], nodeForSpot[j]);
      l_i[arc_vu] = 0;
      u_i[arc_vu] = 1;
      c_i[arc_vu] = icost > std::numeric_limits<int>::max()
                        ? std::numeric_limits<int>::max()
                        : static_cast<int>(icost);

      reverseMap[arc_vu] = std::make_pair(i, j);
    }
  }
  // Try max flow.
  lemon::Preflow<lemon::ListDigraph> preflow(g, u_i, supplyNode, demandNode);
  preflow.run();
  const int maxFlow = preflow.flowValue();
  if (maxFlow != nNodes) {
    return;
  }
  // Find mincost flow.
  lemon::NetworkSimplex<lemon::ListDigraph> mincost(g);
  mincost.lowerMap(l_i);
  mincost.upperMap(u_i);
  mincost.costMap(c_i);
  mincost.stSupply(supplyNode, demandNode, maxFlow);
  // lemon::CycleCanceling<lemon::ListDigraph>::ProblemType ret = mincost.run();
  lemon::NetworkSimplex<lemon::ListDigraph>::ProblemType ret = mincost.run();
  if (ret != lemon::NetworkSimplex<lemon::ListDigraph>::OPTIMAL) {
    return;
  }

  // Get the solution and assign nodes to new spots.  We also need to update the
  // assignment of cells to segments!  I _believe_ it should be fine to go cell
  // by cell and remove, reposition and update segment assignments one-by-one.
  //
  // This is somewhat tricky.  We need to use the target spot to figure out the
  // segments into which the cell needs to be replaced.

  lemon::ListDigraph::ArcMap<int> flow(g);
  mincost.flowMap(flow);

  for (lemon::ListDigraph::ArcMap<int>::ItemIt it(flow); it != lemon::INVALID;
       ++it) {
    if (g.target(it) != demandNode && g.source(it) != supplyNode
        && mincost.flow(it) != 0) {
      auto it1 = reverseMap.find(it);
      if (reverseMap.end() == it1) {
        mgrPtr_->internalError("Unable to interpret flow during matching");
      }

      const int i = it1->second.first;
      const int j = it1->second.second;

      // If cell "i" is assigned to location "i", it means that it has not
      // moved. We don't need to remove and reinsert it...

      Node* ndi = nodes[i];
      const Node* ndj = nodes[j];

      const int spanned_i = arch_->getCellHeightInRows(ndi);
      const int spanned_j = arch_->getCellHeightInRows(ndj);

      if (ndi != ndj) {
        if (spanned_i != spanned_j || ndi->getWidth() != ndj->getWidth()
            || ndi->getHeight() != ndj->getHeight()) {
          mgrPtr_->internalError("Unable to interpret flow during matching");
        }

        // Remove cell "i" from its old segments.
        std::vector<DetailedSeg*>& old_segs = seg[i];
        if (spanned_i != old_segs.size()) {
          // This means an error someplace else...
          mgrPtr_->internalError("Unable to interpret flow during matching");
        }
        for (const DetailedSeg* segPtr : old_segs) {
          const int segId = segPtr->getSegId();
          mgrPtr_->removeCellFromSegment(ndi, segId);
        }

        // Update the postion of cell "i".
        ndi->setLeft(pos[j].first);
        ndi->setBottom(pos[j].second);

        // Determine new segments and add cell "i" to its new segments.
        const std::vector<DetailedSeg*>& new_segs = seg[j];
        if (spanned_i != new_segs.size()) {
          // Not setup for non-same size stuff right now.
          mgrPtr_->internalError("Unable to interpret flow during matching");
        }
        for (const DetailedSeg* segPtr : new_segs) {
          const int segId = segPtr->getSegId();
          mgrPtr_->addCellToSegment(ndi, segId);
        }
      }
    }
  }
}

//////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////
double DetailedMis::getDisp(const Node* ndi, double xi, double yi)
{
  // Compute displacement of cell ndi if placed at (xi,y1) from its orig pos.

  // Specified target is cell center.  Need to offset.
  xi -= 0.5 * ndi->getWidth();
  yi -= 0.5 * ndi->getHeight();
  const double dx = std::fabs(xi - ndi->getOrigLeft());
  const double dy = std::fabs(yi - ndi->getOrigBottom());
  return dx + dy;
}

//////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////
double DetailedMis::getHpwl(const Node* ndi, double xi, double yi)
{
  // Compute the HPWL of nets connected to ndi assuming ndi is at the
  // specified (xi,yi).

  double hpwl = 0.;
  Rectangle box;
  for (int pi = 0; pi < ndi->getNumPins(); pi++) {
    const Pin* pini = ndi->getPins()[pi];

    const Edge* edi = pini->getEdge();

    const int npins = edi->getNumPins();
    if (npins <= 1 || npins > skipEdgesLargerThanThis_) {
      continue;
    }

    box.reset();
    for (int pj = 0; pj < edi->getNumPins(); pj++) {
      const Pin* pinj = edi->getPins()[pj];

      const Node* ndj = pinj->getNode();

      const double x
          = (ndj == ndi)
                ? (xi + pinj->getOffsetX())
                : (ndj->getLeft() + 0.5 * ndj->getWidth() + pinj->getOffsetX());
      const double y = (ndj == ndi) ? (yi + pinj->getOffsetY())
                                    : (ndj->getBottom() + 0.5 * ndj->getHeight()
                                       + pinj->getOffsetY());

      box.addPt(x, y);
    }
    if (box.xmax() >= box.xmin() && box.ymax() >= box.ymin()) {
      hpwl += (box.getWidth() + box.getHeight());
    }
  }

  return hpwl;
}

}  // namespace dpo
