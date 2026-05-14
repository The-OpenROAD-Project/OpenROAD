// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026-2026, The OpenROAD Authors

#include "DelayEstimator.hh"

#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <cstddef>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "OptimizerTypes.hh"
#include "db_sta/dbSta.hh"
#include "rsz/Resizer.hh"
#include "sta/ArcDelayCalc.hh"
#include "sta/Delay.hh"
#include "sta/Graph.hh"
#include "sta/GraphDelayCalc.hh"
#include "sta/Liberty.hh"
#include "sta/LibertyClass.hh"
#include "sta/MinMax.hh"
#include "sta/Network.hh"
#include "sta/Parasitics.hh"
#include "sta/Path.hh"
#include "sta/PathExpanded.hh"
#include "sta/Scene.hh"
#include "sta/Sdc.hh"
#include "sta/StaState.hh"
#include "sta/TableModel.hh"
#include "sta/TimingArc.hh"
#include "utl/Logger.h"

namespace rsz {

namespace {

constexpr float kSlewMatchAbsTolerance = 1.0e-15f;
constexpr float kSlewMatchRelTolerance = 1.0e-6f;
constexpr float kMinSlewRatioDenominator = 1.0e-15f;

void setFailReason(FailReason* fail_reason, const FailReason reason)
{
  if (fail_reason != nullptr) {
    *fail_reason = reason;
  }
}

bool gateDelayAndSlewFromTableModel(const sta::Pvt* pvt,
                                    const sta::TimingArc* arc,
                                    const float in_slew,
                                    const float load_cap,
                                    float& delay,
                                    float& output_slew)
{
  const sta::GateTableModel* model
      = dynamic_cast<const sta::GateTableModel*>(arc->model());
  if (model == nullptr) {
    return false;
  }

  output_slew = 0.0f;
  model->gateDelay(pvt, in_slew, load_cap, delay, output_slew);
  return true;
}

const sta::Pvt* findPvt(const sta::Scene* scene,
                        sta::Instance* inst,
                        const sta::MinMax* min_max)
{
  if (scene == nullptr || min_max == nullptr) {
    return nullptr;
  }

  const sta::Pvt* pvt = scene->sdc()->pvt(inst, min_max);
  if (pvt == nullptr) {
    pvt = scene->sdc()->operatingConditions(min_max);
  }
  return pvt;
}

const sta::TimingArc* selectedPathArc(const sta::PathExpanded& expanded,
                                      const int path_index,
                                      const sta::StaState* sta,
                                      FailReason* fail_reason)
{
  const int start_index = static_cast<int>(expanded.startIndex());
  const int path_count = static_cast<int>(expanded.size());
  if (path_index < start_index || path_index >= path_count) {
    setFailReason(fail_reason, FailReason::kPathIndexOutOfRange);
    return nullptr;
  }

  const sta::Path* driver_path = expanded.path(path_index);
  const sta::TimingArc* arc
      = driver_path != nullptr ? driver_path->prevArc(sta) : nullptr;
  if (driver_path == nullptr || arc == nullptr || arc->from() == nullptr
      || arc->to() == nullptr || arc->fromEdge() == nullptr
      || arc->toEdge() == nullptr) {
    setFailReason(fail_reason, FailReason::kMissingPrevTimingArc);
    return nullptr;
  }

  const sta::RiseFall* in_rf = arc->fromEdge()->asRiseFall();
  const sta::RiseFall* out_rf = arc->toEdge()->asRiseFall();
  if (in_rf == nullptr || out_rf == nullptr) {
    setFailReason(fail_reason, FailReason::kMissingArcTransition);
    return nullptr;
  }

  return arc;
}

std::optional<SelectedArc> buildSelectedArc(const Resizer& resizer,
                                            sta::Instance* inst,
                                            sta::Pin* driver_pin,
                                            const sta::PathExpanded& expanded,
                                            const int path_index,
                                            const sta::Scene* scene,
                                            const sta::MinMax* min_max,
                                            FailReason* fail_reason)
{
  assert(driver_pin != nullptr);
  assert(inst != nullptr);

  const sta::MinMax* actual_min_max
      = min_max != nullptr ? min_max : resizer.maxAnalysisMode();
  if (scene == nullptr || actual_min_max == nullptr) {
    setFailReason(fail_reason, FailReason::kMissingSceneOrMinMax);
    return std::nullopt;
  }

  const sta::LibertyPort* output_port
      = resizer.network()->libertyPort(driver_pin);
  if (output_port == nullptr) {
    setFailReason(fail_reason, FailReason::kMissingOutputPort);
    return std::nullopt;
  }

  sta::LibertyCell* current_cell
      = output_port->libertyCell()->sceneCell(scene, actual_min_max);
  if (current_cell == nullptr) {
    setFailReason(fail_reason, FailReason::kMissingCurrentPortMap);
    return std::nullopt;
  }

  const sta::TimingArc* path_arc
      = selectedPathArc(expanded, path_index, resizer.staState(), fail_reason);
  if (path_arc == nullptr) {
    return std::nullopt;
  }

  const sta::LibertyPort* input_port
      = current_cell->findLibertyPort(path_arc->from()->name());
  const sta::LibertyPort* current_output_port
      = current_cell->findLibertyPort(output_port->name());
  if (input_port == nullptr || current_output_port == nullptr
      || path_arc->to()->name() != current_output_port->name()) {
    setFailReason(fail_reason, FailReason::kMissingCurrentPortMap);
    return std::nullopt;
  }

  const sta::TimingArcSetSeq& current_sets = current_cell->timingArcSets(
      const_cast<sta::LibertyPort*>(input_port),
      const_cast<sta::LibertyPort*>(current_output_port));
  const sta::TimingArc* ref_arc = nullptr;
  for (const sta::TimingArcSet* arc_set : current_sets) {
    ref_arc = findMatchingTimingArc(path_arc, arc_set);
    if (ref_arc != nullptr) {
      break;
    }
  }
  if (ref_arc == nullptr) {
    setFailReason(fail_reason, FailReason::kMissingCurrentTimingArc);
    return std::nullopt;
  }

  return SelectedArc{.scene = scene,
                     .min_max = actual_min_max,
                     .pvt = findPvt(scene, inst, actual_min_max),
                     .ref_arc = ref_arc};
}

void warnInputSlewFallback(const Resizer& resizer,
                           sta::Instance* inst,
                           const sta::LibertyPort* input_port,
                           const char* missing_object)
{
  resizer.logger()->warn(
      utl::RSZ,
      3210,
      "Delay estimator could not find {} for input port {} on instance {}; "
      "using 0 input slew.",
      missing_object,
      input_port->name(),
      resizer.network()->pathName(inst));
}

float inputSlewOrZero(const Resizer& resizer,
                      sta::Instance* inst,
                      const SelectedArc& arc)
{
  const sta::Pin* input_pin
      = resizer.network()->findPin(inst, arc.inputPort()->name());
  if (input_pin == nullptr) {
    warnInputSlewFallback(resizer, inst, arc.inputPort(), "input pin");
    return 0.0f;
  }

  const sta::Vertex* input_vertex = resizer.graph()->pinDrvrVertex(input_pin);
  if (input_vertex == nullptr) {
    warnInputSlewFallback(resizer, inst, arc.inputPort(), "input vertex");
    return 0.0f;
  }

  return resizer.staState()->graphDelayCalc()->edgeFromSlew(input_vertex,
                                                            arc.inputRiseFall(),
                                                            arc.ref_arc->role(),
                                                            arc.scene,
                                                            arc.min_max);
}

float pathGraphCellDelayOrModelDelay(const Resizer& resizer,
                                     const sta::PathExpanded& expanded,
                                     const int path_index,
                                     const SelectedArc& arc,
                                     const float model_delay)
{
  const sta::Path* driver_path = expanded.path(path_index);
  sta::Edge* gate_edge = driver_path->prevEdge(resizer.staState());
  if (gate_edge == nullptr) {
    // Path validation guarantees the timing arc; fall back only if the graph
    // edge is unavailable for a boundary stage.
    return model_delay;
  }

  const int dcalc_ap = arc.scene->dcalcAnalysisPtIndex(arc.min_max);
  const sta::TimingArc* gate_arc = driver_path->prevArc(resizer.staState());
  return sta::delayAsFloat(
      resizer.graph()->arcDelay(gate_edge, gate_arc, dcalc_ap));
}

bool findArcPorts(sta::LibertyCell* cell,
                  const sta::TimingArc* ref_arc,
                  sta::LibertyPort*& input_port,
                  sta::LibertyPort*& output_port)
{
  input_port = cell->findLibertyPort(ref_arc->from()->name());
  output_port = cell->findLibertyPort(ref_arc->to()->name());
  return input_port != nullptr && output_port != nullptr;
}

const sta::TimingArcSetSeq& timingArcSetsForRefPorts(
    sta::LibertyCell* cell,
    const sta::TimingArc* ref_arc)
{
  sta::LibertyPort* input_port = nullptr;
  sta::LibertyPort* output_port = nullptr;
  if (!findArcPorts(cell, ref_arc, input_port, output_port)) {
    static const sta::TimingArcSetSeq kEmpty;
    return kEmpty;
  }
  return cell->timingArcSets(input_port, output_port);
}

// `match_relaxed_out` is OR-set when relaxed fallback succeeds (never reset
// to false here) so a single accumulator can chain multiple lookups.
bool lookupArcDelayAndSlewForArc(const sta::Scene* scene,
                                 const sta::MinMax* min_max,
                                 const sta::Pvt* pvt,
                                 const sta::TimingArc* ref_arc,
                                 const float input_slew,
                                 const float load_cap,
                                 const sta::LibertyCell* cell,
                                 float& delay,
                                 float& output_slew,
                                 ArcMatchMode match_mode = ArcMatchMode::kExact,
                                 bool* match_relaxed_out = nullptr)
{
  delay = -sta::INF;
  output_slew = 0.0f;

  if (cell == nullptr) {
    return false;
  }

  sta::LibertyCell* scene_cell
      = const_cast<sta::LibertyCell*>(cell)->sceneCell(scene, min_max);
  if (scene_cell == nullptr) {
    return false;
  }

  const sta::TimingArcSetSeq& arc_sets
      = timingArcSetsForRefPorts(scene_cell, ref_arc);
  for (const sta::TimingArcSet* arc_set : arc_sets) {
    if (arc_set->role()->isTimingCheck()) {
      continue;
    }
    const sta::TimingArc* candidate_arc
        = findMatchingTimingArc(ref_arc, arc_set);
    if (candidate_arc != nullptr) {
      return gateDelayAndSlewFromTableModel(
          pvt, candidate_arc, input_slew, load_cap, delay, output_slew);
    }
  }

  if (match_mode != ArcMatchMode::kRelaxedCandidate) {
    return false;
  }

  // Conservative selection across relaxed candidates: pick the arc with the
  // worst output slew, so downstream stages see the larger slew on the next
  // input.  Reported delay tracks the same arc.
  bool found = false;
  for (const sta::TimingArcSet* arc_set : arc_sets) {
    if (arc_set->role()->isTimingCheck()) {
      continue;
    }
    for (const sta::TimingArc* arc : arc_set->arcs()) {
      if (matchTimingArc(ref_arc, arc, ArcMatchMode::kRelaxedCandidate)
          != ArcMatchType::kRelaxed) {
        continue;
      }
      float candidate_delay = 0.0f;
      float candidate_slew = 0.0f;
      if (!gateDelayAndSlewFromTableModel(pvt,
                                          arc,
                                          input_slew,
                                          load_cap,
                                          candidate_delay,
                                          candidate_slew)) {
        continue;
      }
      if (!found || min_max->compare(candidate_slew, output_slew)) {
        delay = candidate_delay;
        output_slew = candidate_slew;
        found = true;
      }
    }
  }

  if (found && match_relaxed_out != nullptr) {
    *match_relaxed_out = true;
  }
  return found;
}

bool lookupArcDelayAndSlew(const SelectedArc& arc,
                           const float input_slew,
                           const float load_cap,
                           const sta::LibertyCell* cell,
                           float& delay,
                           float& output_slew,
                           ArcMatchMode match_mode = ArcMatchMode::kExact,
                           bool* match_relaxed_out = nullptr)
{
  return lookupArcDelayAndSlewForArc(arc.scene,
                                     arc.min_max,
                                     arc.pvt,
                                     arc.ref_arc,
                                     input_slew,
                                     load_cap,
                                     cell,
                                     delay,
                                     output_slew,
                                     match_mode,
                                     match_relaxed_out);
}

const sta::TimingArc* findCellTimingArcLike(const sta::TimingArc* graph_arc,
                                            sta::LibertyCell* scene_cell)
{
  const sta::TimingArcSetSeq& arc_sets
      = timingArcSetsForRefPorts(scene_cell, graph_arc);
  for (const sta::TimingArcSet* arc_set : arc_sets) {
    if (arc_set->role()->isTimingCheck()) {
      continue;
    }
    const sta::TimingArc* ref_arc = findMatchingTimingArc(graph_arc, arc_set);
    if (ref_arc != nullptr) {
      return ref_arc;
    }
  }
  return nullptr;
}

std::vector<OutputSlewMergeArc> collectOutputSlewMergeArcs(
    const Resizer& resizer,
    const SelectedArc& selected_arc,
    sta::Pin* driver_pin,
    sta::LibertyCell* current_cell,
    const float load_cap)
{
  std::vector<OutputSlewMergeArc> merge_arcs;
  sta::Vertex* driver_vertex = resizer.graph()->pinDrvrVertex(driver_pin);
  if (driver_vertex == nullptr) {
    return merge_arcs;
  }

  const sta::RiseFall* output_rf = selected_arc.outputRiseFall();
  const sta::LibertyPort* output_port = selected_arc.outputPort();

  // Capture non-path gate arcs that can merge into this driver's selected
  // output transition. The selected path arc is handled by the stage lookup;
  // this cache only controls the output slew propagated to the next stage.
  sta::VertexInEdgeIterator edge_iter(driver_vertex, resizer.graph());
  while (edge_iter.hasNext()) {
    sta::Edge* edge = edge_iter.next();
    const sta::TimingArcSet* arc_set = edge->timingArcSet();
    if (arc_set->role()->isTimingCheck()
        || arc_set->to()->name() != output_port->name()) {
      continue;
    }

    for (const sta::TimingArc* graph_arc : arc_set->arcs()) {
      const sta::RiseFall* input_rf = graph_arc->fromEdge()->asRiseFall();
      const sta::RiseFall* arc_output_rf = graph_arc->toEdge()->asRiseFall();
      if (input_rf == nullptr || arc_output_rf != output_rf) {
        continue;
      }

      const sta::TimingArc* ref_arc
          = findCellTimingArcLike(graph_arc, current_cell);
      if (ref_arc == nullptr) {
        continue;
      }
      if (ref_arc == selected_arc.ref_arc) {
        continue;
      }

      const float input_slew
          = resizer.staState()->graphDelayCalc()->edgeFromSlew(
              edge->from(resizer.graph()),
              input_rf,
              edge,
              selected_arc.scene,
              selected_arc.min_max);
      float delay = 0.0f;
      float output_slew = 0.0f;
      if (!gateDelayAndSlewFromTableModel(selected_arc.pvt,
                                          ref_arc,
                                          input_slew,
                                          load_cap,
                                          delay,
                                          output_slew)) {
        continue;
      }

      merge_arcs.push_back({.ref_arc = ref_arc,
                            .input_slew = input_slew,
                            .current_model_slew = output_slew});
    }
  }
  return merge_arcs;
}

struct OutputSlewArc
{
  const sta::TimingArc* ref_arc{nullptr};
  float input_slew{0.0f};
};

class PiModelRestore
{
 public:
  PiModelRestore(sta::Parasitics* parasitics,
                 sta::Parasitic* parasitic,
                 const float c2,
                 const float rpi,
                 const float c1)
      : parasitics_(parasitics),
        parasitic_(parasitic),
        c2_(c2),
        rpi_(rpi),
        c1_(c1)
  {
  }

  ~PiModelRestore() { parasitics_->setPiModel(parasitic_, c2_, rpi_, c1_); }

 private:
  sta::Parasitics* parasitics_;
  sta::Parasitic* parasitic_;
  const float c2_;
  const float rpi_;
  const float c1_;
};

float slewBias(const SlewBiasSample& sample)
{
  return sample.sta_slew - sample.table_slew;
}

float interpolate(const float x0,
                  const float y0,
                  const float x1,
                  const float y1,
                  const float x)
{
  if (x1 <= x0) {
    return y0;
  }
  const float ratio = (x - x0) / (x1 - x0);
  return y0 + ratio * (y1 - y0);
}

float interpolateStaSlewBias(const SlewBiasModel& model, const float load_cap)
{
  const SlewBiasSample& low = model.samples[0];
  const SlewBiasSample& mid = model.samples[1];
  const SlewBiasSample& high = model.samples[2];
  const float clamped_load = std::clamp(load_cap, low.load_cap, high.load_cap);
  if (clamped_load <= mid.load_cap) {
    return interpolate(
        low.load_cap, slewBias(low), mid.load_cap, slewBias(mid), clamped_load);
  }
  return interpolate(
      mid.load_cap, slewBias(mid), high.load_cap, slewBias(high), clamped_load);
}

std::vector<OutputSlewArc> outputSlewArcs(const DelayStageState& stage)
{
  std::vector<OutputSlewArc> arcs;
  arcs.reserve(stage.output_slew_merge_arcs.size() + 1);
  arcs.push_back(
      {.ref_arc = stage.arc.ref_arc, .input_slew = stage.input_slew});
  for (const OutputSlewMergeArc& merge_arc : stage.output_slew_merge_arcs) {
    arcs.push_back(
        {.ref_arc = merge_arc.ref_arc, .input_slew = merge_arc.input_slew});
  }
  return arcs;
}

bool findTableWorstArcAtLoad(const DelayStageState& stage,
                             const float load_cap,
                             OutputSlewArc& worst_arc,
                             float& worst_slew)
{
  sta::LibertyCell* current_cell = stage.arc.currentCell();
  bool found_worst = false;
  worst_slew = 0.0f;

  const std::vector<OutputSlewArc> arcs = outputSlewArcs(stage);
  for (const OutputSlewArc& arc : arcs) {
    float delay = 0.0f;
    float output_slew = 0.0f;
    if (!lookupArcDelayAndSlewForArc(stage.arc.scene,
                                     stage.arc.min_max,
                                     stage.arc.pvt,
                                     arc.ref_arc,
                                     arc.input_slew,
                                     load_cap,
                                     current_cell,
                                     delay,
                                     output_slew)) {
      continue;
    }

    if (!found_worst || stage.arc.min_max->compare(output_slew, worst_slew)) {
      worst_arc = arc;
      worst_slew = output_slew;
      found_worst = true;
    }
  }
  return found_worst;
}

bool sameOutputSlewArc(const OutputSlewArc& lhs, const OutputSlewArc& rhs)
{
  return lhs.ref_arc == rhs.ref_arc && lhs.input_slew == rhs.input_slew;
}

bool slewsMatch(const float lhs, const float rhs)
{
  const float tolerance = std::max(
      kSlewMatchAbsTolerance,
      kSlewMatchRelTolerance * std::max(std::fabs(lhs), std::fabs(rhs)));
  return std::fabs(lhs - rhs) <= tolerance;
}

void logSlewBiasSkip(const Resizer& resizer,
                     const DelayStageState& stage,
                     const char* reason)
{
  const std::string pin_name
      = stage.driver_pin != nullptr
            ? resizer.network()->pathName(stage.driver_pin)
            : "<unknown>";
  debugPrint(resizer.logger(),
             utl::RSZ,
             "delay_estimator",
             2,
             "Skip STA slew-bias model for {}: {}",
             pin_name,
             reason);
}

bool findStableTableWorstArc(const DelayStageState& stage,
                             const std::array<float, 3>& sample_loads,
                             OutputSlewArc& table_worst_arc)
{
  float ignored_slew = 0.0f;
  if (!findTableWorstArcAtLoad(
          stage, sample_loads[0], table_worst_arc, ignored_slew)) {
    return false;
  }

  for (size_t index = 1; index < sample_loads.size(); ++index) {
    OutputSlewArc sample_worst_arc;
    if (!findTableWorstArcAtLoad(
            stage, sample_loads[index], sample_worst_arc, ignored_slew)
        || !sameOutputSlewArc(table_worst_arc, sample_worst_arc)) {
      return false;
    }
  }
  return true;
}

bool syntheticPiForLoad(const float base_c2,
                        const float base_rpi,
                        const float base_c1,
                        const float base_load_cap,
                        const float sample_load_cap,
                        float& sample_c2,
                        float& sample_rpi,
                        float& sample_c1)
{
  const float total_cap = base_c1 + base_c2;
  if (total_cap <= 0.0f || base_load_cap <= 0.0f || sample_load_cap < 0.0f) {
    return false;
  }

  const float c2_fraction = base_c2 / total_cap;
  const float load_delta = sample_load_cap - base_load_cap;
  sample_c2 = base_c2 + load_delta * c2_fraction;
  sample_c1 = base_c1 + load_delta * (1.0f - c2_fraction);
  sample_rpi = base_rpi;
  return sample_c2 >= 0.0f && sample_c1 >= 0.0f && std::isfinite(sample_c2)
         && std::isfinite(sample_rpi) && std::isfinite(sample_c1);
}

bool staDriverSlewForSyntheticPi(const Resizer& resizer,
                                 const DelayStageState& stage,
                                 const OutputSlewArc& table_worst_arc,
                                 sta::Parasitics* parasitics,
                                 sta::Parasitic* parasitic,
                                 const float sample_c2,
                                 const float sample_rpi,
                                 const float sample_c1,
                                 const float sample_load_cap,
                                 float& sta_slew)
{
  // Reuse OpenSTA's active delay calculator on a synthetic Pi sample.  The
  // caller owns restoring the original Pi model before returning.
  parasitics->setPiModel(parasitic, sample_c2, sample_rpi, sample_c1);

  sta::LoadPinIndexMap load_pin_index_map(resizer.staState()->network());
  sta::ArcDcalcResult result = resizer.staState()->arcDelayCalc()->gateDelay(
      stage.driver_pin,
      table_worst_arc.ref_arc,
      table_worst_arc.input_slew,
      sample_load_cap,
      parasitic,
      load_pin_index_map,
      stage.arc.scene,
      stage.arc.min_max);
  resizer.staState()->arcDelayCalc()->finishDrvrPin();
  sta_slew = sta::delayAsFloat(result.drvrSlew());
  return std::isfinite(sta_slew) && sta_slew >= 0.0f;
}

bool fillSlewBiasSample(const Resizer& resizer,
                        const DelayStageState& stage,
                        const OutputSlewArc& table_worst_arc,
                        sta::Parasitics* parasitics,
                        sta::Parasitic* parasitic,
                        const float base_c2,
                        const float base_rpi,
                        const float base_c1,
                        const float sample_load_cap,
                        SlewBiasSample& sample)
{
  float table_delay = 0.0f;
  float table_slew = 0.0f;
  if (!lookupArcDelayAndSlewForArc(stage.arc.scene,
                                   stage.arc.min_max,
                                   stage.arc.pvt,
                                   table_worst_arc.ref_arc,
                                   table_worst_arc.input_slew,
                                   sample_load_cap,
                                   stage.arc.currentCell(),
                                   table_delay,
                                   table_slew)) {
    return false;
  }

  float sample_c2 = 0.0f;
  float sample_rpi = 0.0f;
  float sample_c1 = 0.0f;
  if (!syntheticPiForLoad(base_c2,
                          base_rpi,
                          base_c1,
                          stage.load_cap,
                          sample_load_cap,
                          sample_c2,
                          sample_rpi,
                          sample_c1)) {
    return false;
  }

  float sta_slew = 0.0f;
  if (!staDriverSlewForSyntheticPi(resizer,
                                   stage,
                                   table_worst_arc,
                                   parasitics,
                                   parasitic,
                                   sample_c2,
                                   sample_rpi,
                                   sample_c1,
                                   sample_load_cap,
                                   sta_slew)) {
    return false;
  }

  sample.load_cap = sample_load_cap;
  sample.table_slew = table_slew;
  sample.sta_slew = sta_slew;
  return std::isfinite(sample.table_slew) && std::isfinite(sample.sta_slew);
}

SlewBiasModel buildSlewBiasModel(Resizer& resizer,
                                 const DelayStageState& stage,
                                 const float max_load_delta)
{
  // STA sampling temporarily mutates the live reduced Pi model and calls the
  // active ArcDelayCalc. Call it only during single-threaded prepare, before
  // candidate worker scoring or any concurrent STA update begins.

  SlewBiasModel model;
  if (max_load_delta <= 0.0f || stage.driver_pin == nullptr) {
    logSlewBiasSkip(resizer, stage, "no load delta or driver pin");
    return model;
  }

  const std::array<float, 3> sample_loads
      = {stage.load_cap,
         stage.load_cap + max_load_delta * 0.5f,
         stage.load_cap + max_load_delta};

  OutputSlewArc table_worst_arc;
  if (!findStableTableWorstArc(stage, sample_loads, table_worst_arc)) {
    logSlewBiasSkip(resizer, stage, "unstable table-worst arc");
    return model;
  }

  sta::Parasitics* parasitics = stage.arc.scene->parasitics(stage.arc.min_max);
  sta::ArcDelayCalc* arc_delay_calc = resizer.staState()->arcDelayCalc();
  if (parasitics == nullptr || arc_delay_calc == nullptr) {
    logSlewBiasSkip(resizer, stage, "missing parasitics or delay calc");
    return model;
  }

  sta::Parasitic* parasitic
      = arc_delay_calc->findParasitic(stage.driver_pin,
                                      stage.arc.outputRiseFall(),
                                      stage.arc.scene,
                                      stage.arc.min_max);
  if (parasitic == nullptr || !parasitics->isPiModel(parasitic)) {
    logSlewBiasSkip(resizer, stage, "missing Pi parasitic");
    return model;
  }

  float base_c2 = 0.0f;
  float base_rpi = 0.0f;
  float base_c1 = 0.0f;
  parasitics->piModel(parasitic, base_c2, base_rpi, base_c1);
  PiModelRestore restore(parasitics, parasitic, base_c2, base_rpi, base_c1);

  model.table_worst_arc = table_worst_arc.ref_arc;
  model.input_slew = table_worst_arc.input_slew;
  for (size_t index = 0; index < sample_loads.size(); ++index) {
    if (!fillSlewBiasSample(resizer,
                            stage,
                            table_worst_arc,
                            parasitics,
                            parasitic,
                            base_c2,
                            base_rpi,
                            base_c1,
                            sample_loads[index],
                            model.samples[index])) {
      logSlewBiasSkip(resizer, stage, "invalid STA sample");
      return SlewBiasModel{};
    }
  }

  model.valid = true;
  return model;
}

bool canUseSlewBias(const DelayStageState& stage,
                    const sta::LibertyCell* cell,
                    const float input_slew)
{
  return stage.sta_slew_bias.valid && cell == stage.arc.currentCell()
         && slewsMatch(input_slew, stage.input_slew);
}

std::optional<float> candidateInputCap(const DelayStageState& target_stage,
                                       sta::LibertyCell* candidate_cell)
{
  if (candidate_cell == nullptr) {
    return std::nullopt;
  }

  sta::LibertyCell* scene_cell = candidate_cell->sceneCell(
      target_stage.arc.scene, target_stage.arc.min_max);
  if (scene_cell == nullptr) {
    return std::nullopt;
  }

  sta::LibertyPort* input_port
      = scene_cell->findLibertyPort(target_stage.arc.inputPort()->name());
  if (input_port == nullptr) {
    return std::nullopt;
  }
  return input_port->capacitance();
}

float maxTargetInputCapDelta(Resizer& resizer,
                             const DelayStageState& target_stage)
{
  // Stage arcs use scene/corner cells for timing lookup, while Resizer
  // replacement queries require the canonical link cell mapped to OpenDB.
  sta::LibertyCell* current_scene_cell = target_stage.arc.currentCell();
  sta::LibertyCell* current_link_cell
      = resizer.network()->findLibertyCell(current_scene_cell->name());
  if (current_link_cell == nullptr) {
    debugPrint(resizer.logger(),
               utl::RSZ,
               "delay_estimator",
               3,
               "STA slew-bias cap range missing link cell for {}",
               current_scene_cell->name());
    return 0.0f;
  }

  const float current_input_cap = target_stage.arc.inputPort()->capacitance();
  float max_input_cap = current_input_cap;
  size_t scanned_cell_count = 0;

  auto update_max_cap_from_cell = [&](sta::LibertyCell* cell) {
    scanned_cell_count++;
    const std::optional<float> input_cap
        = candidateInputCap(target_stage, cell);
    if (input_cap.has_value()) {
      max_input_cap = std::max(max_input_cap, *input_cap);
    }
  };

  // Cover the full equivalent-cell envelope beyond today's policy filters, so
  // future composed size/VT moves stay inside the sampled cap range.
  sta::LibertyCellSeq* equiv_cells
      = resizer.sta()->equivCells(current_link_cell);
  if (equiv_cells != nullptr) {
    for (sta::LibertyCell* cell : *equiv_cells) {
      if (resizer.dontUse(cell) || !resizer.isLinkCell(cell)) {
        continue;
      }
      update_max_cap_from_cell(cell);
    }
  }

  debugPrint(resizer.logger(),
             utl::RSZ,
             "delay_estimator",
             3,
             "STA slew-bias cap range for {} input {}: current {:.3e}, max "
             "{:.3e}, scanned {} cells",
             current_link_cell->name(),
             target_stage.arc.inputPort()->name(),
             current_input_cap,
             max_input_cap,
             scanned_cell_count);

  return std::max(max_input_cap - current_input_cap, 0.0f);
}

void prepareFaninNeighborSlewBias(Resizer& resizer, ArcDelayState& context)
{
  // STA slew bias is applied only to the immediate fanin driver stage.
  if (context.target_stage_index <= 0) {
    return;
  }

  const DelayStageState& target_stage = context.target();
  const float max_input_cap_delta
      = maxTargetInputCapDelta(resizer, target_stage);
  if (max_input_cap_delta <= 0.0f) {
    logSlewBiasSkip(
        resizer, target_stage, "no target input capacitance growth");
    return;
  }

  DelayStageState& fanin_stage
      = context.path_stages[context.target_stage_index - 1];
  fanin_stage.sta_slew_bias
      = buildSlewBiasModel(resizer, fanin_stage, max_input_cap_delta);
}

// Estimate the post-ECO driver output slew for the selected transition.  When
// an STA slew-bias model is prepared for this stage, correct only the stable
// table-worst arc.  Otherwise use the existing merged table-delta calibration.
// `merge_arc_mode = kRelaxedCandidate` activates relaxed fallback for the
// merge loop (target stage only).  `slew_relaxed_out` is OR-set when any
// merge arc lookup used relaxed matching; never reset, so callers can chain.
float estimateOutputSlew(const DelayStageState& stage,
                         const sta::LibertyCell* cell,
                         const float path_input_slew,
                         const float load_cap,
                         const float selected_output_slew,
                         ArcMatchMode merge_arc_mode = ArcMatchMode::kExact,
                         bool* slew_relaxed_out = nullptr)
{
  if (canUseSlewBias(stage, cell, path_input_slew)) {
    float table_delay = 0.0f;
    float table_slew = 0.0f;
    if (lookupArcDelayAndSlewForArc(stage.arc.scene,
                                    stage.arc.min_max,
                                    stage.arc.pvt,
                                    stage.sta_slew_bias.table_worst_arc,
                                    stage.sta_slew_bias.input_slew,
                                    load_cap,
                                    cell,
                                    table_delay,
                                    table_slew)) {
      const float slew_bias
          = interpolateStaSlewBias(stage.sta_slew_bias, load_cap);
      return std::max(table_slew + slew_bias, 0.0f);
    }
  }

  // OpenSTA propagates GBA vertex slew, not the selected path arc's slew, so
  // merge all arc slews that share the selected output transition.
  float model_output_slew = selected_output_slew;
  for (const OutputSlewMergeArc& merge_arc : stage.output_slew_merge_arcs) {
    const bool uses_path_input
        = merge_arc.ref_arc->from() == stage.arc.inputPort()
          && merge_arc.ref_arc->fromEdge()->asRiseFall()
                 == stage.arc.inputRiseFall();
    const float input_slew
        = uses_path_input ? path_input_slew : merge_arc.input_slew;
    float delay = 0.0f;
    float merge_output_slew = 0.0f;
    if (!lookupArcDelayAndSlewForArc(stage.arc.scene,
                                     stage.arc.min_max,
                                     stage.arc.pvt,
                                     merge_arc.ref_arc,
                                     input_slew,
                                     load_cap,
                                     cell,
                                     delay,
                                     merge_output_slew,
                                     merge_arc_mode,
                                     slew_relaxed_out)) {
      continue;
    }

    // Store the worst slew
    if (stage.arc.min_max->compare(merge_output_slew, model_output_slew)) {
      model_output_slew = merge_output_slew;
    }
  }

  // Calibrate the table-model estimate to STA's current merged vertex slew.
  // The delta still captures the candidate/load change, while the baseline
  // preserves STA effects that the lightweight table walk cannot reproduce.
  const float calibrated_slew
      = stage.current_slew + (model_output_slew - stage.current_model_slew);
  return std::max(calibrated_slew, 0.0f);
}

float estimateReceiverInputSlew(const DelayStageState& driver_stage,
                                const DelayStageState& receiver_stage,
                                const float candidate_driver_output_slew)
{
  // Preserve the current net's receiver/driver slew degradation ratio instead
  // of passing the pre-RC driver output slew directly to the receiver input.

  // Pre-ECO driver output slew.
  const float current_driver_output_slew
      = std::max(driver_stage.current_slew, kMinSlewRatioDenominator);

  // Pre-ECO receiver input slew.
  const float current_receiver_input_slew
      = std::max(receiver_stage.input_slew, 0.0f);

  // Post-ECO estimated driver output slew.
  const float candidate_output_slew
      = std::max(candidate_driver_output_slew, 0.0f);

  return candidate_output_slew * current_receiver_input_slew
         / current_driver_output_slew;
}

std::optional<DelayStageState> buildDelayStageState(
    const Resizer& resizer,
    sta::Instance* inst,
    sta::Pin* driver_pin,
    const sta::PathExpanded& expanded,
    const int path_index,
    const sta::Scene* scene,
    const sta::MinMax* min_max,
    FailReason* fail_reason)
{
  const std::optional<SelectedArc> selected_arc = buildSelectedArc(resizer,
                                                                   inst,
                                                                   driver_pin,
                                                                   expanded,
                                                                   path_index,
                                                                   scene,
                                                                   min_max,
                                                                   fail_reason);
  if (!selected_arc.has_value()) {
    return std::nullopt;
  }

  const float input_slew = inputSlewOrZero(resizer, inst, *selected_arc);

  const float load_cap = resizer.staState()->graphDelayCalc()->loadCap(
      driver_pin, selected_arc->scene, selected_arc->min_max);
  sta::LibertyCell* current_cell = selected_arc->currentCell();
  float current_model_delay = 0.0f;
  float current_slew = 0.0f;
  if (!lookupArcDelayAndSlew(*selected_arc,
                             input_slew,
                             load_cap,
                             current_cell,
                             current_model_delay,
                             current_slew)) {
    setFailReason(fail_reason, FailReason::kMissingCurrentTimingArc);
    return std::nullopt;
  }
  const float current_delay = pathGraphCellDelayOrModelDelay(
      resizer, expanded, path_index, *selected_arc, current_model_delay);

  DelayStageState stage;
  stage.arc = *selected_arc;
  stage.driver_pin = driver_pin;
  stage.input_slew = input_slew;
  stage.load_cap = load_cap;
  stage.current_model_delay = current_model_delay;
  stage.current_delay = current_delay;
  stage.output_slew_merge_arcs = collectOutputSlewMergeArcs(
      resizer, *selected_arc, driver_pin, current_cell, load_cap);

  // Compute merged model slew from all arcs sharing the output transition
  stage.current_model_slew = current_slew;
  for (const OutputSlewMergeArc& merge_arc : stage.output_slew_merge_arcs) {
    if (selected_arc->min_max->compare(merge_arc.current_model_slew,
                                       stage.current_model_slew)) {
      stage.current_model_slew = merge_arc.current_model_slew;
    }
  }

  // Use STA graph vertex slew as baseline for delta calibration
  stage.current_slew = stage.current_model_slew;
  sta::Vertex* driver_vertex = resizer.graph()->pinDrvrVertex(driver_pin);
  if (driver_vertex != nullptr) {
    const int dcalc_ap
        = selected_arc->scene->dcalcAnalysisPtIndex(selected_arc->min_max);
    stage.current_slew = sta::delayAsFloat(resizer.graph()->slew(
        driver_vertex, selected_arc->outputRiseFall(), dcalc_ap));
  }
  stage.path_index = path_index;
  return stage;
}

std::optional<DelayStageState> buildDelayStageStateFromPath(
    const Resizer& resizer,
    const sta::PathExpanded& expanded,
    const int path_index,
    const sta::Scene* scene,
    const sta::MinMax* min_max,
    FailReason* fail_reason)
{
  const int start_index = static_cast<int>(expanded.startIndex());
  const int path_count = static_cast<int>(expanded.size());
  if (path_index < start_index || path_index >= path_count) {
    setFailReason(fail_reason, FailReason::kPathIndexOutOfRange);
    return std::nullopt;
  }

  const sta::Path* path = expanded.path(path_index);
  sta::Pin* driver_pin
      = path != nullptr ? path->pin(resizer.staState()) : nullptr;
  sta::Instance* inst = driver_pin != nullptr
                            ? resizer.network()->instance(driver_pin)
                            : nullptr;
  if (inst == nullptr) {
    setFailReason(fail_reason, FailReason::kMissingTargetInstance);
    return std::nullopt;
  }

  return buildDelayStageState(resizer,
                              inst,
                              driver_pin,
                              expanded,
                              path_index,
                              scene,
                              min_max,
                              fail_reason);
}

// Build an ArcDelayState that always contains the target stage; with
// delay_levels > 0, additionally captures up to N valid fanin/fanout
// stages.  target_stage_index points to the target stage in path_stages.
ArcDelayState collectPathStages(const Resizer& resizer,
                                const sta::PathExpanded& expanded,
                                const int target_path_index,
                                const sta::Scene* scene,
                                const sta::MinMax* min_max,
                                const int delay_levels,
                                const DelayStageState& target_stage)
{
  ArcDelayState context;
  context.delay_estimation_levels = delay_levels;
  if (delay_levels == 0) {
    context.path_stages = {target_stage};
    context.target_stage_index = 0;
    context.current_total_delay = target_stage.current_delay;
    return context;
  }

  std::vector<DelayStageState> fanin_stages;
  fanin_stages.reserve(delay_levels);
  const int start_index = static_cast<int>(expanded.startIndex());
  const int path_count = static_cast<int>(expanded.size());
  int found_fanin_stages = 0;
  for (int path_index = target_path_index - 1;
       path_index >= start_index && found_fanin_stages < delay_levels;
       --path_index) {
    FailReason ignored_reason = FailReason::kNone;
    const std::optional<DelayStageState> stage = buildDelayStageStateFromPath(
        resizer, expanded, path_index, scene, min_max, &ignored_reason);
    if (!stage.has_value()) {
      continue;
    }
    fanin_stages.push_back(*stage);
    ++found_fanin_stages;
  }
  std::ranges::reverse(fanin_stages);

  context.path_stages.reserve(fanin_stages.size() + 1 + delay_levels);
  for (DelayStageState& stage : fanin_stages) {
    context.path_stages.push_back(std::move(stage));
  }
  context.target_stage_index = static_cast<int>(context.path_stages.size());
  context.path_stages.push_back(target_stage);

  int found_fanout_stages = 0;
  for (int path_index = target_path_index + 1;
       path_index < path_count && found_fanout_stages < delay_levels;
       ++path_index) {
    FailReason ignored_reason = FailReason::kNone;
    const std::optional<DelayStageState> stage = buildDelayStageStateFromPath(
        resizer, expanded, path_index, scene, min_max, &ignored_reason);
    if (!stage.has_value()) {
      continue;
    }
    context.path_stages.push_back(*stage);
    ++found_fanout_stages;
  }
  for (const DelayStageState& stage : context.path_stages) {
    context.current_total_delay += stage.current_delay;
  }
  return context;
}

DelayEstimate makeEstimate(const float current_delay,
                           const float candidate_delay)
{
  const float arrival_impr = current_delay - candidate_delay;
  return {.legal = arrival_impr > 0.0f,
          .candidate_delay = candidate_delay,
          .arrival_impr = arrival_impr,
          .reason = arrival_impr > 0.0f ? FailReason::kEstimateLegal
                                        : FailReason::kEstimateNonImproving};
}

// Score a candidate over a path-local fanin/fanout window using cached delay
// state.  When `trace` is non-null, populates one StageEvaluation per
// evaluated stage so diagnostic callers can present per-stage detail.
//
// Approximation note: from the fanin neighbor onward, each stage feeds the
// previous stage's merged gate output slew into the next stage as input slew.
// The merge follows STA's same-output-transition slew merge, but still bypasses
// interconnect (RC) slew degradation between a driver's output pin and the next
// cell's input pin.  This remains a speed/accuracy trade-off for candidate
// ranking.
DelayEstimate estimateWindow(const ArcDelayState& context,
                             const sta::LibertyCell* candidate_cell,
                             std::vector<StageEvaluation>* trace = nullptr)
{
  const std::vector<DelayStageState>& stages = context.path_stages;
  const int target_index = context.target_stage_index;
  const DelayStageState& target_stage = stages[target_index];

  sta::LibertyCell* candidate_scene_cell
      = const_cast<sta::LibertyCell*>(candidate_cell)
            ->sceneCell(target_stage.arc.scene, target_stage.arc.min_max);
  if (candidate_scene_cell == nullptr) {
    return {.legal = false, .reason = FailReason::kMissingCandidatePort};
  }
  sta::LibertyPort* candidate_input = candidate_scene_cell->findLibertyPort(
      target_stage.arc.inputPort()->name());
  if (candidate_input == nullptr) {
    return {.legal = false, .reason = FailReason::kMissingCandidatePort};
  }

  const float current_target_input_cap
      = target_stage.arc.inputPort()->capacitance();
  const float candidate_target_input_cap = candidate_input->capacitance();
  const float target_input_cap_delta
      = candidate_target_input_cap - current_target_input_cap;

  const float current_total_delay = context.current_total_delay;
  float candidate_total_delay = 0.0f;
  float propagated_driver_output_slew = 0.0f;
  bool has_propagated_driver_output_slew = false;

  if (trace != nullptr) {
    trace->reserve(stages.size());
  }

  for (int stage_index = 0; std::cmp_less(stage_index, stages.size());
       ++stage_index) {
    const DelayStageState& stage = stages[stage_index];

    // Stages before the fanin neighbor are unaffected by the swap since
    // load cap cannot be changed.  Reuse the cached pre-swap stage values
    // for the trace, which would be identical to a fresh table-model lookup
    // with the same inputs.
    if (stage_index < target_index - 1) {
      candidate_total_delay += stage.current_delay;
      if (trace != nullptr) {
        trace->push_back({.path_index = stage.path_index,
                          .cell = stage.arc.currentCell(),
                          .input_slew = stage.input_slew,
                          .load_cap = stage.load_cap,
                          .stage_delay = stage.current_delay,
                          .output_slew = stage.current_slew});
      }
      continue;
    }

    // Per-stage role: the target stage uses the candidate cell; the fanin
    // neighbor sees the candidate's adjusted input cap; downstream stages
    // reuse the original cell.  Inter-stage slew propagation preserves the
    // current receiver/driver slew ratio for the net between adjacent stages.
    const bool is_target = (stage_index == target_index);
    const bool is_fanin_neighbor = (stage_index == target_index - 1);
    const sta::LibertyCell* cell
        = is_target ? candidate_cell : stage.arc.currentCell();
    const float load_cap = is_fanin_neighbor
                               ? stage.load_cap + target_input_cap_delta
                               : stage.load_cap;
    const float input_slew
        = has_propagated_driver_output_slew
              ? estimateReceiverInputSlew(stages[stage_index - 1],
                                          stage,
                                          propagated_driver_output_slew)
              : stage.input_slew;
    const FailReason fail_reason = is_target
                                       ? FailReason::kMissingCandidateTimingArc
                                       : FailReason::kMissingCurrentTimingArc;

    // Relaxed arc matching is applied only to the target candidate cell.
    const ArcMatchMode arc_mode
        = is_target ? ArcMatchMode::kRelaxedCandidate : ArcMatchMode::kExact;
    bool arc_relaxed = false;
    bool* arc_relaxed_out = is_target ? &arc_relaxed : nullptr;

    float model_stage_delay = 0.0f;
    float stage_slew = 0.0f;
    if (!lookupArcDelayAndSlew(stage.arc,
                               input_slew,
                               load_cap,
                               cell,
                               model_stage_delay,
                               stage_slew,
                               arc_mode,
                               arc_relaxed_out)) {
      return {.legal = false, .reason = fail_reason};
    }
    // Keep candidate scoring on the table-model delta, but report absolute
    // delay from the STA graph baseline so traces compare directly to golden.
    const float stage_delay
        = stage.current_delay + (model_stage_delay - stage.current_model_delay);
    candidate_total_delay += stage_delay;
    propagated_driver_output_slew = estimateOutputSlew(stage,
                                                       cell,
                                                       input_slew,
                                                       load_cap,
                                                       stage_slew,
                                                       arc_mode,
                                                       arc_relaxed_out);
    has_propagated_driver_output_slew = true;

    if (trace != nullptr) {
      trace->push_back({.path_index = stage.path_index,
                        .cell = cell,
                        .input_slew = input_slew,
                        .load_cap = load_cap,
                        .stage_delay = stage_delay,
                        .output_slew = propagated_driver_output_slew,
                        .arc_match_relaxed = arc_relaxed});
    }
  }

  return makeEstimate(current_total_delay, candidate_total_delay);
}

}  // namespace

bool DelayEstimator::findArcDelay(const SelectedArc& arc,
                                  const float input_slew,
                                  const float load_cap,
                                  const sta::LibertyCell* cell,
                                  float& delay)
{
  float output_slew = 0.0f;
  return lookupArcDelayAndSlew(
      arc, input_slew, load_cap, cell, delay, output_slew);
}

std::optional<ArcDelayState> DelayEstimator::buildContext(
    Resizer& resizer,
    const Target& target,
    const int delay_levels,
    FailReason* fail_reason,
    const bool use_sta_slew_bias)
{
  // Capture one active path arc and its electrical state so candidates can be
  // scored without touching shared timing state.  Higher delay levels expand
  // this into a path-local fanin/fanout timing window.
  assert(target.canBePathDriver());

  sta::Instance* inst = target.inst(resizer);
  if (inst == nullptr) {
    setFailReason(fail_reason, FailReason::kMissingTargetInstance);
    return std::nullopt;
  }

  sta::PathExpanded expanded(target.endpoint_path, resizer.staState());
  const sta::Scene* scene = target.endpoint_path->scene(resizer.staState());
  const sta::MinMax* min_max = target.endpoint_path->minMax(resizer.staState());
  if (min_max == nullptr) {
    min_max = resizer.maxAnalysisMode();
  }

  return buildContext(resizer,
                      inst,
                      target.driver_pin,
                      expanded,
                      target.path_index,
                      scene,
                      min_max,
                      delay_levels,
                      fail_reason,
                      use_sta_slew_bias);
}

std::optional<ArcDelayState> DelayEstimator::buildContext(
    Resizer& resizer,
    sta::Instance* inst,
    sta::Pin* driver_pin,
    const sta::PathExpanded& expanded,
    const int path_index,
    const sta::Scene* scene,
    const sta::MinMax* min_max,
    const int delay_levels,
    FailReason* fail_reason,
    const bool use_sta_slew_bias)
{
  // The target stage must be valid; surrounding stages are best-effort because
  // path endpoints and non-cell arcs are expected at window boundaries.
  const std::optional<DelayStageState> target_stage
      = buildDelayStageState(resizer,
                             inst,
                             driver_pin,
                             expanded,
                             path_index,
                             scene,
                             min_max,
                             fail_reason);
  if (!target_stage.has_value()) {
    return std::nullopt;
  }

  const int normalized_delay_levels = delay_levels > 0 ? delay_levels : 0;
  ArcDelayState context = collectPathStages(resizer,
                                            expanded,
                                            path_index,
                                            scene,
                                            min_max,
                                            normalized_delay_levels,
                                            *target_stage);
  if (use_sta_slew_bias) {
    prepareFaninNeighborSlewBias(resizer, context);
  }
  return context;
}

DelayEstimate DelayEstimator::estimate(const ArcDelayState& context,
                                       const sta::LibertyCell* candidate_cell,
                                       std::vector<StageEvaluation>* trace)
{
  // path_stages always contains at least the target stage by construction,
  // so estimateWindow handles delay_levels=0 (size 1) and >0 uniformly.
  if (trace != nullptr) {
    trace->clear();
  }
  return estimateWindow(context, candidate_cell, trace);
}

DelayEstimate DelayEstimator::estimate(const SelectedArc& arc,
                                       const float input_slew,
                                       const float load_cap,
                                       const float current_delay,
                                       const sta::LibertyCell* candidate_cell)
{
  float candidate_delay = 0.0f;
  if (!findArcDelay(
          arc, input_slew, load_cap, candidate_cell, candidate_delay)) {
    return {.legal = false, .reason = FailReason::kMissingCandidateTimingArc};
  }
  return makeEstimate(current_delay, candidate_delay);
}

}  // namespace rsz
