// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include "fsv/FieldSolver.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <exception>
#include <functional>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "fsv/Geometry.h"
#include "fsv/Itf.h"
#include "fsv/NetWindow.h"
#include "fsv/Scene.h"
#include "fsv/Solver.h"
#include "fsv/Stack.h"
#include "odb/db.h"
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

FieldSolver::FieldSolver(odb::dbDatabase* db, utl::Logger* logger)
    : db_(db), logger_(logger)
{
}

FieldSolver::~FieldSolver() = default;

void FieldSolver::readItf(const std::string& path)
{
  Stack stack;
  std::vector<ItfWarning> warnings;
  try {
    stack = readItfFile(path, &warnings);
  } catch (const std::exception& e) {
    logger_->error(utl::FSV, 1, "{}", coreMessage(e));
  }
  for (const ItfWarning& w : warnings) {
    logger_->warn(utl::FSV, 26, "ITF line {}: {}", w.line, w.message);
  }
  for (const Stack::Conductor& c : stack.conductors()) {
    bool etched = c.capacitive_etch != 0;
    for (const Stack::EtchTable& table : c.etch_tables) {
      etched
          = etched || table.affects != Stack::EtchTable::Affects::kResistance;
    }
    if (etched) {
      // Solving as drawn would give capacitances of the wrong geometry.
      logger_->error(utl::FSV,
                     19,
                     "Conductor {} has a capacitive etch, which the solver "
                     "does not apply.",
                     c.name);
    }
  }
  std::set<std::string> conformals;
  for (const Stack::Conductor& c : stack.conductors()) {
    for (const Stack::ConformalDielectric& cf : c.conformals) {
      conformals.insert(cf.name);
    }
  }
  logger_->info(utl::FSV,
                2,
                "Read ITF {}: {} dielectrics, {} conductors, {} conformal "
                "dielectrics, {} vias.",
                stack.technology(),
                stack.dielectrics().size(),
                stack.conductors().size(),
                conformals.size(),
                stack.vias().size());
  resetScenes();
  stack_ = std::move(stack);
  has_stack_ = true;
}

void FieldSolver::setPermittivity(const double eps)
{
  if (!(eps > 0) || !std::isfinite(eps)) {
    logger_->error(utl::FSV, 4, "The permittivity must be positive.");
  }
  resetScenes();
  stack_ = Stack();
  has_stack_ = false;
  eps_ = eps;
}

void FieldSolver::setKind(const bool cross_section)
{
  if (!net_index_.empty() && cross_section != cross_section_) {
    logger_->error(utl::FSV,
                   15,
                   "The field solver holds {}; run clear_field_solver before "
                   "adding {}.",
                   cross_section_ ? "a cross section" : "boxes",
                   cross_section ? "cross-section rectangles" : "boxes");
  }
  cross_section_ = cross_section;
}

void FieldSolver::checkShape(const std::string_view net,
                             const std::function<void()>& check)
{
  try {
    check();
  } catch (const std::exception& e) {
    logger_->error(utl::FSV, 5, "Net {}: {}", net, coreMessage(e));
  }
}

int FieldSolver::netCount() const
{
  return cross_section_ ? section_.netCount() : geometry_.netCount();
}

int FieldSolver::netIndex(const std::string_view net)
{
  const auto [it, added] = net_index_.try_emplace(std::string(net), netCount());
  if (added) {
    if (cross_section_) {
      section_.addNet(net);
    } else {
      geometry_.addNet(net);
    }
  }
  return it->second;
}

void FieldSolver::addBox(const std::string_view net, const Box& box)
{
  setKind(false);
  checkShape(net, [&] {
    Geometry scratch;
    scratch.addBox(scratch.addNet(net), box);
  });
  resetScenes();
  geometry_.addBox(netIndex(net), box);
}

void FieldSolver::addRect(const std::string_view net, const Rect& rect)
{
  setKind(true);
  checkShape(net, [&] {
    Geometry2D scratch;
    scratch.addRect(scratch.addNet(net), rect);
  });
  resetScenes();
  section_.addRect(netIndex(net), rect);
}

std::pair<double, double> FieldSolver::layerExtent(
    const std::string_view layer) const
{
  if (!has_stack_) {
    logger_->error(
        utl::FSV, 6, "Read an ITF file before adding shapes by layer.");
  }
  const Stack::Layer handle = stack_.layer(layer);
  if (!handle.valid()) {
    logger_->error(utl::FSV,
                   7,
                   "Layer {} is not a conductor or via of the ITF stack.",
                   layer);
  }
  if (stack_.isVia(handle) && !stack_.via(handle).lower.valid()) {
    // Its net would be grounded, with no capacitance to solve.
    logger_->error(utl::FSV,
                   14,
                   "Via {} lands on the ground plane, which would ground "
                   "its net.",
                   layer);
  }
  const Stack::ZExtent extent = stack_.zExtent(handle);
  return {extent.bottom, extent.top};
}

void FieldSolver::addLayerRect(const std::string_view net,
                               const std::string_view layer,
                               const std::array<double, 4>& rect)
{
  const auto [bottom, top] = layerExtent(layer);
  addBox(net, {{rect[0], rect[1], bottom}, {rect[2], rect[3], top}});
}

void FieldSolver::addLayerSpan(const std::string_view net,
                               const std::string_view layer,
                               const double x0,
                               const double x1)
{
  const auto [bottom, top] = layerExtent(layer);
  addRect(net, {{x0, bottom}, {x1, top}});
}

void FieldSolver::clearGeometry()
{
  resetScenes();
  geometry_ = Geometry();
  section_ = Geometry2D();
  cross_section_ = false;
  net_index_.clear();
  targets_.clear();
}

void FieldSolver::addNets(
    const std::vector<std::string>& nets,
    const double halo,
    const double margin,
    const std::map<std::string, std::string, std::less<>>& layer_map)
{
  odb::dbBlock* block = db_->getChip() ? db_->getChip()->getBlock() : nullptr;
  if (block == nullptr) {
    logger_->error(utl::FSV, 29, "No design is loaded.");
  }
  std::vector<odb::dbNet*> targets;
  std::vector<std::string> names;
  for (const std::string& name : nets) {
    odb::dbNet* net = block->findNet(name.c_str());
    if (net == nullptr) {
      logger_->error(utl::FSV, 30, "Net {} is not in the design.", name);
    }
    if (std::ranges::find(names, name) == names.end()) {
      targets.push_back(net);
      names.push_back(name);
    }
  }
  const LayerExtent extent = [&](const std::string_view layer) {
    const auto it = layer_map.find(layer);
    return layerExtent(it == layer_map.end() ? layer : it->second);
  };
  NetWindow window;
  try {
    window = readNetWindow(block, targets, extent, {halo, margin});
  } catch (const std::invalid_argument& e) {
    logger_->error(utl::FSV, 31, "{}", coreMessage(e));
  }
  clearGeometry();
  geometry_ = std::move(window.geometry);
  for (int net = 0; net < geometry_.netCount(); net++) {
    net_index_.emplace(geometry_.netName(net), net);
  }
  targets_ = std::move(names);
  logger_->info(utl::FSV,
                18,
                "Read {} target and {} neighbour nets: {} boxes, {} of them "
                "clipped and {} obstructions trimmed, within a halo of {:.3g} "
                "um and a margin of {:.3g} um.",
                targets.size(),
                window.neighbours,
                geometry_.boxes().size(),
                window.clipped,
                window.trimmed,
                window.halo,
                window.margin);
}

std::vector<std::string> FieldSolver::netNames() const
{
  std::vector<std::string> names;
  for (int net = 0; net < netCount(); net++) {
    names.emplace_back(cross_section_ ? section_.netName(net)
                                      : geometry_.netName(net));
  }
  return names;
}

std::vector<std::string> FieldSolver::defaultTargets() const
{
  if (!targets_.empty()) {
    return targets_;
  }
  std::vector<std::string> names;
  for (int net = 0; net < netCount(); net++) {
    if (cross_section_ || !geometry_.grounded(net)) {
      names.push_back(cross_section_ ? section_.netName(net)
                                     : geometry_.netName(net));
    }
  }
  return names;
}

int FieldSolver::findNet(const std::string_view net) const
{
  const auto it = net_index_.find(net);
  return it == net_index_.end() ? -1 : it->second;
}

void FieldSolver::resetScenes()
{
  section_scene_.reset();
  scene_.reset();
}

void FieldSolver::buildScene()
{
  if (scene_ || section_scene_) {
    return;
  }
  try {
    if (cross_section_) {
      section_scene_ = has_stack_ ? std::make_unique<Scene2D>(section_, stack_)
                                  : std::make_unique<Scene2D>(section_, eps_);
    } else {
      scene_ = has_stack_ ? std::make_unique<Scene>(geometry_, stack_)
                          : std::make_unique<Scene>(geometry_, eps_);
    }
  } catch (const std::exception& e) {
    logger_->error(utl::FSV, 8, "{}", coreMessage(e));
  }
}

NetCapacitance FieldSolver::solve(const std::string_view net,
                                  const SolverOptions& solver_options,
                                  const SolveOptions& options)
{
  const int master = findNet(net);
  if (master < 0) {
    logger_->error(utl::FSV, 9, "Net {} has no shapes.", net);
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
  buildScene();
  NetCapacitance result;
  result.net = std::string(net);
  result.cross_section = cross_section_;
  result.nets = netNames();
  for (int net = 0; net < netCount(); net++) {
    result.grounded.push_back(!cross_section_ && geometry_.grounded(net));
  }
  result.windowed = !cross_section_ && geometry_.window().has_value();
  try {
    const auto start = std::chrono::steady_clock::now();
    result.row = cross_section_
                     ? solver_->solve(*section_scene_, master, options)
                     : solver_->solve(*scene_, master, options);
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
  const std::string_view unit = result.cross_section ? "fF/um" : "fF";
  logger_->info(utl::FSV,
                11,
                "Net {}: total {:.5g} {} +- {:.2g} after {} walks ({:.1f} "
                "hops per walk).",
                result.net,
                row.total(),
                unit,
                row.sigma[row.master],
                row.walks,
                static_cast<double>(row.hops) / row.walks);
  logger_->report("  {:<20} {:>12.5g} +- {:.2g} {}",
                  "ground",
                  -row.ground,
                  row.ground_sigma,
                  unit);
  for (int net = 0; net < static_cast<int>(row.cap.size()); net++) {
    if (net != row.master && !result.grounded[net]) {
      logger_->report("  {:<20} {:>12.5g} +- {:.2g} {}",
                      result.nets[net],
                      row.coupling(net),
                      row.sigma[net],
                      unit);
    }
  }
  if (result.windowed) {
    logger_->report("  window leakage {:.2g} {} ({:.2g}% of the total)",
                    row.window_leakage,
                    unit,
                    100 * row.window_leakage / row.total());
  }
}

}  // namespace fsv
