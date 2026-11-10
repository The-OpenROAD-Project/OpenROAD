// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026-2026, The OpenROAD Authors

#pragma once

#include <array>
#include <cstddef>
#include <deque>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "OptimizerTypes.hh"
#include "PowerModel.hh"
#include "PowerVtSwapGenerator.hh"
#include "RepairSetupContext.hh"
#include "SetupParallelEvalPolicy.hh"
#include "VtBudget.hh"
#include "rsz/Resizer.hh"
#include "sta/Delay.hh"

namespace rsz {

class MoveCommitter;

// repair_power: the PARALLEL_EVAL batches with moves that spend positive
// setup slack for less power.  A batch is kept while setup and hold WS stay
// above their margins, or no worse than at the start below them, and their
// TNS within kTnsTolerance of the start.  With a setup tolerance, every
// endpoint above its floor (SlackFloors) replaces the setup WS and TNS rule.
// A VT budget run takes the swaps that bring the budgets down, cheapest
// first, and ends once they hold.
class PowerParallelEvalPolicy : public SetupParallelEvalPolicy
{
 public:
  PowerParallelEvalPolicy(Resizer& resizer,
                          MoveCommitter& committer,
                          RepairSetupContext& setup_context,
                          const OptimizerRunConfig& config);

  const char* name() const override { return "PowerParallelEvalPolicy"; }
  bool start() override;
  // With -verbose, one row per batch.
  void iterate() override;
  // With debug repair_power 1, the power the slack budget left.
  void finish() override;

 protected:
  // Setup violations do not end a power run.
  bool finishIfNoSetupViolations() override { return false; }
  // The output pins of the instances that can still swap with slack above
  // their floor, most room first.  The endpoints are read for the floors and
  // the contract.
  BatchScope collectTargetEndpoints() override;
  // Each of those pins on its own worst path, which bounds what a swap of its
  // instance may spend.
  std::vector<Target> collectTargets(
      size_t begin_endpoint,
      size_t end_endpoint,
      std::unordered_set<sta::Pin*>& visited_pins) const override;
  void selectMoveSequence() override;
  void buildMoveGenerators(const std::vector<MoveType>& move_types,
                           const GeneratorContext& context) override;
  bool acceptsBatch(sta::Slack wns_after, sta::Slack tns_after) const override;
  // Moves whose driver pin fell below its floor and its pre-batch
  // slack, whose guarded side pin made a new worst path, that are on the
  // path to an endpoint that broke the setup contract or start the slew
  // growth that reaches it, or, when hold fails, that own a pin below the
  // lower of the hold margin and the start hold WS.  spared gets the moves
  // beside those paths, for a second salvage pass.  Misestimated culprits and
  // slew sources stay rejected for the run.
  std::vector<MoveCandidate*> findBatchCulprits(
      sta::Slack pre_batch_wns,
      std::vector<MoveCandidate*>& spared) const override;
  // Moves in series on a path add up their costs, moves on parallel branches
  // do not: each admitted move keeps the costs on its worst path within
  // kSlackRatio of its slack, and so does every move admitted after it.
  void selectBatch(BatchPool& pool, int batch_cap) override;
  // Adds the power the kept moves save to the batch's saving.
  void saveBatchCheckpoint(sta::Slack wns, sta::Slack tns) override;
  // Updates the pin access the incremental router defers to the end of the
  // run first: new access points reroute nets, and the batch is judged on
  // the routes it leaves.
  void updateTiming() override;
  // A VT budget run past the setup contract lowers its floor instead.
  void startTailOrFinish(const char* reason) override;

 private:
  struct Contract
  {
    sta::Slack setup_ws{0.0f};
    sta::Slack setup_tns{0.0f};
    sta::Slack hold_ws{0.0f};
    sta::Slack hold_tns{0.0f};
  };

  Contract measureContract() const;
  // The output pins of the instances that can go to a higher VT, with slack
  // above their floor, most slack first.
  void collectSwappableDrivers();
  // Each vertex's setup required times, by rise/fall, with every endpoint's
  // moved down by its floor: a vertex's room is these less its arrivals.
  using FloorRequired = std::vector<std::array<float, 2>>;
  FloorRequired floorRequired() const;
  float floorRoom(sta::Vertex* vertex, const FloorRequired& required) const;
  // Past the setup contract, every endpoint may fall loss percent of the
  // clock period below the lower of the margin and the WS at the start.
  void setBudgetFloor(double loss);
  // Lowers that floor a step and forgets the rejects; false at the last step
  // or within the contract.
  bool lowerBudgetFloor();
  // inst has a higher VT it has not been rejected for.
  bool canSwap(const sta::Instance* inst) const;
  void rejectSwap(const MoveCandidate& candidate) const;
  void noteBudgetReject(const MoveCandidate& candidate);
  Target instanceTarget(sta::Pin* driver_pin) const;
  // The driver vertices on the worst path through driver: its fanin part
  // and the worst-slack loads down to an endpoint.
  const std::vector<sta::Vertex*>& worstPathDrivers(sta::Vertex* driver) const;
  bool setupKept(const Contract& now) const;
  bool holdKept(const Contract& now) const;
  // The endpoints below the setup WS bound, then those that lost the most
  // slack below zero until they cover the TNS beyond the tolerance; with a
  // setup tolerance, the endpoints below their floor.
  std::vector<sta::Vertex*> brokenEndpoints(const Contract& now) const;
  // pin's setup slack is below floor.
  bool setupBelow(const sta::Pin* pin, float floor) const;
  // The target's driver pin, which carries the worst path through the swap
  // whichever endpoint the target came from, fell below its floor and its
  // pre-batch slack.
  bool driverFell(const Target& target) const;
  // A pin of the target's instance has hold slack below floor.
  bool holdBelow(const Target& target, float floor) const;
  // The swap slowed its own and fanin stages by more than the slack its
  // estimate cost, or they cannot be measured.
  bool misestimated(const MoveCandidate& candidate) const;
  // Adds the moves on the worst path to endpoint to on_path, and those
  // beside it to beside: loads on its nets, whose pins reroute its wires,
  // drivers of its cells' other inputs, whose slew a cell output takes when
  // it is the worst, and flops on the clock nets of its flops.
  void addPathMoves(
      sta::Vertex* endpoint,
      const std::unordered_map<const sta::Instance*, MoveCandidate*>& moves,
      std::unordered_set<MoveCandidate*>& on_path,
      std::unordered_set<MoveCandidate*>& beside) const;
  // The largest slew of vertex over the scenes, by rise/fall.
  std::array<float, 2> maxSlews(sta::Vertex* vertex) const;
  void savePreBatchSlews();
  // Adds the moves where the slew growth that reaches the worst path to
  // endpoint starts: back from each pin of the path, the latch D or clock
  // pins before its data start included, through the pins whose slew grew,
  // a moved cell on such a pin or a moved load that added capacitance to its
  // net.  A cell output takes the worst slew of its inputs, so the growth
  // travels through side inputs that addPathMoves() does not reach.
  void addSlewSources(
      sta::Vertex* endpoint,
      const std::unordered_map<const sta::Instance*, MoveCandidate*>& moves,
      std::unordered_set<MoveCandidate*>& sources) const;

  PowerModel power_model_{resizer_};
  // Counted at the start and after every batch.
  VtBudgetCounts vt_budget_;
  // A VT budget run stops after a batch that kept at most kStallRatio of the
  // instances over the limits at its start, or past the setup contract lowers
  // its floor.
  int start_excess_{0};
  // Percent of the clock period the floor is below the WS the run started at.
  double budget_floor_loss_{0.0};
  // The share of its room a swap, and the swaps in series with it, may spend.
  float slack_ratio_;
  // Swaps whose estimate failed for a lasting reason, misestimated culprits
  // and slew sources; selectBatch() and findBatchCulprits() add.
  mutable RejectedSwaps rejected_;
  Contract start_contract_;
  // Set by start(); endpoints recorded at batch 0, drivers every batch.
  SlackFloors floors_{0.0f, 0.0f};
  // With a tolerance, set by collectSwappableDrivers() every batch.
  FloorRequired floor_required_;
  // maxSlews() of each vertex before the batch, by vertex id.
  std::vector<std::array<float, 2>> pre_batch_slews_;
  std::vector<std::pair<const sta::Pin*, sta::Slack>> pre_batch_endpoints_;
  std::vector<sta::Pin*> swappable_drivers_;
  mutable std::unordered_map<sta::Vertex*, std::vector<sta::Vertex*>>
      worst_paths_;

  // Per instance, the cell it had at its latest slack budget reject and the
  // most power a rejected higher VT would have saved from it.
  struct BudgetReject
  {
    const sta::LibertyCell* from_cell{nullptr};
    float gain{0.0f};
  };
  std::unordered_map<const sta::Instance*, BudgetReject> budget_rejects_;

  // Predicted watts saved by the current batch and the run, and by the
  // last kStallBatches batches.
  float batch_saved_{0.0f};
  float total_saved_{0.0f};
  std::deque<float> saved_window_;

  static constexpr float kTnsTolerance = 0.01f;
  // The latches in series the floor room and the culprit paths follow.
  static constexpr int kLatchPasses = 4;
  // The run stops once the last kStallBatches saved batches together saved at
  // most kStallRatio of what the run saved.
  static constexpr size_t kStallBatches = 4;
  static constexpr float kStallRatio = 0.01f;
};

}  // namespace rsz
