// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include "flow/opt_gatefusion.h"

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <iterator>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "sta/FuncExpr.hh"
#include "sta/Liberty.hh"
#include "sta/Network.hh"
#include "sta/NetworkClass.hh"
#include "sta/PortDirection.hh"
#include "syn/ir/Bundle.h"
#include "syn/ir/Graph.h"
#include "syn/ir/Instance.h"
#include "syn/synthesis.h"
#include "utl/Logger.h"

namespace syn {
namespace {

struct Cube
{
  uint32_t pos = 0;  // bitmask: bit i set -> input i appears positively
  uint32_t neg = 0;  // bitmask: bit i set -> input i appears negatively

  uint32_t support() const { return pos | neg; }
  int size() const { return __builtin_popcount(support()); }
  int npos() const { return __builtin_popcount(pos); }
  int nneg() const { return __builtin_popcount(neg); }

  bool contradictory() const { return (pos & neg) != 0; }

  Cube merge(const Cube& o) const
  {
    return {.pos = pos | o.pos, .neg = neg | o.neg};
  }

  bool operator==(const Cube& o) const { return pos == o.pos && neg == o.neg; }
  bool operator<(const Cube& o) const
  {
    auto a = std::make_pair(npos(), nneg());
    auto b = std::make_pair(o.npos(), o.nneg());
    if (a != b) {
      return a < b;
    }
    return std::make_pair(pos, neg) < std::make_pair(o.pos, o.neg);
  }
};

struct SOP
{
  std::set<Cube> cubes;
  bool oc = false;

  bool isDisjoint() const
  {
    uint32_t seen = 0;
    for (auto& c : cubes) {
      uint32_t sup = c.support();
      if (seen & sup) {
        return false;
      }
      seen |= sup;
    }
    return true;
  }

  // Remove cubes that are supersets of other cubes (absorption law).
  // E.g. {~A*~B, ~B} -> {~B} since ~A*~B is absorbed by ~B.
  void absorb()
  {
    std::set<Cube> absorbed;
    for (auto& c1 : cubes) {
      for (auto& c2 : cubes) {
        if (&c1 == &c2) {
          continue;
        }
        if ((c1.pos & c2.pos) == c2.pos && (c1.neg & c2.neg) == c2.neg) {
          // c2 absorbs c1 (c2 is smaller/more general)
          absorbed.insert(c1);
          break;
        }
      }
    }
    for (auto& c : absorbed) {
      cubes.erase(c);
    }
  }
};

struct Fingerprint
{
  std::vector<std::pair<int, int>> cube_types;  // (npos, nneg)
  bool oc = false;

  bool operator<(const Fingerprint& o) const
  {
    if (cube_types != o.cube_types) {
      return cube_types < o.cube_types;
    }
    return oc < o.oc;
  }

  bool operator==(const Fingerprint& o) const
  {
    return cube_types == o.cube_types && oc == o.oc;
  }
};

static Fingerprint fingerprint(const SOP& sop)
{
  Fingerprint fp;
  fp.oc = sop.oc;
  for (auto& c : sop.cubes) {
    fp.cube_types.emplace_back(c.npos(), c.nneg());
  }
  std::ranges::sort(fp.cube_types);
  return fp;
}

static std::string cubeToString(const Cube& c,
                                const std::vector<sta::LibertyPort*>& ins)
{
  std::string s;
  for (int i = 0; i < ins.size(); i++) {
    if (c.pos & (1u << i)) {
      s += ins[i]->name();
    } else if (c.neg & (1u << i)) {
      s += std::string("!") + ins[i]->name();
    }
  }
  if (s.empty()) {
    s = "1";
  }
  return s;
}

static std::string sopToString(const SOP& sop,
                               const std::vector<sta::LibertyPort*>& ins)
{
  std::string s = sop.oc ? "!(" : "(";
  bool first = true;
  for (auto& c : sop.cubes) {
    if (!first) {
      s += " + ";
    }
    first = false;
    s += cubeToString(c, ins);
  }
  s += ")";
  return s;
}

static constexpr int MAX_CUBES = 1024;

static std::optional<SOP> sop_cross(const std::optional<SOP>& a,
                                    const std::optional<SOP>& b)
{
  if (!a || !b) {
    return {};
  }
  assert(!a->oc && !b->oc);
  SOP result;
  for (auto& ca : a->cubes) {
    for (auto& cb : b->cubes) {
      Cube merged = ca.merge(cb);
      if (!merged.contradictory()) {
        result.cubes.insert(merged);
      }
      if ((int) result.cubes.size() > MAX_CUBES) {
        return {};
      }
    }
  }
  result.absorb();
  return result;
}

static std::optional<SOP> sop_union(const std::optional<SOP>& a,
                                    const std::optional<SOP>& b)
{
  if (!a || !b) {
    return {};
  }
  assert(!a->oc && !b->oc);
  SOP result;
  result.cubes = a->cubes;
  result.cubes.insert(b->cubes.begin(), b->cubes.end());
  return result;
}

// Recursively convert FuncExpr to SOP
// `ins` maps LibertyPort* -> bit index in the cube bitmasks.
static std::optional<SOP> fexprToSop(sta::FuncExpr* expr,
                                     bool negated,
                                     const std::vector<sta::LibertyPort*>& ins)
{
  using Op = sta::FuncExpr::Op;
  switch (expr->op()) {
    case Op::port: {
      int idx = -1;
      for (int i = 0; i < ins.size(); i++) {
        if (ins[i] == expr->port()) {
          idx = i;
          break;
        }
      }
      assert(idx >= 0);  // sentinel -1 is excluded by the assert above
      Cube c;
      // NOLINTBEGIN(clang-analyzer-core.BitwiseShift) — idx >= 0 per assert
      if (negated) {
        c.neg = 1u << idx;
      } else {
        c.pos = 1u << idx;
      }
      // NOLINTEND(clang-analyzer-core.BitwiseShift)
      return SOP{.cubes = {c}};
    }
    case Op::not_:
      return fexprToSop(expr->left(), !negated, ins);
    case Op::and_:
      if (negated) {
        // DeMorgan: NOT(A AND B) = NOT(A) OR NOT(B)
        return sop_union(fexprToSop(expr->left(), true, ins),
                         fexprToSop(expr->right(), true, ins));
      } else {
        return sop_cross(fexprToSop(expr->left(), false, ins),
                         fexprToSop(expr->right(), false, ins));
      }
    case Op::or_:
      if (negated) {
        // DeMorgan: NOT(A OR B) = NOT(A) AND NOT(B)
        return sop_cross(fexprToSop(expr->left(), true, ins),
                         fexprToSop(expr->right(), true, ins));
      } else {
        return sop_union(fexprToSop(expr->left(), false, ins),
                         fexprToSop(expr->right(), false, ins));
      }
    case Op::xor_:
      return {};
    case Op::one:
      if (negated) {
        return SOP{.cubes = {}};  // empty SOP = always false
      } else {
        return SOP{.cubes = {{}}};  // one tautology cube (no literals)
      }
    case Op::zero:
      if (negated) {
        return SOP{.cubes = {{}}};
      } else {
        return SOP{.cubes = {}};
      }
  }
  return {};
}

// Extract a normalized disjoint SOP for a cell's output function.
// Returns nullopt if the cell can't be represented.
static std::optional<SOP> extractSop(sta::FuncExpr* fexpr,
                                     const std::vector<sta::LibertyPort*>& ins,
                                     utl::Logger* logger = nullptr,
                                     const char* cell_name = nullptr)
{
  // Try both polarities
  std::optional<SOP> direct = fexprToSop(fexpr, false, ins);
  std::optional<SOP> negated = fexprToSop(fexpr, true, ins);
  if (negated) {
    negated->oc = true;
  }

  if (logger && cell_name) {
    debugPrint(logger, utl::SYN, "gatefusion", 3, "    {}", cell_name);
    if (direct) {
      debugPrint(logger,
                 utl::SYN,
                 "gatefusion",
                 3,
                 "      direct SOP: {}, disjoint={}",
                 sopToString(*direct, ins),
                 direct->isDisjoint());
    }
    if (negated) {
      debugPrint(logger,
                 utl::SYN,
                 "gatefusion",
                 3,
                 "      negated SOP: {}, disjoint={}",
                 sopToString(*negated, ins),
                 negated->isDisjoint());
    }
  }

  // Drop any which are non-disjoint
  if (direct && !direct->isDisjoint()) {
    direct = {};
  }
  if (negated && !negated->isDisjoint()) {
    negated = {};
  }

  if (!direct && !negated) {
    return {};
  }
  if (direct && !negated) {
    return direct;
  }
  if (!direct && negated) {
    return negated;
  }

  // Both valid: prefer more cubes (normalization)
  if (negated->cubes.size() > direct->cubes.size()) {
    return negated;
  }
  if (direct->cubes.size() > negated->cubes.size()) {
    return direct;
  }
  return direct;
}

struct CellInfo
{
  sta::LibertyCell* cell;
  SOP sop;
  std::vector<sta::LibertyPort*> input_ports;
};

static std::string fpToString(const Fingerprint& fp)
{
  std::string s = "[";
  for (int i = 0; i < fp.cube_types.size(); i++) {
    if (i > 0) {
      s += ",";
    }
    s += "(" + std::to_string(fp.cube_types[i].first) + ","
         + std::to_string(fp.cube_types[i].second) + ")";
  }
  s += "] oc=";
  s += fp.oc ? "true" : "false";
  return s;
}

struct CellIndex
{
  std::map<Fingerprint, CellInfo> by_fingerprint;
  std::unordered_map<sta::LibertyCell*, CellInfo> by_cell;
};

static CellIndex buildIndex(sta::Network* network,
                            utl::Logger* logger,
                            const Synthesis& syn)
{
  CellIndex result;
  int indexed = 0, skipped = 0;

  sta::LibertyLibraryIterator* lib_iter = network->libertyLibraryIterator();
  while (lib_iter->hasNext()) {
    sta::LibertyLibrary* lib = lib_iter->next();
    sta::LibertyCellIterator cell_iter(lib);
    while (cell_iter.hasNext()) {
      sta::LibertyCell* cell = cell_iter.next();
      if (syn.dontUse(cell) || cell->hasSequentials()) {
        continue;
      }

      std::vector<sta::LibertyPort*> inputs;
      std::vector<sta::LibertyPort*> outputs;
      bool skip = false;

      sta::LibertyCellPortIterator port_iter(cell);
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

      if (skip || outputs.size() != 1 || !outputs[0]->function()
          || inputs.empty() || inputs.size() > 16) {
        skipped++;
        continue;
      }

      auto sop = extractSop(
          outputs[0]->function(), inputs, logger, cell->name().c_str());
      if (!sop) {
        debugPrint(logger,
                   utl::SYN,
                   "gatefusion",
                   2,
                   "  skip {} (SOP extraction failed)",
                   cell->name());
        skipped++;
        continue;
      }

      Fingerprint fp = fingerprint(*sop);
      debugPrint(logger,
                 utl::SYN,
                 "gatefusion",
                 2,
                 "  {}: {}",
                 cell->name(),
                 fpToString(fp));

      // Store in the per-cell lookup
      result.by_cell[cell]
          = CellInfo{.cell = cell, .sop = *sop, .input_ports = inputs};

      // Store in the fingerprint index (smallest area per fingerprint).
      auto it = result.by_fingerprint.find(fp);
      if (it == result.by_fingerprint.end()
          || cell->area() < it->second.cell->area()) {
        result.by_fingerprint[fp] = CellInfo{.cell = cell,
                                             .sop = std::move(*sop),
                                             .input_ports = std::move(inputs)};
      }
      indexed++;
    }
  }
  delete lib_iter;

  debugPrint(logger,
             utl::SYN,
             "gatefusion",
             1,
             "cell index: {} fingerprints from {} cells ({} skipped)",
             result.by_fingerprint.size(),
             indexed,
             skipped);
  return result;
}

static std::optional<SOP> composeSops(const SOP& sopA,
                                      const SOP& sopB,
                                      int indexB)
{
  uint32_t linkB = 1u << indexB;

  if ((!sopA.oc && sopB.cubes.contains(Cube{.pos = linkB}))
      || (sopA.oc && sopB.cubes.contains(Cube{.neg = linkB}))) {
    SOP result;
    result.cubes = sopB.cubes;
    result.cubes.erase(Cube{.pos = linkB});
    result.cubes.erase(Cube{.neg = linkB});
    // Transpose sopA's cubes
    for (auto& cube : sopA.cubes) {
      result.cubes.insert(Cube{.pos = cube.pos << 16, .neg = cube.neg << 16});
    }
    result.oc = sopB.oc;
    return result;
  }

  Cube sopA_primed;
  for (auto& cube : sopA.cubes) {
    if (cube.size() != 1) {
      return {};
    }
    sopA_primed.pos |= cube.neg;
    sopA_primed.neg |= cube.pos;
  }

  auto it = sopB.cubes.begin();
  for (; it != sopB.cubes.end(); it++) {
    if ((sopA.oc && (linkB & it->pos) != 0)
        || (!sopA.oc && (linkB & it->neg) != 0)) {
      SOP result;
      result.cubes = sopB.cubes;
      result.oc = sopB.oc;
      result.cubes.erase(*it);
      result.cubes.insert(
          Cube{.pos = (it->pos & ~linkB) | sopA_primed.pos << 16,
               .neg = (it->neg & ~linkB) | sopA_primed.neg << 16});
      return result;
    }
  }

  return {};
}

}  // anonymous namespace

class set_bits_iterator
{
  uint32_t bits;

 public:
  explicit set_bits_iterator(uint32_t b) : bits(b) {}
  struct Iterator
  {
    uint32_t bits;
    using value_type = unsigned;
    using difference_type = std::ptrdiff_t;
    using pointer = void;
    using reference = unsigned;
    using iterator_category = std::input_iterator_tag;

    Iterator(uint32_t b) : bits(b) {}
    reference operator*() const
    {
      return static_cast<unsigned>(__builtin_ctz(bits));
    }
    Iterator& operator++()
    {
      bits &= bits - 1;
      return *this;
    }  // clear lowest set bit
    Iterator operator++(int)
    {
      Iterator tmp = *this;
      ++*this;
      return tmp;
    }
    bool operator==(const Iterator& o) const { return bits == o.bits; }
    bool operator!=(const Iterator& o) const { return bits != o.bits; }
  };

  Iterator begin() const { return Iterator(bits); }
  Iterator end() const { return Iterator(0); }
};

void gateFusionOpt(Graph& g,
                   sta::Network* network,
                   utl::Logger* logger,
                   const Synthesis& syn)
{
  g.normalize();

  auto index = buildIndex(network, logger, syn);
  if (index.by_fingerprint.empty()) {
    return;
  }

  int total_fusions = 0;
  float total_area_delta = 0.0f;
  std::map<sta::LibertyCell*, int> removed_cells, added_cells;

  // Iterate until no more fusions are found.
  for (int round = 0;; round++) {
    g.normalize();

    // Recompute per-net fanout counts.
    std::vector<int> use_counts(g.tableSize(), 0);
    g.forEachInstance([&](const Instance* inst) {
      if (!inst->isMapped()) {
        return;
      }
      inst->visit([&](Net net) { use_counts[Graph::netId(net)]++; });
    });

    // Identify and apply fusions.
    std::unordered_set<const Instance*> visited;
    int fusions = 0;

    g.forEachInstance([&](const Instance* instB) {
      if (!instB->is<Target>() || visited.contains(instB)) {
        return;
      }

      auto* targetB = instB->as<Target>();
      if (!index.by_cell.contains(targetB->cell())) {
        return;
      }
      auto& sopB = index.by_cell.at(targetB->cell()).sop;

      const Bundle& inputsB = targetB->inputs();
      for (uint32_t i = 0; i < inputsB.width(); i++) {
        auto [instA, off] = g.resolve(inputsB[i]);
        if (!instA->is<Target>() || visited.contains(instA)) {
          continue;
        }

        auto* targetA = instA->as<Target>();
        if (!index.by_cell.contains(targetA->cell())) {
          return;
        }
        auto& sopA = index.by_cell.at(targetA->cell()).sop;

        // Check that A's output is only consumed by B (among Targets).
        Net outputA = g.output(instA)[off];
        if (use_counts[Graph::netId(outputA)] != 1) {
          continue;
        }

        // Try composition.
        auto composed_sop = composeSops(sopA, sopB, i);
        if (!composed_sop) {
          continue;
        }

        // Look up fingerprint in cell index.
        Fingerprint fp = fingerprint(*composed_sop);
        auto it = index.by_fingerprint.find(fp);
        if (it == index.by_fingerprint.end()) {
          continue;
        }
        const CellInfo& fused_cell = it->second;

        // Check area improvement.
        float area_before
            = targetB->cell()->area() + instA->as<Target>()->cell()->area();
        float area_after = fused_cell.cell->area();
        if (area_after >= area_before) {
          continue;
        }
        total_area_delta += area_after - area_before;

        const Bundle& inputsA = targetA->inputs();
        auto fused_cell_inputs
            = Bundle::sentinel(inputsB.width() + inputsA.width() - 1);

        assert(composed_sop->cubes.size() == fused_cell.sop.cubes.size());
        // Iterate in lockstep over the two SOPs to construct
        // `fused_cell_inputs`. The two SOPs have the same fingerprint, and
        // thanks to the order given to cubes (see `Cube::operator<`) we will be
        // visiting compatible cubes in pairs.
        for (auto composed_it = composed_sop->cubes.begin(),
                  target_it = fused_cell.sop.cubes.begin();
             composed_it != composed_sop->cubes.end()
             && target_it != fused_cell.sop.cubes.end();
             composed_it++, target_it++) {
          assert(composed_it->npos() == target_it->npos());
          assert(composed_it->nneg() == target_it->nneg());

          // Now match individual positive and negative literals
          {
            auto source = set_bits_iterator(composed_it->pos);
            auto target = set_bits_iterator(target_it->pos);
            for (auto s = source.begin(), t = target.begin();
                 s != source.end() && t != target.end();
                 s++, t++) {
              fused_cell_inputs.mutableNet(*t)
                  = *s >= 16 ? inputsA[*s - 16] : inputsB[*s];
            }
          }
          {
            auto source = set_bits_iterator(composed_it->neg);
            auto target = set_bits_iterator(target_it->neg);
            for (auto s = source.begin(), t = target.begin();
                 s != source.end() && t != target.end();
                 s++, t++) {
              fused_cell_inputs.mutableNet(*t)
                  = *s >= 16 ? inputsA[*s - 16] : inputsB[*s];
            }
          }
        }

        // Create the fused Target and replace B's output.
        Bundle fused_out = g.add<Target>(fused_cell.cell, fused_cell_inputs);
        g.replace(BundleView(g.output(instB)[0]),
                  BundleView(fused_out[0]),
                  Graph::Equivalence::TwoValued,
                  fused_cell_inputs);

        removed_cells[instA->as<Target>()->cell()]++;
        removed_cells[targetB->cell()]++;
        added_cells[fused_cell.cell]++;
        visited.insert(instA);
        visited.insert(instB);
        fusions++;
        break;  // B is consumed, move to next instance
      }
    });

    total_fusions += fusions;
    debugPrint(logger,
               utl::SYN,
               "gatefusion",
               1,
               "round {}: {} fusions",
               round,
               fusions);

    if (fusions == 0) {
      break;
    }
  }

  if (total_fusions > 0) {
    g.normalize();
  }

  logger->info(utl::SYN,
               40,
               "gatefusion: {} fusions applied, area delta {:.4f}",
               total_fusions,
               total_area_delta);

  for (auto& [cell, count] : removed_cells) {
    logger->info(utl::SYN, 44, "  -{} {}", cell->name(), count);
  }
  for (auto& [cell, count] : added_cells) {
    logger->info(utl::SYN, 45, "  +{} {}", cell->name(), count);
  }
}

}  // namespace syn
