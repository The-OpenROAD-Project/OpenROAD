// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include "pex/CoverMap.h"

#include <algorithm>
#include <cmath>
#include <set>
#include <string>
#include <vector>

#include "fsv/Stack.h"
#include "pex/Model.h"
#include "pex/Patterns.h"

namespace pex {

CoverMap::CoverMap(const fsv::Stack& stack,
                   const Model& model,
                   const Layer layer)
    : stack_(stack), layer_(layer)
{
  const fsv::Stack::Conductor& wire = stack.conductor(layer);
  // The covers the model tabulates for this layer, each side.
  std::set<std::string> below;
  std::set<std::string> above;
  for (const auto& [key, table] : model.laterals()) {
    if (key.layer != wire.name) {
      continue;
    }
    if (!key.below.empty()) {
      below.insert(key.below);
    }
    if (!key.above.empty()) {
      above.insert(key.above);
    }
  }
  for (const bool is_above : {false, true}) {
    std::vector<Tabulated>& side = is_above ? above_ : below_;
    for (const std::string& name : is_above ? above : below) {
      const Layer cover = stack.layer(name);
      if (!stack.isConductor(cover)) {
        continue;  // a context the stack cannot place
      }
      side.push_back({name,
                      distance(cover),
                      model.crossing({wire.name, name}) != nullptr});
    }
    std::sort(
        side.begin(), side.end(), [](const Tabulated& a, const Tabulated& b) {
          return a.distance < b.distance;
        });
  }
}

double CoverMap::distance(const Layer cover) const
{
  return coverDistance(stack_, layer_, cover);
}

const std::vector<CoverMap::Tabulated>* CoverMap::side(const Layer cover,
                                                       double& distance) const
{
  distance = this->distance(cover);
  if (distance < 0) {
    return nullptr;
  }
  const bool above
      = stack_.conductor(cover).bottom >= stack_.conductor(layer_).top;
  return above ? &above_ : &below_;
}

std::vector<CoverMap::Part> CoverMap::resolve(const Layer cover) const
{
  if (!cover.valid()) {
    return {{"", 1}};
  }
  double d = 0;
  const std::vector<Tabulated>* tabulated = side(cover, d);
  if (tabulated == nullptr || tabulated->empty()) {
    return {{"", 1}};
  }
  const std::string& name = stack_.name(cover);
  for (const Tabulated& t : *tabulated) {
    if (t.name == name) {
      return {{name, 1}};
    }
  }
  if (d <= tabulated->front().distance) {
    return {{tabulated->front().name, 1}};
  }
  if (d >= tabulated->back().distance) {
    return {{tabulated->back().name, 1}};
  }
  for (size_t i = 0; i + 1 < tabulated->size(); i++) {
    const Tabulated& a = (*tabulated)[i];
    const Tabulated& b = (*tabulated)[i + 1];
    if (d <= b.distance) {
      const double la = logCoverDistance(a.distance);
      const double lb = logCoverDistance(b.distance);
      const double t = lb > la ? (logCoverDistance(d) - la) / (lb - la) : 0;
      return {{a.name, 1 - t}, {b.name, t}};
    }
  }
  return {{tabulated->back().name, 1}};
}

std::string CoverMap::crossingCover(const Layer cover) const
{
  if (!cover.valid()) {
    return "";
  }
  double d = 0;
  const std::vector<Tabulated>* tabulated = side(cover, d);
  if (tabulated == nullptr) {
    return "";
  }
  const std::string& name = stack_.name(cover);
  const Tabulated* best = nullptr;
  double best_error = 0;
  for (const Tabulated& t : *tabulated) {
    if (!t.crossing) {
      continue;
    }
    if (t.name == name) {
      return name;
    }
    const double error
        = std::abs(logCoverDistance(d) - logCoverDistance(t.distance));
    if (best == nullptr || error < best_error) {
      best = &t;
      best_error = error;
    }
  }
  return best == nullptr ? "" : best->name;
}

}  // namespace pex
