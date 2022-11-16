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

#include <boost/tokenizer.hpp>
#include <stack>

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

namespace dpo {

bool DetailedRandom::isOperator(char ch)
{
  if (ch == '+' || ch == '-' || ch == '*' || ch == '/' || ch == '^')
    return true;
  return false;
}

bool DetailedRandom::isObjective(char ch)
{
  if (ch >= 'a' && ch <= 'z')
    return true;
  return false;
}

bool DetailedRandom::isNumber(char ch)
{
  if (ch >= '0' && ch <= '9')
    return true;
  return false;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
DetailedRandom::DetailedRandom(Architecture* arch, Network* network)
    : mgrPtr_(nullptr), arch_(arch), network_(network), movesPerCandidate_(3.0)
{
}

//////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////
void DetailedRandom::run(DetailedMgr* mgrPtr, std::string command)
{
  // A temporary interface to allow for a string which we will decode to create
  // the arguments.
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
void DetailedRandom::run(DetailedMgr* mgrPtr, std::vector<std::string>& args)
{
  // This is, more or less, a greedy or low temperature anneal.  It is capable
  // of handling very complex objectives, etc.  There should be a lot of
  // arguments provided actually.  But, right now, I am just getting started.

  mgrPtr_ = mgrPtr;

  std::string generatorStr = "";
  std::string objectiveStr = "";
  std::string costStr = "";
  movesPerCandidate_ = 3.0;
  int passes = 1;
  double tol = 0.01;
  for (size_t i = 1; i < args.size(); i++) {
    if (args[i] == "-f" && i + 1 < args.size()) {
      movesPerCandidate_ = std::atof(args[++i].c_str());
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
  for (size_t i = 0; i < generators_.size(); i++) {
    delete generators_[i];
  }
  generators_.clear();

  // Additional generators per the command. XXX: Need to write the code for
  // these objects; just a concept now.
  if (generatorStr != "") {
    boost::char_separator<char> separators(" \r\t\n:");
    boost::tokenizer<boost::char_separator<char>> tokens(generatorStr,
                                                         separators);
    std::vector<std::string> gens;
    for (boost::tokenizer<boost::char_separator<char>>::iterator it
         = tokens.begin();
         it != tokens.end();
         it++) {
      gens.push_back(*it);
    }

    for (size_t i = 0; i < gens.size(); i++) {
      // if( gens[i] == "ro" )       std::cout << "reorder generator requested."
      // << std::endl; else if( gens[i] == "mis" ) std::cout << "set matching
      // generator requested." << std::endl;
      if (gens[i] == "gs") {
        generators_.push_back(new DetailedGlobalSwap());
      } else if (gens[i] == "vs") {
        generators_.push_back(new DetailedVerticalSwap());
      } else if (gens[i] == "rng") {
        generators_.push_back(new RandomGenerator());
      } else if (gens[i] == "disp") {
        generators_.push_back(new DisplacementGenerator());
      } else {
        ;
      }
    }
  }
  if (generators_.size() == 0) {
    // Default generator.
    generators_.push_back(new RandomGenerator());
  }
  for (size_t i = 0; i < generators_.size(); i++) {
    generators_[i]->init(mgrPtr_);

    mgrPtr_->getLogger()->info(DPO,
                               324,
                               "Random improver is using {:s} generator.",
                               generators_[i]->getName().c_str());
  }

  // Objectives.
  for (size_t i = 0; i < objectives_.size(); i++) {
    delete objectives_[i];
  }
  objectives_.clear();

  // Additional objectives per the command. XXX: Need to write the code for
  // these objects; just a concept now.
  if (objectiveStr != "") {
    boost::char_separator<char> separators(" \r\t\n:");
    boost::tokenizer<boost::char_separator<char>> tokens(objectiveStr,
                                                         separators);
    std::vector<std::string> objs;
    for (boost::tokenizer<boost::char_separator<char>>::iterator it
         = tokens.begin();
         it != tokens.end();
         it++) {
      objs.push_back(*it);
    }

    for (size_t i = 0; i < objs.size(); i++) {
      // else if( objs[i] == "drc" )   std::cout << "drc objective requested."
      // << std::endl;
      if (objs[i] == "abu") {
        auto objABU = new DetailedABU(arch_, network_);
        objABU->init(mgrPtr_, nullptr);
        objectives_.push_back(objABU);
      } else if (objs[i] == "disp") {
        auto objDisp = new DetailedDisplacement(arch_);
        objDisp->init(mgrPtr_, nullptr);
        objectives_.push_back(objDisp);
      } else if (objs[i] == "hpwl") {
        auto objHpwl = new DetailedHPWL(network_);
        objHpwl->init(mgrPtr_, nullptr);
        objectives_.push_back(objHpwl);
      } else {
        ;
      }
    }
  }
  if (objectives_.size() == 0) {
    // Default objective.
    auto objHpwl = new DetailedHPWL(network_);
    objHpwl->init(mgrPtr_, nullptr);
    objectives_.push_back(objHpwl);
  }

  for (size_t i = 0; i < objectives_.size(); i++) {
    mgrPtr_->getLogger()->info(DPO,
                               325,
                               "Random improver is using {:s} objective.",
                               objectives_[i]->getName().c_str());
  }

  // Should I just be figuring out the objectives needed from the cost string?
  if (costStr != "") {
    // Replace substrings of objectives with a number.
    for (size_t i = objectives_.size(); i > 0;) {
      --i;
      for (;;) {
        size_t pos = costStr.find(objectives_[i]->getName());
        if (pos == std::string::npos) {
          break;
        }
        std::string val;
        val.append(1, (char) ('a' + i));
        costStr.replace(pos, objectives_[i]->getName().length(), val);
      }
    }

    mgrPtr_->getLogger()->info(
        DPO, 326, "Random improver cost string is {:s}.", costStr.c_str());

    expr_.clear();
    for (std::string::iterator it = costStr.begin(); it != costStr.end();
         ++it) {
      if (*it == '(' || *it == ')') {
      } else if (isOperator(*it) || isObjective(*it)) {
        expr_.emplace_back(std::string(1, *it));
      } else {
        std::string val;
        while (!isOperator(*it) && !isObjective(*it) && it != costStr.end()
               && *it != '(' && *it != ')') {
          val.append(1, *it);
          ++it;
        }
        expr_.push_back(val);
        --it;
      }
    }
  } else {
    expr_.clear();
    expr_.emplace_back(std::string(1, 'a'));
    for (size_t i = 1; i < objectives_.size(); i++) {
      expr_.emplace_back(std::string(1, (char) ('a' + i)));
      expr_.emplace_back(std::string(1, '+'));
    }
  }

  currCost_.resize(objectives_.size());
  for (size_t i = 0; i < objectives_.size(); i++) {
    currCost_[i] = objectives_[i]->curr();
  }
  double iCost = eval(currCost_, expr_);

  for (int p = 1; p <= passes; p++) {
    mgrPtr_->resortSegments();  // Needed?
    double change = go();
    mgrPtr_->getLogger()->info(
        DPO,
        327,
        "Pass {:3d} of random improver; improvement in cost is {:.2f} percent.",
        p,
        (change * 100));
    if (change < tol) {
      break;
    }
  }
  mgrPtr_->resortSegments();  // Needed?

  currCost_.resize(objectives_.size());
  for (size_t i = 0; i < objectives_.size(); i++) {
    currCost_[i] = objectives_[i]->curr();
  }
  double fCost = eval(currCost_, expr_);

  double imp = (((iCost - fCost) / iCost) * 100.);
  mgrPtr_->getLogger()->info(
      DPO, 328, "End of random improver; improvement is {:.6f} percent.", imp);

  // Cleanup.
  for (size_t i = 0; i < generators_.size(); i++) {
    delete generators_[i];
  }
  generators_.clear();
  for (size_t i = 0; i < objectives_.size(); i++) {
    delete objectives_[i];
  }
  objectives_.clear();
}

double DetailedRandom::doOperation(double a, double b, char op)
{
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

double DetailedRandom::eval(std::vector<double>& costs,
                            std::vector<std::string>& expr)
{
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
      stk.push(costs[(int) (val[0] - 'a')]);
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
double DetailedRandom::go()
{
  if (generators_.size() == 0) {
    mgrPtr_->getLogger()->info(
        DPO, 329, "Random improver requires at least one generator.");
    return 0.0;
  }

  // Collect candidate cells.
  collectCandidates();

  // Try to improve.
  int maxAttempts
      = (int) std::ceil(movesPerCandidate_ * (double) candidates_.size());
  Utility::random_shuffle(
      candidates_.begin(), candidates_.end(), mgrPtr_->rng_);

  deltaCost_.resize(objectives_.size());
  initCost_.resize(objectives_.size());
  currCost_.resize(objectives_.size());
  nextCost_.resize(objectives_.size());
  for (size_t i = 0; i < objectives_.size(); i++) {
    deltaCost_[i] = 0.;
    initCost_[i] = objectives_[i]->curr();
    currCost_[i] = initCost_[i];
    nextCost_[i] = initCost_[i];

    if (objectives_[i]->getName() == "abu") {
      auto ptr = dynamic_cast<DetailedABU*>(objectives_[i]);
      if (ptr != nullptr) {
        ptr->measureABU(true);
      }
    }
  }

  // Test.
  if (eval(currCost_, expr_) < 0.0) {
    mgrPtr_->getLogger()->info(DPO,
                               330,
                               "Test objective function failed, possibly due "
                               "to a badly formed cost function.");
    return 0.0;
  }

  double currTotalCost;
  double initTotalCost;
  double nextTotalCost;
  initTotalCost = eval(currCost_, expr_);
  currTotalCost = initTotalCost;
  nextTotalCost = initTotalCost;

  std::vector<int> gen_count(generators_.size());
  std::fill(gen_count.begin(), gen_count.end(), 0);
  for (int attempt = 0; attempt < maxAttempts; attempt++) {
    // Pick a generator at random.
    int g = (int) ((*(mgrPtr_->rng_))() % (generators_.size()));
    ++gen_count[g];
    // Generate a move list.
    if (generators_[g]->generate(mgrPtr_, candidates_) == false) {
      // Failed to generate anything so just move on to the next attempt.
      continue;
    }

    // The generator has provided a successful move which is stored in the
    // manager.  We need to evaluate that move to see if we should accept
    // or reject it.  Scan over the objective functions and use the move
    // information to compute the weighted deltas; an overall weighted delta
    // better than zero implies improvement.
    for (size_t i = 0; i < objectives_.size(); i++) {
      // XXX: NEED TO WEIGHT EACH OBJECTIVE!
      double change = objectives_[i]->delta(mgrPtr_->nMoved_,
                                            mgrPtr_->movedNodes_,
                                            mgrPtr_->curLeft_,
                                            mgrPtr_->curBottom_,
                                            mgrPtr_->curOri_,
                                            mgrPtr_->newLeft_,
                                            mgrPtr_->newBottom_,
                                            mgrPtr_->newOri_);

      deltaCost_[i] = change;
      nextCost_[i] = currCost_[i] - deltaCost_[i];  // -delta is +ve is less.
    }
    nextTotalCost = eval(nextCost_, expr_);

    //        std::cout << boost::format( "Move consisting of %d cells generated
    //        benefit of %.2lf; Will %s.\n" )
    //            % mgrPtr_->nMoved_ % delta % ((delta>0.)?"accept":"reject");

    //        if( delta > 0.0 )
    if (nextTotalCost <= currTotalCost) {
      mgrPtr_->acceptMove();
      for (size_t i = 0; i < objectives_.size(); i++) {
        objectives_[i]->accept();
      }

      // A great, but time-consuming, check here is to recompute the costs from
      // scratch and make sure they are the same as the incrementally computed
      // costs.  Very useful for debugging!  Could do this check ever so often
      // or just at the end...
      ;
      for (size_t i = 0; i < objectives_.size(); i++) {
        currCost_[i] = nextCost_[i];
      }
      currTotalCost = nextTotalCost;
    } else {
      mgrPtr_->rejectMove();
      for (size_t i = 0; i < objectives_.size(); i++) {
        objectives_[i]->reject();
      }
    }
  }
  for (size_t i = 0; i < gen_count.size(); i++) {
    mgrPtr_->getLogger()->info(DPO,
                               332,
                               "End of pass, Generator {:s} called {:d} times.",
                               generators_[i]->getName().c_str(),
                               gen_count[i]);
  }
  for (size_t i = 0; i < generators_.size(); i++) {
    generators_[i]->stats();
  }

  for (size_t i = 0; i < objectives_.size(); i++) {
    double scratch = objectives_[i]->curr();
    nextCost_[i] = scratch;  // Temporary.
    bool error = (std::fabs(scratch - currCost_[i]) > 1.0e-3);
    mgrPtr_->getLogger()->info(
        DPO,
        333,
        "End of pass, Objective {:s}, Initial cost {:.6e}, Scratch cost "
        "{:.6e}, Incremental cost {:.6e}, Mismatch? {:c}",
        objectives_[i]->getName().c_str(),
        initCost_[i],
        scratch,
        currCost_[i],
        ((error) ? 'Y' : 'N'));

    if (objectives_[i]->getName() == "abu") {
      auto ptr = dynamic_cast<DetailedABU*>(objectives_[i]);
      if (ptr != nullptr) {
        ptr->measureABU(true);
      }
    }
  }
  nextTotalCost = eval(nextCost_, expr_);
  mgrPtr_->getLogger()->info(
      DPO, 338, "End of pass, Total cost is {:.6e}.", nextTotalCost);

  return ((initTotalCost - currTotalCost) / initTotalCost);
}

//////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////
void DetailedRandom::collectCandidates()
{
  candidates_.erase(candidates_.begin(), candidates_.end());
  candidates_.insert(candidates_.end(),
                     mgrPtr_->singleHeightCells_.begin(),
                     mgrPtr_->singleHeightCells_.end());
  for (size_t i = 2; i < mgrPtr_->multiHeightCells_.size(); i++) {
    candidates_.insert(candidates_.end(),
                       mgrPtr_->multiHeightCells_[i].begin(),
                       mgrPtr_->multiHeightCells_[i].end());
  }
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
RandomGenerator::RandomGenerator()
    : DetailedGenerator("displacement"),
      mgr_(nullptr),
      arch_(nullptr),
      network_(nullptr),
      rt_(nullptr),
      attempts_(0),
      moves_(0),
      swaps_(0)
{
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool RandomGenerator::generate(DetailedMgr* mgr, std::vector<Node*>& candidates)
{
  ++attempts_;

  mgr_ = mgr;
  arch_ = mgr->getArchitecture();
  network_ = mgr->getNetwork();
  rt_ = mgr->getRoutingParams();

  double ywid = mgr_->getSingleRowHeight();
  const int ydim = mgr_->getNumSingleHeightRows();
  double xwid = arch_->getRow(0)->getSiteSpacing();
  const int xdim
      = std::max(0, (int) ((arch_->getMaxX() - arch_->getMinX()) / xwid));

  xwid = (arch_->getMaxX() - arch_->getMinX()) / (double) xdim;
  ywid = (arch_->getMaxY() - arch_->getMinY()) / (double) ydim;

  Node* ndi = candidates[(*(mgr_->rng_))() % (candidates.size())];
  const int spanned_i = arch_->getCellHeightInRows(ndi);
  if (spanned_i != 1) {
    return false;
  }
  // Segments for the source.
  const std::vector<DetailedSeg*>& segs_i
      = mgr_->reverseCellToSegs_[ndi->getId()];
  if (segs_i.size() != 1) {
    mgr_->getLogger()->error(
        DPO, 385, "Only working with single height cells currently.");
  }

  // For the window size.  This should be parameterized.
  const int rly = 10;
  const int rlx = 10;

  const int tries = 5;
  for (int t = 1; t <= tries; t++) {
    // Position of the source.
    const double yi = ndi->getBottom() + 0.5 * ndi->getHeight();
    const double xi = ndi->getLeft() + 0.5 * ndi->getWidth();

    // Segment for the source.
    const int si = segs_i[0]->getSegId();

    // Random position within a box centered about (xi,yi).
    const int grid_xi = std::min(
        xdim - 1, std::max(0, (int) ((xi - arch_->getMinX()) / xwid)));
    const int grid_yi = std::min(
        ydim - 1, std::max(0, (int) ((yi - arch_->getMinY()) / ywid)));

    const int rel_x = (*(mgr_->rng_))() % (2 * rlx + 1);
    const int rel_y = (*(mgr_->rng_))() % (2 * rly + 1);

    const int grid_xj
        = std::min(xdim - 1, std::max(0, (grid_xi - rlx + rel_x)));
    const int grid_yj
        = std::min(ydim - 1, std::max(0, (grid_yi - rly + rel_y)));

    // Position of the destination.
    const double xj = arch_->getMinX() + grid_xj * xwid;
    double yj = arch_->getMinY() + grid_yj * ywid;

    // Row and segment for the destination.
    int rj = (int) ((yj - arch_->getMinY()) / mgr_->getSingleRowHeight());
    rj = std::min(mgr_->getNumSingleHeightRows() - 1, std::max(0, rj));
    yj = arch_->getRow(rj)->getBottom();
    int sj = -1;
    for (int s = 0; s < mgr_->segsInRow_[rj].size(); s++) {
      const DetailedSeg* segPtr = mgr_->segsInRow_[rj][s];
      if (xj >= segPtr->getMinX() && xj <= segPtr->getMaxX()) {
        sj = segPtr->getSegId();
        break;
      }
    }

    // Need to determine validity of things.
    if (sj == -1 || ndi->getRegionId() != mgr_->segments_[sj]->getRegId()) {
      // The target segment cannot support the candidate cell.
      continue;
    }

    if (mgr_->tryMove(ndi,
                      ndi->getLeft(),
                      ndi->getBottom(),
                      si,
                      (int) std::round(xj),
                      (int) std::round(yj),
                      sj)) {
      ++moves_;
      return true;
    }
    if (mgr_->trySwap(ndi,
                      ndi->getLeft(),
                      ndi->getBottom(),
                      si,
                      (int) std::round(xj),
                      (int) std::round(yj),
                      sj)) {
      ++swaps_;
      return true;
    }
  }
  return false;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void RandomGenerator::stats()
{
  mgr_->getLogger()->info(
      DPO,
      335,
      "Generator {:s}, "
      "Cumulative attempts {:d}, swaps {:d}, moves {:5d} since last reset.",
      getName().c_str(),
      attempts_,
      swaps_,
      moves_);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
DisplacementGenerator::DisplacementGenerator()
    : DetailedGenerator("random"),
      mgr_(nullptr),
      arch_(nullptr),
      network_(nullptr),
      rt_(nullptr),
      attempts_(0),
      moves_(0),
      swaps_(0)
{
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool DisplacementGenerator::generate(DetailedMgr* mgr,
                                     std::vector<Node*>& candidates)
{
  ++attempts_;

  mgr_ = mgr;
  arch_ = mgr->getArchitecture();
  network_ = mgr->getNetwork();
  rt_ = mgr->getRoutingParams();

  double ywid = mgr_->getSingleRowHeight();
  const int ydim = mgr_->getNumSingleHeightRows();
  double xwid = arch_->getRow(0)->getSiteSpacing();
  const int xdim
      = std::max(0, (int) ((arch_->getMaxX() - arch_->getMinX()) / xwid));

  xwid = (arch_->getMaxX() - arch_->getMinX()) / (double) xdim;
  ywid = (arch_->getMaxY() - arch_->getMinY()) / (double) ydim;

  Node* ndi = candidates[(*(mgr_->rng_))() % (candidates.size())];

  // Segments for the source.
  const std::vector<DetailedSeg*>& segs_i
      = mgr_->reverseCellToSegs_[ndi->getId()];

  // For the window size.  This should be parameterized.
  const int rly = 5;
  const int rlx = 5;

  const int tries = 5;
  for (int t = 1; t <= tries; t++) {
    // Position of the source.
    // yi = ndi->getBottom()+0.5*ndi->getHeight();
    // xi = ndi->getLeft()+0.5*ndi->getWidth();

    // Segment for the source.
    const int si = segs_i[0]->getSegId();

    // Choices: (i) random position within a box centered at the original
    // position; (ii) random position within a box between the current
    // and original position; (iii) the original position itself.  Should
    // this also be a randomized choice??????????????????????????????????
    double xj, yj;
    if (true) {
      // Centered at the original position within a box.
      const double orig_yc = ndi->getOrigBottom() + 0.5 * ndi->getHeight();
      const double orig_xc = ndi->getOrigLeft() + 0.5 * ndi->getWidth();

      const int grid_xi = std::min(
          xdim - 1, std::max(0, (int) ((orig_xc - arch_->getMinX()) / xwid)));
      const int grid_yi = std::min(
          ydim - 1, std::max(0, (int) ((orig_yc - arch_->getMinY()) / ywid)));

      const int rel_x = (*(mgr_->rng_))() % (2 * rlx + 1);
      const int rel_y = (*(mgr_->rng_))() % (2 * rly + 1);

      const int grid_xj
          = std::min(xdim - 1, std::max(0, (grid_xi - rlx + rel_x)));
      const int grid_yj
          = std::min(ydim - 1, std::max(0, (grid_yi - rly + rel_y)));

      xj = arch_->getMinX() + grid_xj * xwid;
      yj = arch_->getMinY() + grid_yj * ywid;
    }
    if (false) {
      // The original position.
      xj = ndi->getOrigLeft() + 0.5 * ndi->getWidth();
      yj = ndi->getOrigBottom() + 0.5 * ndi->getHeight();
    }
    if (false) {
      // Somewhere between current position and original position.
      double orig_yc = ndi->getOrigBottom() + 0.5 * ndi->getHeight();
      double orig_xc = ndi->getOrigLeft() + 0.5 * ndi->getWidth();

      double curr_yc = ndi->getBottom() + 0.5 * ndi->getHeight();
      double curr_xc = ndi->getLeft() + 0.5 * ndi->getWidth();

      int grid_xi = std::min(
          xdim - 1, std::max(0, (int) ((curr_xc - arch_->getMinX()) / xwid)));
      int grid_yi = std::min(
          ydim - 1, std::max(0, (int) ((curr_yc - arch_->getMinY()) / ywid)));

      int grid_xj = std::min(
          xdim - 1, std::max(0, (int) ((orig_xc - arch_->getMinX()) / xwid)));
      int grid_yj = std::min(
          ydim - 1, std::max(0, (int) ((orig_yc - arch_->getMinY()) / ywid)));

      if (grid_xi > grid_xj)
        std::swap(grid_xi, grid_xj);
      if (grid_yi > grid_yj)
        std::swap(grid_yi, grid_yj);

      const int w = grid_xj - grid_xi;
      const int h = grid_yj - grid_yi;

      const int rel_x = (*(mgr_->rng_))() % (w + 1);
      const int rel_y = (*(mgr_->rng_))() % (h + 1);

      grid_xj = std::min(xdim - 1, std::max(0, (grid_xi + rel_x)));
      grid_yj = std::min(ydim - 1, std::max(0, (grid_yi + rel_y)));

      xj = arch_->getMinX() + grid_xj * xwid;
      yj = arch_->getMinY() + grid_yj * ywid;
    }

    // Row and segment for the destination.
    int rj = (int) ((yj - arch_->getMinY()) / mgr_->getSingleRowHeight());
    rj = std::min(mgr_->getNumSingleHeightRows() - 1, std::max(0, rj));
    yj = arch_->getRow(rj)->getBottom();
    int sj = -1;
    for (int s = 0; s < mgr_->segsInRow_[rj].size(); s++) {
      DetailedSeg* segPtr = mgr_->segsInRow_[rj][s];
      if (xj >= segPtr->getMinX() && xj <= segPtr->getMaxX()) {
        sj = segPtr->getSegId();
        break;
      }
    }

    // Need to determine validity of things.
    if (sj == -1 || ndi->getRegionId() != mgr_->segments_[sj]->getRegId()) {
      // The target segment cannot support the candidate cell.
      continue;
    }

    if (mgr_->tryMove(ndi,
                      ndi->getLeft(),
                      ndi->getBottom(),
                      si,
                      (int) std::round(xj),
                      (int) std::round(yj),
                      sj)) {
      ++moves_;
      return true;
    }
    if (mgr_->trySwap(ndi,
                      ndi->getLeft(),
                      ndi->getBottom(),
                      si,
                      (int) std::round(xj),
                      (int) std::round(yj),
                      sj)) {
      ++swaps_;
      return true;
    }
  }
  return false;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void DisplacementGenerator::stats()
{
  mgr_->getLogger()->info(
      DPO,
      337,
      "Generator {:s}, "
      "Cumulative attempts {:d}, swaps {:d}, moves {:5d} since last reset.",
      getName().c_str(),
      attempts_,
      swaps_,
      moves_);
}

}  // namespace dpo
