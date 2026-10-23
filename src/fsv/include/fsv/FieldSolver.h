// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#pragma once

#include <array>
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "fsv/Geometry.h"
#include "fsv/Solver.h"
#include "fsv/Stack.h"

namespace utl {
class Logger;
}

namespace fsv {

class Scene;

// One master's row of the capacitance matrix, in fF.
struct NetCapacitance
{
  std::string net;
  CapRow row;
  // Net names by column index; the ground plane is reported separately.
  std::vector<std::string> nets;
  double seconds = 0;
};

// The state behind the field-solver commands: a dielectric (a uniform
// permittivity or a process stack), the conductor geometry, and a Solver
// reused across solves. Errors are reported through the logger.
class FieldSolver
{
 public:
  explicit FieldSolver(utl::Logger* logger);
  ~FieldSolver();

  // Replaces the dielectric with the stack of an ITF file: its planar
  // layers and conformal dielectrics.
  void readItf(const std::string& path);
  // Replaces the dielectric with a uniform relative permittivity.
  void setPermittivity(double eps);

  // Adds a box in µm to a net, creating the net on first use.
  void addBox(std::string_view net, const Box& box);
  // Adds a rectangle in µm on a conductor or via layer of the stack; the
  // stack gives its z extent.
  void addLayerRect(std::string_view net,
                    std::string_view layer,
                    const std::array<double, 4>& rect);
  // Removes every net and box; the dielectric is kept.
  void clearGeometry();

  std::vector<std::string> netNames() const;
  // Solves one master net and reports its row. The Solver is rebuilt only
  // when solver_options change.
  NetCapacitance solve(std::string_view net,
                       const SolverOptions& solver_options,
                       const SolveOptions& options);

 private:
  const Scene& scene();
  int findNet(std::string_view net) const;
  void report(const NetCapacitance& result) const;

  utl::Logger* logger_;
  Stack stack_;
  bool has_stack_ = false;
  double eps_ = 1.0;
  Geometry geometry_;
  std::map<std::string, int, std::less<>> net_index_;
  // Built on demand from the dielectric and geometry; reset when either
  // changes.
  std::unique_ptr<Scene> scene_;
  std::unique_ptr<Solver> solver_;
  SolverOptions solver_options_;
};

}  // namespace fsv
