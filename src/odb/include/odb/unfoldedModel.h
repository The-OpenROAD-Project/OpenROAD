// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023-2026, The OpenROAD Authors

#pragma once

#include <boost/functional/hash.hpp>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <functional>
#include <mutex>
#include <optional>
#include <ranges>
#include <shared_mutex>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "odb/dbTransform.h"
#include "odb/geom.h"
#include "utl/Logger.h"

namespace odb {

// Custom hash for std::vector<T*> keys — uses stable ODB object IDs
// instead of raw pointer addresses for deterministic iteration order
// across runs (ASLR-immune).
struct VectorPtrHash
{
  template <typename T>
  std::size_t operator()(const std::vector<T*>& vec) const
  {
    auto ids_view = vec | std::ranges::views::transform([](T* ptr) {
                      return ptr->getId();
                    });
    return boost::hash_range(ids_view.begin(), ids_view.end());
  }
};

// Custom hash for pair<T*, vector<U*>> keys — uses stable ODB object IDs
struct PairBumpPathHash
{
  template <typename T, typename U>
  std::size_t operator()(const std::pair<T*, std::vector<U*>>& p) const
  {
    std::size_t seed = std::hash<uint32_t>{}(p.first->getId());
    auto ids_view = p.second | std::ranges::views::transform([](U* ptr) {
                      return ptr->getId();
                    });
    boost::hash_combine(seed,
                        boost::hash_range(ids_view.begin(), ids_view.end()));
    return seed;
  }
};
class dbBlock;
class dbChip;
class dbChipBump;
class dbChipBumpInst;
class dbChipConn;
class dbChipInst;
class dbChipNet;
class dbChipRegion;
class dbChipRegionInst;

enum class UnfoldedRegionSide
{
  TOP,
  BOTTOM,
  INTERNAL,
  INTERNAL_EXT
};

struct UnfoldedChip;
struct UnfoldedRegion;

struct UnfoldedBump
{
 public:
  UnfoldedBump(dbChipBumpInst* bi, UnfoldedRegion* pr, const Point3D& pos)
      : bump_inst(bi), parent_region(pr), global_position(pos)
  {
  }
  dbChipBumpInst* bump_inst = nullptr;
  UnfoldedRegion* parent_region = nullptr;
  Point3D global_position;
  bool isValid() const { return is_valid_; }

 private:
  friend class UnfoldedModel;
  bool is_valid_ = true;
};

struct UnfoldedRegion
{
 public:
  dbChipRegionInst* region_inst = nullptr;
  UnfoldedRegionSide effective_side = UnfoldedRegionSide::TOP;
  Cuboid cuboid;
  UnfoldedChip* parent_chip = nullptr;
  bool isUsed = false;

  int getSurfaceZ() const;
  bool isTop() const { return effective_side == UnfoldedRegionSide::TOP; }
  bool isBottom() const { return effective_side == UnfoldedRegionSide::BOTTOM; }
  bool isInternal() const
  {
    return effective_side == UnfoldedRegionSide::INTERNAL;
  }
  bool isInternalExt() const
  {
    return effective_side == UnfoldedRegionSide::INTERNAL_EXT;
  }
  bool isValid() const { return is_valid_; }
  auto getBumps() const
  {
    return bumps | std::ranges::views::filter([](const UnfoldedBump& b) {
             return b.isValid();
           });
  }

 private:
  friend class UnfoldedModel;
  bool is_valid_ = true;
  std::deque<UnfoldedBump> bumps;
};

struct UnfoldedConnection
{
 public:
  UnfoldedConnection(dbChipConn* conn,
                     UnfoldedRegion* top,
                     UnfoldedRegion* bottom,
                     const std::vector<dbChipInst*>& path)
      : connection(conn),
        top_region(top),
        bottom_region(bottom),
        parent_path(path)
  {
  }
  dbChipConn* connection = nullptr;
  UnfoldedRegion* top_region = nullptr;
  UnfoldedRegion* bottom_region = nullptr;
  std::vector<dbChipInst*> parent_path;
  bool isValid() const { return is_valid_; }

 private:
  friend class UnfoldedModel;
  bool is_valid_ = true;
};

struct UnfoldedNet
{
 public:
  dbChipNet* chip_net = nullptr;
  std::vector<UnfoldedBump*> connected_bumps;
  std::vector<dbChipInst*> parent_path;
  bool isValid() const { return is_valid_; }

 private:
  friend class UnfoldedModel;
  bool is_valid_ = true;
};

struct UnfoldedChip
{
 public:
  std::string name;

  std::vector<dbChipInst*> chip_inst_path;
  Cuboid cuboid;
  dbTransform transform;

  dbBlock* getBlock() const;
  bool isValid() const { return is_valid_; }
  auto getRegions() const
  {
    return regions | std::ranges::views::filter([](const UnfoldedRegion& r) {
             return r.isValid();
           });
  }

 private:
  friend class UnfoldedModel;
  bool is_valid_ = true;
  std::deque<UnfoldedRegion> regions;
  std::unordered_map<dbChipRegionInst*, UnfoldedRegion*> region_map;
};

class UnfoldedModel
{
 public:
  UnfoldedModel(utl::Logger* logger, dbChip* chip);

  const std::deque<UnfoldedChip>& getUnfilteredChips() const
  {
    return unfolded_chips_;
  }
  const std::deque<UnfoldedConnection>& getUnfilteredConnections() const
  {
    return unfolded_connections_;
  }
  const std::deque<UnfoldedNet>& getUnfilteredNets() const
  {
    return unfolded_nets_;
  }

  // Filtered views that hide tombstoned entries — preferred consumer API
  auto getChips() const
  {
    return unfolded_chips_
           | std::ranges::views::filter(
               [](const UnfoldedChip& c) { return c.isValid(); });
  }
  auto getConnections() const
  {
    return unfolded_connections_
           | std::ranges::views::filter(
               [](const UnfoldedConnection& c) { return c.isValid(); });
  }
  auto getNets() const
  {
    return unfolded_nets_
           | std::ranges::views::filter(
               [](const UnfoldedNet& n) { return n.isValid(); });
  }

  // RAII read guard — acquires shared_lock for concurrent reads.
  // Only locks when model is non-null; null model = no lock.
  class ReadGuard
  {
   public:
    explicit ReadGuard(const UnfoldedModel* model);
    ~ReadGuard();

    ReadGuard(const ReadGuard&) = delete;
    ReadGuard& operator=(const ReadGuard&) = delete;

    const UnfoldedModel* operator->() const { return model_; }
    const UnfoldedModel* get() const { return model_; }
    explicit operator bool() const { return model_ != nullptr; }

   private:
    const UnfoldedModel* model_;
    std::optional<std::shared_lock<std::shared_mutex>> lock_;
  };

  // RAII write guard — acquires unique_lock for exclusive mutation.
  // Terminates via logger->error if the current thread holds a ReadGuard.
  class WriteGuard
  {
   public:
    explicit WriteGuard(UnfoldedModel* model);
    ~WriteGuard() = default;

    WriteGuard(const WriteGuard&) = delete;
    WriteGuard& operator=(const WriteGuard&) = delete;

   private:
    std::unique_lock<std::shared_mutex> lock_;
  };

  // Surgical update methods (called by UnfoldedModelUpdater under lock)
  void addChipInst(dbChipInst* inst);
  void removeChipInst(dbChipInst* inst);
  void updateChipGeometry(dbChip* chip);
  void addConnection(dbChipConn* conn);
  void removeConnection(dbChipConn* conn);
  void addNet(dbChipNet* net);
  void removeNet(dbChipNet* net);
  void addBumpInstToNet(dbChipNet* net,
                        dbChipBumpInst* bump_inst,
                        const std::vector<dbChipInst*>& rel_path);
  void updateRegionGeometry(dbChipRegion* region);
  void addBump(dbChipBump* bump);
  void removeBump(dbChipBump* bump);

  // Compact all deques: erase tombstoned entries, rebuild cross-references.
  // Call explicitly after a batch of mutations when a dense view is needed.
  // Invalidates all raw pointers into the model — callers must re-query.
  void compact();

  // Diagnostic: number of tombstoned chip entries since last compact()
  size_t getChipTombstoneCount() const { return chip_tombstone_count_; }

 private:
  // Allow surgical update methods access to internals
  friend class UnfoldedModelUpdater;

  UnfoldedChip* buildUnfoldedChip(dbChipInst* chip_inst,
                                  std::vector<dbChipInst*>& path,
                                  const dbTransform& parent_xform);
  void registerUnfoldedChip(UnfoldedChip& uf_chip);
  void unfoldRegions(UnfoldedChip& uf_chip, dbChipInst* inst);
  void unfoldBumps(UnfoldedRegion& uf_region, const dbTransform& transform);
  void unfoldConnections(dbChip* chip,
                         const std::vector<dbChipInst*>& parent_path);
  void unfoldNets(dbChip* chip, const std::vector<dbChipInst*>& parent_path);

  UnfoldedChip* findUnfoldedChip(const std::vector<dbChipInst*>& path);
  UnfoldedRegion* findUnfoldedRegion(UnfoldedChip* chip,
                                     dbChipRegionInst* region_inst);
  void recomputeSubtreeTransforms(dbChipInst* inst);

  // Emit one unfolded connection/net into the given parent context.
  // Shared by the initial build (unfoldConnections/unfoldNets) and
  // the surgical add callbacks (addConnection/addNet).
  void emitUnfoldedConnection(dbChipConn* conn,
                               const std::vector<dbChipInst*>& parent_path);
  void emitUnfoldedNet(dbChipNet* net,
                       const std::vector<dbChipInst*>& parent_path);

  // Recompute every region's cuboid and every bump's global position
  // from chip.transform. Shared by recomputeSubtreeTransforms and
  // updateChipGeometry.
  void recomputeRegionsAndBumps(UnfoldedChip& chip);

  // Tombstone one unfolded connection entry: mark invalid, clean
  // inst_to_conns_ back-references for all path elements, and remove
  // the entry from conn_map_ (erasing the key when it becomes empty).
  void tombstoneConnection(UnfoldedConnection* conn);

  // Tombstone one unfolded net entry: mark invalid, clean
  // inst_to_nets_ back-references for all path elements, and remove
  // the entry from net_map_ (erasing the key when it becomes empty).
  void tombstoneNet(UnfoldedNet* net);

  // Collect all parent contexts (path + transform) where a chip is used
  // as a HIER parent in the hierarchy. Returns one entry per instantiation.
  std::vector<std::pair<std::vector<dbChipInst*>, dbTransform>>
  collectParentContexts(dbChip* parent_chip) const;

  void rebuildMaps();  // rebuild all pointer-based maps from live deques

  utl::Logger* logger_;
  dbChip* top_chip_ = nullptr;
  mutable std::shared_mutex mutex_;

  // Only track chip tombstones — the expensive compaction path.
  // Net/connection compaction is always inline (cheap).
  size_t chip_tombstone_count_ = 0;

  // Primary storage (pointer-stable via deque/push_back — no reallocation)
  std::deque<UnfoldedChip> unfolded_chips_;
  std::deque<UnfoldedConnection> unfolded_connections_;
  std::deque<UnfoldedNet> unfolded_nets_;

  // Lookup maps (unordered for O(1) average lookup)
  std::unordered_map<std::vector<dbChipInst*>, UnfoldedChip*, VectorPtrHash>
      chip_path_map_;
  std::unordered_map<std::pair<dbChipBumpInst*, std::vector<dbChipInst*>>,
                     UnfoldedBump*,
                     PairBumpPathHash>
      bump_inst_map_;

  // Index maps for surgical updates
  std::unordered_map<dbChip*, std::vector<UnfoldedChip*>> chip_type_map_;
  std::unordered_map<dbChipConn*, std::vector<UnfoldedConnection*>> conn_map_;
  std::unordered_map<dbChipNet*, std::vector<UnfoldedNet*>> net_map_;

  // Instance → unfolded element lookups for O(K) removal.
  // Maps each dbChipInst to all unfolded elements whose parent_path contains
  // it.
  std::unordered_map<dbChipInst*, std::vector<UnfoldedChip*>> inst_to_chips_;
  std::unordered_map<dbChipInst*, std::vector<UnfoldedConnection*>>
      inst_to_conns_;
  std::unordered_map<dbChipInst*, std::vector<UnfoldedNet*>> inst_to_nets_;

  // HIER instance registry: maps each dbChipInst whose master is a HIER chip
  // to all prefix paths where it appears. Unlike chip_path_map_ (which only
  // has leaf entries), this tracks HIER instances directly so empty HIER chips
  // are always discoverable by collectParentContexts.
  std::unordered_map<dbChipInst*, std::vector<std::vector<dbChipInst*>>>
      hier_inst_paths_;
};

}  // namespace odb
