// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026-2026, The OpenROAD Authors

#include "ProgressReporter.hh"

#include <algorithm>
#include <cmath>
#include <limits>
#include <string>

#include "MoveCommitter.hh"
#include "RepairSetupContext.hh"
#include "RepairSetupProfiler.hh"
#include "RepairTargetCollector.hh"
#include "db_sta/dbNetwork.hh"
#include "db_sta/dbSta.hh"
#include "rsz/Resizer.hh"
#include "sta/Delay.hh"
#include "utl/Logger.h"
#include "utl/mem_stats.h"

namespace rsz {

namespace {

constexpr const char* kRule
    = "---------------------------------------------------------------"
      "---------------------------------------------------------------";

}  // namespace

ProgressReporter::ProgressReporter(Resizer& resizer,
                                   MoveCommitter& committer,
                                   RepairSetupContext& context)
    : resizer_(resizer), committer_(committer), context_(context)
{
}

void ProgressReporter::printHeader(const bool show_startpoint_metrics) const
{
  // One header per repair_timing call, reprinted only when a phase changes
  // the column layout.
  if (context_.progress_header_printed
      && context_.progress_header_show_startpoint_metrics
             == show_startpoint_metrics) {
    return;
  }

  const ProfileScope profile("reporting");
  context_.progress_header_printed = true;
  context_.progress_header_show_startpoint_metrics = show_startpoint_metrics;
  utl::Logger* logger = resizer_.logger();
  if (show_startpoint_metrics) {
    logger->report(
        "   Iter   | Removed | Resized | Inserted | Cloned |  Pin  |"
        "   Area   |    WNS   |   StTNS    |    TNS     |  Viol  |  Worst");
    logger->report(
        "          | Buffers |  Gates  | Buffers  |  Gates | Swaps |"
        "          |          |            |            | Endpts | St/EnPt");
  } else {
    logger->report(
        "   Iter   | Removed | Resized | Inserted | Cloned |  Pin  |"
        "   Area   |    WNS   |    TNS     |  Viol  |  Worst");
    logger->report(
        "          | Buffers |  Gates  | Buffers  |  Gates | Swaps |"
        "          |          |            | Endpts | Endpoint");
  }

  logger->report(kRule);
}

void ProgressReporter::printRow(const std::string_view label,
                                RepairTargetCollector& collector,
                                const char* profile_point,
                                const bool show_startpoint_metrics,
                                const bool worst_by_startpoint) const
{
  const ProfileScope profile("reporting");
  if (show_startpoint_metrics) {
    collector.collectViolatingStartpoints();
  }

  sta::Slack wns;
  sta::Slack en_tns;
  const sta::Pin* worst_pin;

  {
    const ProfileScope collection("qor_collection");
    wns = collector.getWns();
    en_tns = collector.getTns(false);
    if (profile_point != nullptr) {
      RepairSetupProfiler::qor(profile_point, wns, en_tns);
    }

    worst_pin = collector.getWorstPin(worst_by_startpoint);
  }

  sta::dbSta* sta = resizer_.sta();
  const std::string progress_prefix = fmt::format(
      "{: >9s} | {: >7d} | {: >7d} | {: >8d} | {: >6d} | {: >5d} "
      "| {: >+7.1f}% | {: >8s}",
      label,
      committer_.totalMoves(MoveType::kUnbuffer),
      committer_.totalMoves(MoveType::kSizeUp)
          + committer_.totalMoves(MoveType::kSizeDownFanout)
          + committer_.totalMoves(MoveType::kSizeUpMatch)
          + committer_.totalMoves(MoveType::kVtSwap),
      committer_.totalMoves(MoveType::kBuffer)
          + committer_.totalMoves(MoveType::kSplitLoad),
      committer_.totalMoves(MoveType::kClone),
      committer_.totalMoves(MoveType::kSwapPins),
      areaGrowthPercent(),
      sta::delayAsString(wns, 3, sta));

  // StTNS is a column only in the startpoint layout.
  const std::string startpoint_tns_field
      = show_startpoint_metrics
            ? fmt::format(" | {: >10s}",
                          sta::delayAsString(collector.getTns(true), 1, sta))
            : "";

  resizer_.logger()->report(
      "{}{} | {: >10s} | {: >6d} | {}",
      progress_prefix,
      startpoint_tns_field,
      sta::delayAsString(en_tns, 1, sta),
      std::max(0, collector.getNumViolatingEndpoints()),
      worst_pin != nullptr ? resizer_.network()->pathName(worst_pin) : "");

  debugPrint(resizer_.logger(),
             utl::RSZ,
             "memory",
             1,
             "RSS = {}",
             utl::getCurrentRSS());
}

void ProgressReporter::printFinal(RepairTargetCollector& collector,
                                  const bool show_startpoint_metrics) const
{
  if (!context_.progress_header_printed) {
    printHeader(show_startpoint_metrics);
  }

  // The closing row keeps the layout of the last header and names the worst
  // endpoint.
  printRow("final",
           collector,
           "final_progress",
           context_.progress_header_show_startpoint_metrics,
           /*worst_by_startpoint=*/false);
  resizer_.logger()->report(kRule);
}

double ProgressReporter::designArea() const
{
  // computeDesignArea() walks every instance, and most rows follow passes that
  // changed nothing.  The committer's edit count only grows, so a new count
  // means an edit, even when the move totals come back equal.
  const int edits = committer_.netlistEdits();
  if (context_.progress_area_at_edit != edits) {
    const ProfileScope area("design_area");
    context_.progress_design_area = resizer_.computeDesignArea();
    context_.progress_area_at_edit = edits;
  }
  return context_.progress_design_area;
}

double ProgressReporter::areaGrowthPercent() const
{
  const double design_area = designArea();
  const double initial_area = context_.initial_design_area;
  if (std::abs(initial_area) <= 0.0) {
    return std::numeric_limits<double>::infinity();
  }

  return (design_area - initial_area) / initial_area * 100.0;
}

}  // namespace rsz
