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
  using SdcPathVisitor
      = std::function<void(Instance*, const std::string& sdc_path)>;

  // DFS the hierarchy, invoking visitor(child, sdc_path) once per instance.
  // The path is built incrementally in a fmt::memory_buffer; the string
  // passed to the visitor is valid only for the call.
  void visitAllInstancesSdcPath(const SdcPathVisitor& visitor) const;

  // Lazy full-path -> Instance lookup, used by findInstancesMatching1 to
  // resolve literal SDC patterns that the hierarchy walker missed (which
  // happens when an instance name contains the path divider, e.g. a
  // Verilog escaped identifier "\foo/bar"). Without this, each literal
  // miss would do an O(N) DFS — the original O(patterns × N) hang.
  const SdcPathToInstMap& sdcPathToInstMap() const;
  mutable std::optional<SdcPathToInstMap> sdc_path_to_inst_;
};

}  // namespace sta
