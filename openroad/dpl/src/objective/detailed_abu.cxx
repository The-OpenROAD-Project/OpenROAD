// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021-2025, The OpenROAD Authors

///////////////////////////////////////////////////////////////////////////////
//
// Description:
// Implements an objective to penalize areas of the placement which have
// a utilization larger than a global target utilization.
// The idea is that it returns a value >= 0 if there are bins with high
// utilization and can be used within a cost function in a form such as
// hpwl * (1+penalty), where "penalty" is the returned value.  If there
// is no bins over the target utilization, then penalty will be zero.
//
// Code is also included to compute the so-called "ABU" metric which is
// a weighted metric used in a prior placement contest.  The idea here
// is the overfilled bins are weighted such that those with higher utilization
// are more heavily weighted.  This metric is for information purposes:
// it is not part of the cost function, but can be called to print out the
// ABU metric (and the resulting ABU penalty from the contest).
// ABU = Average Bin Utilization (of the top x% densest bins).

#include "detailed_abu.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <vector>

#include "objective/detailed_objective.h"
#include "optimization/detailed_orient.h"
#include "util/journal.h"
#include "utl/Logger.h"

namespace dpl {

using utl::DPL;

////////////////////////////////////////////////////////////////////////////////
// Defines.
////////////////////////////////////////////////////////////////////////////////

#if defined(USE_ISPD14)
constexpr double BIN_DIM = 4.0;
#else
constexpr double BIN_DIM = 9.0;
#endif
constexpr double BIN_AREA_THRESHOLD = 0.2;
constexpr double FREE_SPACE_THRESHOLD = 0.2;
constexpr double ABU_ALPHA = 1.0;
constexpr int ABU2_WGT = 10;
constexpr int ABU5_WGT = 4;
constexpr int ABU10_WGT = 2;
constexpr int ABU20_WGT = 1;

////////////////////////////////////////////////////////////////////////////////
// Classes.
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
DetailedABU::DetailedABU(Architecture* arch, Network* network)
    : DetailedObjective("abu"), arch_(arch), network_(network)
{
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void DetailedABU::init()
{
  // Determine how to size the ABU grid.  We can set according to the detailed
  // placement contest, or we can set according to the GCELL size...  Or, we
  // can do something even different...

  abuTargUt_ = mgrPtr_->getTargetUt();  // XXX: Need to set this somehow!!!

  abuGridUnit_ = BIN_DIM * arch_->getRow(0)->getHeight().v;
  abuGridNumX_ = (int) ceil((arch_->getWidth().v) / abuGridUnit_);
  abuGridNumY_ = (int) ceil((arch_->getHeight().v) / abuGridUnit_);
  abuNumBins_ = abuGridNumX_ * abuGridNumY_;
  abuBins_.resize(abuNumBins_);

  clearBuckets();
  clearBins();

  // Initialize the density map.  Then, add in the fixed stuff.  We only need to
  // do this once!  During ABU computation (either incremental or from
  // scratch), we only deal with moveable stuff.

  for (int j = 0; j < abuGridNumY_; j++) {
    for (int k = 0; k < abuGridNumX_; k++) {
      unsigned binId = j * abuGridNumX_ + k;

      abuBins_[binId].id = binId;

      abuBins_[binId].lx = arch_->getMinX().v + k * abuGridUnit_;
      abuBins_[binId].ly = arch_->getMinY().v + j * abuGridUnit_;
      abuBins_[binId].hx = abuBins_[binId].lx + abuGridUnit_;
      abuBins_[binId].hy = abuBins_[binId].ly + abuGridUnit_;

      abuBins_[binId].hx
          = std::min(abuBins_[binId].hx, (double) arch_->getMaxX().v);
      abuBins_[binId].hy
          = std::min(abuBins_[binId].hy, (double) arch_->getMaxY().v);

      double w = abuBins_[binId].hx - abuBins_[binId].lx;
      double h = abuBins_[binId].hy - abuBins_[binId].ly;

      abuBins_[binId].area = std::max(w * h, 0.0);
      abuBins_[binId].m_util = 0.0;
      abuBins_[binId].f_util = 0.0;
      abuBins_[binId].c_util = 0.0;
      abuBins_[binId].free_space = abuBins_[binId].area;
    }
  }

  // Insert fixed area.
  for (int i = 0; i < network_->getNumNodes(); i++) {
    Node* nd = network_->getNode(i);

    if (!nd->isFixed()) {
      continue;
    }

    const double xmin = nd->getLeft().v;
    const double xmax = nd->getRight().v;
    const double ymin = nd->getBottom().v;
    const double ymax = nd->getTop().v;

    const int lcol
        = std::max((int) floor((xmin - arch_->getMinX().v) / abuGridUnit_), 0);
    const int rcol
        = std::min((int) floor((xmax - arch_->getMinX().v) / abuGridUnit_),
                   abuGridNumX_ - 1);
    const int brow
        = std::max((int) floor((ymin - arch_->getMinY().v) / abuGridUnit_), 0);
    const int trow
        = std::min((int) floor((ymax - arch_->getMinY().v) / abuGridUnit_),
                   abuGridNumY_ - 1);

    for (int j = brow; j <= trow; j++) {
      for (int k = lcol; k <= rcol; k++) {
        const unsigned binId = j * abuGridNumX_ + k;

        // Get intersection
        const double lx = std::max(abuBins_[binId].lx, xmin);
        const double hx = std::min(abuBins_[binId].hx, xmax);
        const double ly = std::max(abuBins_[binId].ly, ymin);
        const double hy = std::min(abuBins_[binId].hy, ymax);

        if ((hx - lx) > 1.0e-5 && (hy - ly) > 1.0e-5) {
          const double common_area = (hx - lx) * (hy - ly);
          abuBins_[binId].f_util += common_area;
        }
      }
    }
  }
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void DetailedABU::init(DetailedMgr* mgrPtr, DetailedOrient* orientPtr)
{
  orientPtr_ = orientPtr;
  mgrPtr_ = mgrPtr;
  init();
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void DetailedABU::clearUtils()
{
  // Set utilizations to zero.
  for (int j = 0; j < abuGridNumY_; j++) {
    for (int k = 0; k < abuGridNumX_; k++) {
      const unsigned binId = j * abuGridNumX_ + k;

      abuBins_[binId].m_util = 0.0;
      abuBins_[binId].c_util = 0.0;
    }
  }
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void DetailedABU::computeUtils()
{
  // Insert movables.
  for (int i = 0; i < network_->getNumNodes(); i++) {
    const Node* nd = network_->getNode(i);

    if (nd->isTerminal() || nd->isFixed()) {
      continue;
    }

    const double nlx = nd->getLeft().v;
    const double nrx = nd->getRight().v;
    const double nly = nd->getBottom().v;
    const double nhy = nd->getTop().v;

    const int lcol
        = std::max((int) floor((nlx - arch_->getMinX().v) / abuGridUnit_), 0);
    const int rcol
        = std::min((int) floor((nrx - arch_->getMinX().v) / abuGridUnit_),
                   abuGridNumX_ - 1);
    const int brow
        = std::max((int) floor((nly - arch_->getMinY().v) / abuGridUnit_), 0);
    const int trow
        = std::min((int) floor((nhy - arch_->getMinY().v) / abuGridUnit_),
                   abuGridNumY_ - 1);

    // Cell area...
    for (int j = brow; j <= trow; j++) {
      for (int k = lcol; k <= rcol; k++) {
        const unsigned binId = j * abuGridNumX_ + k;

        // get intersection
        const double lx = std::max(abuBins_[binId].lx, nlx);
        const double hx = std::min(abuBins_[binId].hx, nrx);
        const double ly = std::max(abuBins_[binId].ly, nly);
        const double hy = std::min(abuBins_[binId].hy, nhy);

        if ((hx - lx) > 1.0e-5 && (hy - ly) > 1.0e-5) {
          const double common_area = (hx - lx) * (hy - ly);
          abuBins_[binId].m_util += common_area;
        }
      }
    }
  }
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void DetailedABU::computeBuckets()
{
  // Put bins into buckets.
  for (auto bucket : utilBuckets_) {
    bucket.clear();
  }
  for (int j = 0; j < abuGridNumY_; j++) {
    for (int k = 0; k < abuGridNumX_; k++) {
      const unsigned binId = j * abuGridNumX_ + k;

      const int ix = getBucketId(binId, abuBins_[binId].m_util);
      if (ix != -1) {
        utilBuckets_[ix].insert(binId);
      }
    }
  }
  for (size_t i = 0; i < utilBuckets_.size(); i++) {
    utilTotals_[i] = 0.;
    for (auto it = utilBuckets_[i].begin(); it != utilBuckets_[i].end(); it++) {
      const double space = abuBins_[*it].area - abuBins_[*it].f_util;
      const double util = abuBins_[*it].m_util;

      utilTotals_[i] += util / space;
    }
  }
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
int DetailedABU::getBucketId(int binId, double occ)
{
  // Update the utilization buckets based on the current utilization.

  if (abuBins_[binId].area
      <= abuGridUnit_ * abuGridUnit_ * BIN_AREA_THRESHOLD) {
    return -1;
  }
  const double free_space = abuBins_[binId].area - abuBins_[binId].f_util;
  if (free_space <= FREE_SPACE_THRESHOLD * abuBins_[binId].area) {
    return -1;
  }

  const double util = occ / free_space;

  const double denom = 1. / (double) utilBuckets_.size();
  const int ix = std::max(
      0, std::min((int) (util / denom), (int) utilBuckets_.size() - 1));

  return ix;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
double DetailedABU::measureABU(bool print)
{
  clearUtils();
  computeUtils();
  clearBuckets();
  computeBuckets();

  return calculateABU(print);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
double DetailedABU::calculateABU(bool print)
{
  // Computes the ABU using the bin's current utilizations.  So, could be wrong
  // if bins not kept up-to-date.

  abuTargUt02_ = 0.0;
  abuTargUt05_ = 0.0;
  abuTargUt10_ = 0.0;
  abuTargUt20_ = 0.0;

  // Determine free space and utilization per bin.
  std::vector<double> util_array(abuNumBins_, 0.0);
  for (int j = 0; j < abuGridNumY_; j++) {
    for (int k = 0; k < abuGridNumX_; k++) {
      const unsigned binId = j * abuGridNumX_ + k;
      if (abuBins_[binId].area
          > abuGridUnit_ * abuGridUnit_ * BIN_AREA_THRESHOLD) {
        abuBins_[binId].free_space
            = abuBins_[binId].area - abuBins_[binId].f_util;
        if (abuBins_[binId].free_space
            > FREE_SPACE_THRESHOLD * abuBins_[binId].area) {
          util_array[binId]
              = abuBins_[binId].m_util / abuBins_[binId].free_space;
        }
      }
    }
  }

  std::ranges::sort(util_array);

  // Get different values.
  double abu2 = 0.0, abu5 = 0.0, abu10 = 0.0, abu20 = 0.0;
  int clip_index = (int) (0.02 * abuNumBins_);
  for (int j = abuNumBins_ - 1; j > abuNumBins_ - 1 - clip_index; j--) {
    abu2 += util_array[j];
  }
  abu2 = (clip_index) ? abu2 / clip_index : util_array[abuNumBins_ - 1];

  clip_index = (int) (0.05 * abuNumBins_);
  for (int j = abuNumBins_ - 1; j > abuNumBins_ - 1 - clip_index; j--) {
    abu5 += util_array[j];
  }
  abu5 = (clip_index) ? abu5 / clip_index : util_array[abuNumBins_ - 1];

  clip_index = (int) (0.10 * abuNumBins_);
  for (int j = abuNumBins_ - 1; j > abuNumBins_ - 1 - clip_index; j--) {
    abu10 += util_array[j];
  }
  abu10 = (clip_index) ? abu10 / clip_index : util_array[abuNumBins_ - 1];

  clip_index = (int) (0.20 * abuNumBins_);
  for (int j = abuNumBins_ - 1; j > abuNumBins_ - 1 - clip_index; j--) {
    abu20 += util_array[j];
  }
  abu20 = (clip_index) ? abu20 / clip_index : util_array[abuNumBins_ - 1];
  util_array.clear();

  abuTargUt02_ = abu2;
  abuTargUt05_ = abu5;
  abuTargUt10_ = abu10;
  abuTargUt20_ = abu20;

  /* calculate overflow and penalty */
  if (std::fabs(abuTargUt_ - 1.0) <= 1.0e-3) {
    abu2 = abu5 = abu10 = abu20 = 0.0;
  } else {
    abu2 = std::max(0.0, abu2 / abuTargUt_ - 1.0);
    abu5 = std::max(0.0, abu5 / abuTargUt_ - 1.0);
    abu10 = std::max(0.0, abu10 / abuTargUt_ - 1.0);
    abu20 = std::max(0.0, abu20 / abuTargUt_ - 1.0);
  }

  const double penalty
      = (ABU2_WGT * abu2 + ABU5_WGT * abu5 + ABU10_WGT * abu10
         + ABU20_WGT * abu20)
        / (double) (ABU2_WGT + ABU5_WGT + ABU10_WGT + ABU20_WGT);

  if (print) {
    mgrPtr_->getLogger()->info(DPL,
                               317,
                               "ABU: Target {:.2f}, "
                               "ABU_2,5,10,20: "
                               "{:.2f}, {:.2f}, {:.2f}, {:.2f}, "
                               "Penalty {:.2f}",
                               abuTargUt_,
                               abu2,
                               abu5,
                               abu10,
                               abu20,
                               penalty);
  }

  return penalty;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
double DetailedABU::curr()
{
  if (abuTargUt_ >= 0.999) {
    return 0;
  }

  clearUtils();
  computeUtils();
  clearBuckets();
  computeBuckets();

  // Create a cost based on the buckets.  Something based on the buckets
  // which is "normalized" towards 0 when nothing is wrong.  This
  // implies we can use some sort of (1.0+penalty) during annealing.
  const int n = (int) utilBuckets_.size();
  const double denom = 1.0 / (double) n;
  double fof = 0.;
  for (int i = n; (i * denom) > abuTargUt_;) {
    --i;
    if (!utilBuckets_[i].empty()) {
      fof += utilTotals_[i] / (double) utilBuckets_[i].size();
    }
  }
  return fof;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
double DetailedABU::delta(const Journal& journal)
{
  // Need change in fof metric.  Not many bins involved, so should be
  // fast to compute old and new.
  const double denom = 1.0 / (double) utilBuckets_.size();

  double fofOld = 0.;
  for (int i = (int) utilBuckets_.size(); (i * denom) > abuTargUt_;) {
    --i;
    if (!utilBuckets_[i].empty()) {
      fofOld += utilTotals_[i] / (double) utilBuckets_[i].size();
    }
  }

  // Compute changed bins and changed occupancy.
  ++abuChangedBinsCounter_;
  journal.undo(true);
  for (const auto& node : journal.getAffectedNodes()) {
    updateBins(node,
               node->getLeft().v + 0.5 * node->getWidth().v,
               node->getBottom().v + 0.5 * node->getHeight().v,
               -1);
  }
  journal.redo(true);
  for (const auto& node : journal.getAffectedNodes()) {
    updateBins(node,
               node->getLeft().v + 0.5 * node->getWidth().v,
               node->getBottom().v + 0.5 * node->getHeight().v,
               +1);
  }

  for (int binId : abuChangedBins_) {
    const double space = abuBins_[binId].area - abuBins_[binId].f_util;

    const double util_c = abuBins_[binId].c_util;
    const int ix_c = getBucketId(binId, util_c);
    if (ix_c != -1) {
      auto it = utilBuckets_[ix_c].find(binId);
      if (it == utilBuckets_[ix_c].end()) {
        mgrPtr_->internalError(
            "Unable to find bin within utilization objective");
      }
      utilBuckets_[ix_c].erase(it);
      utilTotals_[ix_c] -= util_c / space;
    }

    const double util_m = abuBins_[binId].m_util;
    const int ix_m = getBucketId(binId, util_m);
    if (ix_m != -1) {
      utilBuckets_[ix_m].insert(binId);
      utilTotals_[ix_m] += util_m / space;
    }
  }

  double fofNew = 0.;
  for (int i = (int) utilBuckets_.size(); (i * denom) > abuTargUt_;) {
    --i;
    if (!utilBuckets_[i].empty()) {
      fofNew += utilTotals_[i] / (double) utilBuckets_[i].size();
    }
  }
  const double fofDelta = fofOld - fofNew;
  return fofDelta;

  // The following is not any sort of normalized number which can
  // be bad...
  /*
  double delta_of = delta();
  return delta_of;
  */
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
double DetailedABU::delta()
{
  if (abuTargUt_ >= 0.999) {
    return 0.0;
  }

  double delta = 0.;
  double sum_0 = 0.;
  double sum_1 = 0.;
  for (int binId : abuChangedBins_) {
    if (abuBins_[binId].area
        > abuGridUnit_ * abuGridUnit_ * BIN_AREA_THRESHOLD) {
      const double free_space = abuBins_[binId].area - abuBins_[binId].f_util;
      if (free_space > FREE_SPACE_THRESHOLD * abuBins_[binId].area) {
        const double util_0 = abuBins_[binId].c_util / free_space;
        const double pen_0
            = (abuTargUt_ - std::max(util_0, abuTargUt_)) / (abuTargUt_ - 1.);
        sum_0 += pen_0;

        const double util_1 = abuBins_[binId].m_util / free_space;
        const double pen_1
            = (abuTargUt_ - std::max(util_1, abuTargUt_)) / (abuTargUt_ - 1.);
        sum_1 += pen_1;

        // delta += pen_0;
        // delta -= pen_1;
      }
    }
  }
  delta = sum_0 - sum_1;

  // XXX: A +ve value returned means improvement...
  return delta;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void DetailedABU::updateBins(const Node* nd,
                             const double x,
                             const double y,
                             const int addSub)
{
  // Updates the bins incrementally.  Can add or remove a *MOVABLE* node's
  // contribution to the bin utilization.  Assumes the node is located at (x,y)
  // rather than the position stored in the node...

  if (nd->isTerminal() || nd->isFixed()) {
    mgrPtr_->internalError("Problem updating bins for utilization objective");
  }

  const double lx = x - 0.5 * nd->getWidth().v - arch_->getMinX().v;
  const double ux = x + 0.5 * nd->getWidth().v - arch_->getMinX().v;
  const double ly = y - 0.5 * nd->getHeight().v - arch_->getMinY().v;
  const double uy = y + 0.5 * nd->getHeight().v - arch_->getMinY().v;

  const int lcol = std::max((int) floor(lx / abuGridUnit_), 0);
  const int rcol = std::min((int) floor(ux / abuGridUnit_), abuGridNumX_ - 1);
  const int brow = std::max((int) floor(ly / abuGridUnit_), 0);
  const int trow = std::min((int) floor(uy / abuGridUnit_), abuGridNumY_ - 1);

  for (int j = brow; j <= trow; j++) {
    for (int k = lcol; k <= rcol; k++) {
      const int binId = j * abuGridNumX_ + k;

      // get intersection
      const double lx
          = std::max(abuBins_[binId].lx, x - 0.5 * nd->getWidth().v);
      const double hx
          = std::min(abuBins_[binId].hx, x + 0.5 * nd->getWidth().v);
      const double ly
          = std::max(abuBins_[binId].ly, y - 0.5 * nd->getHeight().v);
      const double hy
          = std::min(abuBins_[binId].hy, y + 0.5 * nd->getHeight().v);

      if ((hx - lx) > 1.0e-5 && (hy - ly) > 1.0e-5) {
        // XXX: Keep track of the bins that change.
        if (abuChangedBinsMask_[binId] != abuChangedBinsCounter_) {
          abuChangedBinsMask_[binId] = abuChangedBinsCounter_;
          abuChangedBins_.push_back(binId);

          // Record original usage the first time we observe that this bin is
          // going to change usage.
          abuBins_[binId].c_util = abuBins_[binId].m_util;
        }
        const double common_area = (hx - lx) * (hy - ly);
        abuBins_[binId].m_util += common_area * addSub;
      }
    }
  }
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void DetailedABU::acceptBins()
{
  abuChangedBins_.clear();
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void DetailedABU::rejectBins()
{
  for (int binId : abuChangedBins_) {
    const double space = abuBins_[binId].area - abuBins_[binId].f_util;

    // Remove from current bucket.
    const double util_m = abuBins_[binId].m_util;
    const int ix_m = getBucketId(binId, util_m);
    if (ix_m != -1) {
      auto it = utilBuckets_[ix_m].find(binId);
      if (it == utilBuckets_[ix_m].end()) {
        mgrPtr_->internalError(
            "Error rejecting bins for utilization objective");
      }
      utilBuckets_[ix_m].erase(it);
      utilTotals_[ix_m] -= util_m / space;
    }

    // Insert into original bucket.
    const double util_c = abuBins_[binId].c_util;
    const int ix_c = getBucketId(binId, util_c);
    if (ix_c != -1) {
      utilBuckets_[ix_c].insert(binId);
      utilTotals_[ix_c] += util_c / space;
    }

    // Restore original utilization.
    abuBins_[binId].m_util = abuBins_[binId].c_util;
  }

  abuChangedBins_.clear();
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void DetailedABU::clearBins()
{
  abuChangedBinsCounter_ = 0;
  abuChangedBinsMask_.resize(abuNumBins_);
  std::ranges::fill(abuChangedBinsMask_,

                    abuChangedBinsCounter_);
  ++abuChangedBinsCounter_;
  abuChangedBins_.reserve(abuNumBins_);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void DetailedABU::clearBuckets()
{
  utilBuckets_.resize(10);
  utilTotals_.resize(10);
  std::ranges::fill(utilTotals_, 0.0);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void DetailedABU::accept()
{
  acceptBins();
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void DetailedABU::reject()
{
  rejectBins();
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
double DetailedABU::freeSpaceThreshold()
{
  return FREE_SPACE_THRESHOLD;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
double DetailedABU::binAreaThreshold()
{
  return BIN_AREA_THRESHOLD;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
double DetailedABU::alpha()
{
  return ABU_ALPHA;
}

}  // namespace dpl
