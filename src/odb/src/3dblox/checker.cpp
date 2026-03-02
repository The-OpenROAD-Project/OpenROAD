// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023-2026, The OpenROAD Authors

#include "checker.h"

#include <algorithm>
#include <cstddef>
#include <filesystem>
#include <memory>
#include <numeric>
#include <ranges>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "odb/db.h"
#include "odb/dbObject.h"
#include "odb/geom.h"
#include "odb/unfoldedModel.h"
#include "sta/ConcreteNetwork.hh"
#include "sta/Network.hh"
#include "sta/NetworkClass.hh"
#include "sta/PatternMatch.hh"
#include "sta/Sta.hh"
#include "sta/VerilogReader.hh"
#include "utl/Logger.h"
#include "utl/unionFind.h"

namespace odb {

namespace {

const char* sideToString(UnfoldedRegionSide side)
{
  switch (side) {
    case UnfoldedRegionSide::TOP:
      return "TOP";
    case UnfoldedRegionSide::BOTTOM:
      return "BOTTOM";
    case UnfoldedRegionSide::INTERNAL:
      return "INTERNAL";
    case UnfoldedRegionSide::INTERNAL_EXT:
      return "INTERNAL_EXT";
  }
  return "UNKNOWN";
}

MatingSurfaces getMatingSurfaces(const UnfoldedConnection& conn)
{
  auto* r1 = conn.top_region;
  auto* r2 = conn.bottom_region;
  if (!r1 || !r2) {
    return {.valid = false, .top_z = 0, .bot_z = 0};
  }

  // r1 faces down (Bottom side) and r2 faces up (Top side) -> r1 is above r2
  bool r1_down_r2_up = r1->isBottom() && r2->isTop();
  // r1 faces up (Top side) and r2 faces down (Bottom side) -> r2 is above r1
  bool r1_up_r2_down = r1->isTop() && r2->isBottom();

  if (r1_down_r2_up == r1_up_r2_down) {
    return {.valid = false, .top_z = 0, .bot_z = 0};
  }

  auto* top = r1_down_r2_up ? r1 : r2;
  auto* bot = r1_down_r2_up ? r2 : r1;
  return {
      .valid = true, .top_z = top->getSurfaceZ(), .bot_z = bot->getSurfaceZ()};
}

bool isValid(const UnfoldedConnection& conn)
{
  if (!conn.top_region || !conn.bottom_region) {
    return true;
  }
  if (!conn.top_region->cuboid.xyIntersects(conn.bottom_region->cuboid)) {
    return false;
  }
  if (conn.top_region->isInternalExt() || conn.bottom_region->isInternalExt()) {
    return std::max(conn.top_region->cuboid.zMin(),
                    conn.bottom_region->cuboid.zMin())
           <= std::min(conn.top_region->cuboid.zMax(),
                       conn.bottom_region->cuboid.zMax());
  }

  auto surfaces = getMatingSurfaces(conn);
  if (!surfaces.valid) {
    return false;
  }
  if (surfaces.top_z < surfaces.bot_z) {
    return false;
  }
  return (surfaces.top_z - surfaces.bot_z) == conn.connection->getThickness();
}

class StaGuard
{
 public:
  StaGuard(sta::Sta* sta) : sta_ptr_(sta)
  {
    if (!sta_ptr_) {
      temp_sta_ = std::make_unique<sta::Sta>();
      temp_sta_->makeComponents();
      sta_ptr_ = temp_sta_.get();
    }
  }
  ~StaGuard()
  {
    if (temp_sta_) {
      temp_sta_->setReport(nullptr);
    }
  }
  sta::Sta* operator->() const { return sta_ptr_; }

 private:
  std::unique_ptr<sta::Sta> temp_sta_;
  sta::Sta* sta_ptr_;
};

}  // namespace

Checker::Checker(utl::Logger* logger) : logger_(logger)
{
}

void Checker::check(dbChip* chip, sta::Sta* sta)
{
  auto* top_cat = dbMarkerCategory::createOrReplace(chip, "3DBlox");
  checkLogicalConnectivity(top_cat, chip, sta);
  UnfoldedModel model(logger_, chip);
  checkFloatingChips(top_cat, model);
  checkOverlappingChips(top_cat, model);
  checkInternalExtUsage(top_cat, model);
  checkConnectionRegions(top_cat, model);
}

void Checker::checkFloatingChips(dbMarkerCategory* top_cat,
                                 const UnfoldedModel& model)
{
  const auto& chips = model.getChips();
  // Add one more node for "ground" (external world: package, PCB, ...)
  utl::UnionFind uf(chips.size() + 1);
  const size_t ground_node = chips.size();

  std::unordered_map<const UnfoldedChip*, size_t> chip_map;
  for (size_t i = 0; i < chips.size(); ++i) {
    chip_map[&chips[i]] = i;
  }

  for (const auto& conn : model.getConnections()) {
    if (isValid(conn)) {
      // Case 1: Both regions exist - connect the two chips together
      if (conn.top_region && conn.bottom_region) {
        auto it1 = chip_map.find(conn.top_region->parent_chip);
        auto it2 = chip_map.find(conn.bottom_region->parent_chip);
        if (it1 != chip_map.end() && it2 != chip_map.end()) {
          uf.unite(it1->second, it2->second);
        }
      }
      // Case 2: Virtual connection (one region is null) - connect chip to
      // ground
      else if (conn.top_region || conn.bottom_region) {
        const UnfoldedRegion* region
            = conn.top_region ? conn.top_region : conn.bottom_region;
        auto it = chip_map.find(region->parent_chip);
        if (it != chip_map.end()) {
          uf.unite(it->second, ground_node);
        }
      }
    }
  }

  std::vector<std::vector<const UnfoldedChip*>> groups(chips.size() + 1);
  for (size_t i = 0; i < chips.size(); ++i) {
    groups[uf.find(i)].push_back(&chips[i]);
  }
  auto ground_leader = uf.find(ground_node);
  const bool ground_empty = groups[ground_leader].empty();
  groups.erase(groups.begin() + ground_leader);

  std::erase_if(groups, [](const auto& g) { return g.empty(); });

  std::ranges::sort(
      groups, [](const auto& a, const auto& b) { return a.size() < b.size(); });

  if (ground_empty) {
    logger_->warn(
        utl::ODB,
        206,
        "No ground group found. Erasing biggest group from floating chips.");
    if (!groups.empty()) {
      groups.pop_back();
    }
  }

  if (!groups.empty()) {
    auto* cat = dbMarkerCategory::createOrReplace(top_cat, "Floating chips");
    logger_->warn(utl::ODB, 151, "Found {} floating chip sets", groups.size());
    for (const auto& group : groups | std::views::reverse) {
      auto* marker = dbMarker::create(cat);
      for (auto* chip : group) {
        marker->addShape(chip->cuboid);
        marker->addSource(chip->chip_inst_path.back());
      }
      marker->setComment("Isolated chip set starting with " + group[0]->name);
    }
  }
}

void Checker::checkOverlappingChips(dbMarkerCategory* top_cat,
                                    const UnfoldedModel& model)
{
  const auto& chips = model.getChips();
  std::vector<int> sorted(chips.size());
  std::iota(sorted.begin(), sorted.end(), 0);
  std::ranges::sort(sorted, [&](int a, int b) {
    return chips[a].cuboid.xMin() < chips[b].cuboid.xMin();
  });

  std::vector<std::pair<const UnfoldedChip*, const UnfoldedChip*>> overlaps;
  for (size_t i = 0; i < sorted.size(); ++i) {
    auto* c1 = &chips[sorted[i]];
    for (size_t j = i + 1; j < sorted.size(); ++j) {
      auto* c2 = &chips[sorted[j]];
      if (c2->cuboid.xMin() >= c1->cuboid.xMax()) {
        break;
      }
      if (c1->cuboid.overlaps(c2->cuboid)) {
        overlaps.emplace_back(c1, c2);
      }
    }
  }

  if (!overlaps.empty()) {
    auto* cat = dbMarkerCategory::createOrReplace(top_cat, "Overlapping chips");
    logger_->warn(utl::ODB, 156, "Found {} overlapping chips", overlaps.size());
    for (const auto& [inst1, inst2] : overlaps) {
      auto* marker = dbMarker::create(cat);
      auto intersection = inst1->cuboid.intersect(inst2->cuboid);
      marker->addShape(intersection);
      marker->addSource(inst1->chip_inst_path.back());
      marker->addSource(inst2->chip_inst_path.back());
      marker->setComment(
          fmt::format("Chips {} and {} overlap", inst1->name, inst2->name));
    }
  }
}

void Checker::checkInternalExtUsage(dbMarkerCategory* top_cat,
                                    const UnfoldedModel& model)
{
  dbMarkerCategory* cat = nullptr;
  for (const auto& chip : model.getChips()) {
    for (const auto& region : chip.regions) {
      if (region.isInternalExt() && !region.isUsed) {
        if (!cat) {
          cat = dbMarkerCategory::createOrReplace(top_cat,
                                                  "Unused internal_ext");
        }
        logger_->warn(utl::ODB,
                      464,
                      "Region {} is internal_ext but unused",
                      region.region_inst->getChipRegion()->getName());
        auto* marker = dbMarker::create(cat);
        marker->addSource(region.region_inst);
        marker->addShape(region.cuboid);
        marker->setComment(
            fmt::format("Unused internal_ext region: {}",
                        region.region_inst->getChipRegion()->getName()));
      }
    }
  }
}

void Checker::checkConnectionRegions(dbMarkerCategory* top_cat,
                                     const UnfoldedModel& model)
{
  auto describe = [](const UnfoldedRegion* r, dbMarker* marker) {
    marker->addSource(r->region_inst);
    marker->addShape(Rect(r->cuboid.xMin(),
                          r->cuboid.yMin(),
                          r->cuboid.xMax(),
                          r->cuboid.yMax()));
    return fmt::format("{}/{} (faces {})",
                       r->parent_chip->name,
                       r->region_inst->getChipRegion()->getName(),
                       sideToString(r->effective_side));
  };
  int count = 0;
  dbMarkerCategory* cat = nullptr;
  for (const auto& conn : model.getConnections()) {
    if (!isValid(conn)) {
      if (!cat) {
        cat = dbMarkerCategory::createOrReplace(top_cat, "Connection regions");
      }
      auto* marker = dbMarker::create(cat);
      marker->addSource(conn.connection);
      std::string msg = fmt::format("Invalid connection {}: {} to {}",
                                    conn.connection->getName(),
                                    describe(conn.top_region, marker),
                                    describe(conn.bottom_region, marker));
      marker->setComment(msg);
      logger_->warn(utl::ODB, 207, msg);
      count++;
    }
  }
  if (count > 0) {
    logger_->warn(utl::ODB, 273, "Found {} invalid connections", count);
  }
}

void Checker::checkBumpPhysicalAlignment(dbMarkerCategory* top_cat,
                                         const UnfoldedModel& model)
{
}

void Checker::checkNetConnectivity(dbMarkerCategory* top_cat,
                                   const UnfoldedModel& model)
{
}

void Checker::checkLogicalConnectivity(dbMarkerCategory* top_cat,
                                       dbChip* chip,
                                       sta::Sta* sta)
{
  StaGuard sta_guard(sta);
  std::unordered_set<dbChip*> processed_masters;

  dbMarkerCategory* design_cat = nullptr;
  dbMarkerCategory* alignment_cat = nullptr;

  for (auto* inst : chip->getChipInsts()) {
    auto* master = inst->getMasterChip();
    if (!master || !processed_masters.insert(master).second) {
      continue;
    }

    auto* network = sta_guard->network();
    sta::Cell* cell = nullptr;
    {
      std::unique_ptr<sta::LibraryIterator> lib_iter(
          network->libraryIterator());
      while (lib_iter->hasNext()) {
        auto* lib = lib_iter->next();
        cell = network->findCell(lib, master->getName());
        if (cell) {
          break;
        }
      }
    }

    if (!cell) {
      std::vector<std::string> modules;
      std::unique_ptr<sta::LibraryIterator> li(network->libraryIterator());
      sta::PatternMatch all("*");
      while (li->hasNext()) {
        auto matches = network->findCellsMatching(li->next(), &all);
        for (auto* c : matches) {
          const char* name = network->name(c);
          if (name) {
            modules.emplace_back(name);
          }
        }
      }
      std::ranges::sort(modules);
      if (modules.size() > 10) {
        modules.resize(10);
        modules.emplace_back("...");
      }
      if (!design_cat) {
        design_cat
            = dbMarkerCategory::createOrReplace(top_cat, "Design Alignment");
      }
      auto* marker = dbMarker::create(design_cat);
      std::string msg = fmt::format(
          "Design Alignment Violation: Verilog module {} not found. "
          "Available modules: {}",
          master->getName(),
          modules.empty() ? "None"
                          : fmt::format("{}", fmt::join(modules, ", ")));
      marker->setComment(msg);
      marker->addSource(master);
      logger_->warn(utl::ODB, 550, msg);
      continue;
    }

    std::unordered_set<std::string> nets;
    auto* prop = dbStringProperty::find(master, "verilog_file");
    if (prop && sta) {
      std::string verilog_file = prop->getValue();
      if (!verilog_file.empty() && std::filesystem::exists(verilog_file)) {
        sta::ConcreteNetwork temp_network;
        temp_network.copyState(sta);
        sta::VerilogReader verilog_reader(&temp_network);
        if (verilog_reader.read(verilog_file.c_str())) {
          sta::Instance* top_inst
              = verilog_reader.linkNetwork(master->getName(),
                                           /*make_black_boxes*/ true,
                                           /*delete_modules*/ false);
          if (top_inst) {
            std::unique_ptr<sta::NetIterator> net_iter(
                temp_network.netIterator(top_inst));
            while (net_iter->hasNext()) {
              nets.insert(temp_network.name(net_iter->next()));
            }
          }
        }
      }
    }

    std::vector<std::string> sorted_nets(nets.begin(), nets.end());
    std::ranges::sort(sorted_nets);
    if (sorted_nets.size() > 10) {
      sorted_nets.resize(10);
      sorted_nets.emplace_back("...");
    }

    for (auto* region : master->getChipRegions()) {
      for (auto* bump : region->getChipBumps()) {
        auto* net = bump->getNet();
        if (net && !nets.contains(net->getName())) {
          if (!alignment_cat) {
            alignment_cat = dbMarkerCategory::createOrReplace(
                top_cat, "Logical Alignment");
          }
          auto* marker = dbMarker::create(alignment_cat);
          std::string bump_name
              = bump->getInst() ? bump->getInst()->getName() : "UNKNOWN_BUMP";

          std::string msg = fmt::format(
              "Logical net {} (bump {}) not found in Verilog. Available nets: "
              "[{}]",
              net->getName(),
              bump_name,
              fmt::join(sorted_nets, ", "));

          marker->setComment(msg);
          marker->addSource(bump);
          logger_->warn(
              utl::ODB,
              560,
              "Logical net {} in bmap for chiplet {} (bump {}) not found in "
              "Verilog",
              net->getName(),
              master->getName(),
              bump_name);
        }
      }
    }
  }
}

}  // namespace odb
