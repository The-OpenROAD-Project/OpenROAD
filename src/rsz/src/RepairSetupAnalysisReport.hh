// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026-2026, The OpenROAD Authors

#pragma once

#include <chrono>
#include <cstdint>
#include <fstream>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <vector>

#include "OptimizerTypes.hh"
#include "RepairSetupMoveSnapshot.hh"

namespace rsz {

class RepairSetupMoveSampler;
class Resizer;

// Writes per-move timing evidence and classified bad-move reports for mode 3.
// It is run-local; optimization policies do not own its output streams.
class RepairSetupAnalysisReport
{
 public:
  // Move identity and estimate attached to one before/after observation.
  struct Context
  {
    uint64_t move_id;
    uint64_t candidate_id;
    std::string phase;
    uint64_t iteration;
    MoveType type;
    // Unset when the move was applied without an estimate.
    std::optional<Estimate> estimate;
  };

  // === Construction =========================================================
  RepairSetupAnalysisReport(Resizer& resizer,
                            const std::string& directory,
                            std::chrono::steady_clock::time_point start);
  // False when the output directory or one of the files could not be opened.
  bool opened() const;
  const std::string& directory() const { return directory_; }

  // === Move lifecycle =======================================================
  // Capture the pre-ECO state and establish the active move identity.
  void beginMove(const Context& context, RepairSetupMoveSampler& observer);
  // Capture post-ECO state and classify any timing regression.
  void endMove(const MoveResult& result, RepairSetupMoveSampler& observer);
  // Record one OpenDB edit under the active move.
  void edit(const std::string& event,
            const std::string& object,
            const std::string& before,
            const std::string& after);
  // Finalize a flagged move once its ECO journal is kept or reverted.
  void settle(uint64_t move_id, bool reverted);
  // Flush all outputs and report whether case files were written.
  bool finish();

  // === Active move metrics ==================================================
  const std::map<std::string, double>& beforeMetrics() const
  {
    return before_.metrics;
  }
  const std::map<std::string, double>& afterMetrics() const
  {
    return after_.metrics;
  }
  const std::map<std::string, double>& deltaMetrics() const { return delta_; }

 private:
  // One database mutation associated with the active move.
  struct Edit
  {
    std::string event;
    std::string object;
    std::string before;
    std::string after;
  };

  // Diagnostic state of one applied move until its journal settles.
  struct Entry
  {
    Context context;
    std::string target;
    std::string before_master;
    std::string after_master;
    std::string outcome{"tentative"};
    std::vector<std::string> findings;
    std::optional<double> wns_delta;
    std::optional<double> tns_delta;
    bool applied{false};
  };

  // The report of one flagged move.  It renders when the move applies and is
  // written once its journal settles, so the outcome sits between the head with
  // the overview and DB edits and the body with the paths.
  struct CaseReport
  {
    std::string head;
    std::string body;
  };

  // === Report writing =======================================================
  // Write the pin, arc, path, and net rows of one observation point.
  void dumpSnapshot(const AnalysisSnapshot& snapshot, const std::string& point);
  // Write the estimator window rows of the active move.
  void dumpWindow(const std::vector<AnalysisWindowStage>& window);
  // The metric deltas of the move, with the window gain and the estimate
  // error of an applied cell swap.
  void measureDeltas(const Context& context, bool accepted);
  // Write the move's row of the moves table.
  void writeMoveRow(const Entry& entry,
                    bool accepted,
                    const RepairSetupMoveSampler& observer,
                    double after_time);
  // Render the case report of a flagged move and write it, or keep it until
  // the move's journal settles.
  void reportFlaggedMove(const Entry& entry,
                         const RepairSetupMoveSampler& observer);
  // Identify timing losses and their likely electrical cause.
  void classify(Entry& entry, const RepairSetupMoveSampler& observer) const;
  // The nets whose load grew because a target input pin on them grew.
  std::set<std::string> growingInputNets() const;
  // Build a self-contained explanation for one flagged move.
  CaseReport renderCase(const Entry& entry,
                        const RepairSetupMoveSampler& observer) const;
  void writeCase(const Entry& entry, const CaseReport& report);
  void writeOverview(std::ostream& output,
                     const Entry& entry,
                     const RepairSetupMoveSampler& observer) const;
  void writeInterpretation(std::ostream& output, const Entry& entry) const;
  void writeTargetTiming(std::ostream& output, const Entry& entry) const;
  void writeElectrical(std::ostream& output, const Entry& entry) const;
  void writeConnections(std::ostream& output) const;
  void writeArcs(std::ostream& output) const;
  void writePaths(std::ostream& output,
                  const std::string& selected_endpoint) const;
  double elapsed() const;

  // === State ================================================================
  Resizer& resizer_;
  std::string directory_;
  std::chrono::steady_clock::time_point start_;
  // The entry of the move beginMove() opened; entries_ keeps its address.
  Entry* active_{nullptr};
  // The snapshots of the active move around its apply().
  AnalysisSnapshot before_;
  AnalysisSnapshot after_;
  // Index into after_ for matching before/after pin rows.
  std::map<AnalysisPinKey, const AnalysisPinState*> after_pins_;
  // after_ minus before_ for each metric of the active move.
  std::map<std::string, double> delta_;
  // The OpenDB edits of the active move.
  std::vector<Edit> edits_;
  // Move records awaiting or retaining their final journal outcome.
  std::map<uint64_t, Entry> entries_;
  // Reports of flagged moves whose journal has not settled them yet.
  std::map<uint64_t, CaseReport> pending_cases_;
  // Set when a report could not be written; finish() reports it, since a
  // journal settle is no place to throw.
  bool case_write_failed_{false};
  double before_time_{0.0};
  std::ofstream moves_;
  std::ofstream pins_;
  std::ofstream arcs_;
  std::ofstream paths_;
  std::ofstream connections_;
  std::ofstream windows_;
};

}  // namespace rsz
