// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#pragma once

#include <compare>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace fsv {

// A process stack in µm, with z = 0 at the ground plane. Planar dielectric
// layers are listed bottom-up and tile [0, top) without gaps; above the top
// layer the background dielectric extends to infinity. Conductors and vias
// are added in that order and addressed by handle afterwards; every add
// checks its layer, so a stack never holds one it cannot use.
class Stack
{
 public:
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

  // A table over widths and spacings, as the ITF *_VS_WIDTH_AND_SPACING
  // keywords give it: both axes ascending, values row by row per width.
  struct WidthSpacingTable
  {
    std::vector<double> widths;
    std::vector<double> spacings;
    std::vector<double> values;  // [width][spacing]

    bool empty() const { return values.empty(); }
    // Throws std::out_of_range off the grid.
    double at(const int width, const int spacing) const
    {
      if (spacing < 0 || spacing >= static_cast<int>(spacings.size())) {
        throw std::out_of_range("spacing index off the table");
      }
      return values.at(width * spacings.size() + spacing);
    }
  };

  // A quantity against one variable, ascending in x.
  struct Curve
  {
    std::vector<double> x;
    std::vector<double> y;

    bool empty() const { return x.empty(); }
  };

  // The temperature coefficients of a resistance: R(T) = R(T0) × (1 + crt1
  // (T - T0) + crt2 (T - T0)²).
  struct Crt
  {
    double crt1 = 0;
    double crt2 = 0;
  };

  // Temperature coefficients against a width or area, ascending in x.
  struct CrtCurve
  {
    std::vector<double> x;
    std::vector<Crt> crt;

    bool empty() const { return x.empty(); }
  };

  // An etch of each sidewall, µm, positive shrinking the drawn width, that
  // varies with the wire's width and its spacing to the neighbor on that
  // side (ITF ETCH_VS_WIDTH_AND_SPACING).
  struct EtchTable
  {
    enum class Affects
    {
      kBoth,
      kCapacitance,
      kResistance,
    };
    // Which wires the table applies to, against the stack's reference
    // direction.
    enum class Direction
    {
      kAll,
      kParallelToReference,
      kPerpendicularToReference,
    };
    Affects affects = Affects::kBoth;
    Direction direction = Direction::kAll;
    WidthSpacingTable table;
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
    // ITF LAYER_TYPE; "" for a routing layer.
    std::string layer_type;
    // The etch of each sidewall, µm, positive shrinking the drawn width:
    // the tables in order, then the constant for capacitance and the one
    // for resistance (ITF ETCH sets both).
    std::vector<EtchTable> etch_tables;
    double capacitive_etch = 0;
    double resistive_etch = 0;
    // Sheet resistance, ohm/square, in one of five forms, the others left
    // at zero or empty: a constant, against the silicon width, or against
    // width and spacing; or a resistivity rho in ohm·µm, constant or
    // against width and spacing, over the thickness.
    double rpsq = 0;
    Curve rpsq_vs_width;
    WidthSpacingTable rpsq_vs_width_spacing;
    double rho = 0;
    WidthSpacingTable rho_vs_width_spacing;
    // Temperature: the nominal temperature of the resistances, °C (the
    // stack's global one unless the layer gives its own), and the
    // coefficients, constant or against the silicon width.
    double t0 = 25;
    Crt crt;
    CrtCurve crt_vs_width;
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
    friend class Stack;
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
    // Resistance, in one of three forms, the others left at zero or
    // empty: per via of `area` µm², against the via's area, or a
    // resistivity in ohm·µm.
    double rpv = 0;
    double area = 0;
    Curve rpv_vs_area;
    double rho = 0;
    double t0 = 25;  // °C
    Crt crt;
    CrtCurve crt_vs_area;
  };

  enum class ReferenceDirection
  {
    kNone,
    kVertical,
    kHorizontal,
  };

  // Building. Each add throws std::invalid_argument for a layer that
  // does not fit: a dielectric must start where the last one ends (at 0
  // for the first) with a positive permittivity; a conductor needs a
  // name no layer has yet, a positive thickness at or above the ground
  // plane, and must come before any via, since via handles follow the
  // conductors; a via needs a name no layer has, a conductor above (its
  // upper) and a conductor or the ground plane (no handle) below it.
  void setTechnology(std::string technology);
  void setBackgroundEps(double eps);
  void setGlobalTemperature(double celsius);
  // Must be positive.
  void setHalfNodeScaleFactor(double factor);
  void setReferenceDirection(ReferenceDirection direction);
  void addDielectric(const Dielectric& dielectric);
  Layer addConductor(const Conductor& conductor);
  Layer addVia(const Via& via);

  const std::string& technology() const { return technology_; }
  double backgroundEps() const { return background_eps_; }
  double globalTemperature() const { return global_temperature_; }
  // The design shrinks by this before extraction (ITF
  // HALF_NODE_SCALE_FACTOR); 1 when none.
  double halfNodeScaleFactor() const { return half_node_scale_factor_; }
  // The direction the etch tables' PARALLEL_TO_REFERENCE and
  // PERPENDICULAR_TO_REFERENCE refer to.
  ReferenceDirection referenceDirection() const { return reference_direction_; }
  const std::vector<Dielectric>& dielectrics() const { return dielectrics_; }
  const std::vector<Conductor>& conductors() const { return conductors_; }
  const std::vector<Via>& vias() const { return vias_; }

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
  // that names no layer.
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

 private:
  std::string technology_;
  double background_eps_ = 1.0;
  double global_temperature_ = 25;  // °C
  double half_node_scale_factor_ = 1;
  ReferenceDirection reference_direction_ = ReferenceDirection::kNone;
  std::vector<Dielectric> dielectrics_;
  std::vector<Conductor> conductors_;
  std::vector<Via> vias_;
};

}  // namespace fsv
