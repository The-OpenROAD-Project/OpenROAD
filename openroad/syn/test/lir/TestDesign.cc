// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include <atomic>
#include <cstdint>
#include <string_view>
#include <thread>
#include <utility>
#include <vector>

#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/strings/str_cat.h"
#include "gtest/gtest.h"
#include "helpers/TestDesignOptions.h"
#include "helpers/TestLiberty.h"
#include "helpers/TestModules.h"
#include "sta/LibertyClass.hh"
#include "syn/lir/Design.h"
#include "syn/lir/Ids.h"
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
#include "utl/Logger.h"

namespace syn::test {
namespace {

using ::syn::lir::checkWellFormed;
using ::syn::lir::ConstId;
using ::syn::lir::Design;
using ::syn::lir::LibCellId;
using ::syn::lir::ModId;
using ::syn::lir::Module;
using ::syn::lir::ModuleAttributes;
using ::syn::lir::ModuleBuilder;
using ::syn::lir::NameId;
using ::syn::lir::NameInterner;
using ::syn::lir::ObjAttributes;
using ::syn::lir::ObjId;
using ::syn::lir::Op;
using ::syn::lir::OperatorKey;
using ::syn::lir::OpOperand;
using ::syn::lir::OpSpec;
using ::syn::lir::RegClass;
using ::syn::lir::RegClassId;
using ::syn::lir::RegFlags;
using ::syn::lir::regPinBit;
using ::syn::lir::RegPinMask;
using ::syn::lir::RegPinType;
using ::syn::lir::SourceLoc;

// A fresh port name, interned in `design`: every call returns a new one.
NameId pn(Design& design)
{
  static int next = 0;
  return design.names().intern(absl::StrCat("port", next++));
}

Module buildInverter(Design& design, std::string_view name)
{
  ModuleBuilder builder(design.names().intern(name));
  const ObjId a = builder.addPi(design.names().intern("a"), 1);
  builder.addPo(design.names().intern("y"), builder.add(Op::kNot, 1, {a}));
  absl::StatusOr<Module> module = std::move(builder).finish();
  EXPECT_TRUE(module.ok()) << module.status();
  return module.ok() ? *std::move(module) : Module();
}

TEST(DesignTest, AddAndFindModules)
{
  Design design(kCheckWellFormed);
  Design::SingleWriteScope write(design);
  const absl::StatusOr<ModId> inv
      = design.addModule(buildInverter(design, "inv"));
  const absl::StatusOr<ModId> top
      = design.addModule(buildInverter(design, "top"));
  ASSERT_TRUE(inv.ok());
  ASSERT_TRUE(top.ok());
  EXPECT_NE(*inv, *top);
  EXPECT_EQ(design.numModules(), 2);
  EXPECT_EQ(design.findModule("inv"), *inv);
  EXPECT_EQ(design.findModule(design.names().intern("top")), *top);
  EXPECT_FALSE(design.findModule("missing").isValid());
  EXPECT_EQ(design.names().name(design.module(*top).name()), "top");
  EXPECT_GT(design.memoryBytes(), 0u);
}

TEST(DesignTest, RejectsDuplicateNames)
{
  Design design(kCheckWellFormed);
  Design::SingleWriteScope write(design);
  ASSERT_TRUE(design.addModule(buildInverter(design, "inv")).ok());
  const absl::StatusOr<ModId> again
      = design.addModule(buildInverter(design, "inv"));
  EXPECT_EQ(again.status().code(), absl::StatusCode::kAlreadyExists);
}

TEST(DesignTest, AnonymousModulesAreAllowed)
{
  Design design(kCheckWellFormed);
  Design::SingleWriteScope write(design);
  EXPECT_TRUE(design.addModule(Module()).ok());
  EXPECT_TRUE(design.addModule(Module()).ok());
  EXPECT_EQ(design.numModules(), 2);
}

TEST(DesignTest, ReplaceModuleKeepsIdAndAddress)
{
  Design design(kCheckWellFormed);
  Design::SingleWriteScope write(design);
  const ModId id = *design.addModule(buildInverter(design, "inv"));
  const Module* before = &design.module(id);
  // Add more modules so the table grows.
  for (int i = 0; i < 100; ++i) {
    ASSERT_TRUE(
        design.addModule(buildInverter(design, absl::StrCat("m", i))).ok());
  }
  EXPECT_EQ(&design.module(id), before);

  ModuleBuilder builder(design.names().intern("inv"));
  builder.addPo(pn(design), builder.addPi(pn(design), 1));
  ASSERT_TRUE(design.replaceModule(id, *std::move(builder).finish()).ok());
  EXPECT_EQ(&design.module(id), before);
  EXPECT_EQ(design.module(id).size(), 2u);

  ModuleBuilder renamed(design.names().intern("other"));
  EXPECT_FALSE(design.replaceModule(id, *std::move(renamed).finish()).ok());
}

// A removed slot stays removed: an anonymous replacement would pass the name
// check (the slot is anonymous too) and bring it back.
TEST(DesignTest, ReplaceModuleRefusesARemovedModule)
{
  Design design(kCheckWellFormed);
  Design::SingleWriteScope write(design);
  const absl::StatusOr<ModId> leaf
      = design.addModule(buildInverter(design, "leaf"));
  ASSERT_TRUE(leaf.ok());
  ASSERT_TRUE(design.removeModule(*leaf).ok());

  ModuleBuilder anonymous{NameId()};
  absl::StatusOr<Module> empty = std::move(anonymous).finish();
  ASSERT_TRUE(empty.ok()) << empty.status();
  EXPECT_EQ(design.replaceModule(*leaf, *std::move(empty)).code(),
            absl::StatusCode::kFailedPrecondition);
  EXPECT_TRUE(design.module(*leaf).isRemoved());
}

TEST(DesignTest, ConcurrentAddModule)
{
  Design design(kCheckWellFormed);
  constexpr int kThreads = 8;
  constexpr int kPerThread = 200;
  {
    Design::ParallelWriteScope write(design);
    std::vector<std::thread> threads;
    threads.reserve(kThreads);
    for (int t = 0; t < kThreads; ++t) {
      threads.emplace_back([&, t] {
        for (int i = 0; i < kPerThread; ++i) {
          const absl::StatusOr<ModId> id = design.addModule(
              buildInverter(design, absl::StrCat("t", t, "_", i)));
          ASSERT_TRUE(id.ok());
        }
      });
    }
    for (std::thread& thread : threads) {
      thread.join();
    }
  }
  EXPECT_EQ(design.numModules(), kThreads * kPerThread);
  const ModId id = design.findModule("t3_17");
  ASSERT_TRUE(id.isValid());
  EXPECT_EQ(design.names().name(design.module(id).name()), "t3_17");
}

TEST(DesignTest, DefaultsToReadOnly)
{
  Design design(kCheckWellFormed);
  EXPECT_TRUE(design.isReadOnly());
  EXPECT_TRUE(design.names().isReadOnly());
  EXPECT_EQ(design.addModule(Module()).status().code(),
            absl::StatusCode::kFailedPrecondition);
  EXPECT_EQ(design.numModules(), 0);
}

TEST(DesignTest, ReadOnlyModeAllowsLookupsAndModuleReplacement)
{
  Design design(kCheckWellFormed);
  {
    Design::SingleWriteScope write(design);
    EXPECT_FALSE(design.isReadOnly());
    EXPECT_FALSE(design.names().isReadOnly());
    ASSERT_TRUE(design.addModule(buildInverter(design, "inv")).ok());
  }
  ASSERT_TRUE(design.isReadOnly());
  // Adding a module is rejected; this module's names already exist.
  EXPECT_EQ(design.addModule(buildInverter(design, "inv")).status().code(),
            absl::StatusCode::kFailedPrecondition);
  // Lookups and replacing a module's contents still work.
  const ModId inv = design.findModule("inv");
  ASSERT_TRUE(inv.isValid());
  EXPECT_EQ(design.names().intern("inv"), design.module(inv).name());
  EXPECT_TRUE(design.replaceModule(inv, buildInverter(design, "inv")).ok());
  {
    Design::SingleWriteScope write(design);
    EXPECT_TRUE(design.addModule(buildInverter(design, "inv2")).ok());
  }
  EXPECT_EQ(design.numModules(), 2);
}

// The cell table follows the design's mode: writable only inside a write
// scope.
TEST(DesignTest, CellTableFollowsTheWriteScope)
{
  TestLiberty lib("test_lib", "test.lib");  // outlives the design
  sta::LibertyCell* inv = lib.combinational("INV_X1", {"A"}, {"Y"});

  Design design(kCheckWellFormed);
  EXPECT_TRUE(design.cells().isReadOnly());
  LibCellId id;
  {
    Design::SingleWriteScope write(design);
    EXPECT_FALSE(design.cells().isReadOnly());
    id = design.cells().intern(inv);
  }
  EXPECT_TRUE(design.cells().isReadOnly());
  EXPECT_EQ(design.cells().find(inv), id);
  EXPECT_EQ(design.cells().info(id).name(), "INV_X1");
}

TEST(DesignTest, ScopesRestoreAndNest)
{
  Design design(kCheckWellFormed);
  EXPECT_TRUE(design.isReadOnly());
  EXPECT_FALSE(design.isWritable());
  EXPECT_TRUE(design.isModulesReadOnly());
  EXPECT_TRUE(design.isPoolsReadOnly());
  {
    Design::SingleWriteScope write(design);
    EXPECT_FALSE(design.isReadOnly());
    EXPECT_TRUE(design.isWritable());
    EXPECT_TRUE(design.isModulesWritable());
    EXPECT_TRUE(design.isPoolsWritable());
    {
      Design::ReadOnlyScope read_only(design);
      EXPECT_TRUE(design.isReadOnly());
      EXPECT_FALSE(design.isWritable());
      EXPECT_TRUE(design.isModulesReadOnly());
      EXPECT_TRUE(design.isPoolsReadOnly());
      EXPECT_TRUE(design.names().isReadOnly());
      {
        Design::SingleWriteScope inner(design);
        EXPECT_FALSE(design.isReadOnly());
        EXPECT_TRUE(design.isWritable());
      }
      EXPECT_TRUE(design.isReadOnly());
      EXPECT_FALSE(design.isWritable());
    }
    {
      Design::ModulesReadOnlyScope modules_ro(design);
      EXPECT_TRUE(design.isModulesReadOnly());
      EXPECT_FALSE(design.isModulesWritable());
      EXPECT_FALSE(design.isPoolsReadOnly());
      EXPECT_TRUE(design.isPoolsWritable());
      EXPECT_FALSE(design.isReadOnly());
      EXPECT_FALSE(design.isWritable());
      {
        Design::PoolsReadOnlyScope pools_ro(design);
        EXPECT_TRUE(design.isPoolsReadOnly());
        EXPECT_TRUE(design.isReadOnly());
        EXPECT_FALSE(design.isWritable());
      }
      EXPECT_TRUE(design.isModulesReadOnly());
      EXPECT_TRUE(design.isPoolsWritable());
    }
    EXPECT_FALSE(design.isReadOnly());
    EXPECT_TRUE(design.isWritable());
    EXPECT_FALSE(design.isModulesReadOnly());
    EXPECT_FALSE(design.isPoolsReadOnly());
    EXPECT_FALSE(design.names().isReadOnly());
  }
  EXPECT_TRUE(design.isReadOnly());
  EXPECT_FALSE(design.isWritable());
  EXPECT_TRUE(design.isModulesReadOnly());
  EXPECT_TRUE(design.isPoolsReadOnly());
  EXPECT_TRUE(design.names().isReadOnly());
}

TEST(DesignTest, PoolsSingleWriteScopeAllowsPoolWritesWhileModulesStayReadOnly)
{
  Design design(kCheckWellFormed);
  ModId inv;
  {
    Design::SingleWriteScope write(design);
    inv = *design.addModule(buildInverter(design, "inv"));
  }
  ASSERT_TRUE(design.isReadOnly());

  {
    Design::PoolsSingleWriteScope pools_write(design);
    EXPECT_TRUE(design.isModulesReadOnly());
    EXPECT_FALSE(design.isModulesWritable());
    EXPECT_FALSE(design.isPoolsReadOnly());
    EXPECT_TRUE(design.isPoolsWritable());
    EXPECT_FALSE(design.isPoolsParallelWrite());
    EXPECT_FALSE(design.isReadOnly());
    EXPECT_FALSE(design.isWritable());
    {
      Design::PoolsParallelWriteScope parallel_pools(design);
      EXPECT_TRUE(design.isModulesReadOnly());
      EXPECT_TRUE(design.isPoolsWritable());
      EXPECT_TRUE(design.isPoolsParallelWrite());
      EXPECT_FALSE(design.isParallelWrite());
    }
    EXPECT_FALSE(design.isPoolsParallelWrite());

    // Pools accept new entries.
    const NameId fresh = design.names().intern("fresh_net");
    EXPECT_EQ(design.names().name(fresh), "fresh_net");
    const ConstId wide = design.consts().internUint(64, 42);
    EXPECT_EQ(design.consts().get(wide).toUint64(), uint64_t{42});

    // Module table mutations still fail with FailedPrecondition.
    EXPECT_EQ(design.addModule(Module()).status().code(),
              absl::StatusCode::kFailedPrecondition);
    EXPECT_EQ(design.copyModule(inv, fresh).status().code(),
              absl::StatusCode::kFailedPrecondition);
    EXPECT_EQ(design.removeModule(inv).code(),
              absl::StatusCode::kFailedPrecondition);
    EXPECT_EQ(design.setModuleAttributes(inv, {.keep_hierarchy = true}).code(),
              absl::StatusCode::kFailedPrecondition);
    EXPECT_EQ(design.markFrozenModules().code(),
              absl::StatusCode::kFailedPrecondition);

    // Replacing a module's contents in place is allowed.
    EXPECT_TRUE(design.replaceModule(inv, buildInverter(design, "inv")).ok());
  }
  {
    Design::PoolsParallelWriteScope parallel_pools(design);
    EXPECT_TRUE(design.isModulesReadOnly());
    EXPECT_FALSE(design.isModulesParallelWrite());
    EXPECT_TRUE(design.isPoolsWritable());
    EXPECT_TRUE(design.isPoolsParallelWrite());
    EXPECT_FALSE(design.isParallelWrite());
    EXPECT_EQ(design.findModule(design.module(inv).name()), inv);
  }
  {
    Design::ParallelWriteScope parallel_all(design);
    EXPECT_TRUE(design.isParallelWrite());
  }
  EXPECT_TRUE(design.isReadOnly());
}

TEST(DesignTest, ModulesSingleWriteScopeAllowsModuleEditsWhilePoolsStayReadOnly)
{
  Design design(kCheckWellFormed);
  NameId clone_name;
  ModId inv;
  {
    Design::SingleWriteScope write(design);
    clone_name = design.names().intern("inv_clone");
    inv = *design.addModule(buildInverter(design, "inv"));
  }
  ASSERT_TRUE(design.isReadOnly());

  {
    Design::ModulesSingleWriteScope modules_write(design);
    EXPECT_FALSE(design.isModulesReadOnly());
    EXPECT_TRUE(design.isModulesWritable());
    EXPECT_TRUE(design.isPoolsReadOnly());
    EXPECT_FALSE(design.isPoolsWritable());
    EXPECT_FALSE(design.isReadOnly());
    EXPECT_FALSE(design.isWritable());

    // Copying, adding, setting attributes, freezing, and removing modules work
    // while pools are read-only.
    absl::StatusOr<ModId> copy = design.copyModule(inv, clone_name);
    ASSERT_TRUE(copy.ok()) << copy.status();
    EXPECT_EQ(design.findModule("inv_clone"), *copy);
    EXPECT_TRUE(design.setModuleAttributes(*copy, {.dont_touch = true}).ok());
    EXPECT_TRUE(design.markFrozenModules().ok());
    EXPECT_TRUE(design.module(*copy).isFrozen());
    EXPECT_TRUE(design.removeModule(*copy).ok());
  }
  EXPECT_TRUE(design.isReadOnly());
}

TEST(DesignTest, PoolsFollowTheDesignMode)
{
  Design design(kCheckWellFormed);
  EXPECT_TRUE(design.consts().isReadOnly());

  const RegClass flop = RegClass::generic(
      RegFlags(), regPinBit(RegPinType::kData) | regPinBit(RegPinType::kClock));
  ModId counter;
  ConstId one;
  {
    Design::SingleWriteScope write(design);
    EXPECT_FALSE(design.consts().isReadOnly());
    // 64 bits: wider than kMaxInlineConstWidth, so the constant is pooled.
    one = design.consts().internUint(64, 1);

    // count <= count + 64'd1
    ModuleBuilder b(design.names().intern("counter"));
    const RegClassId flop_id = b.regClasses().intern(flop);
    const ObjId clk = b.addPi(pn(design), 1);
    const ObjId q = b.addRegisterOut(64, flop_id);
    const ObjId k = b.addConst(design.consts().get(one), design.consts());
    b.addRegisterIn(q, {b.add(Op::kAdd, 64, {q, k}), clk});
    b.addPo(pn(design), q);
    counter = addModuleOrFail(design, std::move(b));
  }
  EXPECT_TRUE(design.consts().isReadOnly());

  // Read-only: find the constant from the module and the class in the
  // module's own pool.
  const Module& module = design.module(counter);
  const ObjId q(1);
  ASSERT_EQ(module.op(q), Op::kRegOut);
  EXPECT_EQ(module.regClass(q), flop);
  EXPECT_EQ(module.regClasses().size(), 1u);
  const ObjId k(2);
  ASSERT_EQ(module.op(k), Op::kConst);
  EXPECT_FALSE(module.isInlineConst(k));
  EXPECT_EQ(module.data(k), one.rawIndex());
  EXPECT_EQ(module.constValue(k, design.consts()).toUint64(), uint64_t{1});
  EXPECT_EQ(design.consts().internUint(64, 1), one);  // a lookup
  EXPECT_GT(design.memoryBytes(), design.names().memoryBytes());
}

// A pass running while the design is read-only (e.g. in parallel with
// others) can still create constants of up to 32 bits: they are stored in
// the objects and never touch the shared pool.
TEST(DesignTest, SmallConstantsNeedNoPoolWrites)
{
  const Design design(kCheckWellFormed);
  ASSERT_TRUE(design.isReadOnly());

  ModuleBuilder b;  // e.g. rebuilding a module after folding a + 1 + 2
  // Port names are reused from the source module, interned earlier.
  const ObjId a = b.addPi(NameId(0), 16);
  const ObjId three = b.addConst(16, 3);
  b.addPo(NameId(1), b.add(Op::kAdd, 16, {a, three}));
  absl::StatusOr<Module> module = std::move(b).finish();
  ASSERT_TRUE(module.ok()) << module.status();

  EXPECT_EQ(module->constValue(three, design.consts()).toUint64(), uint64_t{3});
  EXPECT_EQ(design.consts().size(), 0u);
}

// Parallel passes: the design is read-only and workers read modules and
// names without locks. Run under TSan.
TEST(DesignTest, ParallelReadsInReadOnlyMode)
{
  Design design(kCheckWellFormed);
  Design::SingleWriteScope write(design);
  constexpr int kModules = 200;
  for (int i = 0; i < kModules; ++i) {
    ASSERT_TRUE(
        design.addModule(buildInverter(design, absl::StrCat("m", i))).ok());
  }
  constexpr int kThreads = 8;
  std::atomic<int64_t> checked{0};
  {
    Design::ReadOnlyScope read_only(design);
    std::vector<std::thread> threads;
    threads.reserve(kThreads);
    for (int t = 0; t < kThreads; ++t) {
      threads.emplace_back([&] {
        const NameId a = design.names().intern("a");  // exists: a lookup
        int64_t local = 0;
        for (int round = 0; round < 20; ++round) {
          for (int m = 0; m < design.numModules(); ++m) {
            const auto mod_id = ModId(static_cast<uint32_t>(m));
            const Module& module = design.module(mod_id);
            const std::string_view name = design.names().name(module.name());
            ASSERT_EQ(design.findModule(name), mod_id);
            ASSERT_EQ(module.objName(module.inputs()[0]), a);
            ++local;
          }
        }
        checked += local;
      });
    }
    for (std::thread& thread : threads) {
      thread.join();
    }
  }
  EXPECT_EQ(checked.load(), int64_t{kThreads} * 20 * kModules);
}

// Parallel write mode: writers add modules while readers walk them; every
// access takes the design's mutex. Run under TSan.
TEST(DesignTest, WriteModeReadsWhileAdding)
{
  Design design(kCheckWellFormed);
  Design::ParallelWriteScope write(design);
  constexpr int kWriters = 4;
  constexpr int kReaders = 4;
  constexpr int kPerWriter = 300;
  std::atomic<bool> done{false};
  std::vector<std::thread> threads;
  threads.reserve(kWriters);
  for (int w = 0; w < kWriters; ++w) {
    threads.emplace_back([&, w] {
      for (int i = 0; i < kPerWriter; ++i) {
        ASSERT_TRUE(
            design
                .addModule(buildInverter(design, absl::StrCat("w", w, "_", i)))
                .ok());
      }
    });
  }
  std::atomic<int64_t> checked{0};
  for (int r = 0; r < kReaders; ++r) {
    threads.emplace_back([&] {
      int64_t local = 0;
      do {
        const int count = design.numModules();
        for (int m = 0; m < count; ++m) {
          const Module& module = design.module(ModId(static_cast<uint32_t>(m)));
          ASSERT_EQ(design.names().name(module.name()).front(), 'w');
          ASSERT_EQ(module.outputs().size(), 1u);
          ++local;
        }
      } while (!done.load(std::memory_order_relaxed));
      checked += local;
    });
  }
  for (int w = 0; w < kWriters; ++w) {
    threads[w].join();
  }
  done = true;
  for (int r = 0; r < kReaders; ++r) {
    threads[kWriters + r].join();
  }
  EXPECT_EQ(design.numModules(), kWriters * kPerWriter);
  EXPECT_GT(checked.load(), 0);
}

TEST(ObjectNamesTest, ObjectsAndPlaceholdersCarryNames)
{
  Design design(kCheckWellFormed);
  Design::SingleWriteScope write(design);
  NameInterner& names = design.names();
  ModuleBuilder b(names.intern("counter"));
  const ObjId p_count = b.addPlaceholder(8);
  b.setName(p_count, names.intern("count"));
  const ObjId p_out = b.addPlaceholder(8);
  b.setName(p_out, names.intern("out"));
  const ObjId clk = b.addPi(names.intern("clk"), 1);
  const ObjId anonymous = b.add(Op::kAdd, 8, {p_count, p_count});
  const ObjId q = b.addRegisterOut(8, RegClassId(0));
  b.addRegisterIn(q, {anonymous, clk});
  b.drivePlaceholder(p_count, q);
  b.drivePlaceholder(p_out, p_count);
  const ObjId po = b.addPo(names.intern("result"), p_out);
  const Module m = *std::move(b).finish();

  EXPECT_EQ(names.name(m.objName(clk)), "clk");
  // Both placeholders end at q; the first name given wins.
  EXPECT_EQ(names.name(m.objName(q)), "count");
  EXPECT_FALSE(m.objName(anonymous).isValid());
  EXPECT_EQ(names.name(m.objName(po)), "result");
}

TEST(ObjectNamesTest, PortNamesAreStoredPerPort)
{
  Design design(kCheckWellFormed);
  Design::SingleWriteScope write(design);
  NameInterner& names = design.names();
  ModuleBuilder b;
  const ObjId a = b.addPi(names.intern("a"), 1);
  const ObjId n = b.add(Op::kNot, 1, {a});
  const ObjId y = b.addPo(names.intern("y"), n);
  const Module m = *std::move(b).finish();
  EXPECT_EQ(m.inputName(0), names.intern("a"));
  EXPECT_EQ(m.outputName(0), names.intern("y"));
  EXPECT_EQ(m.objName(a), m.inputName(0));
  EXPECT_EQ(m.objName(y), m.outputName(0));
  EXPECT_FALSE(m.objName(n).isValid());
}

TEST(DesignTest, RemoveModuleKeepsTheSlotAndReleasesTheName)
{
  Design design(kCheckWellFormed);
  Design::SingleWriteScope write(design);
  const ModId inv = *design.addModule(buildInverter(design, "inv"));
  const ModId keep = *design.addModule(buildInverter(design, "keep"));
  ASSERT_TRUE(design.removeModule(inv).ok());

  EXPECT_EQ(design.numModules(), 2);
  EXPECT_TRUE(design.module(inv).isRemoved());
  EXPECT_EQ(design.module(inv).size(), 0u);
  EXPECT_FALSE(design.findModule("inv").isValid());
  EXPECT_EQ(design.findModule("keep"), keep);
  EXPECT_TRUE(checkWellFormed(design).ok());
  EXPECT_EQ(design.removeModule(inv).code(),
            absl::StatusCode::kFailedPrecondition);
  EXPECT_EQ(design.removeModule(ModId(7)).code(), absl::StatusCode::kNotFound);

  // The name can be used again.
  EXPECT_TRUE(design.addModule(buildInverter(design, "inv")).ok());

  Design::ReadOnlyScope read_only(design);
  EXPECT_EQ(design.removeModule(keep).code(),
            absl::StatusCode::kFailedPrecondition);
}

// A removed module keeps its slot but cannot be copied (the copy would be a
// named removed module) or have its operator record cleared.
TEST(DesignTest, RemovedModulesCannotBeCopiedOrCleared)
{
  Design design(kCheckWellFormed);
  Design::SingleWriteScope write(design);
  const ModId gone = *design.addModule(buildInverter(design, "gone"));
  ASSERT_TRUE(design.removeModule(gone).ok());
  const NameId copy_name = design.names().intern("gone_copy");

  EXPECT_EQ(design.copyModule(gone, copy_name).status().code(),
            absl::StatusCode::kFailedPrecondition);
  EXPECT_EQ(design.numModules(), 1);
  EXPECT_FALSE(design.findModule("gone_copy").isValid());
  EXPECT_EQ(design.clearOperatorImpl(gone).code(),
            absl::StatusCode::kFailedPrecondition);
  EXPECT_TRUE(design.module(gone).isRemoved());
}

TEST(DesignTest, WellFormedRejectsInstancesOfRemovedModules)
{
  Design design(kCheckWellFormed);
  Design::SingleWriteScope write(design);
  const ModId inv = *design.addModule(buildInverter(design, "inv"));
  ModuleBuilder builder(design.names().intern("top"));
  const ObjId a = builder.addPi(design.names().intern("a"), 1);
  const ObjId inst = builder.add(Op::kInst, 0, {a}, inv.value());
  builder.addPo(design.names().intern("y"),
                builder.add(Op::kFan, 1, {inst}, 0));
  ASSERT_TRUE(design.addModule(*std::move(builder).finish()).ok());
  ASSERT_TRUE(checkWellFormed(design).ok());
  ASSERT_TRUE(design.removeModule(inv).ok());
  EXPECT_EQ(checkWellFormed(design).code(), absl::StatusCode::kInvalidArgument);
}

TEST(DesignTest, SetModuleAttributesInPlace)
{
  Design design(kCheckWellFormed);
  Design::SingleWriteScope write(design);
  const ModId inv = *design.addModule(buildInverter(design, "inv"));
  const Module* address = &design.module(inv);
  const int64_t revision = design.module(inv).revision();
  ASSERT_TRUE(design.setModuleAttributes(inv, {.keep_hierarchy = true}).ok());
  EXPECT_EQ(&design.module(inv), address);
  EXPECT_EQ(design.module(inv).attributes(),
            ModuleAttributes{.keep_hierarchy = true});
  EXPECT_EQ(design.module(inv).revision(), revision);  // not structural
  EXPECT_TRUE(checkWellFormed(design).ok());

  EXPECT_EQ(design.setModuleAttributes(ModId(7), {}).code(),
            absl::StatusCode::kNotFound);
  const ModId gone = *design.addModule(buildInverter(design, "gone"));
  ASSERT_TRUE(design.removeModule(gone).ok());
  EXPECT_EQ(design.setModuleAttributes(gone, {}).code(),
            absl::StatusCode::kFailedPrecondition);
  Design::ReadOnlyScope read_only(design);
  EXPECT_EQ(design.setModuleAttributes(inv, {}).code(),
            absl::StatusCode::kFailedPrecondition);
}

TEST(DesignTest, LoggerAccessorAndOpenRoadLogCall)
{
  Design design(kCheckWellFormed);
  EXPECT_EQ(design.logger(), utl::Logger::defaultLogger());  // never null

  utl::Logger logger;
  design.setLogger(&logger);
  ASSERT_EQ(design.logger(), &logger);
  // Message ids must be unique across the tree; 1199 is kept for this test.
  design.logger()->info(utl::SYN, 1199, "L-IR logger test message");

  Design with_logger(kCheckWellFormed, &logger);
  EXPECT_EQ(with_logger.logger(), &logger);
}

// Object, module, and design checksums do not depend on the unordered pool
// IDs.
TEST(DesignTest, ChecksumIsCanonicalOverUnorderedPools)
{
  auto buildDesign = [](Design& design, bool reverse_pool_intern_order) {
    Design::SingleWriteScope write(design);

    const RegClass rc_a = RegClass::generic(
        RegFlags(),
        regPinBit(RegPinType::kData) | regPinBit(RegPinType::kClock));
    const RegClass rc_b = RegClass::generic(
        {.sync_over_enable = true},
        static_cast<RegPinMask>(uint32_t{regPinBit(RegPinType::kData)}
                                | regPinBit(RegPinType::kClock)
                                | regPinBit(RegPinType::kEnable)));

    if (reverse_pool_intern_order) {
      design.names().intern("extra_noise_name");
      design.consts().internUint(64, 0xdeadbeefULL);
      design.names().intern("file.v");
      design.names().intern("net_q");
      design.names().intern("y");
      design.names().intern("clk");
      design.names().intern("a");
      design.names().intern("top");
      design.consts().internUint(48, 0x123456789abcULL);
    } else {
      design.names().intern("top");
      design.names().intern("a");
      design.names().intern("clk");
      design.names().intern("y");
      design.names().intern("net_q");
      design.names().intern("file.v");
      design.consts().internUint(48, 0x123456789abcULL);
    }

    ModuleBuilder b(design.names().intern("top"));
    if (reverse_pool_intern_order) {
      b.regClasses().intern(rc_b);
      b.regClasses().intern(rc_a);
    } else {
      b.regClasses().intern(rc_a);
      b.regClasses().intern(rc_b);
    }
    const RegClassId flop_rc = b.regClasses().intern(rc_a);
    b.setModuleSourceLoc(SourceLoc(design.names().intern("file.v"), 10));

    const ObjId a = b.addPi(design.names().intern("a"), 48);
    const ObjId clk = b.addPi(design.names().intern("clk"), 1);
    const ConstId k_id = design.consts().internUint(48, 0x123456789abcULL);
    const ObjId k = b.addConst(design.consts().get(k_id), design.consts());
    const ObjId q = b.addRegisterOut(48, flop_rc);
    b.setName(q, design.names().intern("net_q"));
    b.addObjAttributes(q, ObjAttributes().setKeepValue());
    b.setObjSourceLoc(q, SourceLoc(design.names().intern("file.v"), 42));
    const ObjId sum = b.add(Op::kAdd, 48, {a, k});
    b.addRegisterIn(q, {sum, clk});
    b.addPo(design.names().intern("y"), q);

    EXPECT_TRUE(design.addModule(*std::move(b).finish()).ok());
  };

  Design d1(kCheckWellFormed);
  Design d2(kCheckWellFormed);
  buildDesign(d1, false);
  buildDesign(d2, true);

  const Module& m1 = d1.module(ModId(0));
  const Module& m2 = d2.module(ModId(0));
  // Unordered pool IDs differ between d1 and d2.
  EXPECT_NE(m1.name(), m2.name());

  // Object, module, and design checksums are identical despite different pool
  // IDs and extra unreferenced pool entries in d2.
  ASSERT_EQ(m1.size(), m2.size());
  for (const ObjId id : m1.objects()) {
    EXPECT_EQ(m1.objChecksum(d1, id), m2.objChecksum(d2, id)) << id.value();
  }
  EXPECT_EQ(m1.checksum(d1), m2.checksum(d2));
  EXPECT_EQ(d1.checksum(), d2.checksum());
}

// Ordered sequences (fanin order, object order, module order) change the
// checksum when permuted.
TEST(DesignTest, ChecksumDependsOnFaninOrder)
{
  auto buildWithFaninOrder = [](Design& design, bool swap_fanins) {
    Design::SingleWriteScope write(design);
    ModuleBuilder b(design.names().intern("m"));
    const ObjId a = b.addPi(design.names().intern("a"), 8);
    const ObjId c = b.addPi(design.names().intern("c"), 8);
    const ObjId sum
        = swap_fanins ? b.add(Op::kAdd, 8, {c, a}) : b.add(Op::kAdd, 8, {a, c});
    b.addPo(design.names().intern("y"), sum);
    EXPECT_TRUE(design.addModule(*std::move(b).finish()).ok());
  };
  Design f_ab(kCheckWellFormed);
  Design f_ba(kCheckWellFormed);
  buildWithFaninOrder(f_ab, false);
  buildWithFaninOrder(f_ba, true);
  EXPECT_NE(f_ab.module(ModId(0)).objChecksum(f_ab, ObjId(2)),
            f_ba.module(ModId(0)).objChecksum(f_ba, ObjId(2)));
  EXPECT_NE(f_ab.checksum(), f_ba.checksum());
}

// Swapping ModId order in Design changes Design::checksum().
TEST(DesignTest, ChecksumDependsOnModuleOrder)
{
  Design mod_order_1(kCheckWellFormed);
  Design mod_order_2(kCheckWellFormed);
  {
    Design::SingleWriteScope w1(mod_order_1);
    Design::SingleWriteScope w2(mod_order_2);
    ASSERT_TRUE(mod_order_1.addModule(buildInverter(mod_order_1, "m0")).ok());
    ASSERT_TRUE(mod_order_1.addModule(buildInverter(mod_order_1, "m1")).ok());
    ASSERT_TRUE(mod_order_2.addModule(buildInverter(mod_order_2, "m1")).ok());
    ASSERT_TRUE(mod_order_2.addModule(buildInverter(mod_order_2, "m0")).ok());
  }
  EXPECT_NE(mod_order_1.checksum(), mod_order_2.checksum());
}

// A module `name` holding one instance of `child` (1-bit in, 1-bit out).
Module buildParent(Design& design, std::string_view name, ModId child)
{
  ModuleBuilder builder(design.names().intern(name));
  const ObjId a = builder.addPi(design.names().intern("a"), 1);
  const ObjId inst = builder.add(Op::kInst, 0, {a}, child.value());
  builder.addPo(design.names().intern("y"),
                builder.add(Op::kFan, 1, {inst}, 0));
  absl::StatusOr<Module> module = std::move(builder).finish();
  EXPECT_TRUE(module.ok()) << module.status();
  return module.ok() ? *std::move(module) : Module();
}

// The key of an unsigned 8-bit a + b, interned in `design`.
OperatorKey adderKey(Design& design)
{
  OpSpec spec;
  spec.op = Op::kAdd;
  spec.result_width = 8;
  spec.operands = {OpOperand{.width = 8}, OpOperand{.width = 8}};
  return OperatorKey{.spec = design.operators().intern(spec)};
}

// An 8-bit adder module marked as the implementation of adderKey().
ModId addOperatorImpl(Design& design, std::string_view name)
{
  ModuleBuilder builder(design.names().intern(name));
  const ObjId a = builder.addPi(design.names().intern("a"), 8);
  const ObjId b = builder.addPi(design.names().intern("b"), 8);
  builder.addPo(design.names().intern("y"), builder.add(Op::kAdd, 8, {a, b}));
  const ModId id = addModuleOrFail(design, std::move(builder));
  EXPECT_TRUE(design.setOperatorImpl(id, adderKey(design)).ok());
  return id;
}

TEST(DesignTopTest, SingleRootIsTheTop)
{
  Design design(kCheckWellFormed);
  Design::SingleWriteScope write(design);
  const ModId leaf = *design.addModule(buildInverter(design, "leaf"));
  const ModId top = *design.addModule(buildParent(design, "top", leaf));
  const absl::StatusOr<ModId> found = design.top();
  ASSERT_TRUE(found.ok()) << found.status();
  EXPECT_EQ(*found, top);

  Design::ReadOnlyScope read_only(design);
  EXPECT_EQ(design.top().value(), top);
}

TEST(DesignTopTest, SetTopOverridesTheComputedRoot)
{
  Design design(kCheckWellFormed);
  Design::SingleWriteScope write(design);
  const ModId leaf = *design.addModule(buildInverter(design, "leaf"));
  const ModId top = *design.addModule(buildParent(design, "top", leaf));
  ASSERT_TRUE(design.setTop(leaf).ok());
  EXPECT_EQ(design.top().value(), leaf);
  design.clearTop();
  EXPECT_EQ(design.top().value(), top);
}

TEST(DesignTopTest, SeveralRootsListTheCandidatesInModIdOrder)
{
  Design design(kCheckWellFormed);
  Design::SingleWriteScope write(design);
  ASSERT_TRUE(design.addModule(buildInverter(design, "zeta")).ok());
  ASSERT_TRUE(design.addModule(buildInverter(design, "alpha")).ok());
  ASSERT_TRUE(design.addModule(Module()).ok());  // anonymous: module 2
  const absl::StatusOr<ModId> found = design.top();
  EXPECT_EQ(found.status().code(), absl::StatusCode::kFailedPrecondition);
  EXPECT_EQ(found.status().message(),
            "3 candidate tops: zeta, alpha, module 2; call setTop()");
}

TEST(DesignTopTest, NoCandidateIsAnError)
{
  Design design(kCheckWellFormed);
  Design::SingleWriteScope write(design);
  EXPECT_EQ(design.top().status().code(),
            absl::StatusCode::kFailedPrecondition);  // empty design

  // Only a blackbox and an operator implementation are left.
  const ModId box = *design.addModule(buildInverter(design, "box"));
  ASSERT_TRUE(design.setModuleAttributes(box, {.blackbox = true}).ok());
  addOperatorImpl(design, "add8");
  EXPECT_EQ(design.top().status().code(),
            absl::StatusCode::kFailedPrecondition);
}

TEST(DesignTopTest, EveryModuleInstantiatedLeavesNoTop)
{
  Design design(kCheckWellFormed);
  Design::SingleWriteScope write(design);
  // a and b instantiate each other (ill-formed, but a valid table).
  const ModId a = *design.addModule(buildParent(design, "a", ModId(1)));
  ASSERT_TRUE(design.addModule(buildParent(design, "b", a)).ok());
  const absl::StatusOr<ModId> found = design.top();
  EXPECT_EQ(found.status().code(), absl::StatusCode::kFailedPrecondition);
  EXPECT_NE(found.status().message().find("has no top"),
            std::string_view::npos);
}

TEST(DesignTopTest, UnusedBlackboxesAndOperatorImplsAreNotRoots)
{
  Design design(kCheckWellFormed);
  Design::SingleWriteScope write(design);
  const ModId box = *design.addModule(buildInverter(design, "box"));
  ASSERT_TRUE(design.setModuleAttributes(box, {.blackbox = true}).ok());
  addOperatorImpl(design, "add8");
  const ModId top = *design.addModule(buildInverter(design, "top"));
  EXPECT_EQ(design.top().value(), top);
}

TEST(DesignTopTest, SetTopRejectsBadModulesAndReadOnlyMode)
{
  Design design(kCheckWellFormed);
  Design::SingleWriteScope write(design);
  const ModId top = *design.addModule(buildInverter(design, "top"));
  const ModId gone = *design.addModule(buildInverter(design, "gone"));
  ASSERT_TRUE(design.removeModule(gone).ok());
  const ModId box = *design.addModule(buildInverter(design, "box"));
  ASSERT_TRUE(design.setModuleAttributes(box, {.blackbox = true}).ok());

  EXPECT_EQ(design.setTop(ModId()).code(), absl::StatusCode::kInvalidArgument);
  EXPECT_EQ(design.setTop(ModId(99)).code(),
            absl::StatusCode::kInvalidArgument);
  EXPECT_EQ(design.setTop(gone).code(), absl::StatusCode::kInvalidArgument);
  EXPECT_EQ(design.setTop(box).code(), absl::StatusCode::kInvalidArgument);
  const ModId impl = addOperatorImpl(design, "add8");
  EXPECT_EQ(design.setTop(impl).code(), absl::StatusCode::kInvalidArgument);
  {
    Design::ReadOnlyScope read_only(design);
    EXPECT_EQ(design.setTop(top).code(), absl::StatusCode::kFailedPrecondition);
  }
  EXPECT_TRUE(design.setTop(top).ok());
}

TEST(DesignTopTest, TheStoredTopCannotBeRemoved)
{
  Design design(kCheckWellFormed);
  Design::SingleWriteScope write(design);
  const ModId top = *design.addModule(buildInverter(design, "top"));
  ASSERT_TRUE(design.setTop(top).ok());
  EXPECT_EQ(design.removeModule(top).code(),
            absl::StatusCode::kFailedPrecondition);
  EXPECT_FALSE(design.module(top).isRemoved());

  // replaceModule keeps the ModId, so the top stays.
  ASSERT_TRUE(design.replaceModule(top, buildInverter(design, "top")).ok());
  EXPECT_EQ(design.top().value(), top);

  design.clearTop();
  EXPECT_TRUE(design.removeModule(top).ok());
}

// setTop() checks the module only when it is set; checkWellFormed() reports
// a stored top that later became an operator implementation or a child.
TEST(DesignTopTest, WellFormedChecksTheStoredTop)
{
  Design design(kCheckWellFormed);
  Design::SingleWriteScope write(design);
  const ModId leaf = *design.addModule(buildInverter(design, "leaf"));
  const ModId top = *design.addModule(buildParent(design, "top", leaf));
  const ModId impl = addOperatorImpl(design, "add8");
  EXPECT_TRUE(checkWellFormed(design).ok());  // computed top
  ASSERT_TRUE(design.setTop(top).ok());
  EXPECT_TRUE(checkWellFormed(design).ok());

  ASSERT_TRUE(design.setTop(leaf).ok());
  const absl::Status child = checkWellFormed(design);
  EXPECT_EQ(child.code(), absl::StatusCode::kInvalidArgument);
  EXPECT_NE(child.message().find("the top is instantiated by"),
            std::string_view::npos)
      << child;

  ASSERT_TRUE(design.clearOperatorImpl(impl).ok());
  ASSERT_TRUE(design.setTop(impl).ok());
  EXPECT_TRUE(checkWellFormed(design).ok());
  ASSERT_TRUE(design.setOperatorImpl(impl, adderKey(design)).ok());
  const absl::Status operator_top = checkWellFormed(design);
  EXPECT_EQ(operator_top.code(), absl::StatusCode::kInvalidArgument);
  EXPECT_NE(operator_top.message().find("operator implementation"),
            std::string_view::npos)
      << operator_top;
}

TEST(DesignTopTest, ChecksumCoversTheStoredTop)
{
  Design design(kCheckWellFormed);
  Design::SingleWriteScope write(design);
  const ModId leaf = *design.addModule(buildInverter(design, "leaf"));
  const ModId top = *design.addModule(buildParent(design, "top", leaf));
  const uint64_t unset = design.checksum();
  ASSERT_TRUE(design.setTop(top).ok());
  const uint64_t with_top = design.checksum();
  ASSERT_TRUE(design.setTop(leaf).ok());
  const uint64_t with_leaf = design.checksum();
  EXPECT_NE(unset, with_top);
  EXPECT_NE(with_top, with_leaf);
  EXPECT_NE(unset, with_leaf);
  design.clearTop();
  EXPECT_EQ(design.checksum(), unset);
}

}  // namespace
}  // namespace syn::test
