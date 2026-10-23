// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026-2026, The OpenROAD Authors

#include "RepairSetupProfiler.hh"

#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <iomanip>
#include <memory>
#include <sstream>
#include <system_error>
#include <utility>

#include "MoveCandidate.hh"
#include "RepairSetupAnalysisReport.hh"
#include "RepairSetupMoveSampler.hh"
#include "db_sta/dbNetwork.hh"
#include "db_sta/dbSta.hh"
#include "est/EstimateParasitics.h"
#include "odb/db.h"
#include "rsz/Resizer.hh"
#include "sta/Delay.hh"
#include "sta/Network.hh"
#include "utl/Logger.h"

namespace rsz {
namespace {

// RFC 4180 quoting also preserves hierarchical names containing delimiters.
std::string csv(const std::string& value)
{
  std::string result = "\"";
  for (const char ch : value) {
    if (ch == '"') {
      result += '"';
    }
    result += ch;
  }

  return result + '"';
}

std::string number(const double value)
{
  std::ostringstream stream;
  stream << std::setprecision(17) << value;
  return stream.str();
}

// The phases the profiler covers completely; others are marked partial.
constexpr std::array<std::string_view, 4> kCoveredPhases
    = {"LEGACY", "LAST_GASP", "CRIT_VT_SWAP", "PARALLEL_EVAL"};

}  // namespace

RepairSetupProfiler::RepairSetupProfiler(Resizer& resizer,
                                         const Mode mode,
                                         const std::string& directory)
    : resizer_(resizer), mode_(mode)
{
  if (mode_ == Mode::kOff) {
    return;
  }

  const std::filesystem::path root
      = directory.empty() ? "repair_setup_profile" : directory;
  std::error_code error;
  std::filesystem::create_directories(root, error);

  // Reserve a fresh directory even across successive processes and calls.
  for (uint64_t run = 1; !error; ++run) {
    const std::filesystem::path run_directory
        = root / ("run_" + std::to_string(run));
    if (std::filesystem::create_directory(run_directory, error)) {
      directory_ = std::filesystem::absolute(run_directory, error).string();
      break;
    }
  }

  // The event trace, events.csv, starts at mode 2.
  const bool trace_events = mode_ >= Mode::kEvents;
  if (!error) {
    stats_.open(directory_ + "/stats.csv");
    if (trace_events) {
      events_.open(directory_ + "/events.csv");
    }
    timing_.open(directory_ + "/timing.csv");
    metadata_.open(directory_ + "/metadata.csv");
  }
  if (error || !stats_ || (trace_events && !events_) || !timing_
      || !metadata_) {
    errorCannotOpen(directory_.empty() ? root.string() : directory_);
  }

  stats_ << "phase_id,phase,iteration,iteration_kind,category,metric,move,"
            "delta,phase_total,run_total\n";
  if (trace_events) {
    events_ << "event_id,elapsed_s,phase,iteration,move_id,candidate_id,"
               "journal_id,event,move,object,before,after,detail\n";
    events_ << std::setprecision(17);
  }
  timing_ << "event_id,elapsed_s,phase,iteration,move_id,point,metric,value\n";
  metadata_ << "key,value\n";
  stats_ << std::setprecision(17);
  timing_ << std::setprecision(17);
  start_ = Clock::now();
  if (mode_ == Mode::kAnalysis) {
    analysis_report_ = std::make_unique<RepairSetupAnalysisReport>(
        resizer_, directory_, start_);
    if (!analysis_report_->opened()) {
      errorCannotOpen(analysis_report_->directory());
    }
    resizer_.logger()->warn(utl::RSZ,
                            2031,
                            "Profile mode 3 updates parasitics and timing "
                            "around every move, so the result can differ "
                            "slightly from mode 0.");
  }

  current_ = this;
  if (mode_ >= Mode::kEvents) {
    addOwner(resizer_.dbNetwork()->block());
  }

  metadata("schema_version", "1");
  metadata("profile_mode", std::to_string(static_cast<int>(mode_)));
  metadata("time_unit", "s");
  metadata("timing_unit", "s");
  metadata("status", "running");
  metadata("external_journal_depth", std::to_string(journal_depth_));
  metadata(
      "acceptance_scope",
      "run-local checkpoint; later or direct ODB outer undo is not observed");
  std::string covered_phases;
  for (const std::string_view phase : kCoveredPhases) {
    covered_phases += (covered_phases.empty() ? "" : " ") + std::string(phase);
  }
  metadata("complete_policy_coverage", covered_phases);
  if (mode_ == Mode::kAnalysis) {
    metadata("analysis_report", "analysis/bad_moves.md");
    metadata("analysis_capture",
             "selected candidates; per-scene/RF matched pin timing; direct "
             "incident-net neighborhood");
  }
}

void RepairSetupProfiler::errorCannotOpen(const std::string& directory) const
{
  resizer_.logger()->error(utl::RSZ,
                           2032,
                           "Cannot open the repair setup profile files "
                           "under {}.",
                           directory);
}

RepairSetupProfiler::~RepairSetupProfiler()
{
  if (mode_ == Mode::kOff) {
    return;
  }

  removeOwner();
  if (analysis_report_ != nullptr) {
    metadata("analysis_status",
             analysis_report_->finish() ? "completed" : "write_failed");
  }

  flush();
  metadata("wall_seconds", number(elapsed()));
  metadata("status", completed_ ? "completed" : "interrupted");
  metadata("policy_result", result_ ? "true" : "false");
  metadata("open_journals", std::to_string(journals_.size()));
  current_ = nullptr;
  resizer_.logger()->report(
      "Repair setup profile: {} ({:.3f}s)", directory_, elapsed());
}

double RepairSetupProfiler::elapsed() const
{
  return std::chrono::duration<double>(Clock::now() - start_).count();
}

void RepairSetupProfiler::metadata(const std::string& key,
                                   const std::string& value)
{
  if (mode_ != Mode::kOff) {
    metadata_ << csv(key) << ',' << csv(value) << '\n';
    metadata_.flush();
  }
}

void RepairSetupProfiler::runConfig(const OptimizerRunConfig& config)
{
  metadata("requested_phases", config.phases);
  metadata("threads", std::to_string(resizer_.staState()->threadCount()));
  metadata("max_iterations", std::to_string(config.max_iterations));
  metadata("max_passes", std::to_string(config.max_passes));
  std::string sequence;
  for (const MoveType move : config.sequence) {
    sequence += std::string(moveName(move)) + " ";
  }
  metadata("sequence", sequence);
}

void RepairSetupProfiler::complete(const bool result)
{
  completed_ = true;
  result_ = result;
}

void RepairSetupProfiler::beginPhase(const std::string& name, const int index)
{
  if (mode_ == Mode::kOff) {
    return;
  }

  accountRootStageInterval();
  flush();
  phase_ = name;
  phase_id_ = index;
  iteration_ = 0;
  iteration_kind_ = "phase_boundary";
  phase_total_.clear();
  candidates_.clear();
  endpoint_ = nullptr;
  metadata("phase_" + std::to_string(index), name);
  if (std::ranges::find(kCoveredPhases, name) == kCoveredPhases.end()) {
    metadata("partial_phase_coverage", name);
  }

  event("phase_begin", name);
}

void RepairSetupProfiler::add(const Key& key, const double value)
{
  delta_[key] += value;
  phase_total_[key] += value;
  run_total_[key] += value;
}

void RepairSetupProfiler::event(const std::string_view name,
                                const EventOptions options)
{
  if (options.count.has_value()) {
    const std::string_view metric
        = options.metric.empty() ? name : options.metric;
    if (enabled()) {
      current_->add({"count", std::string(metric), std::string(options.move)},
                    static_cast<double>(*options.count));
    } else if (deferred_counts_ != nullptr) {
      deferred_counts_->push_back(
          {std::string(metric), std::string(options.move), *options.count});
    }
  }

  if (options.trace && enabled() && current_->mode_ >= Mode::kEvents) {
    current_->emit(std::string(name),
                   std::string(options.object),
                   std::string(options.before),
                   std::string(options.after),
                   std::string(options.detail));
  }
}

void RepairSetupProfiler::event(const std::string_view name)
{
  event(name, EventOptions{});
}

void RepairSetupProfiler::event(const std::string_view name,
                                const uint64_t count,
                                const std::string_view move)
{
  event(name, {.count = count, .move = move});
}

void RepairSetupProfiler::event(const std::string_view name,
                                const std::string_view detail)
{
  event(name, {.count = std::nullopt, .detail = detail, .trace = true});
}

void RepairSetupProfiler::replay(const DeferredCounts& counts)
{
  for (const DeferredCount& deferred : counts) {
    event(deferred.name, deferred.value, deferred.move);
  }
}

void RepairSetupProfiler::enter(const char* stage)
{
  stages_.push_back({stage, Clock::now(), 0.0});
}

void RepairSetupProfiler::leave()
{
  const Stage stage = stages_.back();
  stages_.pop_back();
  const double wall
      = std::chrono::duration<double>(Clock::now() - stage.start).count();
  if (!stages_.empty()) {
    stages_.back().children += wall;
  }

  add({"time", stage.name, ""}, wall - stage.children);
  add({"inclusive_time", stage.name, ""}, wall);
  event(stage.name + "_calls");
}

void RepairSetupProfiler::workerTime(const std::string_view task,
                                     const double seconds)
{
  if (enabled()) {
    event(std::string(task) + "_tasks");
    current_->add({"worker_time", "candidate_" + std::string(task), ""},
                  seconds);
  }
}

void RepairSetupProfiler::accountRootStageInterval()
{
  if (stages_.size() != 1) {
    return;
  }

  Stage& stage = stages_.front();
  const Clock::time_point now = Clock::now();
  const double wall = std::chrono::duration<double>(now - stage.start).count();
  add({"time", stage.name, ""}, wall - stage.children);
  add({"inclusive_time", stage.name, ""}, wall);
  stage.start = now;
  stage.children = 0.0;
}

void RepairSetupProfiler::flush()
{
  for (const std::pair<const Key, double>& entry : delta_) {
    const Key& key = entry.first;
    stats_ << phase_id_ << ',' << csv(phase_) << ',' << iteration_ << ','
           << csv(iteration_kind_) << ',' << csv(std::get<0>(key)) << ','
           << csv(std::get<1>(key)) << ',' << csv(std::get<2>(key)) << ','
           << entry.second << ',' << phase_total_[key] << ',' << run_total_[key]
           << '\n';
  }

  delta_.clear();
  stats_.flush();
  events_.flush();
  timing_.flush();
}

void RepairSetupProfiler::beginIteration(const char* kind)
{
  accountRootStageInterval();
  flush();
  ++iteration_;
  iteration_kind_ = kind;
  iteration_start_ = Clock::now();
  candidates_.clear();
  event("iteration_begin",
        {.metric = "iterations", .detail = kind, .trace = true});
}

void RepairSetupProfiler::endIteration()
{
  const double wall
      = std::chrono::duration<double>(Clock::now() - iteration_start_).count();
  add({"iteration_wall", "elapsed", ""}, wall);
  event("iteration_end", {.count = std::nullopt, .trace = true});
  accountRootStageInterval();
  flush();
}

void RepairSetupProfiler::moveGenerated(MoveCandidate& candidate)
{
  if (enabled()) {
    Candidate& info = current_->candidates_[&candidate];
    info = {++current_->candidate_id_, 0, std::nullopt, nullptr};
    event("generated", 1, moveName(candidate.type()));
  }
}

void RepairSetupProfiler::movesGenerated(const MoveType type,
                                         const CandidateVector& candidates)
{
  if (candidates.empty()) {
    event("skip_no_candidates", 1, moveName(type));
  }

  if (enabled()) {
    for (const std::unique_ptr<MoveCandidate>& candidate : candidates) {
      moveGenerated(*candidate);
    }
  }
}

void RepairSetupProfiler::moveEstimated(MoveCandidate& candidate,
                                        const Estimate& estimate)
{
  if (enabled()) {
    Candidate& info = current_->candidates_[&candidate];
    info.estimate = estimate;
    event("estimated", 1, moveName(candidate.type()));
    event(estimate.legal ? "estimate_legal" : "estimate_rejected",
          1,
          moveName(candidate.type()));
    if (estimate.sequential_penalty.has_value()) {
      event("sequential_estimated", 1, moveName(candidate.type()));
      if (*estimate.sequential_penalty > 0.0f) {
        event("sequential_penalized", 1, moveName(candidate.type()));
      }
    }

    if (estimate.side_loss.has_value() && *estimate.side_loss > 0.0f) {
      event("side_loss_penalized", 1, moveName(candidate.type()));
    }

    if (!estimate.reason.empty()) {
      event("estimate_reason:" + std::string(estimate.reason),
            1,
            moveName(candidate.type()));
    }
  }
}

void RepairSetupProfiler::moveSelected(MoveCandidate& candidate)
{
  moveSelected(candidate, &candidate.target(), nullptr);
}

void RepairSetupProfiler::moveSelected(MoveCandidate& candidate,
                                       const Target* observed_target,
                                       const sta::Pin* original_endpoint)
{
  if (enabled()) {
    Candidate& info = current_->candidates_[&candidate];
    if (analysis() && observed_target == nullptr) {
      info.observed = false;
    } else if (analysis()) {
      const ProfileScope scope("analysis_observation");
      info.observer
          = std::make_shared<RepairSetupMoveSampler>(current_->resizer_,
                                                     *observed_target,
                                                     original_endpoint,
                                                     current_->endpoint_);

      event("analysis_selected_snapshots");
    }

    current_->active_candidate_ = info.id;
    current_->active_type_ = moveName(candidate.type());
    // An unestimated candidate reports an empty gain and a zero score.
    const Estimate estimate = info.estimate.value_or(Estimate{});
    event("candidate_selected",
          {.metric = "selected",
           .move = moveName(candidate.type()),
           .before
           = estimate.raw_gain.has_value() ? number(*estimate.raw_gain) : "",
           .after = number(estimate.score),
           .detail = "before=raw_gain_s;after=weighted_score",
           .trace = true});
    current_->active_candidate_ = 0;
    current_->active_type_.clear();
  }
}

void RepairSetupProfiler::endpoint(const sta::Pin* pin)
{
  if (enabled()) {
    current_->endpoint_ = pin;
  }
}

void RepairSetupProfiler::emit(const std::string& name,
                               const std::string& object,
                               const std::string& before,
                               const std::string& after,
                               const std::string& detail)
{
  if (mode_ < Mode::kEvents) {
    return;
  }

  const ProfileScope scope("trace_io");
  events_ << ++event_id_ << ',' << elapsed() << ',' << csv(phase_) << ','
          << iteration_ << ',' << active_move_ << ',' << active_candidate_
          << ',' << (journals_.empty() ? 0 : journals_.back().id) << ','
          << csv(name) << ',' << csv(active_type_) << ',' << csv(object) << ','
          << csv(before) << ',' << csv(after) << ',' << csv(detail) << '\n';
}

void RepairSetupProfiler::stop(const std::string_view reason)
{
  if (!enabled()) {
    return;
  }

  const std::string metric = "stop_" + std::string(reason);
  event("stop", {.metric = metric, .detail = reason, .trace = true});
}

void RepairSetupProfiler::observation(
    const char* point,
    const std::map<std::string, double>& values)
{
  const ProfileScope scope("trace_io");
  const uint64_t event = ++event_id_;
  const double time = elapsed();
  for (const std::pair<const std::string, double>& value : values) {
    if (std::isfinite(value.second)) {
      timing_ << event << ',' << time << ',' << csv(phase_) << ',' << iteration_
              << ',' << active_move_ << ',' << csv(point) << ','
              << csv(value.first) << ',' << value.second << '\n';
    }
  }
}

void RepairSetupProfiler::qor(const char* point,
                              const double wns,
                              const double tns)
{
  if (enabled()) {
    std::map<std::string, double> values;
    if (!sta::delayInf(wns, current_->resizer_.sta())) {
      values["wns_s"] = wns;
    }

    if (!sta::delayInf(tns, current_->resizer_.sta())) {
      values["tns_s"] = tns;
    }

    current_->observation(point, values);
  }
}

void RepairSetupProfiler::journalBegin()
{
  ++journal_depth_;
  if (enabled()) {
    const uint64_t parent
        = current_->journals_.empty() ? 0 : current_->journals_.back().id;
    current_->journals_.push_back({++current_->journal_id_, {}});
    const std::string detail
        = "parent=" + std::to_string(parent) + ";external_journal_depth="
          + std::to_string(journal_depth_ - current_->journals_.size());
    event("journal_begin", {.detail = detail, .trace = true});
  }
}

void RepairSetupProfiler::journalEnd(const bool undo)
{
  if (journal_depth_ > 0) {
    --journal_depth_;
  }

  if (!enabled() || current_->journals_.empty()) {
    return;
  }

  RepairSetupProfiler& profiler = *current_;
  Journal journal = std::move(profiler.journals_.back());
  event(undo ? "journal_undo" : "journal_commit", {.trace = true});
  const uint64_t saved_move = profiler.active_move_;
  const std::string saved_type = profiler.active_type_;
  for (const std::pair<uint64_t, MoveType>& move : journal.moves) {
    profiler.active_move_ = move.first;
    profiler.active_type_ = moveName(move.second);
    if (undo || profiler.journals_.size() == 1) {
      event(undo ? "move_reverted" : "move_checkpoint_accepted",
            {.metric = undo ? "reverted" : "checkpoint_accepted",
             .move = profiler.active_type_,
             .trace = true});
      if (profiler.analysis_report_ != nullptr) {
        profiler.analysis_report_->settle(move.first, undo);
      }
    }
  }

  profiler.active_move_ = saved_move;
  profiler.active_type_ = saved_type;
  profiler.journals_.pop_back();
  if (!undo && !profiler.journals_.empty()) {
    std::vector<std::pair<uint64_t, MoveType>>& parent
        = profiler.journals_.back().moves;
    parent.insert(parent.end(), journal.moves.begin(), journal.moves.end());
  }
}

void RepairSetupProfiler::beginApply(MoveCandidate& candidate)
{
  if (!enabled()) {
    return;
  }

  RepairSetupProfiler& profiler = *current_;
  const Candidate info = profiler.candidates_[&candidate];
  profiler.active_move_ = ++profiler.move_id_;
  profiler.candidates_[&candidate].move_id = profiler.active_move_;
  profiler.active_candidate_ = info.id;
  profiler.active_type_ = moveName(candidate.type());
  const sta::Pin* pin = candidate.target().driver_pin;
  const std::string object
      = pin != nullptr ? profiler.resizer_.network()->pathName(pin) : "";
  event("apply_begin",
        {.metric = "apply_attempted",
         .move = profiler.active_type_,
         .object = object,
         .trace = true});
  if (info.estimate.has_value()) {
    // The values of one estimate share one observation event.
    const Estimate& estimate = *info.estimate;
    std::map<std::string, double> values{{"weighted_score", estimate.score}};
    if (estimate.raw_gain.has_value()) {
      values["raw_gain_s"] = *estimate.raw_gain;
    }

    if (estimate.sequential_penalty.has_value()) {
      values["sequential_penalty_s"] = *estimate.sequential_penalty;
    }

    if (estimate.sequential_side_loss.has_value()) {
      values["sequential_side_loss_s"] = *estimate.sequential_side_loss;
    }

    if (estimate.side_loss.has_value()) {
      values["side_loss_s"] = *estimate.side_loss;
    }
    profiler.observation("estimate", values);
  }

  profiler.active_observer_ = info.observer;
  if (analysis() && info.observed) {
    if (profiler.active_observer_ == nullptr) {
      profiler.active_observer_ = std::make_shared<RepairSetupMoveSampler>(
          profiler.resizer_, candidate.target(), nullptr, profiler.endpoint_);
    }

    profiler.analysis_report_->beginMove({profiler.active_move_,
                                          info.id,
                                          profiler.phase_,
                                          profiler.iteration_,
                                          candidate.type(),
                                          info.estimate},
                                         *profiler.active_observer_);
    profiler.observation("before", profiler.analysis_report_->beforeMetrics());
  }
}

void RepairSetupProfiler::endApply(MoveCandidate& candidate,
                                   const MoveResult& result)
{
  if (!enabled()) {
    return;
  }

  RepairSetupProfiler& profiler = *current_;
  event(result.accepted ? "apply_succeeded" : "apply_failed",
        {.move = profiler.active_type_, .trace = true});
  event("reported_move_count", result.move_count, profiler.active_type_);
  if (result.accepted) {
    if (profiler.journals_.empty()) {
      event("move_checkpoint_accepted",
            {.metric = "checkpoint_accepted",
             .move = profiler.active_type_,
             .trace = true});
      if (profiler.analysis_report_ != nullptr) {
        profiler.analysis_report_->settle(profiler.active_move_, false);
      }
    } else {
      profiler.journals_.back().moves.emplace_back(profiler.active_move_,
                                                   candidate.type());
    }
  }

  if (profiler.active_observer_ != nullptr) {
    profiler.analysis_report_->endMove(result, *profiler.active_observer_);
    profiler.observation("after", profiler.analysis_report_->afterMetrics());
    profiler.observation("delta", profiler.analysis_report_->deltaMetrics());
    profiler.active_observer_.reset();
  }

  profiler.active_move_ = 0;
  profiler.active_candidate_ = 0;
  profiler.active_type_.clear();
}

void RepairSetupProfiler::beginRevert(MoveCandidate& candidate)
{
  if (!enabled()) {
    return;
  }

  RepairSetupProfiler& profiler = *current_;
  assert(profiler.active_move_ == 0);
  const Candidate& info = profiler.candidates_.at(&candidate);
  assert(info.move_id != 0);

  // The inverse swap is recorded under the original move's identity.
  profiler.active_move_ = info.move_id;
  profiler.active_candidate_ = info.id;
  profiler.active_type_ = moveName(candidate.type());
}

void RepairSetupProfiler::endRevert(const char* detail)
{
  if (!enabled()) {
    return;
  }

  RepairSetupProfiler& profiler = *current_;
  assert(profiler.active_move_ != 0 && !profiler.journals_.empty());
  std::vector<std::pair<uint64_t, MoveType>>& moves
      = profiler.journals_.back().moves;
  const std::vector<std::pair<uint64_t, MoveType>>::iterator move
      = std::find_if(moves.begin(),
                     moves.end(),
                     [&](const std::pair<uint64_t, MoveType>& item) {
                       return item.first == profiler.active_move_;
                     });
  // Remove the original identity before its parent journal can settle it.
  if (move != moves.end()) {
    moves.erase(move);
  }

  event("move_reverted",
        {.metric = "reverted",
         .move = profiler.active_type_,
         .detail = detail,
         .trace = true});
  if (profiler.analysis_report_ != nullptr) {
    profiler.analysis_report_->settle(profiler.active_move_, true);
  }

  profiler.active_move_ = 0;
  profiler.active_candidate_ = 0;
  profiler.active_type_.clear();
}

void RepairSetupProfiler::edit(const char* name,
                               const std::string& object,
                               const std::string& before,
                               const std::string& after)
{
  event("db_edits");
  event(name,
        {.object = object, .before = before, .after = after, .trace = true});
  if (analysis_report_ != nullptr && active_move_ != 0) {
    analysis_report_->edit(name, object, before, after);
  }
}

void RepairSetupProfiler::inDbInstCreate(odb::dbInst* inst)
{
  edit("inst_create", inst->getName(), "", inst->getMaster()->getName());
}
void RepairSetupProfiler::inDbInstDestroy(odb::dbInst* inst)
{
  edit("inst_destroy", inst->getName(), inst->getMaster()->getName());
}
void RepairSetupProfiler::inDbInstSwapMasterBefore(odb::dbInst* inst,
                                                   odb::dbMaster* master)
{
  edit("inst_swap",
       inst->getName(),
       inst->getMaster()->getName(),
       master->getName());
}
void RepairSetupProfiler::inDbPostMoveInst(odb::dbInst* inst)
{
  int x, y;
  inst->getLocation(x, y);
  edit("inst_location",
       inst->getName(),
       "",
       std::to_string(x) + ":" + std::to_string(y));
}
void RepairSetupProfiler::inDbNetCreate(odb::dbNet* net)
{
  edit("net_create", net->getName());
}
void RepairSetupProfiler::inDbNetDestroy(odb::dbNet* net)
{
  edit("net_destroy", net->getName());
}
void RepairSetupProfiler::inDbITermPostConnect(odb::dbITerm* pin)
{
  edit(
      "pin_connect",
      pin->getInst()->getName() + std::string("/") + pin->getMTerm()->getName(),
      "",
      pin->getNet()->getName());
}
void RepairSetupProfiler::inDbITermPostDisconnect(odb::dbITerm* pin,
                                                  odb::dbNet* net)
{
  edit(
      "pin_disconnect",
      pin->getInst()->getName() + std::string("/") + pin->getMTerm()->getName(),
      net->getName());
}

}  // namespace rsz
