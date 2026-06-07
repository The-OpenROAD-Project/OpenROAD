// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include "constant_fold.h"

#include <cassert>
#include <cstdint>
#include <optional>
#include <utility>
#include <vector>

#include "syn/ir/Bundle.h"
#include "syn/ir/Const.h"
#include "syn/ir/ControlNet.h"
#include "syn/ir/Graph.h"
#include "syn/ir/Instance.h"
#include "syn/ir/Net.h"

namespace syn {

static Net stripBuffers(const Graph& g, Net net)
{
  while (true) {
    auto [inst, offset] = g.resolve(net);
    if (!inst->is<Buffer>()) {
      break;
    }
    net = inst->as<Buffer>()->a()[offset];
  }
  return net;
}

// Build replacement net from a Trit.
static Net fromTrit(Trit t)
{
  switch (t) {
    case Trit::Zero:
      return Net::zero();
    case Trit::One:
      return Net::one();
    case Trit::Undef:
      return Net::undef();
  }
  return Net::undef();
}

// Lookup all inputs through buffers, return as Const if all are const.
static std::optional<Const> lookupConst(const Graph& g, BundleView bv)
{
  std::vector<Trit> bits;
  bits.reserve(bv.width());
  for (uint32_t i = 0; i < bv.width(); i++) {
    Net n = stripBuffers(g, bv[i]);
    if (!n.isConst()) {
      return std::nullopt;
    }
    bits.push_back(n.constTrit());
  }
  return Const::from(std::move(bits));
}

// Return the integer value of a Const, or nullopt if it contains undef.
static std::optional<uint64_t> constToUint(const Const& c)
{
  if (c.hasUndef()) {
    return std::nullopt;
  }
  uint64_t val = 0;
  for (int i = (int) c.width() - 1; i >= 0; i--) {
    val <<= 1;
    if (c[i] == Trit::One) {
      val |= 1;
    }
  }
  return val;
}

// If val is a power of 2, return the exponent. Otherwise -1.
static int log2exact(uint64_t val)
{
  if (val == 0 || (val & (val - 1)) != 0) {
    return -1;
  }
  return __builtin_ctzll(val);
}

static void foldCombinationals(Graph& g)
{
  std::vector<Net> new_output;
  g.forEachInstance([&](const Instance* inst) {
    uint32_t width = inst->outputWidth();
    new_output.resize(width, Net::sentinel());
    BundleView out = g.output(inst);
    bool do_replace = false;

    if (auto* op = inst->try_as<Not>()) {
      for (uint32_t i = 0; i < width; i++) {
        Net abit = stripBuffers(g, op->a()[i]);
        if (abit.isConst()) {
          new_output[i] = fromTrit(!abit.constTrit());
          do_replace = true;
        } else {
          new_output[i] = out[i];
        }
      }

    } else if (auto* op = inst->try_as<And>()) {
      for (uint32_t i = 0; i < width; i++) {
        Net ai = stripBuffers(g, op->a()[i]);
        Net bi = stripBuffers(g, op->b()[i]);
        Net r = out[i];
        if (ai == Net::zero() || bi == Net::zero()) {
          r = Net::zero();
        } else if (ai == Net::one()) {
          r = bi;
        } else if (bi == Net::one()) {
          r = ai;
        } else if (ai == bi) {
          r = ai;
        } else if (ai.isConst() && bi.isConst()) {
          r = fromTrit(ai.constTrit() & bi.constTrit());
        }
        if (r != out[i]) {
          do_replace = true;
        }
        new_output[i] = r;
      }

    } else if (auto* op = inst->try_as<Or>()) {
      for (uint32_t i = 0; i < width; i++) {
        Net ai = stripBuffers(g, op->a()[i]);
        Net bi = stripBuffers(g, op->b()[i]);
        Net r = out[i];
        if (ai == Net::one() || bi == Net::one()) {
          r = Net::one();
        } else if (ai == Net::zero()) {
          r = bi;
        } else if (bi == Net::zero()) {
          r = ai;
        } else if (ai == bi) {
          r = ai;
        } else if (ai.isConst() && bi.isConst()) {
          r = fromTrit(ai.constTrit() | bi.constTrit());
        }
        if (r != out[i]) {
          do_replace = true;
        }
        new_output[i] = r;
      }

    } else if (auto* op = inst->try_as<Xor>()) {
      for (uint32_t i = 0; i < width; i++) {
        Net ai = stripBuffers(g, op->a()[i]);
        Net bi = stripBuffers(g, op->b()[i]);
        Net r = out[i];
        if (ai == Net::zero()) {
          r = bi;
        } else if (bi == Net::zero()) {
          r = ai;
        } else if (ai == bi) {
          r = Net::zero();
        } else if (ai.isConst() && bi.isConst()) {
          r = fromTrit(ai.constTrit() ^ bi.constTrit());
        }
        if (r != out[i]) {
          do_replace = true;
        }
        new_output[i] = r;
      }

    } else if (auto* op = inst->try_as<Andnot>()) {
      for (uint32_t i = 0; i < width; i++) {
        Net ai = stripBuffers(g, op->a()[i]);
        Net bi = stripBuffers(g, op->b()[i]);
        Net r = out[i];
        if (ai == Net::zero() || bi == Net::one()) {
          r = Net::zero();
        } else if (bi == Net::zero()) {
          r = ai;
        } else if (ai.isConst() && bi.isConst()) {
          r = fromTrit(ai.constTrit() & !bi.constTrit());
        }
        if (r != out[i]) {
          do_replace = true;
        }
        new_output[i] = r;
      }

    } else if (auto* op = inst->try_as<Mux>()) {
      Net sel = stripBuffers(g, op->sel());
      for (uint32_t i = 0; i < width; i++) {
        Net ai = stripBuffers(g, op->a()[i]);
        Net bi = stripBuffers(g, op->b()[i]);
        Net r = out[i];
        if (sel == Net::one()) {
          r = ai;
        } else if (sel == Net::zero()) {
          r = bi;
        } else if (ai == bi) {
          r = ai;
        } else if (sel.isConst() && ai.isConst() && bi.isConst()) {
          r = fromTrit(mux(sel.constTrit(), ai.constTrit(), bi.constTrit()));
        } else if (ai == Net::undef()) {
          r = bi;
        } else if (bi == Net::undef()) {
          r = ai;
        } else if (ai == Net::one() && bi == Net::zero()) {
          r = sel;
        }
        if (r != out[i]) {
          do_replace = true;
        }
        new_output[i] = r;
      }

    } else if (auto* op = inst->try_as<Adc>()) {
      auto ac = lookupConst(g, op->a());
      auto bc = lookupConst(g, op->b());
      Net cin = stripBuffers(g, op->cin());
      if (ac && bc && cin.isConst()) {
        Const result = ac->adc(*bc, cin.constTrit());
        Bundle repl = Bundle::fromConst(result);
        for (uint32_t i = 0; i < width; i++) {
          new_output[i] = repl[i];
        }
        do_replace = true;
      }

    } else if (auto* op = inst->try_as<Eq>()) {
      auto ac = lookupConst(g, op->a());
      auto bc = lookupConst(g, op->b());
      if (ac && bc) {
        new_output[0] = fromTrit(ac->eq(*bc));
        do_replace = true;
      }

    } else if (auto* op = inst->try_as<ULt>()) {
      auto ac = lookupConst(g, op->a());
      auto bc = lookupConst(g, op->b());
      if (ac && bc) {
        new_output[0] = fromTrit(ac->ult(*bc));
        do_replace = true;
      }

    } else if (auto* op = inst->try_as<SLt>()) {
      auto ac = lookupConst(g, op->a());
      auto bc = lookupConst(g, op->b());
      if (ac && bc) {
        new_output[0] = fromTrit(ac->slt(*bc));
        do_replace = true;
      }

    } else if (auto* op = inst->try_as<Shl>()) {
      auto ac = lookupConst(g, op->a());
      auto bc = lookupConst(g, op->b());
      if (ac && bc) {
        Bundle repl = Bundle::fromConst(ac->shl(*bc, op->stride()));
        for (uint32_t i = 0; i < width; i++) {
          new_output[i] = repl[i];
        }
        do_replace = true;
      } else if (bc) {
        // Constant shift amount: wire bits directly.
        auto amt = constToUint(*bc);
        if (amt) {
          uint64_t shift = *amt * op->stride();
          for (uint32_t i = 0; i < width; i++) {
            if (i >= shift && (i - shift) < op->a().width()) {
              new_output[i] = stripBuffers(g, op->a()[i - shift]);
            } else {
              new_output[i] = Net::zero();
            }
          }
          do_replace = true;
        }
      }

    } else if (auto* op = inst->try_as<UShr>()) {
      auto ac = lookupConst(g, op->a());
      auto bc = lookupConst(g, op->b());
      if (ac && bc) {
        Bundle repl = Bundle::fromConst(ac->ushr(*bc, op->stride()));
        for (uint32_t i = 0; i < width; i++) {
          new_output[i] = repl[i];
        }
        do_replace = true;
      } else if (bc) {
        auto amt = constToUint(*bc);
        if (amt) {
          uint64_t shift = *amt * op->stride();
          for (uint32_t i = 0; i < width; i++) {
            if ((i + shift) < op->a().width()) {
              new_output[i] = stripBuffers(g, op->a()[i + shift]);
            } else {
              new_output[i] = Net::zero();
            }
          }
          do_replace = true;
        }
      }

    } else if (auto* op = inst->try_as<SShr>()) {
      auto ac = lookupConst(g, op->a());
      auto bc = lookupConst(g, op->b());
      if (ac && bc) {
        Bundle repl = Bundle::fromConst(ac->sshr(*bc, op->stride()));
        for (uint32_t i = 0; i < width; i++) {
          new_output[i] = repl[i];
        }
        do_replace = true;
      } else if (bc) {
        auto amt = constToUint(*bc);
        if (amt) {
          uint64_t shift = *amt * op->stride();
          // Sign-extend with MSB of a.
          Net sign = stripBuffers(g, op->a()[op->a().width() - 1]);
          for (uint32_t i = 0; i < width; i++) {
            if ((i + shift) < op->a().width()) {
              new_output[i] = stripBuffers(g, op->a()[i + shift]);
            } else {
              new_output[i] = sign;
            }
          }
          do_replace = true;
        }
      }

    } else if (auto* op = inst->try_as<XShr>()) {
      auto ac = lookupConst(g, op->a());
      auto bc = lookupConst(g, op->b());
      if (ac && bc) {
        Bundle repl = Bundle::fromConst(ac->xshr(*bc, op->stride()));
        for (uint32_t i = 0; i < width; i++) {
          new_output[i] = repl[i];
        }
        do_replace = true;
      } else if (bc) {
        auto amt = constToUint(*bc);
        if (amt) {
          uint64_t shift = *amt * op->stride();
          for (uint32_t i = 0; i < width; i++) {
            if ((i + shift) < op->a().width()) {
              new_output[i] = stripBuffers(g, op->a()[i + shift]);
            } else {
              new_output[i] = Net::undef();
            }
          }
          do_replace = true;
        }
      }

    } else if (auto* op = inst->try_as<Mul>()) {
      auto ac = lookupConst(g, op->a());
      auto bc = lookupConst(g, op->b());
      if (ac && bc) {
        Bundle repl = Bundle::fromConst(ac->mul(*bc));
        for (uint32_t i = 0; i < width; i++) {
          new_output[i] = repl[i];
        }
        do_replace = true;
      } else if (bc) {
        // Multiply by power of 2: left shift.
        auto val = constToUint(*bc);
        if (val) {
          int shift = log2exact(*val);
          if (shift >= 0) {
            for (uint32_t i = 0; i < width; i++) {
              if (i >= (uint32_t) shift && (i - shift) < op->a().width()) {
                new_output[i] = stripBuffers(g, op->a()[i - shift]);
              } else {
                new_output[i] = Net::zero();
              }
            }
            do_replace = true;
          }
        }
      }

    } else if (auto* op = inst->try_as<UDiv>()) {
      auto ac = lookupConst(g, op->a());
      auto bc = lookupConst(g, op->b());
      if (ac && bc) {
        Bundle repl = Bundle::fromConst(ac->udiv(*bc));
        for (uint32_t i = 0; i < width; i++) {
          new_output[i] = repl[i];
        }
        do_replace = true;
      } else if (bc) {
        // Division by power of 2: right shift.
        int shift = -1;
        for (uint32_t i = 0; i < bc->width(); i++) {
          if ((*bc)[i] == Trit::One) {
            shift = (shift == -1) ? (int) i : -2;
          }
        }
        if (shift >= 0) {
          for (uint32_t i = 0; i < width; i++) {
            if (i + shift < op->a().width()) {
              new_output[i] = stripBuffers(g, op->a()[i + shift]);
            } else {
              new_output[i] = Net::zero();
            }
          }
          do_replace = true;
        }
      }

    } else if (auto* op = inst->try_as<UMod>()) {
      auto ac = lookupConst(g, op->a());
      auto bc = lookupConst(g, op->b());
      if (ac && bc) {
        Bundle repl = Bundle::fromConst(ac->umod(*bc));
        for (uint32_t i = 0; i < width; i++) {
          new_output[i] = repl[i];
        }
        do_replace = true;
      } else if (bc) {
        // Modulo by power of 2: mask lower bits.
        int shift = -1;
        for (uint32_t i = 0; i < bc->width(); i++) {
          if ((*bc)[i] == Trit::One) {
            shift = (shift == -1) ? (int) i : -2;
          }
        }
        if (shift >= 0) {
          for (uint32_t i = 0; i < width; i++) {
            if (i < shift) {
              new_output[i] = stripBuffers(g, op->a()[i]);
            } else {
              new_output[i] = Net::zero();
            }
          }
          do_replace = true;
        }
      }
    }

    if (do_replace) {
      // Chase all instance inputs through buffers so the support
      // matches the looked-up nets used in the replacement.
      std::vector<Net> support;
      inst->visit([&](Net n) { support.push_back(stripBuffers(g, n)); });
      g.replace(out,
                Bundle::fromVec(new_output),
                Graph::Equivalence::Refinement,
                Bundle::fromVec(std::move(support)));
    }
  });
}

static BundleView sliceDff(Graph& g,
                           const Dff* original,
                           uint32_t base,
                           uint32_t width)
{
  assert(width > 0);

  return g.add<Dff>(original->data().slice(base, width),
                    original->clock(),
                    original->clear(),
                    original->reset(),
                    original->enable(),
                    original->initValue().slice(base, width),
                    original->resetValue().slice(base, width),
                    original->clearValue().slice(base, width));
}

static bool foldSequentials(Graph& g)
{
  // --- Flop optimization: replace DFF bits with constants ---
  //
  // A DFF bit can be replaced with a constant when its D input is
  // that same constant under all reachable conditions:
  //
  //   (1) D is constant C, AND the clear value (if clear is active)
  //       is also C, AND the reset value (if reset is active) is also C.
  //       Then Q = C always.
  //
  //   (2) D equals Q (self-feedback).  The flop never changes from its
  //       initial/clear/reset value.  If that value is a known constant
  //       C, then Q = C.
  //
  bool changed = false;
  auto absorbNots = [&](ControlNet cn) -> ControlNet {
    while (true) {
      Net n = stripBuffers(g, cn.net());
      if (n.isConst()) {
        return ControlNet::withPolarity(n, cn.isPositive());
      }
      auto [drv, off] = g.resolve(n);
      auto* not_op = drv->try_as<Not>();
      if (!not_op) {
        return ControlNet::withPolarity(n, cn.isPositive());
      }
      cn = ControlNet::withPolarity(not_op->a()[off], !cn.isPositive());
    }
  };

  g.forEachInstance([&](const Instance* inst) {
    auto* dff = inst->try_as<Dff>();
    if (!dff) {
      return;
    }

    // Fold any inverter on the control nets into the ControlNet
    // polarity.  Sound on every control: clk-edge polarity flips with
    // a clock inverter, and active-level polarity flips on
    // clear/reset/enable.
    auto* mut = const_cast<Dff*>(dff);
    mut->setClock(absorbNots(dff->clock()));
    mut->setClear(absorbNots(dff->clear()));
    mut->setReset(absorbNots(dff->reset()));
    mut->setEnable(absorbNots(dff->enable()));

    BundleView out = g.output(inst);
    uint32_t width = out.width();
    bool any_replaced = false;
    std::vector<Net> new_out;
    new_out.reserve(width);

    for (uint32_t i = 0; i < width; i++) {
      new_out.push_back(out[i]);
      Net d = stripBuffers(g, dff->data()[i]);

      // Check case (2): D == Q (self-feedback).
      if (d == out[i]) {
        // Flop holds its initial value.  Check if init/clear/reset
        // values give us a known constant.
        std::optional<Trit> value = dff->initValue()[i];

        if (!dff->clear().isAlways(false)) {
          value = refine(value, dff->clearValue()[i]);
        }
        if (!dff->reset().isAlways(false)) {
          value = refine(value, dff->resetValue()[i]);
        }

        if (value) {
          new_out[i] = *value;
          any_replaced = true;
          continue;
        }
      }

      // Check case (1): D is constant
      if (d.isConst()) {
        std::optional<Trit> value = refine(dff->initValue()[i], d.constTrit());

        if (!dff->clear().isAlways(false)) {
          value = refine(value, dff->clearValue()[i]);
        }
        if (!dff->reset().isAlways(false)) {
          value = refine(value, dff->resetValue()[i]);
        }

        if (value) {
          new_out[i] = *value;
          any_replaced = true;
          continue;
        }
      }
    }

    if (any_replaced) {
      std::vector<Net> support;
      inst->visit([&](Net n) {
        support.push_back(absorbNots(ControlNet::pos(n)).net());
      });

      for (uint32_t i = 0; i < out.width(); i++) {
        if (new_out[i] == out[i]) {
          uint32_t slice_width;
          for (slice_width = 1; i + slice_width < out.width(); slice_width++) {
            if (new_out[i + slice_width] != out[i + slice_width]) {
              break;
            }
          }
          BundleView new_out_slice = sliceDff(g, dff, i, slice_width);
          for (uint32_t k = 0; k < slice_width; k++) {
            new_out[i + k] = new_out_slice[k];
          }
          i += slice_width - 1;
        }
      }

      g.replace(out,
                Bundle::fromVec(new_out),
                Graph::Equivalence::Refinement,
                Bundle::fromVec(std::move(support)));
      changed = true;
      g.removeInstance(
          const_cast<Instance*>(static_cast<const Instance*>(dff)));
    }
  });

  return changed;
}

void constantFold(Graph& g)
{
  for (;;) {
    g.normalize();
    foldCombinationals(g);
    if (!foldSequentials(g)) {
      // We have reached fixed point and can stop
      break;
    }
  }
}

}  // namespace syn
