// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include "PackingOracle.h"

#include <algorithm>
#include <cstdint>
#include <fstream>
#include <istream>
#include <memory>
#include <ostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "SACoreHardMacro.h"
#include "SACoreSoftMacro.h"
#include "hier_rtlmp.h"
#include "mpl-util.h"
#include "object.h"
#include "odb/db.h"
#include "odb/geom.h"
#include "ortools/sat/cp_model.h"
#include "ortools/sat/cp_model.pb.h"
#include "ortools/sat/cp_model_solver.h"
#include "ortools/sat/sat_parameters.pb.h"
#include "ortools/util/sorted_interval_list.h"
#include "shapes.h"
#include "utl/Logger.h"

namespace mpl {

namespace {

using operations_research::ClosedInterval;
using operations_research::Domain;
using operations_research::sat::BoolVar;
using operations_research::sat::CpModelBuilder;
using operations_research::sat::CpSolverResponse;
using operations_research::sat::CpSolverStatus;
using operations_research::sat::IntervalVar;
using operations_research::sat::IntVar;
using operations_research::sat::LinearExpr;
using operations_research::sat::NoOverlap2DConstraint;
using operations_research::sat::SatParameters;
using operations_research::sat::SolutionIntegerValue;
using operations_research::sat::SolveWithParameters;

struct BlockVars
{
  bool is_fixed = false;
  int fixed_x = 0;
  int fixed_y = 0;
  int fixed_w = 0;
  int fixed_h = 0;
  IntVar x;
  IntVar y;
  IntVar w;
  IntVar h;
};

void addBoundedAxisInterval(CpModelBuilder& cp_model,
                            NoOverlap2DConstraint& no_overlap,
                            int outline_width,
                            int outline_height,
                            int min_w,
                            int min_h,
                            IntVar w_var,
                            IntVar h_var,
                            BlockVars& out_vars)
{
  out_vars.x = cp_model.NewIntVar(Domain(0, outline_width - min_w));
  out_vars.y = cp_model.NewIntVar(Domain(0, outline_height - min_h));
  out_vars.w = w_var;
  out_vars.h = h_var;

  IntVar end_x = cp_model.NewIntVar(Domain(min_w, outline_width));
  IntVar end_y = cp_model.NewIntVar(Domain(min_h, outline_height));
  IntervalVar x_iv = cp_model.NewIntervalVar(out_vars.x, w_var, end_x);
  IntervalVar y_iv = cp_model.NewIntervalVar(out_vars.y, h_var, end_y);
  no_overlap.AddRectangle(x_iv, y_iv);
}

bool addFixedBlockToModel(const PackingBlock& block,
                          int outline_width,
                          int outline_height,
                          CpModelBuilder& cp_model,
                          NoOverlap2DConstraint& no_overlap,
                          BlockVars& out_vars)
{
  const bool valid_size = block.fixed_width >= 0 && block.fixed_height >= 0;
  const bool inside_x = block.fixed_x >= 0
                        && block.fixed_x + block.fixed_width <= outline_width;
  const bool inside_y = block.fixed_y >= 0
                        && block.fixed_y + block.fixed_height <= outline_height;
  if (!valid_size || !inside_x || !inside_y) {
    return false;
  }

  out_vars.is_fixed = true;
  out_vars.fixed_x = block.fixed_x;
  out_vars.fixed_y = block.fixed_y;
  out_vars.fixed_w = block.fixed_width;
  out_vars.fixed_h = block.fixed_height;

  if (block.fixed_width > 0 && block.fixed_height > 0) {
    IntervalVar x_iv
        = cp_model.NewFixedSizeIntervalVar(block.fixed_x, block.fixed_width);
    IntervalVar y_iv
        = cp_model.NewFixedSizeIntervalVar(block.fixed_y, block.fixed_height);
    no_overlap.AddRectangle(x_iv, y_iv);
  }
  return true;
}

std::vector<DiscreteShape> filterValidShapes(
    const std::vector<DiscreteShape>& shapes,
    int outline_width,
    int outline_height)
{
  std::vector<DiscreteShape> valid;
  for (const DiscreteShape& s : shapes) {
    if (s.width > 0 && s.height > 0 && s.width <= outline_width
        && s.height <= outline_height) {
      valid.push_back(s);
    }
  }
  return valid;
}

bool addDiscreteBlockToModel(const PackingBlock& block,
                             int outline_width,
                             int outline_height,
                             CpModelBuilder& cp_model,
                             NoOverlap2DConstraint& no_overlap,
                             BlockVars& out_vars)
{
  const std::vector<DiscreteShape> valid_shapes
      = filterValidShapes(block.shapes, outline_width, outline_height);
  if (valid_shapes.empty()) {
    return false;
  }

  std::vector<BoolVar> lits;
  std::vector<int64_t> widths;
  std::vector<int64_t> heights;
  int min_w = outline_width;
  int max_w = 0;
  int min_h = outline_height;
  int max_h = 0;

  for (const DiscreteShape& s : valid_shapes) {
    lits.push_back(cp_model.NewBoolVar());
    widths.push_back(s.width);
    heights.push_back(s.height);
    min_w = std::min(min_w, s.width);
    max_w = std::max(max_w, s.width);
    min_h = std::min(min_h, s.height);
    max_h = std::max(max_h, s.height);
  }

  cp_model.AddExactlyOne(lits);
  IntVar w_var = cp_model.NewIntVar(Domain(min_w, max_w));
  IntVar h_var = cp_model.NewIntVar(Domain(min_h, max_h));
  cp_model.AddEquality(w_var, LinearExpr::WeightedSum(lits, widths));
  cp_model.AddEquality(h_var, LinearExpr::WeightedSum(lits, heights));

  addBoundedAxisInterval(cp_model,
                         no_overlap,
                         outline_width,
                         outline_height,
                         min_w,
                         min_h,
                         w_var,
                         h_var,
                         out_vars);
  return true;
}

std::vector<WidthInterval> normalizeWidthIntervals(
    const std::vector<WidthInterval>& raw_intervals)
{
  std::vector<WidthInterval> sorted;
  for (const WidthInterval& iv : raw_intervals) {
    const int min_w = std::max(1, iv.min_width);
    if (iv.max_width >= min_w) {
      sorted.push_back({min_w, iv.max_width});
    }
  }
  std::ranges::sort(sorted, [](const WidthInterval& a, const WidthInterval& b) {
    return a.min_width < b.min_width;
  });

  std::vector<WidthInterval> merged;
  for (const WidthInterval& iv : sorted) {
    if (merged.empty() || iv.min_width > merged.back().max_width) {
      merged.push_back(iv);
    } else {
      merged.back().max_width = std::max(merged.back().max_width, iv.max_width);
    }
  }
  return merged;
}

int64_t intervalMinEffectiveArea(int64_t area, const WidthInterval& iv)
{
  const int64_t area_lo
      = static_cast<int64_t>(iv.min_width) * (area / iv.min_width);
  const int64_t area_hi
      = static_cast<int64_t>(iv.max_width) * (area / iv.max_width);
  return std::min({area, area_lo, area_hi});
}

std::pair<Domain, Domain> buildSoftDomains(
    int64_t area,
    const std::vector<WidthInterval>& intervals,
    int outline_width,
    int outline_height)
{
  const int64_t min_feasible_w
      = (area / (static_cast<int64_t>(outline_height) + 1)) + 1;
  std::vector<ClosedInterval> w_intervals;
  std::vector<ClosedInterval> h_intervals;

  for (const WidthInterval& iv : intervals) {
    const int64_t lo = std::max<int64_t>({1, iv.min_width, min_feasible_w});
    const int64_t hi = std::min<int64_t>({iv.max_width, outline_width, area});
    if (lo <= hi) {
      const int64_t h_lo = std::max<int64_t>(1, area / hi);
      const int64_t h_hi = std::min<int64_t>(outline_height, area / lo);
      if (h_lo <= h_hi) {
        w_intervals.emplace_back(lo, hi);
        h_intervals.emplace_back(h_lo, h_hi);
      }
    }
  }
  return {Domain::FromIntervals(w_intervals),
          Domain::FromIntervals(h_intervals)};
}

bool addSoftIntervalBlockToModel(const PackingBlock& block,
                                 int outline_width,
                                 int outline_height,
                                 CpModelBuilder& cp_model,
                                 NoOverlap2DConstraint& no_overlap,
                                 BlockVars& out_vars)
{
  const std::vector<WidthInterval> intervals
      = normalizeWidthIntervals(block.width_intervals);
  if (block.area <= 0 || intervals.empty()) {
    return false;
  }

  auto [w_domain, h_domain]
      = buildSoftDomains(block.area, intervals, outline_width, outline_height);
  if (w_domain.IsEmpty() || h_domain.IsEmpty()) {
    return false;
  }

  IntVar w_var = cp_model.NewIntVar(w_domain);
  IntVar h_var = cp_model.NewIntVar(h_domain);
  const int64_t prod_min
      = std::max<int64_t>(0, block.area - w_domain.Max() + 1);
  IntVar prod_var = cp_model.NewIntVar(Domain(prod_min, block.area));
  cp_model.AddMultiplicationEquality(prod_var, {w_var, h_var});
  cp_model.AddLessOrEqual(prod_var, block.area);
  cp_model.AddGreaterOrEqual(LinearExpr(prod_var) - block.area + w_var, 1);

  addBoundedAxisInterval(cp_model,
                         no_overlap,
                         outline_width,
                         outline_height,
                         static_cast<int>(w_domain.Min()),
                         static_cast<int>(h_domain.Min()),
                         w_var,
                         h_var,
                         out_vars);
  return true;
}

bool addBlockToModel(const PackingBlock& block,
                     int outline_width,
                     int outline_height,
                     CpModelBuilder& cp_model,
                     NoOverlap2DConstraint& no_overlap,
                     BlockVars& out_vars)
{
  switch (block.kind) {
    case PackingBlock::Kind::Fixed:
      return addFixedBlockToModel(
          block, outline_width, outline_height, cp_model, no_overlap, out_vars);
    case PackingBlock::Kind::Discrete:
      return addDiscreteBlockToModel(
          block, outline_width, outline_height, cp_model, no_overlap, out_vars);
    case PackingBlock::Kind::SoftInterval:
      return addSoftIntervalBlockToModel(
          block, outline_width, outline_height, cp_model, no_overlap, out_vars);
  }
  return false;
}

std::vector<PlacedBlock> extractSolution(const PackingProblem& problem,
                                         const std::vector<BlockVars>& vars,
                                         const CpSolverResponse& response)
{
  std::vector<PlacedBlock> result;
  result.reserve(problem.blocks.size());
  for (size_t i = 0; i < problem.blocks.size(); ++i) {
    PlacedBlock placed;
    placed.name = problem.blocks[i].name;
    if (vars[i].is_fixed) {
      placed.x = vars[i].fixed_x;
      placed.y = vars[i].fixed_y;
      placed.width = vars[i].fixed_w;
      placed.height = vars[i].fixed_h;
    } else {
      placed.x = static_cast<int>(SolutionIntegerValue(response, vars[i].x));
      placed.y = static_cast<int>(SolutionIntegerValue(response, vars[i].y));
      placed.width
          = static_cast<int>(SolutionIntegerValue(response, vars[i].w));
      placed.height
          = static_cast<int>(SolutionIntegerValue(response, vars[i].h));
    }
    result.push_back(std::move(placed));
  }
  return result;
}

bool setFailureReason(std::string* reason, const std::string& msg)
{
  if (reason != nullptr) {
    *reason = msg;
  }
  return false;
}

bool verifyFixedBlock(const PackingBlock& spec,
                      const PlacedBlock& placed,
                      std::string* reason)
{
  const bool matches = placed.x == spec.fixed_x && placed.y == spec.fixed_y
                       && placed.width == spec.fixed_width
                       && placed.height == spec.fixed_height;
  if (!matches) {
    return setFailureReason(
        reason,
        "Fixed block " + spec.name + " does not match fixed coordinates/size");
  }
  return true;
}

bool verifyDiscreteBlock(const PackingBlock& spec,
                         const PlacedBlock& placed,
                         std::string* reason)
{
  for (const DiscreteShape& s : spec.shapes) {
    if (placed.width == s.width && placed.height == s.height) {
      return true;
    }
  }
  return setFailureReason(
      reason, "Discrete block " + spec.name + " uses an unlisted shape");
}

bool verifySoftIntervalBlock(const PackingBlock& spec,
                             const PlacedBlock& placed,
                             std::string* reason)
{
  const std::vector<WidthInterval> intervals
      = normalizeWidthIntervals(spec.width_intervals);
  if (placed.width <= 0 || spec.area <= 0 || intervals.empty()) {
    return setFailureReason(
        reason, "Soft block " + spec.name + " has non-positive width or area");
  }
  for (const WidthInterval& iv : intervals) {
    if (placed.width >= iv.min_width && placed.width <= iv.max_width) {
      const int min_h = static_cast<int>(intervalMinEffectiveArea(spec.area, iv)
                                         / placed.width);
      const int max_h = static_cast<int>(spec.area / placed.width);
      if (placed.height >= min_h && placed.height <= max_h) {
        return true;
      }
      return setFailureReason(
          reason,
          "Soft block " + spec.name
              + " height does not match floor-divided area");
    }
  }
  return setFailureReason(
      reason,
      "Soft block " + spec.name + " width lies outside allowed intervals");
}

bool verifySingleBlock(const PackingBlock& spec,
                       const PlacedBlock& placed,
                       int outline_width,
                       int outline_height,
                       std::string* reason)
{
  const bool in_bounds = placed.x >= 0 && placed.y >= 0 && placed.width >= 0
                         && placed.height >= 0
                         && placed.x + placed.width <= outline_width
                         && placed.y + placed.height <= outline_height;
  if (!in_bounds) {
    return setFailureReason(
        reason, "Block " + spec.name + " violates outline boundaries");
  }

  switch (spec.kind) {
    case PackingBlock::Kind::Fixed:
      return verifyFixedBlock(spec, placed, reason);
    case PackingBlock::Kind::Discrete:
      return verifyDiscreteBlock(spec, placed, reason);
    case PackingBlock::Kind::SoftInterval:
      return verifySoftIntervalBlock(spec, placed, reason);
  }
  return false;
}

bool rectanglesOverlap(const PlacedBlock& a, const PlacedBlock& b)
{
  const bool separated_x = (a.x + a.width <= b.x) || (b.x + b.width <= a.x);
  const bool separated_y = (a.y + a.height <= b.y) || (b.y + b.height <= a.y);
  return !separated_x && !separated_y;
}

std::string stageToString(PackingStage stage)
{
  switch (stage) {
    case PackingStage::Shaping:
      return "SHAPING";
    case PackingStage::ClusterPlacement:
      return "CLUSTER_PLACEMENT";
    case PackingStage::MacroPlacement:
      return "MACRO_PLACEMENT";
  }
  return "CLUSTER_PLACEMENT";
}

PackingStage stringToStage(const std::string& token)
{
  if (token == "SHAPING") {
    return PackingStage::Shaping;
  }
  if (token == "CLUSTER_PLACEMENT") {
    return PackingStage::ClusterPlacement;
  }
  if (token == "MACRO_PLACEMENT") {
    return PackingStage::MacroPlacement;
  }
  throw std::runtime_error("Unknown STAGE token: " + token);
}

std::string clusterTypeToString(ClusterType type)
{
  return (type == MixedCluster) ? "MIXED" : "STD_CELL";
}

ClusterType stringToClusterType(const std::string& token)
{
  if (token == "MIXED") {
    return MixedCluster;
  }
  if (token == "STD_CELL") {
    return StdCellCluster;
  }
  throw std::runtime_error("Unknown soft cluster type token: " + token);
}

void serializeBlock(const PackingBlock& block, std::ostream& os)
{
  if (block.kind == PackingBlock::Kind::Fixed) {
    os << "FIXED " << block.name << ' ' << block.fixed_x << ' ' << block.fixed_y
       << ' ' << block.fixed_width << ' ' << block.fixed_height << '\n';
    return;
  }
  if (block.kind == PackingBlock::Kind::Discrete) {
    os << "DISCRETE " << block.name << ' ' << block.shapes.size();
    for (const DiscreteShape& s : block.shapes) {
      os << ' ' << s.width << ' ' << s.height;
    }
    os << '\n';
    return;
  }
  os << "SOFT " << block.name << ' '
     << clusterTypeToString(block.soft_cluster_type) << ' ' << block.area << ' '
     << block.width_intervals.size();
  for (const WidthInterval& iv : block.width_intervals) {
    os << ' ' << iv.min_width << ' ' << iv.max_width;
  }
  os << '\n';
}

PackingBlock parseFixedBlock(std::istream& ls)
{
  PackingBlock block;
  block.kind = PackingBlock::Kind::Fixed;
  ls >> block.name >> block.fixed_x >> block.fixed_y >> block.fixed_width
      >> block.fixed_height;
  return block;
}

PackingBlock parseDiscreteBlock(std::istream& ls)
{
  PackingBlock block;
  block.kind = PackingBlock::Kind::Discrete;
  size_t count = 0;
  ls >> block.name >> count;
  block.shapes.resize(count);
  for (size_t i = 0; i < count; ++i) {
    ls >> block.shapes[i].width >> block.shapes[i].height;
  }
  return block;
}

PackingBlock parseSoftBlock(std::istream& ls)
{
  PackingBlock block;
  block.kind = PackingBlock::Kind::SoftInterval;
  std::string type_token;
  size_t count = 0;
  ls >> block.name >> type_token >> block.area >> count;
  block.soft_cluster_type = stringToClusterType(type_token);
  block.width_intervals.resize(count);
  for (size_t i = 0; i < count; ++i) {
    ls >> block.width_intervals[i].min_width
        >> block.width_intervals[i].max_width;
  }
  return block;
}

bool applyProblemLine(const std::string& keyword,
                      std::istream& ls,
                      PackingProblem& problem)
{
  if (keyword == "PROBLEM") {
    ls >> problem.name;
    return true;
  }
  if (keyword == "STAGE") {
    std::string stage_tok;
    ls >> stage_tok;
    problem.stage = stringToStage(stage_tok);
    return true;
  }
  if (keyword == "OUTLINE") {
    ls >> problem.outline_width >> problem.outline_height;
    return true;
  }
  if (keyword == "FIXED") {
    problem.blocks.push_back(parseFixedBlock(ls));
    return true;
  }
  if (keyword == "DISCRETE") {
    problem.blocks.push_back(parseDiscreteBlock(ls));
    return true;
  }
  if (keyword == "SOFT") {
    problem.blocks.push_back(parseSoftBlock(ls));
    return true;
  }
  if (keyword == "END_PROBLEM") {
    return false;
  }
  throw std::runtime_error("Unexpected keyword in packing problem: " + keyword);
}

void initSoftHarnessRoot(SoftPackingHarness& harness,
                         const PackingProblem& problem,
                         utl::Logger* logger)
{
  const odb::Rect die_rect(0, 0, problem.outline_width, problem.outline_height);
  harness.tree.die_area = die_rect;
  harness.tree.floorplan_shape = die_rect;
  harness.tree.root = std::make_unique<Cluster>(0, "root", logger);

  const IntervalList root_iv = {{problem.outline_width, problem.outline_width}};
  const int64_t root_area = static_cast<int64_t>(problem.outline_width)
                            * static_cast<int64_t>(problem.outline_height);
  harness.root_soft_macro
      = std::make_unique<SoftMacro>(harness.tree.root.get());
  harness.root_soft_macro->setShapes(root_iv, root_area);
  harness.root_soft_macro->setWidth(problem.outline_width);
  harness.root_soft_macro->setX(0);
  harness.root_soft_macro->setY(0);
  harness.tree.root->setSoftMacro(
      std::make_unique<SoftMacro>(*harness.root_soft_macro));
}

SoftMacro buildFixedSoftMacro(const PackingBlock& block)
{
  const odb::Rect rect(block.fixed_x,
                       block.fixed_y,
                       block.fixed_x + block.fixed_width,
                       block.fixed_y + block.fixed_height);
  return SoftMacro(rect, block.name);
}

SoftMacro buildDiscreteSoftMacro(const PackingBlock& block,
                                 int cluster_id,
                                 utl::Logger* logger,
                                 SoftPackingHarness& harness)
{
  auto cluster = std::make_unique<Cluster>(cluster_id, block.name, logger);
  cluster->setClusterType(HardMacroCluster);

  TilingList tilings;
  int64_t max_area = 0;
  for (const DiscreteShape& s : block.shapes) {
    tilings.emplace_back(s.width, s.height);
    const int64_t shape_area
        = static_cast<int64_t>(s.width) * static_cast<int64_t>(s.height);
    max_area = std::max(max_area, shape_area);
  }
  cluster->setMetrics(
      Metrics(0, static_cast<unsigned int>(tilings.size()), 0, max_area));

  SoftMacro macro(cluster.get());
  macro.setShapes(tilings, /*force=*/true);
  harness.clusters.push_back(std::move(cluster));
  return macro;
}

SoftMacro buildIntervalSoftMacro(const PackingBlock& block,
                                 int cluster_id,
                                 utl::Logger* logger,
                                 SoftPackingHarness& harness)
{
  auto cluster = std::make_unique<Cluster>(cluster_id, block.name, logger);
  const ClusterType ctype = (block.soft_cluster_type == MixedCluster)
                                ? MixedCluster
                                : StdCellCluster;
  cluster->setClusterType(ctype);
  cluster->setMetrics(Metrics(1, 0, block.area, 0));

  IntervalList intervals;
  for (const WidthInterval& iv :
       normalizeWidthIntervals(block.width_intervals)) {
    intervals.emplace_back(iv.min_width, iv.max_width);
  }

  SoftMacro macro(cluster.get());
  macro.setShapes(intervals, block.area);
  harness.clusters.push_back(std::move(cluster));
  return macro;
}

int countMovableBlocks(const PackingProblem& problem)
{
  int count = 0;
  for (const PackingBlock& block : problem.blocks) {
    if (block.kind != PackingBlock::Kind::Fixed) {
      ++count;
    }
  }
  return count;
}

}  // namespace

PackingOracleResult solveWithCpSat(const PackingProblem& problem,
                                   double time_limit_sec)
{
  PackingOracleResult result;
  if (problem.outline_width <= 0 || problem.outline_height <= 0) {
    result.status = problem.blocks.empty() ? PackingStatus::Feasible
                                           : PackingStatus::Infeasible;
    return result;
  }

  CpModelBuilder cp_model;
  NoOverlap2DConstraint no_overlap = cp_model.AddNoOverlap2D();
  std::vector<BlockVars> vars(problem.blocks.size());

  for (size_t i = 0; i < problem.blocks.size(); ++i) {
    if (!addBlockToModel(problem.blocks[i],
                         problem.outline_width,
                         problem.outline_height,
                         cp_model,
                         no_overlap,
                         vars[i])) {
      result.status = PackingStatus::Infeasible;
      return result;
    }
  }

  SatParameters params;
  params.set_max_time_in_seconds(time_limit_sec);
  params.set_num_search_workers(1);

  const CpSolverResponse response
      = SolveWithParameters(cp_model.Build(), params);
  if (response.status() == CpSolverStatus::OPTIMAL
      || response.status() == CpSolverStatus::FEASIBLE) {
    result.status = PackingStatus::Feasible;
    result.placement = extractSolution(problem, vars, response);
  } else if (response.status() == CpSolverStatus::INFEASIBLE) {
    result.status = PackingStatus::Infeasible;
  } else {
    result.status = PackingStatus::Unknown;
  }
  return result;
}

bool verifyPlacement(const PackingProblem& problem,
                     const std::vector<PlacedBlock>& placement,
                     std::string* reason)
{
  if (placement.size() != problem.blocks.size()) {
    return setFailureReason(reason, "Placement block count mismatch");
  }

  for (size_t i = 0; i < problem.blocks.size(); ++i) {
    if (!verifySingleBlock(problem.blocks[i],
                           placement[i],
                           problem.outline_width,
                           problem.outline_height,
                           reason)) {
      return false;
    }
  }

  for (size_t i = 0; i < placement.size(); ++i) {
    for (size_t j = i + 1; j < placement.size(); ++j) {
      if (rectanglesOverlap(placement[i], placement[j])) {
        return setFailureReason(reason,
                                "Blocks " + placement[i].name + " and "
                                    + placement[j].name + " overlap");
      }
    }
  }
  return true;
}

int defaultMovesPerStep(PackingStage stage, int num_movable_blocks)
{
  const int floor_moves = (stage == PackingStage::Shaping) ? 50 : 500;
  return std::max(num_movable_blocks, floor_moves);
}

void serializePackingProblem(const PackingProblem& problem, std::ostream& os)
{
  os << "PROBLEM " << problem.name << '\n';
  os << "STAGE " << stageToString(problem.stage) << '\n';
  os << "OUTLINE " << problem.outline_width << ' ' << problem.outline_height
     << '\n';
  for (const PackingBlock& block : problem.blocks) {
    serializeBlock(block, os);
  }
  os << "END_PROBLEM\n";
}

std::string serializePackingProblemToString(const PackingProblem& problem)
{
  std::ostringstream oss;
  serializePackingProblem(problem, oss);
  return oss.str();
}

PackingProblem parsePackingProblem(std::istream& is)
{
  std::vector<PackingProblem> corpus = parsePackingProblemCorpus(is);
  if (corpus.empty()) {
    throw std::runtime_error("No PROBLEM found in input stream");
  }
  return std::move(corpus.front());
}

PackingProblem parsePackingProblemFromString(const std::string& text)
{
  std::istringstream iss(text);
  return parsePackingProblem(iss);
}

void serializePackingProblemCorpus(const std::vector<PackingProblem>& problems,
                                   std::ostream& os)
{
  for (size_t i = 0; i < problems.size(); ++i) {
    if (i > 0) {
      os << '\n';
    }
    serializePackingProblem(problems[i], os);
  }
}

std::vector<PackingProblem> parsePackingProblemCorpus(std::istream& is)
{
  std::vector<PackingProblem> problems;
  PackingProblem current;
  bool in_problem = false;
  std::string line;

  while (std::getline(is, line)) {
    std::istringstream ls(line);
    std::string keyword;
    if ((ls >> keyword) && keyword[0] != '#') {
      if (keyword == "PROBLEM") {
        current = PackingProblem{};
        in_problem = true;
      }
      if (!in_problem) {
        throw std::runtime_error("Token outside PROBLEM block: " + keyword);
      }
      if (!applyProblemLine(keyword, ls, current)) {
        problems.push_back(std::move(current));
        in_problem = false;
      }
    }
  }
  if (in_problem) {
    throw std::runtime_error("Unterminated PROBLEM block: " + current.name);
  }
  return problems;
}

std::vector<PackingProblem> loadPackingProblemCorpus(const std::string& path)
{
  std::ifstream ifs(path);
  if (!ifs.is_open()) {
    throw std::runtime_error("Failed to open packing corpus file: " + path);
  }
  return parsePackingProblemCorpus(ifs);
}

std::unique_ptr<SoftPackingHarness> buildSoftPackingHarness(
    const PackingProblem& problem,
    odb::dbBlock* block,
    utl::Logger* logger,
    int random_seed)
{
  if (block != nullptr && block->getDb()->getDbuPerMicron() == 0) {
    block->getDb()->setDbuPerMicron(1000);
  }
  auto harness = std::make_unique<SoftPackingHarness>();
  initSoftHarnessRoot(*harness, problem, logger);

  for (size_t i = 0; i < problem.blocks.size(); ++i) {
    const PackingBlock& spec = problem.blocks[i];
    const int cid = static_cast<int>(i) + 1;
    if (spec.kind == PackingBlock::Kind::Fixed) {
      harness->macros.push_back(buildFixedSoftMacro(spec));
    } else if (spec.kind == PackingBlock::Kind::Discrete) {
      harness->macros.push_back(
          buildDiscreteSoftMacro(spec, cid, logger, *harness));
    } else {
      harness->macros.push_back(
          buildIntervalSoftMacro(spec, cid, logger, *harness));
    }
  }

  SACoreWeights core_weights;
  core_weights.area = 1.0f;
  core_weights.outline = 1000.0f;
  SASoftWeights soft_weights;
  soft_weights.soft_blockage = 1000.0f;

  const int moves_per_step
      = defaultMovesPerStep(problem.stage, countMovableBlocks(problem));
  const odb::Rect outline(0, 0, problem.outline_width, problem.outline_height);

  harness->annealer
      = std::make_unique<SACoreSoftMacro>(&harness->tree,
                                          outline,
                                          harness->macros,
                                          core_weights,
                                          soft_weights,
                                          0.0f,
                                          0.0f,
                                          0.2f,
                                          0.2f,
                                          0.1f,
                                          0.2f,
                                          0.3f,
                                          0.9f,
                                          2000,
                                          moves_per_step,
                                          static_cast<unsigned>(random_seed),
                                          nullptr,
                                          logger,
                                          block);
  return harness;
}

std::unique_ptr<HardPackingHarness> buildHardPackingHarness(
    const PackingProblem& problem,
    odb::dbBlock* block,
    utl::Logger* logger,
    int random_seed)
{
  if (block != nullptr && block->getDb()->getDbuPerMicron() == 0) {
    block->getDb()->setDbuPerMicron(1000);
  }
  auto harness = std::make_unique<HardPackingHarness>();
  const odb::Rect outline(0, 0, problem.outline_width, problem.outline_height);
  harness->tree.die_area = outline;
  harness->tree.floorplan_shape = outline;

  size_t shape_combo = static_cast<size_t>(std::max(0, random_seed - 1));
  for (const PackingBlock& spec : problem.blocks) {
    if (spec.kind != PackingBlock::Kind::Discrete || spec.shapes.empty()) {
      throw std::runtime_error(
          "SACoreHardMacro requires Discrete blocks with at least one shape");
    }
    std::vector<DiscreteShape> candidates = filterValidShapes(
        spec.shapes, problem.outline_width, problem.outline_height);
    if (candidates.empty()) {
      candidates = spec.shapes;
    }
    const DiscreteShape& chosen = candidates[shape_combo % candidates.size()];
    shape_combo /= candidates.size();
    harness->macros.emplace_back(chosen.width, chosen.height, spec.name);
  }

  SACoreWeights core_weights;
  core_weights.area = 1.0f;
  core_weights.outline = 1000.0f;

  const int num_movable = static_cast<int>(harness->macros.size());
  const int moves_per_step = defaultMovesPerStep(problem.stage, num_movable);

  harness->annealer
      = std::make_unique<SACoreHardMacro>(&harness->tree,
                                          outline,
                                          harness->macros,
                                          core_weights,
                                          0.3f,
                                          0.3f,
                                          0.2f,
                                          0.2f,
                                          0.9f,
                                          2000,
                                          moves_per_step,
                                          static_cast<unsigned>(random_seed),
                                          nullptr,
                                          logger,
                                          block);
  harness->annealer->setNumberOfSequencePairMacros(num_movable);
  return harness;
}

std::vector<PlacedBlock> extractPlacement(const SoftPackingHarness& harness)
{
  const std::vector<SoftMacro> macros = harness.annealer->getMacros();
  std::vector<PlacedBlock> placed;
  placed.reserve(macros.size());
  for (const SoftMacro& m : macros) {
    placed.push_back(
        {m.getName(), m.getX(), m.getY(), m.getWidth(), m.getHeight()});
  }
  return placed;
}

std::vector<PlacedBlock> extractPlacement(const HardPackingHarness& harness,
                                          const PackingProblem& problem)
{
  const std::vector<HardMacro> macros = harness.annealer->getMacros();
  std::vector<PlacedBlock> placed;
  placed.reserve(macros.size());
  for (size_t i = 0; i < macros.size(); ++i) {
    const std::string name
        = (i < problem.blocks.size()) ? problem.blocks[i].name : "";
    placed.push_back({name,
                      macros[i].getX(),
                      macros[i].getY(),
                      macros[i].getWidth(),
                      macros[i].getHeight()});
  }
  return placed;
}

}  // namespace mpl
