// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include "fsv/FieldSolver.h"

#include <array>
#include <chrono>
#include <cmath>
#include <exception>
#include <memory>
#include <set>
#include <string>
#include <string_view>
#include <vector>

#include "fsv/Geometry.h"
#include "fsv/Itf.h"
#include "fsv/Scene.h"
#include "fsv/Solver.h"
#include "fsv/Stack.h"
#include "utl/Logger.h"

namespace fsv {

namespace {

// The message of an error from the solver core, without its "fsv: " prefix:
// the logger adds the tool id.
std::string_view coreMessage(const std::exception& e)
{
  std::string_view text = e.what();
  if (text.starts_with("fsv: ")) {
    text.remove_prefix(5);
  }
  return text;
}

}  // namespace

FieldSolver::FieldSolver(utl::Logger* logger) : logger_(logger)
{
}

FieldSolver::~FieldSolver() = default;

void FieldSolver::readItf(const std::string& path)
{
  Stack stack;
  try {
    stack = readItfFile(path);
  } catch (const std::exception& e) {
    logger_->error(utl::FSV, 1, "{}", coreMessage(e));
  }
  std::set<std::string> conformals;
  for (const Stack::Conductor& c : stack.conductors) {
    for (const Stack::ConformalDielectric& cf : c.conformals) {
      conformals.insert(cf.name);
    }
  }
  logger_->info(utl::FSV,
                2,
                "Read ITF {}: {} dielectrics, {} conductors, {} conformal "
                "dielectrics, {} vias.",
                stack.technology,
                stack.dielectrics.size(),
                stack.conductors.size(),
                conformals.size(),
                stack.vias.size());
  scene_.reset();
  stack_ = std::move(stack);
  has_stack_ = true;
}

void FieldSolver::setPermittivity(const double eps)
{
  if (!(eps > 0) || !std::isfinite(eps)) {
    logger_->error(utl::FSV, 4, "The permittivity must be positive.");
  }
  scene_.reset();
  stack_ = Stack();
  has_stack_ = false;
  eps_ = eps;
}

void FieldSolver::addBox(const std::string_view net, const Box& box)
{
  int index = findNet(net);
  const bool is_new = index < 0;
  if (is_new) {
    index = geometry_.netCount();
  }
  // Validate before creating the net, so a rejected box leaves no empty net.
  Geometry check;
  try {
    check.addBox(check.addNet(net), box);
  } catch (const std::exception& e) {
    logger_->error(utl::FSV, 5, "Net {}: {}", net, coreMessage(e));
  }
  scene_.reset();
  if (is_new) {
    geometry_.addNet(net);
    net_index_.emplace(std::string(net), index);
  }
  geometry_.addBox(index, box);
}

void FieldSolver::addLayerRect(const std::string_view net,
                               const std::string_view layer,
                               const std::array<double, 4>& rect)
{
  if (!has_stack_) {
    logger_->error(
        utl::FSV, 6, "Read an ITF file before adding boxes by layer.");
  }
  double bottom = 0;
  double top = 0;
  if (const Stack::Conductor* c = stack_.findConductor(layer)) {
    bottom = c->bottom;
    top = c->top;
  } else {
    // A via spans from the top of its lower conductor to the bottom of its
    // upper one.
    const Stack::Via* via = nullptr;
    for (const Stack::Via& v : stack_.vias) {
      if (v.name == layer) {
        via = &v;
      }
    }
    if (via == nullptr) {
      logger_->error(utl::FSV,
                     7,
                     "Layer {} is not a conductor or via of the ITF stack.",
                     layer);
    }
    const Stack::Conductor* lower = stack_.findConductor(via->lower);
    const Stack::Conductor* upper = stack_.findConductor(via->upper);
    if (lower == nullptr) {
      // A SUBSTRATE via lands on the ground plane: its net is grounded and
      // has no capacitance to solve.
      logger_->error(utl::FSV,
                     14,
                     "Via {} lands on the ground plane, which would ground "
                     "its net.",
                     layer);
    }
    if (upper == nullptr) {
      logger_->error(utl::FSV,
                     13,
                     "Via {} has no upper conductor {} in the ITF stack.",
                     layer,
                     via->upper);
    }
    bottom = lower->top;
    top = upper->bottom;
  }
  addBox(net, {{rect[0], rect[1], bottom}, {rect[2], rect[3], top}});
}

void FieldSolver::clearGeometry()
{
  scene_.reset();
  geometry_ = Geometry();
  net_index_.clear();
}

std::vector<std::string> FieldSolver::netNames() const
{
  std::vector<std::string> names;
  for (int net = 0; net < geometry_.netCount(); net++) {
    names.push_back(geometry_.netName(net));
  }
  return names;
}

int FieldSolver::findNet(const std::string_view net) const
{
  const auto it = net_index_.find(net);
  return it == net_index_.end() ? -1 : it->second;
}

const Scene& FieldSolver::scene()
{
  if (!scene_) {
    try {
      scene_ = has_stack_ ? std::make_unique<Scene>(geometry_, stack_)
                          : std::make_unique<Scene>(geometry_, eps_);
    } catch (const std::exception& e) {
      logger_->error(utl::FSV, 8, "{}", coreMessage(e));
    }
  }
  return *scene_;
}

NetCapacitance FieldSolver::solve(const std::string_view net,
                                  const SolverOptions& solver_options,
                                  const SolveOptions& options)
{
  const int master = findNet(net);
  if (master < 0) {
    logger_->error(utl::FSV, 9, "Net {} has no boxes.", net);
  }
  if (!solver_ || solver_options_ != solver_options) {
    solver_.reset();
    try {
      solver_ = std::make_unique<Solver>(solver_options);
    } catch (const std::exception& e) {
      logger_->error(utl::FSV, 12, "{}", coreMessage(e));
    }
    solver_options_ = solver_options;
  }
  // Outside the try below, so that its own error is not reported again.
  const Scene& s = scene();
  NetCapacitance result;
  result.net = std::string(net);
  result.nets = netNames();
  try {
    const auto start = std::chrono::steady_clock::now();
    result.row = solver_->solve(s, master, options);
    result.seconds = std::chrono::duration<double>(
                         std::chrono::steady_clock::now() - start)
                         .count();
  } catch (const std::exception& e) {
    logger_->error(utl::FSV, 10, "Net {}: {}", net, coreMessage(e));
  }
  report(result);
  return result;
}

// Couplings are reported as positive capacitances, with their standard
// errors.
void FieldSolver::report(const NetCapacitance& result) const
{
  const CapRow& row = result.row;
  logger_->info(utl::FSV,
                11,
                "Net {}: total {:.5g} fF +- {:.2g} after {} walks ({:.1f} "
                "hops per walk).",
                result.net,
                row.total(),
                row.sigma[row.master],
                row.walks,
                static_cast<double>(row.hops) / row.walks);
  logger_->report("  {:<20} {:>12.5g} +- {:.2g} fF",
                  "ground",
                  -row.ground,
                  row.ground_sigma);
  for (int net = 0; net < static_cast<int>(row.cap.size()); net++) {
    if (net != row.master) {
      logger_->report("  {:<20} {:>12.5g} +- {:.2g} fF",
                      result.nets[net],
                      row.coupling(net),
                      row.sigma[net]);
    }
  }
}

}  // namespace fsv
