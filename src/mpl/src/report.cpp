// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021-2026, The OpenROAD Authors

// Post-placement macro-placement QoR report.
//
// This is a purely additive, read-only analysis command. It inspects the
// final ODB state after macro placement (or any flow that has placed macros)
// and reports quality-of-result metrics. It NEVER modifies the database, so
// the macro placement result is unchanged by calling it.

#include <algorithm>
#include <cstdint>
#include <limits>
#include <set>
#include <vector>

#include "mpl/rtl_mp.h"
#include "odb/db.h"
#include "odb/geom.h"
#include "odb/util.h"
#include "utl/Logger.h"

namespace mpl {

using utl::MPL;

namespace {

// Collect the placed block macros (hard macros) in the block.
std::vector<odb::dbInst*> getPlacedMacros(odb::dbBlock* block)
{
  std::vector<odb::dbInst*> macros;
  for (odb::dbInst* inst : block->getInsts()) {
    if (inst->isBlock() && inst->isPlaced()) {
      macros.push_back(inst);
    }
  }
  return macros;
}

// Minimum non-overlapping gap between two rectangles. Returns 0 if they
// touch or overlap (including overlap on one axis but separated on the
// other -> the separating-axis distance is used).
int64_t rectGap(const odb::Rect& a, const odb::Rect& b)
{
  int64_t dx = 0;
  if (a.xMax() < b.xMin()) {
    dx = b.xMin() - a.xMax();
  } else if (b.xMax() < a.xMin()) {
    dx = a.xMin() - b.xMax();
  }

  int64_t dy = 0;
  if (a.yMax() < b.yMin()) {
    dy = b.yMin() - a.yMax();
  } else if (b.yMax() < a.yMin()) {
    dy = a.yMin() - b.yMax();
  }

  // If separated on both axes the closest approach is the corner distance;
  // we report the Manhattan-style min of the axis gaps, which is the channel
  // width seen along the dominant separation. Use max() of the two axis gaps
  // would over-report channels; min() best reflects the routing channel.
  if (dx > 0 && dy > 0) {
    return std::min(dx, dy);
  }
  return std::max(dx, dy);
}

}  // namespace

void MacroPlacer::reportMacroPlacement()
{
  odb::dbBlock* block = db_->getChip()->getBlock();
  const std::vector<odb::dbInst*> macros = getPlacedMacros(block);

  logger_->report("Macro Placement Report");
  logger_->report("======================");

  if (macros.empty()) {
    logger_->info(MPL, 80, "No placed macros found.");
    return;
  }

  const odb::Rect core = block->getCoreArea();

  // --- Areas and bounding envelope of the macros. ---
  int64_t total_macro_area_dbu2 = 0;
  odb::Rect envelope;
  envelope.mergeInit();
  for (odb::dbInst* macro : macros) {
    const odb::Rect bbox = macro->getBBox()->getBox();
    total_macro_area_dbu2 += bbox.area();
    envelope.merge(bbox);
  }

  const double dbu = block->getDbUnitsPerMicron();
  const double dbu2 = dbu * dbu;
  const double total_macro_area = total_macro_area_dbu2 / dbu2;
  const double envelope_area = static_cast<double>(envelope.area()) / dbu2;
  const double packing_efficiency
      = envelope_area > 0.0 ? total_macro_area / envelope_area : 0.0;
  const double dead_space = envelope_area - total_macro_area;

  // --- HPWL: total design and macro-net contribution. ---
  odb::WireLengthEvaluator wl_eval(block);
  const double total_hpwl = block->dbuToMicrons(wl_eval.hpwl());

  int64_t macro_net_hpwl_dbu = 0;
  int macro_net_count = 0;
  for (odb::dbNet* net : block->getNets()) {
    if (net->getSigType().isSupply()) {
      continue;
    }
    bool touches_macro = false;
    for (odb::dbITerm* iterm : net->getITerms()) {
      odb::dbInst* inst = iterm->getInst();
      if (inst != nullptr && inst->isBlock()) {
        touches_macro = true;
        break;
      }
    }
    if (!touches_macro) {
      continue;
    }
    int64_t hpwl_x = 0;
    int64_t hpwl_y = 0;
    macro_net_hpwl_dbu += wl_eval.hpwl(net, hpwl_x, hpwl_y);
    ++macro_net_count;
  }
  const double macro_net_hpwl = block->dbuToMicrons(macro_net_hpwl_dbu);

  // --- Boundary spacing: distance from each macro to the closest core edge.
  int64_t min_boundary_dbu = std::numeric_limits<int64_t>::max();
  int64_t sum_boundary_dbu = 0;
  for (odb::dbInst* macro : macros) {
    const odb::Rect bbox = macro->getBBox()->getBox();
    const int64_t left = bbox.xMin() - core.xMin();
    const int64_t bottom = bbox.yMin() - core.yMin();
    const int64_t right = core.xMax() - bbox.xMax();
    const int64_t top = core.yMax() - bbox.yMax();
    const int64_t closest
        = std::min(std::min(left, right), std::min(bottom, top));
    min_boundary_dbu = std::min(min_boundary_dbu, closest);
    sum_boundary_dbu += closest;
  }
  const double min_boundary_spacing = block->dbuToMicrons(min_boundary_dbu);
  const double avg_boundary_spacing
      = block->dbuToMicrons(sum_boundary_dbu) / static_cast<double>(macros.size());

  // --- Macro-to-macro minimum spacing (channel width). ---
  int64_t min_macro_gap_dbu = std::numeric_limits<int64_t>::max();
  int overlap_pairs = 0;
  for (size_t i = 0; i < macros.size(); ++i) {
    const odb::Rect a = macros[i]->getBBox()->getBox();
    for (size_t j = i + 1; j < macros.size(); ++j) {
      const odb::Rect b = macros[j]->getBBox()->getBox();
      if (a.overlaps(b)) {
        ++overlap_pairs;
        min_macro_gap_dbu = 0;
        continue;
      }
      min_macro_gap_dbu = std::min(min_macro_gap_dbu, rectGap(a, b));
    }
  }

  // --- Emit the summary. ---
  logger_->report("  Placed macros            : {}", macros.size());
  logger_->report("  Total macro area         : {:.3f} um^2", total_macro_area);
  logger_->report("  Macro bounding-box area  : {:.3f} um^2", envelope_area);
  logger_->report("  Dead space in bbox       : {:.3f} um^2", dead_space);
  logger_->report("  Packing efficiency       : {:.1f} %",
                  packing_efficiency * 100.0);
  logger_->report("  Total design HPWL        : {:.3f} um", total_hpwl);
  logger_->report("  Macro-net HPWL           : {:.3f} um ({} nets)",
                  macro_net_hpwl,
                  macro_net_count);
  logger_->report("  Min boundary spacing     : {:.3f} um",
                  min_boundary_spacing);
  logger_->report("  Avg boundary spacing     : {:.3f} um",
                  avg_boundary_spacing);
  if (macros.size() > 1) {
    if (overlap_pairs > 0) {
      logger_->report(
          "  Min macro-to-macro gap   : 0.000 um ({} overlapping "
          "pair(s))",
          overlap_pairs);
    } else {
      logger_->report("  Min macro-to-macro gap   : {:.3f} um",
                      block->dbuToMicrons(min_macro_gap_dbu));
    }
  }

  // --- Metrics for downstream tooling / regression checks. ---
  logger_->metric("macro_place__num_macros", static_cast<int>(macros.size()));
  logger_->metric("macro_place__macro_area_um2", total_macro_area);
  logger_->metric("macro_place__packing_efficiency", packing_efficiency);
  logger_->metric("macro_place__macro_net_hpwl_um", macro_net_hpwl);
  logger_->metric("macro_place__min_boundary_spacing_um", min_boundary_spacing);

  // --- Per-macro detail table. ---
  logger_->report("");
  logger_->report("  {:<28} {:>10} {:>10} {:>10} {:>10} {:>6} {:>12}",
                  "Macro",
                  "llx(um)",
                  "lly(um)",
                  "urx(um)",
                  "ury(um)",
                  "orient",
                  "area(um^2)");
  std::vector<odb::dbInst*> sorted_macros = macros;
  std::sort(sorted_macros.begin(),
            sorted_macros.end(),
            [](odb::dbInst* a, odb::dbInst* b) {
              return a->getName() < b->getName();
            });
  for (odb::dbInst* macro : sorted_macros) {
    const odb::Rect bbox = macro->getBBox()->getBox();
    const double area = static_cast<double>(bbox.area()) / dbu2;
    logger_->report(
        "  {:<28} {:>10.3f} {:>10.3f} {:>10.3f} {:>10.3f} {:>6} "
        "{:>12.3f}",
        macro->getName(),
        block->dbuToMicrons(bbox.xMin()),
        block->dbuToMicrons(bbox.yMin()),
        block->dbuToMicrons(bbox.xMax()),
        block->dbuToMicrons(bbox.yMax()),
        macro->getOrient().getString(),
        area);
  }
}

}  // namespace mpl
