// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "dbSdcNetwork.hh"

#include <cstddef>
#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <utility>

#include "spdlog/fmt/fmt.h"
#include "sta/NetworkClass.hh"
#include "sta/ParseBus.hh"
#include "sta/PatternMatch.hh"
#include "sta/SdcNetwork.hh"
#include "sta/StringUtil.hh"

namespace sta {

dbSdcNetwork::dbSdcNetwork(Network* network) : SdcNetwork(network)
{
}

// Override SdcNetwork to NetworkNameAdapter.
Instance* dbSdcNetwork::findInstance(std::string_view path_name) const
{
  Instance* inst = network_->findInstance(path_name);
  if (inst == nullptr) {
    inst = network_->findInstance(escapeDividers(path_name, this));
  }
  if (inst == nullptr) {
    inst = network_->findInstance(escapeBrackets(path_name, this));
  }
  return inst;
}

InstanceSeq dbSdcNetwork::findInstancesMatching(
    const Instance*,
    const PatternMatch* pattern) const
{
  InstanceSeq insts;
  if (pattern->hasWildcards()) {
    findInstancesMatching1(pattern, insts);
  } else {
    Instance* inst = findInstance(pattern->pattern());
    if (inst) {
      insts.push_back(inst);
    } else {
      // Look for a match with path dividers escaped.
      std::string escaped
          = escapeChars(pattern->pattern(), divider_, '\0', escape_);
      inst = findInstance(escaped.c_str());
      if (inst) {
        insts.push_back(inst);
      } else {
        // Malo
        findInstancesMatching1(pattern, insts);
      }
    }
  }

  return insts;
}

void dbSdcNetwork::findInstancesMatching1(const PatternMatch* pattern,
                                          InstanceSeq& insts) const
{
  // Literal pattern: serve from the precomputed pathological-path map.
  // Most designs contribute zero entries to this map, so the lookup is
  // O(1) and the miss path simply falls through.
  if (!pattern->isRegexp() && !pattern->hasWildcards()) {
    const SdcPathToInstMap& map = sdcPathToInstMap();
    auto it = map.find(pattern->pattern());
    if (it != map.end()) {
      insts.push_back(it->second);
    }
    return;
  }
  visitAllInstancesSdcPath(
      [&](Instance* child, const std::string& sdc_path, bool /*any_div*/) {
        if (pattern->match(sdc_path)) {
          insts.push_back(child);
        }
      });
}

NetSeq dbSdcNetwork::findNetsMatching(const Instance*,
                                      const PatternMatch* pattern) const
{
  NetSeq nets;
  if (pattern->hasWildcards()) {
    findNetsMatching1(pattern, nets);
  } else {
    Net* net = findNet(pattern->pattern());
    if (net) {
      nets.push_back(net);
    } else {
      // Look for a match with path dividers escaped.
      std::string escaped
          = escapeChars(pattern->pattern(), divider_, '\0', escape_);
      net = findNet(escaped.c_str());
      if (net) {
        nets.push_back(net);
      } else {
        findNetsMatching1(pattern, nets);
      }
    }
  }
  return nets;
}

void dbSdcNetwork::findNetsMatching1(const PatternMatch* pattern,
                                     NetSeq& nets) const
{
  std::unique_ptr<NetIterator> net_iter{netIterator(topInstance())};
  while (net_iter->hasNext()) {
    Net* net = net_iter->next();
    if (pattern->match(staToSdc(name(net)))) {
      nets.push_back(net);
    }
  }
}

PinSeq dbSdcNetwork::findPinsMatching(const Instance* instance,
                                      const PatternMatch* pattern) const
{
  PinSeq pins;
  if (pattern->pattern() == "*") {
    // Pattern of '*' matches all child instance pins.
    std::unique_ptr<InstanceChildIterator> child_iter{childIterator(instance)};
    while (child_iter->hasNext()) {
      Instance* child = child_iter->next();
      std::unique_ptr<InstancePinIterator> pin_iter{pinIterator(child)};
      while (pin_iter->hasNext()) {
        Pin* pin = pin_iter->next();
        pins.push_back(pin);
      }
    }
  } else {
    std::string inst_path, port_name;
    pathNameLast(pattern->pattern(), inst_path, port_name);
    if (!port_name.empty()) {
      PatternMatch inst_pattern(inst_path, pattern);
      InstanceSeq insts = findInstancesMatching(nullptr, &inst_pattern);
      PatternMatch port_pattern(port_name, pattern);
      for (auto inst : insts) {
        findMatchingPins(inst, &port_pattern, pins);
      }
    }
  }

  return pins;
}

void dbSdcNetwork::findMatchingPins(const Instance* instance,
                                    const PatternMatch* port_pattern,
                                    PinSeq& pins) const
{
  if (instance != network_->topInstance()) {
    Cell* cell = network_->cell(instance);
    std::unique_ptr<CellPortIterator> port_iter{network_->portIterator(cell)};
    while (port_iter->hasNext()) {
      Port* port = port_iter->next();
      const std::string port_name = network_->name(port);
      if (network_->hasMembers(port)) {
        bool bus_matches
            = port_pattern->match(port_name)
              || port_pattern->match(escapeDividers(port_name, network_));
        std::unique_ptr<PortMemberIterator> member_iter{
            network_->memberIterator(port)};
        while (member_iter->hasNext()) {
          Port* member_port = member_iter->next();
          Pin* pin = network_->findPin(instance, member_port);
          if (pin) {
            if (bus_matches) {
              pins.push_back(pin);
            } else {
              const std::string member_name = network_->name(member_port);
              if (port_pattern->match(member_name)
                  || port_pattern->match(
                      escapeDividers(member_name, network_))) {
                pins.push_back(pin);
              }
            }
          }
        }
      } else if (port_pattern->match(port_name)
                 || port_pattern->match(escapeDividers(port_name, network_))) {
        Pin* pin = network_->findPin(instance, port);
        if (pin) {
          pins.push_back(pin);
        }
      }
    }
  }
}

Pin* dbSdcNetwork::findPin(std::string_view path_name) const
{
  std::string inst_path, port_name;
  pathNameLast(path_name, inst_path, port_name);
  Pin* pin = nullptr;
  if (!inst_path.empty()) {
    Instance* inst = findInstance(inst_path);
    if (inst) {
      pin = findPin(inst, port_name);
    } else {
      pin = nullptr;
    }
  } else {
    pin = findPin(topInstance(), path_name);
  }
  return pin;
}

void dbSdcNetwork::visitAllInstancesSdcPath(const SdcPathVisitor& visitor) const
{
  // Build paths incrementally in a fmt::memory_buffer to avoid the
  // per-step std::string allocations a naive DFS would incur. Using a
  // generic recursive lambda (auto& self) instead of a std::function
  // skips type erasure and the heap allocation it can incur.
  // The any_div flag propagates down so the visitor can cheaply tell
  // whether sdc_path's recursive splitter resolution would fail.
  auto rec = [&](auto& self,
                 Instance* parent,
                 fmt::memory_buffer& buf,
                 bool any_div) -> void {
    std::unique_ptr<InstanceChildIterator> it{childIterator(parent)};
    while (it->hasNext()) {
      Instance* child = it->next();
      const size_t orig_size = buf.size();
      if (orig_size > 0) {
        buf.push_back(pathDivider());
      }
      const std::string leaf = name(child);
      // Short-circuit: once a subtree is pathological, descendants
      // inherit that flag without re-scanning the leaf string.
      const bool subtree_any_div
          = any_div || leaf.find(pathDivider()) != std::string::npos;
      buf.append(std::string_view(leaf));
      buf.push_back('\0');  // null-terminate for staToSdc's C-string input
      visitor(child, staToSdc(buf.data()), subtree_any_div);
      buf.resize(buf.size() - 1);
      if (!isLeaf(child)) {
        self(self, child, buf, subtree_any_div);
      }
      buf.resize(orig_size);
    }
  };
  fmt::memory_buffer buf;
  rec(rec, topInstance(), buf, false);
}

const dbSdcNetwork::SdcPathToInstMap& dbSdcNetwork::sdcPathToInstMap() const
{
  if (!sdc_path_to_inst_) {
    sdc_path_to_inst_.emplace();
    inst_to_sdc_path_.emplace();
    visitAllInstancesSdcPath(
        [&](Instance* inst, std::string sdc_path, bool any_div) {
          // Only pathological entries (those the recursive splitter in
          // findInstance cannot resolve) need to live in the cache.
          if (any_div) {
            inst_to_sdc_path_->emplace(inst, sdc_path);
            sdc_path_to_inst_->emplace(std::move(sdc_path), inst);
          }
        });
  }
  return *sdc_path_to_inst_;
}

bool dbSdcNetwork::hasPathologicalPath(const Instance* inst) const
{
  const Instance* top = topInstance();
  for (const Instance* a = inst; a && a != top; a = network_->parent(a)) {
    if (name(a).find(pathDivider()) != std::string::npos) {
      return true;
    }
  }
  return false;
}

void dbSdcNetwork::insertEntry(Instance* inst) const
{
  std::string sdc_path = SdcNetwork::pathName(inst);
  inst_to_sdc_path_->emplace(inst, sdc_path);
  sdc_path_to_inst_->emplace(std::move(sdc_path), inst);
}

void dbSdcNetwork::eraseEntry(const Instance* inst) const
{
  auto rev_it = inst_to_sdc_path_->find(inst);
  if (rev_it == inst_to_sdc_path_->end()) {
    return;
  }
  sdc_path_to_inst_->erase(rev_it->second);
  inst_to_sdc_path_->erase(rev_it);
}

void dbSdcNetwork::onInstCreated(Instance* inst)
{
  if (!sdc_path_to_inst_) {
    return;
  }
  if (!hasPathologicalPath(inst)) {
    return;
  }
  insertEntry(inst);
}

void dbSdcNetwork::onInstDestroyed(Instance* inst)
{
  if (!sdc_path_to_inst_) {
    return;
  }
  eraseEntry(inst);
}

void dbSdcNetwork::onInstRenamed(Instance* inst)
{
  if (!sdc_path_to_inst_) {
    return;
  }
  // Erase by Instance* (uses the reverse map) — no need to reconstruct
  // the pre-rename path. Re-insert if still pathological.
  eraseEntry(inst);
  if (hasPathologicalPath(inst)) {
    insertEntry(inst);
  }
}

}  // namespace sta
