// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023-2026, The OpenROAD Authors

#include "checker.h"

#include <algorithm>
#include <cstddef>
#include <numeric>
#include <string>
#include <unordered_map>
#include <vector>

#include "odb/db.h"
#include "odb/dbObject.h"
#include "odb/geom.h"
#include "unfoldedModel.h"
#include "utl/Logger.h"
#include "utl/unionFind.h"

namespace odb {

Checker::Checker(utl::Logger* logger) : logger_(logger)
{
}

void Checker::check(dbChip* chip)
{
  UnfoldedModel model(logger_, chip);
  auto* top_cat = dbMarkerCategory::createOrReplace(chip, "3DBlox");
  auto* conn_cat = dbMarkerCategory::createOrReplace(top_cat, "Connectivity");

  checkFloatingChips(model, conn_cat);
}

void Checker::checkFloatingChips(const UnfoldedModel& model,
                                 dbMarkerCategory* category)
{
  const auto& chips = model.getChips();
  utl::UnionFind uf(chips.size());
  std::unordered_map<const UnfoldedChip*, int> chip_map;
  for (size_t i = 0; i < chips.size(); ++i) {
    chip_map[&chips[i]] = (int) i;
  }

  for (const auto& conn : model.getConnections()) {
    if (isValid(conn) && conn.top_region && conn.bottom_region) {
      auto it1 = chip_map.find(conn.top_region->parent_chip);
      auto it2 = chip_map.find(conn.bottom_region->parent_chip);
      if (it1 != chip_map.end() && it2 != chip_map.end()) {
        uf.unite(it1->second, it2->second);
      }
    }
  }

  std::vector<int> sorted(chips.size());
  std::iota(sorted.begin(), sorted.end(), 0);
  std::ranges::sort(sorted, [&](int a, int b) {
    return chips[a].cuboid.xMin() < chips[b].cuboid.xMin();
  });

  for (size_t i = 0; i < sorted.size(); ++i) {
    const auto& c1 = chips[sorted[i]].cuboid;
    for (size_t j = i + 1; j < sorted.size(); ++j) {
      if (chips[sorted[j]].cuboid.xMin() > c1.xMax()) {
        break;
      }
      if (c1.intersects(chips[sorted[j]].cuboid)) {
        uf.unite(sorted[i], sorted[j]);
      }
    }
  }

  std::vector<std::vector<const UnfoldedChip*>> groups(chips.size());
  for (size_t i = 0; i < chips.size(); ++i) {
    groups[uf.find((int) i)].push_back(&chips[i]);
  }
  std::erase_if(groups, [](const auto& g) { return g.empty(); });

  if (groups.size() > 1) {
    std::ranges::sort(groups, [](const auto& a, const auto& b) {
      return a.size() > b.size();
    });
    auto* cat = dbMarkerCategory::createOrReplace(category, "Floating chips");
    logger_->warn(
        utl::ODB, 151, "Found {} floating chip sets", groups.size() - 1);
    for (size_t i = 1; i < groups.size(); ++i) {
      auto* marker = dbMarker::create(cat);
      for (auto* chip : groups[i]) {
        marker->addShape(Rect(chip->cuboid.xMin(),
                              chip->cuboid.yMin(),
                              chip->cuboid.xMax(),
                              chip->cuboid.yMax()));
        marker->addSource(chip->chip_inst_path.back());
      }
      marker->setComment("Isolated chip set starting with "
                         + groups[i][0]->name);
    }
  }
}

Checker::ContactSurfaces Checker::getContactSurfaces(
    const UnfoldedConnection& conn) const
{
  auto* r1 = conn.top_region;
  auto* r2 = conn.bottom_region;
  if (!r1 || !r2) {
    return {.valid = false, .top_z = 0, .bot_z = 0};
  }

  auto up = [](auto* r) { return r->isTop() || r->isInternal(); };
  auto down = [](auto* r) { return r->isBottom() || r->isInternal(); };

  bool r1_down_r2_up = down(r1) && up(r2);
  bool r1_up_r2_down = up(r1) && down(r2);

  if (!r1_down_r2_up && !r1_up_r2_down) {
    return {.valid = false, .top_z = 0, .bot_z = 0};
  }

  if (r1_down_r2_up && r1_up_r2_down) {
    if (r1->cuboid.zCenter() < r2->cuboid.zCenter()) {
      r1_down_r2_up = false;
    } else {
      r1_up_r2_down = false;
    }
  }

  auto* top = r1_down_r2_up ? r1 : r2;
  auto* bot = r1_down_r2_up ? r2 : r1;
  int top_z = top->isInternal() ? top->cuboid.zMin() : top->getSurfaceZ();
  int bot_z = bot->isInternal() ? bot->cuboid.zMax() : bot->getSurfaceZ();
  return {.valid = true, .top_z = top_z, .bot_z = bot_z};
}

bool Checker::isValid(const UnfoldedConnection& conn) const
{
  if (!conn.top_region || !conn.bottom_region) {
    return true;
  }
  if (!conn.top_region->cuboid.xyIntersects(conn.bottom_region->cuboid)) {
    return false;
  }
  if (conn.top_region->isInternalExt() || conn.bottom_region->isInternalExt()) {
    return true;
  }

  if ((conn.top_region->isInternal() || conn.bottom_region->isInternal())
      && std::max(conn.top_region->cuboid.zMin(),
                  conn.bottom_region->cuboid.zMin())
             <= std::min(conn.top_region->cuboid.zMax(),
                         conn.bottom_region->cuboid.zMax())) {
    return true;
  }

  auto surfaces = getContactSurfaces(conn);
  if (!surfaces.valid) {
    return false;
  }
  if (surfaces.top_z < surfaces.bot_z) {
    return false;
  }
  return (surfaces.top_z - surfaces.bot_z) == conn.connection->getThickness();
}

}  // namespace odb
