// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023-2026, The OpenROAD Authors

#include "odb/unfoldedModel.h"

#include <algorithm>
#include <cstdint>
#include <deque>
#include <iterator>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "odb/db.h"
#include "odb/dbTransform.h"
#include "odb/geom.h"
#include "utl/Logger.h"

namespace odb {

namespace {
// Thread-local flag for deadlock detection. Only accessed by ReadGuard
// and WriteGuard — not exposed outside this translation unit.
thread_local bool t_holds_read_guard = false;
}  // namespace

// ---- ReadGuard / WriteGuard implementations ----

UnfoldedModel::ReadGuard::ReadGuard(const UnfoldedModel* model) : model_(model)
{
  if (model_ != nullptr) {
    lock_.emplace(model_->mutex_);
    t_holds_read_guard = true;
  }
}

UnfoldedModel::ReadGuard::~ReadGuard()
{
  if (model_ != nullptr) {
    t_holds_read_guard = false;
  }
}

UnfoldedModel::WriteGuard::WriteGuard(UnfoldedModel* model)
{
  if (t_holds_read_guard) {
    model->logger_->error(
        utl::ODB,
        481,
        "Deadlock detected: attempting to acquire UnfoldedModel WriteGuard "
        "while holding a ReadGuard on the same thread. Drop the ReadGuard "
        "before mutating chiplet objects.");
  }
  lock_ = std::unique_lock(model->mutex_);
}

namespace {

std::vector<dbChipInst*> concatPath(const std::vector<dbChipInst*>& head,
                                    const std::vector<dbChipInst*>& tail)
{
  if (tail.empty()) {
    return head;
  }
  std::vector<dbChipInst*> full = head;
  full.insert(full.end(), tail.begin(), tail.end());
  return full;
}

std::string getFullPathName(const std::vector<dbChipInst*>& path)
{
  std::string name;
  for (auto* p : path) {
    if (!name.empty()) {
      name += '/';
    }
    name += p->getName();
  }
  return name;
}

UnfoldedRegionSide mirrorSide(UnfoldedRegionSide side)
{
  if (side == UnfoldedRegionSide::TOP) {
    return UnfoldedRegionSide::BOTTOM;
  }
  if (side == UnfoldedRegionSide::BOTTOM) {
    return UnfoldedRegionSide::TOP;
  }
  return side;
}

}  // namespace

int UnfoldedRegion::getSurfaceZ() const
{
  if (isTop()) {
    return cuboid.zMax();
  }
  if (isBottom()) {
    return cuboid.zMin();
  }
  return cuboid.zCenter();
}

dbBlock* UnfoldedChip::getBlock() const
{
  if (chip_inst_path.empty()) {
    return nullptr;
  }
  dbChip* master = chip_inst_path.back()->getMasterChip();
  return master != nullptr ? master->getBlock() : nullptr;
}

UnfoldedModel::UnfoldedModel(utl::Logger* logger, dbChip* chip)
    : logger_(logger), top_chip_(chip)
{
  std::vector<dbChipInst*> path;
  for (dbChipInst* inst : chip->getChipInsts()) {
    buildUnfoldedChip(inst, path, dbTransform());
  }
  unfoldConnections(chip, {});
  unfoldNets(chip, {});
}

UnfoldedChip* UnfoldedModel::buildUnfoldedChip(dbChipInst* inst,
                                               std::vector<dbChipInst*>& path,
                                               const dbTransform& parent_xform)
{
  dbChip* master = inst->getMasterChip();
  path.push_back(inst);

  const dbTransform inst_xform = inst->getTransform();
  dbTransform total = inst_xform;
  total.concat(parent_xform);

  if (master->getChipType() == dbChip::ChipType::HIER) {
    // Register this HIER instance so collectParentContexts can find it
    // even when the HIER chip has no leaf descendants yet.
    hier_inst_paths_[inst].push_back(path);

    for (auto* sub : master->getChipInsts()) {
      buildUnfoldedChip(sub, path, total);
    }

    unfoldConnections(master, path);
    unfoldNets(master, path);
    path.pop_back();
    return nullptr;
  }

  UnfoldedChip uf_chip;
  uf_chip.name = getFullPathName(path);
  uf_chip.chip_inst_path = path;
  uf_chip.transform = total;
  uf_chip.cuboid = master->getCuboid();

  // Transform cuboid to global space
  uf_chip.transform.apply(uf_chip.cuboid);
  unfoldRegions(uf_chip, inst);

  unfolded_chips_.push_back(std::move(uf_chip));
  UnfoldedChip* created_chip = &unfolded_chips_.back();
  registerUnfoldedChip(*created_chip);

  path.pop_back();
  return created_chip;
}

void UnfoldedModel::registerUnfoldedChip(UnfoldedChip& chip)
{
  for (auto& region : chip.regions) {
    region.parent_chip = &chip;
    chip.region_map[region.region_inst] = &region;
    for (auto& bump : region.bumps) {
      bump.parent_region = &region;
      bump_inst_map_[{bump.bump_inst, chip.chip_inst_path}] = &bump;
    }
  }
  chip_path_map_[chip.chip_inst_path] = &chip;
  // Populate reverse map: master chip type → all unfolded instances
  if (!chip.chip_inst_path.empty()) {
    dbChip* master = chip.chip_inst_path.back()->getMasterChip();
    chip_type_map_[master].push_back(&chip);
  }
  // Populate inst → chips map for O(K) delta transforms
  for (dbChipInst* path_inst : chip.chip_inst_path) {
    inst_to_chips_[path_inst].push_back(&chip);
  }
}

void UnfoldedModel::unfoldRegions(UnfoldedChip& uf_chip, dbChipInst* inst)
{
  auto regions = inst->getRegions();

  for (auto* region_inst : regions) {
    auto region = region_inst->getChipRegion();

    UnfoldedRegionSide side = UnfoldedRegionSide::INTERNAL;
    switch (region->getSide()) {
      case dbChipRegion::Side::FRONT:
        side = UnfoldedRegionSide::TOP;
        break;
      case dbChipRegion::Side::BACK:
        side = UnfoldedRegionSide::BOTTOM;
        break;
      case dbChipRegion::Side::INTERNAL_EXT:
        side = UnfoldedRegionSide::INTERNAL_EXT;
        break;
      default:
        // INTERNAL: variable already initialized to INTERNAL, no assignment needed
        break;
    }

    if (uf_chip.transform.isMirrorZ()) {
      side = mirrorSide(side);
    }

    UnfoldedRegion uf_region;
    uf_region.region_inst = region_inst;
    uf_region.effective_side = side;
    uf_region.cuboid = region->getCuboid();

    uf_chip.transform.apply(uf_region.cuboid);
    uf_chip.regions.push_back(std::move(uf_region));
    unfoldBumps(uf_chip.regions.back(), uf_chip.transform);
  }
}

void UnfoldedModel::unfoldBumps(UnfoldedRegion& uf_region,
                                const dbTransform& transform)
{
  const int z = uf_region.getSurfaceZ();
  for (auto* bump_inst : uf_region.region_inst->getChipBumpInsts()) {
    dbChipBump* bump = bump_inst->getChipBump();
    if (auto* inst = bump->getInst()) {
      Point global_xy = inst->getLocation();
      transform.apply(global_xy);
      uf_region.bumps.emplace_back(
          bump_inst,
          nullptr,  // set later in registerUnfoldedChip
          Point3D(global_xy.x(), global_xy.y(), z));
    }
  }
}

UnfoldedChip* UnfoldedModel::findUnfoldedChip(
    const std::vector<dbChipInst*>& path)
{
  auto it = chip_path_map_.find(path);
  return it != chip_path_map_.end() ? it->second : nullptr;
}

UnfoldedRegion* UnfoldedModel::findUnfoldedRegion(UnfoldedChip* chip,
                                                  dbChipRegionInst* inst)
{
  if (!chip || !inst) {
    return nullptr;
  }
  auto it = chip->region_map.find(inst);
  return it != chip->region_map.end() ? it->second : nullptr;
}

void UnfoldedModel::emitUnfoldedConnection(
    dbChipConn* conn,
    const std::vector<dbChipInst*>& parent_path)
{
  UnfoldedRegion* top = findUnfoldedRegion(
      findUnfoldedChip(concatPath(parent_path, conn->getTopRegionPath())),
      conn->getTopRegion());
  UnfoldedRegion* bot = findUnfoldedRegion(
      findUnfoldedChip(concatPath(parent_path, conn->getBottomRegionPath())),
      conn->getBottomRegion());

  if (top == nullptr && bot == nullptr) {
    return;
  }
  UnfoldedConnection uf_conn(conn, top, bot, parent_path);
  if (top != nullptr && top->isInternalExt()) {
    top->isUsed = true;
  }
  if (bot != nullptr && bot->isInternalExt()) {
    bot->isUsed = true;
  }
  unfolded_connections_.push_back(uf_conn);
  UnfoldedConnection* added = &unfolded_connections_.back();
  conn_map_[conn].push_back(added);
  // Register in inst_to_conns_ for O(K) removal
  for (dbChipInst* path_inst : parent_path) {
    inst_to_conns_[path_inst].push_back(added);
  }
}

void UnfoldedModel::emitUnfoldedNet(dbChipNet* net,
                                    const std::vector<dbChipInst*>& parent_path)
{
  UnfoldedNet uf_net;
  uf_net.chip_net = net;
  uf_net.parent_path = parent_path;
  for (uint32_t i = 0; i < net->getNumBumpInsts(); i++) {
    std::vector<dbChipInst*> rel_path;
    dbChipBumpInst* b_inst = net->getBumpInst(i, rel_path);
    auto it = bump_inst_map_.find({b_inst, concatPath(parent_path, rel_path)});
    if (it != bump_inst_map_.end()) {
      uf_net.connected_bumps.push_back(it->second);
    }
  }
  unfolded_nets_.push_back(std::move(uf_net));
  UnfoldedNet* added = &unfolded_nets_.back();
  net_map_[net].push_back(added);
  // Register in inst_to_nets_ for O(K) removal
  for (dbChipInst* path_inst : parent_path) {
    inst_to_nets_[path_inst].push_back(added);
  }
}

void UnfoldedModel::unfoldConnections(
    dbChip* chip,
    const std::vector<dbChipInst*>& parent_path)
{
  for (dbChipConn* conn : chip->getChipConns()) {
    emitUnfoldedConnection(conn, parent_path);
  }
}

void UnfoldedModel::unfoldNets(dbChip* chip,
                               const std::vector<dbChipInst*>& parent_path)
{
  for (dbChipNet* net : chip->getChipNets()) {
    emitUnfoldedNet(net, parent_path);
  }
}

////////////////////////////////////////////////////////////////////
//
// Surgical update methods
//
////////////////////////////////////////////////////////////////////

void UnfoldedModel::tombstoneConnection(UnfoldedConnection* conn)
{
  if (conn->is_valid_ == false) {
    return;
  }
  conn->is_valid_ = false;
  // Clean inst_to_conns_ back-references for all path elements
  for (dbChipInst* path_inst : conn->parent_path) {
    auto vec_it = inst_to_conns_.find(path_inst);
    if (vec_it != inst_to_conns_.end()) {
      auto& vec = vec_it->second;
      vec.erase(std::ranges::remove(vec, conn).begin(), vec.end());
    }
  }
  // Remove from conn_map_; erase key when the vector becomes empty
  auto map_it = conn_map_.find(conn->connection);
  if (map_it != conn_map_.end()) {
    auto& vec = map_it->second;
    vec.erase(std::ranges::remove(vec, conn).begin(), vec.end());
    if (vec.empty()) {
      conn_map_.erase(map_it);
    }
  }
}

void UnfoldedModel::tombstoneNet(UnfoldedNet* net)
{
  if (net->is_valid_ == false) {
    return;
  }
  net->is_valid_ = false;
  // Clean inst_to_nets_ back-references for all path elements
  for (dbChipInst* path_inst : net->parent_path) {
    auto vec_it = inst_to_nets_.find(path_inst);
    if (vec_it != inst_to_nets_.end()) {
      auto& vec = vec_it->second;
      vec.erase(std::ranges::remove(vec, net).begin(), vec.end());
    }
  }
  // Remove from net_map_; erase key when the vector becomes empty
  auto map_it = net_map_.find(net->chip_net);
  if (map_it != net_map_.end()) {
    auto& vec = map_it->second;
    vec.erase(std::ranges::remove(vec, net).begin(), vec.end());
    if (vec.empty()) {
      net_map_.erase(map_it);
    }
  }
}

std::vector<std::pair<std::vector<dbChipInst*>, dbTransform>>
UnfoldedModel::collectParentContexts(dbChip* parent_chip) const
{
  std::vector<std::pair<std::vector<dbChipInst*>, dbTransform>> contexts;

  if (parent_chip == nullptr || parent_chip == top_chip_) {
    // Top-level chip: single empty parent path
    contexts.push_back({{}, dbTransform()});
    return contexts;
  }

  // Use hier_inst_paths_ for direct O(1) lookup of HIER instances.
  // This works even when the HIER chip has no leaf descendants.
  for (const auto& [hier_inst, paths] : hier_inst_paths_) {
    if (hier_inst->getMasterChip() != parent_chip) {
      continue;
    }
    for (const auto& path : paths) {
      // Compute the transform for this path
      dbTransform total;
      for (dbChipInst* path_inst : path) {
        dbTransform t = path_inst->getTransform();
        t.concat(total);
        total = t;
      }
      contexts.emplace_back(path, total);
    }
  }

  // No fallback — if the parent chip is not instantiated in the
  // hierarchy, return empty. Objects should NOT be hoisted to top level.
  return contexts;
}

void UnfoldedModel::addChipInst(dbChipInst* inst)
{
  // Collect ALL parent contexts before mutating (buildUnfoldedChip
  // modifies chip_path_map_, so we must snapshot first).
  dbChip* parent_chip = inst->getParentChip();
  auto parent_contexts = collectParentContexts(parent_chip);

  // Build the unfolded chip(s) under every parent context
  for (auto& [parent_path, parent_xform] : parent_contexts) {
    std::vector<dbChipInst*> path = parent_path;
    buildUnfoldedChip(inst, path, parent_xform);
  }
}

void UnfoldedModel::removeChipInst(dbChipInst* inst)
{
  // --- Phase 1: Clean HIER registry entries referencing this instance ---
  hier_inst_paths_.erase(inst);
  // Also remove paths that contain this inst as a prefix element,
  // which, after the erase above, can only be as an ancestor/prefix element
  for (auto it = hier_inst_paths_.begin(); it != hier_inst_paths_.end();) {
    std::erase_if(it->second, [inst](const std::vector<dbChipInst*>& path) {
      return std::ranges::find(path, inst) != path.end();
    });
    it = it->second.empty() ? hier_inst_paths_.erase(it) : std::next(it);
  }

  // --- Phase 2: Tombstone leaf chips and clean their map entries ---
  auto it = inst_to_chips_.find(inst);
  if (it != inst_to_chips_.end()) {
    // Copy since we modify inst_to_chips_ during iteration
    std::vector<UnfoldedChip*> affected = it->second;

    for (UnfoldedChip* chip : affected) {
      if (!chip->is_valid_) {
        continue;
      }

      chip->is_valid_ = false;
      chip_tombstone_count_++;
      chip_path_map_.erase(chip->chip_inst_path);

      // Remove from chip_type_map_
      if (!chip->chip_inst_path.empty()) {
        dbChip* master = chip->chip_inst_path.back()->getMasterChip();
        auto type_it = chip_type_map_.find(master);
        if (type_it != chip_type_map_.end()) {
          auto& vec = type_it->second;
          vec.erase(std::ranges::remove(vec, chip).begin(), vec.end());
        }
      }

      // Remove from inst_to_chips_ for all path elements
      for (dbChipInst* path_inst : chip->chip_inst_path) {
        auto inst_it = inst_to_chips_.find(path_inst);
        if (inst_it != inst_to_chips_.end()) {
          auto& vec = inst_it->second;
          vec.erase(std::ranges::remove(vec, chip).begin(), vec.end());
        }
      }

      for (auto& region : chip->regions) {
        region.is_valid_ = false;
        for (auto& bump : region.bumps) {
          bump.is_valid_ = false;
          bump_inst_map_.erase({bump.bump_inst, chip->chip_inst_path});
        }
      }
    }
  }

  // --- Phase 3: Cascade tombstones to connections and nets ---
  // O(K) lookup via reverse index.  Copy vectors before iterating —
  // tombstoneConnection/tombstoneNet modify inst_to_conns_/inst_to_nets_
  // for all path elements of each tombstoned object, including the
  // very entry we would be iterating.
  auto conn_it = inst_to_conns_.find(inst);
  if (conn_it != inst_to_conns_.end()) {
    std::vector<UnfoldedConnection*> to_tombstone = conn_it->second;
    for (UnfoldedConnection* conn : to_tombstone) {
      tombstoneConnection(conn);
    }
  }
  auto net_it = inst_to_nets_.find(inst);
  if (net_it != inst_to_nets_.end()) {
    std::vector<UnfoldedNet*> to_tombstone = net_it->second;
    for (UnfoldedNet* net : to_tombstone) {
      tombstoneNet(net);
    }
  }

  // Also tombstone connections whose regions became invalid
  // (connection's parent_path may not contain inst, but its regions
  // are inside a chip that was just tombstoned).
  for (auto& conn : unfolded_connections_) {
    if (conn.is_valid_ == false) {
      continue;
    }
    if ((conn.top_region != nullptr && conn.top_region->is_valid_ == false)
        || (conn.bottom_region != nullptr
            && conn.bottom_region->is_valid_ == false)) {
      tombstoneConnection(&conn);
    }
  }

  // Tombstone nets whose entire parent context lost all valid chips
  // (e.g., destroying a leaf inside a HIER — the net's parent_path
  // doesn't contain the leaf, but no chips remain under the context).
  for (auto& net : unfolded_nets_) {
    if (net.is_valid_ == false) {
      continue;
    }
    // Check if any valid chip still exists under this net's context
    bool has_valid_chip = false;
    const auto& pp = net.parent_path;
    auto pp_it = inst_to_chips_.find(pp.empty() ? nullptr : pp.back());
    if (pp.empty()) {
      // chip_path_map_ is eagerly maintained — non-empty iff a valid chip exists
      has_valid_chip = !chip_path_map_.empty();
    } else if (pp_it != inst_to_chips_.end()) {
      for (UnfoldedChip* chip : pp_it->second) {
        if (chip->is_valid_) {
          has_valid_chip = true;
          break;
        }
      }
    }
    if (has_valid_chip == false) {
      tombstoneNet(&net);
    }
  }

  // Erase the now-empty reverse-index keys for this dead instance.
  // tombstone_conn/tombstone_net already removed the entries element by
  // element; erase() removes the key itself so dead dbChipInst* pointers
  // do not accumulate as empty map buckets.
  inst_to_chips_.erase(inst);
  inst_to_conns_.erase(inst);
  inst_to_nets_.erase(inst);
}

void UnfoldedModel::recomputeRegionsAndBumps(UnfoldedChip& chip)
{
  // Recompute every region's cuboid and every bump's global XY/Z
  // from chip.transform (which the caller has already updated).
  for (auto& region : chip.regions) {
    if (!region.is_valid_) {
      continue;
    }
    dbChipRegion* region_def = region.region_inst->getChipRegion();
    region.cuboid = region_def->getCuboid();
    chip.transform.apply(region.cuboid);

    const int z = region.getSurfaceZ();
    for (auto& bump : region.bumps) {
      if (!bump.is_valid_) {
        continue;
      }
      dbChipBump* bump_def = bump.bump_inst->getChipBump();
      if (dbInst* phys_inst = bump_def->getInst()) {
        Point global_xy = phys_inst->getLocation();
        chip.transform.apply(global_xy);
        bump.global_position = Point3D(global_xy.x(), global_xy.y(), z);
      }
    }
  }
}

void UnfoldedModel::recomputeSubtreeTransforms(dbChipInst* inst)
{
  // For orientation changes, recompute transforms from scratch.
  // O(K) lookup via inst_to_chips_ instead of full sweep.
  auto it = inst_to_chips_.find(inst);
  if (it == inst_to_chips_.end()) {
    return;
  }

  for (UnfoldedChip* chip_ptr : it->second) {
    UnfoldedChip& chip = *chip_ptr;
    if (!chip.is_valid_) {
      continue;
    }

    // Recompute this chip's total transform from scratch
    dbTransform chip_total;
    for (dbChipInst* path_inst : chip.chip_inst_path) {
      dbTransform t = path_inst->getTransform();
      t.concat(chip_total);
      chip_total = t;
    }
    chip.transform = chip_total;

    // Recompute cuboid and all region/bump geometry
    dbChip* master = chip.chip_inst_path.back()->getMasterChip();
    chip.cuboid = master->getCuboid();
    chip.transform.apply(chip.cuboid);
    recomputeRegionsAndBumps(chip);
  }
}

void UnfoldedModel::updateChipGeometry(dbChip* chip)
{
  auto it = chip_type_map_.find(chip);
  if (it == chip_type_map_.end()) {
    return;
  }
  // Update all instances of this chip type
  for (UnfoldedChip* uf_chip : it->second) {
    if (!uf_chip->is_valid_) {
      continue;
    }
    // Recompute cuboid and all region/bump geometry
    uf_chip->cuboid = chip->getCuboid();
    uf_chip->transform.apply(uf_chip->cuboid);
    recomputeRegionsAndBumps(*uf_chip);
  }
}

void UnfoldedModel::addConnection(dbChipConn* conn)
{
  dbChip* parent_chip = conn->getParentChip();
  auto parent_contexts = collectParentContexts(parent_chip);

  // Unfold the connection under every parent context
  for (const auto& [parent_path, parent_xform] : parent_contexts) {
    emitUnfoldedConnection(conn, parent_path);
  }
}

void UnfoldedModel::removeConnection(dbChipConn* conn)
{
  auto it = conn_map_.find(conn);
  if (it == conn_map_.end()) {
    return;
  }
  // Snapshot — tombstoneConnection modifies conn_map_ during iteration
  std::vector<UnfoldedConnection*> entries = it->second;
  for (UnfoldedConnection* entry : entries) {
    tombstoneConnection(entry);
  }
}

void UnfoldedModel::addNet(dbChipNet* net)
{
  dbChip* parent_chip = net->getChip();
  auto parent_contexts = collectParentContexts(parent_chip);

  // Unfold the net under every parent context
  for (const auto& [parent_path, parent_xform] : parent_contexts) {
    emitUnfoldedNet(net, parent_path);
  }
}

void UnfoldedModel::removeNet(dbChipNet* net)
{
  auto it = net_map_.find(net);
  if (it == net_map_.end()) {
    return;
  }
  // Snapshot — tombstoneNet modifies net_map_ during iteration
  std::vector<UnfoldedNet*> entries = it->second;
  for (UnfoldedNet* entry : entries) {
    tombstoneNet(entry);
  }
}

void UnfoldedModel::addBumpInstToNet(dbChipNet* net,
                                     dbChipBumpInst* bump_inst,
                                     const std::vector<dbChipInst*>& rel_path)
{
  // A net may be unfolded multiple times (once per parent context).
  // Use net_map_ for O(K) lookup instead of scanning all unfolded nets.
  // Scope the bump lookup to each net's own parent context to avoid
  // cross-contamination between hierarchy instances.
  auto map_it = net_map_.find(net);
  if (map_it == net_map_.end()) {
    return;
  }
  for (UnfoldedNet* uf_net : map_it->second) {
    if (uf_net->is_valid_ == false) {
      continue;
    }
    auto full_path = concatPath(uf_net->parent_path, rel_path);
    auto it = bump_inst_map_.find({bump_inst, full_path});
    if (it != bump_inst_map_.end() && it->second->is_valid_) {
      uf_net->connected_bumps.push_back(it->second);
    }
  }
}

void UnfoldedModel::updateRegionGeometry(dbChipRegion* region)
{
  // Find all unfolded chips that have regions from this chip region definition
  // and update their cuboids
  for (auto& chip : unfolded_chips_) {
    if (!chip.is_valid_) {
      continue;
    }
    for (auto& uf_region : chip.regions) {
      if (!uf_region.is_valid_) {
        continue;
      }
      if (uf_region.region_inst->getChipRegion() == region) {
        uf_region.cuboid = region->getCuboid();
        chip.transform.apply(uf_region.cuboid);

        // Update bump Z coordinates to match the new surface
        const int z = uf_region.getSurfaceZ();
        for (auto& bump : uf_region.bumps) {
          if (!bump.is_valid_) {
            continue;
          }
          bump.global_position
              = Point3D(bump.global_position.x(), bump.global_position.y(), z);
        }
      }
    }
  }
}

void UnfoldedModel::addBump(dbChipBump* bump)
{
  dbChipRegion* region = bump->getChipRegion();
  // Find all UnfoldedRegions that correspond to this region definition
  for (auto& chip : unfolded_chips_) {
    if (!chip.is_valid_) {
      continue;
    }
    for (auto& uf_region : chip.regions) {
      if (!uf_region.is_valid_) {
        continue;
      }
      if (uf_region.region_inst->getChipRegion() != region) {
        continue;
      }
      // Create the unfolded bump in this context
      if (dbInst* phys_inst = bump->getInst()) {
        Point global_xy = phys_inst->getLocation();
        chip.transform.apply(global_xy);
        const int z = uf_region.getSurfaceZ();
        // Note: dbChipBumpInst is created by dbChipInst::create, not
        // independently. For bumps added after model construction,
        // the bump_inst must be resolved via the region's bump insts.
        for (dbChipBumpInst* bi : uf_region.region_inst->getChipBumpInsts()) {
          if (bi->getChipBump() == bump) {
            uf_region.bumps.emplace_back(
                bi, &uf_region, Point3D(global_xy.x(), global_xy.y(), z));
            bump_inst_map_[{bi, chip.chip_inst_path}] = &uf_region.bumps.back();
          }
        }
      }
    }
  }
}

void UnfoldedModel::removeBump(dbChipBump* bump)
{
  // Tombstone all UnfoldedBumps tied to this physical bump
  for (auto& chip : unfolded_chips_) {
    if (!chip.is_valid_) {
      continue;
    }
    for (auto& uf_region : chip.regions) {
      if (!uf_region.is_valid_) {
        continue;
      }
      for (auto& uf_bump : uf_region.bumps) {
        if (!uf_bump.is_valid_) {
          continue;
        }
        if (uf_bump.bump_inst->getChipBump() == bump) {
          uf_bump.is_valid_ = false;
          bump_inst_map_.erase({uf_bump.bump_inst, chip.chip_inst_path});
        }
      }
    }
  }
}

// Erases all tombstoned entries and rebuilds cross-references.
// Callers must re-query any cached pointers after calling this.
void UnfoldedModel::compact()
{
  // First erase nets/connections (they hold pointers into chips)
  std::erase_if(unfolded_nets_,
                [](const UnfoldedNet& n) { return !n.is_valid_; });
  std::erase_if(unfolded_connections_,
                [](const UnfoldedConnection& c) { return !c.is_valid_; });

  // Now erase dead chips and their internal regions/bumps
  std::erase_if(unfolded_chips_,
                [](const UnfoldedChip& c) { return !c.is_valid_; });
  for (auto& chip : unfolded_chips_) {
    for (auto& region : chip.regions) {
      std::erase_if(region.bumps,
                    [](const UnfoldedBump& b) { return !b.is_valid_; });
    }
    std::erase_if(chip.regions,
                  [](const UnfoldedRegion& r) { return !r.is_valid_; });
  }

  // Rebuild ALL pointer-based maps (chip erase invalidated everything)
  rebuildMaps();

  // Rebuild connection → region cross-references
  for (auto& conn : unfolded_connections_) {
    conn.top_region = findUnfoldedRegion(
        findUnfoldedChip(
            concatPath(conn.parent_path, conn.connection->getTopRegionPath())),
        conn.connection->getTopRegion());
    conn.bottom_region = findUnfoldedRegion(
        findUnfoldedChip(concatPath(conn.parent_path,
                                    conn.connection->getBottomRegionPath())),
        conn.connection->getBottomRegion());
  }

  // Rebuild net → bump cross-references
  for (auto& net : unfolded_nets_) {
    net.connected_bumps.clear();
    for (uint32_t i = 0; i < net.chip_net->getNumBumpInsts(); i++) {
      std::vector<dbChipInst*> rel_path;
      dbChipBumpInst* b_inst = net.chip_net->getBumpInst(i, rel_path);
      auto it = bump_inst_map_.find(
          {b_inst, concatPath(net.parent_path, rel_path)});
      if (it != bump_inst_map_.end()) {
        net.connected_bumps.push_back(it->second);
      }
    }
  }

  chip_tombstone_count_ = 0;
}

void UnfoldedModel::rebuildMaps()
{
  // Clear all pointer-based maps
  chip_path_map_.clear();
  bump_inst_map_.clear();
  chip_type_map_.clear();
  conn_map_.clear();
  net_map_.clear();
  inst_to_chips_.clear();
  inst_to_conns_.clear();
  inst_to_nets_.clear();
  // Note: hier_inst_paths_ is NOT rebuilt here — it is maintained
  // independently and only tracks HIER dbChipInst* objects, not
  // pointers into the deques.

  // Rebuild from surviving entries
  for (auto& chip : unfolded_chips_) {
    // Re-wire internal pointers
    chip.region_map.clear();
    for (auto& region : chip.regions) {
      region.parent_chip = &chip;
      chip.region_map[region.region_inst] = &region;
      for (auto& bump : region.bumps) {
        bump.parent_region = &region;
        bump_inst_map_[{bump.bump_inst, chip.chip_inst_path}] = &bump;
      }
    }
    chip_path_map_[chip.chip_inst_path] = &chip;
    if (!chip.chip_inst_path.empty()) {
      dbChip* master = chip.chip_inst_path.back()->getMasterChip();
      chip_type_map_[master].push_back(&chip);
    }
    for (dbChipInst* path_inst : chip.chip_inst_path) {
      inst_to_chips_[path_inst].push_back(&chip);
    }
  }
  for (auto& conn : unfolded_connections_) {
    conn_map_[conn.connection].push_back(&conn);
    for (dbChipInst* path_inst : conn.parent_path) {
      inst_to_conns_[path_inst].push_back(&conn);
    }
  }
  for (auto& net : unfolded_nets_) {
    net_map_[net.chip_net].push_back(&net);
    for (dbChipInst* path_inst : net.parent_path) {
      inst_to_nets_[path_inst].push_back(&net);
    }
  }
}

}  // namespace odb
