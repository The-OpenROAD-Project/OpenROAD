// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include "fsv/Stack.h"

#include <string>
#include <string_view>
#include <vector>

namespace fsv {

const Stack::Conductor* Stack::findConductor(const std::string_view name) const
{
  for (const Conductor& c : conductors) {
    if (c.name == name) {
      return &c;
    }
  }
  return nullptr;
}

double Stack::epsAt(const double z) const
{
  for (const Dielectric& d : dielectrics) {
    if (z < d.top) {
      return d.eps;
    }
  }
  return background_eps;
}

std::vector<Stack::PlanarInterface> Stack::planarInterfaces() const
{
  std::vector<PlanarInterface> interfaces;
  for (size_t i = 0; i < dielectrics.size(); i++) {
    const double above
        = i + 1 < dielectrics.size() ? dielectrics[i + 1].eps : background_eps;
    if (above != dielectrics[i].eps) {
      interfaces.push_back({dielectrics[i].top, dielectrics[i].eps, above});
    }
  }
  return interfaces;
}

}  // namespace fsv
