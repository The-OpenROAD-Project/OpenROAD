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

#include "detailed_mis.h"
#include <lemon/cost_scaling.h>
#include <lemon/cycle_canceling.h>
#include <lemon/list_graph.h>
#include <lemon/preflow.h>
#include <lemon/smart_graph.h>
#include <boost/format.hpp>
#include <boost/tokenizer.hpp>
#include <deque>
#include <set>
#include <vector>
#include "architecture.h"
#include "color.h"
#include "detailed_manager.h"
#include "detailed_segment.h"
#include "network.h"
#include "router.h"

// DEFINES ===================================================================
#ifdef DEBUG
#define SANITY_CHECK_NO_DUPLICATES(nodes)         \
  do {                                            \
    std::set<Node*> dups;                         \
    for (unsigned i = 0; i < nodes.size(); ++i) { \
      Node* nd = nodes[i];                        \
      ASSERT(dups.count(nd) == 0);                \
      dups.insert(nd);                            \
    }                                             \
  } while (0);
#else
#define SANITY_CHECK_NO_DUPLICATES(nodes)
#endif

namespace dpo {

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
class DetailedMis::Bucket {
 public:
  Bucket(void) {}
  virtual ~Bucket(void) {}

 public:
  inline void clear(void) { m_nodes.clear(); }

 public:
  std::deque<Node*> m_nodes;
  double m_xmin;
  double m_xmax;
  double m_ymin;
  double m_ymax;
  int m_i;
  int m_j;
  int m_travId;
};

/*

// BBOX ======================================================================
class DetailedMis::BBox
// ***************
// Used for tracking edge bounding boxes.
{
public:
        inline bool IsValid( void )
        // ************************
        {
                return _xmax >= _xmin;	// (Only need to check x, as y would be
redundant)
        }

public:
        double			_xmax;
        double			_xmin;
        double			_ymax;
        double			_ymin;
};







// VARIOUS STRUCTURES ========================================================
struct DetailedMis::NodeBin
// *******************
{
        unsigned	_high;
        unsigned	_low;
};


struct DetailedMis::SortNodesByArea
// ***************************
// Used to sort netlist nodes by area.
{
        inline bool operator() ( Node *p, Node *q ) const
        // **********************************************
        {
                return p->GetArea() < q->GetArea();
        }
};


struct VertexDegree
// ****************
{
        unsigned	_degree;
        unsigned	_v;
};


struct SortVertexDegree
// ********************
{
        inline bool operator() ( const VertexDegree &p, const VertexDegree &q )
const
        // ********************************************************************
        {
                return p._degree > q._degree;
        }
};

*/

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
DetailedMis::DetailedMis(Architecture* arch, Network* network,
                         RoutingParams* rt)
    : m_mgrPtr(0),
      m_arch(arch),
      m_network(network),
      m_rt(rt),
      m_skipEdgesLargerThanThis(100),
      m_maxProblemSize(50),
      m_useSameSize(true),
      m_useSameColor(true),
      m_maxTimesUsed(3),
      m_obj(DetailedMis::Hpwl) {
  /*
          _mappingNodeToLocation.resize( _params._maxNumNodes * 2 + 2 );
          std::fill( _mappingNodeToLocation.begin(),
     _mappingNodeToLocation.end(), (SwapLocation *) NULL );
   */
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
DetailedMis::~DetailedMis(void) { clearGrid(); }

//////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////
void DetailedMis::run(DetailedMgr* mgrPtr, std::string command) {
  // A temporary interface to allow for a string which we will decode to create
  // the arguments.
  std::string scriptString = command;
  boost::char_separator<char> separators(" \r\t\n;");
  boost::tokenizer<boost::char_separator<char> > tokens(scriptString,
                                                        separators);
  std::vector<std::string> args;
  for (boost::tokenizer<boost::char_separator<char> >::iterator it =
           tokens.begin();
       it != tokens.end(); it++) {
    args.push_back(*it);
  }
  run(mgrPtr, args);
}

//////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////
void DetailedMis::run(DetailedMgr* mgrPtr, std::vector<std::string>& args) {
  // Given the arguments, figure out which routine to run to do the reordering.

  m_mgrPtr = mgrPtr;

  // Defaults.
  m_obj = DetailedMis::Hpwl;

  int passes = 1;
  double tol = 0.01;
  for (size_t i = 1; i < args.size(); i++) {
    if (args[i] == "-p" && i + 1 < args.size()) {
      passes = std::atoi(args[++i].c_str());
    } else if (args[i] == "-t" && i + 1 < args.size()) {
      tol = std::atof(args[++i].c_str());
    } else if (args[i] == "-d") {
      m_obj = DetailedMis::Disp;
    }
  }
  tol = std::max(tol, 0.01);
  passes = std::max(passes, 1);

  double last_hpwl, curr_hpwl, init_hpwl, hpwl_x, hpwl_y;
  double last_disp, curr_disp, init_disp, tot_disp, max_disp, avg_disp;

  std::cout << "Set matching objective is "
            << ((m_obj == DetailedMis::Hpwl) ? "wire length" : "displacement")
            << std::endl;

  // If using displacement objective, then it isn't required to use colors.
  if (m_obj == DetailedMis::Disp) {
    m_useSameColor = false;
  }

  m_mgrPtr->resortSegments();
  curr_hpwl = Utility::hpwl(m_network, hpwl_x, hpwl_y);
  init_hpwl = curr_hpwl;

  curr_disp = Utility::disp_l1(m_network, tot_disp, max_disp, avg_disp);
  init_disp = curr_disp;
  for (int p = 1; p <= passes; p++) {
    std::cout << boost::format(
                     "Pass %3d of mis; hpwl is %.6e; disp is %.6e\n") %
                     p % curr_hpwl % curr_disp;

    // Run the algo here...
    place();

    last_hpwl = curr_hpwl;
    curr_hpwl = Utility::hpwl(m_network, hpwl_x, hpwl_y);
    if (m_obj == DetailedMis::Hpwl &&
        std::fabs(curr_hpwl - last_hpwl) / last_hpwl <= tol) {
      std::cout << "Terminating due to low improvement in hpwl." << std::endl;
      break;
    }
    last_disp = curr_disp;
    curr_disp = Utility::disp_l1(m_network, tot_disp, max_disp, avg_disp);
    if (m_obj == DetailedMis::Disp &&
        std::fabs(curr_disp - last_disp) / last_disp <= tol) {
      std::cout << "Terminating due to low improvement in disp." << std::endl;
      break;
    }
  }
  m_mgrPtr->resortSegments();
  std::cout << boost::format(
                   "End of passes for mis; hpwl is %.6e, total imp is %.2lf%%; "
                   "disp is %.6e, total imp is %.2lf%%\n") %
                   curr_hpwl % (((init_hpwl - curr_hpwl) / init_hpwl) * 100.) %
                   curr_disp % (((init_disp - curr_disp) / init_disp) * 100.);
}

//////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////
void DetailedMis::place(void) {
  // Collect cells we will consider moving.
  collectMovableCells();

  // Need to color the cells.
  colorCells();

  // Build a grid for locating close cells.
  buildGrid();
  populateGrid();

  m_timesUsed.resize(m_network->m_nodes.size());
  std::fill(m_timesUsed.begin(), m_timesUsed.end(), 0);

  // Select candidates and solve matching problem.  Note that we need to do
  // something to make this more efficient, otherwise we will solve way too
  // many problems for larger circuits.  I think one effective idea is to
  // keep track of how many problems a candidate cell has been involved in;
  // if it has been involved is >= a certain number of problems, it has "had
  // some chance" to be moved, so skip it.
  Utility::random_shuffle(m_candidates.begin(), m_candidates.end(),
                          m_mgrPtr->m_rng);
  for (size_t i = 0; i < m_candidates.size(); i++) {
    // Pick a candidate as a seed.
    Node* ndi = m_candidates[i];

    // Skip seed if it has been used already.
    if (m_timesUsed[ndi->getId()] >= m_maxTimesUsed) {
      continue;
    }

    // Get other cells within the vicinity.
    gatherNeighbours(ndi);

    // Solve the flow.
    solveMatch();

    // Increment times each node has been used.
    for (size_t j = 0; j < m_neighbours.size(); j++) {
      Node* ndj = m_neighbours[j];
      ++m_timesUsed[ndj->getId()];
    }

    // Update grid?  Or, do we need to even bother?
    ;
  }
}

//////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////
void DetailedMis::collectMovableCells(void) {
  m_candidates.erase(m_candidates.begin(), m_candidates.end());
  m_candidates.insert(m_candidates.end(), m_mgrPtr->m_singleHeightCells.begin(),
                      m_mgrPtr->m_singleHeightCells.end());
  for (size_t i = 2; i < m_mgrPtr->m_multiHeightCells.size(); i++) {
    m_candidates.insert(m_candidates.end(),
                        m_mgrPtr->m_multiHeightCells[i].begin(),
                        m_mgrPtr->m_multiHeightCells[i].end());
  }
  std::cout << "Number of candidate cells is " << m_candidates.size()
            << std::endl;
}

//////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////
void DetailedMis::colorCells(void) {
  std::cout << "Coloring the cells (skipping edges > "
            << m_skipEdgesLargerThanThis << ")" << std::endl;

  m_colors.resize(m_network->m_nodes.size());
  std::fill(m_colors.begin(), m_colors.end(), -1);

  m_movable.resize(m_network->m_nodes.size());
  std::fill(m_movable.begin(), m_movable.end(), false);
  for (size_t i = 0; i < m_candidates.size(); i++) {
    Node* ndi = m_candidates[i];
    m_movable[ndi->getId()] = true;
  }

  Graph gr(m_network->m_nodes.size());
  for (int e = 0; e < m_network->m_edges.size(); e++) {
    Edge* edi = &(m_network->m_edges[e]);

    int numPins = edi->getNumPins();
    if (numPins <= 1 || numPins > m_skipEdgesLargerThanThis) {
      continue;
    }

    for (int pi = edi->getFirstPinIdx(); pi < edi->getLastPinIdx(); pi++) {
      Pin* pini = m_network->m_edgePins[pi];
      Node* ndi = &(m_network->m_nodes[pini->getNodeId()]);
      if (!m_movable[ndi->getId()]) {
        continue;
      }

      for (int pj = pi + 1; pj < edi->getLastPinIdx(); pj++) {
        Pin* pinj = m_network->m_edgePins[pj];
        Node* ndj = &(m_network->m_nodes[pinj->getNodeId()]);
        if (!m_movable[ndj->getId()]) {
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
  for (size_t i = 0; i < m_network->m_nodes.size(); i++) {
    Node* ndi = &(m_network->m_nodes[i]);

    int color = gr.m_color[i];
    if (color < 0 || color >= gr.m_ncolors) {
      std::cout << "Error." << std::endl;
      exit(-1);
    }
    if (m_movable[ndi->getId()]) {
      m_colors[ndi->getId()] = color;

      if (color >= hist.size()) {
        hist.resize(color + 1, 0);
      }
      ++hist[color];
    }
  }
  std::cout << "Colors required for netlist is " << gr.m_ncolors << std::endl;
  //    std::cout << "Histogram of colors:" << std::endl;
  //    size_t j = 0;
  //    for( size_t i = 0; i < hist.size(); i++ )
  //    {
  //        if( hist[i] > 0 )
  //        {
  //            ++j;
  //            std::cout << boost::format( "Color %3d, Cells with this color is
  //            %d\n" ) % j % hist[i];
  //        }
  //    }
}

//////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////
void DetailedMis::buildGrid(void) {
  // Builds a coarse grid over the placement region for locating cells.
  m_traversal = 0;

  double xmin = m_arch->getMinX();
  double xmax = m_arch->getMaxX();
  double ymin = m_arch->getMinY();
  double ymax = m_arch->getMaxY();

  // Design each grid bin to hold a few hundred cells.  Do this based on the
  // average width and height of the cells.
  double avgH = 0.;
  double avgW = 0.;
  double avgA = 0.;
  for (size_t i = 0; i < m_candidates.size(); i++) {
    Node* ndi = m_candidates[i];
    avgH += ndi->getHeight();
    avgW += ndi->getWidth();
    avgA += ndi->getHeight() * ndi->getWidth();
  }
  avgH /= (double)m_candidates.size();
  avgW /= (double)m_candidates.size();
  avgA /= (double)m_candidates.size();

  m_stepX = avgW * std::sqrt(200);
  m_stepY = avgH * std::sqrt(200);

  m_dimW = (int)std::ceil((xmax - xmin) / m_stepX);
  m_dimH = (int)std::ceil((ymax - ymin) / m_stepY);

  //    std::cout << "Grid is " << m_dimW << "x" << m_dimH << std::endl;

  clearGrid();
  m_grid.resize(m_dimW);
  for (size_t i = 0; i < m_grid.size(); i++) {
    m_grid[i].resize(m_dimH);
    for (size_t j = 0; j < m_grid[i].size(); j++) {
      m_grid[i][j] = new Bucket;
      m_grid[i][j]->m_xmin = xmin + (i)*m_stepX;
      m_grid[i][j]->m_xmax = xmin + (i + 1) * m_stepX;
      m_grid[i][j]->m_ymin = ymin + (j)*m_stepY;
      m_grid[i][j]->m_ymax = ymin + (j + 1) * m_stepY;
      m_grid[i][j]->m_i = i;
      m_grid[i][j]->m_j = j;
      m_grid[i][j]->m_travId = m_traversal;
    }
  }
}

//////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////
void DetailedMis::populateGrid(void) {
  // Inserts movable cells into the grid.

  //    std::cout << "Populating grid." << std::endl;

  for (size_t i = 0; i < m_grid.size(); i++) {
    for (size_t j = 0; j < m_grid[i].size(); j++) {
      m_grid[i][j]->clear();
    }
  }

  double xmin = m_arch->getMinX();
  double xmax = m_arch->getMaxX();
  double ymin = m_arch->getMinY();
  double ymax = m_arch->getMaxY();

  // Insert cells into the constructed grid.
  m_cellToBinMap.clear();
  for (size_t n = 0; n < m_candidates.size(); ++n) {
    Node* ndi = m_candidates[n];

    int i = std::max(
        std::min((int)((ndi->getX() - xmin) / m_stepX), m_dimW - 1), 0);
    int j = std::max(
        std::min((int)((ndi->getY() - ymin) / m_stepY), m_dimH - 1), 0);

    m_grid[i][j]->m_nodes.push_back(ndi);
    m_cellToBinMap[ndi] = m_grid[i][j];
  }
}

//////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////
void DetailedMis::clearGrid(void)
// Clear out any old grid.  The dimensions of the grid are also stored in the
// class...
{
  unsigned i, j;

  for (size_t i = 0; i < m_grid.size(); i++) {
    for (size_t j = 0; j < m_grid[i].size(); j++) {
      delete m_grid[i][j];
    }
    m_grid[i].clear();
  }
  m_grid.clear();
}

//////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////
void DetailedMis::gatherNeighbours(Node* ndi) {
  double singleRowHeight = m_arch->m_rows[0]->getH();

  m_neighbours.clear();
  m_neighbours.push_back(ndi);

  std::map<Node*, Bucket*>::iterator it;

  // Scan the grid structure gathering up cells which are compatible with the
  // current cell.

  if (m_cellToBinMap.end() == (it = m_cellToBinMap.find(ndi))) {
    std::cout << "Error." << std::endl;
    exit(-1);
  }

  int spanned_i = (int)(ndi->getHeight() / singleRowHeight + 0.5);

  std::deque<Bucket*> Q;
  Q.push_back(it->second);
  ++m_traversal;
  while (Q.size() > 0) {
    Bucket* currPtr = Q.front();
    Q.pop_front();

    if (currPtr->m_travId == m_traversal) {
      continue;
    }
    currPtr->m_travId = m_traversal;

    // Scan all the cells in this bucket.  If they are compatible with the
    // original cell, then add them to the neighbour list.
    for (size_t j = 0; j < currPtr->m_nodes.size(); j++) {
      Node* ndj = currPtr->m_nodes[j];

      int spanned_j = (int)(ndj->getHeight() / singleRowHeight + 0.5);

      // Check to make sure the cell is not the original, that they have
      // the same region, that they have the same size (if applicable),
      // and that they have the same color (if applicable).
      bool compat = true;
      if (compat) {
        // Same node!
        if (ndj == ndi) {
          compat = false;
        }
      }
      if (compat) {
        // Must be the same color to avoid sharing nets.
        if (m_useSameColor &&
            m_colors[ndi->getId()] != m_colors[ndj->getId()]) {
          compat = false;
        }
      }
      if (compat) {
        // Must be the same size.
        if (m_useSameSize && (ndi->getWidth() != ndj->getWidth() ||
                              ndi->getHeight() != ndj->getHeight())) {
          compat = false;
        }
      }
      if (compat) {
        // Must span the same number of rows and also be voltage compatible.
        if (spanned_i != spanned_j || ndi->getBottomPower() != ndj->getBottomPower() ||
            ndi->getTopPower() != ndj->getTopPower()) {
          compat = false;
        }
      }
      if (compat) {
        // Must be in the same region.
        if (ndj->getRegionId() != ndi->getRegionId()) {
          compat = false;
        }
      }

      // If compatible, include this current cell.
      if (compat) {
        m_neighbours.push_back(ndj);
      }
    }

    if (m_neighbours.size() >= m_maxProblemSize) {
      break;
    }

    // Add more bins to the queue if we have not yet collected enough cells.
    if (currPtr->m_i - 1 >= 0)
      Q.push_back(m_grid[currPtr->m_i - 1][currPtr->m_j]);
    if (currPtr->m_i + 1 <= m_dimW - 1)
      Q.push_back(m_grid[currPtr->m_i + 1][currPtr->m_j]);
    if (currPtr->m_j - 1 >= 0)
      Q.push_back(m_grid[currPtr->m_i][currPtr->m_j - 1]);
    if (currPtr->m_j + 1 <= m_dimH - 1)
      Q.push_back(m_grid[currPtr->m_i][currPtr->m_j + 1]);
  }
}

//////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////
void DetailedMis::solveMatch(void) {
  if (m_neighbours.size() <= 1) {
    return;
  }
  std::vector<Node*>& nodes = m_neighbours;
  double hpwl_x, hpwl_y, hpwl;
  double singleRowHeight = m_arch->m_rows[0]->getH();

  int nNodes = (int)nodes.size();
  int nSpots = (int)nodes.size();
  std::vector<std::pair<double, double> >
      pos;  // Original location of each node.
  pos.resize(nNodes);
  std::vector<std::vector<DetailedSeg*> >
      seg;  // Original segments for each node.
  seg.resize(nNodes);
  for (size_t i = 0; i < nodes.size(); i++) {
    Node* ndi = nodes[i];
    pos[i] = std::make_pair(ndi->getX(), ndi->getY());     // COPY!
    seg[i] = m_mgrPtr->m_reverseCellToSegs[ndi->getId()];  // COPY!
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

  std::map<lemon::ListDigraph::Arc, std::pair<int, int> > reverseMap;

  int origCost = 0, icost;
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
    Node* ndi = nodes[i];
    for (int j = 0; j < nSpots; j++) {
      icost = 0;
      if (m_obj == DetailedMis::Hpwl) {
        icost = (int)getHpwl(ndi, pos[j].first, pos[j].second);
      } else {
        icost = (int)getDisp(ndi, pos[j].first, pos[j].second);
      }
      origCost += (i != j) ? 0 : icost;

      bool addEdge = true;
      // Reasons to skip the edge?
      ;

      // Add the edge if its okay.
      if (addEdge) {
        // Node to spot.
        lemon::ListDigraph::Arc arc_vu =
            g.addArc(nodeForCell[i], nodeForSpot[j]);
        l_i[arc_vu] = 0;
        u_i[arc_vu] = 1;
        c_i[arc_vu] = icost;

        reverseMap[arc_vu] = std::make_pair(i, j);
      }
    }
  }
  // Try max flow.
  lemon::Preflow<lemon::ListDigraph> preflow(g, u_i, supplyNode, demandNode);
  preflow.run();
  int maxFlow = preflow.flowValue();
  if (maxFlow != nNodes) {
    std::cout << "Error." << std::endl;
    exit(-1);
  }
  // Find mincost flow.
  lemon::CycleCanceling<lemon::ListDigraph> mincost(g);
  mincost.lowerMap(l_i);
  mincost.upperMap(u_i);
  mincost.costMap(c_i);
  mincost.stSupply(supplyNode, demandNode, maxFlow);
  lemon::CycleCanceling<lemon::ListDigraph>::ProblemType ret = mincost.run();

  switch (ret) {
    case lemon::CostScaling<lemon::ListDigraph>::INFEASIBLE:
      // std::cout << "Min cost flow solver, result is INFEASIBLE" << std::endl;
      exit(-1);
      break;
    case lemon::CostScaling<lemon::ListDigraph>::OPTIMAL:
      // std::cout << "Min cost flow solver, result is OPTIMAL   " << std::endl;
      break;
    case lemon::CostScaling<lemon::ListDigraph>::UNBOUNDED:
      // std::cout << "Min cost flow solver, result is UNBOUNDED " << std::endl;
      exit(-1);
      break;
    default:
      // std::cout << "Min cost flow solver, no result" << std::endl;
      exit(-1);
      break;
  }

  // Get the solution and assign nodes to new spots.  We also need to update the
  // assignment of cells to segments!  I _believe_ it should be fine to go cell
  // by cell and remove, reposition and update segment assignments one-by-one.
  //
  // This is somewhat tricky.  We need to use the target spot to figure out the
  // segments into which the cell needs to be replaced.

  lemon::ListDigraph::ArcMap<int> flow(g);
  mincost.flowMap(flow);
  int supplyFlow = 0;
  int demandFlow = 0;
  int finalCost = 0;
  int nMoved = 0;

  // m_mgrPtr->debugSegments();

  for (lemon::ListDigraph::ArcMap<int>::ItemIt it(flow); it != lemon::INVALID;
       ++it) {
    if (g.target(it) == demandNode) {
      demandFlow += mincost.flow(it);
    }
    if (g.source(it) == supplyNode) {
      supplyFlow += mincost.flow(it);
    }

    if (g.target(it) != demandNode && g.source(it) != supplyNode &&
        mincost.flow(it) != 0) {
      std::map<lemon::ListDigraph::Arc, std::pair<int, int> >::iterator it1 =
          reverseMap.find(it);
      if (reverseMap.end() == it1) {
        std::cout << "Error." << std::endl;
        exit(-1);
      }

      int i = it1->second.first;
      int j = it1->second.second;

      // If cell "i" is assigned to location "i", it means that it has not
      // moved. We don't need to remove and reinsert it...

      Node* ndi = nodes[i];
      Node* ndj =
          nodes[j];  // We effectively "moved" into this cells's location.
      int spanned_i = (int)(ndi->getHeight() / singleRowHeight + 0.5);
      int spanned_j = (int)(ndj->getHeight() / singleRowHeight + 0.5);

      if (ndi != ndj) {
        ++nMoved;
        if (spanned_i != spanned_j) {
          std::cout << "Cells " << ndi->getId() << " and " << ndj->getId()
                    << ", "
                    << "don't span the same number of rows." << std::endl;
          exit(-1);
        }
        if (ndi->getWidth() != ndj->getWidth() ||
            ndi->getHeight() != ndj->getHeight()) {
          std::cout << "Cells " << ndi->getId() << " and " << ndj->getId()
                    << ", "
                    << "don't have the same dimensions." << std::endl;
          exit(-1);
        }

        // Remove cell "i" from its old segments.
        std::vector<DetailedSeg*>& old_segs = seg[i];
        if (spanned_i != old_segs.size()) {
          // This means an error someplace else...
          std::cout << "Error." << std::endl;
          exit(-1);
        }
        for (size_t s = 0; s < old_segs.size(); s++) {
          DetailedSeg* segPtr = old_segs[s];
          int segId = segPtr->m_segId;
          m_mgrPtr->removeCellFromSegment(ndi, segId);
        }

        // Update the postion of cell "i".
        ndi->setX(pos[j].first);
        ndi->setY(pos[j].second);

        // Determine new segments and add cell "i" to its new segments.
        std::vector<DetailedSeg*>& new_segs = seg[j];
        if (spanned_i != new_segs.size()) {
          // Not setup for non-same size stuff right now.
          std::cout << "Cell " << ndi->getId() << ", Rows spanned is "
                    << spanned_i << ", "
                    << "assigned to position occupied by cell " << ndj->getId()
                    << ", Rows spanned is " << spanned_j << ", "
                    << "found in only " << new_segs.size() << " segments."
                    << std::endl;
          exit(-1);
        }
        for (size_t s = 0; s < new_segs.size(); s++) {
          DetailedSeg* segPtr = new_segs[s];
          int segId = segPtr->m_segId;
          m_mgrPtr->addCellToSegment(ndi, new_segs[s]->m_segId);
        }
      }

      icost = 0;
      if (m_obj == DetailedMis::Hpwl) {
        icost = (int)getHpwl(ndi, pos[j].first, pos[j].second);
      } else {
        icost = (int)getDisp(ndi, pos[j].first, pos[j].second);
      }
      finalCost += icost;
    }
  }

  // std::cout << "Flow-based replacement; Moved " << nMoved << " cells of " <<
  // m_neighbours.size() << ", "
  //    << "original hpwl is " << origCost << ", "
  //    << "final hpwl is " << finalCost << std::endl;

  // int err = m_mgrPtr->checkOverlapInSegments();
  //(void)err;
}

//////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////
double DetailedMis::getDisp(Node* ndi, double xi, double yi) {
  // Compute displacement of cell ndi if placed at (xi,y1) from its orig pos.
  double dx = std::fabs(xi - ndi->getOrigX());
  double dy = std::fabs(yi - ndi->getOrigY());
  return dx + dy;
}

//////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////
double DetailedMis::getHpwl(Node* ndi, double xi, double yi) {
  // Compute the HPWL of nets connected to ndi assuming ndi is at the
  // specified (xi,yi).

  double hpwl = 0.;
  double x, y, l, r, b, t;
  for (int pi = ndi->getFirstPinIdx(); pi < ndi->getLastPinIdx(); pi++) {
    Pin* pini = m_network->m_nodePins[pi];

    Edge* edi = &(m_network->m_edges[pini->getEdgeId()]);

    int npins = edi->getNumPins();
    if (npins <= 1 || npins > m_skipEdgesLargerThanThis) {
      continue;
    }

    l = std::numeric_limits<double>::max();
    r = -std::numeric_limits<double>::max();
    b = std::numeric_limits<double>::max();
    t = -std::numeric_limits<double>::max();

    for (int pj = edi->getFirstPinIdx(); pj < edi->getLastPinIdx(); pj++) {
      Pin* pinj = m_network->m_edgePins[pj];

      Node* ndj = &(m_network->m_nodes[pinj->getNodeId()]);

      x = (ndj == ndi) ? (xi + pinj->getOffsetX())
                       : (ndj->getX() + pinj->getOffsetX());
      y = (ndj == ndi) ? (yi + pinj->getOffsetY())
                       : (ndj->getY() + pinj->getOffsetY());

      l = std::min(l, x);
      r = std::max(r, x);
      b = std::min(b, y);
      t = std::max(t, y);
    }
    if (r >= l && t >= b) {
      hpwl += ((r - l) + (t - b));
    }
  }

  return hpwl;
}

}  // namespace dpo
