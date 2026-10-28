// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include "fsv/Stack.h"

#include <cmath>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace fsv {

namespace {

// Heights closer than this (µm) are treated as equal.
constexpr double kHeightTolerance = 1e-9;

void requireEps(const double eps, const std::string& what)
{
  if (!(eps > 0) || !std::isfinite(eps)) {
    throw std::invalid_argument("permittivity of " + what
                                + " must be positive");
  }
}

}  // namespace

void Stack::setTechnology(std::string technology)
{
  technology_ = std::move(technology);
}

void Stack::setBackgroundEps(const double eps)
{
  requireEps(eps, "the background");
  background_eps_ = eps;
}

void Stack::addDielectric(const Dielectric& dielectric)
{
  requireEps(dielectric.eps, "dielectric " + dielectric.name);
  const double below = dielectrics_.empty() ? 0 : dielectrics_.back().top;
  if (std::abs(dielectric.bottom - below) > kHeightTolerance) {
    throw std::invalid_argument("dielectric " + dielectric.name + " starts at "
                                + std::to_string(dielectric.bottom)
                                + ", not where the stack ends ("
                                + std::to_string(below) + ")");
  }
  if (!(dielectric.top > dielectric.bottom)) {
    throw std::invalid_argument("dielectric " + dielectric.name
                                + " needs a positive thickness");
  }
  dielectrics_.push_back(dielectric);
  dielectrics_.back().bottom = below;
}

Stack::Layer Stack::addConductor(const Conductor& conductor)
{
  if (conductor.name.empty() || layer(conductor.name).valid()) {
    throw std::invalid_argument("conductor '" + conductor.name
                                + "' needs a name no layer has");
  }
  if (!vias_.empty()) {
    throw std::invalid_argument("conductor " + conductor.name
                                + " comes after the vias");
  }
  if (!(conductor.top > conductor.bottom) || conductor.bottom < 0
      || !std::isfinite(conductor.top)) {
    throw std::invalid_argument(
        "conductor " + conductor.name
        + " needs a positive thickness at or above the ground plane");
  }
  if (conductor.min_width < 0 || conductor.min_spacing < 0) {
    throw std::invalid_argument("conductor " + conductor.name
                                + " has a negative minimum width or spacing");
  }
  for (const ConformalDielectric& cf : conductor.conformals) {
    requireEps(cf.eps, "conformal dielectric " + cf.name);
    if (!(cf.side_thickness >= 0 && cf.top_thickness >= 0)) {
      throw std::invalid_argument("conformal dielectric " + cf.name
                                  + " has a negative thickness");
    }
  }
  conductors_.push_back(conductor);
  return Layer(static_cast<int>(conductors_.size()) - 1);
}

Stack::Layer Stack::addVia(const Via& via)
{
  if (via.name.empty() || layer(via.name).valid()) {
    throw std::invalid_argument("via '" + via.name
                                + "' needs a name no layer has");
  }
  if (!isConductor(via.upper)) {
    throw std::invalid_argument("via " + via.name
                                + " needs a conductor above it");
  }
  if (via.lower.valid() && !isConductor(via.lower)) {
    throw std::invalid_argument("via " + via.name
                                + " needs a conductor or the ground plane "
                                  "below it");
  }
  const double bottom = via.lower.valid() ? conductor(via.lower).top : 0;
  if (conductor(via.upper).bottom < bottom - kHeightTolerance) {
    throw std::invalid_argument(
        "via " + via.name + " connects " + name(via.upper) + " and "
        + (via.lower.valid() ? name(via.lower) : "the ground plane")
        + ", which overlap in height");
  }
  if (via.lower == via.upper) {
    throw std::invalid_argument("via " + via.name + " connects "
                                + name(via.upper) + " to itself");
  }
  vias_.push_back(via);
  return Layer(static_cast<int>(conductors_.size() + vias_.size()) - 1);
}

Stack::Layer Stack::layer(const std::string_view name) const
{
  for (size_t i = 0; i < conductors_.size(); i++) {
    if (conductors_[i].name == name) {
      return Layer(static_cast<int>(i));
    }
  }
  for (size_t i = 0; i < vias_.size(); i++) {
    if (vias_[i].name == name) {
      return Layer(static_cast<int>(conductors_.size() + i));
    }
  }
  return Layer();
}

std::vector<Stack::Layer> Stack::conductorLayers() const
{
  std::vector<Layer> layers;
  for (size_t i = 0; i < conductors_.size(); i++) {
    layers.push_back(Layer(static_cast<int>(i)));
  }
  return layers;
}

std::vector<Stack::Layer> Stack::viaLayers() const
{
  std::vector<Layer> layers;
  for (size_t i = 0; i < vias_.size(); i++) {
    layers.push_back(Layer(static_cast<int>(conductors_.size() + i)));
  }
  return layers;
}

bool Stack::isConductor(const Layer layer) const
{
  return layer.valid() && layer.index() < static_cast<int>(conductors_.size());
}

bool Stack::isVia(const Layer layer) const
{
  return layer.valid() && !isConductor(layer)
         && layer.index() < static_cast<int>(conductors_.size() + vias_.size());
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
  return conductors_[layer.index()];
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
  return vias_[layer.index() - conductors_.size()];
}

const std::string& Stack::name(const Layer layer) const
{
  if (isConductor(layer)) {
    return conductors_[layer.index()].name;
  }
  if (isVia(layer)) {
    return vias_[layer.index() - conductors_.size()].name;
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
  return {v.lower.valid() ? conductor(v.lower).top : 0,
          conductor(v.upper).bottom};
}

double Stack::epsAt(const double z) const
{
  for (const Dielectric& d : dielectrics_) {
    if (z < d.top) {
      return d.eps;
    }
  }
  return background_eps_;
}

std::vector<Stack::PlanarInterface> Stack::planarInterfaces() const
{
  std::vector<PlanarInterface> interfaces;
  for (size_t i = 0; i < dielectrics_.size(); i++) {
    const double above = i + 1 < dielectrics_.size() ? dielectrics_[i + 1].eps
                                                     : background_eps_;
    if (above != dielectrics_[i].eps) {
      interfaces.push_back({dielectrics_[i].top, dielectrics_[i].eps, above});
    }
  }
  return interfaces;
}

}  // namespace fsv
