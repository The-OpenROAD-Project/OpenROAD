// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include "syn/lir/WellFormed.h"

#include <cstddef>
#include <cstdint>
#include <iterator>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "absl/container/flat_hash_map.h"
#include "absl/container/flat_hash_set.h"
#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/strings/str_cat.h"
#include "syn/lir/ConstPool.h"
#include "syn/lir/ConstView.h"
#include "syn/lir/Design.h"
#include "syn/lir/Ids.h"
#include "syn/lir/LibCellTable.h"
#include "syn/lir/Module.h"
#include "syn/lir/ObjAttributes.h"
#include "syn/lir/Op.h"
#include "syn/lir/OperatorPool.h"
#include "syn/lir/OperatorSpec.h"
#include "syn/lir/RegClassPool.h"
#include "syn/lir/RegPinTypes.h"
#include "syn/lir/SourceLoc.h"

namespace syn::lir {
namespace {

constexpr int kMaxLutInputs = 16;

// Control pins are 1 bit; value pins have the register's width.
bool isOneBitPin(RegPinType type)
{
  switch (type) {
    case RegPinType::kClock:
    case RegPinType::kEnable:
    case RegPinType::kSyncReset:
    case RegPinType::kSyncSet:
    case RegPinType::kAsyncReset:
    case RegPinType::kAsyncSet:
    case RegPinType::kAsyncLoad:
    case RegPinType::kScanEnable:
      return true;
    default:
      return false;
  }
}

std::string moduleLabel(const Design& design, const Module& module)
{
  if (!module.name().isValid()) {
    return "module <anonymous>";
  }
  return absl::StrCat("module '", design.names().name(module.name()), "'");
}

class WellFormedChecker
{
 public:
  WellFormedChecker(const Design& design, const Module& module, ModId self)
      : design_(design),
        module_(module),
        self_(self),
        num_consts_(design.consts().size()),
        num_reg_classes_(module.regClasses().size()),
        num_cells_(design.cells().size()),
        num_modules_(design.numModules())
  {
  }

  absl::Status run()
  {
    if (checkPorts() && checkOperatorImpl() && checkObjects()) {
      checkAcyclic();
    }
    if (error_.empty()) {
      return absl::OkStatus();
    }
    return absl::InvalidArgumentError(
        absl::StrCat(moduleLabel(design_, module_), ": ", error_));
  }

 private:
  // Records the first error; always returns false.
  bool fail(std::string message)
  {
    if (error_.empty()) {
      error_ = std::move(message);
    }
    return false;
  }
  bool fail(ObjId id, std::string_view message)
  {
    std::string label = absl::StrCat("object ", id.value());
    if (module_.objName(id).isValid()) {
      absl::StrAppend(
          &label, " '", design_.names().name(module_.objName(id)), "'");
    }
    return fail(
        absl::StrCat(label, " (", opName(module_.op(id)), "): ", message));
  }

  bool expectFaninCount(ObjId id, int64_t actual, int64_t expected)
  {
    if (actual == expected) {
      return true;
    }
    return fail(id,
                absl::StrCat("has ", actual, " fanins, expected ", expected));
  }
  bool expectMinFaninCount(ObjId id, int64_t actual, int64_t min)
  {
    if (actual >= min) {
      return true;
    }
    return fail(
        id, absl::StrCat("has ", actual, " fanins, expected at least ", min));
  }
  // A binary select of `sel_width` bits: 1 << sel_width must fit 32 bits.
  bool expectNarrowSelect(ObjId id, int64_t sel_width)
  {
    if (sel_width >= 0 && sel_width < 32) {
      return true;
    }
    return fail(id, absl::StrCat("select of ", sel_width, " bits is too wide"));
  }
  bool expectFaninWidth(ObjId id, int k, int64_t expected)
  {
    const int actual = module_.width(module_.fanin(id, k));
    if (actual == expected) {
      return true;
    }
    return fail(
        id,
        absl::StrCat(
            "fanin ", k, " has width ", actual, ", expected ", expected));
  }
  bool expectWidth(ObjId id, int64_t expected)
  {
    if (module_.width(id) == expected) {
      return true;
    }
    return fail(
        id,
        absl::StrCat("has width ", module_.width(id), ", expected ", expected));
  }
  // `count` fanins, each as wide as the object.
  bool expectUniform(ObjId id, int64_t count)
  {
    const int n = module_.faninCount(id);
    if (!expectFaninCount(id, n, count)) {
      return false;
    }
    for (int k = 0; k < n; ++k) {
      if (!expectFaninWidth(id, k, module_.width(id))) {
        return false;
      }
    }
    return true;
  }

  bool checkPorts()
  {
    const std::span<const ObjId> inputs = module_.inputs();
    const int num_inputs = static_cast<int>(inputs.size());
    for (int i = 0; i < num_inputs; ++i) {
      const ObjId id = inputs[i];
      if (!module_.isValid(id) || module_.isDead(id)
          || module_.op(id) != Op::kPi
          || module_.data(id) != static_cast<ObjIndex>(i)) {
        return fail(absl::StrCat(
            "input ", i, " is not a live Pi with port ", "index ", i));
      }
    }
    const std::span<const ObjId> outputs = module_.outputs();
    const int num_outputs = static_cast<int>(outputs.size());
    for (int i = 0; i < num_outputs; ++i) {
      const ObjId id = outputs[i];
      if (!module_.isValid(id) || module_.isDead(id)
          || module_.op(id) != Op::kPo
          || module_.data(id) != static_cast<ObjIndex>(i)) {
        return fail(absl::StrCat(
            "output ", i, " is not a live Po with port ", "index ", i));
      }
    }
    if (absl::Status status = checkPortNames(module_); !status.ok()) {
      return fail(std::string(status.message()));
    }
    for (int i = 0; i < num_inputs; ++i) {
      if (!design_.names().contains(module_.inputName(i))) {
        return fail(absl::StrCat("input ",
                                 i,
                                 " has an unknown name id ",
                                 module_.inputName(i).rawIndex()));
      }
    }
    for (int i = 0; i < num_outputs; ++i) {
      if (!design_.names().contains(module_.outputName(i))) {
        return fail(absl::StrCat("output ",
                                 i,
                                 " has an unknown name id ",
                                 module_.outputName(i).rawIndex()));
      }
    }
    if (module_.sourceLoc().isValid()
        && !design_.names().contains(module_.sourceLoc().file())) {
      return fail(
          absl::StrCat("module source location has an unknown file "
                       "name id ",
                       module_.sourceLoc().file().rawIndex()));
    }
    return true;
  }

  // An implementation module's ports follow its spec.
  bool checkOperatorImpl()
  {
    const std::optional<OperatorImpl>& impl = module_.operatorImpl();
    if (!impl.has_value()) {
      return true;
    }
    const OpSpecId spec = impl->key.spec;
    // Defensive: Design::setOperatorImpl() rejects such a spec.
    if (spec.rawIndex() >= design_.operators().size()) {
      return fail(absl::StrCat(
          "operator spec ", spec.rawIndex(), " is not in the pool"));
    }
    const absl::Status status = checkOperatorInterface(
        design_, module_, design_.operators().get(spec));
    return status.ok() || fail(std::string(status.message()));
  }

  bool checkObjects()
  {
    for (const ObjId id : module_.objects()) {
      if (!module_.isDead(id) && !checkObject(id)) {
        return false;
      }
    }
    return checkCellFans() && checkObjAttributes() && checkObjSourceLocs();
  }

  // Each output other than 0 of a Liberty cell is a pin, so it has exactly
  // one kFan, read or not. checkFan() already rejected duplicates.
  bool checkCellFans()
  {
    for (const ObjId id : module_.objects()) {
      if (module_.isDead(id)) {
        continue;
      }
      LibCellId cell;
      if (module_.op(id) == Op::kCell) {
        cell = module_.cellId(id);
      } else if (module_.op(id) == Op::kRegOut
                 && module_.regClass(id).isMapped()) {
        cell = module_.regClass(id).cell();
      } else {
        continue;
      }
      const LibCellInfo& info = design_.cells().info(cell);
      for (int k = 1; k < info.numOutputs(); ++k) {
        if (!fans_.contains({id.value(), static_cast<ObjIndex>(k)})) {
          return fail(
              id,
              absl::StrCat(
                  "cell '", info.name(), "' has no Fan for output ", k));
        }
      }
    }
    return true;
  }

  // Dead objects keep their entries until Editor::compact(). Comparing counts
  // avoids iterating the map.
  bool checkObjAttributes()
  {
    int64_t flagged = 0;
    for (const ObjId id : module_.objects()) {
      if (!module_.record(id).flags.has_attr) {
        continue;
      }
      const ObjAttributes attrs = module_.objAttributes(id);
      if (!attrs.any()) {
        return fail(id, "has_attr flag without attributes");
      }
      ++flagged;
      if (module_.isDead(id)) {
        continue;
      }
      if (attrs.boundaryOpt() != BoundaryOpt::kFull
          && module_.op(id) != Op::kInst) {
        return fail(id,
                    "boundary restriction on an object that is not an "
                    "Inst");
      }
      if (attrs.keepHierarchy() && module_.op(id) != Op::kInst) {
        return fail(id,
                    "keep_hierarchy attribute on an object that is not "
                    "an Inst");
      }
    }
    if (flagged != module_.numObjAttributes()) {
      return fail(absl::StrCat(flagged,
                               " objects have the has_attr flag, but ",
                               "the attribute map has ",
                               module_.numObjAttributes(),
                               " entries"));
    }
    return true;
  }

  // Dead objects keep their locations until Editor::compact().
  bool checkObjSourceLocs()
  {
    int64_t flagged = 0;
    for (const ObjId id : module_.objects()) {
      if (!module_.record(id).flags.has_loc) {
        continue;
      }
      const SourceLoc loc = module_.objSourceLoc(id);
      if (!loc.isValid()) {
        return fail(id, "has_loc flag without a source location");
      }
      if (!design_.names().contains(loc.file())) {
        return fail(id,
                    absl::StrCat("source location has an unknown file "
                                 "name id ",
                                 loc.file().rawIndex()));
      }
      ++flagged;
    }
    if (flagged != module_.numObjSourceLocs()) {
      return fail(absl::StrCat(flagged,
                               " objects have the has_loc flag, but ",
                               "the source location map has ",
                               module_.numObjSourceLocs(),
                               " entries"));
    }
    return true;
  }

  // Many of these checks are sanity checks: ModuleBuilder and Editor never
  // produce such records.
  bool checkObject(ObjId id)
  {
    if (!checkRecord(id)) {
      return false;
    }
    switch (module_.op(id)) {
      case Op::kPi:
      case Op::kPo:
      case Op::kConst:
      case Op::kUndriven:
      case Op::kBuf:
      case Op::kSlice:
      case Op::kConcat:
      case Op::kReplicate:
      case Op::kCast:
      case Op::kInst:
      case Op::kFan:
        return checkStructureOp(id);
      case Op::kRegOut:
      case Op::kLoopOut:
        return checkStateOut(id);
      case Op::kRegIn:
        return checkRegister(id);
      case Op::kLoopIn:
        return checkLoopIn(id);
      case Op::kNot:
      case Op::kAnd:
      case Op::kOr:
      case Op::kXor:
      case Op::kReduceAnd:
      case Op::kReduceOr:
      case Op::kReduceXor:
        return checkBitwiseOp(id);
      case Op::kMux:
      case Op::kPmux:
      case Op::kBmux:
      case Op::kDemux:
      case Op::kTribuf:
      case Op::kTriJoin:
        return checkSelectOp(id);
      case Op::kNeg:
      case Op::kAdd:
      case Op::kSub:
      case Op::kDiv:
      case Op::kMod:
      case Op::kMul:
      case Op::kPow:
      case Op::kEq:
      case Op::kLt:
      case Op::kShl:
      case Op::kShr:
      case Op::kShiftx:
        return checkArithmeticOp(id);
      case Op::kLut:
        return checkLut(id);
      case Op::kCell:
        return checkCell(id);
      default:
        return fail(id, "unhandled op");
    }
  }

  // Op, flags, width and fanin ids, common to every op.
  bool checkRecord(ObjId id)
  {
    const ObjRecord& r = module_.record(id);
    const Op op = r.op;
    if (static_cast<int>(op) >= kNumOps) {
      return fail(absl::StrCat(
          "object ", id.value(), ": unknown op ", static_cast<int>(op)));
    }
    if (isReserved(op)) {
      return fail(id, "reserved op");
    }
    if (r.flags.inline_const && op != Op::kConst) {
      return fail(id, "inline-constant flag on a non-constant");
    }
    if (r.flags.is_signed && !hasSignedFlag(op)) {
      return fail(id, "signed flag on an op without signedness");
    }
    if (r.flags.reserved != 0) {
      return fail(id, "reserved flag bits set");
    }
    if (r.flags.mark1 || r.flags.mark2) {
      return fail(id, "scratch mark left set");
    }
    if (op == Op::kInst ? r.width != 0 : r.width <= 0) {
      return fail(id,
                  op == Op::kInst ? "an instance must have width 0"
                                  : absl::StrCat("width ", r.width));
    }
    return checkFaninIds(id);
  }

  // Ports, constants, wiring, instances and Fans.
  bool checkStructureOp(ObjId id)
  {
    const ObjRecord& r = module_.record(id);
    const std::span<const ObjId> f = module_.fanins(id);
    const int n = static_cast<int>(f.size());
    const int w = r.width;
    auto fw = [&](int k) -> int64_t { return module_.width(f[k]); };
    switch (r.op) {
      case Op::kPi:
        return expectFaninCount(id, n, 0)
               && checkPortIndex(id, module_.inputs());
      case Op::kPo:
        return expectUniform(id, 1) && checkPortIndex(id, module_.outputs());
      case Op::kConst:
        return checkConst(id);
      case Op::kUndriven:
        return expectFaninCount(id, n, 0);
      case Op::kBuf:
        return expectUniform(id, 1);
      case Op::kSlice:
        if (!expectFaninCount(id, n, 1)) {
          return false;
        }
        if (uint64_t{r.data} + static_cast<uint32_t>(w)
            > static_cast<uint64_t>(fw(0))) {
          return fail(id,
                      absl::StrCat("bits [",
                                   r.data,
                                   " +: ",
                                   w,
                                   "] are outside a ",
                                   fw(0),
                                   "-bit fanin"));
        }
        return true;
      case Op::kConcat: {
        if (!expectMinFaninCount(id, n, 2)) {
          return false;
        }
        int64_t total = 0;
        for (int k = 0; k < n; ++k) {
          total += fw(k);
        }
        return total == w
               || fail(id,
                       absl::StrCat(
                           "fanins add up to ", total, " bits, expected ", w));
      }
      case Op::kReplicate:
        if (!expectFaninCount(id, n, 1)) {
          return false;
        }
        if (r.data == 0) {
          return fail(id, "replication count 0");
        }
        return expectWidth(id,
                           static_cast<int64_t>(static_cast<uint64_t>(fw(0))
                                                * uint64_t{r.data}));
      case Op::kCast:
        return expectFaninCount(id, n, 1);
      case Op::kInst:
        return checkInstance(id);
      case Op::kFan:
        return expectFaninCount(id, n, 1) && checkFan(id);
      default:
        return fail(id, "unhandled op");
    }
  }

  // Bitwise ops and reductions.
  bool checkBitwiseOp(ObjId id)
  {
    const int n = static_cast<int>(module_.fanins(id).size());
    switch (module_.op(id)) {
      case Op::kNot:
        return expectUniform(id, 1);
      case Op::kAnd:
      case Op::kOr:
      case Op::kXor:
        return expectMinFaninCount(id, n, 2) && expectUniform(id, n);
      case Op::kReduceAnd:
      case Op::kReduceOr:
      case Op::kReduceXor:
        return expectFaninCount(id, n, 1) && expectWidth(id, 1);
      default:
        return fail(id, "unhandled op");
    }
  }

  // Muxes and tri-state ops.
  bool checkSelectOp(ObjId id)
  {
    const std::span<const ObjId> f = module_.fanins(id);
    const int n = static_cast<int>(f.size());
    const int w = module_.width(id);
    auto fw = [&](int k) -> int64_t { return module_.width(f[k]); };
    switch (module_.op(id)) {
      case Op::kMux:
        return expectFaninCount(id, n, 3) && expectFaninWidth(id, 0, 1)
               && expectFaninWidth(id, 1, w) && expectFaninWidth(id, 2, w);
      case Op::kPmux:
        // [one_hot_sel(s), packed alternatives(s * W), default(W)]
        return expectFaninCount(id, n, 3) && expectFaninWidth(id, 1, fw(0) * w)
               && expectFaninWidth(id, 2, w);
      case Op::kBmux:
        // [binary_sel(s), packed alternatives((1 << s) * W)]
        if (!expectFaninCount(id, n, 2) || !expectNarrowSelect(id, fw(0))) {
          return false;
        }
        return expectFaninWidth(
            id,
            1,
            static_cast<int64_t>(uint64_t{static_cast<uint32_t>(w)}
                                 << static_cast<uint32_t>(fw(0))));
      case Op::kDemux:
        // [binary_sel(s), a(A)] -> (A << s) bits
        if (!expectFaninCount(id, n, 2) || !expectNarrowSelect(id, fw(0))) {
          return false;
        }
        return expectWidth(
            id,
            static_cast<int64_t>(static_cast<uint64_t>(fw(1))
                                 << static_cast<uint32_t>(fw(0))));
      case Op::kTribuf:
        return expectFaninCount(id, n, 2) && expectFaninWidth(id, 0, 1)
               && expectFaninWidth(id, 1, w);
      case Op::kTriJoin:
        return expectMinFaninCount(id, n, 2) && expectUniform(id, n);
      default:
        return fail(id, "unhandled op");
    }
  }

  // Arithmetic, comparisons and shifts.
  bool checkArithmeticOp(ObjId id)
  {
    const std::span<const ObjId> f = module_.fanins(id);
    const int n = static_cast<int>(f.size());
    const int w = module_.width(id);
    switch (module_.op(id)) {
      case Op::kNeg:
        return expectUniform(id, 1);
      case Op::kAdd:
      case Op::kSub:
      case Op::kDiv:
      case Op::kMod:
        return expectUniform(id, 2);
      case Op::kMul:
      case Op::kPow:
        return expectFaninCount(id, n, 2);  // operands keep their widths
      case Op::kEq:
      case Op::kLt:
        return expectFaninCount(id, n, 2) && expectWidth(id, 1)
               && expectFaninWidth(id, 1, module_.width(f[0]));
      case Op::kShl:
      case Op::kShr:
      case Op::kShiftx:
        return expectFaninCount(id, n, 2) && expectFaninWidth(id, 0, w);
      default:
        return fail(id, "unhandled op");
    }
  }

  // Fanins name live objects that carry a value.
  bool checkFaninIds(ObjId id)
  {
    const Op op = module_.op(id);
    const std::span<const ObjId> f = module_.fanins(id);
    const int n = static_cast<int>(f.size());
    for (int k = 0; k < n; ++k) {
      const ObjId fanin = f[k];
      if (!module_.isValid(fanin)) {
        return fail(
            id,
            absl::StrCat(
                "fanin ", k, " (", fanin.value(), ") is not an object"));
      }
      if (module_.isDead(fanin)) {
        return fail(
            id, absl::StrCat("fanin ", k, " (", fanin.value(), ") is dead"));
      }
      const Op fanin_op = module_.op(fanin);
      if (fanin_op == Op::kPo || isStateIn(fanin_op)) {
        return fail(id,
                    absl::StrCat("fanin ",
                                 k,
                                 " (",
                                 fanin.value(),
                                 ", ",
                                 opName(fanin_op),
                                 ") does not carry a value"));
      }
      if (fanin_op == Op::kInst && op != Op::kFan) {
        return fail(id,
                    absl::StrCat("fanin ",
                                 k,
                                 " (",
                                 fanin.value(),
                                 ") is an instance; read it through a "
                                 "Fan"));
      }
    }
    return true;
  }

  bool checkPortIndex(ObjId id, std::span<const ObjId> ports)
  {
    const ObjIndex index = module_.data(id);
    if (index < ports.size() && ports[index] == id) {
      return true;
    }
    return fail(
        id, absl::StrCat("port index ", index, " does not point back to it"));
  }

  bool checkConst(ObjId id)
  {
    const ObjRecord& r = module_.record(id);
    if (!expectFaninCount(id, module_.faninCount(id), 0)) {
      return false;
    }
    const bool is_inline = r.flags.inline_const;
    if (r.width <= kMaxInlineConstWidth) {
      if (!is_inline) {
        return fail(id,
                    absl::StrCat("a constant of at most ",
                                 kMaxInlineConstWidth,
                                 " bits must be inline"));
      }
      const uint32_t mask
          = r.width == 32 ? ~uint32_t{0}
                          : (uint32_t{1} << static_cast<uint32_t>(r.width)) - 1;
      if (r.data > mask || r.fanin_begin > mask) {
        return fail(id, "inline value has bits above its width");
      }
      if ((r.data & r.fanin_begin) != 0) {
        return fail(id, "inline value has both bits and x_mask set");
      }
      return true;
    }
    if (is_inline) {
      return fail(id,
                  absl::StrCat("a constant wider than ",
                               kMaxInlineConstWidth,
                               " bits cannot be inline"));
    }
    if (r.data >= num_consts_) {
      return fail(id, absl::StrCat("ConstId ", r.data, " is not in the pool"));
    }
    const int pooled
        = design_.consts().get(ConstId(static_cast<uint32_t>(r.data))).width();
    return pooled == r.width
           || fail(id, absl::StrCat("pooled value has width ", pooled));
  }

  bool checkInstance(ObjId id)
  {
    const ObjIndex child_id = module_.data(id);
    if (std::cmp_greater_equal(child_id, num_modules_)) {
      return fail(id, absl::StrCat("ModId ", child_id, " does not exist"));
    }
    if (self_.isValid() && child_id == self_.value()) {
      return fail(id, "instantiates its own module");
    }
    const Module& child
        = design_.module(ModId(static_cast<uint32_t>(child_id)));
    if (child.isRemoved()) {
      return fail(id, absl::StrCat("module ", child_id, " is removed"));
    }
    const std::span<const ObjId> inputs = child.inputs();
    const int num_inputs = static_cast<int>(inputs.size());
    if (!expectFaninCount(id, module_.faninCount(id), num_inputs)) {
      return false;
    }
    for (int k = 0; k < num_inputs; ++k) {
      if (!expectFaninWidth(id, k, child.width(inputs[k]))) {
        return false;
      }
    }
    return true;
  }

  bool checkFan(ObjId id)
  {
    const ObjId owner = module_.fanin(id, 0);
    switch (module_.op(owner)) {
      case Op::kInst:
        return checkInstFan(id, owner);
      case Op::kRegOut:
        return checkRegFan(id, owner);
      case Op::kCell:
        return checkCellFan(id, owner);
      default:
        return fail(id, "owner is not an Inst, Cell or RegOut");
    }
  }

  // Records that `owner` has a Fan for `output`; fails on a second one.
  bool claimFan(ObjId id,
                ObjId owner,
                ObjIndex output,
                std::string_view owner_kind,
                const absl::AlphaNum& output_name)
  {
    if (fans_.insert({owner.value(), output}).second) {
      return true;
    }
    return fail(id,
                absl::StrCat(owner_kind,
                             " ",
                             owner.value(),
                             " already has a Fan for output ",
                             output_name));
  }

  bool checkInstFan(ObjId id, ObjId owner)
  {
    // Defensive: the owner, checked first, already fails.
    if (module_.data(owner) >= static_cast<ObjIndex>(num_modules_)) {
      return fail(id, "owner instance names no module");
    }
    const Module& child
        = design_.module(ModId(static_cast<uint32_t>(module_.data(owner))));
    const ObjIndex index = module_.data(id);
    if (index >= child.outputs().size()) {
      return fail(
          id, absl::StrCat("output ", index, " does not exist on the child"));
    }
    if (!claimFan(id, owner, index, "instance", index)) {
      return false;
    }
    return expectWidth(id, child.width(child.outputs()[index]));
  }

  // Fan.data is a RegOutput for a generic register (Q is the owner itself),
  // or a Liberty output index for a mapped one.
  bool checkRegFan(ObjId id, ObjId owner)
  {
    const ObjIndex output = module_.data(id);
    // Defensive: the owner, checked first, already fails.
    if (module_.data(owner) >= num_reg_classes_) {
      return fail(id, "owner register has an unknown RegClassId");
    }
    const RegClass c = module_.regClass(owner);
    if (c.isMapped()) {
      if (c.cell().rawIndex() >= num_cells_) {
        return fail(id,
                    absl::StrCat("owner register LibCellId ",
                                 c.cell().rawIndex(),
                                 " is not in the table"));
      }
      const LibCellInfo& info = design_.cells().info(c.cell());
      if (std::cmp_greater_equal(output, info.numOutputs())) {
        return fail(id,
                    absl::StrCat("output ",
                                 output,
                                 " does not exist on cell '",
                                 info.name(),
                                 "'"));
      }
      if (output == 0) {
        return fail(id, "output 0 is the register itself");
      }
      return claimFan(id, owner, output, "register", output)
             && expectWidth(id, 1);
    }
    if (output == 0 || output >= static_cast<ObjIndex>(kNumRegOutputs)) {
      return fail(
          id,
          absl::StrCat("register output ", output, " is not QN or scan-out"));
    }
    return claimFan(id,
                    owner,
                    output,
                    "register",
                    regOutputName(static_cast<RegOutput>(output)))
           && expectWidth(id, module_.width(owner));
  }

  bool checkCellFan(ObjId id, ObjId owner)
  {
    const ObjIndex cell_id = module_.data(owner);
    if (cell_id >= num_cells_) {
      return fail(
          id,
          absl::StrCat(
              "owner cell LibCellId ", cell_id, " is not in the table"));
    }
    const LibCellInfo& info
        = design_.cells().info(LibCellId(static_cast<uint32_t>(cell_id)));
    const ObjIndex output = module_.data(id);
    if (output == 0 || std::cmp_greater_equal(output, info.numOutputs())) {
      return fail(id,
                  absl::StrCat("cell output ",
                               output,
                               " is out of range for '",
                               info.name(),
                               "'"));
    }
    return claimFan(id, owner, output, "cell", output) && expectWidth(id, 1);
  }

  // RegOut / LoopOut: no fanins; the pair link (stateIn()) names the input.
  bool checkStateOut(ObjId id)
  {
    const Op in_op = module_.op(id) == Op::kRegOut ? Op::kRegIn : Op::kLoopIn;
    // faninCount() would misread the pair link as an escape header.
    if (!expectFaninCount(id, module_.record(id).fanin_count, 0)) {
      return false;
    }
    const ObjId in = module_.stateIn(id);
    if (!module_.isValid(in) || module_.isDead(in) || module_.op(in) != in_op
        || module_.data(in) != id.value()) {
      return fail(
          id,
          absl::StrCat(
              "pair link (", in.value(), ") is not its ", opName(in_op)));
    }
    if (module_.op(id) == Op::kRegOut && module_.data(id) >= num_reg_classes_) {
      return fail(
          id,
          absl::StrCat("RegClassId ", module_.data(id), " is not in the pool"));
    }
    if (module_.op(id) == Op::kLoopOut && module_.data(id) != 0) {
      return fail(id,
                  absl::StrCat("data is ", module_.data(id), ", must be 0"));
    }
    return true;
  }

  // RegIn / LoopIn: data links back to the output, same width.
  bool checkPairLink(ObjId id, Op out_op)
  {
    const ObjId out(module_.data(id));
    if (!module_.isValid(out) || module_.isDead(out)
        || module_.op(out) != out_op || module_.stateIn(out) != id) {
      return fail(id, absl::StrCat("is not paired with a ", opName(out_op)));
    }
    return expectWidth(id, module_.width(out));
  }

  bool checkLoopIn(ObjId id)
  {
    return checkPairLink(id, Op::kLoopOut) && expectUniform(id, 1);
  }

  bool checkRegister(ObjId id)
  {
    if (!checkPairLink(id, Op::kRegOut)) {
      return false;
    }
    const ObjId q(module_.data(id));
    if (module_.data(q) >= num_reg_classes_) {
      return fail(
          id,
          absl::StrCat("RegClassId ", module_.data(q), " is not in the pool"));
    }
    const RegClass c = module_.regClasses().get(
        RegClassId(static_cast<uint32_t>(module_.data(q))));
    const int w = module_.width(id);
    if (c.isMapped()) {
      return checkMappedRegister(id, c);
    }
    if (!checkGenericRegClass(id, c)) {
      return false;
    }
    if (!checkRegInit(id, c.init(), w)) {
      return false;
    }
    if (!checkRegPins(id, c, w)) {
      return false;
    }
    if (!checkResetNormalForm(id, c, w)) {
      return false;
    }
    if (c.has(RegPinType::kAsyncLoad) != c.has(RegPinType::kAsyncData)) {
      return fail(id, "async load and async data must come together");
    }
    if (c.has(RegPinType::kScanEnable) != c.has(RegPinType::kScanIn)) {
      return fail(id, "scan enable and scan in must come together");
    }
    return true;
  }

  // A register mapped to a Liberty cell: 1 bit, one 1-bit fanin per input.
  bool checkMappedRegister(ObjId id, const RegClass& c)
  {
    if (!expectWidth(id, 1)) {
      return false;
    }
    if (c.cell().rawIndex() >= num_cells_) {
      return fail(
          id,
          absl::StrCat(
              "LibCellId ", c.cell().rawIndex(), " is not in the table"));
    }
    const LibCellInfo& info = design_.cells().info(c.cell());
    if (info.kind() != LibCellKind::kRegister) {
      return fail(id,
                  absl::StrCat("mapped register cell '",
                               info.name(),
                               "' is a ",
                               libCellKindName(info.kind()),
                               " cell, not a register"));
    }
    if (!expectFaninCount(id, module_.faninCount(id), info.numInputs())) {
      return false;
    }
    for (int k = 0; k < info.numInputs(); ++k) {
      if (!expectFaninWidth(id, k, 1)) {
        return false;
      }
    }
    return true;
  }

  // The pin set and flags of a generic register's class.
  bool checkGenericRegClass(ObjId id, const RegClass& c)
  {
    if ((c.pins() >> static_cast<uint32_t>(kNumRegPinTypes)) != 0) {
      return fail(id, "RegClass has unknown pins");
    }
    if (c.flags().reserved != 0) {
      return fail(id, "RegClass has reserved flags set");
    }
    if (!c.has(RegPinType::kData) || !c.has(RegPinType::kClock)) {
      return fail(id, "RegClass lacks a data or clock pin");
    }
    if (c.isLatch()) {
      constexpr uint32_t kLatchPins = uint32_t{regPinBit(RegPinType::kData)}
                                      | regPinBit(RegPinType::kClock)
                                      | regPinBit(RegPinType::kAsyncReset)
                                      | regPinBit(RegPinType::kAsyncSet)
                                      | regPinBit(RegPinType::kAsyncResetValue);
      if ((c.pins() & ~kLatchPins) != 0) {
        return fail(id,
                    "a latch has only data, gate and async reset/set "
                    "pins");
      }
    }
    return true;
  }

  // One fanin per pin of `c`, 1 bit for control pins, else `w` bits.
  bool checkRegPins(ObjId id, const RegClass& c, int w)
  {
    if (!expectFaninCount(id, module_.faninCount(id), c.numPins())) {
      return false;
    }
    for (int t = 0; t < kNumRegPinTypes; ++t) {
      const RegPinType type = static_cast<RegPinType>(t);
      if (!c.has(type)) {
        continue;
      }
      const int k = c.pinIndex(type);
      const int want = isOneBitPin(type) ? 1 : w;
      if (module_.width(module_.fanin(id, k)) != want) {
        return fail(id,
                    absl::StrCat("pin ",
                                 regPinTypeName(type),
                                 " has width ",
                                 module_.width(module_.fanin(id, k)),
                                 ", expected ",
                                 want));
      }
    }
    return true;
  }

  bool checkResetNormalForm(ObjId id, const RegClass& c, int w)
  {
    struct ResetPins
    {
      RegPinType reset;
      RegPinType value;
    };
    for (const ResetPins pins : {
             ResetPins{
                 .reset = RegPinType::kSyncReset,
                 .value = RegPinType::kSyncResetValue,
             },
             ResetPins{
                 .reset = RegPinType::kAsyncReset,
                 .value = RegPinType::kAsyncResetValue,
             },
         }) {
      if (!c.has(pins.value)) {
        continue;
      }
      const std::string_view name = regPinTypeName(pins.value);
      if (!c.has(pins.reset)) {
        return fail(id, absl::StrCat("pin ", name, " without its reset"));
      }
      if (w == 1) {
        return fail(
            id,
            absl::StrCat(
                "a bit register uses reset or set, not ", "pin ", name));
      }
      const ObjId value = module_.fanin(id, c.pinIndex(pins.value));
      if (module_.op(value) != Op::kConst) {
        return fail(id, absl::StrCat("pin ", name, " is not a constant"));
      }
      // In an unordered module the constant may come after the register and
      // not be checked yet; check it before reading its value.
      if (!checkConst(value)) {
        return false;
      }
      const ConstView v = module_.constValue(value, design_.consts());
      if (v.isFill(LogicValue::kZero)) {
        return fail(
            id,
            absl::StrCat("pin ", name, " is all 0; use the ", "reset alone"));
      }
      if (v.isFill(LogicValue::kOne)) {
        return fail(id, absl::StrCat("pin ", name, " is all 1; use a set"));
      }
    }
    return true;
  }

  // Initial state of a generic register of width `w`: inline values for at
  // most kMaxInlineConstWidth bits, else a pooled constant of width `w`.
  bool checkRegInit(ObjId id, const RegInit& init, int w)
  {
    if (!init.isPooled()) {
      if (init.isNone()) {
        return true;
      }
      if (w > kMaxInlineConstWidth) {
        return fail(id,
                    absl::StrCat("a ",
                                 w,
                                 "-bit register needs a pooled "
                                 "initial state"));
      }
      const uint32_t mask = w == 32
                                ? ~uint32_t{0}
                                : (uint32_t{1} << static_cast<uint32_t>(w)) - 1;
      if ((init.known() & ~mask) != 0 || (init.value() & ~init.known()) != 0) {
        return fail(id, "initial state has bits outside the register");
      }
      return true;
    }
    if (init.constId().rawIndex() >= num_consts_) {
      return fail(id,
                  absl::StrCat("initial state ConstId ",
                               init.constId().rawIndex(),
                               " is not in the pool"));
    }
    const ConstView v = design_.consts().get(init.constId());
    if (v.width() != w) {
      return fail(
          id,
          absl::StrCat("initial state has ", v.width(), " bits, expected ", w));
    }
    return true;
  }

  bool checkLut(ObjId id)
  {
    const std::span<const ObjId> f = module_.fanins(id);
    if (!expectWidth(id, 1)) {
      return false;
    }
    const int n = static_cast<int>(f.size());
    if (n == 0 || n > kMaxLutInputs) {
      return fail(
          id,
          absl::StrCat("has ", n, " inputs, expected 1 to ", kMaxLutInputs));
    }
    for (int k = 0; k < n; ++k) {
      if (!expectFaninWidth(id, k, 1)) {
        return false;
      }
    }
    const ObjIndex table = module_.data(id);
    if (table >= num_consts_) {
      return fail(
          id,
          absl::StrCat("truth table ConstId ", table, " is not in the pool"));
    }
    const int want = static_cast<int>(uint32_t{1} << static_cast<uint32_t>(n));
    const int width
        = design_.consts().get(ConstId(static_cast<uint32_t>(table))).width();
    return width == want
           || fail(id,
                   absl::StrCat(
                       "truth table has ", width, " bits, expected ", want));
  }

  bool checkCell(ObjId id)
  {
    if (!expectWidth(id, 1)) {
      return false;
    }
    const ObjIndex cell_id = module_.data(id);
    if (cell_id >= num_cells_) {
      return fail(id,
                  absl::StrCat("LibCellId ", cell_id, " is not in the table"));
    }
    const LibCellInfo& info
        = design_.cells().info(LibCellId(static_cast<uint32_t>(cell_id)));
    if (info.numOutputs() == 0) {
      return fail(id, absl::StrCat("cell '", info.name(), "' has no outputs"));
    }
    if (info.kind() == LibCellKind::kRegister) {
      return fail(id,
                  absl::StrCat("cell '",
                               info.name(),
                               "' is sequential; use a mapped RegOut/RegIn "
                               "pair"));
    }
    if (info.kind() == LibCellKind::kBlackBox) {
      return fail(id,
                  absl::StrCat("cell '",
                               info.name(),
                               "' is not a combinational or clock-gate cell; "
                               "instantiate it as a black box"));
    }
    if (!expectFaninCount(id, module_.faninCount(id), info.numInputs())) {
      return false;
    }
    for (int k = 0; k < info.numInputs(); ++k) {
      if (!expectFaninWidth(id, k, 1)) {
        return false;
      }
    }
    return true;
  }

  bool checkAcyclic()
  {
    if (module_.isOrdered()) {
      for (const ObjId id : module_.objects()) {
        if (module_.isDead(id)) {
          continue;
        }
        for (const ObjId fanin : module_.fanins(id)) {
          if (fanin.value() >= id.value()) {
            return fail(id,
                        absl::StrCat("module is marked ordered but fanin ",
                                     fanin.value(),
                                     " is not below it"));
          }
        }
      }
      return true;
    }

    // Kahn's algorithm over the live objects.
    const ObjIndex size = module_.size();
    std::vector<int> pending(size, 0);  // unprocessed comb fanins
    std::vector<ObjIndex> fanout_begin(size + 1, 0);
    for (const ObjId id : module_.objects()) {
      if (module_.isDead(id)) {
        continue;
      }
      for (const ObjId fanin : module_.fanins(id)) {
        ++pending[id.value()];
        ++fanout_begin[fanin.value() + 1];
      }
    }
    for (ObjIndex i = 0; i < size; ++i) {
      fanout_begin[i + 1] += fanout_begin[i];
    }
    std::vector<ObjIndex> fanouts(fanout_begin[size]);
    std::vector<ObjIndex> cursor(fanout_begin.begin(), fanout_begin.end() - 1);
    for (const ObjId id : module_.objects()) {
      if (module_.isDead(id)) {
        continue;
      }
      for (const ObjId fanin : module_.fanins(id)) {
        fanouts[cursor[fanin.value()]++] = id.value();
      }
    }
    std::vector<ObjIndex> ready;
    for (const ObjId id : module_.objects()) {
      if (!module_.isDead(id) && pending[id.value()] == 0) {
        ready.push_back(id.value());
      }
    }
    while (!ready.empty()) {
      const ObjIndex id = ready.back();
      ready.pop_back();
      for (ObjIndex e = fanout_begin[id]; e < fanout_begin[id + 1]; ++e) {
        if (--pending[fanouts[e]] == 0) {
          ready.push_back(fanouts[e]);
        }
      }
    }
    for (const ObjId start : module_.objects()) {
      if (module_.isDead(start) || pending[start.value()] == 0) {
        continue;
      }
      // Walk back through unprocessed fanins; after `size` steps the walk
      // is on the cycle.
      ObjId on_cycle = start;
      for (ObjIndex step = 0; step < size; ++step) {
        for (const ObjId fanin : module_.fanins(on_cycle)) {
          if (pending[fanin.value()] != 0) {
            on_cycle = fanin;
            break;
          }
        }
      }
      return fail(on_cycle, "is on a combinational cycle");
    }
    return true;
  }

  const Design& design_;
  const Module& module_;
  const ModId self_;  // invalid if the module is not installed
  const size_t num_consts_;
  const size_t num_reg_classes_;
  const size_t num_cells_;
  // (owner, output) of the kFans seen so far.
  absl::flat_hash_set<std::pair<ObjIndex, ObjIndex>> fans_;
  const int num_modules_;
  std::string error_;
};

// The instance hierarchy must be a DAG. Iterative DFS with colors. Child
// ids are in range: the module checks ran first.
absl::Status checkHierarchyIsAcyclic(const Design& design)
{
  const int num_modules = design.numModules();
  std::vector<std::vector<int>> children(num_modules);
  for (int i = 0; i < num_modules; ++i) {
    const Module& module = design.module(ModId(static_cast<uint32_t>(i)));
    for (const ObjId id : module.objects()) {
      if (!module.isDead(id) && module.op(id) == Op::kInst) {
        children[i].push_back(static_cast<int>(module.data(id)));
      }
    }
  }
  enum class VisitState : uint8_t
  {
    kWhite,
    kGray,
    kBlack,
  };
  std::vector<VisitState> color(num_modules, VisitState::kWhite);
  std::vector<std::pair<int, int>> stack;  // module, next child
  for (int root = 0; root < num_modules; ++root) {
    if (color[root] != VisitState::kWhite) {
      continue;
    }
    stack.emplace_back(root, 0);
    color[root] = VisitState::kGray;
    while (!stack.empty()) {
      auto& [module, next] = stack.back();
      if (next == std::ssize(children[module])) {
        color[module] = VisitState::kBlack;
        stack.pop_back();
        continue;
      }
      const int child = children[module][next++];
      if (color[child] == VisitState::kGray) {
        return absl::InvalidArgumentError(absl::StrCat(
            moduleLabel(design,
                        design.module(ModId(static_cast<uint32_t>(child)))),
            ": the instance hierarchy has a cycle through it"));
      }
      if (color[child] == VisitState::kWhite) {
        color[child] = VisitState::kGray;
        stack.emplace_back(child, 0);
      }
    }
  }
  return absl::OkStatus();
}

// A module under a dont_touch instance or module must be marked frozen, or
// passes would optimize it. A stale mark only costs optimization.
absl::Status checkFrozenMarks(const Design& design)
{
  const std::vector<uint8_t> frozen = design.computeFrozenModules();
  const int n = static_cast<int>(frozen.size());
  for (int i = 0; i < n; ++i) {
    const Module& module = design.module(ModId(static_cast<uint32_t>(i)));
    if (frozen[i] != 0 && !module.attributes().frozen_by_parent) {
      return absl::InvalidArgumentError(absl::StrCat(
          moduleLabel(design, module),
          ": it is under a dont_touch instance or module but not marked "
          "frozen (run Design::markFrozenModules)"));
    }
  }
  return absl::OkStatus();
}

// setTop() checks the top only when it is set; a computed top qualifies by
// construction.
absl::Status checkTop(const Design& design)
{
  const absl::StatusOr<ModId> top = design.top();
  if (!top.ok()) {
    return absl::OkStatus();
  }
  const Module& top_module = design.module(*top);
  if (top_module.attributes().blackbox || top_module.isOperatorImpl()) {
    return absl::InvalidArgumentError(
        absl::StrCat(moduleLabel(design, top_module),
                     ": the top is a blackbox or an operator implementation"));
  }
  const int num_modules = design.numModules();
  for (int i = 0; i < num_modules; ++i) {
    const Module& module = design.module(ModId(static_cast<uint32_t>(i)));
    for (const ObjId id : module.objects()) {
      if (!module.isDead(id) && module.op(id) == Op::kInst
          && module.instModule(id) == *top) {
        return absl::InvalidArgumentError(
            absl::StrCat(moduleLabel(design, top_module),
                         ": the top is instantiated by ",
                         moduleLabel(design, module)));
      }
    }
  }
  return absl::OkStatus();
}

}  // namespace

absl::Status checkWellFormed(const Design& design, const Module& module)
{
  return WellFormedChecker(design, module, ModId()).run();
}

absl::Status checkWellFormed(const Design& design)
{
  const int num_modules = design.numModules();
  for (int i = 0; i < num_modules; ++i) {
    const auto id = ModId(static_cast<uint32_t>(i));
    const absl::Status status
        = WellFormedChecker(design, design.module(id), id).run();
    if (!status.ok()) {
      return status;
    }
  }
  if (absl::Status status = checkHierarchyIsAcyclic(design); !status.ok()) {
    return status;
  }
  if (absl::Status status = checkFrozenMarks(design); !status.ok()) {
    return status;
  }
  return checkTop(design);
}

absl::Status checkOperatorInterface(const Design& design,
                                    const Module& module,
                                    const OpSpec& spec)
{
  if (absl::Status status = checkOpSpec(spec); !status.ok()) {
    return status;
  }
  const std::span<const ObjId> inputs = module.inputs();
  const int ports = numInputPorts(spec);
  if (std::ssize(inputs) != ports) {
    return absl::InvalidArgumentError(
        absl::StrCat("operator implementation has ",
                     inputs.size(),
                     " inputs, its spec has ",
                     ports,
                     " port operands"));
  }
  const size_t num_consts = design.consts().size();
  int port = 0;
  const int num_operands = static_cast<int>(spec.operands.size());
  for (int k = 0; k < num_operands; ++k) {
    const OpOperand& operand = spec.operands[k];
    if (!isPort(operand)) {
      if (operand.constant.rawIndex() >= num_consts) {
        return absl::InvalidArgumentError(absl::StrCat(
            "operator spec: constant operand ", k, " is not in the pool"));
      }
      const int width = design.consts().get(operand.constant).width();
      if (width != operand.width) {
        return absl::InvalidArgumentError(
            absl::StrCat("operator spec: constant operand ",
                         k,
                         " has width ",
                         width,
                         ", expected ",
                         operand.width));
      }
      continue;
    }
    const int width = module.width(inputs[port]);
    if (width != operand.width) {
      return absl::InvalidArgumentError(
          absl::StrCat("operator implementation: input ",
                       port,
                       " has width ",
                       width,
                       ", its operand ",
                       k,
                       " has width ",
                       operand.width));
    }
    ++port;
  }
  const std::span<const ObjId> outputs = module.outputs();
  if (outputs.size() != 1) {
    return absl::InvalidArgumentError(
        absl::StrCat("operator implementation has ",
                     outputs.size(),
                     " outputs, expected 1"));
  }
  if (module.width(outputs[0]) != spec.result_width) {
    return absl::InvalidArgumentError(
        absl::StrCat("operator implementation: output has width ",
                     module.width(outputs[0]),
                     ", its spec's result has width ",
                     spec.result_width));
  }
  return absl::OkStatus();
}

absl::Status checkPortNames(const Module& module)
{
  struct Port
  {
    NameId name;
    bool output = false;
    int index = 0;
  };
  auto label = [](const Port& port) {
    return absl::StrCat(port.output ? "output " : "input ", port.index);
  };
  std::vector<Port> ports;
  ports.reserve(module.inputs().size() + module.outputs().size());
  const int num_inputs = static_cast<int>(module.inputs().size());
  for (int i = 0; i < num_inputs; ++i) {
    ports.push_back(
        Port{.name = module.inputName(i), .output = false, .index = i});
  }
  const int num_outputs = static_cast<int>(module.outputs().size());
  for (int i = 0; i < num_outputs; ++i) {
    ports.push_back(
        Port{.name = module.outputName(i), .output = true, .index = i});
  }
  for (const Port& port : ports) {
    if (!port.name.isValid()) {
      return absl::InvalidArgumentError(
          absl::StrCat(label(port), " has no name"));
    }
  }
  // The first clash in port order, whatever the NameId values.
  absl::flat_hash_map<NameId, const Port*> first_with_name;
  for (const Port& port : ports) {
    const auto [it, inserted] = first_with_name.try_emplace(port.name, &port);
    if (!inserted) {
      return absl::InvalidArgumentError(absl::StrCat(
          label(*it->second), " and ", label(port), " have the same name"));
    }
  }
  return absl::OkStatus();
}

}  // namespace syn::lir
