// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

//
// syn IR backend for slang-elab
//

#include "backend_builder.h"

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "ir.h"
#include "log_stubs.h"
#include "slang/ast/expressions/Operator.h"
#include "slang_frontend.h"
#include "syn/ir/Bundle.h"
#include "syn/ir/Const.h"
#include "syn/ir/ControlNet.h"
#include "syn/ir/Instance.h"
#include "syn/ir/Net.h"

namespace slang_frontend {

BackendGraphBuilder::BackendGraphBuilder()
{
  graph_ = std::make_unique<syn::Graph>();
}

std::unique_ptr<BackendGraphBuilder> BackendGraphBuilder::start_new_graph(
    std::string_view)
{
  return std::make_unique<BackendGraphBuilder>();
}

void BackendGraphBuilder::finalize()
{
}

std::string BackendGraphBuilder::new_id(const std::string& base)
{
  if (base.empty()) {
    return std::string("$") + std::to_string(next_id++);
  }
  return std::string("$") + base + "$" + std::to_string(next_id++);
}

syn::Bundle BackendGraphBuilder::toBundle(const ir::Value& v)
{
  return v.to_bundle();
}

ir::Value BackendGraphBuilder::fromBundle(const syn::Bundle& b)
{
  return ir::Value(b);
}

// ============================================================
// Unary operators
// ============================================================

static ir::Value constResult(const syn::Const& c)
{
  return ir::Value(syn::Bundle::fromConst(c));
}

static syn::Const extendConst(const syn::Const& c, bool is_signed, uint64_t w)
{
  if (c.width() >= w) {
    return c.slice(0, w);
  }
  uint32_t pad = static_cast<uint32_t>(w) - c.width();
  syn::Trit fill = is_signed && !c.empty() ? c.msb() : syn::Trit::Zero;
  // NOLINTBEGIN(readability-avoid-nested-conditional-operator)
  syn::Const padding = (fill == syn::Trit::One)     ? syn::Const::ones(pad)
                       : (fill == syn::Trit::Undef) ? syn::Const::undef(pad)
                                                    : syn::Const::zero(pad);
  // NOLINTEND(readability-avoid-nested-conditional-operator)
  return c.concat(padding);
}

ir::Value BackendGraphBuilder::Unop(ast::UnaryOperator op,
                                    ir::Value a,
                                    bool a_signed,
                                    uint64_t y_width)
{
  if (y_width == 0) {
    return ir::Value();
  }

  syn::Bundle ab = toBundle(a);

  // Constant folding: if input is fully constant, compute at compile time.
  if (ab.isConst()) {
    syn::Const ac = ab.toConst();
    syn::Const ext = extendConst(ac, a_signed, y_width);
    syn::Const result;

    switch (op) {
      case ast::UnaryOperator::Plus:
        return constResult(ext);
      case ast::UnaryOperator::Minus:
        return constResult(ext.not_()
                               .adc(syn::Const::zero(y_width), syn::Trit::One)
                               .slice(0, y_width));
      case ast::UnaryOperator::BitwiseNot:
        return constResult(ext.not_());
      case ast::UnaryOperator::BitwiseAnd:
      case ast::UnaryOperator::BitwiseOr:
      case ast::UnaryOperator::BitwiseXor:
      case ast::UnaryOperator::BitwiseNand:
      case ast::UnaryOperator::BitwiseNor:
      case ast::UnaryOperator::BitwiseXnor: {
        if (ac.width() == 0) {
          return constResult(syn::Const::undef(y_width));
        }
        syn::Trit r = ac[0];
        for (uint32_t i = 1; i < ac.width(); i++) {
          switch (op) {
            case ast::UnaryOperator::BitwiseAnd:
            case ast::UnaryOperator::BitwiseNand:
              r = r & ac[i];
              break;
            case ast::UnaryOperator::BitwiseOr:
            case ast::UnaryOperator::BitwiseNor:
              r = r | ac[i];
              break;
            default:
              r = r ^ ac[i];
              break;
          }
        }
        if (op == ast::UnaryOperator::BitwiseNand
            || op == ast::UnaryOperator::BitwiseNor
            || op == ast::UnaryOperator::BitwiseXnor) {
          r = !r;
        }
        return constResult(extendConst(syn::Const::from({r}), false, y_width));
      }
      case ast::UnaryOperator::LogicalNot: {
        if (ac.width() == 0) {
          return constResult(
              extendConst(syn::Const::from({syn::Trit::One}), false, y_width));
        }
        syn::Trit any = ac[0];
        for (uint32_t i = 1; i < ac.width(); i++) {
          any = any | ac[i];
        }
        return constResult(
            extendConst(syn::Const::from({!any}), false, y_width));
      }
      default:
        break;
    }
  }

  auto& g = graph();

  switch (op) {
    case ast::UnaryOperator::Plus:
      // Identity; extend if needed
      if (y_width > a.size()) {
        if (a_signed) {
          return fromBundle(ab.signExtend(y_width));
        }
        return fromBundle(ab.zeroExtend(y_width));
      }
      return a;

    case ast::UnaryOperator::Minus: {
      if (y_width == 0) {
        return ir::Value();
      }
      // Two's complement negate: ~a + 1
      // Extend first, then negate
      syn::Bundle ext
          = a_signed ? ab.signExtend(y_width) : ab.zeroExtend(y_width);
      syn::Bundle inv = g.add<syn::Not>(ext);
      return fromBundle(
          g.add<syn::Adc>(inv, syn::Bundle::zero(y_width), syn::Net::one())
              .slice(0, y_width));
    }

    case ast::UnaryOperator::BitwiseNot: {
      if (y_width == 0) {
        return ir::Value();
      }
      syn::Bundle ext
          = a_signed ? ab.signExtend(y_width) : ab.zeroExtend(y_width);
      return fromBundle(g.add<syn::Not>(ext));
    }

    case ast::UnaryOperator::BitwiseAnd:
    case ast::UnaryOperator::BitwiseOr:
    case ast::UnaryOperator::BitwiseXor:
    case ast::UnaryOperator::BitwiseNand:
    case ast::UnaryOperator::BitwiseNor:
    case ast::UnaryOperator::BitwiseXnor: {
      // Reduction operators: reduce all bits to a single bit
      if (ab.width() == 0) {
        return ir::Value(ir::Sx);
      }

      syn::Net result = ab[0];
      for (uint32_t i = 1; i < ab.width(); i++) {
        syn::Bundle r1(result);
        syn::Bundle r2(ab[i]);
        switch (op) {
          case ast::UnaryOperator::BitwiseAnd:
          case ast::UnaryOperator::BitwiseNand:
            result = g.add<syn::And>(r1, r2)[0];
            break;
          case ast::UnaryOperator::BitwiseOr:
          case ast::UnaryOperator::BitwiseNor:
            result = g.add<syn::Or>(r1, r2)[0];
            break;
          case ast::UnaryOperator::BitwiseXor:
          case ast::UnaryOperator::BitwiseXnor:
            result = g.add<syn::Xor>(r1, r2)[0];
            break;
          default:
            break;
        }
      }

      // Negate for NAND/NOR/XNOR
      if (op == ast::UnaryOperator::BitwiseNand
          || op == ast::UnaryOperator::BitwiseNor
          || op == ast::UnaryOperator::BitwiseXnor) {
        result = g.add<syn::Not>(syn::Bundle(result))[0];
      }

      return fromBundle(syn::Bundle(result).zeroExtend(y_width));
    }

    case ast::UnaryOperator::LogicalNot: {
      // !empty == 1 (vacuous truth)
      if (ab.width() == 0) {
        ir::Value rv{ir::S1};
        rv.extend_u0(y_width);
        return rv;
      }
      // Reduce to bool, then invert
      syn::Net any_set = ab[0];
      for (uint32_t i = 1; i < ab.width(); i++) {
        any_set = g.add<syn::Or>(syn::Bundle(any_set), syn::Bundle(ab[i]))[0];
      }
      syn::Net result = g.add<syn::Not>(syn::Bundle(any_set))[0];
      return fromBundle(syn::Bundle(result).zeroExtend(y_width));
    }

    default:
      log_error("Unsupported unary operator\n");
  }
}

// ============================================================
// Binary operators
// ============================================================

ir::Value BackendGraphBuilder::Biop(ast::BinaryOperator op,
                                    ir::Value a,
                                    ir::Value b,
                                    bool a_signed,
                                    bool b_signed,
                                    uint64_t y_width)
{
  syn::Bundle ab = toBundle(a);
  syn::Bundle bb = toBundle(b);

  // Constant folding: if both inputs are constant, compute at compile time.
  if (ab.isConst() && bb.isConst()) {
    syn::Const ac = ab.toConst();
    syn::Const bc = bb.toConst();

    auto ext = [](const syn::Const& c, bool is_signed, uint64_t w) {
      return extendConst(c, is_signed, w);
    };

    switch (op) {
      case ast::BinaryOperator::BinaryAnd:
        return constResult(
            ext(ac, a_signed, y_width).and_(ext(bc, b_signed, y_width)));
      case ast::BinaryOperator::BinaryOr:
        return constResult(
            ext(ac, a_signed, y_width).or_(ext(bc, b_signed, y_width)));
      case ast::BinaryOperator::BinaryXor:
        return constResult(
            ext(ac, a_signed, y_width).xor_(ext(bc, b_signed, y_width)));
      case ast::BinaryOperator::BinaryXnor:
        return constResult(
            ext(ac, a_signed, y_width).xor_(ext(bc, b_signed, y_width)).not_());
      case ast::BinaryOperator::Add: {
        auto ea = ext(ac, a_signed, y_width);
        auto eb = ext(bc, b_signed, y_width);
        return constResult(ea.adc(eb, syn::Trit::Zero).slice(0, y_width));
      }
      case ast::BinaryOperator::Subtract: {
        auto ea = ext(ac, a_signed, y_width);
        auto eb = ext(bc, b_signed, y_width).not_();
        return constResult(ea.adc(eb, syn::Trit::One).slice(0, y_width));
      }
      case ast::BinaryOperator::Multiply:
        return constResult(
            ext(ac, a_signed, y_width).mul(ext(bc, b_signed, y_width)));
      case ast::BinaryOperator::Equality:
      case ast::BinaryOperator::CaseEquality: {
        uint64_t w = std::max(ac.width(), bc.width());
        syn::Trit r = ext(ac, a_signed, w).eq(ext(bc, b_signed, w));
        return constResult(extendConst(syn::Const::from({r}), false, y_width));
      }
      case ast::BinaryOperator::Inequality:
      case ast::BinaryOperator::CaseInequality: {
        uint64_t w = std::max(ac.width(), bc.width());
        syn::Trit r = !ext(ac, a_signed, w).eq(ext(bc, b_signed, w));
        return constResult(extendConst(syn::Const::from({r}), false, y_width));
      }
      case ast::BinaryOperator::LessThan: {
        uint64_t w = std::max(ac.width(), bc.width());
        auto ea = ext(ac, a_signed, w);
        auto eb = ext(bc, b_signed, w);
        syn::Trit r = (a_signed && b_signed) ? ea.slt(eb) : ea.ult(eb);
        return constResult(extendConst(syn::Const::from({r}), false, y_width));
      }
      case ast::BinaryOperator::LessThanEqual: {
        uint64_t w = std::max(ac.width(), bc.width());
        auto ea = ext(ac, a_signed, w);
        auto eb = ext(bc, b_signed, w);
        syn::Trit gt = (a_signed && b_signed) ? eb.slt(ea) : eb.ult(ea);
        return constResult(
            extendConst(syn::Const::from({!gt}), false, y_width));
      }
      case ast::BinaryOperator::GreaterThan: {
        uint64_t w = std::max(ac.width(), bc.width());
        auto ea = ext(ac, a_signed, w);
        auto eb = ext(bc, b_signed, w);
        syn::Trit r = (a_signed && b_signed) ? eb.slt(ea) : eb.ult(ea);
        return constResult(extendConst(syn::Const::from({r}), false, y_width));
      }
      case ast::BinaryOperator::GreaterThanEqual: {
        uint64_t w = std::max(ac.width(), bc.width());
        auto ea = ext(ac, a_signed, w);
        auto eb = ext(bc, b_signed, w);
        syn::Trit lt = (a_signed && b_signed) ? ea.slt(eb) : ea.ult(eb);
        return constResult(
            extendConst(syn::Const::from({!lt}), false, y_width));
      }
      case ast::BinaryOperator::LogicalAnd: {
        syn::Trit a_bool = ac[0];
        for (uint32_t i = 1; i < ac.width(); i++) {
          a_bool = a_bool | ac[i];
        }
        syn::Trit b_bool = bc[0];
        for (uint32_t i = 1; i < bc.width(); i++) {
          b_bool = b_bool | bc[i];
        }
        return constResult(
            extendConst(syn::Const::from({a_bool & b_bool}), false, y_width));
      }
      case ast::BinaryOperator::LogicalOr: {
        syn::Trit a_bool = ac[0];
        for (uint32_t i = 1; i < ac.width(); i++) {
          a_bool = a_bool | ac[i];
        }
        syn::Trit b_bool = bc[0];
        for (uint32_t i = 1; i < bc.width(); i++) {
          b_bool = b_bool | bc[i];
        }
        return constResult(
            extendConst(syn::Const::from({a_bool | b_bool}), false, y_width));
      }
      case ast::BinaryOperator::LogicalShiftLeft:
        return constResult(ext(ac, a_signed, y_width).shl(bc, 1));
      case ast::BinaryOperator::LogicalShiftRight:
        return constResult(ext(ac, a_signed, y_width).ushr(bc, 1));
      case ast::BinaryOperator::ArithmeticShiftLeft:
        return constResult(ext(ac, a_signed, y_width).shl(bc, 1));
      case ast::BinaryOperator::ArithmeticShiftRight:
        if (a_signed) {
          return constResult(ext(ac, a_signed, y_width).sshr(bc, 1));
        } else {
          return constResult(ext(ac, a_signed, y_width).ushr(bc, 1));
        }
      default:
        break;  // Fall through to graph-building path.
    }
  }

  auto& g = graph();

  // Extend operands to common width for bitwise/arithmetic ops
  auto extend = [&](ir::Value& v, bool is_signed, uint64_t w) {
    if (v.size() < w) {
      if (is_signed) {
        v = fromBundle(toBundle(v).signExtend(w));
      } else {
        v = fromBundle(toBundle(v).zeroExtend(w));
      }
    }
  };

  switch (op) {
    case ast::BinaryOperator::BinaryAnd: {
      uint64_t w = y_width;
      extend(a, a_signed, w);
      extend(b, b_signed, w);
      return fromBundle(g.add<syn::And>(toBundle(a), toBundle(b)));
    }
    case ast::BinaryOperator::BinaryOr: {
      uint64_t w = y_width;
      extend(a, a_signed, w);
      extend(b, b_signed, w);
      return fromBundle(g.add<syn::Or>(toBundle(a), toBundle(b)));
    }
    case ast::BinaryOperator::BinaryXor: {
      uint64_t w = y_width;
      extend(a, a_signed, w);
      extend(b, b_signed, w);
      return fromBundle(g.add<syn::Xor>(toBundle(a), toBundle(b)));
    }
    case ast::BinaryOperator::BinaryXnor: {
      uint64_t w = y_width;
      extend(a, a_signed, w);
      extend(b, b_signed, w);
      syn::Bundle xor_result = g.add<syn::Xor>(toBundle(a), toBundle(b));
      return fromBundle(g.add<syn::Not>(xor_result));
    }

    case ast::BinaryOperator::Add: {
      uint64_t w = y_width;
      extend(a, a_signed, w);
      extend(b, b_signed, w);
      // Adc returns w+1 bits (sum + carry); slice to y_width
      syn::Bundle sum
          = g.add<syn::Adc>(toBundle(a), toBundle(b), syn::Net::zero());
      return fromBundle(sum.slice(0, y_width));
    }
    case ast::BinaryOperator::Subtract: {
      uint64_t w = y_width;
      extend(a, a_signed, w);
      extend(b, b_signed, w);
      // a - b = a + ~b + 1 (cin=1)
      syn::Bundle nb = g.add<syn::Not>(toBundle(b));
      syn::Bundle sum = g.add<syn::Adc>(toBundle(a), nb, syn::Net::one());
      return fromBundle(sum.slice(0, y_width));
    }
    case ast::BinaryOperator::Multiply: {
      uint64_t w = std::max({a.size(), b.size(), y_width});
      extend(a, a_signed, w);
      extend(b, b_signed, w);
      ir::Value r = fromBundle(g.add<syn::Mul>(toBundle(a), toBundle(b)));
      if (r.size() > y_width) {
        r = r.extract(0, y_width);
      }
      return r;
    }
    case ast::BinaryOperator::Divide: {
      uint64_t w = y_width;
      extend(a, a_signed, w);
      extend(b, b_signed, w);
      if (a_signed || b_signed) {
        return fromBundle(g.add<syn::SDivTrunc>(toBundle(a), toBundle(b)));
      }
      return fromBundle(g.add<syn::UDiv>(toBundle(a), toBundle(b)));
    }
    case ast::BinaryOperator::Mod: {
      uint64_t w = y_width;
      extend(a, a_signed, w);
      extend(b, b_signed, w);
      if (a_signed || b_signed) {
        return fromBundle(g.add<syn::SModTrunc>(toBundle(a), toBundle(b)));
      }
      return fromBundle(g.add<syn::UMod>(toBundle(a), toBundle(b)));
    }

    case ast::BinaryOperator::Equality:
    case ast::BinaryOperator::CaseEquality: {
      uint64_t w = std::max(a.size(), b.size());
      extend(a, a_signed, w);
      extend(b, b_signed, w);
      syn::Bundle eq = g.add<syn::Eq>(toBundle(a), toBundle(b));
      return fromBundle(eq.zeroExtend(y_width));
    }
    case ast::BinaryOperator::Inequality:
    case ast::BinaryOperator::CaseInequality: {
      uint64_t w = std::max(a.size(), b.size());
      extend(a, a_signed, w);
      extend(b, b_signed, w);
      syn::Bundle eq = g.add<syn::Eq>(toBundle(a), toBundle(b));
      syn::Bundle neq = g.add<syn::Not>(eq);
      return fromBundle(neq.zeroExtend(y_width));
    }

    case ast::BinaryOperator::LessThan: {
      uint64_t w = std::max(a.size(), b.size());
      extend(a, a_signed, w);
      extend(b, b_signed, w);
      syn::Bundle lt = (a_signed && b_signed)
                           ? g.add<syn::SLt>(toBundle(a), toBundle(b))
                           : g.add<syn::ULt>(toBundle(a), toBundle(b));
      return fromBundle(lt.zeroExtend(y_width));
    }
    case ast::BinaryOperator::LessThanEqual: {
      // a <= b is !(b < a)
      uint64_t w = std::max(a.size(), b.size());
      extend(a, a_signed, w);
      extend(b, b_signed, w);
      syn::Bundle gt = (a_signed && b_signed)
                           ? g.add<syn::SLt>(toBundle(b), toBundle(a))
                           : g.add<syn::ULt>(toBundle(b), toBundle(a));
      syn::Bundle ngt = g.add<syn::Not>(gt);
      return fromBundle(ngt.zeroExtend(y_width));
    }
    case ast::BinaryOperator::GreaterThan: {
      // a > b is b < a
      uint64_t w = std::max(a.size(), b.size());
      extend(a, a_signed, w);
      extend(b, b_signed, w);
      syn::Bundle gt = (a_signed && b_signed)
                           ? g.add<syn::SLt>(toBundle(b), toBundle(a))
                           : g.add<syn::ULt>(toBundle(b), toBundle(a));
      return fromBundle(gt.zeroExtend(y_width));
    }
    case ast::BinaryOperator::GreaterThanEqual: {
      // a >= b is !(a < b)
      uint64_t w = std::max(a.size(), b.size());
      extend(a, a_signed, w);
      extend(b, b_signed, w);
      syn::Bundle lt = (a_signed && b_signed)
                           ? g.add<syn::SLt>(toBundle(a), toBundle(b))
                           : g.add<syn::ULt>(toBundle(a), toBundle(b));
      syn::Bundle nlt = g.add<syn::Not>(lt);
      return fromBundle(nlt.zeroExtend(y_width));
    }

    case ast::BinaryOperator::LogicalAnd: {
      // Reduce both to bool, then AND
      syn::Net a_bool = ir::Net(a[0]).raw_;
      for (uint64_t i = 1; i < a.size(); i++) {
        a_bool = g.add<syn::Or>(syn::Bundle(a_bool),
                                syn::Bundle(ir::Net(a[i]).raw_))[0];
      }
      syn::Net b_bool = ir::Net(b[0]).raw_;
      for (uint64_t i = 1; i < b.size(); i++) {
        b_bool = g.add<syn::Or>(syn::Bundle(b_bool),
                                syn::Bundle(ir::Net(b[i]).raw_))[0];
      }
      syn::Bundle result
          = g.add<syn::And>(syn::Bundle(a_bool), syn::Bundle(b_bool));
      return fromBundle(result.zeroExtend(y_width));
    }
    case ast::BinaryOperator::LogicalOr: {
      syn::Net a_bool = ir::Net(a[0]).raw_;
      for (uint64_t i = 1; i < a.size(); i++) {
        a_bool = g.add<syn::Or>(syn::Bundle(a_bool),
                                syn::Bundle(ir::Net(a[i]).raw_))[0];
      }
      syn::Net b_bool = ir::Net(b[0]).raw_;
      for (uint64_t i = 1; i < b.size(); i++) {
        b_bool = g.add<syn::Or>(syn::Bundle(b_bool),
                                syn::Bundle(ir::Net(b[i]).raw_))[0];
      }
      syn::Bundle result
          = g.add<syn::Or>(syn::Bundle(a_bool), syn::Bundle(b_bool));
      return fromBundle(result.zeroExtend(y_width));
    }

    case ast::BinaryOperator::LogicalShiftLeft: {
      extend(a, a_signed, y_width);
      return fromBundle(g.add<syn::Shl>(toBundle(a), toBundle(b), 1));
    }
    case ast::BinaryOperator::LogicalShiftRight: {
      extend(a, a_signed, y_width);
      return fromBundle(g.add<syn::UShr>(toBundle(a), toBundle(b), 1));
    }
    case ast::BinaryOperator::ArithmeticShiftLeft: {
      extend(a, a_signed, y_width);
      return fromBundle(g.add<syn::Shl>(toBundle(a), toBundle(b), 1));
    }
    case ast::BinaryOperator::ArithmeticShiftRight: {
      extend(a, a_signed, y_width);
      if (a_signed) {
        return fromBundle(g.add<syn::SShr>(toBundle(a), toBundle(b), 1));
      }
      return fromBundle(g.add<syn::UShr>(toBundle(a), toBundle(b), 1));
    }

    default:
      log_error("Unsupported binary operator\n");
  }
}

// ============================================================
// Mux / shift / demux
// ============================================================

ir::Value BackendGraphBuilder::Mux(ir::Value a, ir::Value b, ir::Net s)
{
  // Elaborator convention: s=0 selects a, s=1 selects b.
  // Instance convention: sel=1 selects a, sel=0 selects b.
  // Swap a/b to reconcile.
  return fromBundle(graph().add<syn::Mux>(s.raw_, toBundle(b), toBundle(a)));
}

ir::Value BackendGraphBuilder::Shift(ir::Value a,
                                     ir::Value s,
                                     bool s_signed,
                                     uint64_t result_width)
{
  (void) s_signed;
  auto& g = graph();
  auto ab = toBundle(a);
  auto sb = toBundle(s);
  // Use right shift (UShr) for positive shift amounts
  ab = ab.zeroExtend(std::max((uint32_t) result_width, ab.width()));
  syn::Bundle result = g.add<syn::UShr>(ab, sb, 1);
  return fromBundle(result.slice(0, result_width));
}

ir::Value BackendGraphBuilder::Shiftx(ir::Value a,
                                      ir::Value s,
                                      bool s_signed,
                                      uint64_t result_width)
{
  (void) s_signed;
  auto& g = graph();
  auto ab = toBundle(a);
  auto sb = toBundle(s);
  ab = ab.zeroExtend(std::max((uint32_t) result_width, ab.width()));
  syn::Bundle result = g.add<syn::XShr>(ab, sb, 1);
  return fromBundle(result.slice(0, result_width));
}

ir::Value BackendGraphBuilder::Demux(ir::Value a, ir::Value s)
{
  auto& g = graph();
  uint64_t out_width = a.size() << s.size();
  return fromBundle(g.add<syn::Shl>(
      toBundle(a).zeroExtend(out_width), toBundle(s), a.size()));
}

ir::Value BackendGraphBuilder::Bwmux(ir::Value a, ir::Value b, ir::Value s)
{
  ir::Value result;
  result.reserve(a.size());
  for (uint64_t base = 0; base < a.size();) {
    uint64_t width = 1;
    while (base + width < s.size() && s[base + width] == s[base]) {
      width++;
    }
    result.append(Mux(a.extract(base, width), b.extract(base, width), s[base]));
    base += width;
  }
  return result;
}

ir::Value BackendGraphBuilder::Bmux(ir::Value a, ir::Value s)
{
  log_assert(a.size() % (1 << s.size()) == 0);
  log_assert(a.size() >= 1ull << s.size());

  uint64_t width = a.size() >> s.size();
  auto& g = graph();

  // Bmux(a, s) selects the width-bit slice at offset s*width.
  // This is equivalent to (a >> (s * width))[width-1:0], implemented
  // as a barrel shifter via UShr with stride=width.
  syn::Bundle ab = toBundle(a);
  syn::Bundle sb = toBundle(s);
  syn::Bundle result = g.add<syn::UShr>(ab, sb, (uint32_t) width);
  return fromBundle(result.slice(0, width));
}

// ============================================================
// Placeholder signals and connections
// ============================================================

ir::Value BackendGraphBuilder::add_placeholder_signal(
    uint64_t width,
    std::string_view name_suggestion,
    bool public_name)
{
  auto& g = graph();
  ir::Value sig = fromBundle(g.add<syn::Buffer>(syn::Bundle::undef(width)));
  if (public_name && !name_suggestion.empty()) {
    g.add<syn::Name>(std::string(name_suggestion), toBundle(sig));
  }
  return sig;
}

void BackendGraphBuilder::add_input(std::string_view name, ir::Value signal)
{
  auto& g = graph();
  syn::Bundle input = g.add<syn::Input>(std::string(name), signal.size());
  connect(signal, fromBundle(input));
}

void BackendGraphBuilder::add_output(std::string_view name, ir::Value signal)
{
  auto& g = graph();
  g.add<syn::Output>(std::string(name), toBundle(signal));
}

void BackendGraphBuilder::add_instance(std::string_view cell_type,
                                       std::vector<PortConnection> ports)
{
  auto& g = graph();
  std::vector<syn::Other::Port> syn_ports;
  syn_ports.reserve(ports.size());
  for (auto& pc : ports) {
    syn::Other::Port::Direction dir = syn::Other::Port::kInput;
    uint32_t port_width = pc.value.size();
    syn::Bundle val;

    switch (pc.direction) {
      case PortConnection::kInput:
        dir = syn::Other::Port::kInput;
        val = toBundle(pc.value);
        break;
      case PortConnection::kOutput:
        dir = syn::Other::Port::kOutput;
        break;
      case PortConnection::kInOut:
        dir = syn::Other::Port::kInOut;
        val = toBundle(pc.value);
        break;
    }
    syn_ports.push_back({std::move(pc.name), dir, port_width, std::move(val)});
  }

  // Create the Other instance.
  syn::Bundle instance_outputs
      = g.add<syn::Other>(std::string(cell_type), std::move(syn_ports));

  // Connect output port placeholders to the instance's output nets.
  uint32_t out_offset = 0;
  for (auto& port : ports) {
    if (port.direction == PortConnection::kOutput) {
      uint32_t w = port.value.size();
      ir::Value out_slice = fromBundle(instance_outputs.slice(out_offset, w));
      connect(port.value, out_slice);
      out_offset += w;
    }
  }
}

void BackendGraphBuilder::connect(ir::Value target, ir::Value source)
{
  auto& g = graph();
  log_assert(target.size() == source.size());
  for (uint64_t i = 0; i < target.size(); i++) {
    syn::Net target_net = ir::Net(target[i]).raw_;
    auto [inst, offset] = g.resolve(target_net);
    log_assert(inst->is<syn::Buffer>());
    inst->as<syn::Buffer>()->setA(offset, ir::Net(source[i]).raw_);
  }
}

void BackendGraphBuilder::set_initialization(ir::Value, ir::Const)
{
}

// ============================================================
// Sequential elements
// ============================================================

void BackendGraphBuilder::add_dff(std::string_view,
                                  const ir::Value& clk,
                                  const ir::Value& d,
                                  const ir::Value& q,
                                  bool clk_polarity)
{
  auto& g = graph();
  syn::Net clk_net = clk.as_net().raw_;
  syn::Bundle db = toBundle(d);
  uint32_t w = db.width();
  auto clk_cn = clk_polarity ? syn::ControlNet::pos(clk_net)
                             : syn::ControlNet::neg(clk_net);
  syn::Bundle dff_out = g.add<syn::Dff>(db,
                                        clk_cn,
                                        syn::ControlNet::zero(),
                                        syn::ControlNet::zero(),
                                        syn::ControlNet::one(),
                                        syn::Const::undef(w),
                                        syn::Const::undef(w),
                                        syn::Const::undef(w));
  connect(q, fromBundle(dff_out));
}

void BackendGraphBuilder::add_dffe(std::string_view,
                                   const ir::Value& clk,
                                   const ir::Value& en,
                                   const ir::Value& d,
                                   const ir::Value& q,
                                   bool clk_polarity,
                                   bool en_polarity)
{
  auto& g = graph();
  syn::Net clk_net = clk.as_net().raw_;
  syn::Net en_net = en.as_net().raw_;
  syn::Bundle db = toBundle(d);
  uint32_t w = db.width();

  auto clk_cn = clk_polarity ? syn::ControlNet::pos(clk_net)
                             : syn::ControlNet::neg(clk_net);
  auto en_cn = en_polarity ? syn::ControlNet::pos(en_net)
                           : syn::ControlNet::neg(en_net);

  syn::Bundle dff_out = g.add<syn::Dff>(db,
                                        clk_cn,
                                        syn::ControlNet::zero(),
                                        syn::ControlNet::zero(),
                                        en_cn,
                                        syn::Const::undef(w),
                                        syn::Const::undef(w),
                                        syn::Const::undef(w));
  connect(q, fromBundle(dff_out));
}

void BackendGraphBuilder::add_aldff(std::string_view,
                                    const ir::Value& clk,
                                    const ir::Value& aload,
                                    const ir::Value& d,
                                    const ir::Value& q,
                                    const ir::Value& ad,
                                    bool clk_polarity,
                                    bool aload_polarity)
{
  auto& g = graph();
  syn::Net clk_net = clk.as_net().raw_;
  syn::Net aload_net = aload.as_net().raw_;
  syn::Bundle db = toBundle(d);
  uint32_t w = db.width();

  auto clk_cn = clk_polarity ? syn::ControlNet::pos(clk_net)
                             : syn::ControlNet::neg(clk_net);
  auto clear_cn = aload_polarity ? syn::ControlNet::pos(aload_net)
                                 : syn::ControlNet::neg(aload_net);

  if (!ad.is_fully_const()) {
    reportError(graph_->logger(),
                91,
                "Flops with non-constant asynchronous load unsupported");
  }

  syn::Bundle dff_out = g.add<syn::Dff>(db,
                                        clk_cn,
                                        clear_cn,
                                        syn::ControlNet::zero(),
                                        syn::ControlNet::one(),
                                        syn::Const::undef(w),
                                        syn::Const::undef(w),
                                        toBundle(ad).toConst());
  connect(q, fromBundle(dff_out));
}

void BackendGraphBuilder::add_dual_edge_aldff(const std::string&,
                                              ir::Value clk,
                                              ir::Value aload,
                                              ir::Value d,
                                              ir::Value q,
                                              ir::Value ad,
                                              bool aload_polarity)
{
  log_error("no support\n");
}

void BackendGraphBuilder::add_memory_init(std::string_view,
                                          uint64_t,
                                          bool,
                                          ir::Const)
{
}

}  // namespace slang_frontend
