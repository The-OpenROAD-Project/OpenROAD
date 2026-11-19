// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include "Resistance.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#include "fsv/Stack.h"
#include "odb/db.h"
#include "odb/dbShape.h"
#include "odb/dbTypes.h"
#include "odb/geom.h"

namespace pex {

namespace {

// Linear in x between the points, constant past the ends.
double interpolate(const std::vector<double>& x,
                   const std::vector<double>& y,
                   const double at)
{
  if (at <= x.front()) {
    return y.front();
  }
  if (at >= x.back()) {
    return y.back();
  }
  const size_t i = std::ranges::upper_bound(x, at) - x.begin();
  const double t = (at - x[i - 1]) / (x[i] - x[i - 1]);
  return ((1 - t) * y[i - 1]) + (t * y[i]);
}

// The value at the point nearest in x.
double nearest(const std::vector<double>& x,
               const std::vector<double>& y,
               const double at)
{
  size_t best = 0;
  for (size_t i = 1; i < x.size(); i++) {
    if (std::abs(x[i] - at) < std::abs(x[best] - at)) {
      best = i;
    }
  }
  return y[best];
}

// R(T) / R(T0) for coefficients against x (a width or area), or constant.
double temperatureFactor(const fsv::Stack::Crt& crt,
                         const fsv::Stack::CrtCurve& curve,
                         const double x,
                         const double t0,
                         const double temperature)
{
  double crt1 = crt.crt1;
  double crt2 = crt.crt2;
  if (!curve.empty()) {
    std::vector<double> c1;
    std::vector<double> c2;
    for (const fsv::Stack::Crt& c : curve.crt) {
      c1.push_back(c.crt1);
      c2.push_back(c.crt2);
    }
    crt1 = interpolate(curve.x, c1, x);
    crt2 = interpolate(curve.x, c2, x);
  }
  const double dt = temperature - t0;
  return 1 + (crt1 * dt) + (crt2 * dt * dt);
}

// Why a conductor's resistance cannot be computed yet, or "".
std::string_view unsupported(const fsv::Stack::Conductor& c)
{
  if (!c.rpsq_vs_width_spacing.empty() || !c.rho_vs_width_spacing.empty()) {
    return "resistance against width and spacing";
  }
  for (const fsv::Stack::EtchTable& e : c.etch_tables) {
    if (e.affects != fsv::Stack::EtchTable::Affects::kCapacitance) {
      return "an etch table for resistance";
    }
  }
  if (c.rpsq <= 0 && c.rpsq_vs_width.empty() && c.rho <= 0) {
    return "no resistance";
  }
  return "";
}

}  // namespace

Resistance::Resistance(const fsv::Stack& stack, odb::dbTech* tech)
    : stack_(stack),
      dbu_(tech->getDbUnitsPerMicron()),
      scale_(stack.halfNodeScaleFactor())
{
  for (odb::dbTechLayer* layer : tech->getLayers()) {
    if (layer->getType() == odb::dbTechLayerType::ROUTING) {
      const fsv::Stack::Layer c = stack.layer(layer->getName());
      if (!stack.isConductor(c)) {
        unusable_[layer] = "the stack has no conductor for it";
        continue;
      }
      const std::string_view why = unsupported(stack.conductor(c));
      if (!why.empty()) {
        unusable_[layer] = "unsupported resistance data: " + std::string(why);
        continue;
      }
      conductors_[layer] = c;
    } else if (layer->getType() == odb::dbTechLayerType::CUT) {
      odb::dbTechLayer* lower = layer->getLowerLayer();
      odb::dbTechLayer* upper = layer->getUpperLayer();
      if (lower == nullptr || upper == nullptr) {
        continue;
      }
      const fsv::Stack::Layer below = stack.layer(lower->getName());
      const fsv::Stack::Layer above = stack.layer(upper->getName());
      for (const fsv::Stack::Layer v : stack.viaLayers()) {
        const fsv::Stack::Via& via = stack.via(v);
        if (via.lower == below && via.upper == above) {
          vias_[layer] = v;
          break;
        }
      }
    }
  }
}

const fsv::Stack::Conductor& Resistance::conductor(
    odb::dbTechLayer* layer) const
{
  const auto it = conductors_.find(layer);
  if (it == conductors_.end()) {
    const auto why = unusable_.find(layer);
    throw std::invalid_argument("a wire on " + layer->getName() + ": "
                                + (why != unusable_.end()
                                       ? why->second
                                       : std::string("not a routing layer")));
  }
  return stack_.conductor(it->second);
}

double Resistance::wire(odb::dbTechLayer* layer,
                        const int width,
                        const int length) const
{
  const fsv::Stack::Conductor& c = conductor(layer);
  const double silicon = (width / dbu_ * scale_) - (2 * c.resistive_etch);
  if (silicon <= 0) {
    throw std::invalid_argument("a wire on " + layer->getName()
                                + " has no width left after its etch");
  }
  double sheet = c.rpsq;
  if (!c.rpsq_vs_width.empty()) {
    sheet = interpolate(c.rpsq_vs_width.x, c.rpsq_vs_width.y, silicon);
  } else if (sheet <= 0) {
    sheet = c.rho / (c.top - c.bottom);
  }
  return sheet * (length / dbu_ * scale_) / silicon
         * temperatureFactor(
             c.crt, c.crt_vs_width, silicon, c.t0, stack_.globalTemperature());
}

double Resistance::cut(odb::dbTechLayer* layer, const odb::Rect& cut) const
{
  const auto it = vias_.find(layer);
  if (it == vias_.end()) {
    throw std::invalid_argument("the stack has no via for cut layer "
                                + layer->getName());
  }
  const fsv::Stack::Via& v = stack_.via(it->second);
  const double area = (cut.dx() / dbu_ * scale_) * (cut.dy() / dbu_ * scale_);
  if (area <= 0) {
    throw std::invalid_argument("a cut on " + layer->getName()
                                + " has no area");
  }
  double r = 0;
  if (!v.rpv_vs_area.empty()) {
    r = v.interpolate_rpv_vs_area
            ? interpolate(v.rpv_vs_area.x, v.rpv_vs_area.y, area)
            : nearest(v.rpv_vs_area.x, v.rpv_vs_area.y, area);
  } else if (v.rpv > 0) {
    r = v.area > 0 ? v.rpv * v.area / area : v.rpv;
  } else if (v.rho > 0) {
    const fsv::Stack::ZExtent z = stack_.zExtent(it->second);
    r = v.rho * (z.top - z.bottom) / area;
  } else {
    throw std::invalid_argument("the stack gives no resistance for via "
                                + v.name);
  }
  return r
         * temperatureFactor(
             v.crt, v.crt_vs_area, area, v.t0, stack_.globalTemperature());
}

double Resistance::via(const odb::dbShape& via) const
{
  std::vector<odb::dbShape> boxes;
  odb::dbShape::getViaBoxes(via, boxes);
  double conductance = 0;
  for (const odb::dbShape& box : boxes) {
    odb::dbTechLayer* layer = box.getTechLayer();
    if (layer != nullptr && layer->getType() == odb::dbTechLayerType::CUT) {
      conductance += 1 / cut(layer, box.getBox());
    }
  }
  if (conductance <= 0) {
    throw std::invalid_argument("a via with no cut");
  }
  return 1 / conductance;
}

}  // namespace pex
