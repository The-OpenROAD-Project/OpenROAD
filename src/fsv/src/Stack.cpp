// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include "fsv/Stack.h"

#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace fsv {

Stack::Layer Stack::layer(const std::string_view name) const
{
  for (size_t i = 0; i < conductors.size(); i++) {
    if (conductors[i].name == name) {
      return Layer(static_cast<int>(i));
    }
  }
  for (size_t i = 0; i < vias.size(); i++) {
    if (vias[i].name == name) {
      return Layer(static_cast<int>(conductors.size() + i));
    }
  }
  return Layer();
}

std::vector<Stack::Layer> Stack::conductorLayers() const
{
  std::vector<Layer> layers;
  for (size_t i = 0; i < conductors.size(); i++) {
    layers.push_back(Layer(static_cast<int>(i)));
  }
  return layers;
}

std::vector<Stack::Layer> Stack::viaLayers() const
{
  std::vector<Layer> layers;
  for (size_t i = 0; i < vias.size(); i++) {
    layers.push_back(Layer(static_cast<int>(conductors.size() + i)));
  }
  return layers;
}

bool Stack::isConductor(const Layer layer) const
{
  return layer.valid() && layer.index() < static_cast<int>(conductors.size());
}

bool Stack::isVia(const Layer layer) const
{
  return layer.valid() && !isConductor(layer)
         && layer.index() < static_cast<int>(conductors.size() + vias.size());
}

namespace {

// What a handle that is neither a conductor nor a via of the stack is.
[[noreturn]] void failHandle(const Stack::Layer layer)
{
  throw std::invalid_argument(
      layer.valid() ? "layer handle " + std::to_string(layer.index())
                          + " is past the stack's layers"
                    : std::string("no layer given"));
}

}  // namespace

const Stack::Conductor& Stack::conductor(const Layer layer) const
{
  if (isVia(layer)) {
    throw std::invalid_argument(name(layer)
                                + " is a via, not a conductor of the stack");
  }
  if (!isConductor(layer)) {
    failHandle(layer);
  }
  return conductors[layer.index()];
}

const Stack::Via& Stack::via(const Layer layer) const
{
  if (isConductor(layer)) {
    throw std::invalid_argument(name(layer)
                                + " is a conductor, not a via of the stack");
  }
  if (!isVia(layer)) {
    failHandle(layer);
  }
  return vias[layer.index() - conductors.size()];
}

const std::string& Stack::name(const Layer layer) const
{
  if (isConductor(layer)) {
    return conductors[layer.index()].name;
  }
  if (isVia(layer)) {
    return vias[layer.index() - conductors.size()].name;
  }
  failHandle(layer);
}

Stack::ZExtent Stack::zExtent(const Layer layer) const
{
  if (isConductor(layer)) {
    const Conductor& c = conductor(layer);
    return {c.bottom, c.top};
  }
  const Via& v = via(layer);
  if (!isConductor(v.upper)) {
    throw std::invalid_argument("via " + v.name + " has no upper conductor");
  }
  return {v.lower.valid() ? conductor(v.lower).top : 0,
          conductor(v.upper).bottom};
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
