///////////////////////////////////////////////////////////////////////////////
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

///////////////////////////////////////////////////////////////////////////////
// Description:
// Simple implementation of optimal single row interleaving.

////////////////////////////////////////////////////////////////////////////////
// Includes.
////////////////////////////////////////////////////////////////////////////////
#include <stdio.h>
#include <stdlib.h>

#include <algorithm>
#include <boost/format.hpp>
#include <boost/tokenizer.hpp>
#include <cmath>
#include <iostream>
#include <stack>
#include <utility>

#include "detailed_interleaving.h"
#include "detailed_manager.h"
#include "detailed_segment.h"
#include "utility.h"
#include "utl/Logger.h"

using utl::DPO;

namespace dpo {

////////////////////////////////////////////////////////////////////////////////
// Defines.
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
// Classes.
////////////////////////////////////////////////////////////////////////////////
class DetailedInterleave::EdgeAndOffset
{
 public:
  EdgeAndOffset(int edge, double offset) : m_edge(edge), m_offset(offset) {}
  EdgeAndOffset(const EdgeAndOffset& other)
      : m_edge(other.m_edge), m_offset(other.m_offset)
  {
  }
  EdgeAndOffset& operator=(const EdgeAndOffset& other)
  {
    if (this != &other) {
      m_edge = other.m_edge;
      m_offset = other.m_offset;
    }
    return *this;
  }

 public:
  int m_edge;
  double m_offset;
};

class DetailedInterleave::EdgeInterval
{
 public:
  EdgeInterval()
      : m_xmin(std::numeric_limits<double>::max()),
        m_xmax(-std::numeric_limits<double>::max()),
        m_empty(true)
  {
  }
  EdgeInterval(double xmin, double xmax)
      : m_xmin(xmin), m_xmax(xmax), m_empty(false)
  {
  }
  EdgeInterval(const EdgeInterval& other)
      : m_xmin(other.m_xmin), m_xmax(other.m_xmax), m_empty(other.m_empty)
  {
  }
  EdgeInterval& operator=(const EdgeInterval& other)
  {
    if (this != &other) {
      m_xmin = other.m_xmin;
      m_xmax = other.m_xmax;
      m_empty = other.m_empty;
    }
    return *this;
  }

  double width() const { return (m_empty ? 0.0 : (m_xmax - m_xmin)); }

  void add(double x)
  {
    m_xmin = std::min(x, m_xmin);
    m_xmax = std::max(x, m_xmax);
    m_empty = false;
  }

 public:
  double m_xmin;
  double m_xmax;
  bool m_empty;
};

class DetailedInterleave::SmallProblem
{
 public:
  SmallProblem() { clear(); }
  SmallProblem(int nNodes, int nEdges)
  {
    clear();
    init(nNodes, nEdges);
  }

  void init(int nNodes, int nEdges)
  {
    m_nNodes = nNodes;
    m_nEdges = nEdges;
    m_adjEdges.resize(nNodes);
    m_widths.resize(nNodes);
    m_x.resize(nNodes);
    m_netBoxes.resize(nEdges);
  }
  void clear()
  {
    m_xmin = std::numeric_limits<double>::max();
    m_xmax = -std::numeric_limits<double>::min();
    m_nNodes = 0;
    m_nEdges = 0;
    m_adjEdges.clear();
    m_widths.clear();
    m_x.clear();
    m_netBoxes.clear();
  }

 public:
  int m_nNodes;
  int m_nEdges;
  double m_xmin;
  double m_xmax;
  std::vector<std::vector<EdgeAndOffset>> m_adjEdges;
  std::vector<double> m_widths;
  std::vector<double> m_x;
  std::vector<EdgeInterval> m_netBoxes;
};

class DetailedInterleave::TableEntry
{
 public:
  TableEntry(SmallProblem* problem) : m_problem(problem), m_parent(0)
  {
    init();
  }
  void clear() { init(); }
  void init()
  {
    m_leftEdge = m_problem->m_xmin;
    m_cost = 0;
    m_netBoxes.resize(m_problem->m_nEdges);
    for (int i = 0; i < m_problem->m_nEdges; i++) {
      m_netBoxes[i] = m_problem->m_netBoxes[i];
      m_cost += m_netBoxes[i].width();
    }
  }
  void copy(TableEntry* other)
  {
    m_leftEdge = other->m_leftEdge;
    m_cost = other->m_cost;
    m_netBoxes = other->m_netBoxes;
  }
  void add(int i)
  {
    double x = m_leftEdge + 0.5 * m_problem->m_widths[i];

    std::vector<EdgeAndOffset>& pins = m_problem->m_adjEdges[i];
    for (int p = 0; p < pins.size(); p++) {
      int edge = pins[p].m_edge;
      double offset = pins[p].m_offset;

      m_cost -= m_netBoxes[edge].width();
      m_netBoxes[edge].add(x + offset);
      m_cost += m_netBoxes[edge].width();
    }
    m_leftEdge += m_problem->m_widths[i];
  }

 public:
  SmallProblem* m_problem;
  TableEntry* m_parent;

  std::vector<EdgeInterval> m_netBoxes;
  double m_leftEdge;
  double m_cost;
};

struct DetailedInterleave::CompareNodesX {
  bool operator()(Node* p, Node* q) const {
    return p->getX() < q->getX();
  }
};

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
DetailedInterleave::DetailedInterleave(Architecture* arch,
                                       Network* network,
                                       RoutingParams* rt)
    : m_arch(arch),
      m_network(network),
      m_rt(rt),
      m_mgrPtr(nullptr),
      m_skipNetsLargerThanThis(100),
      m_traversal(0),
      m_windowStep(8),
      m_windowSize(20)
{
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
DetailedInterleave::~DetailedInterleave()
{
}

/////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////
void DetailedInterleave::run(DetailedMgr* mgrPtr, std::string command)
{
  // Interface to allow for a string which can be decoded into arguments.
  std::string scriptString = command;
  boost::char_separator<char> separators(" \r\t\n;");
  boost::tokenizer<boost::char_separator<char>> tokens(scriptString,
                                                       separators);
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
void DetailedInterleave::run(DetailedMgr* mgrPtr,
                             std::vector<std::string>& args)
{
  m_mgrPtr = mgrPtr;

  // Defaults.
  m_windowSize = 20;
  m_windowStep = 8;

  double tol = 0.01;
  int passes = 1;
  for (size_t i = 1; i < args.size(); i++) {
    if (args[i] == "-w" && i + 1 < args.size()) {
      m_windowSize = std::atoi(args[++i].c_str());
    } else if (args[i] == "-s" && i + 1 < args.size()) {
      m_windowStep = std::atoi(args[++i].c_str());
    } else if (args[i] == "-p" && i + 1 < args.size()) {
      passes = std::atoi(args[++i].c_str());
    } else if (args[i] == "-t" && i + 1 < args.size()) {
      tol = std::atof(args[++i].c_str());
    } else {
      ;
    }
  }

  double last_hpwl, curr_hpwl, init_hpwl, hpwl_x, hpwl_y;

  m_mgrPtr->resortSegments();
  curr_hpwl = Utility::hpwl(m_network, hpwl_x, hpwl_y);
  init_hpwl = curr_hpwl;

  for (int pass = 1; pass <= passes; pass++) {
    last_hpwl = curr_hpwl;

    dp();

    curr_hpwl = Utility::hpwl(m_network, hpwl_x, hpwl_y);

    m_mgrPtr->getLogger()->info(DPO,
                                322,
                                "Pass {:3d} of interleaving; hpwl is {:.6e}.",
                                pass,
                                curr_hpwl);

    if (std::fabs(curr_hpwl - last_hpwl) / last_hpwl <= tol) {
      break;
    }
  }
  m_mgrPtr->resortSegments();

  double curr_imp = (((init_hpwl - curr_hpwl) / init_hpwl) * 100.);
  m_mgrPtr->getLogger()->info(DPO,
                              323,
                              "End of interleaving; objective is {:.6e}, "
                              "improvement is {:.2f} percent.",
                              curr_hpwl,
                              curr_imp);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void DetailedInterleave::dp()
{
  // Scan all segments and apply interleaving.  Interleaving only works for
  // single height cells (at this point).
  //
  // Notes:
  // There is some possibility for parallelism.  If cells being optimized
  // are in intervals that don't overlap and the problems are all set up
  // first, then outside terminals can be propagated and don't affect each
  // other.
  //
  // Right now, I don't add any filler cells.  Cells get bloated to fill up
  // the space.  I do try to account for edge spacing/cell padding.

  // Initializations.
  m_traversal = 0;

  m_nodeIds.reserve(m_network->getNumNodes());
  m_edgeIds.reserve(m_network->getNumEdges());

  m_edgeMask.resize(m_network->getNumEdges());
  m_nodeMask.resize(m_network->getNumNodes());
  std::fill(m_edgeMask.begin(), m_edgeMask.end(), m_traversal);
  std::fill(m_nodeMask.begin(), m_nodeMask.end(), m_traversal);

  m_edgeMap.resize(m_network->getNumEdges());
  m_nodeMap.resize(m_network->getNumNodes());
  std::fill(m_edgeMap.begin(), m_edgeMap.end(), -1);
  std::fill(m_nodeMap.begin(), m_nodeMap.end(), -1);

  double leftLimit, rightLimit, leftPadding, rightPadding;

  // Loop over each segment; find single height cells and interleave.
  for (size_t s = 0; s < m_mgrPtr->getNumSegments(); s++) {
    DetailedSeg* segPtr = m_mgrPtr->getSegment(s);
    int segId = segPtr->getSegId();

    std::vector<Node*>& nodes = m_mgrPtr->m_cellsInSeg[segId];
    if (nodes.size() < 2) {
      continue;
    }
    std::sort(nodes.begin(), nodes.end(), CompareNodesX());

    int j = 0;
    int n = nodes.size();
    while (j < n) {
      while (j < n && m_arch->isMultiHeightCell(nodes[j])) {
        ++j;
      }
      int jstrt = j;
      while (j < n && m_arch->isSingleHeightCell(nodes[j])) {
        ++j;
      }
      int jstop = j - 1;

      // Single height cells in [jstrt,jstop].
      for (int i = jstrt; i <= jstop; i += m_windowStep) {
        int istrt = i;
        int istop = std::min(jstop, istrt + m_windowSize - 1);
        if (istop == jstop) {
          istrt = std::max(jstrt, istop - m_windowSize + 1);
        }

        if (istop - istrt + 1 <= 4) {
          // Too small.
          continue;
        }

        // Enough cells to interleave.

        Node* nextPtr = (istop != n - 1) ? nodes[istop + 1] : 0;
        rightLimit = segPtr->getMaxX();
        if (nextPtr != 0) {
          m_arch->getCellPadding(nextPtr, leftPadding, rightPadding);
          rightLimit = std::min(
              nextPtr->getX() - 0.5 * nextPtr->getWidth() - leftPadding,
              rightLimit);
        }
        Node* prevPtr = (istrt != 0) ? nodes[istrt - 1] : 0;
        leftLimit = segPtr->getMinX();
        if (prevPtr != 0) {
          m_arch->getCellPadding(prevPtr, leftPadding, rightPadding);
          leftLimit = std::max(
              prevPtr->getX() + 0.5 * prevPtr->getWidth() + rightPadding,
              leftLimit);
        }

        SmallProblem sm;
        if (build(&sm, leftLimit, rightLimit, nodes, istrt, istop) == false) {
          continue;
        }

        // Proceed to solve.  Due to spacing, it is possible that
        // interleaving will not produce a result as good as the
        // current placement.  So, we should record the cost of the
        // current placement.
        double initCost = 0.;
        {
          std::vector<EdgeInterval> intervals(sm.m_nEdges, EdgeInterval());
          for (size_t e = 0; e < sm.m_nEdges; e++) {
            intervals[e] = sm.m_netBoxes[e];
          }
          for (size_t n = 0; n < sm.m_nNodes; n++) {
            std::vector<EdgeAndOffset>& pins = sm.m_adjEdges[n];
            for (size_t p = 0; p < pins.size(); p++) {
              double offset = pins[p].m_offset;
              int e = pins[p].m_edge;
              intervals[e].add(sm.m_x[n] + offset);
            }
          }
          for (size_t e = 0; e < sm.m_nEdges; e++) {
            initCost += intervals[e].width();
          }
        }
        double nextCost = solve(&sm);

        if (nextCost >= initCost) {
          continue;
        }

        // Update placement.  Need to site align or should
        // this be okay due to how we set spacing, etc.???
        //
        // I think that because (if?) the boundaries are always
        // site aligned and the widths and paddings are all
        // multiples of site widths, then everything should be
        // okay and we can just update the cell position!!!
        for (size_t n = 0; n < m_nodeIds.size(); n++) {
          Node* nd = m_network->getNode(m_nodeIds[n]);
          int nid = m_nodeMap[nd->getId()];
          nd->setX(sm.m_x[nid]);
        }
        std::sort(nodes.begin() + jstrt,
                  nodes.begin() + (jstop + 1),
                  CompareNodesX());
      }
    }
  }
  return;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool DetailedInterleave::build(SmallProblem* sm,
                               double leftLimit,
                               double rightLimit,
                               std::vector<Node*>& nodes,
                               int jstrt,
                               int jstop)
{
  // Create a small problem for the cells indexed from [istrt,istop].

  double siteWidth = m_arch->getRow(0)->getSiteWidth();

  // Figure out node mappings and the problem size.
  ++m_traversal;

  int nNodes = 0;
  int nEdges = 0;
  // Must clear the previous Ids.
  m_nodeIds.erase(m_nodeIds.begin(), m_nodeIds.end());
  m_edgeIds.erase(m_edgeIds.begin(), m_edgeIds.end());
  for (int j = jstrt; j <= jstop; j++) {
    Node* nd = nodes[j];

    m_nodeMask[nd->getId()] = m_traversal;
    m_nodeMap[nd->getId()] = nNodes;
    ++nNodes;
    m_nodeIds.push_back(nd->getId());

    for (int pj = 0; pj < nd->getPins().size(); pj++) {
      Pin* pin = nd->getPins()[pj];
      Edge* ed = pin->getEdge();
      int npins = ed->getPins().size();
      if (npins < 2 || npins >= m_skipNetsLargerThanThis) {
        continue;
      }

      if (m_edgeMask[ed->getId()] != m_traversal) {
        m_edgeMask[ed->getId()] = m_traversal;
        m_edgeMap[ed->getId()] = nEdges;
        ++nEdges;
        m_edgeIds.push_back(ed->getId());
      }
    }
  }

  // Create the small problem.
  sm->clear();
  sm->init(nNodes, nEdges);

  // Setup up interval and cell widths, including padding and extra spacing.
  {
    double leftPadding, rightPadding, dummyPadding;

    Node* ndr = nodes[jstop];
    Node* ndl = nodes[jstrt];

    // Determine current left and right edge for the cells
    // involved.
    m_arch->getCellPadding(ndr, dummyPadding, rightPadding);
    double rightEdge = std::min(
        ndr->getX() + 0.5 * ndr->getWidth() + rightPadding, rightLimit);
    m_arch->getCellPadding(ndl, leftPadding, dummyPadding);
    double leftEdge = std::max(
        ndl->getX() - 0.5 * ndl->getWidth() - leftPadding, leftLimit);

    // Determine width and padding requirements.
    double totalPadding = 0.;
    double totalWidth = 0.;
    for (size_t n = 0; n < m_nodeIds.size(); n++) {
      Node* nd = m_network->getNode(m_nodeIds[n]);
      m_arch->getCellPadding(nd, leftPadding, rightPadding);
      totalPadding += (leftPadding + rightPadding);
      totalWidth += nd->getWidth();
    }

    // Enlarge if we do not have enough space.
    bool changed = true;
    while (changed
           && (rightEdge - leftEdge < totalWidth + totalPadding - 1.0e-3)) {
      changed = false;
      if (rightEdge + siteWidth <= rightLimit) {
        rightEdge += siteWidth;
        changed = true;
      }
      if (leftEdge - siteWidth >= leftLimit) {
        leftEdge -= siteWidth;
        changed = true;
      }
    }

    if (rightEdge - leftEdge >= totalWidth + totalPadding) {
      // Proceed with padding, and maybe a bit more space.
      double amtPerCell = ((rightEdge - leftEdge) - (totalWidth + totalPadding))
                          / (double) sm->m_nNodes;
      int sitesPerCell = std::max(1, (int) std::floor(amtPerCell / siteWidth));

      totalWidth = 0.;
      for (size_t n = 0; n < m_nodeIds.size(); n++) {
        Node* nd = m_network->getNode(m_nodeIds[n]);
        int nid = m_nodeMap[nd->getId()];
        m_arch->getCellPadding(nd, leftPadding, rightPadding);
        sm->m_widths[nid] = nd->getWidth() + leftPadding + rightPadding;

        totalWidth += sm->m_widths[nid];
      }
      for (size_t n = 0; n < m_nodeIds.size(); n++) {
        Node* nd = m_network->getNode(m_nodeIds[n]);
        int nid = m_nodeMap[nd->getId()];
        if (totalWidth + sitesPerCell * siteWidth < rightEdge - leftEdge) {
          totalWidth -= sm->m_widths[nid];
          sm->m_widths[nid] += sitesPerCell * siteWidth;
          totalWidth += sm->m_widths[nid];
        }
      }
    } else if (rightEdge - leftEdge >= totalWidth) {
      // Can proceed without padding, but maybe a bit of space.
      double amtPerCell
          = ((rightEdge - leftEdge) - (totalWidth)) / (double) sm->m_nNodes;
      int sitesPerCell = std::max(1, (int) std::floor(amtPerCell / siteWidth));

      totalWidth = 0.;
      for (size_t n = 0; n < m_nodeIds.size(); n++) {
        Node* nd = m_network->getNode(m_nodeIds[n]);
        int nid = m_nodeMap[nd->getId()];
        sm->m_widths[nid] = nd->getWidth();

        totalWidth += sm->m_widths[nid];
      }
      for (size_t n = 0; n < m_nodeIds.size(); n++) {
        Node* nd = m_network->getNode(m_nodeIds[n]);
        int nid = m_nodeMap[nd->getId()];
        if (totalWidth + sitesPerCell * siteWidth < rightEdge - leftEdge) {
          totalWidth -= sm->m_widths[nid];
          sm->m_widths[nid] += sitesPerCell * siteWidth;
          totalWidth += sm->m_widths[nid];
        }
      }
    } else {
      // Not enough space so abort.
      return false;
    }

    sm->m_xmin = leftEdge;
    sm->m_xmax = rightEdge;
  }
  // Setup initial positions.
  for (size_t n = 0; n < m_nodeIds.size(); n++) {
    Node* nd = m_network->getNode(m_nodeIds[n]);
    int nid = m_nodeMap[nd->getId()];
    sm->m_x[nid] = nd->getX();
  }
  // Setup connectivity and net boxes.
  double cost = 0.;
  for (size_t e = 0; e < m_edgeIds.size(); e++) {
    Edge* ed = m_network->getEdge(m_edgeIds[e]);
    int eid = m_edgeMap[ed->getId()];

    EdgeInterval tmp;
    for (int pi = 0; pi < ed->getPins().size(); pi++) {
      Pin* pin = ed->getPins()[pi];
      Node* nd = pin->getNode();
      double x = nd->getX() + pin->getOffsetX();
      x = std::min(std::max(x, sm->m_xmin), sm->m_xmax);
      if (m_nodeMask[nd->getId()] == m_traversal) {
        int nid = m_nodeMap[nd->getId()];
        sm->m_adjEdges[nid].push_back(EdgeAndOffset(eid, pin->getOffsetX()));
      } else {
        sm->m_netBoxes[eid].add(x);
      }
      tmp.add(x);
    }
    cost += tmp.width();
  }

  return true;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
struct SortPosDP
{
  bool operator()(const std::pair<int, double>& p,
                  const std::pair<int, double>& q) const
  {
    return p.second < q.second;
  }
};

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
double DetailedInterleave::solve(SmallProblem* problem)
{
  std::vector<int> a;
  std::vector<int> b;
  std::vector<int> q;
  a.reserve(problem->m_nNodes);
  b.reserve(problem->m_nNodes);
  q.reserve(problem->m_nNodes);

  // Ensure the cells are sorted.
  std::vector<std::pair<int, double>> pos;
  for (int i = 0; i < problem->m_nNodes; i++) {
    pos.push_back(std::pair<int, double>(i, problem->m_x[i]));
  }
  std::sort(pos.begin(), pos.end(), SortPosDP());
  for (size_t i = 0; i < pos.size(); i++) {
    q.push_back(pos[i].first);
  }

  // Build the table.
  int nEntries = (problem->m_nNodes + 1) * (problem->m_nNodes + 1);
  std::vector<TableEntry> entries(nEntries, TableEntry(problem));
  std::vector<std::vector<TableEntry*>> matrix;
  matrix.resize(problem->m_nNodes + 1);
  for (size_t i = 0; i < matrix.size(); i++) {
    matrix[i].resize(problem->m_nNodes + 1);
  }

  TableEntry* curr;
  TableEntry* prev;

  TableEntry workspace_a(problem);
  TableEntry workspace_b(problem);

  double bestCost = 0.;

  // Find the current solution.
  workspace_a.clear();
  for (size_t i = 0; i < q.size(); i++) {
    workspace_a.add(q[i]);
  }
  bestCost = workspace_a.m_cost;

  double left = problem->m_xmin;
  for (size_t n = 0; n < q.size(); n++) {
    problem->m_x[q[n]] = left + 0.5 * problem->m_widths[q[n]];
    left += problem->m_widths[q[n]];
  }

  // Perform a few improvement passes.  Take the current order of cells,
  // split it into two lists and interleave them.
  for (int pass = 1; pass <= 5; pass++) {
    a.erase(a.begin(), a.end());
    b.erase(b.begin(), b.end());
    for (int i = 0; i < problem->m_nNodes; i++) {
      if (i % 2 == 0)
        a.push_back(q[i]);
      else
        b.push_back(q[i]);
    }
    int na = a.size();
    int nb = b.size();

    {
      int k = 0;
      for (int i = 0; i <= nb; i++) {
        for (int j = 0; j <= na; j++) {
          matrix[i][j] = &(entries[k]);
          matrix[i][j]->clear();
          ++k;
        }
      }
    }

    // build first row of matrix (add entries of b)...
    {
      for (int i = 1; i <= nb; i++) {
        curr = matrix[i - 0][0];
        prev = matrix[i - 1][0];
        curr->m_parent = prev;
        curr->copy(prev);
        curr->add(b[i - 1]);
      }
    }

    // build first column of matrix (add entries of a)...
    {
      for (int j = 1; j <= na; j++) {
        curr = matrix[0][j - 0];
        prev = matrix[0][j - 1];
        curr->m_parent = prev;
        curr->copy(prev);
        curr->add(a[j - 1]);
      }
    }

    // complete the rest of the table...
    {
      for (int i = 1; i <= nb; i++) {
        for (int j = 1; j <= na; j++) {
          workspace_a.copy(matrix[i][j - 1]);  // add from list "a"...
          workspace_a.add(a[j - 1]);

          workspace_b.copy(matrix[i - 1][j]);  // add from list "b"...
          workspace_b.add(b[i - 1]);

          if (workspace_a.m_cost <= workspace_b.m_cost) {
            matrix[i][j]->m_parent = matrix[i][j - 1];
            matrix[i][j]->copy(&workspace_a);
          } else {
            matrix[i][j]->m_parent = matrix[i - 1][j];
            matrix[i][j]->copy(&workspace_b);
          }
        }
      }
    }

    if (matrix[nb][na]->m_cost >= bestCost) {
      break;
    } else {
      bestCost = matrix[nb][na]->m_cost;

      int i = nb;
      int j = na;
      curr = matrix[i][j];
      q.erase(q.begin(), q.end());
      while (curr != 0) {
        if (curr->m_parent == 0) {
          break;
        }

        if (i != 0 && curr->m_parent == matrix[i - 1][j]) {
          --i;
          q.push_back(b[i]);
        } else if (j != 0 && curr->m_parent == matrix[i][j - 1]) {
          --j;
          q.push_back(a[j]);
        } else {
          m_mgrPtr->internalError(
              "Unable to trace solution while interleaving cells");
        }

        curr = curr->m_parent;
      }
      std::reverse(q.begin(), q.end());

      left = problem->m_xmin;
      for (size_t n = 0; n < q.size(); n++) {
        problem->m_x[q[n]] = left + 0.5 * problem->m_widths[q[n]];
        left += problem->m_widths[q[n]];
      }
    }
  }

  return bestCost;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void DetailedInterleave::dp(std::vector<Node*>& nodes, double minX, double maxX)
{
  if (nodes.size() < 2) {
    return;
  }
  if (nodes.size() > 200) {
    return;
  }

  m_traversal = 0;

  m_nodeIds.reserve(m_network->getNumNodes());
  m_edgeIds.reserve(m_network->getNumEdges());

  m_edgeMask.resize(m_network->getNumEdges());
  m_nodeMask.resize(m_network->getNumNodes());
  std::fill(m_edgeMask.begin(), m_edgeMask.end(), m_traversal);
  std::fill(m_nodeMask.begin(), m_nodeMask.end(), m_traversal);

  m_edgeMap.resize(m_network->getNumEdges());
  m_nodeMap.resize(m_network->getNumNodes());
  std::fill(m_edgeMap.begin(), m_edgeMap.end(), -1);
  std::fill(m_nodeMap.begin(), m_nodeMap.end(), -1);

  double totalSpace = maxX - minX;
  double totalWidth = 0.0;
  double minWidth = 0.;
  double scaling = 0.;

  ++m_traversal;

  minWidth = std::numeric_limits<double>::max();
  for (size_t j = 0; j < nodes.size(); j++) {
    Node* nd = nodes[j];

    totalWidth += nd->getWidth();
    minWidth = std::min(minWidth, nd->getWidth());
  }

  scaling = 1.;
  if ((totalSpace - totalWidth < 0.0) || (totalSpace - totalWidth > minWidth)) {
    scaling = totalSpace / totalWidth;
  }

  // Build the small problem.
  int nNodes = 0;
  int nEdges = 0;
  // Must clear the previous Ids.
  m_nodeIds.erase(m_nodeIds.begin(), m_nodeIds.end());
  m_edgeIds.erase(m_edgeIds.begin(), m_edgeIds.end());
  for (size_t j = 0; j < nodes.size(); j++) {
    Node* nd = nodes[j];

    m_nodeMask[nd->getId()] = m_traversal;
    m_nodeMap[nd->getId()] = nNodes;
    ++nNodes;
    m_nodeIds.push_back(nd->getId());

    for (int pj = 0; pj < nd->getPins().size(); pj++) {
      Pin* pin = nd->getPins()[pj];
      Edge* ed = pin->getEdge();
      int npins = ed->getPins().size();
      if (npins < 2 || npins >= m_skipNetsLargerThanThis) {
        continue;
      }

      if (m_edgeMask[ed->getId()] != m_traversal) {
        m_edgeMask[ed->getId()] = m_traversal;
        m_edgeMap[ed->getId()] = nEdges;
        ++nEdges;
        m_edgeIds.push_back(ed->getId());
      }
    }
  }
  SmallProblem sm(nNodes, nEdges);
  sm.m_xmin = minX;
  sm.m_xmax = maxX;
  for (size_t n = 0; n < m_nodeIds.size(); n++) {
    Node* nd = m_network->getNode(m_nodeIds[n]);
    int nid = m_nodeMap[nd->getId()];

    sm.m_widths[nid] = nd->getWidth() * scaling;
    sm.m_x[nid] = nd->getX();
  }
  double currCost = 0.;
  for (size_t e = 0; e < m_edgeIds.size(); e++) {
    Edge* ed = m_network->getEdge(m_edgeIds[e]);
    int eid = m_edgeMap[ed->getId()];

    EdgeInterval tmp;
    for (int pi = 0; pi < ed->getPins().size(); pi++) {
      Pin* pin = ed->getPins()[pi];
      Node* nd = pin->getNode();
      double x = std::min(std::max(nd->getX() + pin->getOffsetX(), minX), maxX);
      if (m_nodeMask[nd->getId()] == m_traversal) {
        int nid = m_nodeMap[nd->getId()];
        sm.m_adjEdges[nid].push_back(EdgeAndOffset(eid, pin->getOffsetX()));
      } else {
        sm.m_netBoxes[eid].add(x);
      }
      tmp.add(x);
    }
    currCost += tmp.width();
  }

  // Solve the interleaving.  Could be that the returned solution is
  // not, in fact, better; e.g., due to spacing.
  double bestCost = solve(&sm);
  if (1 || bestCost < currCost) {
    for (size_t n = 0; n < m_nodeIds.size(); n++) {
      Node* nd = m_network->getNode(m_nodeIds[n]);
      int nid = m_nodeMap[nd->getId()];
      nd->setX(sm.m_x[nid]);
    }
  }
}

}  // namespace dpo
