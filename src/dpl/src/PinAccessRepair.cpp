// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <exception>
#include <map>
#include <set>
#include <tuple>
#include <vector>

#include "dpl/Opendp.h"
#include "drt/PinAccessService.h"
#include "infrastructure/Grid.h"
#include "infrastructure/Objects.h"
#include "infrastructure/network.h"
#include "odb/PtrSetMap.h"
#include "utl/Logger.h"

namespace dpl {

class PinAccessRepair
{
 public:
  PinAccessRepair(Opendp& dp, drt::PinAccessService* pa, int dx, int dy)
      : dp_(dp), pa_(pa), max_displacement_x_(dx), max_displacement_y_(dy)
  {
  }

  int run(const int max_rounds,
          const int recovery_attempts,
          const int min_access_points)
  {
    initialize();
    if (recovery_attempts > 0) {
      recover(recovery_attempts, min_access_points);
    }
    const bool preserve_minimum
        = recovery_attempts > 0 && min_access_points > 1;
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
        writeLocations(moves);
        pa_->updateDirtyPinAccess();
        if (!preserve_minimum || pa_->hasMinimumPinAccess(min_access_points)) {
          // A move can change access for neighbors outside the moved batch.
          auto next = scan();
          const bool healthy_preserved = subset(next, failures);
          if (next.size() < failures.size() && healthy_preserved) {
            failures = std::move(next);
            continue;
          }
        }
      } catch (...) {
        const auto error = std::current_exception();
        restorePlacement(moves);
        try {
          pa_->updateDirtyPinAccess();
          if (preserve_minimum
              && !pa_->hasMinimumPinAccess(min_access_points)) {
            dp_.logger_->warn(
                utl::DPL,
                519,
                "Pin access repair restored placement, but the native access "
                "point minimum {} was not restored.",
                min_access_points);
          }
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
      if (preserve_minimum && !pa_->hasMinimumPinAccess(min_access_points)) {
        dp_.logger_->error(utl::DPL,
                           520,
                           "Pin access repair could not restore the native "
                           "access point minimum {} after restoring placement.",
                           min_access_points);
      }
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

  struct RecoveryTrials
  {
    int attempts = 0;
    std::set<std::pair<int, int>> sites;
  };

  static bool subset(const odb::PtrSet<odb::dbITerm>& next,
                     const odb::PtrSet<odb::dbITerm>& previous)
  {
    return std::ranges::includes(previous, next, previous.key_comp());
  }

  static bool subset(const std::vector<Node*>& next,
                     const std::vector<Node*>& previous)
  {
    return std::ranges::includes(
        previous, next, [](const Node* lhs, const Node* rhs) {
          return lhs->getDbInst()->getId() < rhs->getDbInst()->getId();
        });
  }

  void writeLocations(const std::vector<Move>& moves)
  {
    for (const Move& move : moves) {
      const odb::Point location = dp_.getDplLocation(move.cell);
      move.cell->getDbInst()->setLocation(location.x(), location.y());
    }
  }

  void recover(const int max_attempts, const int min_access_points)
  {
    std::vector<Move> originals;
    std::map<Node*, RecoveryTrials> trials;
    try {
      const auto initial = pa_->beginPinAccessRecovery(min_access_points);
      odb::PtrSet<odb::dbITerm> missing(initial.begin(), initial.end());
      if (missing.empty()) {
        if (!pa_->hasPinAccess()) {
          dp_.logger_->error(
              utl::DPL, 517, "Pin access recovery did not publish access.");
        }
        return;
      }
      auto failures = scan();
      while (!missing.empty()) {
        std::set<Node*> missing_cells;
        for (auto* pin : missing) {
          missing_cells.insert(dp_.network_->getNode(pin->getInst()));
        }
        std::vector<Move> moves;
        for (Node* cell : cells_) {
          if (missing_cells.contains(cell) && !cell->isFixed()) {
            recoverySearch(cell, max_attempts, trials[cell], originals, moves);
          }
        }
        if (moves.empty()) {
          break;
        }
        writeLocations(moves);
        const auto updated = pa_->updatePinAccessRecovery();
        odb::PtrSet<odb::dbITerm> next(updated.begin(), updated.end());
        auto next_failures = scan();
        if (next.size() < missing.size() && subset(next, missing)
            && subset(next_failures, failures)) {
          missing = std::move(next);
          failures = std::move(next_failures);
          continue;
        }
        restorePlacement(moves);
        const auto restored = pa_->updatePinAccessRecovery();
        if (odb::PtrSet<odb::dbITerm>(restored.begin(), restored.end())
                != missing
            || !subset(scan(), failures)) {
          dp_.logger_->error(
              utl::DPL,
              518,
              "Pin access recovery could not restore its previous access.");
        }
      }
      if (!missing.empty()) {
        dp_.logger_->error(
            utl::DPL,
            516,
            "Pin access recovery stopped with {} pins below {} native access "
            "points after at most {} attempts per cell.",
            missing.size(),
            min_access_points,
            max_attempts);
      }
      pa_->finishPinAccessRecovery();
    } catch (...) {
      // Detach native callbacks before restoring placement after a failed
      // recovery.
      pa_->cancelPinAccessRecovery();
      restorePlacement(originals);
      throw;
    }
  }

  void recoverySearch(Node* cell,
                      const int max_attempts,
                      RecoveryTrials& trials,
                      std::vector<Move>& originals,
                      std::vector<Move>& moves)
  {
    if (trials.attempts >= max_attempts) {
      return;
    }
    auto* inst = cell->getDbInst();
    const GridPt old{dp_.grid_->gridX(cell), dp_.grid_->gridSnapDownY(cell)};
    const auto orient = cell->getOrient();
    const auto candidates = candidateSites(cell);
    if (trials.sites.empty()) {
      trials.sites.emplace(old.x.v, old.y.v);
      originals.push_back({cell, old, inst->getLocation(), orient});
    }
    // Record the active trial before unplacing so every exception can roll
    // back.
    moves.push_back({cell, old, inst->getLocation(), orient});
    dp_.unplaceCell(cell);
    for (const auto& candidate : candidates) {
      const GridX gx{std::get<2>(candidate)};
      const GridY gy{std::get<1>(candidate)};
      if (trials.sites.contains({gx.v, gy.v})
          || !legalCandidate(cell, gx, gy, orient)) {
        continue;
      }
      trials.sites.emplace(gx.v, gy.v);
      ++trials.attempts;
      dp_.placeCell(cell, gx, gy);
      cell->setOrient(orient);
      return;
    }
    dp_.placeCell(cell, old.x, old.y);
    cell->setOrient(orient);
    moves.pop_back();
  }

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
                            const int max_rounds,
                            const int recovery_attempts,
                            const int min_access_points)
{
  if (pa == nullptr || (recovery_attempts == 0 && !pa->hasPinAccess())) {
    logger_->error(
        utl::DPL,
        510,
        "repair_pin_access requires successful current live pin access.");
  }
  if (max_displacement_x < 0 || max_displacement_y < 0 || max_rounds < 1
      || recovery_attempts < 0 || min_access_points < 1) {
    logger_->error(
        utl::DPL,
        511,
        "Pin access repair requires nonnegative displacement and "
        "positive rounds, nonnegative recovery attempts, and a positive "
        "native access point minimum.");
  }
  if (db_->getChip() == nullptr || db_->getChip()->getBlock() == nullptr) {
    logger_->error(utl::DPL, 512, "Pin access repair requires a loaded block.");
  }
  if (db_->getChip()->getBlock()->getRows().empty()) {
    logger_->error(utl::DPL, 515, "Pin access repair requires placement rows.");
  }
  if (recovery_attempts == 0) {
    pa->updateDirtyPinAccess();
  }
  try {
    // Fixed physical cells can lie outside placement rows.
    checkPlacement(false, "", false, true);
    const int remaining
        = PinAccessRepair(*this, pa, max_displacement_x, max_displacement_y)
              .run(max_rounds, recovery_attempts, min_access_points);
    grid_->releasePixels();
    return remaining;
  } catch (...) {
    grid_->releasePixels();
    throw;
  }
}

}  // namespace dpl
