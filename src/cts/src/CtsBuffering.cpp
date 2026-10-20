// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include <algorithm>
#include <array>
#include <cmath>
#include <functional>
#include <limits>
#include <map>
#include <optional>
#include <stdexcept>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include "BottomUpBuffering.h"
#include "BufferingSearch.h"
#include "Clock.h"
#include "CtsOptions.h"
#include "TechChar.h"
#include "TreeBuilder.h"
#include "cts/TritonCTS.h"
#include "db_sta/dbNetwork.hh"
#include "db_sta/dbSta.hh"
#include "est/EstimateParasitics.h"
#include "odb/PtrSetMap.h"
#include "odb/db.h"
#include "rsz/Resizer.hh"
#include "sta/Clock.hh"
#include "sta/Delay.hh"
#include "sta/Fuzzy.hh"
#include "sta/Liberty.hh"
#include "sta/MinMax.hh"
#include "sta/Mode.hh"
#include "sta/Scene.hh"
#include "sta/Sdc.hh"
#include "sta/SdcClass.hh"
#include "sta/Transition.hh"
#include "utl/Logger.h"

namespace cts {
namespace {
BufferingWireRC wireRCBound(est::EstimateParasitics* parasitics,
                            sta::dbSta* sta)
{
  BufferingWireRC bound;
  if (sta->scenes().empty()) {
    throw std::invalid_argument("No scenes available for bottom-up CTS");
  }
  for (const auto* scene : sta->scenes()) {
    // Register-terminal clock nets use SIGNAL RC in placement estimation.
    // Enclose both net classes and both directions, never their averages.
    const std::array<BufferingWireRC, 4> values{
        {{parasitics->wireClkHResistance(scene),
          parasitics->wireClkHCapacitance(scene)},
         {parasitics->wireClkVResistance(scene),
          parasitics->wireClkVCapacitance(scene)},
         {parasitics->wireSignalHResistance(scene),
          parasitics->wireSignalHCapacitance(scene)},
         {parasitics->wireSignalVResistance(scene),
          parasitics->wireSignalVCapacitance(scene)}}};
    BufferingWireRC scene_bound;
    try {
      scene_bound = conservativeBufferingRC(values);
    } catch (const std::exception& error) {
      throw std::invalid_argument("Scene " + scene->name() + ": "
                                  + error.what());
    }
    bound.resistance = std::max(bound.resistance, scene_bound.resistance);
    bound.capacitance = std::max(bound.capacitance, scene_bound.capacitance);
  }
  return bound;
}

struct BufferCell
{
  std::string name;
  sta::LibertyPort* output;
  double input_cap;
  double nominal_input_cap;
  double max_cap;
  double area;
  int max_fanout;
  bool in_buffer_list;
};

struct CellRejection
{
  std::string constraint;
  double value = 0;
  double limit = 0;
  std::string scene;
  size_t transition = 0;
};

double inputCap(sta::LibertyPort* port, sta::dbSta* sta)
{
  double cap = 0;
  for (const auto* scene : sta->scenes()) {
    const auto* scene_port = port->scenePort(scene, sta::MinMax::max());
    if (scene_port == nullptr) {
      throw std::runtime_error("Missing scene data for a CTS input pin");
    }
    for (const auto* rf : {sta::RiseFall::rise(), sta::RiseFall::fall()}) {
      cap = std::max(
          cap,
          static_cast<double>(scene_port->capacitance(rf, sta::MinMax::max())));
    }
  }
  return cap;
}

void tightenSlew(sta::LibertyPort* port,
                 sta::Port* network_port,
                 sta::dbSta* sta,
                 double& limit)
{
  for (const auto* scene : sta->scenes()) {
    float value;
    bool exists;
    sta->findSlewLimit(port, scene, sta::MinMax::max(), value, exists);
    if (exists) {
      limit = std::min(limit, static_cast<double>(value));
    }
  }
  for (auto* mode : sta->modes()) {
    float value;
    bool exists;
    mode->sdc()->slewLimit(network_port, sta::MinMax::max(), value, exists);
    if (exists) {
      limit = std::min(limit, static_cast<double>(value));
    }
  }
}

class ClockBuffering
{
 public:
  using FanoutLimit = std::function<int(const std::string&)>;
  using DummySelector = std::function<sta::LibertyCell*(float, float, size_t)>;

  ClockBuffering(CtsOptions* options,
                 odb::dbDatabase* db,
                 sta::dbNetwork* network,
                 sta::dbSta* sta,
                 rsz::Resizer* resizer,
                 est::EstimateParasitics* parasitics,
                 double length_unit,
                 FanoutLimit fanout_limit,
                 DummySelector dummy_selector)
      : options_(options),
        db_(db),
        network_(network),
        sta_(sta),
        resizer_(resizer),
        parasitics_(parasitics),
        length_unit_(length_unit),
        fanout_limit_(std::move(fanout_limit)),
        dummy_selector_(std::move(dummy_selector))
  {
  }

  void run(TreeBuilder* builder)
  {
    builder_ = builder;
    clock_ = &builder->getClock();
    slew_limit_ = options_->getMaxCharSlew();
    prepareCells();
    readTree();
    tightenModeSlew();
    if (!(slew_limit_ > 0) || !std::isfinite(slew_limit_)) {
      throw std::runtime_error("Bottom-up CTS requires a positive slew limit");
    }
    prepareWireModel();
    const double max_length
        = options_->getMaxWl() > 0
              ? std::min(options_->getMaxWl(), options_->getBufferDistance())
              : options_->getBufferDistance();
    const BufferingSearchModel model{
        cap_per_dbu_, res_per_dbu_, max_length, 2 * sta_->scenes().size(), 16};
    const BufferingChoices choices
        = [this](const auto& load, const auto& fixed) {
            return cellChoices(load, fixed);
          };
    const BufferingDummy dummy
        = [this](float actual, float ideal, size_t fanout) {
            return dummyLoad(actual, ideal, fanout);
          };
    auto reference = evaluateReference(model, dummy);
    const size_t initial_size = nodes_.size();
    prepareSites();
    const auto plan = choosePlan(model, choices, dummy, reference);
    nodes_ = plan.nodes;
    restoreLocations();
    if (!plan.reference) {
      writeTree(plan.depth);
    }
    reportPlan(plan, reference, initial_size);
  }

 private:
  void reportPlan(const BufferingPlan& plan,
                  const std::optional<BufferingPlan>& reference,
                  size_t initial_size)
  {
    options_->getLogger()->info(
        utl::CTS,
        555,
        "Bottom-up buffered {}: {} plan, {} cells, depth {}, "
        "{} wire-only junctions, frontier <= {} ({} added sites).",
        clock_->getName(),
        plan.reference ? "legacy reference" : "bounded search",
        plan.buffers,
        plan.depth,
        std::ranges::count_if(
            nodes_,
            [](const auto& node) { return !node.sink && node.master.empty(); }),
        plan.peak_candidates,
        nodes_.size() - initial_size);
    if (reference) {
      options_->getLogger()->info(
          utl::CTS,
          561,
          "Bottom-up surrogate timing for {}: reference latest {:.3f} ps, "
          "spread {:.3f} ps, depth {}; selected latest {:.3f} ps, spread "
          "{:.3f} ps, depth {}. These are model values, not STA measurements.",
          clock_->getName(),
          reference->delay() * 1e12,
          reference->skew() * 1e12,
          reference->depth,
          plan.delay() * 1e12,
          plan.skew() * 1e12,
          plan.depth);
    } else {
      options_->getLogger()->info(
          utl::CTS,
          564,
          "Bottom-up surrogate timing for {}: reference guard inactive; "
          "selected latest {:.3f} ps, spread {:.3f} ps, depth {}. "
          "These are model values, not STA measurements.",
          clock_->getName(),
          plan.delay() * 1e12,
          plan.skew() * 1e12,
          plan.depth);
    }
  }

  std::optional<BufferingPlan> evaluateReference(
      const BufferingSearchModel& model,
      const BufferingDummy& dummy)
  {
    BufferingRejection rejection;
    CellRejection cell_rejection;
    const BufferingChoices choices
        = [this, &cell_rejection](const auto& load, const auto& fixed) {
            cell_rejection = {};
            return cellChoices(load, fixed, &cell_rejection);
          };
    auto reference
        = evaluateBuffering(nodes_, root_, model, choices, dummy, &rejection);
    if (!reference) {
      const auto& load = rejection.load;
      options_->getLogger()->info(
          utl::CTS,
          560,
          "Bottom-up legacy reference unavailable for {}: {} at site {} "
          "({}), load {:.3f} fF, fanout {}, wire span {:.3f} um (limit {:.3f} "
          "um); "
          "maxWl {:.3f} um, buffer distance {:.3f} um, simple segments {}.",
          clock_->getName(),
          rejection.reason,
          rejection.node,
          nodes_.at(rejection.node).master,
          load.capacitance() * 1e15,
          load.fanout,
          load.max_wire_length / options_->getDbUnits(),
          model.max_wire_length / options_->getDbUnits(),
          static_cast<double>(options_->getMaxWl()) / options_->getDbUnits(),
          static_cast<double>(options_->getBufferDistance())
              / options_->getDbUnits(),
          options_->isSimpleSegmentEnabled());
      if (!cell_rejection.constraint.empty()) {
        options_->getLogger()->info(utl::CTS,
                                    563,
                                    "Bottom-up reference cell rejection: {}, "
                                    "value {:.6g}, limit {:.6g}, "
                                    "scene {}, transition {}; common "
                                    "input/output slew budget {:.3f} ps.",
                                    cell_rejection.constraint,
                                    cell_rejection.value,
                                    cell_rejection.limit,
                                    cell_rejection.scene,
                                    cell_rejection.transition,
                                    slew_limit_ * 0.5e12);
      }
    }
    return reference;
  }

  void prepareSites()
  {
    // A common footprint prevents resizing from invalidating the load of an
    // already selected child. All coordinates are physical dbu, not LUT bins.
    for (auto& node : nodes_) {
      if (!node.sink) {
        builder_->uncommitLoc(Point<double>(node.point.x / length_unit_,
                                            node.point.y / length_unit_));
      }
    }
    for (auto& node : nodes_) {
      if (!node.sink) {
        node.point = legalize(node.point);
      }
    }
  }

  BufferingPlan choosePlan(const BufferingSearchModel& model,
                           const BufferingChoices& choices,
                           const BufferingDummy& dummy,
                           const std::optional<BufferingPlan>& reference)
  {
    // Expansion chooses only one electrical seed. Its fastest child can
    // overload a fixed parent even when a smaller child or bypass is legal.
    // Keep the complete original-site search outside that seed's frontier.
    std::optional<BufferingPlan> best;
    try {
      best = searchBuffering(nodes_, root_, model, choices, dummy, reference);
    } catch (const std::runtime_error&) {
      // A legal plan may require sites that the bounded expansion adds.
    }
    BottomUpBuffering strategy(
        cap_per_dbu_,
        model.max_wire_length,
        [this](const auto& load, const auto& fixed) {
          return select(load, fixed);
        },
        [this](auto point) { return legalize(point); });
    try {
      nodes_ = strategy.run(nodes_, root_);
      // The expander updates electrical input caps, but its generic choice
      // has no nominal-cap field. Synchronize the unchanged dummy policy's
      // nominal loads after seed resizing and insertion.
      for (auto& node : nodes_) {
        if (!node.sink) {
          node.nominal_input_cap
              = std::ranges::find(cells_, node.master, &BufferCell::name)
                    ->nominal_input_cap;
        }
      }
      auto expanded
          = searchBuffering(nodes_, root_, model, choices, dummy, reference);
      const auto rank = [](const BufferingPlan& plan) {
        return std::tuple(
            plan.delay(), plan.skew(), plan.depth, plan.buffers, plan.area);
      };
      if (!best || rank(expanded) < rank(*best)) {
        best = std::move(expanded);
      }
    } catch (const std::runtime_error&) {
      if (!best) {
        throw;
      }
    }
    const auto& expansion = strategy.expansion();
    if (expansion.length_splits + expansion.electrical_splits > 0) {
      options_->getLogger()->info(
          utl::CTS,
          562,
          "Bottom-up expansion for {}: {} length and {} electrical split "
          "attempts; "
          "first at site {}, reason {}, load {:.3f} fF, fanout {}, "
          "span {:.3f} um (limit {:.3f} um).",
          clock_->getName(),
          expansion.length_splits,
          expansion.electrical_splits,
          expansion.first_node,
          expansion.first_length ? "wire length"
                                 : "cell electrical constraints",
          expansion.first_load.capacitance() * 1e15,
          expansion.first_load.fanout,
          expansion.first_load.max_wire_length / options_->getDbUnits(),
          model.max_wire_length / options_->getDbUnits());
    }
    return std::move(*best);
  }

  void addCell(const std::string& name)
  {
    const auto* max = sta::MinMax::max();
    auto* master = db_->findMaster(name.c_str());
    auto* cell = network_->findLibertyCell(name.c_str());
    if (master == nullptr || cell == nullptr || !cell->isBuffer()) {
      throw std::runtime_error("Bottom-up CTS requires a buffer: " + name);
    }
    sta::LibertyPort *input, *output;
    cell->bufferPorts(input, output);
    if (footprint_ == nullptr || master->getWidth() > footprint_->getWidth()) {
      footprint_ = master;
    }
    auto* input_port
        = network_->dbToSta(master->findMTerm(input->name().c_str()));
    auto* output_port
        = network_->dbToSta(master->findMTerm(output->name().c_str()));
    tightenSlew(input, input_port, sta_, slew_limit_);
    tightenSlew(output, output_port, sta_, slew_limit_);
    double max_cap = std::numeric_limits<double>::infinity();
    for (const auto* scene : sta_->scenes()) {
      const auto* scene_output = output->scenePort(scene, max);
      if (scene_output == nullptr) {
        throw std::runtime_error("Missing scene data for CTS buffer " + name);
      }
      float cap;
      bool exists;
      scene_output->capacitanceLimit(max, cap, exists);
      if (!exists) {
        scene_output->libertyLibrary()->defaultMaxCapacitance(cap, exists);
      }
      if (exists) {
        max_cap = std::min(max_cap, static_cast<double>(cap));
      }
    }
    for (auto* mode : sta_->modes()) {
      float cap;
      bool exists;
      mode->sdc()->capacitanceLimit(output_port, max, cap, exists);
      if (exists) {
        max_cap = std::min(max_cap, static_cast<double>(cap));
      }
      mode->sdc()->capacitanceLimit(
          network_->cell(network_->topInstance()), max, cap, exists);
      if (exists) {
        max_cap = std::min(max_cap, static_cast<double>(cap));
      }
    }
    const auto buffers = options_->getBufferList();
    cells_.push_back({name,
                      output,
                      inputCap(input, sta_),
                      input->capacitance(),
                      max_cap,
                      cell->area(),
                      fanout_limit_(name),
                      std::ranges::find(buffers, name) != buffers.end()});
  }

  void prepareCells()
  {
    auto names = options_->getBufferList();
    names.push_back(options_->getRootBuffer());
    names.push_back(options_->getSinkBuffer());
    if (!options_->getTreeBuffer().empty()) {
      names.push_back(options_->getTreeBuffer());
    }
    std::ranges::sort(names);
    names.erase(std::unique(names.begin(), names.end()), names.end());
    for (const auto& name : names) {
      addCell(name);
    }
    for (const auto& name : names) {
      if (db_->findMaster(name.c_str())->getHeight()
          != footprint_->getHeight()) {
        throw std::runtime_error(
            "Bottom-up CTS requires equal-height buffer masters");
      }
    }
    std::ranges::sort(cells_, [](const auto& a, const auto& b) {
      return std::tie(a.area, a.name) < std::tie(b.area, b.name);
    });
  }

  void addInst(ClockInst& inst)
  {
    indices_[&inst] = nodes_.size();
    instances_.push_back(&inst);
    BufferingNode node;
    node.point
        = {static_cast<double>(inst.getX()), static_cast<double>(inst.getY())};
    node.sink = !inst.isClockBuffer();
    if (node.sink) {
      auto* port
          = network_->libertyPort(network_->dbToSta(inst.getDbInputPin()));
      if (port == nullptr) {
        throw std::runtime_error("No Liberty capacitance for CTS sink "
                                 + inst.getName());
      }
      node.input_cap = inputCap(port, sta_);
      node.nominal_input_cap = port->capacitance();
      for (const auto* scene : sta_->scenes()) {
        for (const auto* rf : {sta::RiseFall::rise(), sta::RiseFall::fall()}) {
          double early = 0, late = 0;
          if (options_->insertionDelayEnabled()) {
            const auto* min_port = port->scenePort(scene, sta::MinMax::min());
            if (min_port == nullptr) {
              throw std::runtime_error(
                  "Missing scene data for CTS sink insertion delay");
            }
            early = min_port->clkTreeDelay(0.0, rf, sta::MinMax::min());
            late = port->scenePort(scene, sta::MinMax::max())
                       ->clkTreeDelay(0.0, rf, sta::MinMax::max());
          }
          node.sink_arrivals.emplace_back(std::min(early, late),
                                          std::max(early, late));
        }
      }
      tightenSlew(port,
                  network_->port(network_->dbToSta(inst.getDbInputPin())),
                  sta_,
                  slew_limit_);
    } else if (inst.getName() == "clkbuf_0_" + clock_->getName()) {
      // Keep tree interfaces fixed, including separate macro/register trees.
      node.fixed_master = options_->getRootBuffer();
    } else if (builder_->isLeafBuffer(&inst)
               && !options_->isSinkBufferInferred()) {
      node.fixed_master = options_->getSinkBuffer();
    } else if (!options_->getTreeBuffer().empty()) {
      node.fixed_master = options_->getTreeBuffer();
    }
    if (!node.sink) {
      node.master = inst.getMaster();
      const auto cell
          = std::ranges::find(cells_, node.master, &BufferCell::name);
      if (cell == cells_.end()) {
        throw std::runtime_error(
            "Legacy CTS cell missing from candidate library");
      }
      node.input_cap = cell->input_cap;
      node.nominal_input_cap = cell->nominal_input_cap;
    }
    nodes_.push_back(std::move(node));
  }

  void readTree()
  {
    clock_->forEachClockBuffer([this](ClockInst& inst) { addInst(inst); });
    clock_->forEachSink([this](ClockInst& inst) { addInst(inst); });
    clock_->forEachSubNet([this](const ClockSubNet& net) {
      const size_t driver = indices_.at(net.getDriver());
      net.forEachSink([&](ClockInst* sink) {
        nodes_[driver].children.push_back(indices_.at(sink));
      });
    });
    root_
        = indices_.at(clock_->findClockByName("clkbuf_0_" + clock_->getName()));
  }

  void tightenModeSlew()
  {
    const auto* max = sta::MinMax::max();
    for (auto* mode : sta_->modes()) {
      float limit;
      bool exists;
      auto* sdc = mode->sdc();
      sdc->slewLimit(
          network_->cell(network_->topInstance()), max, limit, exists);
      if (exists) {
        slew_limit_ = std::min(slew_limit_, static_cast<double>(limit));
      }
      for (const auto* clock : sdc->clocks()) {
        if (clock_->getSdcName() != clock->name()) {
          continue;
        }
        for (const auto* rf : {sta::RiseFall::rise(), sta::RiseFall::fall()}) {
          sdc->slewLimit(
              clock, rf, sta::PathClkOrData::clk, max, limit, exists);
          if (exists) {
            slew_limit_ = std::min(slew_limit_, static_cast<double>(limit));
          }
        }
      }
    }
  }

  void prepareWireModel()
  {
    const double scale = 1e-6 / options_->getDbUnits();
    const auto bound = wireRCBound(parasitics_, sta_);
    cap_per_dbu_ = bound.capacitance * scale;
    res_per_dbu_ = bound.resistance * scale;
  }

  std::vector<BufferingCellChoice> cellChoices(const BufferingLoad& load,
                                               const std::string& fixed,
                                               CellRejection* rejection
                                               = nullptr)
  {
    std::vector<BufferingCellChoice> result;
    // Use a realizable common input-slew bound, with margin for placement RC.
    const double slew_budget = slew_limit_ * 0.5;
    // Upper-bound the farthest receiver's wire degradation with the complete
    // star-net capacitance rather than a fictitious LUT load bucket.
    const double wire_slew
        = 2.2 * res_per_dbu_ * load.max_wire_length * load.capacitance();
    const std::string target
        = fixed.empty() ? options_->getTreeBuffer() : fixed;
    const auto reject = [rejection](const std::string& constraint,
                                    double value,
                                    double limit,
                                    const std::string& scene = "",
                                    size_t transition = 0) {
      if (rejection && rejection->constraint.empty()) {
        *rejection = {constraint, value, limit, scene, transition};
      }
    };
    for (const auto& cell : cells_) {
      if ((!target.empty() && cell.name != target)
          || (target.empty() && !cell.in_buffer_list)) {
        continue;
      }
      if (load.capacitance() > cell.max_cap) {
        reject("max capacitance (fF)",
               load.capacitance() * 1e15,
               cell.max_cap * 1e15);
        continue;
      }
      if (cell.max_fanout > 0
          && load.fanout > static_cast<size_t>(cell.max_fanout)) {
        reject("max fanout", load.fanout, cell.max_fanout);
        continue;
      }
      bool feasible = true;
      BufferingCellChoice choice{
          {cell.name, cell.input_cap}, cell.nominal_input_cap, cell.area, {}};
      const sta::Slew input_slews[]
          = {static_cast<float>(slew_budget), static_cast<float>(slew_budget)};
      for (const auto* scene : sta_->scenes()) {
        sta::ArcDelay delays[2];
        sta::Slew slews[2];
        resizer_->gateDelays(cell.output,
                             load.capacitance(),
                             input_slews,
                             scene,
                             sta::MinMax::max(),
                             delays,
                             slews);
        for (size_t rf = 0; rf < 2; ++rf) {
          const double value = sta::delayAsFloat(slews[rf]) + wire_slew;
          const double delay = sta::delayAsFloat(delays[rf]);
          if (!std::isfinite(value) || value <= 0 || value > slew_budget) {
            reject("output slew budget (ps)",
                   value * 1e12,
                   slew_budget * 1e12,
                   scene->name(),
                   rf);
          } else if (!std::isfinite(delay) || delay < 0) {
            reject(
                "invalid gate delay (ps)", delay * 1e12, 0, scene->name(), rf);
          }
          feasible &= std::isfinite(value) && value > 0 && value <= slew_budget;
          feasible &= std::isfinite(delay) && delay >= 0;
          choice.delays.push_back(delay);
        }
      }
      if (feasible) {
        result.push_back(std::move(choice));
      }
    }
    return result;
  }

  std::optional<BufferingChoice> select(const BufferingLoad& load,
                                        const std::string& fixed)
  {
    auto choices = cellChoices(load, fixed);
    if (!choices.empty()) {
      const auto best
          = std::ranges::min_element(choices, {}, [](const auto& cell) {
              return std::tuple(*std::ranges::max_element(cell.delays),
                                cell.area,
                                cell.buffer.master);
            });
      return best->buffer;
    }
    return std::nullopt;
  }

  std::optional<BufferingCellChoice> dummyLoad(float actual,
                                               float ideal,
                                               size_t fanout)
  {
    auto* cell = dummy_selector_(actual, ideal, fanout);
    if (cell == nullptr) {
      return std::nullopt;
    }
    sta::LibertyPort *input, *output;
    cell->bufferPorts(input, output);
    return BufferingCellChoice{{cell->name(), inputCap(input, sta_)},
                               input->capacitance(),
                               cell->area(),
                               {}};
  }

  BufferingPoint legalize(BufferingPoint point)
  {
    const Point<double> old(point.x / length_unit_, point.y / length_unit_);
    const auto legal = builder_->legalizeOneBuffer(old, footprint_->getName());
    builder_->commitLoc(legal);
    committed_sites_.push_back(
        {legal.getX() * length_unit_, legal.getY() * length_unit_});
    return {std::round(legal.getX() * length_unit_),
            std::round(legal.getY() * length_unit_)};
  }

  void restoreLocations()
  {
    for (const auto point : committed_sites_) {
      builder_->uncommitLoc(
          Point<double>(point.x / length_unit_, point.y / length_unit_));
    }
    for (const auto& node : nodes_) {
      if (!node.sink && !node.master.empty()) {
        builder_->commitLoc(Point<double>(node.point.x / length_unit_,
                                          node.point.y / length_unit_));
      }
    }
  }

  std::vector<size_t> frontier(size_t index) const
  {
    std::vector<size_t> result;
    std::vector<size_t> pending(nodes_[index].children.rbegin(),
                                nodes_[index].children.rend());
    while (!pending.empty()) {
      const auto child = pending.back();
      pending.pop_back();
      if (nodes_[child].sink || !nodes_[child].master.empty()) {
        result.push_back(child);
      } else {
        pending.insert(pending.end(),
                       nodes_[child].children.rbegin(),
                       nodes_[child].children.rend());
      }
    }
    return result;
  }

  void writeTree(unsigned depth)
  {
    // Sink objects remain stable. Remove all old buffer pointers before
    // rebuilding the chosen plan; virtual junctions never become DB cells.
    builder_->clearBuffering(depth);
    instances_.resize(nodes_.size(), nullptr);
    for (size_t index = 0; index < nodes_.size(); ++index) {
      const auto& node = nodes_[index];
      if (node.sink) {
        continue;
      }
      instances_[index] = nullptr;
      if (node.master.empty()) {
        continue;
      }
      auto& inst = clock_->addClockBuffer(
          index == root_ ? "clkbuf_0"
                         : "clkbuf_bottom_up_" + std::to_string(index),
          node.master,
          node.point.x,
          node.point.y);
      inst.setInputCap(node.input_cap);
      builder_->addTreeLevelBuffer(&inst);
      instances_[index] = &inst;
    }
    // Root-first order lets the DB writer discover driving nets correctly.
    std::function<void(size_t)> connect = [&](size_t index) {
      const auto& node = nodes_[index];
      if (node.sink) {
        return;
      }
      auto* inst = instances_[index];
      auto& net = clock_->addSubNet(
          index == root_ ? "clknet_0"
                         : "clknet_bottom_up_" + std::to_string(index));
      net.addInst(*inst);
      const auto children = frontier(index);
      bool leaf = true;
      for (const size_t child : children) {
        net.addInst(*instances_[child]);
        leaf &= nodes_[child].sink;
      }
      net.setLeafLevel(leaf);
      if (leaf) {
        builder_->addFirstLevelSinkDriver(inst);
      }
      for (const size_t child : children) {
        connect(child);
      }
    };
    connect(root_);
  }

  CtsOptions* options_;
  odb::dbDatabase* db_;
  sta::dbNetwork* network_;
  sta::dbSta* sta_;
  rsz::Resizer* resizer_;
  est::EstimateParasitics* parasitics_;
  double length_unit_;
  FanoutLimit fanout_limit_;
  DummySelector dummy_selector_;
  TreeBuilder* builder_ = nullptr;
  Clock* clock_ = nullptr;
  odb::dbMaster* footprint_ = nullptr;
  std::vector<BufferCell> cells_;
  std::vector<BufferingNode> nodes_;
  std::vector<BufferingPoint> committed_sites_;
  std::vector<ClockInst*> instances_;
  std::map<ClockInst*, size_t> indices_;
  size_t root_ = 0;
  double slew_limit_ = 0;
  double cap_per_dbu_ = 0;
  double res_per_dbu_ = 0;
};
}  // namespace

void TritonCTS::bufferClockTreeBottomUp(TreeBuilder* builder)
{
  try {
    std::vector<sta::LibertyCell*> dummy_cells;
    if (options_->dummyLoadEnabled()) {
      findCandidateDummyCells(dummy_cells);
    }
    ClockBuffering buffering(
        options_,
        db_,
        network_,
        openSta_,
        resizer_,
        estimate_parasitics_,
        techChar_->getLengthUnit(),
        [this](const auto& name) { return getBufferFanoutLimit(name); },
        [this, &dummy_cells](float actual, float ideal, size_t fanout) {
          if (!options_->dummyLoadEnabled() || sta::fuzzyEqual(actual, ideal)
              || fanout == options_->getMaxFanout()) {
            return static_cast<sta::LibertyCell*>(nullptr);
          }
          return findBestDummyCell(dummy_cells, ideal - actual);
        });
    buffering.run(builder);
  } catch (const std::exception& error) {
    logger_->error(utl::CTS,
                   554,
                   "Bottom-up buffering failed for {}: {}.",
                   builder->getClock().getName(),
                   error.what());
  }
}

BufferingWireRC TritonCTS::validateBottomUpWireRC()
{
  BufferingWireRC bound;
  try {
    bound = wireRCBound(estimate_parasitics_, openSta_);
  } catch (const std::exception& error) {
    logger_->error(
        utl::CTS, 558, "Invalid bottom-up CTS wire RC: {}.", error.what());
  }
  // Do not bypass placement / congestion / source validation by directly
  // requesting kPlacement from the estimator.
  estimate_parasitics_->designParasiticsSrc();
  return bound;
}

void TritonCTS::checkClockPinElectricals(const sta::Pin* pin,
                                         bool driver,
                                         BufferingAudit& audit)
{
  float value, limit, slack;
  const sta::RiseFall* rf;
  const sta::Scene* scene;
  sta::Slew slew;
  openSta_->checkSlew(pin,
                      openSta_->scenes(),
                      sta::MinMax::max(),
                      true,
                      slew,
                      limit,
                      slack,
                      rf,
                      scene);
  audit.slew_violations += slack < 0;
  if (slack < 0 && audit.first_slew_violation.empty()) {
    audit.first_slew_violation = network_->pathName(pin);
    audit.first_slew_scene = scene->name();
  }
  if (driver) {
    openSta_->checkCapacitance(pin,
                               openSta_->scenes(),
                               sta::MinMax::max(),
                               value,
                               limit,
                               slack,
                               rf,
                               scene);
    audit.cap_violations += slack < 0;
    if (slack < 0 && audit.first_cap_violation.empty()) {
      audit.first_cap_violation = network_->pathName(pin);
    }
  }
}

BufferingAudit TritonCTS::checkClockTreeElectricals()
{
  validateBottomUpWireRC();
  estimate_parasitics_->estimateParasitics(est::ParasiticsSrc::kPlacement);
  openSta_->checkSlewsPreamble();
  openSta_->checkCapacitancesPreamble(openSta_->scenes());
  BufferingAudit audit;
  odb::PtrSet<odb::dbNet> visited;
  odb::PtrSet<odb::dbITerm> reached_sinks;
  std::vector<odb::dbNet*> pending;
  for (const auto& builder : builders_) {
    if (builder->getParent() == nullptr) {
      pending.push_back(builder->getClock().getNetObj());
    }
  }
  while (!pending.empty()) {
    auto* net = pending.back();
    pending.pop_back();
    if (!visited.insert(net).second) {
      continue;
    }
    ++audit.checked_nets;
    for (auto* iterm : net->getITerms()) {
      checkClockPinElectricals(
          network_->dbToSta(iterm), iterm->isOutputSignal(), audit);
      if (!iterm->isInputSignal()) {
        continue;
      }
      reached_sinks.insert(iterm);
      if (!isSink(iterm)) {
        auto* cell = network_->libertyCell(network_->dbToSta(iterm->getInst()));
        const bool clock_cell = cell != nullptr
                                && (cell->isBuffer() || cell->isInverter()
                                    || cell->isClockGate());
        // getSingleOutput() is discovery-only: it filters staClockNets_,
        // which predates CTS and does not contain clknet_* or delay nets.
        for (auto* output : iterm->getInst()->getITerms()) {
          auto* next = output->getNet();
          if (output->isOutputSignal() && next != nullptr
              && (clock_cell || next->getSigType() == odb::dbSigType::CLOCK)) {
            pending.push_back(next);
          }
        }
      }
    }
    for (auto* bterm : net->getBTerms()) {
      checkClockPinElectricals(network_->dbToSta(bterm),
                               bterm->getIoType() == odb::dbIoType::INPUT,
                               audit);
    }
  }
  odb::PtrSet<odb::dbITerm> expected_sinks;
  for (const auto& builder : builders_) {
    builder->getClock().forEachSink([&](const ClockInst& sink) {
      expected_sinks.insert(sink.getDbInputPin());
    });
    builder->getClock().forEachSubNet([&](const ClockSubNet& net) {
      ++audit.expected_nets;
      audit.reached_nets
          += net.getNetObj() != nullptr && visited.contains(net.getNetObj());
    });
  }
  audit.expected_sinks = expected_sinks.size();
  for (auto* sink : expected_sinks) {
    audit.reached_sinks += reached_sinks.contains(sink);
  }
  finishClockTreeElectricals(audit);
  return audit;
}

void TritonCTS::finishClockTreeElectricals(BufferingAudit& audit)
{
  logger_->metric("cts__bottom_up__checked_nets", audit.checked_nets);
  logger_->metric("cts__bottom_up__expected_nets", audit.expected_nets);
  logger_->metric("cts__bottom_up__reached_nets", audit.reached_nets);
  logger_->metric("cts__bottom_up__expected_sinks", audit.expected_sinks);
  logger_->metric("cts__bottom_up__reached_sinks", audit.reached_sinks);
  logger_->metric("cts__bottom_up__cap_violations", audit.cap_violations);
  logger_->metric("cts__bottom_up__slew_violations", audit.slew_violations);
  logger_->info(utl::CTS,
                556,
                "Bottom-up CTS electrical audit: {} nets, {} max-cap and {} "
                "max-slew violations.",
                audit.checked_nets,
                audit.cap_violations,
                audit.slew_violations);
  logger_->info(utl::CTS,
                559,
                "Bottom-up CTS audit coverage: {}/{} original sinks, {}/{} "
                "generated subnets.",
                audit.reached_sinks,
                audit.expected_sinks,
                audit.reached_nets,
                audit.expected_nets);
  if (audit.reached_nets != audit.expected_nets
      || audit.reached_sinks != audit.expected_sinks) {
    audit.error
        = "Bottom-up CTS electrical audit did not reach every original sink "
          "and generated subnet.";
  } else if (audit.cap_violations != 0 || audit.slew_violations != 0) {
    audit.error
        = "Bottom-up CTS failed its electrical audit. First max-cap "
          "violation: "
          + (audit.first_cap_violation.empty() ? "none"
                                               : audit.first_cap_violation)
          + "; first max-slew violation: "
          + (audit.first_slew_violation.empty() ? "none"
                                                : audit.first_slew_violation)
          + (audit.first_slew_scene.empty()
                 ? ""
                 : " in scene " + audit.first_slew_scene)
          + ".";
  }
}
}  // namespace cts
