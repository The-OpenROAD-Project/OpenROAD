// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// Bitblast pass: converts arithmetic operations and muxes
// to And/Andnot/Or/Xor/Not gates.

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <map>
#include <optional>
#include <set>
#include <utility>
#include <vector>

#include "syn/ir/Bundle.h"
#include "syn/ir/ControlNet.h"
#include "syn/ir/Graph.h"
#include "syn/ir/Instance.h"
#include "syn/ir/Net.h"
#include "syn/synthesis.h"

namespace syn {

using Literal = ControlNet;

struct BitblastedOperation
{
  Graph& g;
  std::set<Net> support;
  std::set<Net> decomposed;
  Bundle replacement;

  // Structural hashing cache: canonical (a, b) → AND(a, b) result.
  // Shared across all BitblastedOperations for global dedup.
  using LitPair = std::pair<Literal, Literal>;
  std::map<LitPair, Literal>& and_cache;

  // Decompose a literal into its AND children, if it is the output
  // of an And/Andnot/Or gate.  `neg` is set to true when the literal
  // represents NOT(AND(left, right)).
  struct AndDec
  {
    Literal left, right;
    bool neg;
  };

  std::optional<AndDec> decompose(Literal lit)
  {
    auto [inst, offset] = g.resolve(lit.net());
    if (auto* a = inst->try_as<And>()) {
      decomposed.insert(lit.net());
      return AndDec{.left = Literal::pos(a->a()[offset]),
                    .right = Literal::pos(a->b()[offset]),
                    .neg = lit.isNegative()};
    }
    if (auto* a = inst->try_as<Andnot>()) {
      decomposed.insert(lit.net());
      return AndDec{.left = Literal::pos(a->a()[offset]),
                    .right = Literal::neg(a->b()[offset]),
                    .neg = lit.isNegative()};
    }
    if (auto* a = inst->try_as<Or>()) {
      // Or(a,b) = NOT(AND(NOT(a), NOT(b)))
      decomposed.insert(lit.net());
      return AndDec{.left = Literal::neg(a->a()[offset]),
                    .right = Literal::neg(a->b()[offset]),
                    .neg = lit.isPositive()};
    }
    return std::nullopt;
  }

  // The AND helper applies local two-level AIG rewriting rules from
  // Brummayer & Biere, MEMICS'06 (Table 2).
  Literal AND(Literal a, Literal b)
  {
    // ---- Level 1 (O1): single-level rules ----

    // Boundedness
    if (a.isAlways(false) || b.isAlways(false)) {
      return Literal::zero();
    }
    // Neutrality
    if (a.isAlways(true)) {
      return b;
    }
    if (b.isAlways(true)) {
      return a;
    }
    // Contradiction
    if (a == !b) {
      return Literal::zero();
    }
    // Idempotence
    if (a == b) {
      return a;
    }

    // ---- Level 2 (O2-O4): two-level rules ----

    auto da = decompose(a);
    auto db = decompose(b);

    // -- Asymmetric: a = AND(p, q), b = leaf --
    if (da && !da->neg) {
      // Rule 5: AND(AND(p,q), b) → FALSE  if p=!b or q=!b
      if (da->left == !b || da->right == !b) {
        return Literal::zero();
      }
      // Rule 9: AND(AND(p,q), b) → a  if p=b or q=b
      if (da->left == b || da->right == b) {
        return a;
      }
    }
    // -- Asymmetric: a = NOT(AND(p, q)), b = leaf --
    if (da && da->neg) {
      // Rule 7: NOT(AND(p,q)) AND b → b  if p=!b or q=!b
      if (da->left == !b || da->right == !b) {
        return b;
      }
      // Rule 11: NOT(AND(p,q)) AND b → AND(!p, b) if q=b
      if (da->right == b) {
        return AND(!da->left, b);
      }
      if (da->left == b) {
        return AND(!da->right, b);
      }
    }
    // -- Asymmetric: b = AND(r, s), a = leaf --
    if (db && !db->neg) {
      if (db->left == !a || db->right == !a) {
        return Literal::zero();
      }
      if (db->left == a || db->right == a) {
        return b;
      }
    }
    // -- Asymmetric: b = NOT(AND(r, s)), a = leaf --
    if (db && db->neg) {
      if (db->left == !a || db->right == !a) {
        return a;
      }
      if (db->right == a) {
        return AND(a, !db->left);
      }
      if (db->left == a) {
        return AND(a, !db->right);
      }
    }

    // -- Symmetric: both children are AND nodes --
    if (da && db) {
      // Rule 6: AND(AND(p,q), AND(r,s)) → FALSE  if any cross-pair
      // complementary
      if (!da->neg && !db->neg) {
        if (da->left == !db->left || da->left == !db->right
            || da->right == !db->left || da->right == !db->right) {
          return Literal::zero();
        }

        // Rules 13-16: Idempotence (shared child)
        if (da->left == db->left || da->right == db->left) {
          return AND(a, db->right);
        }
        if (da->left == db->right || da->right == db->right) {
          return AND(a, db->left);
        }
      }

      // Rule 8: NOT(AND(p,q)) AND AND(r,s) → AND(r,s) if cross-pair
      // complementary
      if (da->neg && !db->neg) {
        if (da->left == !db->left || da->left == !db->right
            || da->right == !db->left || da->right == !db->right) {
          return b;
        }

        // Rule 12: NOT(AND(p,q)) AND AND(r,s) → AND(!p, b) if q=r or q=s
        if (da->right == db->left || da->right == db->right) {
          return AND(!da->left, b);
        }
        if (da->left == db->left || da->left == db->right) {
          return AND(!da->right, b);
        }
      }

      // Symmetric of rule 8/12 (swap a and b)
      if (!da->neg && db->neg) {
        if (db->left == !da->left || db->left == !da->right
            || db->right == !da->left || db->right == !da->right) {
          return a;
        }

        if (db->right == da->left || db->right == da->right) {
          return AND(a, !db->left);
        }
        if (db->left == da->left || db->left == da->right) {
          return AND(a, !db->right);
        }
      }

      // Rule 10: NOT(AND(p,q)) AND NOT(AND(r,s)) → !p  if p=s and q=!r
      if (da->neg && db->neg) {
        if (da->left == db->right && da->right == !db->left) {
          return !da->left;
        }
        if (da->left == db->left && da->right == !db->right) {
          return !da->left;
        }
        if (da->right == db->right && da->left == !db->left) {
          return !da->right;
        }
        if (da->right == db->left && da->left == !db->right) {
          return !da->right;
        }
      }
    }

    // ---- Cache lookup / emit gate ----

    LitPair key = (a < b) ? LitPair{a, b} : LitPair{b, a};
    auto it = and_cache.find(key);
    if (it != and_cache.end()) {
      return it->second;
    }

    Literal result;
    if (a.isPositive() && b.isPositive()) {
      result = Literal::pos(g.add<And>(a.net(), b.net()).asNet());
    } else if (a.isPositive() && b.isNegative()) {
      result = Literal::pos(g.add<Andnot>(a.net(), b.net()).asNet());
    } else if (a.isNegative() && b.isPositive()) {
      result = Literal::pos(g.add<Andnot>(b.net(), a.net()).asNet());
    } else {
      result = Literal::neg(g.add<Or>(a.net(), b.net()).asNet());
    }

    and_cache[key] = result;
    return result;
  };

  Literal OR(Literal a, Literal b) { return !AND(!a, !b); }

  Literal XOR(Literal a, Literal b) { return OR(AND(a, !b), AND(!a, b)); }

  Literal MUX(Literal sel, Literal a, Literal b)
  {
    return OR(AND(sel, a), AND(!sel, b));
  }

  Literal CARRY(Literal a, Literal b, Literal c)
  {
    if (c.isAlways(true)) {
      return OR(a, b);
    }
    if (c.isAlways(false)) {
      return AND(a, b);
    }
    return OR(AND(a, b), AND(c, OR(a, b)));
  }

  // Ripple-carry adder: returns n+1 literals (sum[0..n-1], carry_out).
  std::vector<Literal> RIPPLE_ADDER(const std::vector<Literal>& a,
                                    const std::vector<Literal>& b,
                                    Literal cin)
  {
    uint32_t n = a.size();
    std::vector<Literal> result;
    result.reserve(n + 1);
    Literal carry = cin;
    for (uint32_t i = 0; i < n; i++) {
      Literal ab = XOR(a[i], b[i]);
      result.push_back(XOR(ab, carry));
      carry = OR(AND(a[i], b[i]), AND(carry, ab));
    }
    result.push_back(carry);
    return result;
  }

  // Han-Carlson parallel prefix adder: O(log n) depth.
  // Returns n+1 literals (sum[0..n-1], carry_out).
  std::vector<Literal> ADDER(const std::vector<Literal>& a,
                             const std::vector<Literal>& b,
                             Literal cin)
  {
    uint32_t n = a.size();
    if (n <= 4) {
      return RIPPLE_ADDER(a, b, cin);
    }

    // Bit-level generate and propagate.
    std::vector<Literal> g(n), p(n);
    for (uint32_t i = 0; i < n; i++) {
      p[i] = XOR(a[i], b[i]);
      g[i] = AND(a[i], b[i]);
    }

    // Incorporate carry-in into position 0.
    // G'[0] = G[0] | (P[0] & CI)
    g[0] = OR(g[0], AND(p[0], cin));

    // Compute number of prefix levels.
    int levels = 0;
    for (uint32_t v = n; v > 1; v = (v + 1) / 2) {
      levels++;
    }

    // Stage 1 (Brent-Kung first level): operate on odd indices.
    // For j odd: (g[j], p[j]) = (g[j], p[j]) ∘ (g[j-1], p[j-1])
    for (uint32_t j = 1; j < n; j += 2) {
      Literal new_g = OR(g[j], AND(p[j], g[j - 1]));
      Literal new_p = AND(p[j], p[j - 1]);
      g[j] = new_g;
      p[j] = new_p;
    }

    // Stages 2..levels-1 (Kogge-Stone on odd indices only).
    for (int i = 1; i < levels; i++) {
      uint32_t stride = 1u << i;
      for (int j = (int) n - 1; j >= stride; j--) {
        if (j % 2 == 1) {
          Literal new_g = OR(g[j], AND(p[j], g[j - stride]));
          Literal new_p = AND(p[j], p[j - stride]);
          g[j] = new_g;
          p[j] = new_p;
        }
      }
    }

    // Final stage: fill in even indices from their odd neighbor.
    // For j even, j > 0: (g[j], p[j]) = (g[j], p[j]) ∘ (g[j-1], p[j-1])
    for (uint32_t j = 2; j < n; j += 2) {
      Literal new_g = OR(g[j], AND(p[j], g[j - 1]));
      Literal new_p = AND(p[j], p[j - 1]);
      g[j] = new_g;
      p[j] = new_p;
    }

    // The carries are the group generates: CO[i] = g[i].
    // sum[i] = P_original[i] XOR CO[i-1], with CO[-1] = cin.
    std::vector<Literal> result;
    result.reserve(n + 1);
    for (uint32_t i = 0; i < n; i++) {
      Literal carry_in = (i == 0) ? cin : g[i - 1];
      result.push_back(XOR(XOR(a[i], b[i]), carry_in));
    }
    result.push_back(g[n - 1]);  // carry out
    return result;
  }

  Literal import(Net net)
  {
    Literal lit = Literal::pos(net);
    while (true) {
      auto [inst, offset] = g.resolve(lit.net());
      if (auto buf = inst->try_as<Buffer>()) {
        lit = Literal::withPolarity(buf->a()[offset], lit.isPositive());
      } else if (auto not1 = inst->try_as<Not>()) {
        lit = Literal::withPolarity(not1->a()[offset], !lit.isPositive());
      } else {
        support.insert(lit.net());
        return lit;
      }
    }
  }

  void produce(uint32_t i, Literal lit)
  {
    replacement.mutableNet(i) = lit.emitNet(g);
  }

  BitblastedOperation(Graph& graph,
                      const Instance* inst,
                      std::map<LitPair, Literal>& cache)
      : g(graph), and_cache(cache)
  {
    uint32_t ninputs = 0;
    inst->visit([&](Net _net) { ++ninputs; });
    replacement = Bundle::undef(inst->outputWidth());

    if (auto* adc = inst->try_as<Adc>()) {
      std::vector<Literal> a_bits, b_bits;
      for (uint32_t i = 0; i < adc->a().width(); i++) {
        a_bits.push_back(import(adc->a()[i]));
        b_bits.push_back(import(adc->b()[i]));
      }
      auto sum = ADDER(a_bits, b_bits, import(adc->cin()));
      for (uint32_t i = 0; i < inst->outputWidth(); i++) {
        produce(i, sum[i]);
      }
    } else if (auto* mux = inst->try_as<Mux>()) {
      auto sel = import(mux->sel());
      for (uint32_t i = 0; i < mux->a().width(); i++) {
        auto a = import(mux->a()[i]);
        auto b = import(mux->b()[i]);
        produce(i, MUX(sel, a, b));
      }
    } else if (auto* eq = inst->try_as<Eq>()) {
      std::vector<ControlNet> entries;
      entries.reserve(eq->a().width());
      for (uint32_t i = 0; i < eq->a().width(); i++) {
        entries.push_back(XOR(import(eq->a()[i]), import(eq->b()[i])));
      }

      while (entries.size() > 1) {
        std::vector<ControlNet> next;
        for (uint32_t i = 0; i < entries.size(); i += 2) {
          if (entries.size() > i + 1) {
            next.push_back(OR(entries[i], entries[i + 1]));
          } else {
            next.push_back(entries[i]);
          }
        }
        std::swap(entries, next);
      }
      assert(!entries.empty());
      produce(0, !entries[0]);
    } else if (auto* ult = inst->try_as<ULt>()) {
      // a < b  iff  borrow out of a - b  iff  NOT carry out of a + ~b + 1.
      uint32_t n = ult->a().width();
      std::vector<Literal> a_bits(n), nb_bits(n);
      for (uint32_t i = 0; i < n; i++) {
        a_bits[i] = import(ult->a()[i]);
        nb_bits[i] = !import(ult->b()[i]);
      }
      auto sum = ADDER(a_bits, nb_bits, Literal::one());
      produce(0, !sum[n]);  // NOT carry-out = borrow
    } else if (auto* slt = inst->try_as<SLt>()) {
      // Signed: a < b iff sign bit of (a - b) with overflow correction.
      // sign(a-b) XOR overflow = (sum[n-1]) XOR (carry[n-1] XOR carry[n-2])
      // Simpler: use ADDER for a + ~b + 1, then result = carry XOR sign_diff.
      uint32_t n = slt->a().width();
      std::vector<Literal> a_bits(n), nb_bits(n);
      for (uint32_t i = 0; i < n; i++) {
        a_bits[i] = import(slt->a()[i]);
        nb_bits[i] = !import(slt->b()[i]);
      }
      auto sum = ADDER(a_bits, nb_bits, Literal::one());
      // slt = carry XOR (NOT (a_msb XOR b_msb))
      // which equals carry XOR (a_msb XNOR b_msb)
      Literal a_msb = import(slt->a()[n - 1]);
      Literal b_msb = import(slt->b()[n - 1]);
      produce(0, XOR(sum[n], !XOR(a_msb, b_msb)));
    } else if (auto* xor_ = inst->try_as<Xor>()) {
      for (uint32_t i = 0; i < xor_->a().width(); i++) {
        Literal a = import(xor_->a()[i]);
        Literal b = import(xor_->b()[i]);
        produce(i, XOR(a, b));
      }
    } else if (auto* op = inst->try_as<Shl>()) {
      // Barrel shifter: for each bit k of b, conditionally shift left
      // by 2^k * stride.
      uint32_t n = op->outputWidth();
      std::vector<Literal> cur(n);
      for (uint32_t i = 0; i < n; i++) {
        cur[i] = (i < op->a().width()) ? import(op->a()[i]) : Literal::zero();
      }
      for (uint32_t k = 0; k < op->b().width(); k++) {
        Literal bk = import(op->b()[k]);
        uint32_t shift = (1u << k) * op->stride();
        if (shift >= n) {
          // If this bit is set, result is all zeros.
          for (uint32_t i = 0; i < n; i++) {
            cur[i] = AND(cur[i], !bk);
          }
          break;
        }
        std::vector<Literal> next(n);
        for (uint32_t i = 0; i < n; i++) {
          Literal shifted = (i >= shift) ? cur[i - shift] : Literal::zero();
          next[i] = MUX(bk, shifted, cur[i]);
        }
        cur = std::move(next);
      }
      for (uint32_t i = 0; i < n; i++) {
        produce(i, cur[i]);
      }

    } else if (auto* op = inst->try_as<UShr>()) {
      uint32_t n = op->outputWidth();
      std::vector<Literal> cur(n);
      for (uint32_t i = 0; i < n; i++) {
        cur[i] = (i < op->a().width()) ? import(op->a()[i]) : Literal::zero();
      }
      for (uint32_t k = 0; k < op->b().width(); k++) {
        Literal bk = import(op->b()[k]);
        uint32_t shift = (1u << k) * op->stride();
        if (shift >= n) {
          for (uint32_t i = 0; i < n; i++) {
            cur[i] = AND(cur[i], !bk);
          }
          break;
        }
        std::vector<Literal> next(n);
        for (uint32_t i = 0; i < n; i++) {
          Literal shifted = (i + shift < n) ? cur[i + shift] : Literal::zero();
          next[i] = MUX(bk, shifted, cur[i]);
        }
        cur = std::move(next);
      }
      for (uint32_t i = 0; i < n; i++) {
        produce(i, cur[i]);
      }

    } else if (auto* op = inst->try_as<SShr>()) {
      uint32_t n = op->outputWidth();
      std::vector<Literal> cur(n);
      for (uint32_t i = 0; i < n; i++) {
        cur[i] = (i < op->a().width()) ? import(op->a()[i]) : Literal::zero();
      }
      Literal sign = import(op->a()[op->a().width() - 1]);
      for (uint32_t k = 0; k < op->b().width(); k++) {
        Literal bk = import(op->b()[k]);
        uint32_t shift = (1u << k) * op->stride();
        if (shift >= n) {
          // All bits become sign.
          for (uint32_t i = 0; i < n; i++) {
            cur[i] = MUX(bk, sign, cur[i]);
          }
          break;
        }
        std::vector<Literal> next(n);
        for (uint32_t i = 0; i < n; i++) {
          Literal shifted = (i + shift < n) ? cur[i + shift] : sign;
          next[i] = MUX(bk, shifted, cur[i]);
        }
        cur = std::move(next);
      }
      for (uint32_t i = 0; i < n; i++) {
        produce(i, cur[i]);
      }

    } else if (auto* op = inst->try_as<XShr>()) {
      // Like UShr but fill with undef (which in the AIG is zero —
      // the SAT model handles X separately).
      uint32_t n = op->outputWidth();
      std::vector<Literal> cur(n);
      for (uint32_t i = 0; i < n; i++) {
        cur[i] = (i < op->a().width()) ? import(op->a()[i]) : Literal::zero();
      }
      for (uint32_t k = 0; k < op->b().width(); k++) {
        Literal bk = import(op->b()[k]);
        uint32_t shift = (1u << k) * op->stride();
        if (shift >= n) {
          for (uint32_t i = 0; i < n; i++) {
            cur[i] = AND(cur[i], !bk);
          }
          break;
        }
        std::vector<Literal> next(n);
        for (uint32_t i = 0; i < n; i++) {
          Literal shifted = (i + shift < n) ? cur[i + shift] : Literal::zero();
          next[i] = MUX(bk, shifted, cur[i]);
        }
        cur = std::move(next);
      }
      for (uint32_t i = 0; i < n; i++) {
        produce(i, cur[i]);
      }

    } else if (auto* op = inst->try_as<Mul>()) {
      uint32_t n = op->outputWidth();
      uint32_t a_len = op->a().width();
      uint32_t b_len = op->b().width();

      while (a_len >= 2 && op->a()[a_len - 1] == op->a()[a_len - 2]) {
        a_len--;
      }
      while (b_len >= 2 && op->b()[b_len - 1] == op->b()[b_len - 2]) {
        b_len--;
      }

      std::vector<Literal> a_bits(a_len), b_bits(b_len);
      for (uint32_t i = 0; i < a_len; i++) {
        a_bits[i] = import(op->a()[i]);
      }
      for (uint32_t i = 0; i < b_len; i++) {
        b_bits[i] = import(op->b()[i]);
      }

      // Phase 1: Collect partial products into columns.
      std::vector<std::vector<Literal>> columns(n);

      {
        // The constant correction associated with each column
        std::vector<int> corrections(n);

        // Regular products (non-sign rows/columns)
        for (uint32_t k = 0; k < b_len && k < n; k++) {
          for (uint32_t j = 0; j < a_len && j + k < n; j++) {
            Literal product = AND(a_bits[j], b_bits[k]);

            if ((k == b_len - 1 || j == a_len - 1)
                && (k != b_len - 1 || j != a_len - 1)) {
              product = !product;
              corrections[j + k]--;
            }

            if (product.isAlways(false)) {
              // can be ignored
            } else if (product.isAlways(true)) {
              corrections[j + k]++;
            } else {
              columns[j + k].push_back(product);
            }
          }
        }

        // Now transfer constant corrections to columns
        int pending = 0;
        for (uint32_t i = 0; i < n; i++) {
          pending += corrections[i];

          if (pending & 1) {
            columns[i].push_back(ControlNet::one());
          }

          if (pending >= 0) {
            pending /= 2;
          } else {
            pending = (pending / 2) - (pending & 1);
          }
        }
      }

      // Phase 2: Wallace tree reduction using full adders (3→2).
      for (;;) {
        size_t max_height = 0;
        for (uint32_t i = 0; i < n; i++) {
          max_height = std::max(max_height, columns[i].size());
        }
        if (max_height <= 2) {
          break;
        }

        std::vector<std::vector<Literal>> next(n);
        for (uint32_t i = 0; i < n; i++) {
          auto& col = columns[i];
          while (col.size() >= 3) {
            Literal x = col.back();
            col.pop_back();
            Literal y = col.back();
            col.pop_back();
            Literal z = col.back();
            col.pop_back();
            Literal xy = XOR(x, y);
            next[i].push_back(XOR(xy, z));
            if (i + 1 < n) {
              next[i + 1].push_back(OR(AND(x, y), AND(xy, z)));
            }
          }
          for (auto& lit : col) {
            next[i].push_back(lit);
          }
        }
        columns = std::move(next);
      }

      // Phase 3: Final addition via ADDER.
      std::vector<Literal> row_a(n, Literal::zero());
      std::vector<Literal> row_b(n, Literal::zero());
      for (uint32_t i = 0; i < n; i++) {
        if (!columns[i].empty()) {
          row_a[i] = columns[i][0];
        }
        if (columns[i].size() >= 2) {
          row_b[i] = columns[i][1];
        }
      }
      auto result = ADDER(row_a, row_b, Literal::zero());
      for (uint32_t i = 0; i < n; i++) {
        produce(i, result[i]);
      }

    } else if (auto* and_ = inst->try_as<And>()) {
      for (uint32_t i = 0; i < and_->a().width(); i++) {
        Literal a = import(and_->a()[i]);
        Literal b = import(and_->b()[i]);
        produce(i, AND(a, b));
      }
    } else if (auto* or_ = inst->try_as<Or>()) {
      for (uint32_t i = 0; i < or_->a().width(); i++) {
        Literal a = import(or_->a()[i]);
        Literal b = import(or_->b()[i]);
        produce(i, OR(a, b));
      }
    } else if (auto* andnot_ = inst->try_as<Andnot>()) {
      for (uint32_t i = 0; i < andnot_->a().width(); i++) {
        Literal a = import(andnot_->a()[i]);
        Literal b = import(andnot_->b()[i]);
        produce(i, AND(a, !b));
      }
    } else {
      std::abort();
    }
  }

  void commit(const Instance* inst)
  {
    std::set<Net> fixedUpSupport;

    for (auto net : support) {
      if (!decomposed.contains(net)) {
        fixedUpSupport.insert(net);
      }
    }
    for (auto net : decomposed) {
      auto [inst, offset] = g.resolve(net);
      inst->visitSlice(offset, [&](Net fanin) {
        if (!decomposed.contains(fanin)) {
          fixedUpSupport.insert(fanin);
        }
      });
    }

    g.replace(g.output(inst),
              BundleView(replacement),
              Graph::Equivalence::TwoValued,
              Bundle::fromVec(
                  std::vector(fixedUpSupport.begin(), fixedUpSupport.end())));
  }
};

void bitblast(Graph& g, bool blast_arith)
{
  g.normalize();

  std::map<BitblastedOperation::LitPair, Literal> cache;
  g.forEachInstance([&](const Instance* inst) {
    if ((inst->is<Adc>()) || inst->is<Mux>() || inst->is<Eq>()
        || inst->is<ULt>() || inst->is<SLt>() || inst->is<Xor>()
        || inst->is<Shl>() || inst->is<UShr>() || inst->is<SShr>()
        || inst->is<XShr>() || (inst->is<Mul>() && blast_arith)
        || inst->is<And>() || inst->is<Or>() || inst->is<Andnot>()) {
      BitblastedOperation(g, inst, cache).commit(inst);
    }
  });
}

}  // namespace syn
