// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026-2026, The OpenROAD Authors

#include "RepairSetupAnalysisReport.hh"

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <functional>
#include <initializer_list>
#include <optional>
#include <set>
#include <sstream>
#include <system_error>
#include <utility>
#include <vector>

#include "RepairSetupMoveSampler.hh"
#include "RepairSetupProfiler.hh"
#include "rsz/Resizer.hh"
#include "utl/Logger.h"

namespace rsz {
namespace {

constexpr double kNoise = 1e-15;
// A pin capacitance change at or below this is float noise, not growth: the
// Liberty caps are float values near 1e-15 F, which resolve about 1e-22 F.
constexpr double kCapNoise = 1e-21;

std::string number(const std::optional<double> value)
{
  return value.has_value() && std::isfinite(*value)
             ? fmt::format("{:.17g}", *value)
             : "";
}

std::string scaled(const std::optional<double> value, const double factor)
{
  return value.has_value() ? fmt::format("{:.4f}", *value * factor) : "N/A";
}

void row(std::ostream& stream, const std::initializer_list<std::string> values)
{
  bool first = true;
  for (const std::string& value : values) {
    if (!first) {
      stream << ',';
    }

    first = false;
    stream << '"';
    for (const char ch : value) {
      if (ch == '"') {
        stream << '"';
      }
      stream << ch;
    }

    stream << '"';
  }

  stream << '\n';
}

std::string markdown(const std::string& value)
{
  std::string result;
  for (const char ch : value) {
    if (ch == '|') {
      result += "\\|";
    } else if (ch == '\n' || ch == '\r') {
      result += ' ';
    } else {
      result += ch;
    }
  }

  return result;
}

std::optional<double> metric(const AnalysisSnapshot& snapshot,
                             const std::string& name)
{
  const std::map<std::string, double>::const_iterator found
      = snapshot.metrics.find(name);
  return found != snapshot.metrics.end() ? std::optional<double>(found->second)
                                         : std::nullopt;
}

std::optional<double> difference(const std::optional<double> before,
                                 const std::optional<double> after)
{
  return before.has_value() && after.has_value()
             ? std::optional<double>(*after - *before)
             : std::nullopt;
}

bool worsened(const std::optional<double> before,
              const std::optional<double> after)
{
  return before.has_value() && after.has_value() && *after < *before - kNoise;
}

const AnalysisPinState* findPin(
    const std::map<AnalysisPinKey, const AnalysisPinState*>& pins,
    const AnalysisPinState& before)
{
  const std::map<AnalysisPinKey, const AnalysisPinState*>::const_iterator found
      = pins.find(before.key());
  return found != pins.end() ? found->second : nullptr;
}

std::string findingsText(const std::vector<std::string>& findings)
{
  std::string text;
  for (const std::string& finding : findings) {
    if (!text.empty()) {
      text += ';';
    }
    text += finding;
  }

  return text;
}

std::string caseName(const uint64_t id)
{
  return fmt::format("move_{:06}.md", id);
}

// The outcome of a move until it fails to apply or its journal settles it.
constexpr const char* kTentativeOutcome = "tentative";

// "before -> after" when the two differ, the value alone when they are the
// same, so the merged path tables show only the changes.
std::string transition(const std::optional<double> before,
                       const std::optional<double> after,
                       const double factor)
{
  const std::string lhs = scaled(before, factor);
  const std::string rhs = scaled(after, factor);
  return lhs == rhs ? lhs : lhs + " -> " + rhs;
}

std::string transition(const std::string& before, const std::string& after)
{
  return before == after ? before : before + " -> " + after;
}

// The capacitance a connection row shows: the Liberty Cin of a load and the
// total pin load at a driver, the larger over the pin's transitions and
// scenes.
std::optional<double> connectionCap(const AnalysisSnapshot& snapshot,
                                    const std::string& pin,
                                    const bool driver)
{
  std::optional<double> cap;
  for (const AnalysisPinState& state : snapshot.pins) {
    const std::optional<double>& value
        = driver ? state.pin_cap : state.input_cap;
    if (state.pin == pin && value.has_value()
        && (!cap.has_value() || *value > *cap)) {
      cap = value;
    }
  }
  return cap;
}

// The pins of the move target's instance, by their watch roles.
std::set<std::string> targetPins(const AnalysisSnapshot& before,
                                 const AnalysisSnapshot& after)
{
  std::set<std::string> pins;
  for (const AnalysisSnapshot* snapshot : {&before, &after}) {
    for (const AnalysisPinState& state : snapshot->pins) {
      if (state.role.find("target_input") != std::string::npos
          || state.role.find("target_output") != std::string::npos) {
        pins.insert(state.pin);
      }
    }
  }
  return pins;
}

bool passesTarget(const AnalysisPathState& path,
                  const std::set<std::string>& target_pins)
{
  return std::ranges::any_of(path.points, [&](const AnalysisPathPoint& point) {
    return target_pins.contains(point.pin);
  });
}

// Whether a selected endpoint datapath of the snapshot passes the target.
bool datapathPassesTarget(const AnalysisSnapshot& snapshot,
                          const std::set<std::string>& target_pins)
{
  return std::ranges::any_of(snapshot.paths,
                             [&](const AnalysisPathState& path) {
                               return path.kind == "selected_endpoint_datapath"
                                      && passesTarget(path, target_pins);
                             });
}

// Where the move target sits on a before/after path pair.
const char* targetPlacement(const AnalysisPathState* before,
                            const AnalysisPathState* after,
                            const std::set<std::string>& target_pins)
{
  const bool before_on
      = before != nullptr && passesTarget(*before, target_pins);
  const bool after_on = after != nullptr && passesTarget(*after, target_pins);
  if (before == nullptr || after == nullptr || before_on == after_on) {
    return before_on || after_on ? "on this path" : "not on this path";
  }
  return before_on ? "on this path before the move only"
                   : "on this path after the move only";
}

// The key that pairs a before path with its after path.  The global worst
// path keys by scene alone, whatever its endpoint, and the others by end pin,
// transition and analysis.
std::string pathKey(const AnalysisPathState& path)
{
  if (path.kind == "global_endpoint_path") {
    return path.kind + "|" + path.scene;
  }
  return path.kind + "|" + path.scene + "|" + path.rf + "|" + path.sense + "|"
         + path.end_pin;
}

using PathPair = std::pair<const AnalysisPathState*, const AnalysisPathState*>;

// One path table: the path's summary line, its notes and one row per pin
// with the before and after values side by side.
void writePathPair(std::ostream& output,
                   const PathPair& pair,
                   const std::set<std::string>& target_pins,
                   const std::string& target_instance)
{
  const AnalysisPathState* before = pair.first;
  const AnalysisPathState* after = pair.second;
  const AnalysisPathState& any = before != nullptr ? *before : *after;
  output << "\n##### "
         << (any.kind == "target_prefix" ? "Target prefix / " : "")
         << markdown(any.scene) << " / " << any.rf << "/" << any.sense
         << "\n\n";
  if (before == nullptr || after == nullptr) {
    output << "`" << markdown(any.startpoint) << "` -> `"
           << markdown(any.end_pin) << "` ("
           << (before != nullptr ? "before only" : "after only") << "), slack "
           << scaled(any.slack, 1e12) << " ps; required "
           << scaled(any.required, 1e12) << " ps.\n";
  } else {
    if (before->startpoint == after->startpoint
        && before->end_pin == after->end_pin) {
      output << "`" << markdown(any.startpoint) << "` -> `"
             << markdown(any.end_pin) << "`";
    } else {
      output << "`" << markdown(before->startpoint) << "` -> `"
             << markdown(before->end_pin) << "` (before), `"
             << markdown(after->startpoint) << "` -> `"
             << markdown(after->end_pin) << "` (after)";
    }
    output << ", slack " << transition(before->slack, after->slack, 1e12)
           << " ps; required "
           << transition(before->required, after->required, 1e12) << " ps.\n";
  }
  if (any.kind == "target_prefix") {
    output << "This is a prefix ending at the target, shown because the "
              "selected endpoint datapath misses the target.\n";
  }
  if (any.kind == "selected_endpoint_datapath") {
    output << "Current worst path to the selected endpoint; not forced "
              "through the target. The move target `"
           << markdown(target_instance) << "` is "
           << targetPlacement(before, after, target_pins) << ".\n";
  }
  output << "\n| Pin | Master | Net | RF / role | Arrival ps | Slew ps | "
            "Incoming arc delay ps |\n|---|---|---|---|---:|---:|---:|\n";

  // The rows follow the after path; a before pin the after path dropped is
  // printed where it stood, with N/A on the after side.
  const std::vector<AnalysisPathPoint> no_points;
  const std::vector<AnalysisPathPoint>& before_points
      = before != nullptr ? before->points : no_points;
  const std::vector<AnalysisPathPoint>& after_points
      = after != nullptr ? after->points : no_points;
  const AnalysisPathPoint missing{.master = "N/A"};
  const std::function<void(const AnalysisPathPoint&, const AnalysisPathPoint&)>
      write_row
      = [&](const AnalysisPathPoint& lhs, const AnalysisPathPoint& rhs) {
          const AnalysisPathPoint& present = lhs.pin.empty() ? rhs : lhs;
          const bool target = target_pins.contains(present.pin);
          output << "| " << (target ? "**`" : "`") << markdown(present.pin)
                 << (target ? "`** (target)" : "`") << " | `"
                 << markdown(transition(lhs.master, rhs.master)) << "` | `"
                 << markdown(present.net) << "` | " << present.rf << " / "
                 << present.role << " | "
                 << transition(lhs.arrival, rhs.arrival, 1e12) << " | "
                 << transition(lhs.slew, rhs.slew, 1e12) << " | "
                 << transition(lhs.delay, rhs.delay, 1e12) << " |\n";
        };
  size_t next_before = 0;
  for (const AnalysisPathPoint& point : after_points) {
    size_t match = next_before;
    while (match < before_points.size()
           && before_points[match].pin != point.pin) {
      ++match;
    }
    if (match < before_points.size()) {
      for (; next_before < match; ++next_before) {
        write_row(before_points[next_before], missing);
      }
      write_row(before_points[next_before], point);
      ++next_before;
    } else {
      write_row(missing, point);
    }
  }
  for (; next_before < before_points.size(); ++next_before) {
    write_row(before_points[next_before], missing);
  }
}

// Every before path paired with its after path by key; after-only paths
// follow.
std::vector<PathPair> pairPaths(const AnalysisSnapshot& before_snapshot,
                                const AnalysisSnapshot& after_snapshot)
{
  std::vector<PathPair> pairs;
  std::vector<bool> after_used(after_snapshot.paths.size(), false);
  for (const AnalysisPathState& before : before_snapshot.paths) {
    const AnalysisPathState* after = nullptr;
    for (size_t index = 0; index < after_snapshot.paths.size(); ++index) {
      if (!after_used[index]
          && pathKey(after_snapshot.paths[index]) == pathKey(before)) {
        after_used[index] = true;
        after = &after_snapshot.paths[index];
        break;
      }
    }
    pairs.emplace_back(&before, after);
  }
  for (size_t index = 0; index < after_snapshot.paths.size(); ++index) {
    if (!after_used[index]) {
      pairs.emplace_back(nullptr, &after_snapshot.paths[index]);
    }
  }

  return pairs;
}

// The paths of a move report by endpoint: the datapaths and clock paths of
// the global worst endpoint and of the selected endpoint.
struct EndpointPaths
{
  std::vector<PathPair> global_data;
  std::vector<PathPair> global_clock;
  std::vector<PathPair> selected_data;
  std::vector<PathPair> selected_clock;
  // The selected endpoint's flop is a global worst endpoint flop, so its
  // clock path shows once, under the global endpoint.
  bool shared_flop{false};
};

// A clock path belongs to the endpoint whose flop it ends on, and a shared
// flop shows once, under the worst endpoint.  The target prefix follows the
// selected datapaths when it is shown.
EndpointPaths groupPathsByEndpoint(const std::vector<PathPair>& pairs,
                                   const AnalysisSnapshot& before_snapshot,
                                   const AnalysisSnapshot& after_snapshot,
                                   const std::string& selected_endpoint,
                                   const bool show_target_prefix)
{
  std::map<std::string, std::string> pin_instances;
  for (const AnalysisSnapshot* snapshot : {&before_snapshot, &after_snapshot}) {
    for (const AnalysisPinState& state : snapshot->pins) {
      pin_instances.emplace(state.pin, state.instance);
    }
  }
  const std::function<std::string(const std::string&)> instance_of
      = [&](const std::string& pin) {
          const std::map<std::string, std::string>::const_iterator found
              = pin_instances.find(pin);
          return found != pin_instances.end() ? found->second : std::string();
        };

  EndpointPaths paths;
  std::set<std::string> global_instances;
  for (const PathPair& pair : pairs) {
    const AnalysisPathState& any
        = pair.first != nullptr ? *pair.first : *pair.second;
    if (any.kind == "global_endpoint_path") {
      paths.global_data.push_back(pair);
      for (const AnalysisPathState* side : {pair.first, pair.second}) {
        if (side != nullptr) {
          global_instances.insert(instance_of(side->end_pin));
        }
      }
    }
  }

  const std::string selected_instance = instance_of(selected_endpoint);
  paths.shared_flop = !selected_instance.empty()
                      && global_instances.contains(selected_instance);
  std::vector<PathPair> selected_prefix;
  for (const PathPair& pair : pairs) {
    const AnalysisPathState& any
        = pair.first != nullptr ? *pair.first : *pair.second;
    if (any.kind == "selected_endpoint_datapath") {
      paths.selected_data.push_back(pair);
    } else if (any.kind == "target_prefix" && show_target_prefix) {
      selected_prefix.push_back(pair);
    } else if (any.kind == "endpoint_clockpath") {
      const std::string instance = instance_of(any.end_pin);
      const bool selected = !selected_instance.empty()
                            && instance == selected_instance
                            && !paths.shared_flop;
      (selected ? paths.selected_clock : paths.global_clock).push_back(pair);
    }
  }
  // The prefix to the target follows the datapaths it stands in for.
  paths.selected_data.insert(paths.selected_data.end(),
                             selected_prefix.begin(),
                             selected_prefix.end());
  return paths;
}

// One leaf of an endpoint section: a title, an optional lede and its paths.
void writeLeaf(std::ostream& output,
               const char* title,
               const std::string& lede,
               const std::vector<PathPair>& leaf,
               const std::set<std::string>& target_pins,
               const std::string& target_instance)
{
  output << "\n#### " << title << "\n";
  if (!lede.empty()) {
    output << "\n" << lede << "\n";
  }
  if (leaf.empty()) {
    output << "\nNone captured.\n";
  }
  for (const PathPair& pair : leaf) {
    writePathPair(output, pair, target_pins, target_instance);
  }
}

// One endpoint section: its heading, its datapaths and its clock paths.  A
// null clock means the clock path shows under the global worst endpoint.
void writeEndpointSection(std::ostream& output,
                          const std::string& heading,
                          const char* lede,
                          const std::vector<PathPair>& data,
                          const std::vector<PathPair>* clock,
                          const std::set<std::string>& target_pins,
                          const std::string& target_instance)
{
  const std::string clock_lede
      = "Clock source to the endpoint flop's clock pin. A setup check reads "
        "the early (min) arrival of the capture edge; the slack and required "
        "here belong to the paths the flop launches, back-propagated through "
        "clock-to-Q, not to its D check.";
  output << heading << "\n\n" << lede << "\n";
  writeLeaf(output, "Datapath", "", data, target_pins, target_instance);
  if (clock == nullptr) {
    output << "\n#### Clockpath\n\nSame flop as the global worst endpoint; "
              "its clockpath is above.\n";
  } else {
    writeLeaf(
        output, "Clockpath", clock_lede, *clock, target_pins, target_instance);
  }
}

}  // namespace

RepairSetupAnalysisReport::RepairSetupAnalysisReport(
    Resizer& resizer,
    const std::string& directory,
    const std::chrono::steady_clock::time_point start)
    : resizer_(resizer), directory_(directory + "/analysis"), start_(start)
{
  // The profiler reports a directory or file that cannot be opened.
  std::error_code error;
  std::filesystem::create_directories(directory_ + "/moves", error);
  if (error) {
    return;
  }

  moves_.open(directory_ + "/analysis_moves.csv");
  pins_.open(directory_ + "/analysis_pins.csv");
  arcs_.open(directory_ + "/analysis_arcs.csv");
  paths_.open(directory_ + "/analysis_paths.csv");
  connections_.open(directory_ + "/analysis_connections.csv");
  windows_.open(directory_ + "/selected_windows.csv");
  if (!opened()) {
    return;
  }

  moves_ << "move_id,candidate_id,phase,iteration,move,target,selected_"
            "endpoint,violated_endpoint,selected_input,selected_rf,"
            "before_master,after_master,applied,findings,before_elapsed_s,"
            "after_elapsed_s,raw_gain_s,weighted_score,"
            "wns_before_s,wns_after_s,wns_delta_s,tns_before_s,tns_after_s,tns_"
            "delta_s,"
            "target_before_s,target_after_s,actual_window_gain_s,estimate_"
            "error_s,case_report,selected_path_end,path_context\n";
  pins_ << "move_id,point,pin,instance,master,net,role,scene,rf,startpoint,"
           "launch_clock,arrival_s,"
           "required_s,slack_s,slew_s,input_cap_f,pin_cap_f,wire_cap_f,fanout,"
           "drive_resistance_ohm,"
           "drive_resistance_max_ohm,analysis,is_clock\n";
  arcs_ << "move_id,point,from_pin,to_pin,master,scene,from_rf,to_rf,role,"
           "delay_s,condition,mode_name,mode_value,default_condition\n";
  paths_ << "move_id,point,path_index,kind,scene,rf,startpoint,end_pin,launch_"
            "clock,slack_s,step,pin,"
            "master,net,step_rf,role,arrival_s,slew_s,delay_s,analysis,path_"
            "required_s\n";
  connections_ << "move_id,point,net,pin,instance,direction\n";
  windows_ << "move_id,stage,target,driver,input,master,scene,from_rf,to_rf,"
              "input_slew_s,load_cap_f,"
              "model_delay_s,cell_delay_s,wire_delay_s,output_slew_s,role,"
              "condition,mode_name,mode_value\n";
}

bool RepairSetupAnalysisReport::opened() const
{
  return moves_.is_open() && pins_.is_open() && arcs_.is_open()
         && paths_.is_open() && connections_.is_open() && windows_.is_open();
}

double RepairSetupAnalysisReport::elapsed() const
{
  return std::chrono::duration<double>(std::chrono::steady_clock::now()
                                       - start_)
      .count();
}

void RepairSetupAnalysisReport::beginMove(const Context& context,
                                          RepairSetupMoveSampler& observer)
{
  const ProfileScope scope("analysis_observation");
  edits_.clear();
  delta_.clear();
  before_ = observer.snapshot();
  before_time_ = elapsed();
  active_ = &entries_
                 .insert_or_assign(context.move_id,
                                   Entry{context,
                                         observer.targetPin(),
                                         before_.target_master,
                                         "",
                                         kTentativeOutcome,
                                         {},
                                         {},
                                         {},
                                         false})
                 .first->second;
  dumpSnapshot(before_, "before");
  dumpWindow(observer.selectedWindow());
}

void RepairSetupAnalysisReport::dumpWindow(
    const std::vector<AnalysisWindowStage>& window)
{
  const std::string id = std::to_string(active_->context.move_id);
  size_t index = 0;
  for (const AnalysisWindowStage& stage : window) {
    row(windows_,
        {id,
         std::to_string(index++),
         stage.target ? "1" : "0",
         stage.driver,
         stage.input,
         stage.master,
         stage.scene,
         stage.from_rf,
         stage.to_rf,
         number(stage.input_slew),
         number(stage.load_cap),
         number(stage.model_delay),
         number(stage.cell_delay),
         number(stage.wire_delay),
         number(stage.output_slew),
         stage.role,
         stage.condition,
         stage.mode_name,
         stage.mode_value});
  }
}

void RepairSetupAnalysisReport::endMove(const MoveResult& result,
                                        RepairSetupMoveSampler& observer)
{
  const ProfileScope scope("analysis_observation");
  Entry& entry = *active_;
  after_ = observer.snapshot();
  after_pins_.clear();
  for (const AnalysisPinState& pin : after_.pins) {
    after_pins_.emplace(pin.key(), &pin);
  }

  const double after_time = elapsed();
  dumpSnapshot(after_, "after");
  measureDeltas(entry.context, result.accepted);

  entry.after_master = after_.target_master;
  entry.applied = result.accepted;
  if (!result.accepted) {
    entry.outcome = "apply_failed";
  }

  entry.wns_delta
      = difference(metric(before_, "wns_s"), metric(after_, "wns_s"));
  entry.tns_delta
      = difference(metric(before_, "tns_s"), metric(after_, "tns_s"));
  classify(entry, observer);
  writeMoveRow(entry, result.accepted, observer, after_time);
  if (!entry.findings.empty()) {
    reportFlaggedMove(entry, observer);
  }

  moves_.flush();
}

void RepairSetupAnalysisReport::measureDeltas(const Context& context,
                                              const bool accepted)
{
  for (const std::pair<const std::string, double>& value : after_.metrics) {
    const std::map<std::string, double>::const_iterator before
        = before_.metrics.find(value.first);
    if (before != before_.metrics.end()) {
      delta_[value.first] = value.second - before->second;
    }
  }

  if (accepted
      && (context.type == MoveType::kSizeUp
          || context.type == MoveType::kVtSwap)
      && before_.metrics.contains("window_delay_s")
      && after_.metrics.contains("window_delay_s")) {
    const double gain = before_.metrics.at("window_delay_s")
                        - after_.metrics.at("window_delay_s");
    delta_["actual_window_gain_s"] = gain;
    if (context.estimate.has_value()
        && context.estimate->raw_gain.has_value()) {
      delta_["estimate_error_s"] = gain - *context.estimate->raw_gain;
    }
  }
}

void RepairSetupAnalysisReport::writeMoveRow(
    const Entry& entry,
    const bool accepted,
    const RepairSetupMoveSampler& observer,
    const double after_time)
{
  const Context& context = entry.context;
  const std::string report
      = entry.findings.empty() ? "" : "moves/" + caseName(context.move_id);
  const std::optional<double> raw
      = context.estimate.has_value() && context.estimate->raw_gain.has_value()
            ? std::optional<double>(*context.estimate->raw_gain)
            : std::nullopt;
  const std::optional<double> window
      = delta_.contains("actual_window_gain_s")
            ? std::optional<double>(delta_.at("actual_window_gain_s"))
            : std::nullopt;
  const std::optional<double> error
      = delta_.contains("estimate_error_s")
            ? std::optional<double>(delta_.at("estimate_error_s"))
            : std::nullopt;
  row(moves_,
      {std::to_string(context.move_id),
       std::to_string(context.candidate_id),
       context.phase,
       std::to_string(context.iteration),
       moveName(context.type),
       observer.targetPin(),
       observer.selectedEndpoint(),
       observer.violatedEndpoint(),
       observer.selectedInput(),
       observer.selectedRf(),
       before_.target_master,
       after_.target_master,
       accepted ? "1" : "0",
       findingsText(entry.findings),
       number(before_time_),
       number(after_time),
       number(raw),
       context.estimate.has_value() ? number(context.estimate->score) : "",
       number(metric(before_, "wns_s")),
       number(metric(after_, "wns_s")),
       number(entry.wns_delta),
       number(metric(before_, "tns_s")),
       number(metric(after_, "tns_s")),
       number(entry.tns_delta),
       number(metric(before_, "target_slack_s")),
       number(metric(after_, "target_slack_s")),
       number(window),
       number(error),
       report,
       observer.selectedPathEnd(),
       observer.pathContext()});
}

void RepairSetupAnalysisReport::reportFlaggedMove(
    const Entry& entry,
    const RepairSetupMoveSampler& observer)
{
  RepairSetupProfiler::event("analysis_bad_moves");
  // The report leads with the run-local outcome, so a move still inside an
  // open journal waits until the journal settles it.
  CaseReport report = renderCase(entry, observer);
  if (entry.outcome == kTentativeOutcome) {
    pending_cases_.insert_or_assign(entry.context.move_id, std::move(report));
  } else {
    writeCase(entry, report);
  }
}

void RepairSetupAnalysisReport::edit(const std::string& event,
                                     const std::string& object,
                                     const std::string& before,
                                     const std::string& after)
{
  edits_.push_back({event, object, before, after});
}

void RepairSetupAnalysisReport::settle(const uint64_t move_id,
                                       const bool reverted)
{
  const std::map<uint64_t, Entry>::iterator found = entries_.find(move_id);
  if (found == entries_.end()) {
    return;
  }

  found->second.outcome = reverted ? "reverted" : "checkpoint_accepted";
  // A flagged report waiting for this outcome is written now.
  const std::map<uint64_t, CaseReport>::iterator pending
      = pending_cases_.find(move_id);
  if (pending != pending_cases_.end()) {
    writeCase(found->second, pending->second);
    pending_cases_.erase(pending);
  }
}

void RepairSetupAnalysisReport::dumpSnapshot(const AnalysisSnapshot& snapshot,
                                             const std::string& point)
{
  const std::string id = std::to_string(active_->context.move_id);
  for (const AnalysisPinState& pin : snapshot.pins) {
    row(pins_,
        {id,
         point,
         pin.pin,
         pin.instance,
         pin.master,
         pin.net,
         pin.role,
         pin.scene,
         pin.rf,
         pin.startpoint,
         pin.launch_clock,
         number(pin.arrival),
         number(pin.required),
         number(pin.slack),
         number(pin.slew),
         number(pin.input_cap),
         number(pin.pin_cap),
         number(pin.wire_cap),
         number(pin.fanout),
         number(pin.drive_resistance),
         number(pin.drive_resistance_max),
         pin.sense,
         pin.clock ? "1" : "0"});
  }

  for (const AnalysisArcState& arc : snapshot.arcs) {
    row(arcs_,
        {id,
         point,
         arc.from,
         arc.to,
         arc.master,
         arc.scene,
         arc.from_rf,
         arc.to_rf,
         arc.role,
         number(arc.delay),
         arc.condition,
         arc.mode_name,
         arc.mode_value,
         arc.default_condition ? "1" : "0"});
  }

  for (const AnalysisConnection& connection : snapshot.connections) {
    row(connections_,
        {id,
         point,
         connection.net,
         connection.pin,
         connection.instance,
         connection.direction});
  }

  for (size_t index = 0; index < snapshot.paths.size(); ++index) {
    const AnalysisPathState& path = snapshot.paths[index];
    for (size_t step = 0; step < path.points.size(); ++step) {
      const AnalysisPathPoint& pin = path.points[step];
      row(paths_,
          {id,
           point,
           std::to_string(index),
           path.kind,
           path.scene,
           path.rf,
           path.startpoint,
           path.end_pin,
           path.launch_clock,
           number(path.slack),
           std::to_string(step),
           pin.pin,
           pin.master,
           pin.net,
           pin.rf,
           pin.role,
           number(pin.arrival),
           number(pin.slew),
           number(pin.delay),
           path.sense,
           number(path.required)});
    }
  }
}

void RepairSetupAnalysisReport::classify(
    Entry& entry,
    const RepairSetupMoveSampler& observer) const
{
  if (entry.wns_delta.has_value() && *entry.wns_delta < -kNoise) {
    entry.findings.push_back("global_wns_loss");
  }

  if (entry.tns_delta.has_value() && *entry.tns_delta < -kNoise) {
    entry.findings.push_back("global_tns_loss");
  }

  bool rf_loss = false;
  bool side_loss = false;
  bool shared_input_loss = false;
  const std::set<std::string> growing_input_nets = growingInputNets();
  std::map<std::tuple<std::string, std::string, bool>, std::pair<int, int>>
      rf_changes;

  for (const AnalysisPinState& before : before_.pins) {
    const AnalysisPinState* after = findPin(after_pins_, before);
    if (after == nullptr) {
      continue;
    }

    if (before.pin == observer.targetPin()) {
      rf_loss |= worsened(before.slack, after->slack);
      std::pair<int, int>& changes
          = rf_changes[{before.scene, before.sense, before.clock}];
      const int bit = before.rf == "rise" ? 1 : 2;
      if (worsened(before.slack, after->slack)) {
        changes.first |= bit;
      }

      if (worsened(after->slack, before.slack)) {
        changes.second |= bit;
      }
    }

    if (before.role.find("sibling_output") != std::string::npos
        && worsened(before.slack, after->slack)) {
      side_loss = true;
      for (const AnalysisConnection& connection : before_.connections) {
        if (connection.direction == "load"
            && connection.instance == before.instance
            && growing_input_nets.contains(connection.net)) {
          shared_input_loss = true;
        }
      }
    }
  }

  if (rf_loss) {
    bool tradeoff = false;
    for (const std::pair<const std::tuple<std::string, std::string, bool>,
                         std::pair<int, int>>& item : rf_changes) {
      tradeoff |= ((item.second.first & 1) && (item.second.second & 2))
                  || ((item.second.first & 2) && (item.second.second & 1));
    }

    entry.findings.push_back(tradeoff ? "target_rf_tradeoff"
                                      : "target_rf_slack_loss");
  }

  if (side_loss) {
    entry.findings.push_back(shared_input_loss
                                 ? "input_cap_growth_with_side_loss"
                                 : "local_side_slack_loss");
  }

  const std::optional<double> target_delta = difference(
      metric(before_, "target_slack_s"), metric(after_, "target_slack_s"));
  if (target_delta.has_value() && *target_delta > kNoise
      && entry.wns_delta.has_value() && *entry.wns_delta < -kNoise) {
    entry.findings.push_back("target_improves_global_worsens");
  }

  if (!entry.applied && !entry.findings.empty()) {
    entry.findings.push_back("failed_apply_timing_change");
  }
}

void RepairSetupAnalysisReport::writeOverview(
    std::ostream& output,
    const Entry& entry,
    const RepairSetupMoveSampler& observer) const
{
  const std::optional<double> raw_gain
      = entry.context.estimate.has_value()
                && entry.context.estimate->raw_gain.has_value()
            ? std::optional<double>(*entry.context.estimate->raw_gain)
            : std::nullopt;
  output << "# Move " << entry.context.move_id
         << ": observed timing regression\n\n"
         << "- Phase / iteration: `" << markdown(entry.context.phase) << "` / "
         << entry.context.iteration << "\n"
         << "- Move: `" << moveName(entry.context.type)
         << "`; applied: " << (entry.applied ? "yes" : "no") << "\n"
         << "- Target: `" << markdown(entry.target) << "`\n"
         << "- Master: `" << markdown(entry.before_master) << "` -> `"
         << markdown(entry.after_master) << "`\n"
         << "- Selected endpoint: `" << markdown(observer.selectedEndpoint())
         << "`\n"
         << "- Violated endpoint: `" << markdown(observer.violatedEndpoint())
         << "`\n"
         << "- Source path end / context: `"
         << markdown(observer.selectedPathEnd()) << "` / `"
         << observer.pathContext() << "`\n"
         << "- Selected input pin / output RF: `"
         << markdown(observer.selectedInput()) << "` / `"
         << observer.selectedRf() << "`\n"
         << "- Local estimated raw gain: " << scaled(raw_gain, 1e12) << " ps\n"
         << "- Evidence flags: `" << findingsText(entry.findings) << "`\n\n"
         << "These flags describe measured effects, not a proof of a single "
            "mechanism. A local regression may be traded for other gains.\n"
         << "Metadata/window is preserved at selection time; before/after "
            "observations surround this actual apply.\n"
         << "Raw gain is a local move estimate, not global WNS gain. Sizing "
            "and VtSwap use a selected CELL window; buffering may include "
            "wire effects. "
            "Missing values are N/A.\n\n"
         << "| Metric | Before | After | Delta |\n|---|---:|---:|---:|\n";
  for (const std::pair<std::string, double>& item :
       {std::pair<std::string, double>{"wns_s", 1e12},
        {"tns_s", 1e9},
        {"target_slack_s", 1e12}}) {
    const std::optional<double> before = metric(before_, item.first);
    const std::optional<double> after = metric(after_, item.first);
    output << "| " << item.first << (item.first == "tns_s" ? " (ns)" : " (ps)")
           << " | " << scaled(before, item.second) << " | "
           << scaled(after, item.second) << " | "
           << scaled(difference(before, after), item.second) << " |\n";
  }
}

void RepairSetupAnalysisReport::writeInterpretation(std::ostream& output,
                                                    const Entry& entry) const
{
  // The notes on the findings; the global worst path is in the Paths section.
  std::vector<const char*> notes;
  if (std::ranges::find(entry.findings, "target_rf_tradeoff")
      != entry.findings.end()) {
    notes.push_back(
        "Within one scene/analysis context, one target RF improves while the "
        "other loses slack. A positive local score does not protect both "
        "transitions.");
  }

  if (std::ranges::find(entry.findings, "target_improves_global_worsens")
      != entry.findings.end()) {
    notes.push_back(
        "Target worst slack improves while global WNS worsens. Target-only "
        "acceptance would miss this regression.");
  }

  if (std::ranges::find(entry.findings, "input_cap_growth_with_side_loss")
      != entry.findings.end()) {
    notes.push_back(
        "A target input cap increases on a net feeding a degrading sibling. "
        "Connectivity and pin/wire load measurements preserve the "
        "shared-fanin evidence; independent mechanism isolation is not "
        "claimed.");
  }

  if (notes.empty()) {
    return;
  }

  output << "\n## Interpretation\n\n";
  for (const char* note : notes) {
    output << "- " << note << "\n";
  }
}

void RepairSetupAnalysisReport::writeTargetTiming(std::ostream& output,
                                                  const Entry& entry) const
{
  output << "\n## Target RF timing\n\nArrival/required/slack in each row "
            "belong to the same worst-slack context for that scene/RF. Values "
            "read before -> after; a value alone did not change.\n\n"
         << "| Scene / RF / analysis | Arrival ps | Required ps | Slack ps | "
            "Slew ps |\n"
         << "|---|---:|---:|---:|---:|\n";
  for (const AnalysisPinState& before : before_.pins) {
    if (before.pin != entry.target) {
      continue;
    }

    const AnalysisPinState* after = findPin(after_pins_, before);
    const AnalysisPinState missing{};
    const AnalysisPinState& after_state = after != nullptr ? *after : missing;
    output << "| " << markdown(before.scene) << " / " << before.rf << "/"
           << before.sense << (before.clock ? "/clock" : "") << " | "
           << transition(before.arrival, after_state.arrival, 1e12) << " | "
           << transition(before.required, after_state.required, 1e12) << " | "
           << transition(before.slack, after_state.slack, 1e12) << " | "
           << transition(before.slew, after_state.slew, 1e12) << " |\n";
  }
}

void RepairSetupAnalysisReport::writeElectrical(std::ostream& output,
                                                const Entry& entry) const
{
  output << "\n## Local electrical changes\n\nScope: target input/output nets, "
            "their drivers and direct sibling outputs; not a complete "
            "transitive cone. Values read before -> after; a value alone did "
            "not change.\n\n"
         << "| Pin / scene / RF | Net | Role | Cin fF | Pin load fF | Wire "
            "load fF | Slew delta ps | Slack delta ps |\n"
         << "|---|---|---|---:|---:|---:|---:|---:|\n";
  for (const AnalysisPinState& before : before_.pins) {
    const AnalysisPinState* after = findPin(after_pins_, before);
    if (after == nullptr || before.role == "global_path") {
      continue;
    }

    const bool cap_changed = before.input_cap != after->input_cap
                             || before.pin_cap != after->pin_cap
                             || before.wire_cap != after->wire_cap;
    if (!cap_changed && !worsened(before.slack, after->slack)
        && before.pin != entry.target) {
      continue;
    }

    output << "| `" << markdown(before.pin) << "` / " << markdown(before.scene)
           << " / " << before.rf << "/" << before.sense << " | `"
           << markdown(before.net) << "` | " << markdown(before.role) << " | "
           << transition(before.input_cap, after->input_cap, 1e15) << " | "
           << transition(before.pin_cap, after->pin_cap, 1e15) << " | "
           << transition(before.wire_cap, after->wire_cap, 1e15) << " | "
           << scaled(difference(before.slew, after->slew), 1e12) << " | "
           << scaled(difference(before.slack, after->slack), 1e12) << " |\n";
  }
}

std::set<std::string> RepairSetupAnalysisReport::growingInputNets() const
{
  // A target input moved to another net did not grow the load of either net.
  std::set<std::string> nets;
  for (const AnalysisPinState& before : before_.pins) {
    const AnalysisPinState* after = findPin(after_pins_, before);
    if (after != nullptr && before.net == after->net
        && before.role.find("target_input") != std::string::npos
        && before.input_cap.has_value() && after->input_cap.has_value()
        && *after->input_cap > *before.input_cap + kCapNoise) {
      nets.insert(before.net);
    }
  }
  return nets;
}

void RepairSetupAnalysisReport::writeConnections(std::ostream& output) const
{
  const std::set<std::string> growing_nets = growingInputNets();

  output << "\n## Input nets with increased pin capacitance\n\n"
         << "All direct connections are shown, including sibling input pins. "
            "Cap is the Liberty Cin of a load and the total pin load at the "
            "driver, the larger of rise and fall, before -> after; a value "
            "alone did not change.\n\n"
         << "| Net | Pin | Instance | Direction | Cap fF "
            "|\n|---|---|---|---|---:|\n";

  // One row per connected pin: before and after are merged, and a pin the
  // move connected or disconnected says so.
  std::map<std::pair<std::string, std::string>,
           std::pair<const AnalysisConnection*, const AnalysisConnection*>>
      rows;
  for (const AnalysisConnection& connection : before_.connections) {
    if (growing_nets.contains(connection.net)) {
      rows[{connection.net, connection.pin}].first = &connection;
    }
  }
  for (const AnalysisConnection& connection : after_.connections) {
    if (growing_nets.contains(connection.net)) {
      rows[{connection.net, connection.pin}].second = &connection;
    }
  }

  for (const std::pair<const std::pair<std::string, std::string>,
                       std::pair<const AnalysisConnection*,
                                 const AnalysisConnection*>>& row : rows) {
    const AnalysisConnection* before = row.second.first;
    const AnalysisConnection* after = row.second.second;
    const AnalysisConnection& any = before != nullptr ? *before : *after;
    const bool driver = any.direction == "driver";
    const std::optional<double> before_cap
        = before != nullptr ? connectionCap(before_, any.pin, driver)
                            : std::nullopt;
    const std::optional<double> after_cap
        = after != nullptr ? connectionCap(after_, any.pin, driver)
                           : std::nullopt;
    output << "| `" << markdown(any.net) << "` | `" << markdown(any.pin)
           << "` | `" << markdown(any.instance) << "` | " << any.direction
           << (before == nullptr  ? ", added"
               : after == nullptr ? ", removed"
                                  : "")
           << " | " << transition(before_cap, after_cap, 1e15) << " |\n";
  }
}

void RepairSetupAnalysisReport::writeArcs(std::ostream& output) const
{
  output << "\n## Observed cell arc changes\n\nMax-analysis graph-cache "
            "values; not every arc is on a sensitized critical path. Only "
            "exact matching arc families "
            "are differenced; RF or path changes are not silently mixed.\n\n"
         << "| From -> to | Scene / RF | Before ps | After ps | Delta ps "
            "|\n|---|---|---:|---:|---:|\n";

  using ArcKey = std::
      tuple<std::string, std::string, std::string, std::string, std::string>;
  std::map<ArcKey, std::vector<const AnalysisArcState*>> after_arcs;
  for (const AnalysisArcState& arc : after_.arcs) {
    after_arcs[{arc.from, arc.to, arc.scene, arc.from_rf, arc.to_rf}].push_back(
        &arc);
  }

  for (const AnalysisArcState& before : before_.arcs) {
    const std::map<ArcKey, std::vector<const AnalysisArcState*>>::const_iterator
        found = after_arcs.find({before.from,
                                 before.to,
                                 before.scene,
                                 before.from_rf,
                                 before.to_rf});
    if (found == after_arcs.end()) {
      continue;
    }

    for (const AnalysisArcState* observed : found->second) {
      const AnalysisArcState& after = *observed;
      if (before.from == after.from && before.to == after.to
          && before.scene == after.scene && before.from_rf == after.from_rf
          && before.to_rf == after.to_rf
          && matchTimingArc(before.arc, after.arc, ArcMatchMode::kExact)
                 == ArcMatchType::kExact
          && before.delay != after.delay) {
        output << "| `" << markdown(before.from) << "` -> `"
               << markdown(before.to) << "` | " << markdown(before.scene)
               << " / " << before.from_rf << "->" << before.to_rf << " | "
               << scaled(before.delay, 1e12) << " | "
               << scaled(after.delay, 1e12) << " | "
               << scaled(difference(before.delay, after.delay), 1e12) << " |\n";
      }
    }
  }
}

void RepairSetupAnalysisReport::writePaths(
    std::ostream& output,
    const std::string& selected_endpoint) const
{
  // Mark the target's rows.  Its prefix path shows only when the selected
  // endpoint datapath misses the target, because it is part of it otherwise.
  const std::set<std::string> target_pins = targetPins(before_, after_);
  const std::string& target_instance = !before_.target_instance.empty()
                                           ? before_.target_instance
                                           : after_.target_instance;
  const bool show_target_prefix = !datapathPassesTarget(before_, target_pins)
                                  || !datapathPassesTarget(after_, target_pins);
  const EndpointPaths paths = groupPathsByEndpoint(pairPaths(before_, after_),
                                                   before_,
                                                   after_,
                                                   selected_endpoint,
                                                   show_target_prefix);

  output << "\n## Paths (before -> after)\n\nA value alone did not change; "
            "N/A on one side is a pin only that path has. Rows on the move "
            "target `"
         << markdown(target_instance) << "` are bold and marked (target).\n";

  std::string global_heading = "\n### Global worst endpoint";
  if (!paths.global_data.empty()) {
    const AnalysisPathState* before = paths.global_data.front().first;
    const AnalysisPathState* after = paths.global_data.front().second;
    const AnalysisPathState& any = before != nullptr ? *before : *after;
    global_heading += ": `" + markdown(any.end_pin) + "`";
    if (before != nullptr && after != nullptr
        && before->end_pin != after->end_pin) {
      global_heading += " -> `" + markdown(after->end_pin) + "`";
    }
  }
  writeEndpointSection(output,
                       global_heading,
                       "The endpoint of the design's worst path.",
                       paths.global_data,
                       &paths.global_clock,
                       target_pins,
                       target_instance);

  std::string selected_heading = "\n### Selected target endpoint";
  if (!selected_endpoint.empty()) {
    selected_heading += ": `" + markdown(selected_endpoint) + "`";
  }
  writeEndpointSection(
      output,
      selected_heading,
      "The endpoint whose worst path the move target was taken from.",
      paths.selected_data,
      paths.shared_flop ? nullptr : &paths.selected_clock,
      target_pins,
      target_instance);
}

RepairSetupAnalysisReport::CaseReport RepairSetupAnalysisReport::renderCase(
    const Entry& entry,
    const RepairSetupMoveSampler& observer) const
{
  // Everything but the outcome reads the state of this apply, so it is
  // rendered now; the head ends where the outcome goes.
  std::ostringstream head;
  writeOverview(head, entry, observer);
  head << "\n## DB edits\n\n| Event | Object | Before | After "
          "|\n|---|---|---|---|\n";
  for (const Edit& edit : edits_) {
    head << "| " << edit.event << " | `" << markdown(edit.object) << "` | `"
         << markdown(edit.before) << "` | `" << markdown(edit.after) << "` |\n";
  }

  head << "\nFull pin/arc/path/connectivity/selected-window records: parent "
          "analysis CSVs, move_id="
       << entry.context.move_id
       << ". The run-local journal outcome follows and is indexed in "
          "[bad_moves.md](../bad_moves.md).\n";

  std::ostringstream body;
  writeInterpretation(body, entry);
  writeTargetTiming(body, entry);
  writeElectrical(body, entry);
  writeConnections(body);
  writeArcs(body);
  writePaths(body, observer.selectedEndpoint());
  return {.head = head.str(), .body = body.str()};
}

void RepairSetupAnalysisReport::writeCase(const Entry& entry,
                                          const CaseReport& report)
{
  std::ofstream output(directory_ + "/moves/"
                       + caseName(entry.context.move_id));
  output << report.head << "\n## Run-local outcome\n\n"
         << entry.outcome << "\n"
         << report.body;
  output.close();
  if (!output) {
    case_write_failed_ = true;
  }
}

bool RepairSetupAnalysisReport::finish()
{
  // A move whose journal never settled keeps the tentative outcome.
  for (const std::pair<const uint64_t, CaseReport>& pending : pending_cases_) {
    writeCase(entries_.at(pending.first), pending.second);
  }
  pending_cases_.clear();

  std::ofstream summary(directory_ + "/bad_moves.md");
  std::ofstream outcomes(directory_ + "/analysis_outcomes.csv");
  if (!summary || !outcomes) {
    // The profiler destructor calls finish(), so this cannot be an error.
    resizer_.logger()->warn(utl::RSZ,
                            2035,
                            "Cannot write the repair setup analysis summary "
                            "under {}.",
                            directory_);
    return false;
  }

  summary
      << "# Repair setup: observed bad moves\n\n"
      << "Timing-loss threshold: " << kNoise * 1e12
      << " ps; all measured values remain in CSV. Outcomes are run-local "
         "checkpoints, not final instance retention.\n"
      << "No counterfactual run is performed. Flags are observed "
         "regressions/supporting mechanisms; they do not prove the optimizer "
         "should reject every local tradeoff.\n\n"
      << "| Move | Phase / iteration | Type / target | WNS delta ps | TNS "
         "delta ns | Outcome | Evidence |\n"
      << "|---:|---|---|---:|---:|---|---|\n";
  outcomes << "move_id,phase,iteration,move,target,applied,outcome,findings\n";
  size_t bad = 0;
  for (const std::pair<const uint64_t, Entry>& item : entries_) {
    const Entry& entry = item.second;
    row(outcomes,
        {std::to_string(item.first),
         entry.context.phase,
         std::to_string(entry.context.iteration),
         moveName(entry.context.type),
         entry.target,
         entry.applied ? "1" : "0",
         entry.outcome,
         findingsText(entry.findings)});
    if (entry.findings.empty()) {
      continue;
    }
    ++bad;
    summary << "| [" << item.first << "](moves/" << caseName(item.first)
            << ") | " << markdown(entry.context.phase) << " / "
            << entry.context.iteration << " | " << moveName(entry.context.type)
            << " / `" << markdown(entry.target) << "` | "
            << scaled(entry.wns_delta, 1e12) << " | "
            << scaled(entry.tns_delta, 1e9) << " | " << entry.outcome << " | "
            << findingsText(entry.findings) << " |\n";
  }

  summary << "\nObserved moves: " << entries_.size()
          << "; moves with timing-regression flags: " << bad << ".\n";

  pins_.flush();
  arcs_.flush();
  paths_.flush();
  connections_.flush();
  windows_.flush();
  summary.flush();
  outcomes.flush();
  const bool written = moves_ && pins_ && arcs_ && paths_ && connections_
                       && windows_ && summary && outcomes
                       && !case_write_failed_;
  resizer_.logger()->report(
      "Repair setup analysis: {}/{} moves flagged; {} {}/bad_moves.md",
      bad,
      entries_.size(),
      written ? "written" : "write failed:",
      directory_);

  return written;
}

}  // namespace rsz
