// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#pragma once

#include <cstdint>
#include <istream>
#include <memory>
#include <ostream>
#include <string>
#include <vector>

#include "SACoreHardMacro.h"
#include "SACoreSoftMacro.h"
#include "hier_rtlmp.h"
#include "object.h"
#include "odb/db.h"
#include "odb/geom.h"
#include "utl/Logger.h"

namespace mpl {

enum class PackingStage
{
  Shaping,
  ClusterPlacement,
  MacroPlacement
};

struct DiscreteShape
{
  int width = 0;
  int height = 0;

  bool operator==(const DiscreteShape& other) const = default;
};

struct WidthInterval
{
  int min_width = 0;
  int max_width = 0;

  bool operator==(const WidthInterval& other) const = default;
};

struct PackingBlock
{
  enum class Kind
  {
    Fixed,
    Discrete,
    SoftInterval
  };

  std::string name;
  Kind kind = Kind::Discrete;

  // Used when kind == Kind::Fixed.
  int fixed_x = 0;
  int fixed_y = 0;
  int fixed_width = 0;
  int fixed_height = 0;

  // Used when kind == Kind::Discrete (HardMacroCluster tilings or HardMacro
  // orientations).
  std::vector<DiscreteShape> shapes;

  // Used when kind == Kind::SoftInterval (StdCellCluster or MixedCluster).
  int64_t area = 0;
  std::vector<WidthInterval> width_intervals;
  ClusterType soft_cluster_type = StdCellCluster;

  bool operator==(const PackingBlock& other) const = default;
};

struct PackingProblem
{
  std::string name;
  PackingStage stage = PackingStage::ClusterPlacement;
  int outline_width = 0;
  int outline_height = 0;
  std::vector<PackingBlock> blocks;

  bool operator==(const PackingProblem& other) const = default;
};

struct PlacedBlock
{
  std::string name;
  int x = 0;
  int y = 0;
  int width = 0;
  int height = 0;

  odb::Rect getBBox() const { return odb::Rect(x, y, x + width, y + height); }
};

enum class PackingStatus
{
  Feasible,
  Infeasible,
  Unknown
};

struct PackingOracleResult
{
  PackingStatus status = PackingStatus::Unknown;
  std::vector<PlacedBlock> placement;
};

// Solves the 2D fixed-outline packing problem with OR-Tools CP-SAT.
// Models fixed blocks as fixed intervals, discrete blocks via AddExactlyOne
// shape literals, and continuous soft-macro intervals with floor-division
// conjugate height h = floor(A / w) via A - w < w * h <= A.
PackingOracleResult solveWithCpSat(const PackingProblem& problem,
                                   double time_limit_sec = 5.0);

// Verifies whether a candidate placement satisfies fixed-outline containment,
// pairwise non-overlap, and per-block shape / floor-division constraints.
bool verifyPlacement(const PackingProblem& problem,
                     const std::vector<PlacedBlock>& placement,
                     std::string* reason = nullptr);

// Returns the ticket-prescribed moves per temperature step:
// max(n, 50) for Shaping, max(n, 500) for ClusterPlacement / MacroPlacement.
int defaultMovesPerStep(PackingStage stage, int num_movable_blocks);

// Text serializer and deserializer for single problems and multi-problem
// corpora.
void serializePackingProblem(const PackingProblem& problem, std::ostream& os);
std::string serializePackingProblemToString(const PackingProblem& problem);
PackingProblem parsePackingProblem(std::istream& is);
PackingProblem parsePackingProblemFromString(const std::string& text);

void serializePackingProblemCorpus(const std::vector<PackingProblem>& problems,
                                   std::ostream& os);
std::vector<PackingProblem> parsePackingProblemCorpus(std::istream& is);
std::vector<PackingProblem> loadPackingProblemCorpus(const std::string& path);

// Owns the dummy PhysicalHierarchy, Clusters, and SoftMacros
// required to run SACoreSoftMacro without null-cluster no-ops or null-block
// crashes.
struct SoftPackingHarness
{
  PhysicalHierarchy tree;
  std::unique_ptr<SoftMacro> root_soft_macro;
  std::vector<std::unique_ptr<Cluster>> clusters;
  std::vector<SoftMacro> macros;
  std::unique_ptr<SACoreSoftMacro> annealer;
};

// Owns the PhysicalHierarchy and HardMacros required to run SACoreHardMacro.
struct HardPackingHarness
{
  PhysicalHierarchy tree;
  std::vector<HardMacro> macros;
  std::unique_ptr<SACoreHardMacro> annealer;
};

std::unique_ptr<SoftPackingHarness> buildSoftPackingHarness(
    const PackingProblem& problem,
    odb::dbBlock* block,
    utl::Logger* logger,
    int random_seed = 1);

std::unique_ptr<HardPackingHarness> buildHardPackingHarness(
    const PackingProblem& problem,
    odb::dbBlock* block,
    utl::Logger* logger,
    int random_seed = 1);

std::vector<PlacedBlock> extractPlacement(const SoftPackingHarness& harness);
std::vector<PlacedBlock> extractPlacement(const HardPackingHarness& harness,
                                          const PackingProblem& problem);

}  // namespace mpl
