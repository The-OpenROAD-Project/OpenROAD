// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026-2026, The OpenROAD Authors

#pragma once

#include <chrono>
#include <cstdint>
#include <fstream>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <tuple>
#include <unordered_map>
#include <vector>

#include "OptimizerTypes.hh"
#include "odb/dbBlockCallBackObj.h"

namespace rsz {

class MoveCandidate;
class Resizer;

// Run-local diagnostics.  Workers merge their counters after the join and
// never write files or touch the main-thread profiler.
class RepairSetupProfiler : public odb::dbBlockCallBackObj
{
 public:
  // === Types ===============================================================

  // One logical record can update a counter, emit a trace row, or both.
  struct EventOptions
  {
    std::optional<uint64_t> count{1};
    std::string_view metric{};
    std::string_view move{};
    std::string_view object{};
    std::string_view before{};
    std::string_view after{};
    std::string_view detail{};
    bool trace{false};
  };

  // The -profile_mode levels; each one adds to the one below.
  enum class Mode
  {
    kOff = 0,     // no output
    kTiming = 1,  // stats, timing and metadata
    kEvents = 2,  // the event trace and the design edits
  };

  // === Run lifetime and active context ====================================

  // mode 0 disables every output; see OptimizerRunConfig::profile_mode.
  explicit RepairSetupProfiler(Resizer& resizer,
                               int mode = 0,
                               const std::string& directory = "");
  ~RepairSetupProfiler() override;

  // Return the profiler bound to this thread, if any.
  static RepairSetupProfiler* current() { return current_; }
  // Test whether this thread has an active profiler.
  static bool enabled() { return current_ != nullptr; }

  // === Events and move candidates =========================================

  // Record a counter delta, a trace row, or both according to the options.
  static void event(std::string_view name, EventOptions options);
  // Record one occurrence of a counter without a trace row.
  static void event(std::string_view name);
  // Record a counter delta, optionally split by move type.
  static void event(std::string_view name,
                    uint64_t count,
                    std::string_view move = "");
  // Record a trace-only event with a detail string.
  static void event(std::string_view name, std::string_view detail);
  // Assign an identity and count a generated move candidate.
  static void moveGenerated(MoveCandidate& candidate);
  // Record a candidate estimate and its rejection reason.
  static void moveEstimated(MoveCandidate& candidate, const Estimate& estimate);
  // Record selection using the candidate's original target.
  static void moveSelected(MoveCandidate& candidate);

  // === Timing, events and journals ========================================

  // Record WNS and TNS at a named observation point.
  static void qor(const char* point, double wns, double tns);
  // A stop reason: the stop_<reason> counter and the "stop" event.
  static void stop(std::string_view reason);
  // Track a new ECO journal, including journals outside this profiler.
  static void journalBegin();
  // Mark the journal's moves accepted or reverted.
  static void journalEnd(bool undo);
  // Observe one move around the candidate.apply() of MoveCommitter::commit().
  static void beginApply(MoveCandidate& candidate);
  // Record the apply result and the post-move analysis snapshot.
  static void endApply(MoveCandidate& candidate, const MoveResult& result);

  void phase(const std::string& name, int index);
  // Write one run metadata field.
  void metadata(const std::string& key, const std::string& value);
  // The repair_timing options of this run.
  void runConfig(const OptimizerRunConfig& config);
  // Store the policy run's completion state and result.
  void complete(bool result);
  // Begin timing a stage, excluding nested stages from its self time.
  void enter(const char* stage);
  // Accumulate self and inclusive time for the current stage.
  void leave();
  // Flush prior counters and start a new iteration.
  void beginIteration(const char* kind);
  // Record iteration wall time and flush its counters.
  void endIteration();

  // === OpenDB edit callbacks ===============================================

  // Record database edits made while the profiler owns the block.
  void inDbInstCreate(odb::dbInst* inst) override;
  void inDbInstDestroy(odb::dbInst* inst) override;
  void inDbInstSwapMasterBefore(odb::dbInst* inst,
                                odb::dbMaster* master) override;
  void inDbPostMoveInst(odb::dbInst* inst) override;
  void inDbNetCreate(odb::dbNet* net) override;
  void inDbNetDestroy(odb::dbNet* net) override;
  void inDbITermPostConnect(odb::dbITerm* pin) override;
  void inDbITermPostDisconnect(odb::dbITerm* pin, odb::dbNet* net) override;

 private:
  // === Internal records ====================================================

  using Clock = std::chrono::steady_clock;
  using Key = std::tuple<std::string, std::string, std::string>;

  // A timed stage and the time already attributed to nested stages.
  struct Stage
  {
    std::string name;
    Clock::time_point start;
    double children{0.0};
  };

  // A generated candidate's estimate, move identity and analysis observer.
  struct Candidate
  {
    uint64_t id{0};
    uint64_t move_id{0};
    Estimate estimate;
    bool estimated{false};
  };

  // Applied move identities waiting for an ECO journal to settle.
  struct Journal
  {
    uint64_t id;
    std::vector<std::pair<uint64_t, MoveType>> moves;
  };

  // === Aggregation and output =============================================

  // Accumulate a value at iteration, phase and run scope.
  void add(const Key& key, double value);
  // Write pending counters and flush the trace files.
  void flush();
  // Account for root-stage time in the current phase/iteration, then restart
  // its timer so the next interval is not counted twice.
  void accountRootStageInterval();
  // Write one event row with the current move and journal identity.
  void emit(const std::string& event,
            const std::string& object = "",
            const std::string& before = "",
            const std::string& after = "",
            const std::string& detail = "");
  // Count and record a database edit, including mode-3 analysis.
  void edit(const char* event,
            const std::string& object,
            const std::string& before = "",
            const std::string& after = "");
  // Write numeric timing observations at a named point.
  void observation(const char* point,
                   const std::map<std::string, double>& values);
  // Return wall time since this profiler started.
  double elapsed() const;

  // === Thread-local context ===============================================

  inline static thread_local RepairSetupProfiler* current_{nullptr};
  inline static thread_local uint64_t journal_depth_{0};
  Resizer& resizer_;
  Mode mode_{Mode::kOff};
  Clock::time_point start_;
  Clock::time_point iteration_start_;
  std::string directory_;
  std::ofstream stats_, events_, timing_, metadata_;

  // === Phase and iteration state ==========================================

  std::string phase_{"init_onetime"};
  int phase_id_{0};
  uint64_t iteration_{0};
  std::string iteration_kind_{"phase_boundary"};
  bool completed_{false};
  bool result_{false};

  // === Active move and journal context ====================================

  uint64_t event_id_{0}, candidate_id_{0}, move_id_{0}, journal_id_{0};
  uint64_t active_move_{0}, active_candidate_{0};
  std::string active_type_;
  std::vector<Stage> stages_;
  std::map<Key, double> delta_, phase_total_, run_total_;
  std::unordered_map<const MoveCandidate*, Candidate> candidates_;
  std::vector<Journal> journals_;
};

// Times a nested profiler stage for the lifetime of this scope.
class ProfileScope
{
 public:
  explicit ProfileScope(const char* name)
      : profiler_(RepairSetupProfiler::current())
  {
    if (profiler_ != nullptr) {
      profiler_->enter(name);
    }
  }
  ~ProfileScope()
  {
    if (profiler_ != nullptr) {
      profiler_->leave();
    }
  }
  ProfileScope(const ProfileScope&) = delete;
  ProfileScope& operator=(const ProfileScope&) = delete;

 private:
  RepairSetupProfiler* profiler_;
};

class ProfileIteration
{
 public:
  explicit ProfileIteration(const char* kind)
      : profiler_(RepairSetupProfiler::current())
  {
    if (profiler_ != nullptr) {
      profiler_->beginIteration(kind);
    }
  }
  ~ProfileIteration()
  {
    if (profiler_ != nullptr) {
      profiler_->endIteration();
    }
  }
  ProfileIteration(const ProfileIteration&) = delete;
  ProfileIteration& operator=(const ProfileIteration&) = delete;

 private:
  RepairSetupProfiler* profiler_;
};

}  // namespace rsz
