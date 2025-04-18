// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021-2025, The OpenROAD Authors

///////////////////////////////////////////////////////////////////////////////
//
// Description:
// Essentially a zero temperature annealer that can use a variety of
// move generators, different objectives and a cost function in order
// to improve a placement.

#include <algorithm>
#include <boost/tokenizer.hpp>
#include <cmath>
#include <cstddef>
#include <stack>
#include <string>
#include <vector>

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

using utl::DPO;

namespace dpo {

bool DetailedRandom::isOperator(char ch) const
{
  if (ch == '+' || ch == '-' || ch == '*' || ch == '/' || ch == '^') {
    return true;
  }
  return false;
}

bool DetailedRandom::isObjective(char ch) const
{
  if (ch >= 'a' && ch <= 'z') {
    return true;
  }
  return false;
}

bool DetailedRandom::isNumber(char ch) const
{
  if (ch >= '0' && ch <= '9') {
    return true;
  }
  return false;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
DetailedRandom::DetailedRandom(Architecture* arch, Network* network)
    : arch_(arch), network_(network)
{
}

//////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////
void DetailedRandom::run(DetailedMgr* mgrPtr, const std::string& command)
{
  // A temporary interface to allow for a string which we will decode to create
  // the arguments.
  boost::char_separator<char> separators(" \r\t\n;");
  boost::tokenizer<boost::char_separator<char>> tokens(command, separators);
  std::vector<std::string> args;
  for (const auto& token : tokens) {
    args.push_back(token);
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

  std::string generatorStr;
  std::string objectiveStr;
  std::string costStr;
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
  for (auto generator : generators_) {
    delete generator;
  }
  generators_.clear();

  // Additional generators per the command. XXX: Need to write the code for
  // these objects; just a concept now.
  if (!generatorStr.empty()) {
    boost::char_separator<char> separators(" \r\t\n:");
    boost::tokenizer<boost::char_separator<char>> tokens(generatorStr,
                                                         separators);
    std::vector<std::string> gens;
    for (const auto& token : tokens) {
      gens.push_back(token);
    }

    for (const auto& gen : gens) {
      // if( gens[i] == "ro" )       std::cout << "reorder generator requested."
      // << std::endl; else if( gens[i] == "mis" ) std::cout << "set matching
      // generator requested." << std::endl;
      if (gen == "gs") {
        generators_.push_back(new DetailedGlobalSwap());
      } else if (gen == "vs") {
        generators_.push_back(new DetailedVerticalSwap());
      } else if (gen == "rng") {
        generators_.push_back(new RandomGenerator());
      } else if (gen == "disp") {
        generators_.push_back(new DisplacementGenerator());
      }
    }
  }
  if (generators_.empty()) {
    // Default generator.
    generators_.push_back(new RandomGenerator());
  }
  for (auto generator : generators_) {
    generator->init(mgrPtr_);

    mgrPtr_->getLogger()->info(DPO,
                               324,
                               "Random improver is using {:s} generator.",
                               generator->getName().c_str());
  }

  // Objectives.
  for (auto objective : objectives_) {
    delete objective;
  }
  objectives_.clear();

  // Additional objectives per the command. XXX: Need to write the code for
  // these objects; just a concept now.
  if (!objectiveStr.empty()) {
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

    for (const auto& obj : objs) {
      if (obj == "abu") {
        auto objABU = new DetailedABU(arch_, network_);
        objABU->init(mgrPtr_, nullptr);
        objectives_.push_back(objABU);
      } else if (obj == "disp") {
        auto objDisp = new DetailedDisplacement(arch_);
        objDisp->init(mgrPtr_, nullptr);
        objectives_.push_back(objDisp);
      } else if (obj == "hpwl") {
        auto objHpwl = new DetailedHPWL(network_);
        objHpwl->init(mgrPtr_, nullptr);
        objectives_.push_back(objHpwl);
      }
    }
  }
  if (objectives_.empty()) {
    // Default objective.
    auto objHpwl = new DetailedHPWL(network_);
    objHpwl->init(mgrPtr_, nullptr);
    objectives_.push_back(objHpwl);
  }

  for (auto objective : objectives_) {
    mgrPtr_->getLogger()->info(DPO,
                               325,
                               "Random improver is using {:s} objective.",
                               objective->getName().c_str());
  }

  // Should I just be figuring out the objectives needed from the cost string?
  if (!costStr.empty()) {
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
        expr_.emplace_back(1, *it);
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
    expr_.emplace_back(1, 'a');
    for (size_t i = 1; i < objectives_.size(); i++) {
      expr_.emplace_back(1, (char) ('a' + i));
      expr_.emplace_back(1, '+');
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
  for (auto generator : generators_) {
    delete generator;
  }
  generators_.clear();
  for (auto objective : objectives_) {
    delete objective;
  }
  objectives_.clear();
}

double DetailedRandom::doOperation(double a, double b, char op) const
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

double DetailedRandom::eval(const std::vector<double>& costs,
                            const std::vector<std::string>& expr) const
{
  std::stack<double> stk;
  for (const std::string& val : expr) {
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
  if (generators_.empty()) {
    mgrPtr_->getLogger()->info(
        DPO, 329, "Random improver requires at least one generator.");
    return 0.0;
  }

  // Collect candidate cells.
  collectCandidates();
  if (candidates_.empty()) {
    mgrPtr_->getLogger()->info(DPO, 203, "No movable cells found");
    return 0.0;
  }

  // Try to improve.
  int maxAttempts
      = (int) std::ceil(movesPerCandidate_ * (double) candidates_.size());
  mgrPtr_->shuffle(candidates_);

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
  initTotalCost = eval(currCost_, expr_);
  currTotalCost = initTotalCost;

  std::vector<int> gen_count(generators_.size());
  std::fill(gen_count.begin(), gen_count.end(), 0);
  for (int attempt = 0; attempt < maxAttempts; attempt++) {
    // Pick a generator at random.
    int g = mgrPtr_->getRandom(generators_.size());
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
      double change = objectives_[i]->delta(mgrPtr_->getJournal());

      deltaCost_[i] = change;
      nextCost_[i] = currCost_[i] - deltaCost_[i];  // -delta is +ve is less.
    }
    const double nextTotalCost = eval(nextCost_, expr_);
    if (nextTotalCost <= currTotalCost) {
      mgrPtr_->acceptMove();
      for (auto objective : objectives_) {
        objective->accept();
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
      for (auto objective : objectives_) {
        objective->reject();
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
  for (auto generator : generators_) {
    generator->stats();
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
  const double nextTotalCost = eval(nextCost_, expr_);
  mgrPtr_->getLogger()->info(
      DPO, 338, "End of pass, Total cost is {:.6e}.", nextTotalCost);

  return ((initTotalCost - currTotalCost) / initTotalCost);
}

//////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////
void DetailedRandom::collectCandidates()
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

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
RandomGenerator::RandomGenerator() : DetailedGenerator("displacement")
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

  const int ydim = mgr_->getNumSingleHeightRows();
  double xwid = arch_->getRow(0)->getSiteSpacing();
  const int xdim
      = std::max(0, (int) ((arch_->getMaxX() - arch_->getMinX()) / xwid));

  xwid = (arch_->getMaxX() - arch_->getMinX()) / (double) xdim;
  double ywid = (arch_->getMaxY() - arch_->getMinY()) / (double) ydim;

  Node* ndi = candidates[mgr_->getRandom(candidates.size())];
  const int spanned_i = arch_->getCellHeightInRows(ndi);
  if (spanned_i != 1) {
    return false;
  }
  // Segments for the source.
  const std::vector<DetailedSeg*>& segs_i
      = mgr_->getReverseCellToSegs(ndi->getId());
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
    const double yi = ndi->getBottom().v + 0.5 * ndi->getHeight().v;
    const double xi = ndi->getLeft().v + 0.5 * ndi->getWidth().v;

    // Segment for the source.
    const int si = segs_i[0]->getSegId();

    // Random position within a box centered about (xi,yi).
    const int grid_xi = std::min(
        xdim - 1, std::max(0, (int) ((xi - arch_->getMinX()) / xwid)));
    const int grid_yi = std::min(
        ydim - 1, std::max(0, (int) ((yi - arch_->getMinY()) / ywid)));

    const int rel_x = mgr_->getRandom(2 * rlx + 1);
    const int rel_y = mgr_->getRandom(2 * rly + 1);

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
    for (int s = 0; s < mgr_->getNumSegsInRow(rj); s++) {
      const DetailedSeg* segPtr = mgr_->getSegsInRow(rj)[s];
      if (xj >= segPtr->getMinX() && xj <= segPtr->getMaxX()) {
        sj = segPtr->getSegId();
        break;
      }
    }

    // Need to determine validity of things.
    if (sj == -1 || ndi->getGroupId() != mgr_->getSegment(sj)->getRegId()) {
      // The target segment cannot support the candidate cell.
      continue;
    }

    if (mgr_->tryMove(ndi,
                      ndi->getLeft(),
                      ndi->getBottom(),
                      si,
                      DbuX{(int) std::round(xj)},
                      DbuY{(int) std::round(yj)},
                      sj)) {
      ++moves_;
      return true;
    }
    if (mgr_->trySwap(ndi,
                      ndi->getLeft(),
                      ndi->getBottom(),
                      si,
                      DbuX{(int) std::round(xj)},
                      DbuY{(int) std::round(yj)},
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
DisplacementGenerator::DisplacementGenerator() : DetailedGenerator("random")
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

  const int ydim = mgr_->getNumSingleHeightRows();
  double xwid = arch_->getRow(0)->getSiteSpacing();
  const int xdim
      = std::max(0, (int) ((arch_->getMaxX() - arch_->getMinX()) / xwid));

  xwid = (arch_->getMaxX() - arch_->getMinX()) / (double) xdim;
  double ywid = (arch_->getMaxY() - arch_->getMinY()) / (double) ydim;

  Node* ndi = candidates[mgr_->getRandom(candidates.size())];

  // Segments for the source.
  const std::vector<DetailedSeg*>& segs_i
      = mgr_->getReverseCellToSegs(ndi->getId());

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
      const double orig_yc = ndi->getOrigBottom().v + 0.5 * ndi->getHeight().v;
      const double orig_xc = ndi->getOrigLeft().v + 0.5 * ndi->getWidth().v;

      const int grid_xi = std::min(
          xdim - 1, std::max(0, (int) ((orig_xc - arch_->getMinX()) / xwid)));
      const int grid_yi = std::min(
          ydim - 1, std::max(0, (int) ((orig_yc - arch_->getMinY()) / ywid)));

      const int rel_x = mgr_->getRandom(2 * rlx + 1);
      const int rel_y = mgr_->getRandom(2 * rly + 1);

      const int grid_xj
          = std::min(xdim - 1, std::max(0, (grid_xi - rlx + rel_x)));
      const int grid_yj
          = std::min(ydim - 1, std::max(0, (grid_yi - rly + rel_y)));

      xj = arch_->getMinX() + grid_xj * xwid;
      yj = arch_->getMinY() + grid_yj * ywid;
    }
    if (false) {
      // The original position.
      xj = ndi->getOrigLeft().v + 0.5 * ndi->getWidth().v;
      yj = ndi->getOrigBottom().v + 0.5 * ndi->getHeight().v;
    }
    if (false) {
      // Somewhere between current position and original position.
      double orig_yc = ndi->getOrigBottom().v + 0.5 * ndi->getHeight().v;
      double orig_xc = ndi->getOrigLeft().v + 0.5 * ndi->getWidth().v;

      double curr_yc = ndi->getBottom().v + 0.5 * ndi->getHeight().v;
      double curr_xc = ndi->getLeft().v + 0.5 * ndi->getWidth().v;

      int grid_xi = std::min(
          xdim - 1, std::max(0, (int) ((curr_xc - arch_->getMinX()) / xwid)));
      int grid_yi = std::min(
          ydim - 1, std::max(0, (int) ((curr_yc - arch_->getMinY()) / ywid)));

      int grid_xj = std::min(
          xdim - 1, std::max(0, (int) ((orig_xc - arch_->getMinX()) / xwid)));
      int grid_yj = std::min(
          ydim - 1, std::max(0, (int) ((orig_yc - arch_->getMinY()) / ywid)));

      if (grid_xi > grid_xj) {
        std::swap(grid_xi, grid_xj);
      }
      if (grid_yi > grid_yj) {
        std::swap(grid_yi, grid_yj);
      }

      const int w = grid_xj - grid_xi;
      const int h = grid_yj - grid_yi;

      const int rel_x = mgr_->getRandom(w + 1);
      const int rel_y = mgr_->getRandom(h + 1);

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
    for (int s = 0; s < mgr_->getNumSegsInRow(rj); s++) {
      DetailedSeg* segPtr = mgr_->getSegsInRow(rj)[s];
      if (xj >= segPtr->getMinX() && xj <= segPtr->getMaxX()) {
        sj = segPtr->getSegId();
        break;
      }
    }

    // Need to determine validity of things.
    if (sj == -1 || ndi->getGroupId() != mgr_->getSegment(sj)->getRegId()) {
      // The target segment cannot support the candidate cell.
      continue;
    }

    if (mgr_->tryMove(ndi,
                      ndi->getLeft(),
                      ndi->getBottom(),
                      si,
                      DbuX{(int) std::round(xj)},
                      DbuY{(int) std::round(yj)},
                      sj)) {
      ++moves_;
      return true;
    }
    if (mgr_->trySwap(ndi,
                      ndi->getLeft(),
                      ndi->getBottom(),
                      si,
                      DbuX{(int) std::round(xj)},
                      DbuY{(int) std::round(yj)},
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
