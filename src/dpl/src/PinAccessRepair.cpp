// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <exception>
#include <tuple>
#include <vector>

#include "dpl/Opendp.h"
#include "drt/PinAccessService.h"
#include "infrastructure/Grid.h"
#include "infrastructure/Objects.h"
#include "infrastructure/network.h"
#include "utl/Logger.h"

namespace dpl {

class PinAccessRepair
{
 public:
  PinAccessRepair(Opendp& dp, drt::PinAccessService* pa, int dx, int dy)
      : dp_(dp), pa_(pa), max_displacement_x_(dx), max_displacement_y_(dy)
  {
  }

  int run(int max_rounds)
  {
    initialize();
    auto failures = scan();
    for (int round = 0; round < max_rounds && !failures.empty(); ++round) {
      std::vector<Move> moves;
      try {
        for (Node* cell : failures) {
          search(cell, moves);
        }
        if (moves.empty()) {
          break;
        }
        for (const Move& move : moves) {
          const odb::Point location = dp_.getDplLocation(move.cell);
          move.cell->getDbInst()->setLocation(location.x(), location.y());
        }
        pa_->updateDirtyPinAccess();
        // A move can change access for neighbors outside the moved batch.
        auto next = scan();
        const bool healthy_preserved = std::ranges::includes(
            failures, next, [](const Node* lhs, const Node* rhs) {
              return lhs->getDbInst()->getId() < rhs->getDbInst()->getId();
            });
        if (next.size() < failures.size() && healthy_preserved) {
          failures = std::move(next);
          continue;
        }
      } catch (...) {
        const auto error = std::current_exception();
        restorePlacement(moves);
        try {
          pa_->updateDirtyPinAccess();
        } catch (const std::exception& rollback_error) {
          dp_.logger_->warn(utl::DPL,
                            513,
                            "Pin access repair restored placement, but pin "
                            "access refresh failed: {}",
                            rollback_error.what());
        }
        std::rethrow_exception(error);
      }
      restorePlacement(moves);
      pa_->updateDirtyPinAccess();
      break;
    }
    if (!failures.empty()) {
      dp_.logger_->warn(utl::DPL,
                        514,
                        "Pin access repair stopped with {} unresolved cells.",
                        failures.size());
    }
    return failures.size();
  }

 private:
  struct Move
  {
    Node* cell;
    GridPt old;
    odb::Point location;
    odb::dbOrientType orient;
  };

  void initialize()
  {
    dp_.initGrid(true);
    dp_.groupInitPixels2();
    dp_.groupInitPixels();
    dp_.groupAssignCellRegions();
    dp_.setFixedGridCells();
    for (const auto& node : dp_.network_->getNodes()) {
      if (node->getType() == Node::CELL) {
        if (!node->isFixed()) {
          dp_.grid_->paintPixel(node.get());
        }
        if (node->isStdCell()) {
          cells_.push_back(node.get());
        }
      }
    }
    std::ranges::sort(cells_, [](const Node* lhs, const Node* rhs) {
      return lhs->getDbInst()->getId() < rhs->getDbInst()->getId();
    });
  }

  std::vector<Node*> scan()
  {
    std::vector<odb::dbInst*> instances;
    instances.reserve(cells_.size());
    for (Node* cell : cells_) {
      instances.push_back(cell->getDbInst());
    }
    std::vector<Node*> failures;
    for (auto* inst : pa_->findPinAccessFailures(instances)) {
      failures.push_back(dp_.network_->getNode(inst));
    }
    return failures;
  }

  std::vector<std::tuple<int64_t, int, int>> candidateSites(Node* cell)
  {
    const int x0 = dp_.grid_->gridX(cell->getOrigLeft()).v;
    const int y0 = dp_.grid_->gridSnapDownY(cell->getOrigBottom()).v;
    std::vector<std::tuple<int64_t, int, int>> candidates;
    // Grid-bounded results fit in int; widen the displacement arithmetic.
    const int xlow = std::max<int64_t>(0, int64_t{x0} - max_displacement_x_);
    const int xhigh = std::min<int64_t>(dp_.grid_->getRowSiteCount().v - 1,
                                        int64_t{x0} + max_displacement_x_);
    const int ylow = std::max<int64_t>(0, int64_t{y0} - max_displacement_y_);
    const int yhigh = std::min<int64_t>(dp_.grid_->getRowCount().v - 1,
                                        int64_t{y0} + max_displacement_y_);
    for (int y = ylow; y <= yhigh; ++y) {
      for (int x = xlow; x <= xhigh; ++x) {
        const int64_t distance
            = std::abs(int64_t{x} - dp_.grid_->gridX(cell).v)
                  * dp_.grid_->getSiteWidth().v
              + std::abs(int64_t{dp_.grid_->gridYToDbu(GridY{y}).v}
                         - cell->getBottom().v);
        candidates.emplace_back(distance, y, x);
      }
    }
    std::ranges::sort(candidates);
    return candidates;
  }

  bool legalCandidate(Node* cell, GridX gx, GridY gy, odb::dbOrientType orient)
  {
    const auto site_orient
        = dp_.grid_->getSiteOrientation(gx, gy, cell->getSite());
    return site_orient
           && (*site_orient == orient
               || (!orient.isRightAngleRotation()
                   && *site_orient == orient.flipY()))
           && dp_.checkPixels(cell,
                              gx,
                              gy,
                              gx + dp_.grid_->gridWidth(cell),
                              dp_.grid_->gridEndY(dp_.grid_->gridYToDbu(gy)
                                                  + cell->getHeight()),
                              &orient);
  }

  void search(Node* cell, std::vector<Move>& moves)
  {
    if (cell->isFixed()) {
      return;
    }
    auto* inst = cell->getDbInst();
    const GridPt old{dp_.grid_->gridX(cell), dp_.grid_->gridSnapDownY(cell)};
    const auto orient = cell->getOrient();
    const auto candidates = candidateSites(cell);
    // Record the active trial so query errors restore the entire batch.
    moves.push_back({cell, old, inst->getLocation(), orient});
    dp_.unplaceCell(cell);
    for (const auto& candidate : candidates) {
      const GridX gx{std::get<2>(candidate)};
      const GridY gy{std::get<1>(candidate)};
      if (gx == old.x && gy == old.y) {
        continue;
      }
      if (!legalCandidate(cell, gx, gy, orient)) {
        continue;
      }
      const odb::Point location(
          dp_.core_.xMin() + gx.v * dp_.grid_->getSiteWidth().v,
          dp_.core_.yMin() + dp_.grid_->gridYToDbu(gy).v);
      if (!pa_->hasUsablePinAccess(inst, location)) {
        continue;
      }
      dp_.placeCell(cell, gx, gy);
      cell->setOrient(orient);
      return;
    }
    dp_.placeCell(cell, old.x, old.y);
    cell->setOrient(orient);
    moves.pop_back();
  }

  void restorePlacement(const std::vector<Move>& moves)
  {
    // A later move may occupy an earlier cell's source site.
    for (const Move& move : moves) {
      dp_.unplaceCell(move.cell);
    }
    for (const Move& move : moves) {
      dp_.placeCell(move.cell, move.old.x, move.old.y);
      move.cell->setOrient(move.orient);
      move.cell->getDbInst()->setLocation(move.location.x(), move.location.y());
    }
  }

  Opendp& dp_;
  drt::PinAccessService* pa_;
  int max_displacement_x_;
  int max_displacement_y_;
  std::vector<Node*> cells_;
};

int Opendp::repairPinAccess(drt::PinAccessService* pa,
                            const int max_displacement_x,
                            const int max_displacement_y,
                            const int max_rounds)
{
  if (pa == nullptr || !pa->hasPinAccess()) {
    logger_->error(
        utl::DPL,
        510,
        "repair_pin_access requires successful current live pin access.");
  }
  if (max_displacement_x < 0 || max_displacement_y < 0 || max_rounds < 1) {
    logger_->error(utl::DPL,
                   511,
                   "Pin access repair requires nonnegative displacement and "
                   "positive rounds.");
  }
  if (db_->getChip() == nullptr || db_->getChip()->getBlock() == nullptr) {
    logger_->error(utl::DPL, 512, "Pin access repair requires a loaded block.");
  }
  if (db_->getChip()->getBlock()->getRows().empty()) {
    logger_->error(utl::DPL, 515, "Pin access repair requires placement rows.");
  }
  pa->updateDirtyPinAccess();
  try {
    // Fixed physical cells can lie outside placement rows.
    checkPlacement(false, "", false, true);
    const int remaining
        = PinAccessRepair(*this, pa, max_displacement_x, max_displacement_y)
              .run(max_rounds);
    grid_->releasePixels();
    return remaining;
  } catch (...) {
    grid_->releasePixels();
    throw;
  }
}

}  // namespace dpl
