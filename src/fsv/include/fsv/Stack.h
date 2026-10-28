// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#pragma once

#include <compare>
#include <string>
#include <string_view>
#include <vector>

namespace fsv {

// A process stack in µm, with z = 0 at the ground plane. Planar dielectric
// layers are listed bottom-up and tile [0, top) without gaps; above the top
// layer the background dielectric extends to infinity.
struct Stack
{
  struct Dielectric
  {
    std::string name;
    double bottom;  // z of the bottom face, µm
    double top;     // z of the top face, µm
    double eps;
  };

  // A conformal dielectric wrapped around a conductor (ITF IS_CONFORMAL).
  struct ConformalDielectric
  {
    std::string name;
    double side_thickness;  // SW_T: on the conductor's sides
    double top_thickness;   // TW_T: on the conductor's top
    double eps;
  };

  struct Conductor
  {
    std::string name;
    double bottom;  // z of the bottom face, µm
    double top;     // z of the top face, µm
    double min_width = 0;
    double min_spacing = 0;
    // Conformal dielectrics in nesting order, innermost first.
    std::vector<ConformalDielectric> conformals;
  };

  // A handle to a conductor or via of the stack that made it: an index,
  // so it stays valid across copies of that stack, but not across changes
  // to its conductors or vias, and it means nothing to another stack. A
  // default handle names no layer.
  class Layer
  {
   public:
    constexpr Layer() = default;
    constexpr bool valid() const { return index_ >= 0; }
    constexpr int index() const { return index_; }
    constexpr auto operator<=>(const Layer&) const = default;

   private:
    friend struct Stack;
    explicit constexpr Layer(const int index) : index_(index) {}
    int index_ = -1;
  };

  // A via between two conductor layers; it spans from the top of `lower` to
  // the bottom of `upper`. A `lower` that names no layer is the ground
  // plane (an ITF SUBSTRATE via).
  struct Via
  {
    std::string name;
    Layer upper;
    Layer lower;
  };

  std::string technology;
  double background_eps = 1.0;
  std::vector<Dielectric> dielectrics;
  std::vector<Conductor> conductors;
  std::vector<Via> vias;

  // The conductor or via of a name; a handle that names no layer when
  // the stack has neither.
  Layer layer(std::string_view name) const;
  // Handles to every conductor, in the stack's order.
  std::vector<Layer> conductorLayers() const;
  std::vector<Layer> viaLayers() const;
  bool isConductor(Layer layer) const;
  bool isVia(Layer layer) const;
  // The conductor (or via) behind a handle. Throws std::invalid_argument
  // for a handle that names no layer, one past the stack's layers, or one
  // of the other kind.
  const Conductor& conductor(Layer layer) const;
  const Via& via(Layer layer) const;
  const std::string& name(Layer layer) const;

  // The z extent of a layer: a conductor's own, or a via's from the top of
  // its lower conductor (the ground plane, 0, for a substrate via) to the
  // bottom of its upper one. Throws std::invalid_argument for a handle
  // that names no layer, or a via without an upper conductor.
  struct ZExtent
  {
    double bottom = 0;
    double top = 0;
  };
  ZExtent zExtent(Layer layer) const;
  // Relative permittivity of the planar stack at height z (z >= 0).
  double epsAt(double z) const;
  // A horizontal boundary in the planar stack where the permittivity
  // changes.
  struct PlanarInterface
  {
    double z;  // µm
    double eps_below;
    double eps_above;
  };

  // The planar stack's interfaces in increasing z, including the top of the
  // stack when the background differs. Adjacent layers with equal
  // permittivity form no interface. These describe the stack away from
  // conductors and conformal dielectrics: inside a conformal dielectric its
  // own permittivity applies instead, and conformal boundaries are per-wire
  // geometry, not part of the stack.
  std::vector<PlanarInterface> planarInterfaces() const;
};

}  // namespace fsv
