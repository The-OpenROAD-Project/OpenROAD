// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#pragma once

#include <array>
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "fsv/Geometry.h"
#include "fsv/Solver.h"
#include "fsv/Stack.h"

namespace odb {
class dbDatabase;
}

namespace utl {
class Logger;
}

namespace fsv {

class Scene;
class Scene2D;

// One net's row of the capacitance matrix, in fF, or fF/µm for a cross
// section.
struct NetCapacitance
{
  std::string net;
  CapRow row;
  bool cross_section = false;
  // Net names by column index; the ground plane is reported separately.
  std::vector<std::string> nets;
  // By column index: grounded nets, whose couplings count toward ground.
  std::vector<bool> grounded;
  // The geometry came from a window, so row.window_leakage applies.
  bool windowed = false;
  double seconds = 0;
};

// The state behind the field-solver commands: a dielectric (a uniform
// permittivity or a process stack), the conductor geometry, and a Solver
// reused across solves. The geometry is either 3D boxes or a cross section
// of rectangles in the x-z plane, whose wires run along y; the first shape
// added after a clear decides which. Errors are reported through the
// logger.
class FieldSolver
{
 public:
  FieldSolver(odb::dbDatabase* db, utl::Logger* logger);
  ~FieldSolver();

  // Replaces the dielectric with the stack of an ITF file: its planar
  // layers and conformal dielectrics.
  void readItf(const std::string& path);
  // Replaces the dielectric with a uniform relative permittivity.
  void setPermittivity(double eps);

  // Adds a box in µm to a net, creating the net on first use.
  void addBox(std::string_view net, const Box& box);
  // Adds a box on a conductor or via layer of the stack, given by its
  // rectangle in the x-y plane in µm; the stack gives its z extent.
  void addLayerRect(std::string_view net,
                    std::string_view layer,
                    const std::array<double, 4>& rect);
  // Adds a cross-section rectangle in µm to a net, creating the net on
  // first use.
  void addRect(std::string_view net, const Rect& rect);
  // Adds a cross-section rectangle on a conductor or via layer of the
  // stack, spanning [x0, x1] in µm; the stack gives its z extent.
  void addLayerSpan(std::string_view net,
                    std::string_view layer,
                    double x0,
                    double x1);
  // Removes every net and shape; the dielectric is kept.
  void clearGeometry();

  // Replaces the geometry with nets of the design and their neighbours
  // within a halo (see readNetWindow; a negative halo or margin takes the
  // default), and makes the nets the default targets. Design layers map to
  // stack layers by name, after `layer_map`.
  void addNets(
      const std::vector<std::string>& nets,
      double halo,
      double margin,
      const std::map<std::string, std::string, std::less<>>& layer_map);

  std::vector<std::string> netNames() const;
  // The nets to solve when none are named: those given to addNets, else
  // every net that is not grounded.
  std::vector<std::string> defaultTargets() const;
  // Solves one net and reports its row, per unit length for a cross
  // section. The Solver is rebuilt only when solver_options change.
  NetCapacitance solve(std::string_view net,
                       const SolverOptions& solver_options,
                       const SolveOptions& options);

 private:
  // Checks that the geometry takes boxes (or cross-section rectangles),
  // and fixes its kind if it is empty.
  void setKind(bool cross_section);
  // Runs `check`, which adds the shape to a scratch geometry, so that a
  // rejected shape leaves no empty net.
  void checkShape(std::string_view net, const std::function<void()>& check);
  // The z extent of a conductor or via layer of the stack.
  std::pair<double, double> layerExtent(std::string_view layer) const;
  // The net's index, creating the net on first use.
  int netIndex(std::string_view net);
  int netCount() const;
  void buildScene();
  void resetScenes();
  int findNet(std::string_view net) const;
  void report(const NetCapacitance& result) const;

  odb::dbDatabase* db_;
  utl::Logger* logger_;
  Stack stack_;
  bool has_stack_ = false;
  double eps_ = 1.0;
  // One of these holds the shapes, as cross_section_ says; the other is
  // empty.
  Geometry geometry_;
  Geometry2D section_;
  bool cross_section_ = false;
  std::map<std::string, int, std::less<>> net_index_;
  std::vector<std::string> targets_;  // from addNets
  // Built on demand from the dielectric and geometry; reset when either
  // changes.
  std::unique_ptr<Scene> scene_;
  std::unique_ptr<Scene2D> section_scene_;
  std::unique_ptr<Solver> solver_;
  SolverOptions solver_options_;
};

}  // namespace fsv
