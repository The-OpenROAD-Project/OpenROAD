// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <exception>
#include <limits>
#include <map>
#include <set>
#include <tuple>
#include <utility>
#include <vector>

#include "boost/geometry/index/rtree.hpp"
#include "dpl/Opendp.h"
#include "drt/PinAccessService.h"
#include "infrastructure/Grid.h"
#include "infrastructure/Objects.h"
#include "infrastructure/network.h"
#include "odb/PtrSetMap.h"
#include "odb/dbShape.h"
#include "odb/dbTransform.h"
#include "odb/geom_boost.h"
#include "utl/Logger.h"

namespace dpl {
namespace {

struct AccessShape
{
  odb::dbTechLayer* layer;
  odb::Rect rect;
};

// Use conservative routing and default cut spacing for supply checks.
int spacing(odb::dbTechLayer* layer, const odb::Rect& lhs, const odb::Rect& rhs)
{
  if (layer->getType() == odb::dbTechLayerType::CUT) {
    return layer->getSpacing();
  }
  const int width1 = std::min(lhs.dx(), lhs.dy());
  const int width2 = std::min(rhs.dx(), rhs.dy());
  const int length = std::max({lhs.dx(), lhs.dy(), rhs.dx(), rhs.dy()});
  return std::max({layer->getSpacing(width1, length),
                   layer->getSpacing(width2, length),
                   layer->findTwSpacing(width1, width2, length)});
}

int clip(const int64_t value)
{
  return static_cast<int>(std::clamp<int64_t>(
      value, std::numeric_limits<int>::min(), std::numeric_limits<int>::max()));
}

bool conflict(const odb::Rect& lhs, const odb::Rect& rhs, const int space)
{
  if (lhs.overlaps(rhs)) {
    return true;
  }
  const int64_t dx = std::max<int64_t>(
      {0, int64_t{lhs.xMin()} - rhs.xMax(), int64_t{rhs.xMin()} - lhs.xMax()});
  const int64_t dy = std::max<int64_t>(
      {0, int64_t{lhs.yMin()} - rhs.yMax(), int64_t{rhs.yMin()} - lhs.yMax()});
  // Check the axes first to keep the squared distances within int64_t.
  return space > 0 && dx < space && dy < space
         && dx * dx + dy * dy < int64_t{space} * space;
}

class SupplyIndex
{
 public:
  explicit SupplyIndex(odb::dbBlock* block)
  {
    for (odb::dbNet* net : block->getNets()) {
      if (!net->isSpecial() || !net->getSigType().isSupply()) {
        continue;
      }
      for (odb::dbSWire* wire : net->getSWires()) {
        for (odb::dbSBox* box : wire->getWires()) {
          if (box->isVia()) {
            std::vector<odb::dbShape> shapes;
            box->getViaBoxes(shapes);
            for (const odb::dbShape& shape : shapes) {
              add(shape.getTechLayer(), shape.getBox(), net);
            }
          } else {
            add(box->getTechLayer(), box->getBox(), net);
          }
        }
      }
    }
    for (auto& [layer, data] : layers_) {
      auto values = std::move(data.values);
      data.index = Index(values.begin(), values.end());
    }
  }

  bool physicalClear(odb::dbInst* inst, const odb::Point& location)
  {
    auto& orientations = physical_[inst->getMaster()];
    auto [it, inserted]
        = orientations.try_emplace(inst->getOrient().getValue());
    if (inserted) {
      odb::Rect boundary;
      inst->getMaster()->getPlacementBoundary(boundary);
      odb::dbTransform transform(inst->getOrient());
      transform.apply(boundary);
      transform.setOffset({-boundary.xMin(), -boundary.yMin()});
      auto add = [&](odb::dbBox* box, odb::dbMTerm* term) {
        auto append = [&](odb::dbTechLayer* layer, odb::Rect rect) {
          if (layer != nullptr) {
            transform.apply(rect);
            it->second.push_back({{layer, rect}, term});
          }
        };
        if (box->isVia()) {
          std::vector<odb::dbShape> shapes;
          box->getViaBoxes(shapes);
          for (const auto& shape : shapes) {
            append(shape.getTechLayer(), shape.getBox());
          }
        } else {
          append(box->getTechLayer(), box->getBox());
        }
      };
      for (odb::dbBox* box : inst->getMaster()->getObstructions()) {
        add(box, nullptr);
      }
      for (odb::dbMTerm* term : inst->getMaster()->getMTerms()) {
        for (odb::dbMPin* pin : term->getMPins()) {
          for (odb::dbBox* box : pin->getGeometry()) {
            add(box, term);
          }
        }
      }
    }
    return std::ranges::none_of(it->second, [&](const auto& shape) {
      odb::dbNet* net = shape.second == nullptr
                            ? nullptr
                            : inst->getITerm(shape.second)->getNet();
      return blocked(shape.first, location, net);
    });
  }

  bool blocked(const AccessShape& shape,
               const odb::Point& offset,
               odb::dbNet* net)
  {
    const auto it = layers_.find(shape.layer);
    if (it == layers_.end()) {
      return false;
    }
    Layer& data = it->second;
    const auto dimensions = std::pair(shape.rect.dx(), shape.rect.dy());
    auto [halo_it, inserted] = data.halos.try_emplace(dimensions, 0);
    if (inserted) {
      for (const auto& [width, height] : data.dimensions) {
        halo_it->second = std::max(
            halo_it->second,
            spacing(shape.layer, odb::Rect(0, 0, width, height), shape.rect));
      }
    }
    odb::Rect rect = shape.rect;
    rect.moveDelta(offset.x(), offset.y());
    const int halo = halo_it->second;
    const odb::Rect query(clip(int64_t{rect.xMin()} - halo),
                          clip(int64_t{rect.yMin()} - halo),
                          clip(int64_t{rect.xMax()} + halo),
                          clip(int64_t{rect.yMax()} + halo));
    for (auto hit
         = data.index.qbegin(boost::geometry::index::intersects(query));
         hit != data.index.qend();
         ++hit) {
      if (hit->second != net
          && conflict(
              rect, hit->first, spacing(shape.layer, hit->first, rect))) {
        return true;
      }
    }
    return false;
  }

 private:
  using Value = std::pair<odb::Rect, odb::dbNet*>;
  using Index
      = boost::geometry::index::rtree<Value,
                                      boost::geometry::index::quadratic<16>>;
  struct Layer
  {
    std::vector<Value> values;
    std::set<std::pair<int, int>> dimensions;
    std::map<std::pair<int, int>, int> halos;
    Index index;
  };

  void add(odb::dbTechLayer* layer, const odb::Rect& rect, odb::dbNet* net)
  {
    if (layer != nullptr
        && (layer->getType() == odb::dbTechLayerType::ROUTING
            || layer->getType() == odb::dbTechLayerType::CUT)) {
      layers_[layer].values.emplace_back(rect, net);
      layers_[layer].dimensions.emplace(rect.dx(), rect.dy());
    }
  }
  odb::PtrMap<odb::dbTechLayer, Layer> layers_;
  odb::PtrMap<odb::dbMaster,
              std::map<int, std::vector<std::pair<AccessShape, odb::dbMTerm*>>>>
      physical_;
};

struct PinResult
{
  bool blocked = false;
  bool unsupported = false;
  bool missing = false;
  int selection_issues = 0;
};

class AccessRound
{
 public:
  AccessRound(SupplyIndex& supply,
              drt::PinAccessService* pa,
              int min_access_points)
      : supply_(supply), pa_(pa), min_access_points_(min_access_points)
  {
  }

  PinResult inspect(odb::dbInst* inst, const odb::Point& location)
  {
    PinResult result;
    for (odb::dbITerm* term : inst->getITerms()) {
      if (term->getNet() == nullptr || term->getSigType().isSupply()
          || !pa_->isPinAccessRequired(term)) {
        continue;
      }
      int available = 0;
      int surviving = 0;
      bool unsupported = false;
      for (const auto& [pin, aps] : term->getAccessPoints()) {
        for (odb::dbAccessPoint* ap : aps) {
          ++available;
          const Alternatives& choices = alternatives(ap);
          unsupported |= choices.unsupported;
          if (clear(choices, location, term->getNet())) {
            ++surviving;
            if (surviving == min_access_points_) {
              break;
            }
          }
        }
        if (surviving == min_access_points_) {
          break;
        }
      }
      const bool enough = surviving >= min_access_points_;
      result.missing |= available < min_access_points_;
      // Unsupported alternatives cannot establish that every escape is blocked.
      result.unsupported |= !enough && unsupported;
      result.blocked
          |= !enough && !unsupported && available >= min_access_points_;
      if (enough) {
        for (odb::dbAccessPoint* ap : term->getPrefAccessPoints()) {
          const Alternatives& choices = alternatives(ap);
          if (!choices.unsupported
              && !clear(choices, location, term->getNet())) {
            ++result.selection_issues;
          }
        }
      }
    }
    return result;
  }

 private:
  struct Alternatives
  {
    bool unsupported = false;
    std::vector<std::vector<AccessShape>> vias;
  };

  const Alternatives& alternatives(odb::dbAccessPoint* ap)
  {
    auto [it, inserted] = cache_.try_emplace(ap);
    if (!inserted) {
      return it->second;
    }
    Alternatives& result = it->second;
    if (!ap->hasAccess(odb::dbDirection::UP) || !ap->getSegments().empty()) {
      result.unsupported = true;
      return result;
    }
    const odb::Point point = ap->getPoint();
    for (const auto& group : ap->getVias()) {
      for (odb::dbObject* via : group) {
        std::vector<AccessShape> shapes;
        auto add_boxes = [&](const auto& boxes) {
          for (odb::dbBox* box : boxes) {
            odb::Rect rect = box->getBox();
            rect.moveDelta(point.x(), point.y());
            shapes.push_back({box->getTechLayer(), rect});
          }
        };
        if (via->getObjectType() == odb::dbTechViaObj) {
          add_boxes(static_cast<odb::dbTechVia*>(via)->getBoxes());
        } else {
          add_boxes(static_cast<odb::dbVia*>(via)->getBoxes());
        }
        bool bottom = false;
        bool cut = false;
        bool top = false;
        for (const AccessShape& shape : shapes) {
          if (shape.layer == nullptr || ap->getLayer() == nullptr) {
            continue;
          }
          bottom |= shape.layer == ap->getLayer();
          cut |= shape.layer->getType() == odb::dbTechLayerType::CUT;
          top |= shape.layer->getRoutingLevel()
                 == ap->getLayer()->getRoutingLevel() + 1;
        }
        if (bottom && cut && top) {
          result.vias.push_back(std::move(shapes));
        } else {
          result.unsupported = true;
        }
      }
    }
    result.unsupported |= result.vias.empty();
    return result;
  }

  bool clear(const Alternatives& choices,
             const odb::Point& location,
             odb::dbNet* net)
  {
    return std::ranges::any_of(choices.vias, [&](const auto& shapes) {
      return std::ranges::none_of(shapes, [&](const AccessShape& shape) {
        return supply_.blocked(shape, location, net);
      });
    });
  }

  SupplyIndex& supply_;
  drt::PinAccessService* pa_;
  int min_access_points_;
  odb::PtrMap<odb::dbAccessPoint, Alternatives> cache_;
};

bool unresolved(const PinResult& result)
{
  return result.blocked || result.missing || result.unsupported;
}

}  // namespace

class PinAccessRepair
{
 public:
  PinAccessRepair(Opendp& dp,
                  drt::PinAccessService* pa,
                  int dx,
                  int dy,
                  int min_access_points)
      : dp_(dp),
        pa(pa),
        max_displacement_x(dx),
        max_displacement_y(dy),
        min_access_points(min_access_points),
        supply(dp.block_)
  {
  }

  int run(int max_rounds, int recovery_attempts)
  {
    initialize();
    if (recovery_attempts > 0) {
      recover(recovery_attempts);
    }
    int remaining = 0;
    for (int round = 0; round < max_rounds; ++round) {
      AccessRound access(supply, pa, min_access_points);
      const auto failures = scan(access, round == 0);
      remaining = failures.size();
      if (failures.empty()) {
        break;
      }
      std::vector<Move> moves;
      for (Node* cell : failures) {
        search(cell, access, moves);
      }
      if (moves.empty()) {
        break;
      }
      const int count = commit(moves, remaining);
      if (count >= remaining) {
        break;
      }
      remaining = count;
    }
    if (remaining != 0) {
      dp_.logger_->warn(utl::DPL,
                        514,
                        "Pin access repair stopped with {} unresolved cells. "
                        "Review their pin access and supply geometry.",
                        remaining);
    }
    return remaining;
  }

 private:
  struct Move
  {
    Node* cell;
    GridPt old;
    odb::Point location;
    odb::dbOrientType orient;
  };

  using Missing = odb::PtrSet<odb::dbITerm>;

  Missing missing() const
  {
    const auto terms = pa->getMissingPinAccess();
    return {terms.begin(), terms.end()};
  }

  static bool missingCell(const Missing& terms, odb::dbInst* inst)
  {
    return std::ranges::any_of(inst->getITerms(), [&](odb::dbITerm* term) {
      return terms.contains(term);
    });
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

  void recover(int max_attempts)
  {
    std::vector<Move> moves;
    try {
      pa->beginPinAccessRecovery(min_access_points);
      Missing unresolved = missing();
      for (Node* cell : cells) {
        if (unresolved.empty()) {
          break;
        }
        odb::dbInst* inst = cell->getDbInst();
        if (cell->isFixed() || !missingCell(unresolved, inst)) {
          continue;
        }
        const auto orient = cell->getOrient();
        const GridPt old{dp_.grid_->gridX(cell),
                         dp_.grid_->gridSnapDownY(cell)};
        const Move move{cell, old, inst->getLocation(), orient};
        const auto candidates = candidateSites(cell);
        int attempts = 0;
        for (const auto& [distance, y, x] : candidates) {
          if (attempts == max_attempts) {
            break;
          }
          if (x == old.x.v && y == old.y.v) {
            continue;
          }
          // Retain the active trial so any error can restore the whole batch.
          moves.push_back(move);
          dp_.unplaceCell(cell);
          const GridX gx{x};
          const GridY gy{y};
          const odb::Point location(
              dp_.core_.xMin() + x * dp_.grid_->getSiteWidth().v,
              dp_.core_.yMin() + dp_.grid_->gridYToDbu(gy).v);
          if (!legalCandidate(cell, gx, gy, orient)
              || !supply.physicalClear(inst, location)) {
            dp_.placeCell(cell, old.x, old.y);
            cell->setOrient(orient);
            moves.pop_back();
            continue;
          }
          dp_.placeCell(cell, gx, gy);
          cell->setOrient(orient);
          inst->setLocation(location.x(), location.y());
          ++attempts;
          pa->updatePinAccessRecovery();
          Missing next = missing();
          const bool healthy_preserved = std::ranges::all_of(
              next,
              [&](odb::dbITerm* term) { return unresolved.contains(term); });
          if (!missingCell(next, inst) && healthy_preserved) {
            unresolved = std::move(next);
            break;
          }
          restorePlacement({move});
          moves.pop_back();
          pa->updatePinAccessRecovery();
          if (missing() != unresolved) {
            dp_.logger_->error(
                utl::DPL,
                518,
                "Pin access recovery could not restore the required-pin access "
                "state after a rejected trial.");
          }
        }
      }
      if (!unresolved.empty()) {
        dp_.logger_->error(
            utl::DPL,
            516,
            "Pin access recovery failed with {} required pins below {} "
            "access points; recovery placement will be "
            "restored and pin access is not ready.",
            unresolved.size(),
            min_access_points);
      }
      pa->finishPinAccessRecovery();
    } catch (...) {
      // Cancellation makes database move callbacks safe even after PA failed.
      try {
        pa->cancelPinAccessRecovery();
      } catch (const std::exception& error) {
        dp_.logger_->warn(utl::DPL,
                          517,
                          "Pin access recovery cancellation failed: {}",
                          error.what());
      }
      restorePlacement(moves);
      throw;
    }
  }

  void initialize()
  {
    dp_.initGrid();
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
          cells.push_back(node.get());
        }
      }
    }
    std::ranges::sort(cells, [](const Node* lhs, const Node* rhs) {
      return lhs->getDbInst()->getId() < rhs->getDbInst()->getId();
    });
  }

  std::vector<Node*> scan(AccessRound& access, bool report)
  {
    std::vector<Node*> failures;
    int missing = 0;
    int unsupported = 0;
    int selection = 0;
    for (Node* cell : cells) {
      const PinResult result
          = access.inspect(cell->getDbInst(), cell->getDbInst()->getLocation());
      if (unresolved(result)) {
        failures.push_back(cell);
        const auto inst = cell->getDbInst();
        const auto location = inst->getLocation();
        debugPrint(
            dp_.logger_,
            utl::DPL,
            "pin_access_repair",
            1,
            "Unresolved pin access {} at ({}, {}): blocked={} missing={} "
            "unsupported={} fixed={}",
            inst->getName(),
            location.x(),
            location.y(),
            result.blocked,
            result.missing,
            result.unsupported,
            cell->isFixed());
      }
      missing += result.missing;
      unsupported += result.unsupported;
      selection += result.selection_issues;
    }
    if (report) {
      dp_.logger_->report(
          "Pin access repair: {} unresolved cells; {} insufficient AP, {} "
          "unsupported first-hop, {} preferred AP selection issues.",
          failures.size(),
          missing,
          unsupported,
          selection);
    }
    return failures;
  }

  std::vector<std::tuple<int64_t, int, int>> candidateSites(Node* cell)
  {
    const int x0 = dp_.grid_->gridX(cell->getOrigLeft()).v;
    const int y0 = dp_.grid_->gridSnapDownY(cell->getOrigBottom()).v;
    std::vector<std::tuple<int64_t, int, int>> candidates;
    const int xlow = std::max<int64_t>(0, int64_t{x0} - max_displacement_x);
    const int xhigh = std::min<int64_t>(dp_.grid_->getRowSiteCount().v - 1,
                                        int64_t{x0} + max_displacement_x);
    const int ylow = std::max<int64_t>(0, int64_t{y0} - max_displacement_y);
    const int yhigh = std::min<int64_t>(dp_.grid_->getRowCount().v - 1,
                                        int64_t{y0} + max_displacement_y);
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

  void search(Node* cell, AccessRound& access, std::vector<Move>& moves)
  {
    if (cell->isFixed()) {
      return;
    }
    odb::dbInst* inst = cell->getDbInst();
    const odb::Point old_location = inst->getLocation();
    const PinResult result = access.inspect(inst, old_location);
    if (!result.blocked || result.missing || result.unsupported) {
      return;
    }
    const GridPt old{dp_.grid_->gridX(cell), dp_.grid_->gridSnapDownY(cell)};
    const auto orient = cell->getOrient();
    const auto candidates = candidateSites(cell);
    dp_.unplaceCell(cell);
    for (const auto& [distance, y, x] : candidates) {
      if (x == old.x.v && y == old.y.v) {
        continue;
      }
      const GridX gx{x};
      const GridY gy{y};
      // Keep orientation and its vertical power alignment unchanged.
      if (!legalCandidate(cell, gx, gy, orient)) {
        continue;
      }
      const odb::Point location(
          dp_.core_.xMin() + x * dp_.grid_->getSiteWidth().v,
          dp_.core_.yMin() + dp_.grid_->gridYToDbu(gy).v);
      if (!supply.physicalClear(inst, location)
          || unresolved(access.inspect(inst, location))) {
        continue;
      }
      dp_.placeCell(cell, gx, gy);
      cell->setOrient(orient);
      moves.push_back({cell, old, old_location, orient});
      return;
    }
    debugPrint(dp_.logger_,
               utl::DPL,
               "pin_access_repair",
               2,
               "No repair site for {} within displacement {} sites, {} rows.",
               inst->getName(),
               max_displacement_x,
               max_displacement_y);
    dp_.placeCell(cell, old.x, old.y);
    cell->setOrient(orient);
  }

  void restore(const std::vector<Move>& moves)
  {
    restorePlacement(moves);
    pa->updateDirtyPinAccess();
  }

  int commit(const std::vector<Move>& moves, int remaining)
  {
    try {
      for (const Move& move : moves) {
        const odb::Point location = dp_.getDplLocation(move.cell);
        move.cell->getDbInst()->setLocation(location.x(), location.y());
      }
      pa->updateDirtyPinAccess();
      // AP objects may have been replaced by the native dirty-class refresh.
      AccessRound fresh(supply, pa, min_access_points);
      const int count = scan(fresh, true).size();
      if (count < remaining) {
        return count;
      }
    } catch (...) {
      try {
        restore(moves);
      } catch (const std::exception& error) {
        dp_.logger_->warn(utl::DPL,
                          513,
                          "Pin access repair rollback failed: {}",
                          error.what());
      }
      throw;
    }
    restore(moves);
    return remaining;
  }

  Opendp& dp_;
  drt::PinAccessService* pa;
  int max_displacement_x;
  int max_displacement_y;
  int min_access_points;
  SupplyIndex supply;
  std::vector<Node*> cells;
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
    logger_->error(utl::DPL,
                   511,
                   "Pin access repair requires nonnegative displacement and "
                   "attempts, and positive rounds and minimum access points.");
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
  // Fixed physical cells can lie outside placement rows.
  checkPlacement(false, "", false, true);
  return PinAccessRepair(*this,
                         pa,
                         max_displacement_x,
                         max_displacement_y,
                         min_access_points)
      .run(max_rounds, recovery_attempts);
}

}  // namespace dpl
