// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// Combinational mapper: maps And/Andnot/Or gates to
// standard cells from a liberty library using cut-based technology
// mapping with NPN canonicalization.

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <map>
#include <memory>
#include <set>
#include <span>
#include <utility>
#include <vector>

#include "flow/combinational_mapper_npn.h"
#include "sta/FuncExpr.hh"
#include "sta/Liberty.hh"
#include "sta/Network.hh"
#include "sta/NetworkClass.hh"
#include "sta/PortDirection.hh"
#include "syn/ir/Bundle.h"
#include "syn/ir/ControlNet.h"
#include "syn/ir/Graph.h"
#include "syn/ir/Instance.h"
#include "syn/synthesis.h"
#include "utl/Logger.h"

namespace syn {

namespace cm {

static constexpr int kCutMaximum = 6;

struct MapTarget
{
  sta::LibertyCell* cell;
  // The via NPN tells the mapper how to rewire the canonical inputs back to
  // this cell's actual ports.
  NPN via;
};

struct TargetIndex
{
  // The key is (num_inputs, canonical truth table)
  std::map<std::pair<int, Truth6>, std::vector<MapTarget>> classes;
  sta::LibertyCell* inverter = nullptr;
  // The int is the output port index (0 or 1)
  std::pair<sta::LibertyCell*, int> tie_low;
  std::pair<sta::LibertyCell*, int> tie_high;
};

// Recursively converts a Liberty FuncExpr AST into a truth table (Truth6)
static Truth6 fexprEval(sta::FuncExpr* fexpr,
                        const std::vector<sta::LibertyPort*>& inputs)
{
  using Op = sta::FuncExpr::Op;
  switch (fexpr->op()) {
    case Op::port: {
      // Find the index of this port in inputs
      int in_idx;
      for (in_idx = 0; in_idx < inputs.size(); in_idx++) {
        if (inputs[in_idx] == fexpr->port()) {
          break;
        }
      }
      assert(in_idx != (int) inputs.size());
      // The function is 1 whenever this input = 1
      Truth6 ret = 0;
      for (int row = 0; row < (1 << (int) inputs.size()); row++) {
        if (row & 1 << in_idx) {
          ret |= (Truth6) 1 << row;
        }
      }
      return ret;
    }
    case Op::not_:
      return mask6(inputs.size()) & ~fexprEval(fexpr->left(), inputs);
    case Op::or_:
      return fexprEval(fexpr->left(), inputs)
             | fexprEval(fexpr->right(), inputs);
    case Op::and_:
      return fexprEval(fexpr->left(), inputs)
             & fexprEval(fexpr->right(), inputs);
    case Op::xor_:
      return fexprEval(fexpr->left(), inputs)
             ^ fexprEval(fexpr->right(), inputs);
    case Op::one:
      return mask6(inputs.size());
    case Op::zero:
      return 0;
  }
  assert(false);
  return 0;
}

// Returns true if the union fits within max_cut slots.
static bool cutUnion(Net* target,
                     int& len,
                     const int max_cut,
                     const Net* cut1,
                     const Net* cut2)
{
  len = 0;
  int j = 0;
  for (int i = 0; i < kCutMaximum && cut1[i] != Net::sentinel(); i++) {
    // Add smaller cut2 entries first
    while (j < kCutMaximum && cut2[j] != Net::sentinel()
           && Graph::netId(cut2[j]) < Graph::netId(cut1[i])) {
      if (len == max_cut) {
        return false;
      }
      target[len++] = cut2[j++];
    }
    // Skip if a duplicate is found
    if (j < kCutMaximum && cut2[j] != Net::sentinel() && cut2[j] == cut1[i]) {
      j++;
    }
    // Add cut1[i] into target.
    if (len == max_cut) {
      return false;
    }
    target[len++] = cut1[i];
  }
  // Done with cut1; add whatever remains in cut2.
  while (j < kCutMaximum && cut2[j] != Net::sentinel()) {
    if (len == max_cut) {
      return false;
    }
    target[len++] = cut2[j++];
  }
  return true;
}

// Relabels a truth table `t1` (over variables `vars1`) onto the variable
// set `vars2`, returning the equivalent truth table indexed by vars2's
// minterms. Both arrays must be sorted by netId. Variables in vars2 not
// present in vars1 become don't-cares in the result. Variables in vars1
// not present in vars2 must not be in t1's support (typically guaranteed
// by checkSupport6); otherwise the result is meaningless.
static Truth6 recode6(const Truth6 t1,
                      const Net* vars1,
                      const int nvars1,
                      const Net* vars2,
                      const int nvars2)
{
  Truth6 t2 = 0;
  for (int i2 = 0; i2 < (1 << nvars2); i2++) {
    int i1 = 0;
    int var2i = 0;
    for (int var1i = 0; var1i < nvars1; var1i++) {
      while (var2i < nvars2
             && Graph::netId(vars2[var2i]) < Graph::netId(vars1[var1i])) {
        var2i++;
      }
      if (var2i == nvars2) {
        break;
      }
      if (i2 & 1 << var2i) {
        i1 |= 1 << var1i;
      }
    }
    if (t1 & (Truth6) 1 << i1) {
      t2 |= (Truth6) 1 << i2;
    }
  }
  return t2;
}

static bool checkSupport6(const Truth6 t, const int var_idx)
{
  const Truth6 t_shift = t << (1 << var_idx);
  return ((t_shift ^ t) & cofactor_masks[var_idx]) != 0;
}

static void buildIndex(sta::Network* network,
                       TargetIndex& index,
                       utl::Logger* logger,
                       const Synthesis& syn)
{
  std::unique_ptr<sta::LibertyLibraryIterator> lib_iter{
      network->libertyLibraryIterator()};
  while (lib_iter->hasNext()) {
    sta::LibertyLibrary* lib = lib_iter->next();
    sta::LibertyCellIterator cell_iter(lib);
    while (cell_iter.hasNext()) {
      sta::LibertyCell* cell = cell_iter.next();
      if (cell->hasSequentials()) {
        debugPrint(logger,
                   utl::SYN,
                   "cm",
                   3,
                   "ignoring {} because of sequentials",
                   cell->name());
        continue;
      }

      // Collect the inputs and outputs
      std::vector<sta::LibertyPort*> inputs;
      std::vector<sta::LibertyPort*> outputs;

      sta::LibertyCellPortIterator port_iter(cell);
      bool skip = false;
      while (port_iter.hasNext()) {
        sta::LibertyPort* port = port_iter.next();
        if (port->isPwrGnd()) {
          continue;
        }
        if (port->direction()->isInput()) {
          inputs.push_back(port);
        } else if (port->direction()->isOutput()) {
          outputs.push_back(port);
        } else {
          skip = true;
          break;
        }
      }

      if (skip) {
        debugPrint(logger,
                   utl::SYN,
                   "cm",
                   3,
                   "ignoring {} because of unsupported port types",
                   cell->name());
        continue;
      }

      if (inputs.size() > kCutMaximum) {
        debugPrint(
            logger,
            utl::SYN,
            "cm",
            3,
            "ignoring {} because of too many inputs ({}, above limit {})",
            cell->name(),
            inputs.size(),
            kCutMaximum);
        continue;
      }

      if (logger->debugCheck(utl::SYN, "cm", 3)) {
        debugPrint(logger, utl::SYN, "cm", 3, "indexing {}", cell->name());
        debugPrint(logger, utl::SYN, "cm", 3, "  inputs:");
        for (auto input : inputs) {
          debugPrint(logger, utl::SYN, "cm", 3, "   - {}", input->name());
        }
        debugPrint(logger, utl::SYN, "cm", 3, "  outputs:");
        for (auto output : outputs) {
          if (output->function()) {
            debugPrint(logger,
                       utl::SYN,
                       "cm",
                       3,
                       "   - {} (function {:X})",
                       output->name(),
                       fexprEval(output->function(), inputs));
          } else {
            debugPrint(logger,
                       utl::SYN,
                       "cm",
                       3,
                       "   - {} (missing function)",
                       output->name());
          }
        }
      }

      // Handle tie cells (0 inputs, 2 outputs)
      if (inputs.empty()) {
        if (outputs.size() == 2 && outputs[0]->function()
            && outputs[1]->function()) {
          Truth6 f0 = fexprEval(outputs[0]->function(), inputs);
          Truth6 f1 = fexprEval(outputs[1]->function(), inputs);
          if ((f0 == 0 && f1 == 1) || (f0 == 1 && f1 == 0)) {
            if (!index.tie_low.first
                || cell->area() < index.tie_low.first->area()) {
              index.tie_low = {cell, (f0 == 0) ? 0 : 1};
            }
            if (!index.tie_high.first
                || cell->area() < index.tie_high.first->area()) {
              index.tie_high = {cell, (f0 == 1) ? 0 : 1};
            }
          }
        }

        if (outputs.size() == 1 && outputs[0]->function()) {
          Truth6 f0 = fexprEval(outputs[0]->function(), inputs);
          if (f0 == 0
              && (!index.tie_low.first
                  || cell->area() < index.tie_low.first->area())) {
            index.tie_low = {cell, 0};
          }
          if (f0 == 1
              && (!index.tie_high.first
                  || cell->area() < index.tie_high.first->area())) {
            index.tie_high = {cell, 0};
          }
        }
      }

      // Check dont use after detecting tie cells to work around nangate45
      // marking tie cells dont use inside .lib
      if (syn.dontUse(cell)) {
        debugPrint(logger,
                   utl::SYN,
                   "cm",
                   3,
                   "ignoring {} because of dont use",
                   cell->name());
        continue;
      }

      if (outputs.size() != 1 || !outputs[0]->function()) {
        continue;
      }

      Truth6 func = fexprEval(outputs[0]->function(), inputs);

      // Track inverter
      if (inputs.size() == 1 && func == 0b01) {
        if (!index.inverter || cell->area() < index.inverter->area()) {
          index.inverter = cell;
        }
      }

      // Register all NPN representatives
      npn_semiclass_allrepr(
          func, inputs.size(), [&](const Truth6 repr, const NPN& npn) {
            index.classes[{(int) inputs.size(), repr}].push_back(
                MapTarget{.cell = cell, .via = npn.inv()});
          });
    }
  }

  // Prune targets: keep smallest cell per c_fingerprint
  for (auto& [key, targets] : index.classes) {
    std::ranges::sort(targets, [](const MapTarget& a, const MapTarget& b) {
      return std::make_pair(a.via.c_fingerprint(), a.cell->area())
             < std::make_pair(b.via.c_fingerprint(), b.cell->area());
    });
    auto unique_range = std::ranges::unique(
        targets, [](const MapTarget& a, const MapTarget& b) {
          return a.via.c_fingerprint() == b.via.c_fingerprint();
        });
    targets.erase(unique_range.begin(), targets.end());
  }
}

struct ClassMatch
{
  Truth6 semiclass = 0;
  NPN npn;
  Net cut[kCutMaximum] = {Net::sentinel(),
                          Net::sentinel(),
                          Net::sentinel(),
                          Net::sentinel(),
                          Net::sentinel(),
                          Net::sentinel()};
};

struct NodePolarity
{
  int map_fouts = 0;
  float flow_fouts = 1.0f;
  float farea = 0.0f;
  std::pair<ClassMatch*, MapTarget*> selected;
};

struct Node
{
  uint32_t fanouts = 0;
  NodePolarity pol[2];
  ClassMatch* matches = nullptr;
  int nmatches = 0;
  uint32_t fid = 0;
};

struct Subject
{
  std::shared_ptr<TargetIndex> target_index;
  Graph& graph;

  ControlNet stripInverter(Net net) const
  {
    auto [inst, offset] = graph.resolve(net);
    if (auto* not1 = inst->try_as<Not>()) {
      return !stripInverter(not1->a()[offset]);
    }
    if (auto* buf = inst->try_as<Buffer>()) {
      return stripInverter(buf->a()[offset]);
    }
    return ControlNet::pos(net);
  }
};

class Mapping
{
 public:
  using Literal = ControlNet;

  Mapping(utl::Logger* logger, const Subject& subject);

  ClassMatch* reserveMatches(int n);

  ClassMatch* allocMatches(int n);

  Node* node(Net net) { return &nodes_[Graph::netId(net)]; }

  bool isMappable(const Instance* inst);

  bool isMappable(Net net);

  void collectPrimaryOutputs();

  void computeFanouts();

  void prepareMatches(int npriority_cuts, int nmatches_max, int max_cut_len);

  float refCut(const Literal& lit);

  void derefCut(const Literal& lit);

  float refNode(const Literal& lit);

  void derefNode(const Literal& lit);

  float walkSelection(bool initial);

  void areaFlowRound(float refs_blend, bool initial = false);

  void exactRound();

  void integrate();

 private:
  utl::Logger* logger_;
  Subject subject_;
  std::vector<Node> nodes_;
  std::vector<ControlNet> primary_outputs_;
  std::vector<std::pair<ControlNet, Net>> primary_output_fixups_;

  // Match arena
  std::vector<std::unique_ptr<ClassMatch[]>> match_arena_;
  int arena_slot_ = 0;
  static constexpr int kArenaChunk = 2048;
};

Mapping::Mapping(utl::Logger* logger, const Subject& subject)
    : logger_(logger), subject_(subject)
{
  nodes_.resize(subject_.graph.tableSize());
}

bool Mapping::isMappable(const Instance* inst)
{
  return inst->is<And>() || inst->is<Andnot>() || inst->is<Or>();
}

bool Mapping::isMappable(Net net)
{
  auto [inst, offset] = subject_.graph.resolve(net);
  return isMappable(inst);
}

ClassMatch* Mapping::reserveMatches(const int n)
{
  assert(n <= kArenaChunk);
  if (arena_slot_ + n > kArenaChunk || match_arena_.empty()) {
    match_arena_.push_back(std::make_unique<ClassMatch[]>(kArenaChunk));
    arena_slot_ = 0;
  }
  ClassMatch* ret = &match_arena_.back()[arena_slot_];
  // We are reserving but not allocating: do not increment arena_slot_
  return ret;
}

ClassMatch* Mapping::allocMatches(const int n)
{
  assert(n <= kArenaChunk);
  ClassMatch* ret = reserveMatches(n);
  arena_slot_ += n;
  return ret;
}

void Mapping::collectPrimaryOutputs()
{
  std::set<ControlNet> primaryOutputNets;
  std::set<std::pair<ControlNet, Net>> fixupSet;

  auto registerPrimaryOutput = [&](Net net) {
    auto cnet = subject_.stripInverter(net);
    if (isMappable(cnet.net())) {
      primaryOutputNets.insert(cnet);
      if (cnet.net() != net) {
        fixupSet.insert({cnet, net});
      }
    }
  };

  subject_.graph.forEachInstance([&](const Instance* inst) {
    // Ignore tentative names
    if (inst->is<Name>() && inst->as<Name>()->tentative()) {
      return;
    }
    // Ignore Nots, they are transparent when collecting POs
    if (inst->is<Not>()) {
      return;
    }
    if (!isMappable(inst)) {
      inst->visit([&](Net fanin) { registerPrimaryOutput(fanin); });
    } else {
      inst->visit([&](Net fanin) {
        if (!isMappable(fanin) && !fanin.isConst()) {
          // Call registerPrimaryOutput on a mappable node's fanin just in
          // case the fanin itself isn't mappable but
          // subject_.stripInverter(fanin) *is* mappable. Ideally bitblasting
          // makes sure this does not happen as the inverter will be a
          // mapping boundary and quality of mapping can be degraded. Even so
          // we need to produce a correct mapping.
          registerPrimaryOutput(fanin);
        }
      });
    }
  });

  primary_outputs_.assign(primaryOutputNets.begin(), primaryOutputNets.end());
  primary_output_fixups_.assign(fixupSet.begin(), fixupSet.end());
}

void Mapping::computeFanouts()
{
  subject_.graph.forEachInstance([&](const Instance* inst) {
    if (isMappable(inst)) {
      inst->visit([&](Net fanin) { node(fanin)->fanouts++; });
    }
  });

  for (auto& primary_output : primary_outputs_) {
    node(primary_output.net())->fanouts += 100;
  }
}

void Mapping::prepareMatches(const int npriority_cuts,
                             const int nmatches_max,
                             const int max_cut_len)
{
  assert(max_cut_len >= 3 && max_cut_len <= kCutMaximum);
  debugPrint(logger_, utl::SYN, "cm", 1, "collecting matches");

  struct PriorityCut
  {
    Net cut[kCutMaximum] = {Net::sentinel(),
                            Net::sentinel(),
                            Net::sentinel(),
                            Net::sentinel(),
                            Net::sentinel(),
                            Net::sentinel()};
    int ncut = 0;
    Truth6 function = 0;
  };
  struct NodeCache
  {
    std::span<PriorityCut> ps;
    Net mark = Net::sentinel();
  };

  // Frontier computation: assign recyclable slot IDs
  // Walk reverse topological order
  int frontierSize = 1;  // slot 0 unused
  std::vector<uint32_t> freeSlots;

  auto assignFid = [&](Node& nd) {
    if (nd.fid == 0) {
      if (!freeSlots.empty()) {
        nd.fid = freeSlots.back();
        freeSlots.pop_back();
      } else {
        nd.fid = frontierSize++;
      }
    }
  };

  subject_.graph.forEachInstanceReversed([&](const Instance* inst) {
    if (isMappable(inst)) {
      for (uint32_t i = inst->outputWidth() - 1;; i--) {
        Net onet = subject_.graph.output(inst)[i];
        inst->visitSlice(i, [&](Net fanin) { assignFid(*node(fanin)); });
        auto* cur = node(onet);
        assignFid(*cur);
        if (cur->fid != 0) {
          freeSlots.push_back(cur->fid);
        }
        if (i == 0) {
          break;
        }
      }
    } else if (inst->outputWidth() > 0) {
      for (uint32_t i = inst->outputWidth() - 1;; i--) {
        Net onet = subject_.graph.output(inst)[i];
        auto* cur = node(onet);
        assignFid(*cur);
        if (cur->fid != 0) {
          freeSlots.push_back(cur->fid);
        }
        if (i == 0) {
          break;
        }
      }
    }
  });

  size_t matchesTotal = 0;
  // Allocate priority cut storage
  std::vector<PriorityCut> pcuts(
      static_cast<size_t>(frontierSize) * npriority_cuts, PriorityCut{});
  std::vector<NodeCache> cache(frontierSize, NodeCache{});

  subject_.graph.forEachNet([&](Net net, const Instance*, uint32_t) {
    Node* cur = node(net);
    NodeCache* lcache = &cache[cur->fid];
    lcache->ps = std::span<PriorityCut>(
        &pcuts[static_cast<size_t>(cur->fid) * npriority_cuts], 0);
    lcache->mark = net;

    if (!isMappable(net)) {
      return;
    }

    bool in1Compl = false, in2Compl = false, outCompl = false;
    Net fanin1 = Net::sentinel(), fanin2 = Net::sentinel();

    auto [inst, offset] = subject_.graph.resolve(net);
    if (auto* op = inst->try_as<And>()) {
      fanin1 = op->a()[offset];
      fanin2 = op->b()[offset];
    } else if (auto* op = inst->try_as<Andnot>()) {
      fanin1 = op->a()[offset];
      fanin2 = op->b()[offset];
      in2Compl = true;
    } else if (auto* op = inst->try_as<Or>()) {
      fanin1 = op->a()[offset];
      fanin2 = op->b()[offset];
      in1Compl = true;
      in2Compl = true;
      outCompl = true;
    } else {
      std::abort();
    }

    Node *n1 = node(fanin1), *n2 = node(fanin2);
    assert(n1 && n2);
    assert(cache[n1->fid].mark == fanin1);
    assert(cache[n2->fid].mark == fanin2);

    Net t1[kCutMaximum] = {fanin1,
                           Net::sentinel(),
                           Net::sentinel(),
                           Net::sentinel(),
                           Net::sentinel(),
                           Net::sentinel()};
    Net t2[kCutMaximum] = {fanin2,
                           Net::sentinel(),
                           Net::sentinel(),
                           Net::sentinel(),
                           Net::sentinel(),
                           Net::sentinel()};

    auto cutLen = [](const Net* cut) {
      for (int i = 0; i < kCutMaximum; i++) {
        if (cut[i] == Net::sentinel()) {
          return i;
        }
      }
      return kCutMaximum;
    };

    std::set<uint64_t> seenCuts;

    int matchSlot = 0;
    int psSlot = 0;

    // Conservatively reserve a buffer for the maximal number of matches.
    // Later we will call `allocMatches` to allocate the used portion of
    // the buffer. This way we don't overallocate and waste slots.
    ClassMatch* matchBuf = reserveMatches(nmatches_max);

    for (int i = -1; i < (int) cache[n1->fid].ps.size(); i++) {
      for (int j = -1; j < (int) cache[n2->fid].ps.size(); j++) {
        Net* n1Cut = ((i == -1) ? t1 : cache[n1->fid].ps[i].cut);
        Net* n2Cut = ((j == -1) ? t2 : cache[n2->fid].ps[j].cut);
        int n1CutLen = (i == -1) ? 1 : cutLen(n1Cut);
        int n2CutLen = (j == -1) ? 1 : cutLen(n2Cut);

        Truth6 n1Func
            = ((i == -1) ? (Truth6) 2 : cache[n1->fid].ps[i].function);
        if (in1Compl) {
          n1Func ^= mask6(n1CutLen);
        }
        Truth6 n2Func
            = ((j == -1) ? (Truth6) 2 : cache[n2->fid].ps[j].function);
        if (in2Compl) {
          n2Func ^= mask6(n2CutLen);
        }

        Net workingCut[kCutMaximum] = {
            Net::sentinel(),
            Net::sentinel(),
            Net::sentinel(),
            Net::sentinel(),
            Net::sentinel(),
            Net::sentinel(),
        };
        int workingLen = 0;
        if (!cutUnion(workingCut, workingLen, max_cut_len, n1Cut, n2Cut)) {
          continue;
        }

        Truth6 cutFunction
            = recode6(n1Func, n1Cut, n1CutLen, workingCut, workingLen)
              & recode6(n2Func, n2Cut, n2CutLen, workingCut, workingLen);
        if (outCompl) {
          cutFunction ^= mask6(workingLen);
        }

        // Remove unsupported variables
        Net reducedCut[kCutMaximum] = {
            Net::sentinel(),
            Net::sentinel(),
            Net::sentinel(),
            Net::sentinel(),
            Net::sentinel(),
            Net::sentinel(),
        };
        int reducedLen = 0;
        for (int idx = 0; idx < workingLen; idx++) {
          if (checkSupport6(cutFunction, idx)) {
            reducedCut[reducedLen++] = workingCut[idx];
          }
        }

        if (reducedLen < workingLen) {
          cutFunction = recode6(
              cutFunction, workingCut, workingLen, reducedCut, reducedLen);
        }

        // Dedup cuts
        uint64_t cutHash = reducedLen;
        for (int k = 0; k < reducedLen; k++) {
          cutHash
              = cutHash * 6364136223846793005ULL + Graph::netId(reducedCut[k]);
        }
        if (seenCuts.contains(cutHash)) {
          continue;
        }
        seenCuts.insert(cutHash);

        // Try to match against library
        NPN npn;
        Truth6 sc = npn_semiclass(cutFunction, reducedLen, npn);
        if (subject_.target_index->classes.contains({reducedLen, sc})) {
          if (matchSlot < nmatches_max) {
            ClassMatch& m = matchBuf[matchSlot];
            m.semiclass = sc;
            m.npn = npn;
            for (int k = 0; k < reducedLen; k++) {
              m.cut[k] = reducedCut[k];
            }
            for (int k = reducedLen; k < kCutMaximum; k++) {
              m.cut[k] = Net::undef();
            }
            matchSlot++;
            matchesTotal++;
          }
        }

        // Store in priority cut cache
        if (psSlot < npriority_cuts) {
          auto& pc = pcuts[cur->fid * npriority_cuts + psSlot];
          pc.ncut = reducedLen;
          for (int k = 0; k < reducedLen; k++) {
            pc.cut[k] = reducedCut[k];
          }
          for (int k = reducedLen; k < kCutMaximum; k++) {
            pc.cut[k] = Net::sentinel();
          }
          pc.function = cutFunction;
          psSlot++;
        }
      }
    }

    lcache->ps = std::span<PriorityCut>(
        &pcuts[static_cast<size_t>(cur->fid) * npriority_cuts], psSlot);
    lcache->mark = net;
    ClassMatch* allocatedBuf = allocMatches(matchSlot);
    (void) allocatedBuf;
    assert(allocatedBuf == matchBuf);
    cur->matches = matchBuf;
    cur->nmatches = matchSlot;
  });

  debugPrint(logger_, utl::SYN, "cm", 1, "matches total = {}", matchesTotal);
}

float Mapping::refCut(const Literal& lit)
{
  if (!isMappable(lit.net())) {
    return 0.0f;
  }

  auto& pol = node(lit.net())->pol[!lit.isPositive()];
  auto [match, target] = pol.selected;
  assert(match != nullptr && target != nullptr);
  NPN localMap = target->via * match->npn;

  float sum = 0.0f;
  for (int i = 0; i < kCutMaximum && match->cut[i] != Net::sentinel(); i++) {
    Literal cutLit
        = Literal::withPolarity(match->cut[i], !localMap.input_complement[i]);
    Node* cutNode = node(cutLit.net());
    auto& cutPol = cutNode->pol[!cutLit.isPositive()];
    cutPol.map_fouts++;

    if (cutPol.map_fouts == 1) {
      if (isMappable(cutLit.net())) {
        auto [cutMatch, cutTarget] = cutPol.selected;
        assert(cutTarget != nullptr);
        sum += cutTarget->cell->area();
        sum += refCut(cutLit);
      } else if (cutLit.isNegative()) {
        sum += subject_.target_index->inverter->area();
      }
    }
  }
  return sum;
}

void Mapping::derefCut(const Literal& lit)
{
  if (!isMappable(lit.net())) {
    return;
  }

  auto& pol = node(lit.net())->pol[!lit.isPositive()];
  auto [match, target] = pol.selected;
  assert(match != nullptr && target != nullptr);
  NPN localMap = target->via * match->npn;

  for (int i = 0; i < kCutMaximum && match->cut[i] != Net::sentinel(); i++) {
    Literal cutLit
        = Literal::withPolarity(match->cut[i], !localMap.input_complement[i]);
    auto& cutPol = node(cutLit.net())->pol[!cutLit.isPositive()];
    cutPol.map_fouts--;
    assert(cutPol.map_fouts >= 0);

    if (cutPol.map_fouts == 0 && isMappable(cutLit.net())) {
      derefCut(cutLit);
    }
  }
}

float Mapping::refNode(const Literal& lit)
{
  if (lit.net().isConst()) {
    return 0.0f;
  }

  auto& pol = node(lit.net())->pol[!lit.isPositive()];
  pol.map_fouts++;
  if (pol.map_fouts == 1) {
    if (isMappable(lit.net())) {
      auto [match, target] = pol.selected;
      assert(target != nullptr);
      return target->cell->area() + refCut(lit);
    }
    if (lit.isNegative()) {
      return subject_.target_index->inverter->area();
    }
  }
  return 0.0f;
}

void Mapping::derefNode(const Literal& lit)
{
  if (lit.net().isConst()) {
    return;
  }

  auto& pol = node(lit.net())->pol[!lit.isPositive()];
  assert(pol.map_fouts >= 1);
  pol.map_fouts--;
  if (pol.map_fouts == 0 && isMappable(lit.net())) {
    derefCut(lit);
  }
}

float Mapping::walkSelection(const bool initial)
{
  if (!initial) {
    for (auto& primary_output : primary_outputs_) {
      derefNode(primary_output);
    }
  }

#ifndef NDEBUG
  for (auto& nd : nodes_) {
    assert(nd.pol[0].map_fouts == 0);
    assert(nd.pol[1].map_fouts == 0);
  }
#endif

  float area = 0.0f;
  for (auto& primary_output : primary_outputs_) {
    area += refNode(primary_output);
  }
  return area;
}

void Mapping::areaFlowRound(const float refs_blend, const bool initial)
{
  const float invArea = subject_.target_index->inverter->area();

  // Update flow_fouts
  for (auto& nd : nodes_) {
    for (auto& c : nd.pol) {
      const float blended
          = refs_blend * nd.fanouts + (1.0f - refs_blend) * c.map_fouts;
      c.flow_fouts = std::max(blended, 1.0f);
    }
  }

  subject_.graph.forEachNet([&](Net net, const Instance*, uint32_t) {
    Node* nd = node(net);

    if (!isMappable(net)) {
      // PI: normal polarity is free, inverted costs an inverter
      nd->pol[0].farea = 0.0f;
      nd->pol[1].farea = invArea / nd->pol[1].flow_fouts;
      return;
    }

    for (int C = 0; C < 2; C++) {
      float bestArea = 1.0e30f;
      ClassMatch* bestMatch = nullptr;
      MapTarget* bestTarget = nullptr;

      for (int mi = 0; mi < nd->nmatches; mi++) {
        ClassMatch& m = nd->matches[mi];
        auto it = subject_.target_index->classes.find(
            {m.npn.ninputs(), m.semiclass});
        if (it == subject_.target_index->classes.end()) {
          continue;
        }

        for (auto& target : it->second) {
          NPN localMap = target.via * m.npn;
          if (localMap.output_complement != (C != 0)) {
            continue;
          }

          float area = target.cell->area();
          bool feasible = true;
          for (int j = 0; j < kCutMaximum && m.cut[j] != Net::sentinel(); j++) {
            Literal cutLit = Literal::withPolarity(
                m.cut[j], !localMap.input_complement[j]);
            auto& cutPol = node(cutLit.net())->pol[!cutLit.isPositive()];
            if (isMappable(cutLit.net()) && cutPol.selected.first == nullptr) {
              feasible = false;
              break;
            }
            area += cutPol.farea;
          }

          if (!feasible) {
            continue;
          }
          area = std::min(area, 1.0e20f);

          if (area < bestArea) {
            bestArea = area;
            bestMatch = &m;
            bestTarget = &target;
          }
        }
      }

      Literal lit = Literal::withPolarity(net, C == 0);
      auto& pol = nd->pol[C];
      if (pol.map_fouts != 0 && bestMatch != nullptr
          && (pol.selected.first != bestMatch
              || pol.selected.second != bestTarget)) {
        derefCut(lit);
        pol.selected = {bestMatch, bestTarget};
        refCut(lit);
      } else {
        pol.selected = {bestMatch, bestTarget};
      }

      pol.farea = bestArea / pol.flow_fouts;
    }
  });

  const float area = walkSelection(/*initial=*/initial);
  debugPrint(logger_,
             utl::SYN,
             "cm",
             1,
             "area flow ({:.1f}): {:.1f}",
             refs_blend,
             area);
}

void Mapping::exactRound()
{
  subject_.graph.forEachNet([&](Net net, const Instance*, uint32_t) {
    Node* nd = node(net);

    if (!isMappable(net)) {
      return;
    }

    for (int C = 0; C < 2; C++) {
      const Literal lit = Literal::withPolarity(net, C == 0);
      auto& pol = nd->pol[C];

      if (pol.map_fouts != 0) {
        derefCut(lit);
      }

      float bestArea = 1.0e30f;
      ClassMatch* bestMatch = nullptr;
      MapTarget* bestTarget = nullptr;

      for (int mi = 0; mi < nd->nmatches; mi++) {
        ClassMatch& m = nd->matches[mi];
        const auto it = subject_.target_index->classes.find(
            {m.npn.ninputs(), m.semiclass});
        if (it == subject_.target_index->classes.end()) {
          continue;
        }

        for (auto& target : it->second) {
          const NPN localMap = target.via * m.npn;
          if (localMap.output_complement != (C != 0)) {
            continue;
          }

          // Check feasibility
          bool feasible = true;
          for (int j = 0; j < kCutMaximum && m.cut[j] != Net::sentinel(); j++) {
            const Literal cutLit = Literal::withPolarity(
                m.cut[j], !localMap.input_complement[j]);
            const auto& cutPol = node(cutLit.net())->pol[!cutLit.isPositive()];
            if (isMappable(cutLit.net()) && cutPol.selected.first == nullptr) {
              feasible = false;
              break;
            }
          }
          if (!feasible) {
            continue;
          }

          // Temporarily select and measure exact area
          pol.selected = {&m, &target};
          float area = target.cell->area() + refCut(lit);
          area = std::min(area, 1.0e20f);
          derefCut(lit);

          if (area < bestArea) {
            bestArea = area;
            bestMatch = &m;
            bestTarget = &target;
          }
        }
      }

      pol.selected = {bestMatch, bestTarget};
      if (pol.map_fouts != 0) {
        refCut(lit);
      }
    }
  });

  const float area = walkSelection(/*initial=*/false);
  debugPrint(logger_, utl::SYN, "cm", 1, "exact: {:.1f}", area);
}

void Mapping::integrate()
{
  std::map<Net, Net> complements;
  auto& index = subject_.target_index;
  auto& g = subject_.graph;

  complements.insert_or_assign(Net::zero(), Net::one());
  complements.insert_or_assign(Net::undef(), Net::one());
  complements.insert_or_assign(Net::one(), Net::zero());

  subject_.graph.forEachNet([&](Net net, const Instance*, uint32_t) {
    if (net.isConst()) {
      return;
    }

    if (!isMappable(net)) {
      // PI: create inverter if the inverted polarity is needed
      auto& inv = node(net)->pol[1];
      if (inv.map_fouts) {
        Bundle inv_out = g.add<Target>(index->inverter, Bundle(net));
        complements.insert_or_assign(net, inv_out[0]);
      }
      return;
    }

    for (int C = 0; C < 2; C++) {
      auto& pol = node(net)->pol[C];
      if (pol.map_fouts == 0) {
        continue;
      }

      auto [match, target] = pol.selected;
      assert(match != nullptr && target != nullptr);
      NPN localMap = (target->via * match->npn).inv();

      // Build input bundle
      int ninputs = match->npn.ninputs();
      std::vector<Net> inputNets;
      inputNets.reserve(ninputs);
      for (int i = 0; i < ninputs; i++) {
        Net inet = match->cut[localMap.permutation[i]];
        if (localMap.input_complement[i]) {
          auto cit = complements.find(inet);
          assert(cit != complements.end());
          inet = cit->second;
        }
        inputNets.push_back(inet);
      }

      Bundle inputs = Bundle::fromVec(std::move(inputNets));
      const Bundle out = g.add<Target>(target->cell, std::move(inputs));
      if (C == 0) {
        g.forceReplace(BundleView(net), BundleView(out[0]));
      } else {
        complements.insert_or_assign(net, out[0]);
      }
    }

    // If the signal only exists inverted in the mapping, add an inverter back
    // for the sake of naming
    if (node(net)->pol[1].map_fouts && !node(net)->pol[0].map_fouts) {
      g.forceReplace(net, g.add<Not>(complements.at(net)));
    }
  });

  // Primary output fixups: replace original Not outputs with the mapped
  // complement
  for (auto [underlying, original] : primary_output_fixups_) {
    assert(std::find(primary_outputs_.begin(),
                     primary_outputs_.end(),
                     underlying)
           != primary_outputs_.end());  // `underlying` is a primary output
    assert(node(underlying.net())->pol[!underlying.isPositive()].map_fouts
           != 0);  // `underlying` exists in the mapping
    if (underlying.isNegative()) {
      auto cit = complements.find(underlying.net());
      assert(cit != complements.end());
      subject_.graph.forceReplace(BundleView(original),
                                  BundleView(cit->second));
    } else {
      subject_.graph.forceReplace(BundleView(original),
                                  BundleView(underlying.net()));
    }
  }

  Net high = Net::zero();
  Net low = Net::one();

  if (index->tie_low.first == index->tie_high.first) {
    Bundle tie_out = g.add<Target>(index->tie_low.first, Bundle());
    high = tie_out[index->tie_high.second];
    low = tie_out[index->tie_low.second];
  } else {
    high = g.add<Target>(index->tie_high.first,
                         Bundle())[index->tie_high.second];
    low = g.add<Target>(index->tie_low.first, Bundle())[index->tie_low.second];
  }

  subject_.graph.forEachInstance([&](const Instance* inst) {
    if (inst->is<Target>() || inst->is<Output>()) {
      const_cast<Instance*>(inst)->visitMut([&](Net& fanin) {
        if (fanin.isConst()) {
          if (fanin == Net::one()) {
            fanin = high;
          } else {
            fanin = low;
          }
        }
      });
    }
  });
}

}  // namespace cm

void mapCombinationals(Graph& g,
                       sta::Network* network,
                       utl::Logger* logger,
                       const Synthesis& syn)
{
  g.normalize();

  auto index = std::make_shared<cm::TargetIndex>();
  cm::buildIndex(network, *index, logger, syn);

  if (!index->inverter) {
    logger->error(utl::SYN, 16, "mapCombinationals: no inverter cell found");
    return;
  }

  if (!index->tie_low.first || !index->tie_high.first) {
    logger->error(utl::SYN, 1017, "mapCombinationals: no tie cells found");
    return;
  }

  g.normalize();

  cm::Mapping state{logger, {.target_index = index, .graph = g}};
  state.collectPrimaryOutputs();
  state.computeFanouts();
  state.prepareMatches(/*npriority_cuts=*/64,
                       /*nmatches_max=*/16,
                       /*max_cut_len=*/6);

  state.areaFlowRound(1.0f, /*initial=*/true);
  state.areaFlowRound(0.5f);
  state.areaFlowRound(0.2f);
  state.areaFlowRound(0.0f);
  state.exactRound();
  state.exactRound();
  state.exactRound();
  state.integrate();

  logger->info(utl::SYN, 17, "mapCombinationals: done");
  g.normalize();
}

}  // namespace syn
