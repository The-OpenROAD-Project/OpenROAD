// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include "Structures.h"

#include <fstream>
#include <map>
#include <sstream>
#include <stdexcept>
#include <string>

#include "fsv/Geometry.h"
#include "fsv/Stack.h"

namespace fsv {

Structure readStructure(const std::string& name, const double length)
{
  Structure s;
  s.stack.technology = name;
  std::map<std::string, int> nets;
  std::ifstream in(std::string(kGoldenDir) + name + ".txt");
  if (!in.good()) {
    throw std::runtime_error("cannot read structure " + name);
  }
  std::string line;
  double bottom = 0;
  while (std::getline(in, line)) {
    std::istringstream words(line.substr(0, line.find('#')));
    std::string kind;
    if (!(words >> kind)) {
      continue;
    }
    if (kind == "background") {
      words >> s.stack.background_eps;
    } else if (kind == "layer") {
      double top = 0;
      double eps = 0;
      words >> top >> eps;
      s.stack.dielectrics.push_back(
          {.name = "layer", .bottom = bottom, .top = top, .eps = eps});
      bottom = top;
    } else if (kind == "box") {
      std::string net;
      Box b;
      words >> net >> b.lo[0] >> b.lo[1] >> b.lo[2] >> b.hi[0] >> b.hi[1]
          >> b.hi[2];
      if (length > 0) {
        b.lo[1] = 0;
        b.hi[1] = length;
      }
      if (!nets.contains(net)) {
        nets[net] = s.geometry.addNet(net);
      }
      s.geometry.addBox(nets[net], b);
    } else if (kind == "conformal") {
      Stack::ConformalDielectric c;
      double layer_bottom = 0;
      double layer_top = 0;
      words >> c.name >> layer_bottom >> layer_top >> c.side_thickness
          >> c.top_thickness >> c.eps;
      Stack::Conductor* layer = nullptr;
      for (Stack::Conductor& existing : s.stack.conductors) {
        if (existing.bottom == layer_bottom && existing.top == layer_top) {
          layer = &existing;
        }
      }
      if (layer == nullptr) {
        layer = &s.stack.conductors.emplace_back(Stack::Conductor{
            .name = "layer" + std::to_string(s.stack.conductors.size()),
            .bottom = layer_bottom,
            .top = layer_top,
            .conformals = {}});
      }
      layer->conformals.push_back(c);
    }
  }
  return s;
}

Geometry2D crossSection(const Geometry& geometry)
{
  Geometry2D section;
  for (int net = 0; net < geometry.netCount(); net++) {
    section.addNet(geometry.netName(net));
  }
  for (int b = 0; b < static_cast<int>(geometry.boxes().size()); b++) {
    const Box& box = geometry.boxes()[b];
    section.addRect(geometry.boxNet(b),
                    {{box.lo[0], box.lo[2]}, {box.hi[0], box.hi[2]}});
  }
  return section;
}

}  // namespace fsv
