// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// Enumerating Ashenhurst-Curtis decomposition with shared set is
// described in
//
// A. Mishchenko, R. Brayton, A. T. Calvino, and G. De Micheli,
// "Boolean decomposition revisited", Proc. IWLS'23.
// https://people.eecs.berkeley.edu/~alanmi/publications/2023/iwls23_lut.pdf
//
// Here used with adaptations to gate-level resynthesis

#include "flow/acd.h"

#include <algorithm>
#include <array>
#include <bit>
#include <cassert>
#include <cstdint>
#include <functional>
#include <limits>
#include <map>
#include <memory>
#include <optional>
#include <unordered_map>
#include <utility>
#include <vector>

#include "flow/combinational_mapper_npn.h"
#include "flow/target_index.h"
#include "sta/FuncExpr.hh"
#include "sta/Liberty.hh"
#include "sta/PortDirection.hh"
#include "utl/Logger.h"

namespace syn::acd {

void TruthTable::swapVars(int a, int b)
{
  if (a == b) {
    return;
  }
  std::swap(vars[a], vars[b]);
  const int n = nminterms();
  for (int o = 0; o < noutputs; ++o) {
    const int base = o * n;
    for (int i = 0; i < n; ++i) {
      if ((i >> a & 1) || !(i >> b & 1)) {
        continue;
      }
      const int j = (i & ~(1 << b)) | (1 << a);
      // std::swap won't bind std::vector<bool>'s proxy references, so swap the
      // two positions through plain bool temporaries.
      const bool vi = values[base + i], vj = values[base + j];
      values[base + i] = vj;
      values[base + j] = vi;
      const bool di = dontcares[base + i], dj = dontcares[base + j];
      dontcares[base + i] = dj;
      dontcares[base + j] = di;
    }
  }
}

void TruthTable::changeVars(const std::vector<int>& new_vars)
{
  assert(new_vars.size() == vars.size());
  const int nv = (int) vars.size();
  int max_var = 0;
  for (int v : vars) {
    max_var = std::max(max_var, v);
  }
  std::vector<int> vi(max_var + 1, -1);
  for (int i = 0; i < nv; ++i) {
    vi[vars[i]] = i;
  }
  std::vector<int> p(nv);
  for (int i = 0; i < nv; ++i) {
    assert(vi[new_vars[i]] != -1);
    p[vi[new_vars[i]]] = i;
  }
  const int n = nminterms();
  std::vector<bool> nv_vals(values.size());
  std::vector<bool> nv_dcs(dontcares.size());
  for (int o = 0; o < noutputs; ++o) {
    const int base = o * n;
    for (int i = 0; i < n; ++i) {
      int j = 0;
      for (int k = 0; k < nv; ++k) {
        if (i & 1 << k) {
          j |= 1 << p[k];
        }
      }
      nv_vals[base + j] = values[base + i];
      nv_dcs[base + j] = dontcares[base + i];
    }
  }
  vars = new_vars;
  values = std::move(nv_vals);
  dontcares = std::move(nv_dcs);
}

std::optional<int> TruthTable::findUnsupportedVar() const
{
  uint32_t supported = 0;
  for (int i = 0; i < (int) values.size(); i++) {
    for (int j = 0; j < (int) vars.size(); j++) {
      if (i & (1u << j)) {
        continue;
      }
      if (!dontcares[i] && !dontcares[i | (1u << j)]
          && values[i] != values[i | (1u << j)]) {
        supported |= 1u << j;
      }
    }
  }

  for (int j = 0; j < (int) vars.size(); j++) {
    if (!(supported & (1u << j))) {
      return j;
    }
  }
  return {};
}

std::optional<int> TruthTable::outputPassthrough(int out_idx) const
{
  const int nm = nminterms();
  const int base = out_idx * nm;
  for (int v = 0; v < (int) vars.size(); v++) {
    bool match = true;
    for (int m = 0; m < nm; m++) {
      if (dontcares[base + m]) {
        continue;
      }
      // Output value must equal bit v of the minterm at every care position.
      if (values[base + m] != (bool) ((m >> v) & 1)) {
        match = false;
        break;
      }
    }
    if (match) {
      return v;
    }
  }
  return {};
}

void TruthTable::shrinkToSupport()
{
  // Remove unsupported variables one at a time, recomputing after each drop
  // since merging a variable's cofactors can fill don't-cares that newly
  // support another variable.
  while (std::optional<int> j = findUnsupportedVar()) {
    const int nv = (int) vars.size();
    // Move the unsupported variable to the highest position so its two
    // cofactors are the contiguous low and high halves of each output block.
    swapVars(*j, nv - 1);
    const int old_n = 1 << nv;
    const int new_n = old_n >> 1;
    std::vector<bool> nv_vals(noutputs * new_n, false);
    std::vector<bool> nv_dcs(noutputs * new_n, true);
    for (int o = 0; o < noutputs; ++o) {
      for (int i = 0; i < new_n; ++i) {
        const int lo = o * old_n + i;  // dropped var = 0 cofactor
        const int hi = lo + new_n;     // dropped var = 1 cofactor
        const int dst = o * new_n + i;
        // Merge cofactors: take whichever side is care (they agree where both
        // are care, since the variable is unsupported); leave DC otherwise.
        if (!dontcares[lo]) {
          nv_vals[dst] = values[lo];
          nv_dcs[dst] = false;
        } else if (!dontcares[hi]) {
          nv_vals[dst] = values[hi];
          nv_dcs[dst] = false;
        }
      }
    }
    vars.pop_back();
    values = std::move(nv_vals);
    dontcares = std::move(nv_dcs);
  }
}

bool TruthTable::hasDontcares() const
{
  return std::find(dontcares.begin(), dontcares.end(), true) != dontcares.end();
}

namespace {

constexpr int kMaxRecursionDepth = 8;

// Get the output port of a single-output gate
static sta::LibertyPort* cellOutput(sta::LibertyCell* cell)
{
  sta::LibertyCellPortIterator iterator(cell);
  while (iterator.hasNext()) {
    sta::LibertyPort* port = iterator.next();
    if (port->direction()->isOutput()) {
      return port;
    }
  }
  return nullptr;
}

}  // namespace

MatchCache::MatchCache(utl::Logger* logger,
                       const cm::TargetIndex& index,
                       int max_arity)
    : index_(index), min_area_for_width_(max_arity + 1, kInfCost)
{
  nand2_ = nullptr;
  {
    NPN semiclass_map;
    const Truth6 nand2_canonical = npnSemiclass(0b0111, 2, semiclass_map);
    auto nand2_class = index.classes.find({2, nand2_canonical});
    if (nand2_class != index.classes.end()) {
      for (auto& target : nand2_class->second) {
        if ((target.via * semiclass_map).isPermutation()) {
          if (!nand2_ || target.cell->area() < nand2_->area()) {
            nand2_ = target.cell;
          }
        }
      }
    }
  }

  if (!nand2_) {
    logger->error(utl::SYN,
                  39,
                  "ACD remapper requires a NAND2 specimen in the library but "
                  "none was found");
  }

  for (const auto& [key, targets] : index.classes) {
    const auto [arity, repr] = key;
    if (arity < 0 || arity > max_arity) {
      continue;
    }

    for (const auto& t : targets) {
      if (t.cell && (Cost) t.cell->area() < min_area_for_width_[arity]) {
        min_area_for_width_[arity] = (Cost) t.cell->area();
      }
    }
  }
}

std::optional<CellMatch> MatchCache::match(Truth6 tt, int arity)
{
  if (arity == 0) {
    // Constants not supported
    return std::nullopt;
  }

  const auto key = std::make_pair(tt, arity);
  auto it = cache_.find(key);
  if (it != cache_.end()) {
    return it->second;
  }

  NPN to_canonical;
  const Truth6 canonical = npnSemiclass(tt, arity, to_canonical);

  const auto bucket_it = index_.classes.find({arity, canonical});
  std::optional<CellMatch> best;
  if (bucket_it != index_.classes.end()) {
    for (const auto& target : bucket_it->second) {
      const NPN residual = (target.via * to_canonical).inv();
      if (residual.isPermutation()) {
        const Cost area = (Cost) target.cell->area();
        if (!best || area < best->area) {
          best = CellMatch{.driver = cellOutput(target.cell),
                           .perm = {},
                           .arity = arity,
                           .area = area};
          // Cell port j is fed by inputs[residual.permutation[j]].
          for (int i = 0; i < arity; ++i) {
            best->perm[i] = residual.permutation[i];
          }
          for (int i = arity; i < 6; ++i) {
            best->perm[i] = -1;
          }
        }
      }
    }
  }
  cache_.emplace(key, best);
  return best;
}

// Find a cell matching `care_tt` on `care_mask`
std::optional<CellMatch> MatchCache::matchDC(Truth6 care_tt,
                                             Truth6 care_mask,
                                             int arity)
{
  if (arity == 0) {
    return std::nullopt;
  }
  const Truth6 full = mask6(arity);
  care_mask &= full;
  care_tt &= care_mask;
  if (care_mask == full) {
    return match(care_tt, arity);  // fully specified
  }
  const Truth6 dc = full & ~care_mask;
  if (std::popcount(dc) > kMaxDcFill) {
    return match(care_tt, arity);
  }

  std::optional<CellMatch> best;
  bool first = true;
  Truth6 fill_mask = ~care_mask & full;
  Truth6 fill = 0;
  for (; first || fill != 0;
       first = false, fill = ((fill | ~fill_mask) + 1) & fill_mask) {
    if (std::optional<CellMatch> m = match(care_tt | fill, arity);
        m && (!best || m->area < best->area)) {
      best = m;
    }
  }
  return best;
}

namespace {

// Approximate lower bound on the area cost of synthesizing a function
// of `nvars` inputs
Cost lutMinCost(const MatchCache& mc, int max_width, int nvars)
{
  if (nvars <= 0) {
    return 0;
  }
  Cost cost = 0;
  for (int w = max_width; w >= 2; --w) {
    const Cost per = mc.minAreaForWidth(w);
    if (per >= kInfCost) {
      continue;
    }
    const int n = (nvars - 1) / (w - 1);
    cost += n * per;
    nvars -= n * (w - 1);
    if (nvars <= 1) {
      break;
    }
  }
  assert(nvars == 1);
  return cost;
}

struct Fragment
{
  // indices of columns which determine this fragment
  std::vector<int> column_bound_idx;
  // bookkeeping for shared variable detection
  uint32_t bs_high = 0;
  uint32_t bs_low = 0;
};

bool columnsMatch(const TruthTable& tt,
                  int fraglen,
                  int b1,
                  int b2,
                  bool* b1_refined)
{
  bool refining = false;
  int nminterms = tt.nminterms();
  for (int out_idx = 0; out_idx < tt.noutputs; out_idx++) {
    auto v1 = tt.values.begin() + nminterms * out_idx + b1 * fraglen;
    auto v2 = tt.values.begin() + nminterms * out_idx + b2 * fraglen;
    auto d1 = tt.dontcares.begin() + nminterms * out_idx + b1 * fraglen;
    auto d2 = tt.dontcares.begin() + nminterms * out_idx + b2 * fraglen;

    for (int i = 0; i < fraglen; ++i) {
      // Are we refinining the column b1?
      if (*d1 && !*d2) {
        refining = true;
      }
      if (!*d1 && !*d2 && *v1 != *v2) {
        return false;
      }
      ++v1;
      ++v2;
      ++d1;
      ++d2;
    }
  }
  if (refining) {
    *b1_refined = refining;
  }
  return true;
}

bool fragmentMatches(const TruthTable& tt,
                     const Fragment& frag,
                     int fraglen,
                     int candidate_b,
                     bool* refined)
{
  if (frag.column_bound_idx.empty()) {
    return false;
  }
  for (int b1 : frag.column_bound_idx) {
    if (!columnsMatch(tt, fraglen, b1, candidate_b, refined)) {
      return false;
    }
  }
  return true;
}

struct FragmentSet
{
  std::vector<Fragment> fragments;
  // map from bound variable values to fragment index
  std::vector<int> fragment_map;
};

// Identify fragments and fragment decoding map
FragmentSet findFragments(const TruthTable& tt, int bn)
{
  const int nvars = (int) tt.vars.size();
  const int fn = nvars - bn;
  assert(fn >= 0);
  const int fraglen = 1 << fn;
  const uint32_t bs_mask = (1u << bn) - 1;

  FragmentSet result;
  result.fragment_map.assign(1 << bn, -1);
  for (int b = 0; b < 1 << bn; ++b) {
    bool matched = false;
    for (size_t fi = 0; fi < result.fragments.size(); ++fi) {
      bool refined = false;
      if (fragmentMatches(tt, result.fragments[fi], fraglen, b, &refined)) {
        Fragment& frag = result.fragments[fi];
        if (refined) {
          frag.column_bound_idx.push_back(b);
        }
        frag.bs_high |= (uint32_t) b;
        frag.bs_low |= bs_mask & ~(uint32_t) b;
        result.fragment_map[b] = (int) fi;
        matched = true;
        break;
      }
    }
    if (!matched) {
      Fragment frag;
      frag.column_bound_idx.push_back(b);
      frag.bs_high = (uint32_t) b;
      frag.bs_low = bs_mask & ~(uint32_t) b;
      result.fragment_map[b] = (int) result.fragments.size();
      result.fragments.push_back(std::move(frag));
    }
  }
  return result;
}

// Project the multi-output TT `f` down to the outputs in `keep_indices`,
// preserving variables.
TruthTable projectOutputs(const TruthTable& f,
                          const std::vector<int>& keep_indices)
{
  TruthTable g;
  g.vars = f.vars;
  g.noutputs = (int) keep_indices.size();
  const int nm = 1 << (int) f.vars.size();
  g.values.assign((size_t) g.noutputs * nm, false);
  g.dontcares.assign((size_t) g.noutputs * nm, false);
  for (int o = 0; o < g.noutputs; ++o) {
    const int src = keep_indices[o];
    assert(src >= 0 && src < f.noutputs);
    std::copy(f.values.begin() + (size_t) src * nm,
              f.values.begin() + (size_t) (src + 1) * nm,
              g.values.begin() + (size_t) o * nm);
    std::copy(f.dontcares.begin() + (size_t) src * nm,
              f.dontcares.begin() + (size_t) (src + 1) * nm,
              g.dontcares.begin() + (size_t) o * nm);
  }
  return g;
}

// `Round` represents one level of decomposition.
struct Round
{
  sta::LibertyPort* driver = nullptr;  // decoder cell (null at a leaf round)
  std::vector<int> inputs;  // var IDs feeding the cell's ports, in port order
  int output = -1;          // var ID the decoder cell drives

  // One entry per output of this round's function.
  //
  // If >=0, the output passes through the given var ID, if -1, the output
  // is produced by `next` or `split`
  std::vector<int> out_passthrough;

  std::unique_ptr<Round> next;

  // If `split` is non-empty, from now on the decomposition is split into two
  // independent chains per output. This is used when decomposing a multi-output
  // function once the two outputs stop sharing structure.
  std::vector<std::unique_ptr<Round>> split;

  Cost cost = 0;
};

Truth6 swapVars6(Truth6 t, int i, int j)
{
  if (i == j) {
    return t;
  }
  if (i > j) {
    std::swap(i, j);
  }
  const Truth6 mi = cofactor_masks[i];
  const Truth6 mj = cofactor_masks[j];
  const Truth6 stay = t & ~(mi ^ mj);  // x_i == x_j
  const Truth6 i1j0 = t & mi & ~mj;    // x_i=1, x_j=0
  const Truth6 i0j1 = t & ~mi & mj;    // x_i=0, x_j=1
  const int d = (1 << j) - (1 << i);
  return stay | (i1j0 << d) | (i0j1 >> d);
}

// Partition the inputs of `f` into symmetry classes: positions in the same
// class are mutually interchangeable. Returns a class id per position
// (positions sharing an id are interchangeable).
std::vector<int> symmetryClasses(const TruthTable& f)
{
  const int nv = (int) f.vars.size();
  const int nm = 1 << nv;
  const Truth6 fmask = mask6(nv);

  // Pack each output's values / don't-cares into Truth6 words.
  std::vector<Truth6> val(f.noutputs, 0), dc(f.noutputs, 0);
  for (int o = 0; o < f.noutputs; ++o) {
    const int base = o * nm;
    for (int m = 0; m < nm; ++m) {
      if (f.values[base + m]) {
        val[o] |= (Truth6) 1 << m;
      }
      if (f.dontcares[base + m]) {
        dc[o] |= (Truth6) 1 << m;
      }
    }
  }

  auto symmetric = [&](int i, int j) {
    for (int o = 0; o < f.noutputs; ++o) {
      // The don't-care set must be invariant under the swap...
      if ((swapVars6(dc[o], i, j) & fmask) != (dc[o] & fmask)) {
        return false;
      }
      // ...and the care values must agree where defined.
      const Truth6 care = fmask & ~dc[o];
      if (((swapVars6(val[o], i, j) ^ val[o]) & care) != 0) {
        return false;
      }
    }
    return true;
  };

  std::vector<int> parent(nv);
  for (int i = 0; i < nv; ++i) {
    parent[i] = i;
  }
  auto find = [&](int x) {
    while (parent[x] != x) {
      parent[x] = parent[parent[x]];
      x = parent[x];
    }
    return x;
  };
  for (int i = 0; i < nv; ++i) {
    for (int j = i + 1; j < nv; ++j) {
      if (symmetric(i, j)) {
        parent[find(i)] = find(j);
      }
    }
  }
  std::vector<int> cls(nv);
  for (int i = 0; i < nv; ++i) {
    cls[i] = find(i);
  }
  return cls;
}

struct Objective
{
  float min_slacks[2];
  double slack_cost_factor[2];
};

static std::string hexdump(std::vector<bool> tt)
{
  std::string ret;
  for (int i = 0; i < (int) tt.size(); i += 4) {
    int nibble = 0;
    for (int j = i; j < (int) tt.size() && j < i + 4; j++) {
      if (tt[j]) {
        nibble |= 1 << (j - i);
      }
    }
    ret += nibble < 10 ? '0' + nibble : 'A' - 10 + nibble;
  }
  return ret;
}

class Search
{
 public:
  utl::Logger* logger_;
  MatchCache& mc_;
  const DelayEstimationParameters& dparams_;
  const SynthesisProblem& problem_;
  Objective& objective_;
  int max_bound_width_;
  // When false, only variable-count-reducing rounds are explored (no lateral
  // factor-out moves).  Much faster, at the cost of missing decompositions that
  // need a lateral intermediate.
  bool lateral_enabled_ = true;
  int next_var_;

  // Input symmetry classes of the original function. Keyed by variable ID
  // it gives the symmetry class, and ranking of each variable in said class
  std::vector<std::pair<int, int>> sym_info_;

  bool timing_ = false;

  Search(utl::Logger* logger,
         MatchCache& mc,
         const DelayEstimationParameters& dparams,
         const SynthesisProblem& problem,
         Objective& objective,
         int max_bound_width,
         bool lateral_enabled)
      : logger_(logger),
        mc_(mc),
        dparams_(dparams),
        problem_(problem),
        objective_(objective),
        max_bound_width_(max_bound_width),
        lateral_enabled_(lateral_enabled),
        next_var_((int) problem_.inputs.size())
  {
    // Assert cannonical ordering. Variables being in the range 0...ninputs
    // is assumed by the symmetry class processing below and by initilization
    // of `next_var_`.
    for (int i = 0; i < problem_.inputs.size(); i++) {
      assert(problem_.function.vars[i] == i);
    }

    // If slack cost factor is zero on both outputs, disable timing mode.
    timing_ = !(objective_.slack_cost_factor[0] == 0.0
                && objective_.slack_cost_factor[1] == 0.0);

    if (timing_) {
      for (size_t i = 0; i < problem_.inputs.size(); i++) {
        setArrivals(i, problem_.inputs[i].arrivals);
      }
    }

    // Characterize symmetries once up front so that we can use them later
    // to prune redundant branches when searching decompositions
    const std::vector<int> classes = symmetryClasses(problem_.function);

    // pairs with (var ID, class)
    std::vector<std::pair<int, int>> classified;
    classified.reserve(problem_.inputs.size());
    for (int i = 0; i < problem_.inputs.size(); i++) {
      classified.push_back(
          std::make_pair(problem_.function.vars[i], classes[i]));
    }

    // Re-order the classified so that within a symmetry class, variables
    // with extra slack come first.
    std::stable_sort(classified.begin(), classified.end(), [&](auto a, auto b) {
      if (a.second != b.second) {
        return a.second < b.second;
      }
      if (timing_) {
        return arrivals_[a.first].maxEntry() < arrivals_[b.first].maxEntry();
      } else {
        return a.first < b.first;
      }
    });

    // Populate sym_info with findings
    int small_class_id = 0;
    sym_info_.assign(problem.inputs.size(), std::make_pair<int, int>(-1, 0));
    for (size_t i = 0; i < classified.size(); i++) {
      int class_ = classified[i].second;
      size_t size = 1;
      for (; i + size < classified.size()
             && classified[i + size].second == class_;
           size++)
        ;
      // only classes with more than one member require noting down
      if (size > 1) {
        for (int j = 0; j < size; j++) {
          sym_info_[classified[i + j].first]
              = std::make_pair(small_class_id, j);
        }

        if (logger->debugCheck(utl::SYN, "acd", 3)) {
          std::string members_text;
          for (int j = 0; j < size; j++) {
            members_text += std::to_string(classified[i + j].first) + " ";
          }
          debugPrint(logger_,
                     utl::SYN,
                     "acd",
                     2,
                     "symmetry set {}: {}",
                     small_class_id,
                     members_text);
        }

        small_class_id++;
      }

      i += size - 1;
    }

    // 6 input function can have at most 3 classes if each has at least two
    // members.
    assert(small_class_id <= 3);
  }

  std::unique_ptr<Round> run(Cost budget)
  {
    std::vector<int> global_output_ids;
    for (int i = 0; i < problem_.outputs.size(); i++) {
      global_output_ids.push_back(i);
    }

    return recurse(problem_.function,
                   global_output_ids,
                   0,
                   /*allow_lateral=*/true,
                   budget,
                   SITracking{});
  }

  // Total exploreUnified invocations (incl. recursion) during the last run().
  long long exploreCalls() const { return explore_calls_; }

 private:
  struct SITracking
  {
    SITracking() {}
    int sym_classes_consumed[3] = {0, 0, 0};
  };

  // A helper used to enforce variables in a symmetry class being consumed
  // in one canonical order as different orders would be redundant
  bool consumeSymmetricInputs(SITracking prior,
                              SITracking& updated,
                              std::span<int> bound_vars)
  {
    updated = prior;

    for (auto var_idx : bound_vars) {
      if (var_idx < problem_.inputs.size()) {
        const auto [cb, pb] = sym_info_.at(var_idx);
        if (cb >= 0) {
          updated.sym_classes_consumed[cb]++;
        }
      }
    }

    for (auto var_idx : bound_vars) {
      if (var_idx < problem_.inputs.size()) {
        const auto [cb, pb] = sym_info_.at(var_idx);
        if (cb >= 0) {
          if (pb < prior.sym_classes_consumed[cb]
              || pb >= updated.sym_classes_consumed[cb]) {
            return false;
          }
        }
      }
    }

    return true;
  }

  std::pair<int, int> symClass(int var_idx)
  {
    if (var_idx >= problem_.inputs.size()) {
      return std::make_pair(-1, 0);
    } else {
      return sym_info_.at(var_idx);
    }
  }

  NodeArrivals cellDelay(sta::LibertyPort* output,
                         const std::vector<NodeArrivals>& input_arrivals)
  {
    return outputArrival(output,
                         input_arrivals,
                         avgInputCap(output->libertyCell()) * 2,
                         dparams_.corner,
                         dparams_.fixed_slews);
  }

  std::unique_ptr<Round> recurse(TruthTable f,
                                 std::span<int> f_output_ids,
                                 int depth,
                                 bool allow_lateral,
                                 Cost budget,
                                 SITracking prior_si)
  {
    ++explore_calls_;
    if (depth > kMaxRecursionDepth) {
      debugPrint(logger_,
                 utl::SYN,
                 "acd",
                 2,
                 "{:{}s}depth limit reached",
                 "",
                 depth * 2);
      return nullptr;
    }

    int npeeled = 0;
    // Peeling preamble: an output that is just a passthrough of one of the
    // inputs needs no logic of its own, so record the variable it wires to and
    // drop it from the problem.
    std::vector<int> out_passthrough;
    out_passthrough.assign(f.noutputs, -1);
    std::vector<int> kept_positions;
    std::vector<int> kept_outputs;
    assert(f_output_ids.size() == f.noutputs);
    for (int i = 0; i < f.noutputs; i++) {
      if (std::optional<int> v = f.outputPassthrough(i)) {
        out_passthrough[i] = f.vars[*v];
        npeeled++;
      } else {
        kept_positions.push_back(i);
        kept_outputs.push_back(f_output_ids[i]);
      }
    }

    if (kept_outputs.empty()) {
      Round r;
      r.out_passthrough = std::move(out_passthrough);
      if (timing_) {
        for (int i = 0; i < f.noutputs; i++) {
          auto idx = f_output_ids[i];
          r.cost += arrivals_.at(r.out_passthrough[i]).exitSlack(idx)
                    * objective_.slack_cost_factor[idx];
        }
      }
      if (r.cost >= budget) {
        return nullptr;
      }
      return std::make_unique<Round>(std::move(r));
    }
    f = projectOutputs(f, kept_positions);
    f.shrinkToSupport();
    f_output_ids = {kept_outputs};
    // done peeling

    if (f.hasDontcares()) {
      debugPrint(logger_,
                 utl::SYN,
                 "acd",
                 2,
                 "{:{}s}{} (dontcare {}) nvars={} npeeled={}",
                 "",
                 depth * 2,
                 hexdump(f.values),
                 hexdump(f.dontcares),
                 f.vars.size(),
                 npeeled);
    } else {
      debugPrint(logger_,
                 utl::SYN,
                 "acd",
                 2,
                 "{:{}s}{} nvars={} npeeled={}",
                 "",
                 depth * 2,
                 hexdump(f.values),
                 f.vars.size(),
                 npeeled);
    }

    if (f.vars.empty()) {
      return nullptr;
    }

    Cost slack_cost = 0;
    for (int i = 0; i < f.noutputs; i++) {
      auto idx = f_output_ids[i];
      float out_slack = slackUpperBound(f.vars, idx);
      if (out_slack < objective_.min_slacks[idx]) {
        return nullptr;
      }
      slack_cost += out_slack * objective_.slack_cost_factor[idx];
    }

    {
      Cost lb = slack_cost + lutMinCost(mc_, max_bound_width_, f.vars.size());
      if (lb >= budget) {
        debugPrint(logger_,
                   utl::SYN,
                   "acd",
                   2,
                   "{:{}s}  reject by lower cost bound",
                   "",
                   depth * 2);
        return nullptr;
      }
    }

    std::unique_ptr<Round> best_decomp;

    // Leaf base case: realize the whole single-output function as one library
    // cell, exploiting don't-cares.
    if (f.noutputs == 1) {
      const int nvars = f.vars.size();
      const int nm = 1 << nvars;
      Truth6 care_tt = 0;
      Truth6 care_mask = 0;
      for (int m = 0; m < nm; m++) {
        if (!f.dontcares[m]) {
          care_mask |= (Truth6) 1 << m;
          if (f.values[m]) {
            care_tt |= (Truth6) 1 << m;
          }
        }
      }
      if (std::optional<CellMatch> cm
          = mc_.matchDC(care_tt, care_mask, nvars)) {
        Cost solution_cost;
        NodeArrivals leaf_arr;
        if (timing_) {
          std::vector<NodeArrivals> port_in(cm->arity);
          for (int j = 0; j < cm->arity; j++) {
            port_in[j] = arrivals_.at(f.vars[cm->perm[j]]);
          }
          leaf_arr = cellDelay(cm->driver, port_in);
          solution_cost = cm->area
                          + leaf_arr.exitSlack(f_output_ids[0])
                                * objective_.slack_cost_factor[f_output_ids[0]];
        } else {
          solution_cost = cm->area;
        }

        if (solution_cost < budget) {
          const int id = next_var_++;
          if (timing_) {
            setArrivals(id, leaf_arr);
          }
          Round leaf;
          leaf.driver = cm->driver;
          leaf.output = id;
          leaf.inputs.resize(cm->arity);
          for (int j = 0; j < cm->arity; j++) {
            leaf.inputs[j] = f.vars[cm->perm[j]];
          }
          // The single output is the cell's output net.
          leaf.next = std::make_unique<Round>();
          leaf.next->out_passthrough = {id};
          leaf.cost = solution_cost;

          debugPrint(logger_,
                     utl::SYN,
                     "acd",
                     2,
                     "{:{}s} exact match {} accepted (cost={})",
                     "",
                     depth * 2,
                     cm->driver->libertyCell()->name(),
                     solution_cost);

          leaf.out_passthrough = std::move(out_passthrough);
          return std::make_unique<Round>(std::move(leaf));
        }
      }
    }

    for (int bn = 2; bn <= max_bound_width_ && bn <= f.vars.size(); ++bn) {
      if (slack_cost + mc_.minAreaForWidth(bn) >= budget) {
        break;
      }

      const int fn = f.vars.size() - bn;
      if (fn == 0) {
        // Bound = all vars: there is exactly one bound set, so handle it as a
        // single trial.
        if (auto candidate = tryBoundChoice(f,
                                            kept_outputs,
                                            bn,
                                            fn,
                                            budget,
                                            depth,
                                            allow_lateral,
                                            prior_si);
            candidate && candidate->cost < budget) {
          budget = candidate->cost;
          best_decomp = std::move(candidate);
        }
        continue;
      }
      TruthTable cur = f;
      std::vector<int> p(bn, 0);
      int level = bn - 1;

      while (true) {
        {
          // Top `bn` positions in the variable list are the bound set under
          // test
          auto bs = std::span<int>(cur.vars.data() + fn, bn);

          std::unique_ptr<Round> candidate;
          SITracking updated_si;

          if (consumeSymmetricInputs(prior_si, updated_si, bs)) {
            if (logger_->debugCheck(utl::SYN, "acd", 3)) {
              std::string bs_text;
              for (auto bvar : bs) {
                bs_text += std::to_string(bvar) + " ";
              }
              debugPrint(logger_,
                         utl::SYN,
                         "acd",
                         3,
                         "{:{}s} bs {}",
                         "",
                         depth * 2,
                         bs_text);
            }

            candidate = tryBoundChoice(cur,
                                       f_output_ids,
                                       bn,
                                       fn,
                                       budget,
                                       depth,
                                       allow_lateral,
                                       updated_si);
            if (candidate && candidate->cost < budget) {
              budget = candidate->cost;
              best_decomp = std::move(candidate);
            }
          } else {
            if (logger_->debugCheck(utl::SYN, "acd", 3)) {
              std::string bs_text;
              for (auto bvar : bs) {
                bs_text += std::to_string(bvar) + " ";
              }
              debugPrint(logger_,
                         utl::SYN,
                         "acd",
                         3,
                         "{:{}s} bs {}redundant by symmetry",
                         "",
                         depth * 2,
                         bs_text);
            }
          }
        }

        if (level < 0) {
          break;
        }
        cur.swapVars(fn + level, p[level]);
        p[level]++;
        if (p[level] == fn) {
          level--;
        } else {
          for (; level < bn - 1; ++level) {
            p[level + 1] = p[level];
          }
        }
      }
    }

    // Output-split candidate: decompose the two outputs independently
    if (depth >= 1 && f.noutputs == 2) {
      TruthTable f0 = projectOutputs(f, {0});
      TruthTable f1 = projectOutputs(f, {1});
      std::unique_ptr<Round> s0, s1;
      debugPrint(
          logger_, utl::SYN, "acd", 3, "{:{}s} splitting", "", depth * 2);

      static std::array output0 = {0};
      if ((s0 = recurse(f0,
                        output0,
                        depth + 1,
                        allow_lateral,
                        budget
                            - objective_.slack_cost_factor[1]
                                  * slackUpperBound(f.vars, 1),
                        prior_si))
          != nullptr) {
        static std::array output1 = {1};
        if ((s1 = recurse(f1,
                          output1,
                          depth + 1,
                          allow_lateral,
                          budget - s0->cost,
                          prior_si))
            != nullptr) {
          Round round;
          round.cost = s0->cost + s1->cost;
          round.split.push_back(std::move(s0));
          round.split.push_back(std::move(s1));
          best_decomp = std::make_unique<Round>(std::move(round));
        }
      }
    }

    if (!best_decomp) {
      return nullptr;
    }

    best_decomp->out_passthrough = std::move(out_passthrough);
    return best_decomp;
  }

  // Try one choice of bound-set variables
  //
  // When called, `g` will be permuted for bound variables to sit at the top
  // positions, i.e. [fn, fn+b)
  std::unique_ptr<Round> tryBoundChoice(const TruthTable& g,
                                        std::span<int> f_output_ids,
                                        int bn,  // bound set size
                                        int fn,  // free set size
                                        Cost budget,
                                        int depth,
                                        bool allow_lateral,
                                        SITracking prior_si)
  {
    const int nvars = (int) g.vars.size();
    const FragmentSet fset = findFragments(g, bn);
    const int nfrags = (int) fset.fragments.size();
    const int ndecoding_luts_raw
        = nfrags > 1 ? (int) std::bit_width((unsigned) (nfrags - 1)) : 0;

    if (ndecoding_luts_raw < 1 || ndecoding_luts_raw > 2) {
      return nullptr;
    }

    bool have_solution = false;
    Round best_local;

    // sh == -1: no shared variable (raw == 1).  sh >= 0: share bound position
    // sh (raw == 2).  Try every feasible shared variable.
    const int sh_first = (ndecoding_luts_raw == 1) ? -1 : 0;
    const int sh_last = (ndecoding_luts_raw == 1) ? -1 : bn - 1;
    for (int sh = sh_first; sh <= sh_last; ++sh) {
      // Skip redundant shared-variable choices by original function symmetry
      if (sh >= 0) {
        const int sh_cls = symClass(g.vars[fn + sh]).first;
        bool redundant = false;
        for (int sh2 = 0; sh2 < sh; ++sh2) {
          if (sh_cls >= 0 && symClass(g.vars[fn + sh2]).first == sh_cls) {
            redundant = true;
            break;
          }
        }
        if (redundant) {
          continue;
        }
      }

      const uint32_t shmask = (sh < 0) ? 0u : (1u << sh);
      const int nshared = (sh < 0) ? 0 : 1;
      const int ngroups = 1 << nshared;

      // A "lateral" round does not reduce the variable count (upper_nvars ==
      // nvars); it factors out a non-trivial intermediate (e.g. a 2-input
      // decoder) before a later cell match.
      const bool lateral = (fn + 1 + nshared == nvars);
      if (lateral && (!allow_lateral || !lateral_enabled_)) {
        continue;
      }

      // Assign every fragment, within its shared-value group, a stable slot in
      // [0, kSlots).  If a group needs a third slot, one LUT bit cannot encode
      // it, so this shared variable is infeasible.
      std::array<std::vector<int>, 2> group_frags;
      std::vector<int> group_of(1 << bn);
      std::vector<int> slot_of(1 << bn);
      bool feasible = true;
      for (int b = 0; b < (1 << bn) && feasible; ++b) {
        const int grp = shmask ? ((b >> sh) & 1) : 0;
        const int frag = fset.fragment_map[b];
        std::vector<int>& slots = group_frags[grp];
        auto it = std::find(slots.begin(), slots.end(), frag);
        int slot = (int) (it - slots.begin());
        if (it == slots.end()) {
          if (slot >= 2) {
            feasible = false;
            break;
          }
          slots.push_back(frag);
        }
        group_of[b] = grp;
        slot_of[b] = slot;
      }
      if (!feasible) {
        continue;
      }

      // Each candidate flips, per group, whether slot 0 or slot 1 maps to a
      // decoder output of 1: code(b) = flip[group(b)] ^ slot(b).  That is 2
      // candidates for raw == 1 and 4 for raw == 2.
      for (int flip = 0; flip < (1 << ngroups); ++flip) {
        Truth6 decoder = 0;
        std::vector<int> bound_to_code(1 << bn);
        for (int b = 0; b < (1 << bn); ++b) {
          const int code = ((flip >> group_of[b]) & 1) ^ slot_of[b];
          bound_to_code[b] = code;
          if (code) {
            decoder |= (Truth6) 1 << b;
          }
        }

        // Shrink the decoder to its support over the bn bound vars, then look
        // up a P-only library cell for it.
        std::vector<int> bound_var_indices;
        for (int v = 0; v < bn; ++v) {
          const Truth6 mv = cofactor_masks[v];
          if ((decoder & ~mv) != ((decoder & mv) >> (1 << v))) {
            bound_var_indices.push_back(v);
          }
        }
        const int arity = (int) bound_var_indices.size();
        if (arity == 0) {
          continue;  // constant decoder: not a useful round
        }
        if (lateral && arity < 2) {
          continue;
        }
        Truth6 shrunk = 0;
        for (int b = 0; b < (1 << bn); ++b) {
          if (!(decoder & (Truth6) 1 << b)) {
            continue;
          }
          int nb = 0;
          for (int k = 0; k < arity; ++k) {
            if (b & (1 << bound_var_indices[k])) {
              nb |= 1 << k;
            }
          }
          shrunk |= (Truth6) 1 << nb;
        }
        shrunk &= mask6(arity);

        const std::optional<CellMatch> m = mc_.match(shrunk, arity);
        if (!m) {
          continue;
        }
        const Cost decoding_cost = m->area;

        // Fresh, globally-unique ID for this round's decoder output.
        const int dec = next_var_++;
        if (timing_) {
          std::vector<NodeArrivals> port_in(arity);
          for (int j = 0; j < arity; ++j) {
            port_in[j]
                = arrivals_.at(g.vars[fn + bound_var_indices[m->perm[j]]]);
          }
          setArrivals(dec, cellDelay(m->driver, port_in));
        }

        TruthTable upper = buildUpperFunction(g,
                                              bn,
                                              fn,
                                              /*ndecoding_luts=*/1,
                                              shmask,
                                              nshared,
                                              bound_to_code,
                                              dec);
        std::unique_ptr<Round> sub;

        if ((sub = recurse(upper,
                           f_output_ids,
                           depth + 1,
                           /*allow_lateral=*/!lateral,
                           budget - decoding_cost,
                           prior_si))
            == nullptr) {
          continue;
        }

        budget = sub->cost + decoding_cost;

        // Wire the cell's input ports to bound-variable IDs: cell port j is fed
        // by canonical input m->perm[j], i.e. bound position
        // bound_var_indices[m->perm[j]] of the decomposition's variable order.
        std::vector<int> port_inputs(arity);
        for (int j = 0; j < arity; ++j) {
          port_inputs[j] = g.vars[fn + bound_var_indices[m->perm[j]]];
        }
        best_local = Round{};
        best_local.driver = m->driver;
        best_local.inputs = std::move(port_inputs);
        best_local.output = dec;
        best_local.cost = sub->cost + decoding_cost;
        best_local.next = std::move(sub);
        have_solution = true;
      }
    }

    if (have_solution) {
      return std::make_unique<Round>(std::move(best_local));
    } else {
      return nullptr;
    }
  }

  TruthTable buildUpperFunction(const TruthTable& f,
                                int bn,
                                int fn,
                                int ndecoding_luts,
                                uint32_t shmask,
                                int nshared,
                                const std::vector<int>& bound_to_code,
                                int varcounter)
  {
    TruthTable up;
    up.noutputs = f.noutputs;
    up.vars.reserve(fn + ndecoding_luts + nshared);
    for (int i = 0; i < fn; ++i) {
      up.vars.push_back(f.vars[i]);
    }
    for (int i = 0; i < ndecoding_luts; ++i) {
      up.vars.push_back(varcounter + i);
    }
    for (int i = 0; i < bn; ++i) {
      if (shmask & (1u << i)) {
        up.vars.push_back(f.vars[fn + i]);
      }
    }
    const int up_nvars = (int) up.vars.size();
    const int up_nminterms = 1 << up_nvars;
    up.values.assign((size_t) up.noutputs * up_nminterms, true);
    up.dontcares.assign((size_t) up.noutputs * up_nminterms, true);

    const int fraglen = 1 << fn;
    for (int out = 0; out < f.noutputs; ++out) {
      for (int b = 0; b < (1 << bn); ++b) {
        const int code = bound_to_code[b];
        if (code < 0) {
          continue;
        }
        int shared_part = 0;
        int sh_pos = 0;
        for (int i = 0; i < bn; ++i) {
          if (shmask & (1u << i)) {
            if (b & (1 << i)) {
              shared_part |= 1 << sh_pos;
            }
            sh_pos++;
          }
        }
        for (int free_idx = 0; free_idx < fraglen; ++free_idx) {
          const int up_idx = free_idx | (code << fn)
                             | (shared_part << (fn + ndecoding_luts));
          const int src_idx
              = b * fraglen + free_idx + out * (1 << (int) f.vars.size());
          const int dst_idx = out * up_nminterms + up_idx;
          const bool src_is_dc = f.dontcares[src_idx];
          const bool dst_is_dc = up.dontcares[dst_idx];
          if (dst_is_dc || !src_is_dc) {
            up.values[dst_idx] = f.values[src_idx];
            up.dontcares[dst_idx] = src_is_dc;
          }
        }
      }
    }
    return up;
  }

  long long explore_calls_ = 0;

 private:
  // Estimated arrival per variable ID
  std::vector<NodeArrivals> arrivals_;

  void setArrivals(int var, const NodeArrivals& a)
  {
    if (var >= (int) arrivals_.size()) {
      arrivals_.resize(var + 1);
    }
    arrivals_[var] = a;
  }

  float slackUpperBound(std::vector<int>& vars, int output)
  {
    float slack = std::numeric_limits<float>::infinity();

    if (!timing_) {
      return 0;
    }

    for (int idx : vars) {
      for (auto rf1 : sta::RiseFall::range()) {
        for (auto rf2 : sta::RiseFall::range()) {
          slack = std::min(
              slack, -arrivals_[idx].atTransition(rf1).atExit(output, rf2));
        }
      }
    }

    return slack - dparams_.nand_delay;
  }
};

// Emission: walk the Round chain and build a GateNetwork.
class Emitter
{
 public:
  Emitter(int ninputs, GateNetwork& out) : out_(out)
  {
    out_.ninputs = ninputs;
    out_.nodes.clear();
    out_.outs.clear();
  }

  // Emit this round's decoder cell (if any), recurse into the rest of the
  // chain
  std::vector<std::pair<bool, int>> emit(const Round& r)
  {
    if (r.driver) {
      GateNode node;
      node.driver_port = r.driver;
      node.fanins.resize(r.inputs.size());
      for (size_t j = 0; j < r.inputs.size(); ++j) {
        node.fanins[j] = resolve(r.inputs[j]);
      }
      binding_[r.output] = {false, (int) out_.nodes.size()};
      out_.nodes.push_back(std::move(node));
    }

    std::vector<std::pair<bool, int>> inner;
    if (!r.split.empty()) {
      // Each split child is an independent single-output sub-chain; its lone
      // output feeds the k-th kept output of this round, in order.  Children
      // share `binding_`, so each resolves decoder outputs from rounds above
      // the split and binds its own (their IDs are globally disjoint).
      inner.resize(r.split.size());
      for (size_t k = 0; k < r.split.size(); ++k) {
        inner[k] = emit(*r.split[k]).front();
      }
    } else if (r.next) {
      inner = emit(*r.next);
    }

    std::vector<std::pair<bool, int>> refs(r.out_passthrough.size());
    int kept = 0;
    for (size_t o = 0; o < refs.size(); ++o) {
      refs[o] = r.out_passthrough[o] >= 0 ? resolve(r.out_passthrough[o])
                                          : inner[kept++];
    }
    return refs;
  }

 private:
  std::pair<bool, int> resolve(int vid) const
  {
    if (vid >= 0 && vid < out_.ninputs) {
      return {true, vid};  // primary input
    }
    return binding_.at(vid);  // decoder output emitted earlier in the walk
  }

  GateNetwork& out_;
  std::map<int, std::pair<bool, int>> binding_;  // decoder var ID -> node ref
};

}  // namespace

GateNetwork emitNetworkForSolution(int ninputs, std::unique_ptr<Round> round)
{
  GateNetwork network;
  Emitter emitter(ninputs, network);
  network.outs = emitter.emit(*round);
  return network;
}

bool synthesize(const SynthesisProblem& problem,
                MatchCache& mc,
                const DelayEstimationParameters& dparams,
                utl::Logger* logger,
                GateNetwork& out,
                double budget,
                bool allow_lateral,
                long long* explore_calls,
                float effort)
{
  if (problem.outputs.size() > 2) {
    logger->error(
        utl::SYN,
        67,
        "ACD remapper is limited to up to 2 outputs but problem has {}",
        problem.outputs.size());
  }

  Objective objective;
  objective.min_slacks[0] = -std::numeric_limits<float>::infinity();
  objective.min_slacks[1] = -std::numeric_limits<float>::infinity();
  objective.slack_cost_factor[0] = 0;
  objective.slack_cost_factor[1] = 0;
  for (int i = 0; i < (int) problem.outputs.size() && i < 2; ++i) {
    objective.slack_cost_factor[i] = problem.outputs[i].critical ? -effort : 0;
  }

  Search ex(
      logger, mc, dparams, problem, objective, kMaxBoundVars, allow_lateral);

  std::unique_ptr<Round> root = ex.run(budget);
  if (explore_calls) {
    *explore_calls = ex.exploreCalls();
  }

  if (!root) {
    return false;
  }

  out = emitNetworkForSolution(problem.inputs.size(), std::move(root));
  return true;
}

}  // namespace syn::acd
