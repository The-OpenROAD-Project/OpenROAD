// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026-2026, The OpenROAD Authors

#include "DelayEstimator.hh"

#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <cstddef>
#include <limits>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "DelayEstimatorInternal.hh"
#include "OptimizerTypes.hh"
#include "RepairSetupProfiler.hh"
#include "boost/functional/hash.hpp"
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
#include "sta/PortDirection.hh"
#include "sta/Scene.hh"
#include "sta/Sdc.hh"
#include "sta/SearchClass.hh"
#include "sta/Sta.hh"
#include "sta/StaState.hh"
#include "sta/TableModel.hh"
#include "sta/TimingArc.hh"
#include "sta/TimingRole.hh"
#include "utl/Logger.h"

namespace rsz {

using estimator_detail::estimateOutputSlew;
using estimator_detail::estimateReceiverInputSlew;
using estimator_detail::kMinSlewRatioDenominator;
using estimator_detail::lookupArcDelayAndSlew;
using estimator_detail::lookupBufferDelayAndSlew;
using estimator_detail::makeEstimate;

namespace {

constexpr float kSlewMatchAbsTolerance = 1.0e-15f;
constexpr float kSlewMatchRelTolerance = 1.0e-6f;
// Converts positive side-input slew growth into sibling gate delay growth.
constexpr float kSideSlewDelayFactor = 0.3f;
// Estimates side-input slew growth from driver delay growth when the side net
// has no selected-arc slew model.
constexpr float kSideSlewPerDelay = 1.4f;

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

}  // namespace

// Shared with DelayEstimatorBufferTree.cc.
namespace estimator_detail {

bool lookupArcDelayAndSlew(const SelectedArc& arc,
                           const float input_slew,
                           const float load_cap,
                           const sta::LibertyCell* cell,
                           float& delay,
                           float& output_slew,
                           ArcMatchMode match_mode,
                           bool* match_relaxed_out)
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

bool lookupBufferDelayAndSlew(const DelayStageState& target_stage,
                              const sta::LibertyCell* buffer_cell,
                              const float input_slew,
                              const float load_cap,
                              float& delay,
                              float& output_slew)
{
  delay = 0.0f;
  output_slew = 0.0f;
  if (buffer_cell == nullptr) {
    return false;
  }

  sta::LibertyCell* scene_cell
      = const_cast<sta::LibertyCell*>(buffer_cell)
            ->sceneCell(target_stage.arc.scene, target_stage.arc.min_max);
  if (scene_cell == nullptr) {
    return false;
  }

  sta::LibertyPort* buffer_input = nullptr;
  sta::LibertyPort* buffer_output = nullptr;
  scene_cell->bufferPorts(buffer_input, buffer_output);
  if (buffer_input == nullptr || buffer_output == nullptr) {
    return false;
  }

  const sta::TimingArcSetSeq& arc_sets
      = scene_cell->timingArcSets(buffer_input, buffer_output);

  bool found_arc = false;
  for (const sta::TimingArcSet* arc_set : arc_sets) {
    if (arc_set->role()->isTimingCheck()) {
      continue;
    }

    for (const sta::TimingArc* arc : arc_set->arcs()) {
      const sta::RiseFall* input_rf = arc->fromEdge()->asRiseFall();
      const sta::RiseFall* output_rf = arc->toEdge()->asRiseFall();
      if (input_rf != target_stage.arc.outputRiseFall()
          || output_rf != target_stage.arc.outputRiseFall()) {
        continue;
      }

      float candidate_delay = 0.0f;
      float candidate_slew = 0.0f;
      if (!gateDelayAndSlewFromTableModel(target_stage.arc.pvt,
                                          arc,
                                          input_slew,
                                          load_cap,
                                          candidate_delay,
                                          candidate_slew)) {
        continue;
      }

      if (!found_arc
          || target_stage.arc.min_max->compare(candidate_delay, delay)) {
        delay = candidate_delay;
        output_slew = candidate_slew;
        found_arc = true;
      }
    }
  }

  return found_arc;
}

}  // namespace estimator_detail

namespace {

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
  // replacement queries require the canonical link cell mapped to OpenDB. The
  // driver instance already carries that link cell.
  sta::LibertyCell* current_scene_cell = target_stage.arc.currentCell();
  sta::Network* network = resizer.network();
  sta::Instance* driver_inst = network->instance(target_stage.driver_pin);
  sta::LibertyCell* current_link_cell
      = driver_inst != nullptr ? network->libertyCell(driver_inst) : nullptr;
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
  sta::LibertyCellSeq* equiv_cells = resizer.equivCells(current_link_cell);
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

void prepareFaninNeighborSlewBias(Resizer& resizer,
                                  EstimatorContext& context,
                                  DelayEstimatorCache* cache)
{
  // STA slew bias is applied only to the immediate fanin driver stage.
  if (context.target_stage_index <= 0) {
    return;
  }

  const DelayStageState& target_stage = context.target();
  const float max_input_cap_delta
      = cache != nullptr
            ? cache->maxEquivalentInputCapIncrease(resizer, target_stage)
            : maxTargetInputCapDelta(resizer, target_stage);
  if (max_input_cap_delta <= 0.0f) {
    logSlewBiasSkip(
        resizer, target_stage, "no target input capacitance growth");
    return;
  }

  DelayStageState& fanin_stage
      = context.path_stages[context.target_stage_index - 1];

  fanin_stage.sta_slew_bias
      = cache != nullptr
            ? cache->getOrSampleSlewBias(
                  resizer, fanin_stage, max_input_cap_delta)
            : buildSlewBiasModel(resizer, fanin_stage, max_input_cap_delta);
}

}  // namespace

void DelayEstimatorCache::clear()
{
  *this = DelayEstimatorCache{};
}

// The equivalent-cell envelope does not change within a policy run.
float DelayEstimatorCache::maxEquivalentInputCapIncrease(
    Resizer& resizer,
    const DelayStageState& target_stage)
{
  const sta::LibertyPort* input_port = target_stage.arc.inputPort();
  std::unordered_map<const sta::LibertyPort*, float>::iterator cached
      = max_equiv_input_cap_increase_by_port.find(input_port);
  if (cached == max_equiv_input_cap_increase_by_port.end()) {
    cached = max_equiv_input_cap_increase_by_port
                 .emplace(input_port,
                          maxTargetInputCapDelta(resizer, target_stage))
                 .first;
  }

  return cached->second;
}

size_t DelayEstimatorCache::SlewBiasModelKey::Hash::operator()(
    const SlewBiasModelKey& key) const
{
  size_t seed = 0;
  boost::hash_combine(seed, key.driver_pin);
  boost::hash_combine(seed, key.ref_arc);
  boost::hash_combine(seed, key.scene);
  boost::hash_combine(seed, key.min_max);
  boost::hash_combine(seed, key.current_cell);
  boost::hash_combine(seed, key.input_slew);
  boost::hash_combine(seed, key.load_cap);
  boost::hash_combine(seed, key.max_load_delta);
  boost::hash_combine(seed, key.pi_c2);
  boost::hash_combine(seed, key.pi_r);
  boost::hash_combine(seed, key.pi_c1);
  boost::hash_combine(seed, key.merge_arcs.size());
  for (const MergeArc& arc : key.merge_arcs) {
    boost::hash_combine(seed, arc.ref_arc);
    boost::hash_combine(seed, arc.input_slew);
  }
  return seed;
}

// Every input read by buildSlewBiasModel(): the stage identity and slews, the
// sampled load range, and the live reduced Pi model of the driver net. A
// missing Pi model disables memoization because sampling would fail anyway.
std::optional<DelayEstimatorCache::SlewBiasModelKey>
DelayEstimatorCache::makeSlewBiasModelKey(Resizer& resizer,
                                          const DelayStageState& stage,
                                          const float max_load_delta)
{
  sta::Parasitics* parasitics = stage.arc.scene->parasitics(stage.arc.min_max);
  sta::ArcDelayCalc* arc_delay_calc = resizer.staState()->arcDelayCalc();
  if (parasitics == nullptr || arc_delay_calc == nullptr
      || stage.driver_pin == nullptr) {
    return std::nullopt;
  }

  sta::Parasitic* parasitic
      = arc_delay_calc->findParasitic(stage.driver_pin,
                                      stage.arc.outputRiseFall(),
                                      stage.arc.scene,
                                      stage.arc.min_max);
  if (parasitic == nullptr || !parasitics->isPiModel(parasitic)) {
    return std::nullopt;
  }

  float c2 = 0.0f;
  float rpi = 0.0f;
  float c1 = 0.0f;
  parasitics->piModel(parasitic, c2, rpi, c1);

  SlewBiasModelKey key{
      .driver_pin = stage.driver_pin,
      .ref_arc = stage.arc.ref_arc,
      .scene = stage.arc.scene,
      .min_max = stage.arc.min_max,
      .current_cell = stage.arc.currentCell(),
      .input_slew = stage.input_slew,
      .load_cap = stage.load_cap,
      .max_load_delta = max_load_delta,
      .pi_c2 = c2,
      .pi_r = rpi,
      .pi_c1 = c1,
      .merge_arcs = {},
  };
  key.merge_arcs.reserve(stage.output_slew_merge_arcs.size());
  for (const OutputSlewMergeArc& merge_arc : stage.output_slew_merge_arcs) {
    key.merge_arcs.push_back({merge_arc.ref_arc, merge_arc.input_slew});
  }

  return key;
}

SlewBiasModel DelayEstimatorCache::getOrSampleSlewBias(
    Resizer& resizer,
    const DelayStageState& stage,
    const float max_load_delta)
{
  std::optional<SlewBiasModelKey> key
      = makeSlewBiasModelKey(resizer, stage, max_load_delta);
  if (key.has_value()) {
    const SlewBiasModelMap::const_iterator cached = slew_bias_models.find(*key);
    if (cached != slew_bias_models.end()) {
      RepairSetupProfiler::event("slew_bias_model_hits");
      return cached->second;
    }
  }

  SlewBiasModel model = buildSlewBiasModel(resizer, stage, max_load_delta);
  if (key.has_value()) {
    RepairSetupProfiler::event("slew_bias_model_misses");
    slew_bias_models.emplace(std::move(*key), model);
  }
  return model;
}

// Shared with DelayEstimatorBufferTree.cc.
namespace estimator_detail {

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
                         ArcMatchMode merge_arc_mode,
                         bool* slew_relaxed_out)
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

float wireElmoreDelay(const WireRC& wire, const float load_cap)
{
  if (wire.res <= 0.0f || wire.cap < 0.0f || load_cap < 0.0f) {
    return 0.0f;
  }
  return wire.res * (load_cap + 0.5f * wire.cap);
}

}  // namespace estimator_detail

namespace {

float currentPathWireDelay(const Resizer& resizer,
                           const sta::PathExpanded& expanded,
                           const int path_index,
                           const SelectedArc& selected_arc)
{
  if (path_index + 1 >= expanded.size()) {
    return 0.0f;
  }

  const sta::Path* next_path = expanded.path(path_index + 1);
  sta::Edge* wire_edge = next_path != nullptr
                             ? next_path->prevEdge(resizer.staState())
                             : nullptr;
  if (wire_edge == nullptr || !wire_edge->isWire()) {
    return 0.0f;
  }

  const int dcalc_ap
      = selected_arc.scene->dcalcAnalysisPtIndex(selected_arc.min_max);
  return sta::delayAsFloat(resizer.graph()->wireArcDelay(
      wire_edge, selected_arc.outputRiseFall(), dcalc_ap));
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
  stage.current_wire_delay
      = currentPathWireDelay(resizer, expanded, path_index, *selected_arc);
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

// Build an EstimatorContext that always contains the target stage; with
// delay_levels > 0, additionally captures up to N valid fanin/fanout
// stages.  target_stage_index points to the target stage in path_stages.
EstimatorContext collectPathStages(const Resizer& resizer,
                                   const sta::PathExpanded& expanded,
                                   const int target_path_index,
                                   const sta::Scene* scene,
                                   const sta::MinMax* min_max,
                                   const int delay_levels,
                                   const DelayStageState& target_stage)
{
  EstimatorContext context;
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
  sta::Instance* target_inst
      = resizer.network()->instance(target_stage.driver_pin);
  for (int path_index = target_path_index + 1;
       path_index < path_count && found_fanout_stages < delay_levels;
       ++path_index) {
    FailReason ignored_reason = FailReason::kNone;
    const std::optional<DelayStageState> stage = buildDelayStageStateFromPath(
        resizer, expanded, path_index, scene, min_max, &ignored_reason);
    if (!stage.has_value()) {
      continue;
    }

    sta::Instance* fanout_inst = resizer.network()->instance(stage->driver_pin);
    if (fanout_inst != nullptr && target_inst != nullptr
        && fanout_inst == target_inst) {
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

bool latchDataPathMatches(const Resizer& resizer,
                          const sta::Path* q_path,
                          const sta::Path* d_path,
                          const DelayStageState& target_stage)
{
  const SelectedArc& target_arc = target_stage.arc;

  // Only the gated, selected D->Q path may bridge a latch boundary.
  const sta::StaState* sta = resizer.staState();
  const sta::Pin* d_pin = resizer.network()->findPin(
      resizer.network()->instance(target_stage.driver_pin),
      target_arc.inputPort()->name());
  return d_path != nullptr && sta::Path::equal(q_path->prevPath(), d_path, sta)
         && q_path->pin(sta) == target_stage.driver_pin
         && d_path->pin(sta) == d_pin
         && q_path->transition(sta) == target_arc.outputRiseFall()
         && d_path->transition(sta) == target_arc.inputRiseFall()
         && q_path->scene(sta) == target_arc.scene
         && d_path->scene(sta) == target_arc.scene
         && q_path->minMax(sta) == target_arc.min_max
         && d_path->minMax(sta) == target_arc.min_max
         && d_path->pathAnalysisPtIndex(sta)
                == q_path->pathAnalysisPtIndex(sta);
}

bool includeLatchFaninStages(const Resizer& resizer,
                             const sta::PathExpanded& expanded,
                             EstimatorContext& context,
                             FailReason* fail_reason)
{
  const DelayStageState& target_stage = context.target();
  const SelectedArc& target_arc = target_stage.arc;
  if (!target_arc.ref_arc->role()->isLatchDtoQ()
      || target_stage.path_index != static_cast<int>(expanded.startIndex())) {
    return true;
  }

  const sta::StaState* sta = resizer.staState();
  const sta::Instance* target_inst
      = resizer.network()->instance(target_stage.driver_pin);
  const sta::Path* q_path = expanded.path(target_stage.path_index);
  const sta::Path* d_path = latchDataPath(expanded);
  if (!latchDataPathMatches(resizer, q_path, d_path, target_stage)) {
    setFailReason(fail_reason, FailReason::kMissingPrevTimingArc);
    return false;
  }

  // Expand this data segment only; do not walk earlier launch clocks/latches.
  const sta::PathExpanded data_expanded(d_path, sta);
  const int driver_index = static_cast<int>(data_expanded.size()) - 2;
  const sta::Path* driver_path = d_path->prevPath();
  const sta::Edge* wire_edge = d_path->prevEdge(sta);
  if (driver_index < static_cast<int>(data_expanded.startIndex())
      || driver_path == nullptr || wire_edge == nullptr || !wire_edge->isWire()
      || !sta::Path::equal(data_expanded.path(driver_index), driver_path, sta)
      || wire_edge->from(resizer.graph()) != driver_path->vertex(sta)
      || wire_edge->to(resizer.graph()) != d_path->vertex(sta)
      || resizer.network()->instance(driver_path->pin(sta)) == target_inst
      || driver_path->transition(sta) != target_arc.inputRiseFall()
      || driver_path->scene(sta) != target_arc.scene
      || driver_path->minMax(sta) != target_arc.min_max
      || driver_path->pathAnalysisPtIndex(sta)
             != d_path->pathAnalysisPtIndex(sta)) {
    setFailReason(fail_reason, FailReason::kMissingPrevTimingArc);
    return false;
  }

  const std::optional<DelayStageState> driver_stage
      = buildDelayStageStateFromPath(resizer,
                                     data_expanded,
                                     driver_index,
                                     target_arc.scene,
                                     target_arc.min_max,
                                     fail_reason);
  if (!driver_stage.has_value()) {
    return false;
  }

  const int additional_levels
      = std::max(context.delay_estimation_levels, 1) - 1;
  EstimatorContext fanin = collectPathStages(resizer,
                                             data_expanded,
                                             driver_index,
                                             target_arc.scene,
                                             target_arc.min_max,
                                             additional_levels,
                                             *driver_stage);
  for (DelayStageState& stage : fanin.path_stages) {
    // A feedback segment must not reintroduce the resized physical driver.
    const sta::Path* path = data_expanded.path(stage.path_index);
    if (resizer.network()->instance(stage.driver_pin) == target_inst
        || path->scene(sta) != target_arc.scene
        || path->minMax(sta) != target_arc.min_max
        || path->pathAnalysisPtIndex(sta) != d_path->pathAnalysisPtIndex(sta)
        || path->transition(sta) != stage.arc.outputRiseFall()) {
      setFailReason(fail_reason, FailReason::kMissingPrevTimingArc);
      return false;
    }

    // These stages have no index in the primary expanded path.
    stage.path_index = -1;
  }

  context.path_stages.insert(context.path_stages.begin(),
                             fanin.path_stages.begin(),
                             fanin.path_stages.end());
  context.target_stage_index += static_cast<int>(fanin.path_stages.size());
  context.current_total_delay += fanin.current_total_delay;

  return true;
}

}  // namespace

// Shared with DelayEstimatorBufferTree.cc.
namespace estimator_detail {

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

}  // namespace estimator_detail

namespace {

// Record each input net of the target: the driver's delay per load and the
// worst slack of each load pin.  The selected-arc net skips the target pin,
// which the window already scores with the exact fanin delta.
void collectFaninSideLoads(Resizer& resizer,
                           sta::Instance* target_inst,
                           const sta::MinMax* min_max,
                           const FaninSideLoadOptions& options,
                           EstimatorContext& context)
{
  if (min_max == nullptr) {
    return;
  }

  sta::dbSta* sta = resizer.sta();
  sta::Network* network = resizer.network();
  sta::Graph* graph = sta->graph();
  const sta::Pin* selected_input_pin
      = network->findPin(target_inst, context.target().arc.inputPort());
  const sta::Pin* selected_fanin_pin
      = context.target_stage_index >= 1
            ? context.path_stages[context.target_stage_index - 1].driver_pin
            : nullptr;

  size_t load_count = 0;
  std::unique_ptr<sta::InstancePinIterator> input_iter(
      network->pinIterator(target_inst));
  while (input_iter->hasNext()) {
    const sta::Pin* input_pin = input_iter->next();
    sta::LibertyPort* input_port = network->libertyPort(input_pin);
    if (input_port == nullptr || !network->direction(input_pin)->isInput()
        || sta->isClock(input_pin, sta->cmdMode())) {
      continue;
    }

    sta::Net* net = network->net(input_pin);
    if (net == nullptr) {
      continue;
    }

    FaninSideNet side_net;
    side_net.input_port = input_port;
    side_net.input_cap = input_port->capacitance();
    side_net.selected_arc = input_pin == selected_input_pin
                            && selected_fanin_pin != nullptr
                            && network->net(selected_fanin_pin) == net;
    if (!side_net.selected_arc) {
      // Drive resistance is the Liberty delay-versus-load slope of the driver.
      const sta::PinSet* drivers = network->drivers(net);
      const sta::LibertyPort* driver_port
          = drivers != nullptr && !drivers->empty()
                ? network->libertyPort(*drivers->begin())
                : nullptr;
      if (driver_port == nullptr) {
        continue;
      }

      side_net.delay_per_cap = driver_port->driveResistance();
      if (side_net.delay_per_cap <= 0.0f) {
        continue;
      }
    }

    std::unique_ptr<sta::NetConnectedPinIterator> pin_iter(
        network->connectedPinIterator(net));
    while (pin_iter->hasNext()) {
      const sta::Pin* pin = pin_iter->next();
      if ((side_net.selected_arc && pin == input_pin)
          || !network->isLoad(pin)) {
        continue;
      }

      sta::Vertex* vertex = graph->pinLoadVertex(pin);
      if (vertex == nullptr) {
        continue;
      }

      const sta::Slack slack = sta->slack(vertex, min_max);
      if (sta::delayInf(slack, sta)) {
        continue;
      }

      side_net.loads.push_back(
          {.pin = pin,
           .slack = sta::delayAsFloat(slack),
           .weight = sideLoadWeight(
               pin, options.critical_load_pins, options.off_path_weight)});
    }

    if (side_net.loads.empty()) {
      continue;
    }

    load_count += side_net.loads.size();
    context.fanin_side_nets.push_back(std::move(side_net));
  }

  RepairSetupProfiler::event("fanin_side_loads_collected", load_count);
}

// Record the target cell arcs that drive the opposite output transition
// through the selected input, with their current table-model delays, and the
// worst slack of that transition at the driver.
void collectOppositeRfArcs(Resizer& resizer, EstimatorContext& context)
{
  DelayStageState& target = context.path_stages[context.target_stage_index];
  sta::Graph* graph = resizer.graph();
  sta::Vertex* driver_vertex = graph->pinDrvrVertex(target.driver_pin);
  if (driver_vertex == nullptr) {
    return;
  }

  const sta::RiseFall* output_rf = target.arc.outputRiseFall();
  const sta::RiseFall* opposite_rf = output_rf->opposite();
  const std::string& input_name = target.arc.inputPort()->name();
  const std::string& output_name = target.arc.outputPort()->name();
  sta::LibertyCell* current_cell = target.arc.currentCell();

  sta::VertexInEdgeIterator edge_iter(driver_vertex, graph);
  while (edge_iter.hasNext()) {
    sta::Edge* edge = edge_iter.next();
    const sta::TimingArcSet* arc_set = edge->timingArcSet();
    if (arc_set->role()->isTimingCheck() || arc_set->to()->name() != output_name
        || arc_set->from()->name() != input_name) {
      continue;
    }

    for (const sta::TimingArc* graph_arc : arc_set->arcs()) {
      const sta::RiseFall* input_rf = graph_arc->fromEdge()->asRiseFall();
      if (input_rf == nullptr
          || graph_arc->toEdge()->asRiseFall() != opposite_rf) {
        continue;
      }

      const sta::TimingArc* ref_arc
          = findCellTimingArcLike(graph_arc, current_cell);
      if (ref_arc == nullptr) {
        continue;
      }

      const float input_slew
          = resizer.staState()->graphDelayCalc()->edgeFromSlew(
              edge->from(graph),
              input_rf,
              edge,
              target.arc.scene,
              target.arc.min_max);

      float delay = 0.0f;
      float output_slew = 0.0f;
      if (!gateDelayAndSlewFromTableModel(target.arc.pvt,
                                          ref_arc,
                                          input_slew,
                                          target.load_cap,
                                          delay,
                                          output_slew)) {
        continue;
      }

      target.opposite_rf_arcs.push_back({.ref_arc = ref_arc,
                                         .input_slew = input_slew,
                                         .current_model_delay = delay});
    }
  }

  if (target.opposite_rf_arcs.empty()) {
    return;
  }

  const sta::Slack slack
      = resizer.sta()->slack(driver_vertex, opposite_rf, target.arc.min_max);
  if (!sta::delayInf(slack, resizer.sta())) {
    context.opposite_rf_slack = sta::delayAsFloat(slack);
  }

  RepairSetupProfiler::event("dual_rf_arcs_prepared",
                             target.opposite_rf_arcs.size());
}

// Charge the opposite output transition of the target like a side path: its
// delay growth costs negative-region slack, and pushing it below the global
// floor rejects the candidate.  Only arcs the candidate cell also has count.
void applyDualRfCheck(const EstimatorContext& context,
                      const sta::LibertyCell* candidate_cell,
                      DelayEstimate& estimate)
{
  const DelayStageState& target = context.target();
  if (target.opposite_rf_arcs.empty()) {
    return;
  }

  float worst_delta = -std::numeric_limits<float>::infinity();
  for (const OppositeRfArc& opposite : target.opposite_rf_arcs) {
    float delay = 0.0f;
    float output_slew = 0.0f;
    if (!lookupArcDelayAndSlewForArc(target.arc.scene,
                                     target.arc.min_max,
                                     target.arc.pvt,
                                     opposite.ref_arc,
                                     opposite.input_slew,
                                     target.load_cap,
                                     candidate_cell,
                                     delay,
                                     output_slew,
                                     ArcMatchMode::kRelaxedCandidate)) {
      continue;
    }

    worst_delta = std::max(worst_delta, delay - opposite.current_model_delay);
  }

  if (!std::isfinite(worst_delta)) {
    return;
  }

  if (worst_delta <= 0.0f || !std::isfinite(context.opposite_rf_slack)) {
    return;
  }

  const float slack_after = context.opposite_rf_slack - worst_delta;
  estimate.side_loss
      += oppositeRfSideLoss(context.opposite_rf_slack, worst_delta);
  ++estimate.side_paths_evaluated;
  if (slack_after < context.sideSlackFloor(estimate.arrival_impr)) {
    estimate.legal = false;
    estimate.reason = FailReason::kDualRfNonImproving;
  }
}

// Adds the negative-region loss of side_paths side loads, weighted by
// fanin_side_loss_weight, and rejects the candidate when the worst of them
// falls below the side slack floor.  The fanin side loads and the loads a
// split moves share it.
void chargeSideLoss(const EstimatorContext& context,
                    const int side_paths,
                    const float loss,
                    const float worst_slack_after,
                    DelayEstimate& estimate)
{
  // Accumulate: the dual rise/fall check may already have charged a path.
  estimate.side_paths_evaluated += side_paths;
  estimate.side_loss += loss * context.fanin_side_loss_weight;
  if (worst_slack_after < context.sideSlackFloor(estimate.arrival_impr)) {
    estimate.legal = false;
    estimate.reason = FailReason::kSideSlackFloorViolation;
  }
}

// Charge the fanin drivers' delay growth to the recorded side paths.  The loss
// counts only below zero slack, as an estimate of the TNS change.  A side path
// pushed below the side slack floor rejects the candidate.
void applyFaninSideLoss(const EstimatorContext& context,
                        const sta::LibertyCell* candidate_scene_cell,
                        const float selected_fanin_delay_delta,
                        const float selected_fanin_slew_delta,
                        DelayEstimate& estimate)
{
  if (context.fanin_side_nets.empty()) {
    return;
  }

  float loss = 0.0f;
  float worst_slack_after = std::numeric_limits<float>::infinity();
  int side_paths = 0;
  for (const FaninSideNet& side_net : context.fanin_side_nets) {
    // A slower fanin driver also slows the slew into each side gate, adding
    // kSideSlewDelayFactor of the slew growth to its delay.  Nets off the
    // selected arc have no slew model and take kSideSlewPerDelay instead.
    float delay_delta = selected_fanin_delay_delta;
    float slew_delta = selected_fanin_slew_delta;
    if (!side_net.selected_arc) {
      const sta::LibertyPort* candidate_port
          = candidate_scene_cell->findLibertyPort(side_net.input_port->name());
      if (candidate_port == nullptr) {
        continue;
      }

      delay_delta = side_net.delay_per_cap
                    * (candidate_port->capacitance() - side_net.input_cap);
      slew_delta = kSideSlewPerDelay * delay_delta;
    }

    if (delay_delta <= 0.0f) {
      continue;
    }

    delay_delta += kSideSlewDelayFactor * std::max(slew_delta, 0.0f);
    for (const SideLoad& side : side_net.loads) {
      const float slack_after = side.slack - delay_delta;
      loss += side.weight
              * (std::min(side.slack, 0.0f) - std::min(slack_after, 0.0f));
      worst_slack_after = std::min(worst_slack_after, slack_after);
      ++side_paths;
    }
  }

  if (side_paths == 0) {
    return;
  }

  chargeSideLoss(context, side_paths, loss, worst_slack_after, estimate);
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
DelayEstimate estimateWindow(const EstimatorContext& context,
                             const sta::LibertyCell* candidate_cell,
                             std::vector<StageEvaluation>* trace = nullptr,
                             const float target_load_delta = 0.0f)
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
  float fanin_delay_delta = 0.0f;
  float fanin_slew_delta = 0.0f;
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
    float load_cap = stage.load_cap;
    // A load change (split, fanout size-down) moves capacitance off the
    // target net.
    if (is_fanin_neighbor) {
      load_cap += target_input_cap_delta;
    } else if (is_target) {
      load_cap = std::max(load_cap + target_load_delta, 0.0f);
    }
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
    if (is_fanin_neighbor) {
      fanin_delay_delta = stage_delay - stage.current_delay;
      fanin_slew_delta = propagated_driver_output_slew - stage.current_slew;
    }

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

  DelayEstimate estimate
      = makeEstimate(current_total_delay, candidate_total_delay);
  if (estimate.legal) {
    applyDualRfCheck(context, candidate_scene_cell, estimate);
  }

  applyFaninSideLoss(context,
                     candidate_scene_cell,
                     fanin_delay_delta,
                     fanin_slew_delta,
                     estimate);
  return estimate;
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

std::optional<EstimatorContext> DelayEstimator::buildContext(
    Resizer& resizer,
    const Target& target,
    const int delay_levels,
    FailReason* fail_reason,
    const bool use_sta_slew_bias,
    const bool include_latch_fanin,
    DelayEstimatorCache* cache,
    const FaninSideLoadOptions* fanin_side_loads,
    const bool dual_rf_eval)
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
                      use_sta_slew_bias,
                      include_latch_fanin,
                      cache,
                      fanin_side_loads,
                      dual_rf_eval);
}

std::optional<EstimatorContext> DelayEstimator::buildContext(
    Resizer& resizer,
    sta::Instance* inst,
    sta::Pin* driver_pin,
    const sta::PathExpanded& expanded,
    const int path_index,
    const sta::Scene* scene,
    const sta::MinMax* min_max,
    const int delay_levels,
    FailReason* fail_reason,
    const bool use_sta_slew_bias,
    const bool include_latch_fanin,
    DelayEstimatorCache* cache,
    const FaninSideLoadOptions* fanin_side_loads,
    const bool dual_rf_eval)
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
  EstimatorContext context = collectPathStages(resizer,
                                               expanded,
                                               path_index,
                                               scene,
                                               min_max,
                                               normalized_delay_levels,
                                               *target_stage);
  if (include_latch_fanin
      && !includeLatchFaninStages(resizer, expanded, context, fail_reason)) {
    return std::nullopt;
  }
  if (use_sta_slew_bias) {
    const ProfileScope profile("prepare_slew_bias");
    prepareFaninNeighborSlewBias(resizer, context, cache);
  }

  if (fanin_side_loads != nullptr) {
    const ProfileScope profile("prepare_fanin_side_loads");
    collectFaninSideLoads(resizer, inst, min_max, *fanin_side_loads, context);
  }

  if (dual_rf_eval) {
    const ProfileScope profile("prepare_dual_rf");
    collectOppositeRfArcs(resizer, context);
  }
  return context;
}

float oppositeRfSideLoss(const float opposite_slack, const float delay_delta)
{
  if (!std::isfinite(opposite_slack) || delay_delta <= 0.0f) {
    return 0.0f;
  }
  return std::min(opposite_slack, 0.0f)
         - std::min(opposite_slack - delay_delta, 0.0f);
}

DelayEstimate DelayEstimator::estimate(const EstimatorContext& context,
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

DelayEstimate DelayEstimator::estimateLoadChange(
    const EstimatorContext& context,
    const float target_load_delta)
{
  return estimateWindow(
      context, context.target().arc.currentCell(), nullptr, target_load_delta);
}

bool DelayEstimator::findBufferDelay(const EstimatorContext& context,
                                     const sta::LibertyCell* buffer_cell,
                                     const float input_slew,
                                     const float load_cap,
                                     float& delay)
{
  float output_slew = 0.0f;
  return lookupBufferDelayAndSlew(
      context.target(), buffer_cell, input_slew, load_cap, delay, output_slew);
}

void DelayEstimator::chargeMovedLoads(const EstimatorContext& context,
                                      const std::vector<SideLoad>& loads,
                                      const float delay_delta,
                                      DelayEstimate& estimate)
{
  if (loads.empty()) {
    return;
  }

  // The same negative-region clamp and floor guard as the fanin siblings.
  float loss = 0.0f;
  float worst_slack_after = std::numeric_limits<float>::infinity();
  for (const SideLoad& load : loads) {
    const float slack_after = load.slack - delay_delta;
    loss += load.weight
            * (std::min(load.slack, 0.0f) - std::min(slack_after, 0.0f));
    worst_slack_after = std::min(worst_slack_after, slack_after);
  }

  chargeSideLoss(context,
                 static_cast<int>(loads.size()),
                 loss,
                 worst_slack_after,
                 estimate);
}

}  // namespace rsz
