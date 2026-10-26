// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#pragma once

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

  // A via between two conductor layers; it spans from the top of `lower` to
  // the bottom of `upper`. `lower` is "SUBSTRATE" for a via that lands on the
  // ground plane.
  struct Via
  {
    std::string name;
    std::string upper;
    std::string lower;
  };

  std::string technology;
  double background_eps = 1.0;
  std::vector<Dielectric> dielectrics;
  std::vector<Conductor> conductors;
  std::vector<Via> vias;

  const Conductor* findConductor(std::string_view name) const;

  // The z extent of a conductor or via layer: a conductor's own, or a via's
  // from the top of its lower conductor to the bottom of its upper one.
  struct ZExtent
  {
    enum Status
    {
      kFound,
      kUnknownLayer,
      kViaToGround,  // lands on the ground plane (a SUBSTRATE via)
      kViaWithoutUpper,
    };
    Status status = kUnknownLayer;
    double bottom = 0;
    double top = 0;
    const Via* via = nullptr;  // for a via layer
  };
  ZExtent zExtent(std::string_view layer) const;
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
