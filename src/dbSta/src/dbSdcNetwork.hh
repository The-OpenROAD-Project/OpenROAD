// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once
#include <cstddef>
#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>

#include "sta/Network.hh"
#include "sta/NetworkClass.hh"
#include "sta/SdcNetwork.hh"

namespace sta {

class dbSdcNetwork : public SdcNetwork
{
 public:
  explicit dbSdcNetwork(Network* network);
  Instance* findInstance(std::string_view path_name) const override;
  InstanceSeq findInstancesMatching(const Instance* contex,
                                    const PatternMatch* pattern) const override;
  NetSeq findNetsMatching(const Instance*,
                          const PatternMatch* pattern) const override;
  PinSeq findPinsMatching(const Instance* instance,
                          const PatternMatch* pattern) const override;

  // Drop the cache. Used on subtree-wide edits (parent change, modInst
  // destroy) where surgical fix-up would be more error-prone than letting
  // the next literal lookup rebuild lazily.
  void invalidateSdcPathToInstMap()
  {
    sdc_path_to_inst_.reset();
    inst_to_sdc_path_.reset();
  }

  // Incremental cache maintenance for hierarchy edits. Each is a no-op
  // when the cache hasn't been built yet (lazy property preserved) or
  // when the affected instance has no path component containing the
  // divider — the cache only ever holds the small set of "pathological"
  // entries that findInstance's recursive splitter cannot resolve.
  void onInstCreated(Instance* inst);
  void onInstDestroyed(Instance* inst);
  void onInstRenamed(Instance* inst);

 protected:
  void findInstancesMatching1(const PatternMatch* pattern,
                              InstanceSeq& insts) const;
  void findNetsMatching1(const PatternMatch* pattern, NetSeq& nets) const;
  void findMatchingPins(const Instance* instance,
                        const PatternMatch* port_pattern,
                        PinSeq& pins) const;
  Pin* findPin(std::string_view path_name) const override;
  using SdcNetwork::findPin;

 private:
  // Heterogeneous string hashing so callers can look up by std::string_view
  // without allocating a temporary std::string for each query.
  struct TransparentStringHash
  {
    using is_transparent = void;
    size_t operator()(std::string_view s) const noexcept
    {
      return std::hash<std::string_view>{}(s);
    }
  };
  using SdcPathToInstMap = std::unordered_map<std::string,
                                              Instance*,
                                              TransparentStringHash,
                                              std::equal_to<>>;
  // Reverse map for O(1) erase-by-instance during destroy/rename.
  using InstToSdcPathMap = std::unordered_map<const Instance*, std::string>;
  using SdcPathVisitor
      = std::function<void(Instance*, std::string sdc_path, bool any_div)>;

  // DFS the hierarchy, invoking visitor(child, sdc_path, any_div) once per
  // instance. any_div is true when sdc_path or any ancestor's leaf name
  // contains the divider — i.e. findInstance cannot resolve sdc_path.
  void visitAllInstancesSdcPath(const SdcPathVisitor& visitor) const;

  // True iff inst itself or any ancestor has a STA-form leaf name
  // containing the path divider (an escaped Verilog identifier such as
  // "\foo/bar"). These are the only entries we cache.
  bool hasPathologicalPath(const Instance* inst) const;

  // Insert / erase keep both maps consistent. Callers must have ensured
  // the cache is built; insertEntry additionally requires inst pathological.
  void insertEntry(Instance* inst) const;
  void eraseEntry(const Instance* inst) const;

  // Lazy full-path -> Instance lookup, used by findInstancesMatching1 to
  // resolve literal SDC patterns that the hierarchy walker missed (e.g. a
  // Verilog escaped identifier "\foo/bar"). Only pathological entries are
  // stored, so memory is O(escaped-identifier-count), not O(N).
  const SdcPathToInstMap& sdcPathToInstMap() const;
  mutable std::optional<SdcPathToInstMap> sdc_path_to_inst_;
  mutable std::optional<InstToSdcPathMap> inst_to_sdc_path_;
};

}  // namespace sta
