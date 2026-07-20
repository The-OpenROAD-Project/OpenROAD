// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include "flow/acd_resynth.h"

#include <algorithm>
#include <cassert>
#include <chrono>
#include <cstdio>
#include <limits>
#include <map>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "flow/acd.h"
#include "rsz/Resizer.hh"
#include "sta/Delay.hh"
#include "sta/FuncExpr.hh"
#include "sta/Liberty.hh"
#include "sta/PortDirection.hh"
#include "sta/Transition.hh"
#include "syn/synthesis.h"
#include "utl/timer.h"

namespace syn::acd {

namespace {

static bool isSignalInput(odb::dbITerm* pin)
{
  return pin->getSigType() == odb::dbSigType::SIGNAL
         && pin->getIoType() == odb::dbIoType::INPUT;
}

static bool isSignalOutput(odb::dbITerm* pin)
{
  return pin->getSigType() == odb::dbSigType::SIGNAL
         && pin->getIoType() == odb::dbIoType::OUTPUT;
}

// Compose a cell's Liberty function over per-port truth tables
Truth6 composeFexpr(sta::FuncExpr* f,
                    const int nvars,
                    const std::map<sta::LibertyPort*, Truth6>& port_tt)
{
  using Op = sta::FuncExpr::Op;
  switch (f->op()) {
    case Op::port: {
      auto it = port_tt.find(f->port());
      return it == port_tt.end() ? 0 : it->second;
    }
    case Op::not_:
      return mask6(nvars) & ~composeFexpr(f->left(), nvars, port_tt);
    case Op::or_:
      return composeFexpr(f->left(), nvars, port_tt)
             | composeFexpr(f->right(), nvars, port_tt);
    case Op::and_:
      return composeFexpr(f->left(), nvars, port_tt)
             & composeFexpr(f->right(), nvars, port_tt);
    case Op::xor_:
      return composeFexpr(f->left(), nvars, port_tt)
             ^ composeFexpr(f->right(), nvars, port_tt);
    case Op::one:
      return mask6(nvars);
    case Op::zero:
      return 0;
  }
  return 0;
}

}  // namespace

TruthTable evaluateFunction(const GateNetwork& net)
{
  std::vector<Truth6> node_tt(net.nodes.size());
  for (size_t ni = 0; ni < net.nodes.size(); ni++) {
    const GateNode& node = net.nodes[ni];
    std::map<sta::LibertyPort*, Truth6> cell_inputs;
    sta::LibertyCellPortIterator it(node.driver_port->libertyCell());
    int idx = 0;
    while (it.hasNext()) {
      sta::LibertyPort* p = it.next();
      if (p->direction()->isInput() && !p->isPwrGnd()) {
        const auto& fanin = node.fanins[idx++];
        cell_inputs[p]
            = fanin.first ? (cofactor_masks[fanin.second] & mask6(net.ninputs))
                          : node_tt[fanin.second];
      }
    }
    node_tt[ni]
        = composeFexpr(node.driver_port->function(), net.ninputs, cell_inputs);
  }

  std::vector<int> variables(net.ninputs);
  for (int i = 0; i < net.ninputs; ++i) {
    variables[i] = i;
  }
  TruthTable result(std::move(variables), net.outs.size());
  const int nminterms = 1 << net.ninputs;
  for (size_t i = 0; i < net.outs.size(); i++) {
    const auto& [is_pi, idx] = net.outs[i];
    const Truth6 tt
        = is_pi ? (cofactor_masks[idx] & mask6(net.ninputs)) : node_tt[idx];
    for (int j = 0; j < nminterms; j++) {
      result.setValue(i, j, ((tt >> j) & 1) != 0);
    }
  }
  return result;
}

Resynthesis::Resynthesis(utl::Logger* logger,
                         sta::dbSta* sta,
                         rsz::Resizer* resizer,
                         Synthesis* synthesis)
    : logger_(logger), resizer_(resizer), synthesis_(synthesis)
{
  dbStaState::init(sta);
  db_network_ = sta->getDbNetwork();
}

Resynthesis::~Resynthesis()
{
}

void Resynthesis::init()
{
  corner_ = sta_->cmdScene();

  // Delay estimation assumes a fixed input slew rather than propagating one.
  // Borrow the 'target slew' value from resizer.
  resizer_->resizePreamble();
  fixed_slew_[sta::RiseFall::riseIndex()]
      = resizer_->targetSlew(sta::RiseFall::rise());
  fixed_slew_[sta::RiseFall::fallIndex()]
      = resizer_->targetSlew(sta::RiseFall::fall());

  if (fixed_slew_[0] <= 0.0f || fixed_slew_[1] <= 0.0f) {
    logger_->error(utl::SYN,
                   61,
                   "No buffer target slew available; delay estimation would "
                   "be meaningless.");
  }

  // `buildIndex` appends, so start from nothing to stay idempotent
  index_ = cm::TargetIndex();
  cm::buildIndex(db_network_, index_, logger_, *synthesis_);

  // Both of these outlive any one cut: the match cache is the point of
  // keeping them, since cuts across a design keep asking after the same
  // handful of functions.
  match_cache_.emplace(logger_, index_, kMaxBoundVars);
  dparams_.corner = corner_;
  dparams_.fixed_slews[0] = fixed_slew_[0];
  dparams_.fixed_slews[1] = fixed_slew_[1];
  dparams_.nand_delay
      = findDelayLowerBound(corner_, match_cache_->nand2(), fixed_slew_);

  // A NAND2 is about the cheapest thing that does anything, so a fraction of
  // one is the scale below which a slack difference is noise: pin swaps and
  // the like, not logic worth rebuilding for.
  epsilon_ = dparams_.nand_delay / 8;
}

int Resynthesis::nusers(const sta::Pin* pin)
{
  odb::dbNet* net = db_network_->flatNet(pin);
  if (!net) {
    return 0;
  }
  // Everything on the net save for `pin` itself, which is driving it.  Top-
  // level ports count: a value escaping through one is a user like any other.
  const int npins = net->getITerms().size() + net->getBTerms().size();
  assert(npins >= 1);
  return npins - 1;
}

odb::dbITerm* Resynthesis::asDriverITerm(const sta::Pin* pin)
{
  odb::dbITerm* iterm;
  odb::dbBTerm* bterm;
  odb::dbModITerm* moditerm;
  db_network_->staToDb(pin, iterm, bterm, moditerm);
  return iterm;
}

const sta::Pin* Resynthesis::driverOf(odb::dbITerm* input)
{
  odb::dbNet* net = input->getNet();
  if (!net) {
    return nullptr;
  }
  sta::PinSet* drivers = db_network_->drivers(db_network_->dbToSta(net));
  if (!drivers || drivers->size() != 1) {
    return nullptr;
  }
  return *drivers->begin();
}

const sta::Pin* Resynthesis::siblingOutput(odb::dbITerm* driver)
{
  for (odb::dbITerm* pin : driver->getInst()->getITerms()) {
    if (pin != driver && isSignalOutput(pin)) {
      return db_network_->dbToSta(pin);
    }
  }
  return nullptr;
}

bool Resynthesis::isMultiOutput(const sta::Pin* pin)
{
  odb::dbITerm* driver = asDriverITerm(pin);
  return driver && siblingOutput(driver);
}

void Resynthesis::resynthesize(const int max_leaves,
                               const int max_intermediate_leaves,
                               const int max_cells,
                               const int max_outerfans,
                               const bool exclude_buffers,
                               const bool allow_lateral,
                               const float timing_opt_effort,
                               const bool apply)
{
  stats_ = Stats();
  init();

  {
    utl::DebugScopedTimer timer(stats_.t_sta);
    sta_->findRequireds();
    sta::Vertex* worst_vertex;
    sta::Slack worst_slack;
    sta_->worstSlack(sta::MinMax::max(), worst_slack, worst_vertex);
    wns_ = worst_slack;
  }

  max_leaves_ = max_leaves;
  max_intermediate_leaves_ = max_intermediate_leaves;
  max_cells_ = max_cells;
  max_outerfans_ = max_outerfans;
  exclude_buffers_ = exclude_buffers;
  allow_lateral_ = allow_lateral;
  timing_opt_effort_ = timing_opt_effort;
  substitutions_.clear();

  odb::dbBlock* block = db_network_->block();
  if (!block) {
    logger_->error(utl::SYN, 60, "No block. Run synthesize first.");
    return;
  }

  // Take the heads up front: once evaluateCut starts editing the netlist it
  // is no longer sound to walk the instance list as we go.
  std::vector<const sta::Pin*> heads;
  for (odb::dbInst* inst : block->getInsts()) {
    for (odb::dbITerm* pin : inst->getITerms()) {
      if (isSignalOutput(pin)) {
        sta::Pin* sta_pin = db_network_->dbToSta(pin);
        if (sta_->slack(graph_->pinDrvrVertex(sta_pin), sta::MinMax::max())
            < 0) {
          heads.push_back(sta_pin);
        }
      }
    }
  }

  debugPrint(logger_,
             utl::SYN,
             "acd_resynth",
             1,
             "resynthesizing around {} gate outputs",
             heads.size());

  stats_.heads = (long long) heads.size();
  {
    utl::DebugScopedTimer timer(stats_.t_enumerate);
    for (const sta::Pin* head : heads) {
      resynthesizePin(head);
    }
  }

  float gain = 0;
  for (const Substitution& s : substitutions_) {
    gain += s.gain;
  }
  debugPrint(logger_,
             utl::SYN,
             "acd_resynth",
             1,
             "{} substitutions to be had, {} of gain between them",
             substitutions_.size(),
             sta::delayAsString(gain, 6, sta_));

  if (apply) {
    commit();
  }
  reportStats();
}

void Resynthesis::reportStats()
{
  debugPrint(logger_,
             utl::SYN,
             "acd_resynth",
             1,
             "counts: {} heads, {} expansions, {} cuts, {} syntheses, {} "
             "forward scans, {} taken in; {} explores ({} worst)",
             stats_.heads,
             stats_.expansions,
             stats_.cuts,
             stats_.syntheses,
             stats_.forward_scans,
             stats_.taken_in,
             stats_.explores,
             stats_.worst_explore);
  debugPrint(logger_,
             utl::SYN,
             "acd_resynth",
             1,
             "seconds: sta {:.2f}, enumerate {:.2f} (forward "
             "{:.2f}, cut timing {:.2f}, synthesize {:.2f}, slack {:.2f}), "
             "commit {:.2f}",
             stats_.t_sta,
             stats_.t_enumerate,
             stats_.t_forward,
             stats_.t_cut_timing,
             stats_.t_synthesize,
             stats_.t_slack,
             stats_.t_commit);
}

void Resynthesis::resynthesizePin(const sta::Pin* head)
{
  head_ = head;
  pin_best_.reset();

  ConePins cone(PinIdLess{db_network_});
  cone.insert(head);
  expandCut(
      0, {head}, 0, 0, 1, cone, {{head, nusers(head)}}, {{head, nusers(head)}});

  if (logger_->debugCheck(utl::SYN, "acd_resynth", 1)) {
    if (pin_best_) {
      debugPrint(logger_,
                 utl::SYN,
                 "acd_resynth",
                 1,
                 "pin best at {}: gain {}, area {:.2f} -> {:.2f}",
                 db_network_->pathName(head_),
                 sta::delayAsString(pin_best_->gain, 6, sta_),
                 networkArea(pin_best_->original),
                 networkArea(pin_best_->replacement));
      debugPrint(logger_,
                 utl::SYN,
                 "acd_resynth",
                 1,
                 "  -{}",
                 formatGateNetwork(pin_best_->original, pin_best_->leaves));
      debugPrint(logger_,
                 utl::SYN,
                 "acd_resynth",
                 1,
                 "  +{}",
                 formatGateNetwork(pin_best_->replacement, pin_best_->leaves));
    }
  }

  if (pin_best_) {
    substitutions_.push_back(std::move(*pin_best_));
  }
}

bool Resynthesis::applySubstitution(const Substitution& s, ConePins& obsoleted)
{
  odb::dbBlock* block = db_network_->block();

  std::vector<odb::dbNet*> leaf_nets;
  leaf_nets.reserve(s.leaves.size());
  for (const sta::Pin* leaf : s.leaves) {
    leaf_nets.push_back(db_network_->flatNet(leaf));
  }

  // The nets the replacement's outputs take over from the gates they stand in
  // for.  Output i of one is output i of the other.
  assert(s.replacement.outs.size() == s.original.outs.size());
  std::unordered_map<int, odb::dbNet*> taken_over;
  for (size_t i = 0; i < s.replacement.outs.size(); i++) {
    const auto& [is_pi, node] = s.replacement.outs[i];
    if (is_pi) {
      // We do not support passthroughs in the replacement
      // network. This should happen rarely. Drop the substitution.
      return false;
    }
    const auto& [orig_is_pi, orig_node] = s.original.outs[i];
    taken_over[node]
        = db_network_->flatNet(s.original.nodes[orig_node].driver_pin);
  }

  std::vector<odb::dbNet*> node_net(s.replacement.nodes.size(), nullptr);
  for (size_t ni = 0; ni < s.replacement.nodes.size(); ni++) {
    const GateNode& node = s.replacement.nodes[ni];
    sta::LibertyCell* cell = node.driver_port->libertyCell();
    odb::dbInst* inst
        = odb::dbInst::create(block,
                              db_network_->staToDb(cell),
                              ("remap" + std::to_string(made_++)).c_str());

    auto it = taken_over.find((int) ni);
    node_net[ni]
        = it != taken_over.end()
              ? it->second
              : odb::dbNet::create(
                    block, ("remap_net" + std::to_string(made_++)).c_str());
    inst->getITerm(db_network_->staToDb(node.driver_port))
        ->connect(node_net[ni]);

    // Fanins go back on in the same walk over the cell's liberty ports they
    // were read off of, which is the order they are held in.
    sta::LibertyCellPortIterator pit(cell);
    int fi = 0;
    while (pit.hasNext()) {
      sta::LibertyPort* port = pit.next();
      if (!port->direction()->isInput() || port->isPwrGnd()) {
        continue;
      }
      const auto& [is_pi, idx] = node.fanins[fi++];
      inst->getITerm(db_network_->staToDb(port))
          ->connect(is_pi ? leaf_nets[idx] : node_net[idx]);
    }
  }

  // The gates it stands in for are spoken for now.  They stay hooked up for
  // the moment: their nets have two drivers until the cleanup gets to them.
  for (const GateNode& node : s.original.nodes) {
    obsoleted.insert(node.driver_pin);
  }

  return true;
}

void Resynthesis::commit()
{
  utl::DebugScopedTimer timer(stats_.t_commit);
  ConePins obsoleted(PinIdLess{db_network_});

  int applied = 0;
  for (const Substitution& s : substitutions_) {
    // Cuts overlap, and the gates this one stands on may already have been
    // taken over by one we put in earlier.  Then it is not ours to replace.
    bool stale = false;
    for (const GateNode& node : s.original.nodes) {
      if (obsoleted.count(node.driver_pin)) {
        stale = true;
        break;
      }
    }
    if (stale) {
      continue;
    }

    if (applySubstitution(s, obsoleted)) {
      applied++;
    }
  }

  // Only now that every substitution has been read is it safe to unhook
  // anything: a later one can name a leaf an earlier one made obsolete, and
  // the pin has to still be there to say which net it meant.
  std::vector<odb::dbInst*> doomed;
  std::unordered_set<odb::dbInst*> seen;
  for (const sta::Pin* p : obsoleted) {
    odb::dbITerm* driver = asDriverITerm(p);
    odb::dbInst* inst = driver->getInst();

    bool all_spoken_for = true;
    for (odb::dbITerm* pin : inst->getITerms()) {
      if (isSignalOutput(pin) && !obsoleted.count(db_network_->dbToSta(pin))) {
        all_spoken_for = false;
        break;
      }
    }

    if (all_spoken_for) {
      if (seen.insert(inst).second) {
        doomed.push_back(inst);
      }
    } else {
      // Something still wants its other output, so the gate stays; this pin
      // just stops driving what it used to.
      driver->disconnect();
    }
  }

  // Walking `obsoleted` reads the pins, so nothing goes until that is done
  for (odb::dbInst* inst : doomed) {
    odb::dbInst::destroy(inst);
  }

  // `dbStaCbk` picks the creates, connects and destroys up off odb, so the
  // timing graph and the driver cache come along on their own.
  logger_->info(utl::SYN,
                64,
                "acd_resynth: applied {} of {} substitutions, {} gates out",
                applied,
                substitutions_.size(),
                doomed.size());
}

bool Resynthesis::expandableDriver(odb::dbITerm* driver)
{
  odb::dbInst* instance = driver->getInst();

  // Detect when instance is not a logic standard cell
  if (instance->getMaster()->getType() != odb::dbMasterType::CORE) {
    return false;
  }

  // Is don't touch
  if (instance->isDoNotTouch()) {
    return false;
  }

  // Is sequential, or lacks Liberty definition
  sta::LibertyCell* cell
      = db_network_->libertyCell(db_network_->dbToSta(instance));
  if (!cell || cell->isSequential()) {
    return false;
  }
  if (exclude_buffers_ && cell->isBuffer()) {
    return false;
  }

  // If it has too many pins, any are inout, or on a don't touch net
  auto pins = instance->getITerms();
  int ninputs = 0, noutputs = 0;
  for (odb::dbITerm* pin : pins) {
    odb::dbNet* net = pin->getNet();

    if (net && net->isDoNotTouch()) {
      // Gate connected to a don't touch net - reject
      return false;
    }

    if (pin->getSigType() == odb::dbSigType::SIGNAL) {
      auto io_type = pin->getIoType();
      if (io_type == odb::dbIoType::INOUT) {
        // Inout pin - reject
        return false;
      }

      if (io_type == odb::dbIoType::INPUT) {
        ninputs++;
        if (ninputs > max_intermediate_leaves_) {
          // Gate with too many inputs - reject
          return false;
        }
      }
      if (io_type == odb::dbIoType::OUTPUT) {
        noutputs++;
        if (noutputs > 2) {
          // MOG with more than two outputs - reject
          return false;
        }

        sta::LibertyPort* port
            = db_network_->libertyPort(db_network_->dbToSta(driver));
        if (!port || !port->function() || port->tristateEnable()) {
          // Port is not two-state with known function
          return false;
        }
      }
    }
  }

  // Any of its inputs are dangling, or not on signal nets
  for (odb::dbITerm* pin : pins) {
    if (pin == driver) {
      continue;
    }

    if (pin->getSigType() != odb::dbSigType::SIGNAL
        || pin->getIoType() != odb::dbIoType::INPUT) {
      continue;
    }

    odb::dbNet* net = pin->getNet();
    if (!net) {
      return false;
    }

    sta::PinSet* drivers = db_network_->drivers(db_network_->dbToSta(net));
    if (drivers->size() != 1) {
      return false;
    }
  }

  return true;
}

bool Resynthesis::containedInMffc(
    const ConePins& drivers_in_cone,
    const std::unordered_map<const sta::Pin*, int>& mffc_users,
    const sta::Pin* pin)
{
  if (!drivers_in_cone.count(pin)) {
    return false;
  }

  auto it = mffc_users.find(pin);
  return it != mffc_users.end() && it->second == nusers(pin);
}

bool Resynthesis::entersMffc(
    const ConePins& drivers_in_cone,
    const std::unordered_map<const sta::Pin*, int>& mffc_users,
    const sta::Pin* pin)
{
  if (isMultiOutput(pin)) {
    // The gate has a second output to answer for.  Unless that one is spoken
    // for as well, the gate stays behind to keep driving it.
    const sta::Pin* sibling = siblingOutput(asDriverITerm(pin));
    if (!containedInMffc(drivers_in_cone, mffc_users, sibling)) {
      return false;
    }
  }

  return containedInMffc(drivers_in_cone, mffc_users, pin);
}

void Resynthesis::refMffc(const ConePins& drivers_in_cone,
                          int& mffc_size,
                          std::unordered_map<const sta::Pin*, int>& mffc_users,
                          const sta::Pin* pin)
{
  odb::dbITerm* driver = asDriverITerm(pin);
  assert(driver);
  mffc_size++;

  for (odb::dbITerm* input : driver->getInst()->getITerms()) {
    if (!isSignalInput(input)) {
      continue;
    }

    const sta::Pin* a = driverOf(input);
    mffc_users[a]++;

    if (entersMffc(drivers_in_cone, mffc_users, a)) {
      refMffc(drivers_in_cone, mffc_size, mffc_users, a);
    }
  }
}

void Resynthesis::collectCone(
    const std::vector<const sta::Pin*>& leaves,
    const ConePins& drivers_in_cone,
    const std::unordered_map<const sta::Pin*, int>& cut_users,
    std::vector<odb::dbInst*>& cone,
    std::vector<const sta::Pin*>& roots,
    GateNetwork& net)
{
  // Drivers in the order they settle in.  A driver only settles once every
  // user it has inside the cut has been walked, so this comes out as the
  // reverse of the topological order `GateNetwork` asks for.
  std::vector<const sta::Pin*> order;
  std::unordered_set<const sta::Pin*> settled;
  std::unordered_map<const sta::Pin*, int> done_users;

  // Nothing inside the cut reads `head_`, so it seeds the walk rather than
  // being discovered by it, and it is an output of the cut by construction.
  roots.push_back(head_);

  auto settle = [&](const sta::Pin* driver_pin) {
    order.push_back(driver_pin);
    settled.insert(driver_pin);

    // Part of the driver's fanout stays outside the cut, so it is an output
    // of it.  (`head_` is already accounted for above.)
    if (driver_pin != head_ && cut_users.at(driver_pin) != nusers(driver_pin)) {
      roots.push_back(driver_pin);
    }

    // Hold a gate back until every output it contributes to the cone has
    // settled: walking its fanins any earlier would let them settle ahead of
    // one of its own outputs and break the ordering `order` is built for.
    // This is also what puts a two-output gate in `cone` exactly once.
    odb::dbITerm* driver = asDriverITerm(driver_pin);
    const sta::Pin* sibling = siblingOutput(driver);
    if (!sibling || !drivers_in_cone.count(sibling) || settled.count(sibling)) {
      cone.push_back(driver->getInst());
    }
  };

  settle(head_);

  // Gates taken in from below sit above the head, so the walk has to start at
  // them too -- going back from the head alone would never reach them.  They
  // are the drivers nothing in the cut reads; one may read another, and then
  // it is not a top.
  for (const sta::Pin* d : drivers_in_cone) {
    if (d != head_ && cut_users.at(d) == 0) {
      settle(d);
    }
  }

  for (int i = 0; i < (int) cone.size(); i++) {
    for (odb::dbITerm* input : cone[i]->getITerms()) {
      if (!isSignalInput(input)) {
        continue;
      }

      const sta::Pin* a = driverOf(input);
      if (!drivers_in_cone.count(a)) {
        continue;
      }

      // Only once every in-cut user of `a` has been walked do we know whether
      // any of its fanout escapes.
      if (++done_users[a] == cut_users.at(a)) {
        settle(a);
      }
    }
  }

  const int nnodes = (int) order.size();
  std::unordered_map<const sta::Pin*, int> node_index;
  for (int i = 0; i < nnodes; i++) {
    node_index[order[nnodes - 1 - i]] = i;
  }

  std::unordered_map<const sta::Pin*, int> leaf_index;
  for (int i = 0; i < (int) leaves.size(); i++) {
    leaf_index[leaves[i]] = i;
  }

  net.ninputs = (int) leaves.size();
  net.nodes.assign(nnodes, GateNode());
  for (int i = 0; i < nnodes; i++) {
    const sta::Pin* driver_pin = order[nnodes - 1 - i];
    GateNode& node = net.nodes[i];
    node.driver_port = db_network_->libertyPort(driver_pin);
    node.driver_pin = driver_pin;
    assert(node.driver_port);
    odb::dbInst* inst = asDriverITerm(driver_pin)->getInst();

    // Fanins are read back off the same walk over the cell's liberty ports,
    // so they have to be recorded in that order rather than in pin order.
    sta::LibertyCellPortIterator it(node.driver_port->libertyCell());
    while (it.hasNext()) {
      sta::LibertyPort* port = it.next();
      if (!port->direction()->isInput() || port->isPwrGnd()) {
        continue;
      }

      odb::dbITerm* input = inst->getITerm(db_network_->staToDb(port));
      assert(input);
      const sta::Pin* a = driverOf(input);
      auto node_it = node_index.find(a);
      node.fanins.push_back(node_it != node_index.end()
                                ? std::make_pair(false, node_it->second)
                                : std::make_pair(true, leaf_index.at(a)));
    }
  }

  // Every root is a driver we expanded, so none of them is a bare leaf
  net.outs.clear();
  net.outs.reserve(roots.size());
  for (const sta::Pin* root : roots) {
    net.outs.push_back({false, node_index.at(root)});
  }
}

// Take in gates below the cut for as long as that brings its output count
// down.  A gate taken in drives an output of its own, so it only pays for
// itself where it accounts for two or more of the outputs already there --
// which is to say, only where the fanout reconverges.
void Resynthesis::expandForward(
    std::vector<const sta::Pin*>& leaves,
    ConePins& drivers_in_cone,
    std::unordered_map<const sta::Pin*, int>& cut_users,
    int& ngates,
    int& nouterfans)
{
  while (takeInOneGate(leaves, drivers_in_cone, cut_users, ngates, nouterfans))
    ;
}

bool Resynthesis::takeInOneGate(
    std::vector<const sta::Pin*>& leaves,
    ConePins& drivers_in_cone,
    std::unordered_map<const sta::Pin*, int>& cut_users,
    int& ngates,
    int& nouterfans)
{
  stats_.forward_scans++;
  for (const sta::Pin* d : drivers_in_cone) {
    // The head's users belong to somebody else's cut, and `cut_users` already
    // has the head down as spoken for -- reading it again would put it over.
    if (d == head_ || cut_users.at(d) == nusers(d)) {
      continue;
    }

    odb::dbNet* net = db_network_->flatNet(d);
    if (!net) {
      continue;
    }
    for (odb::dbITerm* user : net->getITerms()) {
      if (isSignalInput(user)
          && takeInGate(user->getInst(),
                        leaves,
                        drivers_in_cone,
                        cut_users,
                        ngates,
                        nouterfans)) {
        // The cone grew under us; the caller starts the scan over
        return true;
      }
    }
  }
  return false;
}

bool Resynthesis::takeInGate(
    odb::dbInst* g,
    std::vector<const sta::Pin*>& leaves,
    ConePins& drivers_in_cone,
    std::unordered_map<const sta::Pin*, int>& cut_users,
    int& ngates,
    int& nouterfans)
{
  odb::dbITerm* out = nullptr;
  for (odb::dbITerm* pin : g->getITerms()) {
    if (isSignalOutput(pin)) {
      out = pin;
      break;
    }
  }
  // A two-output gate would come in with both outputs at once, which the
  // accounting here doesn't cover.
  if (!out || siblingOutput(out) || !expandableDriver(out)
      || drivers_in_cone.count(db_network_->dbToSta(out))) {
    return false;
  }

  // What `g` reads, and how often: one gate can read a driver twice.
  std::map<const sta::Pin*, int, PinIdLess> reads(PinIdLess{db_network_});
  for (odb::dbITerm* input : g->getITerms()) {
    if (!isSignalInput(input)) {
      continue;
    }
    const sta::Pin* a = driverOf(input);
    if (!a || a == head_) {
      return false;
    }
    reads[a]++;
  }

  int completed = 0;
  int fresh_leaves = 0;
  for (const auto& [a, count] : reads) {
    if (!drivers_in_cone.count(a)) {
      fresh_leaves += cut_users[a] == 0;
    } else if (cut_users.at(a) != nusers(a)
               && cut_users.at(a) + count == nusers(a)) {
      completed++;
    }
  }

  const sta::Pin* out_pin = db_network_->dbToSta(out);
  const int out_users = cut_users.count(out_pin) ? cut_users.at(out_pin) : 0;
  const bool out_was_leaf = out_users > 0;
  const bool out_is_root = out_users != nusers(out_pin);

  const int delta = (out_is_root ? 1 : 0) - completed;
  const int after_leaves
      = (int) leaves.size() + fresh_leaves - (out_was_leaf ? 1 : 0);
  if (delta > -1 || ngates + 1 > max_cells_ || after_leaves > max_leaves_) {
    return false;
  }

  drivers_in_cone.insert(out_pin);
  if (out_was_leaf) {
    leaves.erase(std::find(leaves.begin(), leaves.end(), out_pin));
  }
  cut_users[out_pin];  // an entry of its own, whatever its users
  ngates++;
  for (const auto& [a, count] : reads) {
    int& users = cut_users[a];
    if (users == 0 && !drivers_in_cone.count(a)) {
      leaves.push_back(a);
    }
    users += count;
  }
  nouterfans += delta;
  stats_.taken_in++;

  debugPrint(logger_,
             utl::SYN,
             "acd_resynth",
             2,
             "took in {} from below: {} outputs accounted for, {} left",
             db_network_->pathName(out_pin),
             completed,
             nouterfans);
  return true;
}

void Resynthesis::expandCut(int wedge,
                            std::vector<const sta::Pin*> leaves,
                            int ngates,
                            int mffc_size,
                            int nouterfans,
                            ConePins drivers_in_cone,
                            std::unordered_map<const sta::Pin*, int> cut_users,
                            std::unordered_map<const sta::Pin*, int> mffc_users)
{
  stats_.expansions++;
  const sta::Pin* y = leaves[wedge];
  assert(cut_users.at(y) <= nusers(y));

  odb::dbITerm* driver = asDriverITerm(y);
  if (!driver || !expandableDriver(driver)) {
    // We can't expand here
    return;
  }

  // The gate may already be in the cone through its second output, in which
  // case it costs us nothing and its fanins are accounted for already.
  const sta::Pin* sibling = siblingOutput(driver);
  const bool gate_in_cone = sibling && drivers_in_cone.count(sibling);

  leaves.erase(leaves.begin() + wedge);
  drivers_in_cone.insert(y);

  bool in_mffc = false;
  if (cut_users.at(y) != nusers(y)) {
    // Part of y's fanout stays outside the cut, so y is an output of it
    nouterfans++;
  } else if (entersMffc(drivers_in_cone, mffc_users, y)) {
    // Expanding y can be what completes a two-output gate, so this has to be
    // asked after y joins the cone, and even when the gate is in it already.
    in_mffc = true;
  }

  if (!gate_in_cone) {
    ngates++;

    for (odb::dbITerm* input : driver->getInst()->getITerms()) {
      if (!isSignalInput(input)) {
        continue;
      }

      const sta::Pin* a = driverOf(input);
      if (!cut_users[a]++) {
        leaves.push_back(a);
      }

      // fixup: a is fully contained now, it's no longer an output of the cut
      if (drivers_in_cone.count(a) && cut_users.at(a) == nusers(a)) {
        nouterfans--;
      }
    }
  }

  // Taking the gate into the mffc hands each of its fanins a reference, which
  // can cascade.  This is the same bookkeeping refMffc does, so defer to it.
  if (in_mffc) {
    refMffc(drivers_in_cone, mffc_size, mffc_users, y);
  }

  if (ngates > max_cells_ || nouterfans > max_outerfans_
      || (int) leaves.size() > max_intermediate_leaves_) {
    return;
  }

  // Expanding a leaf can bring the leaf count back down when the cone
  // reconverges, so an oversized cut is passed over, not given up on.
  if ((int) leaves.size() <= max_leaves_) {
    // Taking gates in from below is for this cut's sake only -- the
    // enumeration goes on from where it was, so these are all copies.
    std::vector<const sta::Pin*> f_leaves = leaves;
    ConePins f_drivers = drivers_in_cone;
    std::unordered_map<const sta::Pin*, int> f_cut_users = cut_users;
    int f_ngates = ngates;
    int f_nouterfans = nouterfans;
    {
      utl::DebugScopedTimer timer(stats_.t_forward);
      expandForward(f_leaves, f_drivers, f_cut_users, f_ngates, f_nouterfans);
    }

    // The timing objective only carries two outputs
    if (f_nouterfans <= 2) {
      std::vector<odb::dbInst*> cone;
      std::vector<const sta::Pin*> roots;
      GateNetwork net;
      collectCone(f_leaves, f_drivers, f_cut_users, cone, roots, net);
      assert((int) cone.size() == f_ngates);
      assert((int) roots.size() == f_nouterfans);
      evaluateCut(roots, f_leaves, cone, net, mffc_size);
    }
  }

  // Leaves before `wedge` were passed over by an ancestor call; leaving them
  // be is what keeps us from enumerating the same cut twice.
  for (; wedge < (int) leaves.size(); wedge++) {
    expandCut(wedge,
              leaves,
              ngates,
              mffc_size,
              nouterfans,
              drivers_in_cone,
              cut_users,
              mffc_users);
  }
}

float Resynthesis::networkArea(const GateNetwork& net)
{
  float area = 0.0f;
  std::unordered_set<odb::dbInst*> counted;

  for (const GateNode& node : net.nodes) {
    // Both outputs of a multi-output gate are nodes of their own, but there is
    // only the one cell to pay for.  Synthesized nodes have no pin to go on.
    if (node.driver_pin) {
      odb::dbInst* inst = asDriverITerm(node.driver_pin)->getInst();
      if (!counted.insert(inst).second) {
        continue;
      }
    }
    area += node.driver_port->libertyCell()->area();
  }
  return area;
}

std::string Resynthesis::formatGateNetwork(
    const GateNetwork& net,
    const std::vector<const sta::Pin*>& leaves)
{
  auto ref = [](const std::pair<bool, int>& fanin) {
    return (fanin.first ? "i" : "n") + std::to_string(fanin.second);
  };

  std::string text = "leaves:";
  for (int i = 0; i < net.ninputs; i++) {
    text += " i" + std::to_string(i) + "=" + db_network_->pathName(leaves[i]);
  }

  text += " | nodes:";
  for (int ni = 0; ni < (int) net.nodes.size(); ni++) {
    const GateNode& node = net.nodes[ni];
    text += " n" + std::to_string(ni) + "="
            + node.driver_port->libertyCell()->name() + "/"
            + node.driver_port->name() + "(";
    for (int fi = 0; fi < (int) node.fanins.size(); fi++) {
      text += (fi ? "," : "") + ref(node.fanins[fi]);
    }
    text += ")";
  }

  text += " | outs:";
  for (int oi = 0; oi < (int) net.outs.size(); oi++) {
    text += " " + ref(net.outs[oi]);
    // A network we read off the netlist can say which pin each output stands
    // for; one `synthesize` came up with has no pins to name yet.
    const auto& [is_pi, idx] = net.outs[oi];
    if (!is_pi && net.nodes[idx].driver_pin) {
      text += "=" + db_network_->pathName(net.nodes[idx].driver_pin);
    }
  }
  return text;
}

void Resynthesis::evaluateCut(const std::vector<const sta::Pin*>& roots,
                              const std::vector<const sta::Pin*>& leaves,
                              const std::vector<odb::dbInst*>& cone,
                              const GateNetwork& orig_net,
                              const int mffc_size)
{
  stats_.cuts++;
  SynthesisProblem problem;
  problem.function = evaluateFunction(orig_net);
  problem.inputs.resize(orig_net.ninputs);
  problem.outputs.resize(orig_net.outs.size());

  {
    std::unordered_set<sta::Instance*> cone_set;
    for (odb::dbInst* inst : cone) {
      cone_set.insert(db_network_->dbToSta(inst));
    }
    // `net()` has no answer for a pin acting as a bterm, so go through the
    // flat net: a leaf can be a top-level input port.
    std::vector<sta::Net*> root_nets;
    for (auto root : roots) {
      root_nets.push_back(db_network_->dbToSta(db_network_->flatNet(root)));
    }
    std::vector<sta::Net*> leaf_nets;
    for (auto leaf : leaves) {
      leaf_nets.push_back(db_network_->dbToSta(db_network_->flatNet(leaf)));
    }

    {
      utl::DebugScopedTimer timer(stats_.t_cut_timing);
      populateCutTiming(
          db_network_, sta_, problem, root_nets, leaf_nets, cone_set);
    }
  }

  // The search only pushes slack on the outputs marked critical; the rest cost
  // it nothing, so it is free to spend them on area.  Only an output failing
  // timing is worth that.
  for (size_t i = 0; i < roots.size(); i++) {
    sta::Vertex* vertex = sta_->graph()->pinDrvrVertex(roots[i]);
    if (!vertex) {
      continue;
    }

    const float root_slack = sta_->slack(vertex, sta::MinMax::max());
    if (root_slack >= 0.0f) {
      continue;
    }

    problem.outputs[i].critical = true;
    // 1 on the design's worst path, tapering off as slack approaches zero
    problem.outputs[i].criticality
        = wns_ < 0.0f ? std::min(1.0, (double) root_slack / (double) wns_)
                      : 1.0;
  }

  // What the logic in place manages against the budget populateCutTiming laid
  // down: the bar any candidate has to clear.
  float slack;
  {
    utl::DebugScopedTimer timer(stats_.t_slack);
    slack = networkSlack(logger_, problem, orig_net, corner_, fixed_slew_);
  }

  GateNetwork cand;
  long long explore_calls = 0;
  bool solved;
  {
    utl::DebugScopedTimer timer(stats_.t_synthesize);
    stats_.syntheses++;
    solved = synthesize(problem,
                        *match_cache_,
                        dparams_,
                        logger_,
                        cand,
                        std::numeric_limits<double>::infinity(),
                        allow_lateral_,
                        &explore_calls,
                        timing_opt_effort_);
    stats_.explores += explore_calls;
    stats_.worst_explore = std::max(stats_.worst_explore, explore_calls);
  }

  if (logger_->debugCheck(utl::SYN, "acd_resynth", 2)) {
    debugPrint(logger_,
               utl::SYN,
               "acd_resynth",
               2,
               "cut at {}: {} gates ({} in mffc), area {:.2f}, slack {} | {}",
               db_network_->pathName(head_),
               cone.size(),
               mffc_size,
               networkArea(orig_net),
               sta::delayAsString(slack, sta_),
               formatGateNetwork(orig_net, leaves));

    if (solved) {
      debugPrint(
          logger_,
          utl::SYN,
          "acd_resynth",
          2,
          "  candidate: {} gates, area {:.2f}, slack {} ({} explored) | {}",
          cand.nodes.size(),
          networkArea(cand),
          sta::delayAsString(
              networkSlack(logger_, problem, cand, corner_, fixed_slew_), sta_),
          explore_calls,
          formatGateNetwork(cand, leaves));
    } else {
      debugPrint(logger_, utl::SYN, "acd_resynth", 2, "  candidate: none");
    }
  }

  if (!solved) {
    return;
  }

  // A candidate that doesn't compute what the cut computes is worthless, and
  // reading it back the way we read the netlist in says so cheaply.
  if (evaluateFunction(cand) != problem.function) {
    logger_->error(utl::SYN,
                   62,
                   "Resynthesized network for the cut at {} is not equivalent "
                   "to the logic it would replace.",
                   db_network_->pathName(head_));
    return;
  }

  const float cand_slack
      = networkSlack(logger_, problem, cand, corner_, fixed_slew_);

  if (cand_slack <= slack + epsilon_) {
    return;
  }

  float gain = cand_slack - slack;
  if (!pin_best_ || gain > pin_best_->gain) {
    pin_best_ = Substitution{.leaves = leaves,
                             .original = orig_net,
                             .replacement = cand,
                             .gain = gain};
  }
}

}  // namespace syn::acd
