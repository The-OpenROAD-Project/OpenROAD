// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// Checks the weighted-average (WA) wirelength gradient against the WA
// function it is the gradient of.
//
// For one net and one axis, with gamma = 1 / wlCoeff,
//   W = sum(x e^(x/g)) / sum(e^(x/g)) - sum(x e^(-x/g)) / sum(e^(-x/g)),
// a smooth stand-in for max(x) - min(x).  The test computes W in double
// precision from its own copy of the pin locations, differentiates it
// numerically, and compares against
// NesterovBaseCommon::getWireLengthGradientWA, which returns -dW/dx (the
// soft-min term's derivative minus the soft-max term's).

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <map>
#include <memory>
#include <numeric>
#include <random>
#include <string>
#include <vector>

#include "gpl/Replace.h"
#include "gtest/gtest.h"
#include "odb/db.h"
#include "odb/dbTypes.h"
#include "odb/geom.h"
#include "src/gpl/src/nesterovBase.h"
#include "src/gpl/src/placerBase.h"
#include "src/gpl/src/point.h"
#include "utl/Logger.h"
#include "utl/deleter.h"

namespace gpl {
namespace {

class WirelengthGradientTest : public ::testing::Test
{
 protected:
  static constexpr int kDbuPerMicron = 1000;
  static constexpr int kSiteWidth = 200;
  static constexpr int kRowHeight = 2000;
  static constexpr int kRowSites = 100;
  static constexpr int kRows = 10;
  static constexpr int kCellWidth = 2 * kSiteWidth;
  static constexpr int kCellCount = 20;
  static constexpr int kNetCount = 30;
  static constexpr int kMaxPinsPerNet = 5;

  struct Net
  {
    std::vector<int> cells;  // indices into centers_
  };

  // Builds kCellCount cells at distinct x and distinct y centers and
  // kNetCount random nets of 2..kMaxPinsPerNet pins on distinct cells.  Pins
  // have no shapes, so gpl puts each one at its cell's center.
  void SetUp() override
  {
    db_ = utl::UniquePtrWithDeleter<odb::dbDatabase>(odb::dbDatabase::create(),
                                                     odb::dbDatabase::destroy);
    db_->setLogger(&logger_);
    db_->setDbuPerMicron(kDbuPerMicron);

    odb::dbTech* tech = odb::dbTech::create(db_.get(), "tech");
    odb::dbTechLayer::create(tech, "metal1", odb::dbTechLayerType::ROUTING);
    odb::dbLib* lib = odb::dbLib::create(db_.get(), "lib", tech);
    odb::dbSite* site = odb::dbSite::create(lib, "site");
    site->setWidth(kSiteWidth);
    site->setHeight(kRowHeight);
    site->setClass(odb::dbSiteClass::CORE);

    odb::dbChip* chip = odb::dbChip::create(db_.get(), tech);
    odb::dbBlock* block = odb::dbBlock::create(chip, "top");
    block->setDieArea(odb::Rect(
        0, 0, (kRowSites + 2) * kSiteWidth, (kRows + 2) * kRowHeight));
    for (int row = 0; row < kRows; ++row) {
      odb::dbRow::create(block,
                         ("row" + std::to_string(row)).c_str(),
                         site,
                         kSiteWidth,
                         (row + 1) * kRowHeight,
                         odb::dbOrientType::R0,
                         odb::dbRowDir::HORIZONTAL,
                         kRowSites,
                         kSiteWidth);
    }
    block->setCoreArea(block->computeCoreArea());

    odb::dbMaster* master = odb::dbMaster::create(lib, "core_cell");
    master->setType(odb::dbMasterType(odb::dbMasterType::CORE));
    master->setWidth(kCellWidth);
    master->setHeight(kRowHeight);
    master->setSite(site);
    for (int p = 0; p < kNetCount; ++p) {
      odb::dbMTerm::create(master,
                           ("p" + std::to_string(p)).c_str(),
                           odb::dbIoType::INOUT,
                           odb::dbSigType::SIGNAL);
    }
    master->setFrozen();

    std::mt19937 rng(1);
    std::vector<int> x_slots(kCellCount);
    std::vector<int> y_slots(kCellCount);
    std::iota(x_slots.begin(), x_slots.end(), 0);
    std::iota(y_slots.begin(), y_slots.end(), 0);
    std::shuffle(x_slots.begin(), x_slots.end(), rng);
    std::shuffle(y_slots.begin(), y_slots.end(), rng);

    std::vector<odb::dbInst*> insts;
    for (int i = 0; i < kCellCount; ++i) {
      const std::string name = "u_" + std::to_string(i);
      odb::dbInst* inst = odb::dbInst::create(block, master, name.c_str());
      const int lx = 2 * kSiteWidth + x_slots[i] * 900;
      const int ly = 2 * kRowHeight + y_slots[i] * 800;
      inst->setLocation(lx, ly);
      inst->setPlacementStatus(odb::dbPlacementStatus::PLACED);
      insts.push_back(inst);
      cell_index_[name] = i;
      centers_.push_back(odb::Point(lx + kCellWidth / 2, ly + kRowHeight / 2));
    }

    std::uniform_int_distribution<int> degree(2, kMaxPinsPerNet);
    std::vector<int> order(kCellCount);
    std::iota(order.begin(), order.end(), 0);
    for (int n = 0; n < kNetCount; ++n) {
      odb::dbNet* db_net
          = odb::dbNet::create(block, ("n" + std::to_string(n)).c_str());
      std::shuffle(order.begin(), order.end(), rng);
      Net net;
      net.cells.assign(order.begin(), order.begin() + degree(rng));
      for (int c : net.cells) {
        insts[c]->findITerm(("p" + std::to_string(n)).c_str())->connect(db_net);
      }
      nets_.push_back(net);
    }

    PlaceOptions options;
    const PlacerBaseVars pb_vars(options);
    pbc_ = std::make_shared<PlacerBaseCommon>(db_.get(), pb_vars, &logger_);
    const NesterovBaseVars nb_vars(options);
    nbc_ = std::make_shared<NesterovBaseCommon>(
        nb_vars, pbc_, &logger_, /* num_threads = */ 1, Clusters{});
  }

  // gpl's gradient for every cell, indexed like centers_.
  std::vector<FloatPoint> gplGradients(float wl_coeff)
  {
    nbc_->updateWireLengthForceWA(wl_coeff, wl_coeff);
    std::vector<FloatPoint> grads(kCellCount);
    for (const GCell* gcell : nbc_->getGCells()) {
      const std::string name = gcell->insts()[0]->dbInst()->getName();
      grads[cell_index_.at(name)]
          = nbc_->getWireLengthGradientWA(gcell, wl_coeff, wl_coeff);
    }
    return grads;
  }

  // WA wirelength of all nets along one axis, in double precision.
  // Shifting by the max / min keeps the exponentials in range without
  // changing the ratios.
  double waWirelength(const std::vector<double>& coords, double gamma) const
  {
    double total = 0;
    for (const Net& net : nets_) {
      double hi = -1e300;
      double lo = 1e300;
      for (int c : net.cells) {
        hi = std::max(hi, coords[c]);
        lo = std::min(lo, coords[c]);
      }
      double max_num = 0, max_den = 0, min_num = 0, min_den = 0;
      for (int c : net.cells) {
        const double e_max = std::exp((coords[c] - hi) / gamma);
        const double e_min = std::exp((lo - coords[c]) / gamma);
        max_num += coords[c] * e_max;
        max_den += e_max;
        min_num += coords[c] * e_min;
        min_den += e_min;
      }
      total += max_num / max_den - min_num / min_den;
    }
    return total;
  }

  // Central-difference dW/d(coord of cell c).
  double numericDerivative(std::vector<double> coords,
                           int c,
                           double gamma) const
  {
    const double h = 0.5;
    const double x = coords[c];
    coords[c] = x + h;
    const double up = waWirelength(coords, gamma);
    coords[c] = x - h;
    const double down = waWirelength(coords, gamma);
    return (up - down) / (2 * h);
  }

  std::vector<double> xs() const
  {
    std::vector<double> v;
    for (const odb::Point& p : centers_) {
      v.push_back(p.x());
    }
    return v;
  }

  std::vector<double> ys() const
  {
    std::vector<double> v;
    for (const odb::Point& p : centers_) {
      v.push_back(p.y());
    }
    return v;
  }

  utl::Logger logger_;
  utl::UniquePtrWithDeleter<odb::dbDatabase> db_;
  std::shared_ptr<PlacerBaseCommon> pbc_;
  std::shared_ptr<NesterovBaseCommon> nbc_;
  std::vector<odb::Point> centers_;
  std::map<std::string, int> cell_index_;
  std::vector<Net> nets_;
};

// gamma of 1000 DBU against pin spreads of up to ~17000 DBU keeps the
// dominant exponents in [-10, 0], where gpl's fastExp (a 1024th-power
// approximation) is within a fraction of a percent of exp.  The tolerance
// covers that approximation and float arithmetic; per-net gradients are
// bounded by about 1 in magnitude.
TEST_F(WirelengthGradientTest, GradientMatchesFiniteDifference)
{
  constexpr double kGamma = 1000;
  const std::vector<FloatPoint> grads = gplGradients(1.0f / kGamma);
  const std::vector<double> x = xs();
  const std::vector<double> y = ys();

  for (int c = 0; c < kCellCount; ++c) {
    EXPECT_NEAR(grads[c].x, -numericDerivative(x, c, kGamma), 0.02)
        << "cell " << c;
    EXPECT_NEAR(grads[c].y, -numericDerivative(y, c, kGamma), 0.02)
        << "cell " << c;
  }
}

// As gamma -> 0, WA becomes max - min (HPWL), whose derivative is +1 for the
// pin at a net's max and -1 for the pin at its min.  Pins are at least 800
// DBU apart in both axes, so with gamma = 10 every non-extreme pin's weight
// is below e^-80.
TEST_F(WirelengthGradientTest, SmallGammaApproachesHpwlSubgradient)
{
  constexpr double kGamma = 10;
  const std::vector<FloatPoint> grads = gplGradients(1.0f / kGamma);

  std::vector<FloatPoint> expected(kCellCount);
  for (const Net& net : nets_) {
    auto by_x
        = [this](int a, int b) { return centers_[a].x() < centers_[b].x(); };
    auto by_y
        = [this](int a, int b) { return centers_[a].y() < centers_[b].y(); };
    const auto [min_x, max_x]
        = std::minmax_element(net.cells.begin(), net.cells.end(), by_x);
    const auto [min_y, max_y]
        = std::minmax_element(net.cells.begin(), net.cells.end(), by_y);
    // gpl returns -dW/dx.
    expected[*min_x].x += 1;
    expected[*max_x].x -= 1;
    expected[*min_y].y += 1;
    expected[*max_y].y -= 1;
  }

  for (int c = 0; c < kCellCount; ++c) {
    EXPECT_NEAR(grads[c].x, expected[c].x, 1e-2) << "cell " << c;
    EXPECT_NEAR(grads[c].y, expected[c].y, 1e-2) << "cell " << c;
  }
}

// WA is translation invariant, so moving every pin of a net by the same
// amount leaves it unchanged: the per-pin derivatives of each net sum to
// zero.  Every pin here is on a movable cell, so the cell gradients sum to
// zero as well.  This holds for any exp approximation, so the tolerance only
// covers float rounding.
TEST_F(WirelengthGradientTest, GradientSumsToZeroForFreeNets)
{
  for (const double gamma : {100.0, 1000.0, 10000.0}) {
    const std::vector<FloatPoint> grads = gplGradients(1.0f / gamma);
    double sum_x = 0;
    double sum_y = 0;
    double magnitude = 0;
    for (const FloatPoint& g : grads) {
      sum_x += g.x;
      sum_y += g.y;
      magnitude += std::abs(g.x) + std::abs(g.y);
    }
    ASSERT_GT(magnitude, 1.0) << "gamma " << gamma;
    EXPECT_NEAR(sum_x, 0.0, 1e-3) << "gamma " << gamma;
    EXPECT_NEAR(sum_y, 0.0, 1e-3) << "gamma " << gamma;
  }
}

}  // namespace
}  // namespace gpl
