// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include <cstdint>
#include <functional>
#include <initializer_list>
#include <string>
#include <utility>
#include <vector>

#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "helpers/TestDesignOptions.h"
#include "helpers/TestLiberty.h"
#include "helpers/TestModules.h"
#include "helpers/WellFormedFixtures.h"
#include "syn/lir/ConstPool.h"
#include "syn/lir/Design.h"
#include "syn/lir/Ids.h"
#include "syn/lir/LibCellTable.h"
#include "syn/lir/Module.h"
#include "syn/lir/ModuleBuilder.h"
#include "syn/lir/NameInterner.h"
#include "syn/lir/ObjAttributes.h"
#include "syn/lir/Op.h"
#include "syn/lir/OperatorSpec.h"
#include "syn/lir/RegClassPool.h"
#include "syn/lir/RegPinTypes.h"
#include "syn/lir/SourceLoc.h"
#include "syn/lir/WellFormed.h"

namespace syn::test {
namespace {

using ::syn::lir::BoundaryOpt;
using ::syn::lir::checkOperatorInterface;
using ::syn::lir::checkWellFormed;
using ::syn::lir::ConstId;
using ::syn::lir::ConstPool;
using ::syn::lir::Design;
using ::syn::lir::kObjIdBits;
using ::syn::lir::LibCellId;
using ::syn::lir::ModId;
using ::syn::lir::Module;
using ::syn::lir::ModuleBuilder;
using ::syn::lir::NameId;
using ::syn::lir::ObjAttributes;
using ::syn::lir::ObjId;
using ::syn::lir::ObjIndex;
using ::syn::lir::ObjRecord;
using ::syn::lir::Op;
using ::syn::lir::opName;
using ::syn::lir::OpOperand;
using ::syn::lir::OpSpec;
using ::syn::lir::RegClass;
using ::syn::lir::RegClassId;
using ::syn::lir::RegFlags;
using ::syn::lir::RegInit;
using ::syn::lir::regPinBit;
using ::syn::lir::RegPinMask;
using ::syn::lir::RegPinType;
using ::syn::lir::Signedness;
using ::syn::lir::SourceLoc;
using ::syn::lir::TableMode;

using ::testing::HasSubstr;

TEST_F(WellFormedTest, AcceptsEveryOpWhenWellFormed)
{
  // A child for Inst/Fan: y = ~a.
  ModuleBuilder child(design_.names().intern("inv"));
  child.addPo(pn(design_),
              child.add(Op::kNot, 4, {child.addPi(pn(design_), 4)}));
  const ModId inv = addModuleOrFail(design_, std::move(child));
  ASSERT_TRUE(inv.isValid());
  const ConstId and2_table = design_.consts().internUint(4, 0b1000);
  const ConstId wide = design_.consts().internUint(40, 0x12345);

  ModuleBuilder b(design_.names().intern("top"));
  const ObjId a = b.addPi(pn(design_), 8);
  const ObjId c = b.addPi(pn(design_), 8);
  const ObjId s = b.addPi(pn(design_), 1);
  const ObjId sel2 = b.addPi(pn(design_), 2);
  const ObjId lo = b.add(Op::kSlice, 4, {a}, 0);
  const ObjId hi = b.add(Op::kSlice, 4, {a}, 4);
  b.addPo(pn(design_), b.add(Op::kConcat, 8, {hi, lo}));
  b.addPo(pn(design_), b.add(Op::kReplicate, 12, {lo}, 3));
  b.addPo(pn(design_), b.add(Op::kCast, 16, {a}, 0, Signedness::kSigned));
  b.addPo(pn(design_), b.add(Op::kMux, 8, {s, a, c}));
  b.addPo(pn(design_),
          b.add(Op::kPmux, 8, {sel2, b.add(Op::kConcat, 16, {a, c}), a}));
  b.addPo(pn(design_),
          b.add(Op::kBmux, 4, {sel2, b.add(Op::kConcat, 16, {a, c})}));
  b.addPo(pn(design_), b.add(Op::kDemux, 32, {sel2, a}));
  b.addPo(pn(design_), b.add(Op::kAnd, 8, {a, c, a}));
  b.addPo(pn(design_), b.add(Op::kAdd, 8, {a, b.addConst(8, 1)}));
  b.addPo(pn(design_), b.add(Op::kMul, 16, {a, lo}));
  // Pow keeps native operand widths, each may differ from the result's.
  b.addPo(pn(design_), b.add(Op::kPow, 6, {a, sel2}, 0, Signedness::kSigned));
  b.addPo(pn(design_), b.add(Op::kPow, 1, {lo, a}));
  b.addPo(pn(design_), b.add(Op::kLt, 1, {a, c}, 0, Signedness::kSigned));
  b.addPo(pn(design_), b.add(Op::kShr, 8, {a, sel2}));
  b.addPo(pn(design_), b.add(Op::kReduceXor, 1, {a}));
  const ObjId s0 = b.add(Op::kSlice, 1, {a}, 0);
  b.addPo(pn(design_), b.add(Op::kLut, 1, {s0, s}, and2_table.rawIndex()));
  b.addPo(pn(design_),
          b.addConst(design_.consts().get(wide), design_.consts()));
  const ObjId inst = b.add(Op::kInst, 0, {lo}, inv.value());
  b.addPo(pn(design_), b.add(Op::kFan, 4, {inst}, 0));
  // A resolved tri-state bus: two buffers and a strong driver.
  const ObjId t0 = b.add(Op::kTribuf, 8, {s, a});
  const ObjId t1 = b.add(Op::kTribuf, 8, {b.add(Op::kNot, 1, {s}), c});
  b.addPo(pn(design_), b.add(Op::kTriJoin, 8, {t0, t1, a}));
  absl::StatusOr<Module> top = std::move(b).finish();
  ASSERT_TRUE(top.ok()) << top.status();
  EXPECT_TRUE(top->isOrdered());
  EXPECT_TRUE(checkWellFormed(design_, *top).ok())
      << checkWellFormed(design_, *top);

  ASSERT_TRUE(design_.addModule(*std::move(top)).ok());
  EXPECT_TRUE(checkWellFormed(design_).ok()) << checkWellFormed(design_);
}

TEST_F(WellFormedTest, ReportsWidthErrors)
{
  struct Case
  {
    std::function<void(ModuleBuilder&, ObjId, ObjId)> build;  // a: 8, s: 1
    std::string message;
  };
  const ConstId table3 = design_.consts().internUint(3, 0);
  const std::vector<Case> cases = {
      {
          .build =
              [](ModuleBuilder& b, ObjId a, ObjId) {
                b.add(Op::kAdd, 8, {a, b.add(Op::kSlice, 4, {a}, 0)});
              },
          .message = "(add): fanin 1 has width 4, expected 8",
      },
      {
          .build = [](ModuleBuilder& b,
                      ObjId a,
                      ObjId) { b.add(Op::kMux, 8, {a, a, a}); },
          .message = "(mux): fanin 0 has width 8, expected 1",
      },
      {
          .build
          = [](ModuleBuilder& b, ObjId a, ObjId) { b.add(Op::kEq, 2, {a, a}); },
          .message = "(eq): has width 2, expected 1",
      },
      {
          .build = [](ModuleBuilder& b,
                      ObjId a,
                      ObjId) { b.add(Op::kSlice, 4, {a}, 6); },
          .message = "are outside a 8-bit fanin",
      },
      {
          .build = [](ModuleBuilder& b,
                      ObjId a,
                      ObjId s) { b.add(Op::kConcat, 8, {a, s}); },
          .message = "fanins add up to 9 bits, expected 8",
      },
      {
          .build
          = [](ModuleBuilder& b, ObjId a, ObjId) { b.add(Op::kAnd, 8, {a}); },
          .message = "has 1 fanins, expected at least 2",
      },
      {
          .build = [](ModuleBuilder& b,
                      ObjId a,
                      ObjId) { b.add(Op::kNot, 8, {a, a}); },
          .message = "has 2 fanins, expected 1",
      },
      {
          .build = [](ModuleBuilder& b,
                      ObjId a,
                      ObjId) { b.add(Op::kShl, 4, {a, a}); },
          .message = "(shl): fanin 0 has width 8, expected 4",
      },
      {
          .build =
              [&](ModuleBuilder& b, ObjId, ObjId s) {
                b.add(Op::kLut, 1, {s, s}, table3.rawIndex());
              },
          .message = "truth table has 3 bits, expected 4",
      },
      {
          .build
          = [](ModuleBuilder& b, ObjId a, ObjId) { b.add(Op::kBuf, 0, {a}); },
          .message = "(buf): width 0",
      },
      {
          .build = [](ModuleBuilder& b,
                      ObjId a,
                      ObjId) { b.add(Op::kTribuf, 8, {a, a}); },
          .message = "(tribuf): fanin 0 has width 8, expected 1",
      },
      {
          .build = [](ModuleBuilder& b,
                      ObjId a,
                      ObjId s) { b.add(Op::kTribuf, 4, {s, a}); },
          .message = "(tribuf): fanin 1 has width 8, expected 4",
      },
      {
          .build = [](ModuleBuilder& b,
                      ObjId,
                      ObjId s) { b.add(Op::kTribuf, 8, {s}); },
          .message = "(tribuf): has 1 fanins, expected 2",
      },
      {
          .build = [](ModuleBuilder& b,
                      ObjId a,
                      ObjId) { b.add(Op::kTriJoin, 8, {a}); },
          .message = "(tri_join): has 1 fanins, expected at least 2",
      },
      {
          .build =
              [](ModuleBuilder& b, ObjId a, ObjId) {
                b.add(Op::kTriJoin, 8, {a, b.add(Op::kSlice, 4, {a}, 0)});
              },
          .message = "(tri_join): fanin 1 has width 4, expected 8",
      },
      {
          .build
          = [](ModuleBuilder& b, ObjId a, ObjId) { b.add(Op::kPow, 8, {a}); },
          .message = "(pow): has 1 fanins, expected 2",
      },
      {
          .build = [](ModuleBuilder& b,
                      ObjId a,
                      ObjId s) { b.add(Op::kPow, 8, {a, s, s}); },
          .message = "(pow): has 3 fanins, expected 2",
      },
      {
          .build = [](ModuleBuilder& b,
                      ObjId a,
                      ObjId s) { b.add(Op::kPow, 0, {a, s}); },
          .message = "(pow): width 0",
      },
  };
  for (const Case& c : cases) {
    ModuleBuilder b;
    const ObjId a = b.addPi(pn(design_), 8);
    const ObjId s = b.addPi(pn(design_), 1);
    c.build(b, a, s);
    EXPECT_THAT(error(std::move(b)), HasSubstr(c.message));
  }
}

TEST_F(WellFormedTest, NamesTheModuleAndTheObject)
{
  ModuleBuilder b(design_.names().intern("m"));
  const ObjId a = b.addPi(pn(design_), 8);
  const ObjId bad = b.add(Op::kNeg, 4, {a});
  b.setName(bad, design_.names().intern("minus_a"));
  EXPECT_THAT(error(std::move(b)),
              HasSubstr("module 'm': object 1 'minus_a' (neg): fanin 0 has "
                        "width 8, expected 4"));
}

TEST_F(WellFormedTest, AcceptsUnorderedAcyclicModules)
{
  ModuleBuilder b;
  const ObjId p = b.addPlaceholder(1);
  b.addPo(pn(design_), b.add(Op::kNot, 1, {p}));
  b.drivePlaceholder(p,
                     b.addPi(pn(design_), 1));  // the Pi comes after its load
  absl::StatusOr<Module> module = std::move(b).finish();
  ASSERT_TRUE(module.ok());
  EXPECT_FALSE(module->isOrdered());
  EXPECT_TRUE(checkWellFormed(design_, *module).ok());
}

TEST_F(WellFormedTest, AcceptsUndriven)
{
  ModuleBuilder b;
  b.setAllowUndriven(true);
  b.addPo(pn(design_), b.addPlaceholder(4));
  EXPECT_TRUE(check(std::move(b)).ok());
}

TEST_F(WellFormedTest, AcceptsRegistersInNormalForm)
{
  // Bit register with async reset; the cycle through Q is fine.
  const RegClass bit_class = regClass(
      {RegPinType::kData, RegPinType::kClock, RegPinType::kAsyncReset});
  EXPECT_TRUE(
      check(registerModule(1, bit_class, [this](ModuleBuilder& b, ObjId q) {
        const ObjId clk = b.addPi(pn(design_), 1);
        const ObjId rst = b.addPi(pn(design_), 1);
        return std::vector<ObjId>{b.add(Op::kNot, 1, {q}), clk, rst};
      })).ok());

  // Word register with a mixed async reset value (inline and pooled).
  const RegClass word_class = regClass({
      RegPinType::kData,
      RegPinType::kClock,
      RegPinType::kEnable,
      RegPinType::kAsyncReset,
      RegPinType::kAsyncResetValue,
  });
  for (const int width : {8, 40}) {
    const ConstId value = design_.consts().internUint(width, 0b1010);
    const absl::Status status
        = check(registerModule(width, word_class, [&](ModuleBuilder& b, ObjId) {
            const ObjId d = b.addPi(pn(design_), width);
            const ObjId clk = b.addPi(pn(design_), 1);
            const ObjId en = b.addPi(pn(design_), 1);
            const ObjId rst = b.addPi(pn(design_), 1);
            const ObjId v
                = b.addConst(design_.consts().get(value), design_.consts());
            return std::vector<ObjId>{d, clk, en, rst, v};
          }));
    EXPECT_TRUE(status.ok()) << width << ": " << status;
  }
}

TEST_F(WellFormedTest, ReportsRegisterErrors)
{
  const RegClass no_clock = regClass({RegPinType::kData});
  EXPECT_THAT(
      error(registerModule(1,
                           no_clock,
                           [this](ModuleBuilder& b, ObjId) {
                             return std::vector<ObjId>{b.addPi(pn(design_), 1)};
                           })),
      HasSubstr("lacks a data or clock pin"));

  const RegClass with_enable
      = regClass({RegPinType::kData, RegPinType::kClock, RegPinType::kEnable});
  EXPECT_THAT(error(registerModule(4,
                                   with_enable,
                                   [this](ModuleBuilder& b, ObjId) {
                                     return std::vector<ObjId>{
                                         b.addPi(pn(design_), 4),
                                         b.addPi(pn(design_), 1)};
                                   })),
              HasSubstr("has 2 fanins, expected 3"));
  EXPECT_THAT(error(registerModule(4,
                                   with_enable,
                                   [this](ModuleBuilder& b, ObjId) {
                                     return std::vector<ObjId>{
                                         b.addPi(pn(design_), 4),
                                         b.addPi(pn(design_), 1),
                                         b.addPi(pn(design_), 2)};
                                   })),
              HasSubstr("has width 2, expected 1"));

  const RegClass mapped = RegClass::mapped(LibCellId(0));
  EXPECT_THAT(error(registerModule(4,
                                   mapped,
                                   [this](ModuleBuilder& b, ObjId) {
                                     return std::vector<ObjId>{
                                         b.addPi(pn(design_), 4),
                                         b.addPi(pn(design_), 1)};
                                   })),
              HasSubstr("has width 4, expected 1"));
  EXPECT_THAT(error(registerModule(1,
                                   mapped,
                                   [this](ModuleBuilder& b, ObjId) {
                                     return std::vector<ObjId>{
                                         b.addPi(pn(design_), 1),
                                         b.addPi(pn(design_), 1)};
                                   })),
              HasSubstr("LibCellId 0 is not in the table"));

  EXPECT_THAT(error(registerModule(1,
                                   RegClassId(999),
                                   [this](ModuleBuilder& b, ObjId) {
                                     return std::vector<ObjId>{
                                         b.addPi(pn(design_), 1),
                                         b.addPi(pn(design_), 1)};
                                   })),
              HasSubstr("RegClassId 999 is not in the pool"));
}

TEST_F(WellFormedTest, ChecksLatchesInitialStatesAndRegisterFans)
{
  auto dataClock = [this](int width) {
    return [this, width](ModuleBuilder& b, ObjId) {
      return std::vector<ObjId>{
          b.addPi(pn(design_), width),
          b.addPi(pn(design_), 1),
      };
    };
  };

  // Latches: data, gate and async reset/set only.
  EXPECT_TRUE(check(registerModule(1,
                                   regClass({RegPinType::kData,
                                             RegPinType::kClock,
                                             RegPinType::kAsyncReset},
                                            RegFlags{.latch = true}),
                                   [this](ModuleBuilder& b, ObjId) {
                                     return std::vector<ObjId>{
                                         b.addPi(pn(design_), 1),
                                         b.addPi(pn(design_), 1),
                                         b.addPi(pn(design_), 1)};
                                   }))
                  .ok());
  EXPECT_THAT(
      error(registerModule(
          1,
          regClass({RegPinType::kData, RegPinType::kClock, RegPinType::kEnable},
                   RegFlags{.latch = true}),
          [this](ModuleBuilder& b, ObjId) {
            return std::vector<ObjId>{b.addPi(pn(design_), 1),
                                      b.addPi(pn(design_), 1),
                                      b.addPi(pn(design_), 1)};
          })),
      HasSubstr("a latch has only data, gate and async reset/set pins"));

  // Initial states.
  const auto flop = {RegPinType::kData, RegPinType::kClock};
  EXPECT_TRUE(
      check(registerModule(4,
                           regClass(flop, {}, RegInit::inlineValue(0b101, 0xF)),
                           dataClock(4)))
          .ok());
  EXPECT_THAT(
      error(registerModule(
          4, regClass(flop, {}, RegInit::inlineValue(0, 0x1F)), dataClock(4))),
      HasSubstr("initial state has bits outside the register"));
  EXPECT_THAT(
      error(registerModule(
          40, regClass(flop, {}, RegInit::inlineValue(0, 1)), dataClock(40))),
      HasSubstr("needs a pooled initial state"));
  const ConstId wide = design_.consts().internUint(40, 3);
  EXPECT_TRUE(
      check(registerModule(
                40, regClass(flop, {}, RegInit::pooled(wide)), dataClock(40)))
          .ok());
  const ConstId narrow = design_.consts().internUint(48, 3);
  EXPECT_THAT(
      error(registerModule(
          40, regClass(flop, {}, RegInit::pooled(narrow)), dataClock(40))),
      HasSubstr("initial state has 48 bits, expected 40"));

  // Register Fans: Fan.data is a RegOutput other than Q, at most one each.
  auto withFans = [&](std::initializer_list<uint32_t> outputs) {
    ModuleBuilder b(design_.names().intern("fans"));
    const ObjId q = b.addRegisterOut(4, b.regClasses().intern(regClass(flop)));
    b.addRegisterIn(q, {b.addPi(pn(design_), 4), b.addPi(pn(design_), 1)});
    for (const uint32_t output : outputs) {
      b.addPo(pn(design_), b.add(Op::kFan, 4, {q}, output));
    }
    return b;
  };
  EXPECT_TRUE(check(withFans({1, 2})).ok());
  EXPECT_THAT(error(withFans({0})), HasSubstr("is not QN or scan-out"));
  EXPECT_THAT(error(withFans({3})), HasSubstr("is not QN or scan-out"));
  EXPECT_THAT(error(withFans({1, 1})),
              HasSubstr("already has a Fan for output qn"));
}

TEST_F(WellFormedTest, ReportsResetNormalFormViolations)
{
  const RegClass with_value = regClass({
      RegPinType::kData,
      RegPinType::kClock,
      RegPinType::kSyncReset,
      RegPinType::kSyncResetValue,
  });
  auto build = [&](int width, auto make_value) {
    return registerModule(width, with_value, [&](ModuleBuilder& b, ObjId) {
      const ObjId d = b.addPi(pn(design_), width);
      const ObjId clk = b.addPi(pn(design_), 1);
      const ObjId rst = b.addPi(pn(design_), 1);
      return std::vector<ObjId>{d, clk, rst, make_value(b, width)};
    });
  };
  auto constant = [](uint32_t value) {
    return [value](ModuleBuilder& b, int width) {
      return b.addConst(width, value);
    };
  };
  EXPECT_THAT(error(build(1, constant(1))), HasSubstr("a bit register"));
  EXPECT_THAT(error(build(4, constant(0))),
              HasSubstr("is all 0; use the reset alone"));
  EXPECT_THAT(error(build(4, constant(0xF))), HasSubstr("is all 1; use a set"));
  EXPECT_THAT(error(build(4,
                          [this](ModuleBuilder& b, int width) {
                            return b.addPi(pn(design_), width);
                          })),
              HasSubstr("is not a constant"));
  EXPECT_TRUE(check(build(4, constant(0b0110))).ok());

  // In an unordered module the reset value comes after the register, so the
  // register's check reaches it first: an unknown ConstId is an error there,
  // not an assert in the pool.
  {
    ConstPool other;
    other.setMode(TableMode::kSingleWrite);
    const ConstId unknown = other.internUint(40, 0b0110);
    ASSERT_EQ(unknown.rawIndex(), 0u);
    ASSERT_EQ(design_.consts().size(), 0u);
    ObjId placeholder;
    ModuleBuilder b = build(40, [&](ModuleBuilder& mb, int width) {
      placeholder = mb.addPlaceholder(width);
      return placeholder;
    });
    b.drivePlaceholder(placeholder, b.addConst(other.get(unknown), other));
    EXPECT_THAT(error(std::move(b)), HasSubstr("ConstId 0 is not in the pool"));
  }

  const RegClass value_only = regClass(
      {RegPinType::kData, RegPinType::kClock, RegPinType::kSyncResetValue});
  EXPECT_THAT(error(registerModule(4,
                                   value_only,
                                   [this](ModuleBuilder& b, ObjId) {
                                     return std::vector<ObjId>{
                                         b.addPi(pn(design_), 4),
                                         b.addPi(pn(design_), 1),
                                         b.addConst(4, 0b0110)};
                                   })),
              HasSubstr("without its reset"));
}

TEST_F(WellFormedTest, ChecksInstances)
{
  ModuleBuilder child(design_.names().intern("inv"));
  child.addPo(pn(design_),
              child.add(Op::kNot, 4, {child.addPi(pn(design_), 4)}));
  const ModId inv = addModuleOrFail(design_, std::move(child));
  ASSERT_TRUE(inv.isValid());

  {
    ModuleBuilder b;
    b.add(Op::kInst, 0, {b.addPi(pn(design_), 3)}, inv.value());
    EXPECT_THAT(error(std::move(b)),
                HasSubstr("(inst): fanin 0 has width 3, expected 4"));
  }
  {
    ModuleBuilder b;
    const ObjId inst
        = b.add(Op::kInst, 0, {b.addPi(pn(design_), 4)}, inv.value());
    b.addPo(pn(design_), b.add(Op::kFan, 4, {inst}, 1));
    EXPECT_THAT(error(std::move(b)), HasSubstr("output 1 does not exist"));
  }
  {
    ModuleBuilder b;
    const ObjId inst
        = b.add(Op::kInst, 0, {b.addPi(pn(design_), 4)}, inv.value());
    b.addPo(pn(design_), b.add(Op::kNot, 4, {inst}));
    EXPECT_THAT(error(std::move(b)), HasSubstr("read it through a Fan"));
  }
  {
    ModuleBuilder b;
    b.add(Op::kInst, 0, {}, 77);
    EXPECT_THAT(error(std::move(b)), HasSubstr("ModId 77 does not exist"));
  }
}

TEST(WellFormedDesignTest, SelfAndMutualInstantiation)
{
  {
    Design design(kCheckWellFormed);
    Design::SingleWriteScope write(design);
    ModuleBuilder self(design.names().intern("self"));
    self.add(Op::kInst, 0, {}, 0);  // it will be ModId 0
    ASSERT_TRUE(design.addModule(*std::move(self).finish()).ok());
    const absl::Status status = checkWellFormed(design);
    EXPECT_THAT(status.message(),
                HasSubstr("module 'self': object 0 (inst): instantiates its "
                          "own module"));
  }
  {
    Design design(kCheckWellFormed);
    Design::SingleWriteScope write(design);
    ModuleBuilder a(design.names().intern("a"));
    a.add(Op::kInst, 0, {}, 1);
    ModuleBuilder b(design.names().intern("b"));
    b.add(Op::kInst, 0, {}, 0);
    ASSERT_TRUE(design.addModule(*std::move(a).finish()).ok());
    ASSERT_TRUE(design.addModule(*std::move(b).finish()).ok());
    EXPECT_THAT(checkWellFormed(design).message(),
                HasSubstr("instance hierarchy has a cycle"));
  }
}

TEST(WellFormedDesignTest, WorksInReadOnlyMode)
{
  Design design(kCheckWellFormed);
  {
    Design::SingleWriteScope write(design);
    ModuleBuilder b(design.names().intern("m"));
    b.addPo(pn(design), b.add(Op::kNot, 1, {b.addPi(pn(design), 1)}));
    ASSERT_TRUE(design.addModule(*std::move(b).finish()).ok());
  }
  ASSERT_TRUE(design.isReadOnly());
  EXPECT_TRUE(checkWellFormed(design).ok());
}

TEST_F(WellFormedTest, ReportsUnknownPortNameId)
{
  ModuleBuilder b(design_.names().intern("m"));
  const ObjId a = b.addPi(NameId(999999), 1);
  b.addPo(pn(design_), a);
  const absl::StatusOr<Module> m = std::move(b).finish();
  ASSERT_TRUE(m.ok()) << m.status();
  EXPECT_THAT(checkWellFormed(design_, *m).message(),
              HasSubstr("has an unknown name id 999999"));
}

// The Liberty cells of the cell and mapped register tests.
TEST_F(WellFormedCellTest, ChecksCombinationalCells)
{
  // Valid combinational cell: output 0 is the kCell itself, output 1 is a Fan.
  {
    ModuleBuilder b;
    const ObjId a = b.addPi(pn(design_), 1);
    const ObjId bi = b.addPi(pn(design_), 1);
    const ObjId cell = b.add(Op::kCell, 1, {a, bi}, ha_id_.rawIndex());
    b.addPo(pn(design_), cell);
    b.addPo(pn(design_), b.add(Op::kFan, 1, {cell}, 1));
    EXPECT_TRUE(check(std::move(b)).ok());
  }

  // Combinational cell errors.
  {
    ModuleBuilder b;
    b.add(Op::kCell, 1, {}, 999);
    EXPECT_THAT(error(std::move(b)),
                HasSubstr("LibCellId 999 is not in the table"));
  }
  // A combinational cell without outputs.
  {
    const LibCellId sink_id
        = design_.cells().intern(lib_.combinational("SINK", {"A"}, {}));
    ModuleBuilder b;
    b.add(Op::kCell, 1, {b.addPi(pn(design_), 1)}, sink_id.rawIndex());
    EXPECT_THAT(error(std::move(b)), HasSubstr("cell 'SINK' has no outputs"));
  }
  {
    ModuleBuilder b;
    b.add(Op::kCell,
          1,
          {b.addPi(pn(design_), 1), b.addPi(pn(design_), 1)},
          dff_id_.rawIndex());
    EXPECT_THAT(error(std::move(b)),
                HasSubstr("is sequential; use a mapped RegOut/RegIn pair"));
  }
  // A clock gate is a combinational kCell.
  {
    ModuleBuilder b;
    const ObjId gclk = b.add(Op::kCell,
                             1,
                             {b.addPi(pn(design_), 1), b.addPi(pn(design_), 1)},
                             icg_id_.rawIndex());
    b.addPo(pn(design_), gclk);
    EXPECT_TRUE(check(std::move(b)).ok());
  }
  // A stateful cell with an input-dependent output is neither a kCell nor a
  // register.
  {
    ModuleBuilder b;
    b.add(Op::kCell,
          1,
          {
              b.addPi(pn(design_), 1),
              b.addPi(pn(design_), 1),
              b.addPi(pn(design_), 1),
          },
          tdff_id_.rawIndex());
    EXPECT_THAT(error(std::move(b)),
                HasSubstr("instantiate it as a black box"));
  }
  {
    ModuleBuilder b;
    b.add(Op::kCell,
          2,
          {b.addPi(pn(design_), 1), b.addPi(pn(design_), 1)},
          ha_id_.rawIndex());
    EXPECT_THAT(error(std::move(b)), HasSubstr("has width 2, expected 1"));
  }
  {
    ModuleBuilder b;
    b.add(Op::kCell, 1, {b.addPi(pn(design_), 1)}, ha_id_.rawIndex());
    EXPECT_THAT(error(std::move(b)), HasSubstr("has 1 fanins, expected 2"));
  }
  {
    ModuleBuilder b;
    b.add(Op::kCell,
          1,
          {b.addPi(pn(design_), 2), b.addPi(pn(design_), 1)},
          ha_id_.rawIndex());
    EXPECT_THAT(error(std::move(b)),
                HasSubstr("fanin 0 has width 2, expected 1"));
  }
  {
    ModuleBuilder b;
    const ObjId cell = b.add(Op::kCell,
                             1,
                             {b.addPi(pn(design_), 1), b.addPi(pn(design_), 1)},
                             ha_id_.rawIndex());
    b.addPo(pn(design_), b.add(Op::kFan, 1, {cell}, 0));
    EXPECT_THAT(error(std::move(b)),
                HasSubstr("cell output 0 is out of range for 'HA'"));
  }
  {
    ModuleBuilder b;
    const ObjId cell = b.add(Op::kCell,
                             1,
                             {b.addPi(pn(design_), 1), b.addPi(pn(design_), 1)},
                             ha_id_.rawIndex());
    b.addPo(pn(design_), b.add(Op::kFan, 1, {cell}, 2));
    EXPECT_THAT(error(std::move(b)),
                HasSubstr("cell output 2 is out of range for 'HA'"));
  }
}

TEST_F(WellFormedCellTest, ChecksMappedRegisters)
{
  // A mapped register bound to a combinational cell.
  {
    ModuleBuilder b;
    const RegClassId rc = b.regClasses().intern(RegClass::mapped(ha_id_));
    const ObjId q = b.addRegisterOut(1, rc);
    b.addRegisterIn(q, {b.addPi(pn(design_), 1), b.addPi(pn(design_), 1)});
    b.addPo(pn(design_), q);
    EXPECT_THAT(error(std::move(b)),
                HasSubstr("mapped register cell 'HA' is a combinational cell, "
                          "not a register"));
  }

  // Valid mapped register with QN Fan output.
  {
    ModuleBuilder b;
    const RegClassId rc = b.regClasses().intern(RegClass::mapped(dff_id_));
    const ObjId q = b.addRegisterOut(1, rc);
    b.addRegisterIn(q, {b.addPi(pn(design_), 1), b.addPi(pn(design_), 1)});
    b.addPo(pn(design_), q);
    b.addPo(pn(design_), b.add(Op::kFan, 1, {q}, 1));
    EXPECT_TRUE(check(std::move(b)).ok());
  }

  // Mapped register errors.
  EXPECT_THAT(
      error(registerModule(1,
                           RegClass::mapped(ha_id_),
                           [this](ModuleBuilder& b, ObjId) {
                             return std::vector<ObjId>{b.addPi(pn(design_), 1),
                                                       b.addPi(pn(design_), 1)};
                           })),
      HasSubstr("mapped register cell 'HA' is a combinational cell, not a "
                "register"));
  EXPECT_THAT(error(registerModule(1,
                                   RegClass::mapped(tdff_id_),
                                   [this](ModuleBuilder& b, ObjId) {
                                     return std::vector<ObjId>{
                                         b.addPi(pn(design_), 1),
                                         b.addPi(pn(design_), 1),
                                         b.addPi(pn(design_), 1)};
                                   })),
              HasSubstr("'TDFF' is a black_box cell, not a register"));
  EXPECT_THAT(error(registerModule(1,
                                   RegClass::mapped(icg_id_),
                                   [this](ModuleBuilder& b, ObjId) {
                                     return std::vector<ObjId>{
                                         b.addPi(pn(design_), 1),
                                         b.addPi(pn(design_), 1)};
                                   })),
              HasSubstr("'ICG' is a clock_gate cell, not a register"));
  {
    ModuleBuilder b;
    const RegClassId rc = b.regClasses().intern(RegClass::mapped(dff_id_));
    const ObjId q = b.addRegisterOut(1, rc);
    b.addRegisterIn(q, {b.addPi(pn(design_), 1), b.addPi(pn(design_), 1)});
    b.addPo(pn(design_), b.add(Op::kFan, 1, {q}, 0));
    EXPECT_THAT(error(std::move(b)),
                HasSubstr("output 0 is the register itself"));
  }
}

// Every output pin of a Liberty cell has exactly one kFan, read or not.
TEST_F(WellFormedCellTest, RequiresOneFanPerCellOutput)
{
  {
    ModuleBuilder b;
    const ObjId cell = b.add(Op::kCell,
                             1,
                             {b.addPi(pn(design_), 1), b.addPi(pn(design_), 1)},
                             ha_id_.rawIndex());
    b.addPo(pn(design_), cell);  // CO has no kFan
    EXPECT_THAT(error(std::move(b)),
                HasSubstr("cell 'HA' has no Fan for output 1"));
  }
  {
    ModuleBuilder b;
    const ObjId cell = b.add(Op::kCell,
                             1,
                             {b.addPi(pn(design_), 1), b.addPi(pn(design_), 1)},
                             ha_id_.rawIndex());
    b.add(Op::kFan, 1, {cell}, 1);  // unread: fine
    b.addPo(pn(design_), cell);
    EXPECT_TRUE(check(std::move(b)).ok());
  }
  {
    ModuleBuilder b;
    const ObjId cell = b.add(Op::kCell,
                             1,
                             {b.addPi(pn(design_), 1), b.addPi(pn(design_), 1)},
                             ha_id_.rawIndex());
    b.addPo(pn(design_), b.add(Op::kFan, 1, {cell}, 1));
    b.addPo(pn(design_), b.add(Op::kFan, 1, {cell}, 1));
    EXPECT_THAT(error(std::move(b)),
                HasSubstr("already has a Fan for output 1"));
  }
  {
    ModuleBuilder b;
    const RegClassId rc = b.regClasses().intern(RegClass::mapped(dff_id_));
    const ObjId q = b.addRegisterOut(1, rc);
    b.addRegisterIn(q, {b.addPi(pn(design_), 1), b.addPi(pn(design_), 1)});
    b.addPo(pn(design_), q);  // QN has no kFan
    EXPECT_THAT(error(std::move(b)),
                HasSubstr("cell 'DFF' has no Fan for output 1"));
  }
}

// A file name id that the design never interned.
constexpr uint32_t kUnknownFile = 12345;

TEST_F(WellFormedTest, ReportsUnknownSourceFiles)
{
  {
    ModuleBuilder b(design_.names().intern("m"));
    b.setModuleSourceLoc(SourceLoc(NameId(kUnknownFile), 3));
    b.addPo(pn(design_), b.addPi(pn(design_), 1));
    EXPECT_THAT(error(std::move(b)),
                HasSubstr("module source location has an unknown file name "
                          "id 12345"));
  }
  {
    ModuleBuilder b(design_.names().intern("m"));
    const ObjId y = b.add(Op::kNot, 1, {b.addPi(pn(design_), 1)});
    b.setObjSourceLoc(y, SourceLoc(NameId(kUnknownFile), 3));
    b.addPo(pn(design_), y);
    EXPECT_THAT(error(std::move(b)),
                HasSubstr("source location has an unknown file name id 12345"));
  }
}

TEST_F(WellFormedTest, ReportsAttributeAndFlagMisuse)
{
  {
    ModuleBuilder b;
    const ObjId y = b.add(Op::kNot, 1, {b.addPi(pn(design_), 1)});
    b.addObjAttributes(y,
                       ObjAttributes().setBoundaryOpt(BoundaryOpt::kConstOnly));
    b.addPo(pn(design_), y);
    EXPECT_THAT(error(std::move(b)),
                HasSubstr("boundary restriction on an object that is not an "
                          "Inst"));
  }
  {
    ModuleBuilder b;
    const ObjId y = b.add(Op::kNot, 1, {b.addPi(pn(design_), 1)});
    b.addObjAttributes(y, ObjAttributes().setKeepHierarchy());
    b.addPo(pn(design_), y);
    EXPECT_THAT(error(std::move(b)),
                HasSubstr("keep_hierarchy attribute on an object that is not "
                          "an Inst"));
  }
  {
    ModuleBuilder child(design_.names().intern("child"));
    child.addPo(pn(design_), child.addPi(pn(design_), 1));
    const ModId child_id = addModuleOrFail(design_, std::move(child));
    ASSERT_TRUE(child_id.isValid());
    ModuleBuilder b;
    b.add(Op::kInst, 1, {b.addPi(pn(design_), 1)}, child_id.value());
    EXPECT_THAT(error(std::move(b)),
                HasSubstr("an instance must have width 0"));
  }
}

TEST_F(WellFormedTest, ReportsOperatorShapeErrors)
{
  {
    ModuleBuilder b;
    b.addPo(pn(design_),
            b.add(Op::kReplicate, 4, {b.addPi(pn(design_), 4)}, 0));
    EXPECT_THAT(error(std::move(b)), HasSubstr("replication count 0"));
  }
  {
    ModuleBuilder b;
    const ObjId a = b.addPi(pn(design_), 4);
    b.addPo(pn(design_), b.add(Op::kConcat, 9, {a, a}));
    EXPECT_THAT(error(std::move(b)),
                HasSubstr("fanins add up to 8 bits, expected 9"));
  }
  for (const Op op : {Op::kBmux, Op::kDemux}) {
    SCOPED_TRACE(opName(op));
    ModuleBuilder b;
    const ObjId sel = b.addPi(pn(design_), 32);
    b.addPo(pn(design_), b.add(op, 1, {sel, b.addPi(pn(design_), 1)}));
    EXPECT_THAT(error(std::move(b)),
                HasSubstr("select of 32 bits is too wide"));
  }
  {
    // A Po is a sink: reading it is an error.
    ModuleBuilder b;
    const ObjId po = b.addPo(pn(design_), b.addPi(pn(design_), 1));
    b.addPo(pn(design_), b.add(Op::kNot, 1, {po}));
    EXPECT_THAT(error(std::move(b)), HasSubstr("does not carry a value"));
  }
}

TEST_F(WellFormedTest, ReportsUnknownPoolReferences)
{
  // A pooled constant whose ConstId belongs to another pool.
  const ConstId forty = design_.consts().internUint(40, 1);
  ASSERT_EQ(forty.rawIndex(), 0u);
  ConstPool other;
  other.setMode(TableMode::kSingleWrite);
  const ConstId wide48 = other.internUint(48, 1);  // index 0, 48 bits
  ConstId far;
  for (int i = 0; i < 4; ++i) {
    far = other.internUint(48, uint64_t{100} + static_cast<uint64_t>(i));
  }
  {
    ModuleBuilder b;
    b.addPo(pn(design_), b.addConst(other.get(wide48), other));
    EXPECT_THAT(error(std::move(b)), HasSubstr("pooled value has width 40"));
  }
  {
    ModuleBuilder b;
    b.addPo(pn(design_), b.addConst(other.get(far), other));
    EXPECT_THAT(error(std::move(b)), HasSubstr("ConstId 4 is not in the pool"));
  }

  // Truth tables.
  {
    ModuleBuilder b;
    b.addPo(pn(design_), b.add(Op::kLut, 1, {}, forty.rawIndex()));
    EXPECT_THAT(error(std::move(b)),
                HasSubstr("has 0 inputs, expected 1 to 16"));
  }
  {
    ModuleBuilder b;
    b.addPo(pn(design_), b.add(Op::kLut, 1, {b.addPi(pn(design_), 1)}, 999));
    EXPECT_THAT(error(std::move(b)),
                HasSubstr("truth table ConstId 999 is not in the pool"));
  }
  {
    const ConstId eight = design_.consts().internUint(8, 0x80);
    ModuleBuilder b;
    const ObjId s = b.addPi(pn(design_), 1);
    b.addPo(pn(design_), b.add(Op::kLut, 1, {s, s}, eight.rawIndex()));
    EXPECT_THAT(error(std::move(b)),
                HasSubstr("truth table has 8 bits, expected 4"));
  }

  // Register initial state.
  {
    const RegClass rc = regClass({RegPinType::kData, RegPinType::kClock},
                                 RegFlags(),
                                 RegInit::pooled(ConstId(999)));
    EXPECT_THAT(error(registerModule(40,
                                     rc,
                                     [&](ModuleBuilder& b, ObjId) {
                                       return std::vector<ObjId>{
                                           b.addPi(pn(design_), 40),
                                           b.addPi(pn(design_), 1)};
                                     })),
                HasSubstr("initial state ConstId 999 is not in the pool"));
  }
}

// With 64-bit object ids, an inline kConst's bits and x_mask fields are 64
// bits wide; bits above 32 are above every inline width.
TEST_F(WellFormedTest, ReportsInlineConstBitsAbove32)
{
  if (kObjIdBits != 64) {
    GTEST_SKIP() << "the record fields are 32 bits wide in this build";
  }
  for (const bool corrupt_x_mask : {false, true}) {
    ModuleBuilder b;
    const ObjId c = b.addConst(8, 0x5);
    b.addPo(pn(design_), c);
    absl::StatusOr<Module> module = std::move(b).finish();
    ASSERT_TRUE(module.ok()) << module.status();
    ASSERT_TRUE(module->isInlineConst(c));
    // No API writes these bits; the module itself is not const.
    ObjRecord& r = const_cast<ObjRecord&>(module->record(c));
    ObjIndex& field = corrupt_x_mask ? r.fanin_begin : r.data;
    field = static_cast<ObjIndex>(field | (uint64_t{1} << 40u));
    const absl::Status status = checkWellFormed(design_, *module);
    EXPECT_THAT(status.message(),
                HasSubstr("inline value has bits above its width"))
        << (corrupt_x_mask ? "x_mask" : "bits");
  }
}

TEST_F(WellFormedTest, ReportsRegisterClassErrors)
{
  auto pins = [&](const std::vector<int>& widths) {
    return [this, widths](ModuleBuilder& b, ObjId) {
      std::vector<ObjId> out;
      out.reserve(widths.size());
      for (const int w : widths) {
        out.push_back(b.addPi(pn(design_), w));
      }
      return out;
    };
  };
  {
    const RegClass rc = RegClass::generic(
        RegFlags(),
        static_cast<RegPinMask>(uint32_t{regPinBit(RegPinType::kData)}
                                | regPinBit(RegPinType::kClock)
                                | regPinBit(RegPinType::kNumTypes)));
    EXPECT_THAT(error(registerModule(4, rc, pins({4, 1, 1}))),
                HasSubstr("RegClass has unknown pins"));
  }
  {
    RegFlags flags;
    flags.reserved = 1;
    EXPECT_THAT(error(registerModule(
                    4,
                    regClass({RegPinType::kData, RegPinType::kClock}, flags),
                    pins({4, 1}))),
                HasSubstr("RegClass has reserved flags set"));
  }
  EXPECT_THAT(error(registerModule(4,
                                   regClass({RegPinType::kData,
                                             RegPinType::kClock,
                                             RegPinType::kAsyncLoad}),
                                   pins({4, 1, 1}))),
              HasSubstr("async load and async data must come together"));
  EXPECT_THAT(error(registerModule(4,
                                   regClass({RegPinType::kData,
                                             RegPinType::kClock,
                                             RegPinType::kScanEnable}),
                                   pins({4, 1, 1}))),
              HasSubstr("scan enable and scan in must come together"));
}

// checkOperatorInterface() against a spec whose operands do not match.
TEST_F(WellFormedTest, ReportsOperatorInterfaceErrors)
{
  ModuleBuilder b(design_.names().intern("impl"));
  const ObjId a = b.addPi(pn(design_), 8);
  b.addPi(pn(design_), 4);
  b.addPo(pn(design_), a);
  absl::StatusOr<Module> m = std::move(b).finish();
  ASSERT_TRUE(m.ok()) << m.status();

  OpSpec spec;
  spec.op = Op::kAdd;
  spec.result_width = 8;
  spec.operands = {OpOperand{.width = 8}, OpOperand{.width = 8}};
  EXPECT_THAT(checkOperatorInterface(design_, *m, spec).message(),
              HasSubstr("operator implementation: input 1 has width 4, its "
                        "operand 1 has width 8"));

  spec.operands = {
      OpOperand{.width = 8},
      OpOperand{.width = 4},
      OpOperand{.width = 8, .constant = ConstId(999)},
  };
  EXPECT_THAT(checkOperatorInterface(design_, *m, spec).message(),
              HasSubstr("operator spec: constant operand 2 is not in the "
                        "pool"));
}

}  // namespace
}  // namespace syn::test
