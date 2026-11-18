// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include <array>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <iterator>
#include <span>
#include <string>
#include <utility>
#include <vector>

#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "helpers/TestModules.h"
#include "syn/lir/ConstPool.h"
#include "syn/lir/ConstView.h"
#include "syn/lir/Design.h"
#include "syn/lir/Ids.h"
#include "syn/lir/Module.h"
#include "syn/lir/ModuleBuilder.h"
#include "syn/lir/Op.h"
#include "syn/lir/RegClassPool.h"
#include "syn/lir/RegPinTypes.h"
#include "syn/lir/Traversal.h"
#include "syn/lir/WellFormed.h"

namespace syn::test {
namespace {

using ::syn::lir::acyclicTopologicalOrder;
using ::syn::lir::checkWellFormed;
using ::syn::lir::ConstId;
using ::syn::lir::ConstPool;
using ::syn::lir::ConstView;
using ::syn::lir::Design;
using ::syn::lir::IdRemap;
using ::syn::lir::kFaninCountEscape;
using ::syn::lir::kObjIdBits;
using ::syn::lir::LogicValue;
using ::syn::lir::loopPairDriver;
using ::syn::lir::ModId;
using ::syn::lir::Module;
using ::syn::lir::ModuleAttributes;
using ::syn::lir::ModuleBuilder;
using ::syn::lir::NameId;
using ::syn::lir::ObjId;
using ::syn::lir::ObjIndex;
using ::syn::lir::ObjRecord;
using ::syn::lir::Op;
using ::syn::lir::opName;
using ::syn::lir::RegClass;
using ::syn::lir::RegClassId;
using ::syn::lir::RegFlags;
using ::syn::lir::RegInit;
using ::syn::lir::RegOutput;
using ::syn::lir::regPinBit;
using ::syn::lir::RegPinType;
using ::syn::lir::Signedness;
using ::syn::lir::TableMode;

// A fresh port name id: every call returns a new one. The builder only
// checks that port names are valid and distinct.
NameId pn()
{
  static uint32_t next = 1000;
  return NameId(next++);
}

using ::testing::ElementsAre;
using ::testing::HasSubstr;

Module finishOrDie(ModuleBuilder&& builder)
{
  absl::StatusOr<Module> module = std::move(builder).finish();
  EXPECT_TRUE(module.ok()) << module.status();
  return module.ok() ? *std::move(module) : Module();
}

std::string finishError(ModuleBuilder&& builder)
{
  absl::StatusOr<Module> module = std::move(builder).finish();
  EXPECT_FALSE(module.ok());
  return module.ok() ? "" : std::string(module.status().message());
}

// The object capacities of a module and of its builder follow the id width.
TEST(ModuleBuilderTest, CapacityFollowsObjectIdWidth)
{
  EXPECT_EQ(sizeof(ObjRecord), kObjIdBits == 64 ? 24u : 16u);
  EXPECT_EQ(Module::kMaxObjects, ObjId::kInvalidValue);
  EXPECT_EQ(ModuleBuilder::kMaxObjects, (ObjIndex{1} << (kObjIdBits - 1u)) - 1);
}

TEST(ModuleBuilderTest, EmptyModule)
{
  ModuleBuilder builder(NameId(7));
  const Module module = finishOrDie(std::move(builder));
  EXPECT_EQ(module.size(), 0u);
  EXPECT_EQ(module.name(), NameId(7));
  EXPECT_TRUE(module.isOrdered());
  EXPECT_EQ(module.revision(), 1);
}

TEST(ModuleBuilderTest, CombinationalModule)
{
  ModuleBuilder builder;
  const ObjId a = builder.addPi(pn(), 8);
  const ObjId b = builder.addPi(pn(), 8, Signedness::kSigned);
  const ObjId sum = builder.add(Op::kAdd, 8, {a, b});
  const ObjId lt = builder.add(Op::kLt, 1, {a, b}, 0, Signedness::kSigned);
  const ObjId y = builder.addPo(pn(), sum);
  const ObjId z = builder.addPo(pn(), lt);
  builder.setModuleAttributes({.dont_touch = true});
  const Module module = finishOrDie(std::move(builder));

  EXPECT_EQ(module.size(), 6u);
  EXPECT_THAT(module.inputs(), ElementsAre(a, b));
  EXPECT_THAT(module.outputs(), ElementsAre(y, z));
  EXPECT_EQ(module.portIndex(a), 0);
  EXPECT_EQ(module.portIndex(b), 1);
  EXPECT_EQ(module.portIndex(z), 1);
  EXPECT_TRUE(module.isSigned(b));
  EXPECT_FALSE(module.isSigned(sum));
  EXPECT_TRUE(module.isSigned(lt));
  EXPECT_EQ(module.op(sum), Op::kAdd);
  EXPECT_EQ(module.width(sum), 8);
  EXPECT_EQ(module.width(lt), 1);
  EXPECT_EQ(module.width(z), 1);
  EXPECT_THAT(module.fanins(sum), ElementsAre(a, b));
  EXPECT_EQ(module.fanin(y, 0), sum);
  EXPECT_EQ(module.faninCount(a), 0);
  EXPECT_FALSE(module.isDead(sum));
  EXPECT_TRUE(module.attributes() == ModuleAttributes{.dont_touch = true});
  EXPECT_TRUE(module.isOrdered());  // computed by finish()

  std::vector<ObjId> ids;
  for (const ObjId id : module.objects()) {
    ids.push_back(id);
  }
  EXPECT_THAT(ids, ElementsAre(a, b, sum, lt, y, z));
  EXPECT_GE(module.memoryBytes(), 6 * sizeof(ObjRecord));
}

TEST(ModuleBuilderTest, SignedFlagIsKeptOnlyWhereItHasMeaning)
{
  ModuleBuilder builder;
  const ObjId a = builder.addPi(pn(), 8, Signedness::kSigned);
  const ObjId b = builder.addPi(pn(), 8);
  const ObjId sum = builder.add(Op::kAdd, 8, {a, b}, 0, Signedness::kSigned);
  const ObjId conj = builder.add(Op::kAnd, 8, {a, b}, 0, Signedness::kSigned);
  const ObjId wide = builder.add(Op::kCast, 16, {a}, 0, Signedness::kSigned);
  const ObjId shr = builder.add(Op::kShr, 8, {a, b}, 0, Signedness::kSigned);
  builder.addPo(pn(), sum);
  builder.addPo(pn(), conj);
  builder.addPo(pn(), wide);
  builder.addPo(pn(), shr);
  const Module module = finishOrDie(std::move(builder));

  EXPECT_TRUE(module.isSigned(a));      // declared signedness of the port
  EXPECT_FALSE(module.isSigned(sum));   // Add is the same either way
  EXPECT_FALSE(module.isSigned(conj));  // so is And
  EXPECT_TRUE(module.isSigned(wide));   // sign extension
  EXPECT_TRUE(module.isSigned(shr));    // arithmetic shift
}

TEST(ModuleBuilderTest, PlaceholderForwardReference)
{
  ModuleBuilder builder;
  const ObjId p = builder.addPlaceholder(4);
  EXPECT_TRUE(ModuleBuilder::isPlaceholder(p));
  EXPECT_EQ(builder.width(p), 4);
  const ObjId n = builder.add(Op::kNot, 4, {p});
  const ObjId po = builder.addPo(pn(), n);
  const ObjId x = builder.addPi(pn(), 4);
  builder.drivePlaceholder(p, x);
  const Module module = finishOrDie(std::move(builder));

  EXPECT_EQ(module.fanin(n, 0), x);
  EXPECT_EQ(module.fanin(po, 0), n);
  EXPECT_FALSE(module.isOrdered());  // x comes after its reader
}

TEST(ModuleBuilderTest, PlaceholderChain)
{
  ModuleBuilder builder;
  const ObjId p1 = builder.addPlaceholder(2);
  const ObjId p2 = builder.addPlaceholder(2);
  const ObjId po = builder.addPo(pn(), p1);
  builder.drivePlaceholder(p1, p2);
  const ObjId x = builder.addPi(pn(), 2);
  builder.drivePlaceholder(p2, x);
  const Module module = finishOrDie(std::move(builder));
  EXPECT_EQ(module.fanin(po, 0), x);
}

TEST(ModuleBuilderTest, RegisterPairIsLinkedAndOrdered)
{
  ModuleBuilder builder;
  const ObjId clk = builder.addPi(pn(), 1);
  const ObjId q = builder.addRegisterOut(4, RegClassId(3));
  const ObjId one = builder.addConst(4, 1);
  const ObjId next = builder.add(Op::kAdd, 4, {q, one});
  const ObjId d = builder.addRegisterIn(q, {next, clk});
  builder.addPo(pn(), q);
  const Module module = finishOrDie(std::move(builder));

  EXPECT_EQ(module.op(q), Op::kRegOut);
  EXPECT_EQ(module.op(d), Op::kRegIn);
  EXPECT_EQ(module.stateIn(q), d);
  EXPECT_EQ(module.stateOut(d), q);
  EXPECT_EQ(module.regClassId(q), RegClassId(3));
  EXPECT_EQ(module.width(d), 4);
  EXPECT_THAT(module.fanins(d), ElementsAre(next, clk));
  // The RegOut -> RegIn link points forward, but it is not a fanin, so it
  // does not break the order.
  EXPECT_EQ(module.faninCount(q), 0);
  EXPECT_TRUE(module.fanins(q).empty());
  EXPECT_TRUE(module.isOrdered());
}

TEST(ModuleBuilderTest, EscapedFaninCount)
{
  constexpr int kBits = 70000;
  ModuleBuilder builder;
  const ObjId bit = builder.addPi(pn(), 1);
  const std::vector<ObjId> bits(kBits, bit);
  const ObjId wide = builder.add(Op::kConcat, kBits, bits);
  const ObjId after = builder.add(Op::kNot, 1, {bit});
  const Module module = finishOrDie(std::move(builder));

  EXPECT_EQ(module.record(wide).fanin_count, kFaninCountEscape);
  EXPECT_EQ(module.faninCount(wide), kBits);
  EXPECT_EQ(std::ssize(module.fanins(wide)), kBits);
  EXPECT_EQ(module.fanin(wide, kBits - 1), bit);
  EXPECT_THAT(module.fanins(after), ElementsAre(bit));
  EXPECT_EQ(module.faninArenaSize(),
            static_cast<size_t>(kBits) + 2);  // header + fanins + 1
}

TEST(ModuleBuilderTest, SmallConstantsAreInline)
{
  ModuleBuilder b;
  const ObjId byte = b.addConst(8, 0x1A5);         // bit 8 is dropped
  const ObjId x1 = b.addConst(4, 0b1001, 0b0010);  // 4'b10x1
  const ObjId all_x
      = b.addConst(32, 0, 0xFFFFFFFF);  // x_mask fills fanin_begin
  // A placeholder makes finish() rewrite fanins across every record.
  const ObjId p = b.addPlaceholder(32);
  const ObjId inv = b.add(Op::kNot, 32, {p});
  b.drivePlaceholder(p, all_x);
  const Module m = finishOrDie(std::move(b));

  const ConstPool pool;  // never used by inline constants
  for (const ObjId k : {byte, x1, all_x}) {
    EXPECT_EQ(m.op(k), Op::kConst);
    EXPECT_TRUE(m.isInlineConst(k));
    EXPECT_EQ(m.faninCount(k), 0);
    EXPECT_TRUE(m.fanins(k).empty());
  }
  EXPECT_EQ(m.constValue(byte, pool).toUint64(), uint64_t{0xA5});
  EXPECT_EQ(m.constValue(x1, pool).toString(), "4'b10x1");
  EXPECT_TRUE(m.constValue(all_x, pool).isFill(LogicValue::kX));
  EXPECT_THAT(m.fanins(inv), ElementsAre(all_x));
  EXPECT_TRUE(m.isOrdered());
  EXPECT_EQ(pool.size(), 0u);
}

TEST(ModuleBuilderTest, AddConstFromThePoolPicksInlineOrPooled)
{
  ConstPool pool;
  pool.setMode(TableMode::kSingleWrite);
  const ConstId narrow_id = pool.internUint(16, 7);
  const ConstId wide_id = pool.internFill(100, LogicValue::kX);
  pool.setMode(TableMode::kReadOnly);  // adding existing values is a lookup

  ModuleBuilder b;
  const ObjId narrow = b.addConst(pool.get(narrow_id), pool);
  const ObjId wide = b.addConst(pool.get(wide_id), pool);
  const Module m = finishOrDie(std::move(b));

  EXPECT_TRUE(m.isInlineConst(narrow));
  EXPECT_EQ(m.constValue(narrow, pool).toUint64(), uint64_t{7});
  EXPECT_FALSE(m.isInlineConst(wide));
  EXPECT_EQ(m.data(wide), wide_id.rawIndex());
  EXPECT_EQ(m.constValue(wide, pool).width(), 100);
  EXPECT_TRUE(m.constValue(wide, pool).isFill(LogicValue::kX));
  EXPECT_EQ(pool.size(), 2u);
}

// With the pools read-only, a rebuild that needs a new pooled constant
// (directly or as a register's initial state) fails with FailedPrecondition
// and leaves the module unchanged, in every build mode.
TEST(ModuleBuilderTest, NewPooledValueInAReadOnlyPoolFails)
{
  Design design;
  const Design::SingleWriteScope write(design);
  ModId id;
  ObjId q;
  {
    ModuleBuilder b(design.names().intern("m"));
    const ObjId clk = b.addPi(design.names().intern("clk"), 1);
    const ObjId a = b.addPi(design.names().intern("a"), 64);
    q = b.addRegisterOut(
        64,
        b.regClasses().intern(RegClass::generic(
            RegFlags(),
            regPinBit(RegPinType::kData) | regPinBit(RegPinType::kClock))));
    b.addRegisterIn(q, {a, clk});
    b.addPo(design.names().intern("y"), q);
    id = *design.addModule(finishOrDie(std::move(b)));
  }
  const Module& m = design.module(id);
  const int64_t revision = m.revision();
  const ObjIndex size = m.size();

  const Design::PoolsReadOnlyScope read_only(design);
  const std::array<uint64_t, 2> words = {0x123456789, 0};  // bits, x_mask
  const ConstView fresh(64, words.data());
  const auto withFreshInit = [&](ModuleBuilder& b) {
    return b.regClasses().intern(
        b.regClasses()
            .get(m.regClassId(q))
            .withInit(RegInit::fromConst(fresh, design.consts())));
  };
  const std::vector<std::function<void(ModuleBuilder&)>> edits = {
      [&](ModuleBuilder& b) {
        EXPECT_FALSE(b.addConst(fresh, design.consts()).isValid());
      },
      [&](ModuleBuilder& b) {
        EXPECT_FALSE(b.addRegisterOut(64, withFreshInit(b)).isValid());
      },
      [&](ModuleBuilder& b) {
        const ObjId copy = b.copy(q, std::span<const ObjId>());
        b.setRegisterClass(copy, withFreshInit(b));
      },
  };
  for (const auto& edit : edits) {
    ModuleBuilder b(m);
    edit(b);
    EXPECT_FALSE(b.ok());
    EXPECT_FALSE(b.add(Op::kNot, 1, {b.input(0)}).isValid());  // a no-op
    const absl::StatusOr<IdRemap> remap = std::move(b).install(design, id);
    EXPECT_EQ(remap.status().code(), absl::StatusCode::kFailedPrecondition);
    EXPECT_THAT(remap.status().message(),
                HasSubstr("constant pool is read-only"));
  }
  EXPECT_EQ(m.revision(), revision);
  EXPECT_EQ(m.size(), size);
  EXPECT_EQ(design.consts().size(), 0u);
}

TEST(ModuleBuilderTest, InlineConstantsStopAt32Bits)
{
  ModuleBuilder b;
  EXPECT_TRUE(b.addConst(32, 0xFFFFFFFF).isValid());
  b.addConst(33, 1);
  EXPECT_THAT(finishError(std::move(b)), HasSubstr("pass a ConstPool"));
}

TEST(ModuleBuilderTest, RejectsForwardReferenceWithoutPlaceholder)
{
  ModuleBuilder builder;
  builder.add(Op::kNot, 1, {ObjId(5)});
  EXPECT_FALSE(builder.ok());
  EXPECT_THAT(finishError(std::move(builder)), HasSubstr("placeholder"));
}

TEST(ModuleBuilderTest, RejectsOpsWithDedicatedMethods)
{
  for (const Op op : {
           Op::kPi,
           Op::kPo,
           Op::kConst,
           Op::kRegOut,
           Op::kRegIn,
           Op::kLoopOut,
           Op::kLoopIn,
           Op::kMemRead,
       }) {
    ModuleBuilder builder;
    builder.add(op, 1, {});
    EXPECT_THAT(finishError(std::move(builder)), HasSubstr(opName(op)));
  }
}

TEST(ModuleBuilderTest, RejectsUnresolvedPlaceholder)
{
  ModuleBuilder builder;
  builder.addPo(pn(), builder.addPlaceholder(1));
  EXPECT_THAT(finishError(std::move(builder)), HasSubstr("never resolved"));
}

TEST(ModuleBuilderTest, RejectsWidthMismatch)
{
  ModuleBuilder builder;
  const ObjId p = builder.addPlaceholder(4);
  builder.drivePlaceholder(p, builder.addPi(pn(), 3));
  EXPECT_THAT(finishError(std::move(builder)), HasSubstr("width"));
}

TEST(ModuleBuilderTest, RejectsDoubleResolve)
{
  ModuleBuilder builder;
  const ObjId p = builder.addPlaceholder(1);
  builder.drivePlaceholder(p, builder.addPi(pn(), 1));
  builder.drivePlaceholder(p, builder.addPi(pn(), 1));
  EXPECT_THAT(finishError(std::move(builder)), HasSubstr("already resolved"));
}

TEST(ModuleBuilderTest, RejectsPlaceholderCycle)
{
  ModuleBuilder builder;
  const ObjId p1 = builder.addPlaceholder(1);
  const ObjId p2 = builder.addPlaceholder(1);
  builder.drivePlaceholder(p1, p2);
  builder.drivePlaceholder(p2, p1);
  EXPECT_THAT(finishError(std::move(builder)), HasSubstr("cycle"));
}

TEST(ModuleBuilderTest, RejectsRegisterWithoutInput)
{
  ModuleBuilder builder;
  builder.addRegisterOut(1, RegClassId(0));
  EXPECT_THAT(finishError(std::move(builder)), HasSubstr("have no input"));
}

TEST(ModuleBuilderTest, RejectsSecondRegisterInput)
{
  ModuleBuilder builder;
  const ObjId clk = builder.addPi(pn(), 1);
  const ObjId q = builder.addRegisterOut(1, RegClassId(0));
  builder.addRegisterIn(q, {q, clk});
  builder.addRegisterIn(q, {q, clk});
  EXPECT_THAT(finishError(std::move(builder)), HasSubstr("already has"));
}

TEST(ModuleBuilderTest, LoopPairIsLinkedAndOrdered)
{
  ModuleBuilder builder;
  const ObjId a = builder.addPi(pn(), 4);
  const ObjId out = builder.addLoopOut(4, Signedness::kSigned);
  const ObjId x = builder.add(Op::kXor, 4, {a, out});
  const ObjId in = builder.addLoopIn(out, x);
  builder.addPo(pn(), x);
  const Module module = finishOrDie(std::move(builder));

  EXPECT_EQ(module.op(out), Op::kLoopOut);
  EXPECT_EQ(module.op(in), Op::kLoopIn);
  EXPECT_EQ(module.stateIn(out), in);
  EXPECT_EQ(module.stateOut(in), out);
  EXPECT_EQ(module.faninCount(out), 0);
  EXPECT_TRUE(module.fanins(out).empty());
  EXPECT_EQ(module.data(out), 0u);
  EXPECT_THAT(module.fanins(in), ElementsAre(x));
  EXPECT_EQ(module.width(in), 4);
  EXPECT_TRUE(module.isSigned(in));
  EXPECT_TRUE(module.isOrdered());
}

TEST(ModuleBuilderTest, RejectsBadLoopPairs)
{
  {
    ModuleBuilder b;
    b.addLoopOut(1);
    EXPECT_THAT(finishError(std::move(b)), HasSubstr("have no input"));
  }
  {
    ModuleBuilder b;
    const ObjId out = b.addLoopOut(2);
    b.addLoopIn(out, b.addPi(pn(), 3));
    EXPECT_THAT(finishError(std::move(b)), HasSubstr("width"));
  }
  {
    ModuleBuilder b;
    const ObjId a = b.addPi(pn(), 1);
    const ObjId out = b.addLoopOut(1);
    b.addLoopIn(out, a);
    b.addLoopIn(out, a);
    EXPECT_THAT(finishError(std::move(b)), HasSubstr("already has"));
  }
  {
    ModuleBuilder b;
    const ObjId a = b.addPi(pn(), 1);
    b.addLoopIn(a, a);
    EXPECT_THAT(finishError(std::move(b)), HasSubstr("not a loop output"));
  }
}

TEST(ModuleBuilderTest, CopiesLoopPairs)
{
  ModuleBuilder src_builder;
  const ObjId a = src_builder.addPi(pn(), 4);
  const ObjId out = src_builder.addLoopOut(4);
  const ObjId x = src_builder.add(Op::kXor, 4, {a, out});
  const ObjId in = src_builder.addLoopIn(out, x);
  src_builder.addPo(pn(), x);
  const Module src = finishOrDie(std::move(src_builder));

  ModuleBuilder b(src);
  std::vector<ObjId> map(src.size());
  for (const ObjId obj : src.objects()) {
    std::vector<ObjId> fanins;
    for (const ObjId f : src.fanins(obj)) {
      fanins.push_back(map[f.value()]);
    }
    map[obj.value()] = b.copy(obj, fanins);
  }
  const Module m = finishOrDie(std::move(b));
  const ObjId new_out = map[out.value()];
  const ObjId new_in = map[in.value()];
  EXPECT_EQ(m.op(new_out), Op::kLoopOut);
  EXPECT_EQ(m.stateIn(new_out), new_in);
  EXPECT_EQ(m.stateOut(new_in), new_out);
  EXPECT_TRUE(m.fanins(new_out).empty());
  EXPECT_THAT(m.fanins(new_in), ElementsAre(map[x.value()]));
  EXPECT_TRUE(m.isOrdered());
}

// A module with one object of each op that has a typed accessor
// (Module.h). Instance and truth-table ids are not checked by finish().
struct AccessorFixture
{
  Module m;
  ObjId clk, a, slice, rep, inst, out1, lut, q, qn, d, loop, loop_in, y;
  RegClassId flop;
};

AccessorFixture accessorFixture()
{
  AccessorFixture f;
  ModuleBuilder b;
  f.clk = b.addPi(pn(), 1);
  f.a = b.addPi(pn(), 8);
  f.slice = b.add(Op::kSlice, 3, {f.a}, 2);
  f.rep = b.add(Op::kReplicate, 6, {f.slice}, 2);
  f.inst = b.add(Op::kInst, 0, {f.a}, 5);
  f.out1 = b.add(Op::kFan, 8, {f.inst}, 1);
  f.lut = b.add(Op::kLut, 1, {f.clk}, 9);
  f.flop = b.regClasses().intern(RegClass::generic(
      RegFlags(),
      regPinBit(RegPinType::kData) | regPinBit(RegPinType::kClock)));
  f.q = b.addRegisterOut(8, f.flop);
  f.qn = b.add(Op::kFan, 8, {f.q}, static_cast<uint32_t>(RegOutput::kQN));
  f.d = b.addRegisterIn(f.q, {f.a, f.clk});
  f.loop = b.addLoopOut(8);
  f.loop_in = b.addLoopIn(f.loop, f.out1);
  f.y = b.addPo(pn(), f.qn);
  f.m = finishOrDie(std::move(b));
  return f;
}

TEST(ModuleBuilderTest, TypedAccessorsReadEachPayload)
{
  const AccessorFixture f = accessorFixture();
  const Module& m = f.m;
  EXPECT_EQ(m.portIndex(f.a), 1);
  EXPECT_EQ(m.portIndex(f.y), 0);
  EXPECT_EQ(m.sliceLsb(f.slice), 2);
  EXPECT_EQ(m.replicateCount(f.rep), 2);
  EXPECT_EQ(m.instModule(f.inst), ModId(5));
  EXPECT_EQ(m.fanOwner(f.out1), f.inst);
  EXPECT_EQ(m.fanOutput(f.out1), 1);
  EXPECT_EQ(m.lutTable(f.lut), ConstId(9));
  EXPECT_EQ(m.regClassId(f.q), f.flop);
  EXPECT_EQ(m.regClass(f.d), m.regClass(f.q));
  EXPECT_EQ(m.fanOwner(f.qn), f.q);
  EXPECT_EQ(m.fanOutput(f.qn), static_cast<int>(RegOutput::kQN));
  EXPECT_EQ(m.fanRegOutput(f.qn), RegOutput::kQN);
}

// The pair links are not fanins: a state output has none, a kFan keeps its
// owner as fanin 0.
TEST(ModuleBuilderTest, StateOutputsHaveNoFanins)
{
  const AccessorFixture f = accessorFixture();
  const Module& m = f.m;
  EXPECT_EQ(m.stateIn(f.q), f.d);
  EXPECT_EQ(m.stateOut(f.d), f.q);
  EXPECT_EQ(m.stateIn(f.loop), f.loop_in);
  EXPECT_EQ(m.stateOut(f.loop_in), f.loop);
  for (const ObjId out : {f.q, f.loop}) {
    EXPECT_EQ(m.faninCount(out), 0);
    EXPECT_TRUE(m.fanins(out).empty());
  }
  EXPECT_THAT(m.fanins(f.d), ElementsAre(f.a, f.clk));
  EXPECT_THAT(m.fanins(f.loop_in), ElementsAre(f.out1));
  EXPECT_THAT(m.fanins(f.qn), ElementsAre(f.q));
  EXPECT_TRUE(m.isOrdered());

  // A clone and a rebuild keep both links.
  const Module c = m.clone();
  EXPECT_EQ(c.stateIn(f.q), f.d);
  EXPECT_EQ(c.stateOut(f.loop_in), f.loop);
  ModuleBuilder b(m);
  std::vector<ObjId> map(m.size());
  for (const ObjId obj : m.objects()) {
    std::vector<ObjId> fanins;
    for (const ObjId fanin : m.fanins(obj)) {
      fanins.push_back(map[fanin.value()]);
    }
    map[obj.value()] = b.copy(obj, fanins);
  }
  const Module r = finishOrDie(std::move(b));
  for (const ObjId out : {f.q, f.loop}) {
    const ObjId new_out = map[out.value()];
    EXPECT_EQ(r.stateIn(new_out), map[m.stateIn(out).value()]);
    EXPECT_EQ(r.stateOut(r.stateIn(new_out)), new_out);
    EXPECT_EQ(r.faninCount(new_out), 0);
  }
}

// The call sequence the slang frontend makes for
//   module counter(input clk, input rst, input [7:0] inc, output [7:0] out);
//     reg [7:0] count; wire [7:0] next;
//     assign out = count;
//     assign next = rst ? 8'd0 : count + inc;
//     always @(posedge clk) count <= next;
//   endmodule
// Every symbol starts as a placeholder; drivers are connected later.
TEST(ModuleBuilderTest, FrontendStyleCounter)
{
  ModuleBuilder b;
  const ObjId p_clk = b.addPlaceholder(1);
  const ObjId p_rst = b.addPlaceholder(1);
  const ObjId p_inc = b.addPlaceholder(8);
  const ObjId p_out = b.addPlaceholder(8);
  const ObjId p_count = b.addPlaceholder(8);
  const ObjId p_next = b.addPlaceholder(8);

  const ObjId clk = b.addPi(pn(), 1);
  b.drivePlaceholder(p_clk, clk);
  const ObjId rst = b.addPi(pn(), 1);
  b.drivePlaceholder(p_rst, rst);
  const ObjId inc = b.addPi(pn(), 8);
  b.drivePlaceholder(p_inc, inc);
  b.drivePlaceholder(p_out, p_count);  // assign out = count
  const ObjId sum = b.add(Op::kAdd, 8, {p_count, p_inc});
  const ObjId zero = b.addConst(8, 0);
  const ObjId mux = b.add(Op::kMux, 8, {p_rst, zero, sum});
  b.drivePlaceholder(p_next, mux);
  const ObjId q = b.addRegisterOut(8, RegClassId(0));  // always @(posedge clk)
  const ObjId d = b.addRegisterIn(q, {p_next, p_clk});
  b.drivePlaceholder(p_count, q);
  const ObjId out = b.addPo(pn(), p_out);
  const Module m = finishOrDie(std::move(b));

  EXPECT_EQ(m.size(), 9u);  // no buffers or placeholders left
  EXPECT_THAT(m.fanins(sum), ElementsAre(q, inc));
  EXPECT_THAT(m.fanins(mux), ElementsAre(rst, zero, sum));
  EXPECT_THAT(m.fanins(d), ElementsAre(mux, clk));
  EXPECT_EQ(m.fanin(out, 0), q);
  // sum reads the RegOut created after it: compact() restores the order.
  EXPECT_FALSE(m.isOrdered());
}

TEST(ModuleBuilderTest, PiecewiseResolveBuildsConcat)
{
  ModuleBuilder builder;
  const ObjId w = builder.addPlaceholder(8);
  const ObjId po = builder.addPo(pn(), w);
  const ObjId a = builder.addPi(pn(), 4);
  const ObjId b = builder.addPi(pn(), 4);
  builder.drivePlaceholder(w, 4, b);  // assign w[7:4] = b;
  builder.drivePlaceholder(w, 0, a);  // assign w[3:0] = a;
  const Module module = finishOrDie(std::move(builder));

  const ObjId concat = module.fanin(po, 0);
  EXPECT_EQ(module.op(concat), Op::kConcat);
  EXPECT_EQ(module.width(concat), 8);
  EXPECT_THAT(module.fanins(concat), ElementsAre(a, b));  // LSB first
}

TEST(ModuleBuilderTest, PiecewiseResolveThroughPlaceholder)
{
  ModuleBuilder builder;
  const ObjId w = builder.addPlaceholder(8);
  const ObjId low = builder.addPlaceholder(4);
  const ObjId po = builder.addPo(pn(), w);
  builder.drivePlaceholder(w, 0, low);
  const ObjId high = builder.addPi(pn(), 4);
  builder.drivePlaceholder(w, 4, high);
  const ObjId a = builder.addPi(pn(), 4);
  builder.drivePlaceholder(low, a);
  const Module module = finishOrDie(std::move(builder));

  EXPECT_THAT(module.fanins(module.fanin(po, 0)), ElementsAre(a, high));
}

TEST(ModuleBuilderTest, UndrivenBitsAreAnErrorByDefault)
{
  ModuleBuilder builder;
  const ObjId w = builder.addPlaceholder(8);
  builder.addPo(pn(), w);
  builder.drivePlaceholder(w, 0, builder.addPi(pn(), 4));
  EXPECT_THAT(finishError(std::move(builder)),
              HasSubstr("bits [4, 8) of placeholder 0 are never driven"));
}

TEST(ModuleBuilderTest, UndrivenBitsBecomeUndrivenObjects)
{
  ModuleBuilder builder;
  builder.setAllowUndriven(true);
  const ObjId w = builder.addPlaceholder(8);
  const ObjId unused = builder.addPlaceholder(3);
  const ObjId po_w = builder.addPo(pn(), w);
  const ObjId po_unused = builder.addPo(pn(), unused);
  const ObjId a = builder.addPi(pn(), 2);
  builder.drivePlaceholder(w, 2, a);
  const Module module = finishOrDie(std::move(builder));

  const ObjId concat = module.fanin(po_w, 0);
  ASSERT_EQ(module.faninCount(concat), 3);
  EXPECT_EQ(module.op(module.fanin(concat, 0)), Op::kUndriven);
  EXPECT_EQ(module.width(module.fanin(concat, 0)), 2);
  EXPECT_EQ(module.fanin(concat, 1), a);
  EXPECT_EQ(module.op(module.fanin(concat, 2)), Op::kUndriven);
  EXPECT_EQ(module.width(module.fanin(concat, 2)), 4);
  const ObjId whole = module.fanin(po_unused, 0);
  EXPECT_EQ(module.op(whole), Op::kUndriven);
  EXPECT_EQ(module.width(whole), 3);
}

TEST(ModuleBuilderTest, RejectsOverlappingPieces)
{
  ModuleBuilder builder;
  const ObjId w = builder.addPlaceholder(8);
  builder.drivePlaceholder(w, 0, builder.addPi(pn(), 5));
  builder.drivePlaceholder(w, 4, builder.addPi(pn(), 4));
  EXPECT_THAT(finishError(std::move(builder)),
              HasSubstr("bit 4 of placeholder 0 is driven more than once"));
}

TEST(ModuleBuilderTest, RejectsPieceOutsidePlaceholder)
{
  ModuleBuilder builder;
  const ObjId w = builder.addPlaceholder(8);
  builder.drivePlaceholder(w, 6, builder.addPi(pn(), 4));
  EXPECT_THAT(finishError(std::move(builder)), HasSubstr("outside"));
}

TEST(ModuleBuilderTest, RejectsPieceAfterWholeResolve)
{
  ModuleBuilder builder;
  const ObjId w = builder.addPlaceholder(8);
  builder.drivePlaceholder(w, builder.addPi(pn(), 8));
  builder.drivePlaceholder(w, 0, builder.addPi(pn(), 4));
  EXPECT_THAT(finishError(std::move(builder)), HasSubstr("already resolved"));
}

TEST(ModuleBuilderTest, RejectsPortsWithoutNames)
{
  {
    ModuleBuilder b;
    b.addPi(NameId(1), 1);
    b.addPi(NameId(), 1);
    EXPECT_THAT(finishError(std::move(b)), HasSubstr("input 1 has no name"));
  }
  {
    ModuleBuilder b;
    const ObjId a = b.addPi(NameId(1), 1);
    b.addPo(NameId(), a);
    EXPECT_THAT(finishError(std::move(b)), HasSubstr("output 0 has no name"));
  }
}

TEST(ModuleBuilderTest, RejectsDuplicateInputNames)
{
  ModuleBuilder b;
  b.addPi(NameId(7), 1);
  b.addPi(NameId(7), 1);
  EXPECT_THAT(finishError(std::move(b)),
              HasSubstr("input 0 and input 1 have the same name"));
}

TEST(ModuleBuilderTest, RejectsDuplicateOutputNames)
{
  ModuleBuilder b;
  const ObjId a = b.addPi(NameId(1), 1);
  b.addPo(NameId(7), a);
  b.addPo(NameId(7), a);
  EXPECT_THAT(finishError(std::move(b)),
              HasSubstr("output 0 and output 1 have the same name"));
}

TEST(ModuleBuilderTest, RejectsAnInputAndAnOutputWithTheSameName)
{
  ModuleBuilder b;
  const ObjId a = b.addPi(NameId(7), 1);
  b.addPo(NameId(7), a);
  EXPECT_THAT(finishError(std::move(b)),
              HasSubstr("input 0 and output 0 have the same name"));
}

// Two clashes: the first in port order is reported, whatever the NameId
// values (the later clash has the smaller id).
TEST(ModuleBuilderTest, ReportsTheFirstPortNameClash)
{
  ModuleBuilder b;
  b.addPi(NameId(9), 1);
  b.addPi(NameId(2), 1);
  b.addPi(NameId(9), 1);
  b.addPi(NameId(2), 1);
  EXPECT_THAT(finishError(std::move(b)),
              HasSubstr("input 0 and input 2 have the same name"));
}

// setName on a port renames it, fixing or creating a collision.
TEST(ModuleBuilderTest, SetNameRenamesAPort)
{
  ModuleBuilder b;
  const ObjId a = b.addPi(NameId(7), 1);
  const ObjId po = b.addPo(NameId(7), a);
  b.setName(po, NameId(8));
  const Module m = finishOrDie(std::move(b));
  EXPECT_EQ(m.inputName(0), NameId(7));
  EXPECT_EQ(m.outputName(0), NameId(8));
}

TEST(ModuleBuilderTest, FullModuleIsResourceExhausted)
{
  const ModuleCapacity capacity(3, Module::kMaxFaninSlots);
  ModuleBuilder builder;
  const ObjId a = builder.addPi(pn(), 8);
  builder.addPo(pn(), builder.add(Op::kNot, 8, {a}));
  EXPECT_EQ(builder.size(), 3u);
  EXPECT_FALSE(builder.add(Op::kNot, 8, {a}).isValid());
  const absl::StatusOr<Module> module = std::move(builder).finish();
  ASSERT_FALSE(module.ok());
  EXPECT_EQ(module.status().code(), absl::StatusCode::kResourceExhausted);
  EXPECT_THAT(module.status().message(), HasSubstr("module is too large"));

  // Inline constants take the same path.
  ModuleBuilder consts;
  for (int i = 0; i < 3; ++i) {
    EXPECT_TRUE(consts.addConst(8, i).isValid());
  }
  EXPECT_FALSE(consts.addConst(8, 3).isValid());
  EXPECT_EQ(std::move(consts).finish().status().code(),
            absl::StatusCode::kResourceExhausted);
}

TEST(ModuleBuilderTest, FullFaninArenaIsResourceExhausted)
{
  // An append needs room for its fanins plus a possible escape header.
  const ModuleCapacity capacity(Module::kMaxObjects, 4);
  ModuleBuilder builder;
  const ObjId a = builder.addPi(pn(), 8);
  const ObjId n = builder.add(Op::kNot, 8, {a});     // slot 0
  const ObjId x = builder.add(Op::kAnd, 8, {a, n});  // slots 1-2
  EXPECT_TRUE(x.isValid());
  EXPECT_FALSE(builder.add(Op::kNot, 8, {x}).isValid());  // 3 + 2 > 4
  EXPECT_EQ(std::move(builder).finish().status().code(),
            absl::StatusCode::kResourceExhausted);
}

TEST(ModuleBuilderTest, CapacityRestored)
{
  {
    const ModuleCapacity capacity(1, 1);
  }
  ModuleBuilder builder;
  const ObjId a = builder.addPi(pn(), 8);
  builder.addPo(pn(), builder.add(Op::kAnd, 8, {a, a}));
  EXPECT_TRUE(std::move(builder).finish().ok());
}

int countLoopOuts(const Module& m)
{
  int n = 0;
  for (const ObjId obj : m.objects()) {
    n += !m.isDead(obj) && m.op(obj) == Op::kLoopOut ? 1 : 0;
  }
  return n;
}

TEST(ModuleBuilderTest, CutsSingleInstanceFeedbackCycle)
{
  Design design;
  const Design::SingleWriteScope write(design);
  // Child module with two independent paths: out0 = in0, out1 = in1.
  ModId child_id;
  {
    ModuleBuilder cb(design.names().intern("child"));
    const ObjId in0 = cb.addPi(design.names().intern("in0"), 4);
    const ObjId in1 = cb.addPi(design.names().intern("in1"), 4);
    cb.addPo(design.names().intern("out0"), in0);
    cb.addPo(design.names().intern("out1"), in1);
    absl::StatusOr<ModId> added = design.addModule(finishOrDie(std::move(cb)));
    ASSERT_TRUE(added.ok()) << added.status();
    child_id = *added;
  }

  // Parent connects child.out0 -> ~ -> child.in1 via a placeholder, creating
  // an apparent cycle through the single kInst.
  ModuleBuilder b(design.names().intern("top"));
  const ObjId x = b.addPi(design.names().intern("x"), 4);
  const ObjId fb = b.addPlaceholder(4);
  const ObjId inst = b.add(Op::kInst, 0, {x, fb}, child_id.value());
  const ObjId out0 = b.add(Op::kFan, 4, {inst}, 0);
  const ObjId out1 = b.add(Op::kFan, 4, {inst}, 1);
  const ObjId inv = b.add(Op::kNot, 4, {out0});
  b.drivePlaceholder(fb, inv);
  b.addPo(design.names().intern("y"), out1);

  absl::StatusOr<ModId> top_id = design.addModule(finishOrDie(std::move(b)));
  ASSERT_TRUE(top_id.ok()) << top_id.status();
  const Module& top = design.module(*top_id);
  EXPECT_TRUE(checkWellFormed(design, top).ok());

  int loop_outs = 0;
  for (const ObjId obj : top.objects()) {
    if (top.op(obj) == Op::kLoopOut) {
      ++loop_outs;
      EXPECT_EQ(top.width(obj), 4);
    }
  }
  EXPECT_EQ(loop_outs, 1);
}

TEST(ModuleBuilderTest, CutsMultiInstanceCycleAndPureGateCycle)
{
  Design design;
  const Design::SingleWriteScope write(design);
  ModId child_id;
  {
    ModuleBuilder cb(design.names().intern("child"));
    const ObjId in0 = cb.addPi(design.names().intern("in0"), 1);
    cb.addPo(design.names().intern("out0"), in0);
    absl::StatusOr<ModId> added = design.addModule(finishOrDie(std::move(cb)));
    ASSERT_TRUE(added.ok()) << added.status();
    child_id = *added;
  }

  // Two instances in a mutual cycle: u0.out0 -> u1.in0 and u1.out0 -> u0.in0.
  {
    ModuleBuilder b(design.names().intern("two_insts"));
    const ObjId p = b.addPlaceholder(1);
    const ObjId u0 = b.add(Op::kInst, 0, {p}, child_id.value());
    const ObjId u0_out = b.add(Op::kFan, 1, {u0}, 0);
    const ObjId u1 = b.add(Op::kInst, 0, {u0_out}, child_id.value());
    const ObjId u1_out = b.add(Op::kFan, 1, {u1}, 0);
    b.drivePlaceholder(p, u1_out);
    b.addPo(design.names().intern("y"), u1_out);

    const Module m = finishOrDie(std::move(b));
    EXPECT_TRUE(checkWellFormed(design, m).ok());
  }

  // A pure gate cycle in a module that also has an instance is cut too.
  {
    ModuleBuilder b(design.names().intern("pure_cycle"));
    const ObjId a = b.addPi(design.names().intern("a"), 1);
    const ObjId u0 = b.add(Op::kInst, 0, {a}, child_id.value());
    const ObjId u0_out = b.add(Op::kFan, 1, {u0}, 0);
    const ObjId p = b.addPlaceholder(1);
    const ObjId g = b.add(Op::kAnd, 1, {u0_out, p});
    b.drivePlaceholder(p, g);
    b.addPo(design.names().intern("y"), g);

    const Module m = finishOrDie(std::move(b));
    EXPECT_TRUE(checkWellFormed(design, m).ok()) << checkWellFormed(design, m);
    EXPECT_EQ(countLoopOuts(m), 1);
  }
}

// Every combinational cycle is cut, also in a module without instances. The
// DFS from the lowest id closes the cycle at g2's fanin: g2 reads the LoopOut,
// the LoopIn reads g1.
TEST(ModuleBuilderTest, CutsCycleWithoutInstance)
{
  Design design;
  const Design::SingleWriteScope write(design);
  ModuleBuilder b(design.names().intern("ring"));
  const ObjId a = b.addPi(design.names().intern("a"), 1);
  const ObjId p = b.addPlaceholder(1);
  const ObjId g1 = b.add(Op::kAnd, 1, {a, p});
  const ObjId g2 = b.add(Op::kNot, 1, {g1});
  b.drivePlaceholder(p, g2);
  b.addPo(design.names().intern("y"), g2);
  const Module m = finishOrDie(std::move(b));
  EXPECT_TRUE(checkWellFormed(design, m).ok()) << checkWellFormed(design, m);
  EXPECT_TRUE(acyclicTopologicalOrder(m).ok());
  ASSERT_EQ(countLoopOuts(m), 1);
  const ObjId loop_out = m.fanin(g2, 0);
  ASSERT_EQ(m.op(loop_out), Op::kLoopOut);
  const ObjId loop_in = m.stateIn(loop_out);
  EXPECT_EQ(m.fanin(loop_in, 0), g1);
  EXPECT_EQ(m.fanin(g1, 1), g2);
  EXPECT_EQ(loopPairDriver(m, loop_out), g1);
}

// A self-loop and a word-level false cycle (v[1] = ~v[0]) are cut as well.
TEST(ModuleBuilderTest, CutsSelfLoopAndWordLevelCycle)
{
  Design design;
  const Design::SingleWriteScope write(design);
  {
    ModuleBuilder b(design.names().intern("self"));
    const ObjId p = b.addPlaceholder(1);
    const ObjId n = b.add(Op::kNot, 1, {p});
    b.drivePlaceholder(p, n);
    b.addPo(design.names().intern("y"), n);
    const Module m = finishOrDie(std::move(b));
    EXPECT_TRUE(checkWellFormed(design, m).ok());
    EXPECT_EQ(countLoopOuts(m), 1);
  }
  {
    ModuleBuilder b(design.names().intern("word"));
    const ObjId x = b.addPi(design.names().intern("x"), 1);
    const ObjId v = b.addPlaceholder(2);
    const ObjId hi = b.add(Op::kNot, 1, {b.add(Op::kSlice, 1, {v}, 0)});
    b.drivePlaceholder(v, b.add(Op::kConcat, 2, {x, hi}));
    b.addPo(design.names().intern("y"), v);
    const Module m = finishOrDie(std::move(b));
    EXPECT_TRUE(checkWellFormed(design, m).ok());
    EXPECT_EQ(countLoopOuts(m), 1);
  }
}

}  // namespace
}  // namespace syn::test
