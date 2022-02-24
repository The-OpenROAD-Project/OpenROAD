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
//
// Description:
// Essentially a zero temperature annealer that can use a variety of
// move generators, different objectives and a cost function in order
// to improve a placement.

#include <stdio.h>
#include <stdlib.h>
#include <algorithm>
#include <boost/format.hpp>
#include <boost/tokenizer.hpp>
#include <cmath>
#include <iostream>
#include <stack>
#include <utility>
#include "plotgnu.h"
#include "utility.h"
#include "utl/Logger.h"
// For detailed improvement.
#include "detailed_manager.h"
#include "detailed_orient.h"
#include "detailed_random.h"
#include "detailed_segment.h"
// Detailed placement objectives.
#include "detailed_abu.h"
#include "detailed_displacement.h"
#include "detailed_global.h"
#include "detailed_hpwl.h"
#include "detailed_objective.h"
#include "detailed_vertical.h"

#include "utility.h"

using utl::DPO;

////////////////////////////////////////////////////////////////////////////////
// Defines.
////////////////////////////////////////////////////////////////////////////////

const int MAX_MOVE_ATTEMPTS = 5;

namespace dpo {

bool DetailedRandom::isOperator(char ch) {
  if (ch == '+' || ch == '-' || ch == '*' || ch == '/' || ch == '^')
    return true;
  return false;
}

bool DetailedRandom::isObjective(char ch) {
  if (ch >= 'a' && ch <= 'z') return true;
  return false;
}

bool DetailedRandom::isNumber(char ch) {
  if (ch >= '0' && ch <= '9') return true;
  return false;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
DetailedRandom::DetailedRandom(Architecture* arch, Network* network,
                               RoutingParams* rt)
  : m_mgrPtr(nullptr),
    m_arch(arch),
    m_network(network),
    m_rt(rt),
    m_movesPerCandidate(3.0)
{
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
DetailedRandom::~DetailedRandom() {}

//////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////
void DetailedRandom::run(DetailedMgr* mgrPtr, std::string command) {
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
void DetailedRandom::run(DetailedMgr* mgrPtr, std::vector<std::string>& args) {
  // This is, more or less, a greedy or low temperature anneal.  It is capable
  // of handling very complex objectives, etc.  There should be a lot of
  // arguments provided actually.  But, right now, I am just getting started.

  m_mgrPtr = mgrPtr;

  std::string generatorStr = "";
  std::string objectiveStr = "";
  std::string costStr = "";
  m_movesPerCandidate = 3.0;
  int passes = 1;
  double tol = 0.01;
  for (size_t i = 1; i < args.size(); i++) {
    if (args[i] == "-f" && i + 1 < args.size()) {
      m_movesPerCandidate = std::atof(args[++i].c_str());
    } else if (args[i] == "-p" && i + 1 < args.size()) {
      passes = std::atoi(args[++i].c_str());
    } else if (args[i] == "-t" && i + 1 < args.size()) {
      tol = std::atof(args[++i].c_str());
    } else if (args[i] == "-gen" && i + 1 < args.size()) {
      generatorStr = args[++i];
    } else if (args[i] == "-obj" && i + 1 < args.size()) {
      objectiveStr = args[++i];
    } else if (args[i] == "-cost" && i + 1 < args.size()) {
      costStr = args[++i];
    }
  }
  tol = std::max(tol, 0.01);
  passes = std::max(passes, 1);

  // Generators.
  for (size_t i = 0; i < m_generators.size(); i++) {
    delete m_generators[i];
  }
  m_generators.clear();

  // Additional generators per the command. XXX: Need to write the code for
  // these objects; just a concept now.
  if (generatorStr != "") {
    boost::char_separator<char> separators(" \r\t\n:");
    boost::tokenizer<boost::char_separator<char> > tokens(generatorStr,
                                                          separators);
    std::vector<std::string> gens;
    for (boost::tokenizer<boost::char_separator<char> >::iterator it =
             tokens.begin();
         it != tokens.end(); it++) {
      gens.push_back(*it);
    }

    for (size_t i = 0; i < gens.size(); i++) {
      // if( gens[i] == "ro" )       std::cout << "reorder generator requested."
      // << std::endl; else if( gens[i] == "mis" ) std::cout << "set matching
      // generator requested." << std::endl;
      if (gens[i] == "gs") {
        m_generators.push_back(new DetailedGlobalSwap());
      } else if (gens[i] == "vs") {
        m_generators.push_back(new DetailedVerticalSwap());
      } else if (gens[i] == "rng") {
        m_generators.push_back(new RandomGenerator());
      } else if (gens[i] == "disp") {
        m_generators.push_back(new DisplacementGenerator());
      } else {
        ;
      }
    }
  }
  if (m_generators.size() == 0) {
    // Default generator.
    m_generators.push_back(new RandomGenerator());
  }
  for (size_t i = 0; i < m_generators.size(); i++) {
    m_generators[i]->init(m_mgrPtr);

    m_mgrPtr->getLogger()->info(DPO, 324,
                                "Random improver is using {:s} generator.",
                                m_generators[i]->getName().c_str());
  }

  // Objectives.
  for (size_t i = 0; i < m_objectives.size(); i++) {
    delete m_objectives[i];
  }
  m_objectives.clear();

  // Additional objectives per the command. XXX: Need to write the code for
  // these objects; just a concept now.
  if (objectiveStr != "") {
    boost::char_separator<char> separators(" \r\t\n:");
    boost::tokenizer<boost::char_separator<char> > tokens(objectiveStr,
                                                          separators);
    std::vector<std::string> objs;
    for (boost::tokenizer<boost::char_separator<char> >::iterator it =
             tokens.begin();
         it != tokens.end(); it++) {
      objs.push_back(*it);
    }

    for (size_t i = 0; i < objs.size(); i++) {
      // else if( objs[i] == "drc" )   std::cout << "drc objective requested."
      // << std::endl;
      if (objs[i] == "abu") {
        DetailedABU* objABU = new DetailedABU(m_arch, m_network, m_rt);
        objABU->init(m_mgrPtr, NULL);
        m_objectives.push_back(objABU);
      } else if (objs[i] == "disp") {
        DetailedDisplacement* objDisp =
            new DetailedDisplacement(m_arch, m_network, m_rt);
        objDisp->init(m_mgrPtr, NULL);
        m_objectives.push_back(objDisp);
      } else if (objs[i] == "hpwl") {
        DetailedHPWL* objHpwl = new DetailedHPWL(m_arch, m_network, m_rt);
        objHpwl->init(m_mgrPtr, NULL);
        m_objectives.push_back(objHpwl);
      } else {
        ;
      }
    }
  }
  if (m_objectives.size() == 0) {
    // Default objective.
    DetailedHPWL* objHpwl = new DetailedHPWL(m_arch, m_network, m_rt);
    objHpwl->init(m_mgrPtr, NULL);
    m_objectives.push_back(objHpwl);
  }

  for (size_t i = 0; i < m_objectives.size(); i++) {
    m_mgrPtr->getLogger()->info(DPO, 325,
                                "Random improver is using {:s} objective.",
                                m_objectives[i]->getName().c_str());
  }

  // Should I just be figuring out the objectives needed from the cost string?
  if (costStr != "") {
    // Replace substrings of objectives with a number.
    for (size_t i = m_objectives.size(); i > 0;) {
      --i;
      for (;;) {
        size_t pos = costStr.find(m_objectives[i]->getName());
        if (pos == std::string::npos) {
          break;
        }
        std::string val;
        val.append(1, (char)('a' + i));
        costStr.replace(pos, m_objectives[i]->getName().length(), val);
      }
    }

    m_mgrPtr->getLogger()->info(
        DPO, 326, "Random improver cost string is {:s}.", costStr.c_str());

    m_expr.clear();
    for (std::string::iterator it = costStr.begin(); it != costStr.end();
         ++it) {
      if (*it == '(' || *it == ')') {
      } else if (isOperator(*it) || isObjective(*it)) {
        m_expr.push_back(std::string(1, *it));
      } else {
        std::string val;
        while (!isOperator(*it) && !isObjective(*it) && it != costStr.end() &&
               *it != '(' && *it != ')') {
          val.append(1, *it);
          ++it;
        }
        m_expr.push_back(val);
        --it;
      }
    }
  } else {
    m_expr.clear();
    m_expr.push_back(std::string(1, 'a'));
    for (size_t i = 1; i < m_objectives.size(); i++) {
      m_expr.push_back(std::string(1, (char)('a' + i)));
      m_expr.push_back(std::string(1, '+'));
    }
  }


  m_currCost.resize(m_objectives.size());
  for (size_t i = 0; i < m_objectives.size(); i++) {
    m_currCost[i] = m_objectives[i]->curr();
  }
  double iCost = eval(m_currCost, m_expr);


  for (int p = 1; p <= passes; p++) {
    m_mgrPtr->resortSegments();  // Needed?
    double change = go();
    m_mgrPtr->getLogger()->info(
        DPO, 327,
        "Pass {:3d} of random improver; improvement in cost is {:.2f} percent.",
        p, (change * 100));
    if (change < tol) {
      break;
    }
  }
  m_mgrPtr->resortSegments();  // Needed?

  m_currCost.resize(m_objectives.size());
  for (size_t i = 0; i < m_objectives.size(); i++) {
    m_currCost[i] = m_objectives[i]->curr();
  }
  double fCost = eval(m_currCost, m_expr);

  double imp = (((iCost - fCost) / iCost) * 100.);
  m_mgrPtr->getLogger()->info(
      DPO, 328, "End of random improver; improvement is {:.6f} percent.",
      imp);

  // Cleanup.
  for (size_t i = 0; i < m_generators.size(); i++) {
    delete m_generators[i];
  }
  m_generators.clear();
  for (size_t i = 0; i < m_objectives.size(); i++) {
    delete m_objectives[i];
  }
  m_objectives.clear();
}

double DetailedRandom::doOperation(double a, double b, char op) {
  switch (op) {
    case '+':
      return b + a;
      break;
    case '-':
      return b - a;
      break;
    case '*':
      return b * a;
      break;
    case '/':
      return b / a;
      break;
    case '^':
      return std::pow(b, a);
      break;
    default:
      break;
  }
  return 0.0;
}

double DetailedRandom::eval(std::vector<double>& costs, std::vector<std::string>& expr) {
  std::stack<double> stk;
  for (size_t i = 0; i < expr.size(); i++) {
    std::string& val = expr[i];
    if (isOperator(val[0])) {
      double a = stk.top();
      stk.pop();
      double b = stk.top();
      stk.pop();
      stk.push(doOperation(a, b, val[0]));
    } else if (isObjective(val[0])) {
      stk.push(costs[(int)(val[0] - 'a')]);
    } else {
      // Assume number.
      stk.push(std::stod(val));
    }
  }
  if (stk.size() != 1) {
    // Cost function should never be negative.  If we have a problem,
    // then return some negative value and we can catch this error.
    return -1.0;
  }
  return stk.top();
}

//////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////
double DetailedRandom::go() {
  if (m_generators.size() == 0) {
    m_mgrPtr->getLogger()->info(
        DPO, 329, "Random improver requires at least one generator.");
    return 0.0;
  }

  // Collect candidate cells.
  collectCandidates();

  // Try to improve.
  int maxAttempts =
      (int)std::ceil(m_movesPerCandidate * (double)m_candidates.size());
  Utility::random_shuffle(m_candidates.begin(), m_candidates.end(),
                          m_mgrPtr->m_rng);

  m_deltaCost.resize(m_objectives.size());
  m_initCost.resize(m_objectives.size());
  m_currCost.resize(m_objectives.size());
  m_nextCost.resize(m_objectives.size());
  for (size_t i = 0; i < m_objectives.size(); i++) {
    m_deltaCost[i] = 0.;
    m_initCost[i] = m_objectives[i]->curr();
    m_currCost[i] = m_initCost[i];
    m_nextCost[i] = m_initCost[i];

    if (m_objectives[i]->getName() == "abu") {
      DetailedABU* ptr = dynamic_cast<DetailedABU*>(m_objectives[i]);
      if (ptr != 0) {
        ptr->measureABU(true);
      }
    }
  }

  // Test.
  if (eval(m_currCost, m_expr) < 0.0) {
    m_mgrPtr->getLogger()->info(DPO, 330,
                                "Test objective function failed, possibly due "
                                "to a badly formed cost function.");
    return 0.0;
  }

  double currTotalCost;
  double initTotalCost;
  double nextTotalCost;
  initTotalCost = eval(m_currCost, m_expr);
  currTotalCost = initTotalCost;
  nextTotalCost = initTotalCost;

  std::vector<int> gen_count(m_generators.size());
  std::fill(gen_count.begin(), gen_count.end(), 0);
  for (int attempt = 0; attempt < maxAttempts; attempt++) {
    // Pick a generator at random.
    int g = (int)((*(m_mgrPtr->m_rng))() % (m_generators.size()));
    ++gen_count[g];
    // Generate a move list.
    if (m_generators[g]->generate(m_mgrPtr, m_candidates) == false) {
      // Failed to generate anything so just move on to the next attempt.
      continue;
    }

    // The generator has provided a successful move which is stored in the
    // manager.  We need to evaluate that move to see if we should accept
    // or reject it.  Scan over the objective functions and use the move
    // information to compute the weighted deltas; an overall weighted delta
    // better than zero implies improvement.
    for (size_t i = 0; i < m_objectives.size(); i++) {
      // XXX: NEED TO WEIGHT EACH OBJECTIVE!
      double change = m_objectives[i]->delta(
          m_mgrPtr->m_nMoved, m_mgrPtr->m_movedNodes,
          m_mgrPtr->m_curLeft, m_mgrPtr->m_curBottom, 
          m_mgrPtr->m_curOri, 
          m_mgrPtr->m_newLeft, m_mgrPtr->m_newBottom, 
          m_mgrPtr->m_newOri);

      m_deltaCost[i] = change;
      m_nextCost[i] = m_currCost[i] - m_deltaCost[i];  // -delta is +ve is less.
    }
    nextTotalCost = eval(m_nextCost, m_expr);

    //        std::cout << boost::format( "Move consisting of %d cells generated
    //        benefit of %.2lf; Will %s.\n" )
    //            % m_mgrPtr->m_nMoved % delta % ((delta>0.)?"accept":"reject");

    //        if( delta > 0.0 )
    if (nextTotalCost <= currTotalCost) {
      m_mgrPtr->acceptMove();
      for (size_t i = 0; i < m_objectives.size(); i++) {
        m_objectives[i]->accept();
      }

      // A great, but time-consuming, check here is to recompute the costs from
      // scratch and make sure they are the same as the incrementally computed
      // costs.  Very useful for debugging!  Could do this check ever so often
      // or just at the end...
      ;
      for (size_t i = 0; i < m_objectives.size(); i++) {
        m_currCost[i] = m_nextCost[i];
      }
      currTotalCost = nextTotalCost;
    } else {
      m_mgrPtr->rejectMove();
      for (size_t i = 0; i < m_objectives.size(); i++) {
        m_objectives[i]->reject();
      }
    }
  }
  for (size_t i = 0; i < gen_count.size(); i++) {
    m_mgrPtr->getLogger()->info(
        DPO, 332,
        "End of pass, Generator {:s} called {:d} times.",
        m_generators[i]->getName().c_str(), gen_count[i]);
  }
  for (size_t i = 0; i < m_generators.size(); i++) {
    m_generators[i]->stats();
  }

  for (size_t i = 0; i < m_objectives.size(); i++) {
    double scratch = m_objectives[i]->curr();
    m_nextCost[i] = scratch;  // Temporary.
    bool error = (std::fabs(scratch - m_currCost[i]) > 1.0e-3);
    m_mgrPtr->getLogger()->info(
        DPO, 333,
        "End of pass, Objective {:s}, Initial cost {:.6e}, Scratch cost "
        "{:.6e}, Incremental cost {:.6e}, Mismatch? {:c}",
        m_objectives[i]->getName().c_str(), m_initCost[i], scratch,
        m_currCost[i], ((error) ? 'Y' : 'N'));

    if (m_objectives[i]->getName() == "abu") {
      DetailedABU* ptr = dynamic_cast<DetailedABU*>(m_objectives[i]);
      if (ptr != 0) {
        ptr->measureABU(true);
      }
    }
  }
  nextTotalCost = eval(m_nextCost, m_expr);
  m_mgrPtr->getLogger()->info( DPO, 338, "End of pass, Total cost is {:.6e}.", nextTotalCost );

  return ((initTotalCost - currTotalCost) / initTotalCost);
}

//////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////
void DetailedRandom::collectCandidates() {
  m_candidates.erase(m_candidates.begin(), m_candidates.end());
  m_candidates.insert(m_candidates.end(), m_mgrPtr->m_singleHeightCells.begin(),
                      m_mgrPtr->m_singleHeightCells.end());
  for (size_t i = 2; i < m_mgrPtr->m_multiHeightCells.size(); i++) {
    m_candidates.insert(m_candidates.end(),
                        m_mgrPtr->m_multiHeightCells[i].begin(),
                        m_mgrPtr->m_multiHeightCells[i].end());
  }
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
RandomGenerator::RandomGenerator()
  : DetailedGenerator("displacement"),
    m_mgr(nullptr),
    m_arch(nullptr),
    m_network(nullptr),
    m_rt(nullptr),
    m_attempts(0),
    m_moves(0),
    m_swaps(0)
{
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
RandomGenerator::~RandomGenerator() {}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool RandomGenerator::generate(DetailedMgr* mgr,
                               std::vector<Node*>& candidates) {
  ++m_attempts;

  m_mgr = mgr;
  m_arch = mgr->getArchitecture();
  m_network = mgr->getNetwork();
  m_rt = mgr->getRoutingParams();

  double ywid = m_mgr->getSingleRowHeight();
  int ydim = m_mgr->getNumSingleHeightRows();
  double xwid = m_arch->getRow(0)->getSiteSpacing();
  int xdim = std::max(0, (int)((m_arch->getMaxX() - m_arch->getMinX()) / xwid));

  xwid = (m_arch->getMaxX() - m_arch->getMinX()) / (double)xdim;
  ywid = (m_arch->getMaxY() - m_arch->getMinY()) / (double)ydim;

  Node* ndi = candidates[(*(m_mgr->m_rng))() % (candidates.size())];
  int spanned_i = m_arch->getCellHeightInRows(ndi);
  if (spanned_i != 1) {
    return false;
  }
  // Segments for the source.
  std::vector<DetailedSeg*>& segs_i = m_mgr->m_reverseCellToSegs[ndi->getId()];
  // Only working with single height cells right now.
  if (segs_i.size() != 1) {
    std::cout << "Error." << std::endl;
    exit(-1);
  }

  double xi, yi;
  double xj, yj;
  int si;      // Row and segment of source.
  int rj, sj;  // Row and segment of destination.
  int grid_xi, grid_yi;
  int grid_xj, grid_yj;
  // For the window size.  This should be parameterized.
  int rly = 10;
  int rlx = 10;
  int rel_x, rel_y;

  const int tries = 5;
  for (int t = 1; t <= tries; t++) {
    // Position of the source.
    yi = ndi->getBottom()+0.5*ndi->getHeight();
    xi = ndi->getLeft()+0.5*ndi->getWidth();

    // Segment for the source.
    si = segs_i[0]->getSegId();

    // Random position within a box centered about (xi,yi).
    grid_xi =
        std::min(xdim - 1, std::max(0, (int)((xi - m_arch->getMinX()) / xwid)));
    grid_yi =
        std::min(ydim - 1, std::max(0, (int)((yi - m_arch->getMinY()) / ywid)));

    rel_x = (*(m_mgr->m_rng))() % (2 * rlx + 1);
    rel_y = (*(m_mgr->m_rng))() % (2 * rly + 1);

    grid_xj = std::min(xdim - 1, std::max(0, (grid_xi - rlx + rel_x)));
    grid_yj = std::min(ydim - 1, std::max(0, (grid_yi - rly + rel_y)));

    // Position of the destination.
    xj = m_arch->getMinX() + grid_xj * xwid;
    yj = m_arch->getMinY() + grid_yj * ywid;

    // Row and segment for the destination.
    rj = (int)((yj - m_arch->getMinY()) / m_mgr->getSingleRowHeight());
    rj = std::min(m_mgr->getNumSingleHeightRows() - 1, std::max(0, rj));
    yj = m_arch->getRow(rj)->getBottom();
    sj = -1;
    for (int s = 0; s < m_mgr->m_segsInRow[rj].size(); s++) {
      DetailedSeg* segPtr = m_mgr->m_segsInRow[rj][s];
      if (xj >= segPtr->getMinX() && xj <= segPtr->getMaxX()) {
        sj = segPtr->getSegId();
        break;
      }
    }

    // Need to determine validity of things.
    if (sj == -1 || ndi->getRegionId() != m_mgr->m_segments[sj]->getRegId()) {
      // The target segment cannot support the candidate cell.
      continue;
    }

    if (m_mgr->tryMove(ndi, ndi->getLeft(), ndi->getBottom(), si,
                       (int)std::round(xj), (int)std::round(yj), sj)) {
      ++m_moves;
      return true;
    }
    if (m_mgr->trySwap(ndi, ndi->getLeft(), ndi->getBottom(), si,
                       (int)std::round(xj), (int)std::round(yj), sj)) {
      ++m_swaps;
      return true;
    }
  }
  return false;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void RandomGenerator::stats() {
  m_mgr->getLogger()->info( DPO, 335, "Generator {:s}, "
    "Cumulative attempts {:d}, swaps {:d}, moves {:5d} since last reset.",
    getName().c_str(), m_attempts, m_swaps, m_moves );
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
DisplacementGenerator::DisplacementGenerator()
  : DetailedGenerator("random"),
    m_mgr(nullptr),
    m_arch(nullptr),
    m_network(nullptr),
    m_rt(nullptr),
    m_attempts(0),
    m_moves(0),
    m_swaps(0)
{
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
DisplacementGenerator::~DisplacementGenerator() {}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool DisplacementGenerator::generate(DetailedMgr* mgr,
                                     std::vector<Node*>& candidates) {
  ++m_attempts;

  m_mgr = mgr;
  m_arch = mgr->getArchitecture();
  m_network = mgr->getNetwork();
  m_rt = mgr->getRoutingParams();

  double ywid = m_mgr->getSingleRowHeight();
  int ydim = m_mgr->getNumSingleHeightRows();
  double xwid = m_arch->getRow(0)->getSiteSpacing();
  int xdim = std::max(0, (int)((m_arch->getMaxX() - m_arch->getMinX()) / xwid));

  xwid = (m_arch->getMaxX() - m_arch->getMinX()) / (double)xdim;
  ywid = (m_arch->getMaxY() - m_arch->getMinY()) / (double)ydim;

  Node* ndi = candidates[(*(m_mgr->m_rng))() % (candidates.size())];

  // Segments for the source.
  std::vector<DetailedSeg*>& segs_i = m_mgr->m_reverseCellToSegs[ndi->getId()];

  double xj, yj;
  int si;      // Row and segment of source.
  int rj, sj;  // Row and segment of destination.
  int grid_xi, grid_yi;
  int grid_xj, grid_yj;
  // For the window size.  This should be parameterized.
  int rly = 5;
  int rlx = 5;
  int rel_x, rel_y;
  std::vector<Node*>::iterator it_j;

  const int tries = 5;
  for (int t = 1; t <= tries; t++) {
    // Position of the source.
    //yi = ndi->getBottom()+0.5*ndi->getHeight();
    //xi = ndi->getLeft()+0.5*ndi->getWidth();

    // Segment for the source.
    si = segs_i[0]->getSegId();

    // Choices: (i) random position within a box centered at the original
    // position; (ii) random position within a box between the current
    // and original position; (iii) the original position itself.  Should
    // this also be a randomized choice??????????????????????????????????
    if (1) {
      // Centered at the original position within a box.
      double orig_yc = ndi->getOrigBottom()+0.5*ndi->getHeight();
      double orig_xc = ndi->getOrigLeft()+0.5*ndi->getWidth();

      grid_xi = std::min(
          xdim - 1,
          std::max(0, (int)((orig_xc - m_arch->getMinX()) / xwid)));
      grid_yi = std::min(
          ydim - 1,
          std::max(0, (int)((orig_yc - m_arch->getMinY()) / ywid)));

      rel_x = (*(m_mgr->m_rng))() % (2 * rlx + 1);
      rel_y = (*(m_mgr->m_rng))() % (2 * rly + 1);

      grid_xj = std::min(xdim - 1, std::max(0, (grid_xi - rlx + rel_x)));
      grid_yj = std::min(ydim - 1, std::max(0, (grid_yi - rly + rel_y)));

      xj = m_arch->getMinX() + grid_xj * xwid;
      yj = m_arch->getMinY() + grid_yj * ywid;
    }
    if (0) {
      // The original position.
      xj = ndi->getOrigLeft() + 0.5*ndi->getWidth();
      yj = ndi->getOrigBottom() + 0.5*ndi->getHeight();
    }
    if (0) {
      // Somewhere between current position and original position.
      double orig_yc = ndi->getOrigBottom()+0.5*ndi->getHeight();
      double orig_xc = ndi->getOrigLeft()+0.5*ndi->getWidth();

      double curr_yc = ndi->getBottom()+0.5*ndi->getHeight();
      double curr_xc = ndi->getLeft()+0.5*ndi->getWidth();

      grid_xi = std::min(
          xdim - 1,
          std::max(0, (int)((curr_xc - m_arch->getMinX()) / xwid)));
      grid_yi = std::min(
          ydim - 1,
          std::max(0, (int)((curr_yc - m_arch->getMinY()) / ywid)));

      grid_xj = std::min(
          xdim - 1,
          std::max(0, (int)((orig_xc - m_arch->getMinX()) / xwid)));
      grid_yj = std::min(
          ydim - 1,
          std::max(0, (int)((orig_yc - m_arch->getMinY()) / ywid)));

      if (grid_xi > grid_xj) std::swap(grid_xi, grid_xj);
      if (grid_yi > grid_yj) std::swap(grid_yi, grid_yj);

      int w = grid_xj - grid_xi;
      int h = grid_yj - grid_yi;

      rel_x = (*(m_mgr->m_rng))() % (w + 1);
      rel_y = (*(m_mgr->m_rng))() % (h + 1);

      grid_xj = std::min(xdim - 1, std::max(0, (grid_xi + rel_x)));
      grid_yj = std::min(ydim - 1, std::max(0, (grid_yi + rel_y)));

      xj = m_arch->getMinX() + grid_xj * xwid;
      yj = m_arch->getMinY() + grid_yj * ywid;
    }

    // Row and segment for the destination.
    rj = (int)((yj - m_arch->getMinY()) / m_mgr->getSingleRowHeight());
    rj = std::min(m_mgr->getNumSingleHeightRows() - 1, std::max(0, rj));
    yj = m_arch->getRow(rj)->getBottom();
    sj = -1;
    for (int s = 0; s < m_mgr->m_segsInRow[rj].size(); s++) {
      DetailedSeg* segPtr = m_mgr->m_segsInRow[rj][s];
      if (xj >= segPtr->getMinX() && xj <= segPtr->getMaxX()) {
        sj = segPtr->getSegId();
        break;
      }
    }

    // Need to determine validity of things.
    if (sj == -1 || ndi->getRegionId() != m_mgr->m_segments[sj]->getRegId()) {
      // The target segment cannot support the candidate cell.
      continue;
    }

    if (m_mgr->tryMove(ndi, 
                       ndi->getLeft(), ndi->getBottom(), si, 
                       (int)std::round(xj), (int)std::round(yj), sj)) {
      ++m_moves;
      return true;
    }
    if (m_mgr->trySwap(ndi, 
                       ndi->getLeft(), ndi->getBottom(), si,
                       (int)std::round(xj), (int)std::round(yj), sj)) {
      ++m_swaps;
      return true;
    }
  }
  return false;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void DisplacementGenerator::stats() {
  m_mgr->getLogger()->info( DPO, 337, "Generator {:s}, "
    "Cumulative attempts {:d}, swaps {:d}, moves {:5d} since last reset.",
    getName().c_str(), m_attempts, m_swaps, m_moves );
}

}  // namespace dpo
