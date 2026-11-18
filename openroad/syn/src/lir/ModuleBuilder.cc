// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include "syn/lir/ModuleBuilder.h"

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <limits>  // IWYU pragma: keep (used in asserts)
#include <span>
#include <string>
#include <utility>
#include <vector>

#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/strings/str_cat.h"
#include "syn/lir/ConstPool.h"
#include "syn/lir/ConstView.h"
#include "syn/lir/Design.h"
#include "syn/lir/Ids.h"
#include "syn/lir/Module.h"
#include "syn/lir/ObjAttributes.h"
#include "syn/lir/Op.h"
#include "syn/lir/SourceLoc.h"
#include "syn/lir/WellFormed.h"

namespace syn::lir {

namespace {

// Ops that add() refuses because they have dedicated methods or are not
// implemented yet.
bool hasDedicatedMethod(Op op)
{
  return op == Op::kPi || op == Op::kPo || op == Op::kConst || isStateOut(op)
         || isStateIn(op);
}

bool sameWidths(const Module& a,
                std::span<const ObjId> a_ports,
                const Module& b,
                std::span<const ObjId> b_ports)
{
  if (a_ports.size() != b_ports.size()) {
    return false;
  }
  const int n = static_cast<int>(a_ports.size());
  for (int i = 0; i < n; ++i) {
    if (a.width(a_ports[i]) != b.width(b_ports[i])) {
      return false;
    }
  }
  return true;
}

}  // namespace

ModuleBuilder::ModuleBuilder(NameId name)
{
  module_.name_ = name;
}

ModuleBuilder::ModuleBuilder(const Module& src) : src_(&src)
{
  module_.name_ = src.name();
  module_.attributes_ = src.attributes();
  module_.source_loc_ = src.sourceLoc();
  // install() keeps the interface, so the operator spec still holds.
  module_.impl_ = src.impl_;
  // Same class ids, so copy() keeps a register's class as is.
  module_.reg_classes_ = src.reg_classes_;
  src_map_.assign(src.size(), ObjId());
  const int num_inputs = static_cast<int>(src.inputs().size());
  for (int port = 0; port < num_inputs; ++port) {
    const ObjId pi = src.inputs()[port];
    const ObjId id
        = addPi(src.inputName(port), src.width(pi), src.signedness(pi));
    if (!id.isValid()) {
      return;
    }
    src_map_[pi.value()] = id;
    addObjAttributes(id, src.objAttributes(pi));
    setObjSourceLoc(id, src.objSourceLoc(pi));
  }
}

void ModuleBuilder::reserve(int64_t objects, int64_t fanin_slots)
{
  assert(objects >= 0 && fanin_slots >= 0);
  module_.objs_.reserve(static_cast<size_t>(objects));
  module_.fanins_.reserve(static_cast<size_t>(fanin_slots));
}

void ModuleBuilder::setModuleName(NameId name)
{
  module_.name_ = name;
}

void ModuleBuilder::setModuleAttributes(const ModuleAttributes& attributes)
{
  module_.attributes_ = attributes;
}

void ModuleBuilder::setModuleSourceLoc(SourceLoc loc)
{
  module_.source_loc_ = loc;
}

ObjId ModuleBuilder::addPi(NameId name, int width, Signedness signedness)
{
  if (!ok()) {
    return ObjId();
  }
  assert(module_.inputs_.size()
         <= static_cast<size_t>(std::numeric_limits<int>::max()));
  const int port = static_cast<int>(module_.inputs_.size());
  const ObjId id = append(Op::kPi,
                          width,
                          {},
                          static_cast<ObjIndex>(port),
                          signedness == Signedness::kSigned);
  if (id.isValid()) {
    module_.inputs_.push_back(id);
    module_.input_names_.push_back(name);
  }
  return id;
}

ObjId ModuleBuilder::addPo(NameId name, ObjId driver)
{
  if (!ok() || !checkFanins(std::span(&driver, 1))) {
    return ObjId();
  }
  assert(module_.outputs_.size()
         <= static_cast<size_t>(std::numeric_limits<int>::max()));
  const int port = static_cast<int>(module_.outputs_.size());
  if (!name.isValid() && src_ != nullptr
      && port < std::ssize(src_->outputs())) {
    name = src_->outputName(port);
  }
  const ObjId id = append(Op::kPo,
                          width(driver),
                          std::span(&driver, 1),
                          static_cast<ObjIndex>(port),
                          false);
  if (id.isValid()) {
    module_.outputs_.push_back(id);
    module_.output_names_.push_back(name);
  }
  return id;
}

ObjId ModuleBuilder::add(Op op,
                         int width,
                         std::span<const ObjId> fanins,
                         uint32_t data,
                         Signedness signedness)
{
  if (!ok()) {
    return ObjId();
  }
  if (static_cast<int>(op) >= kNumOps || hasDedicatedMethod(op)
      || isReserved(op)) {
    fail(absl::StrCat("add() cannot create a '", opName(op), "' object"));
    return ObjId();
  }
  if (!checkFanins(fanins)) {
    return ObjId();
  }
  return append(op, width, fanins, data, signedness == Signedness::kSigned);
}

ObjId ModuleBuilder::addConst(int width, uint32_t bits, uint32_t x_mask)
{
  assert(width >= 0);
  if (!ok()) {
    return ObjId();
  }
  if (width > kMaxInlineConstWidth) {
    fail(absl::StrCat("addConst(): width ",
                      width,
                      " is above ",
                      kMaxInlineConstWidth,
                      "; pass a ConstPool"));
    return ObjId();
  }
  const ObjId id = module_.size() >= kMaxObjects
                       ? ObjId()
                       : module_.appendInlineConst(width, bits, x_mask);
  if (!id.isValid()) {
    failTooLarge();
  }
  return id;
}

ObjId ModuleBuilder::addConst(ConstView value, ConstPool& pool)
{
  if (!ok()) {
    return ObjId();
  }
  if (value.width() <= kMaxInlineConstWidth) {
    const int n = value.numWords();
    return addConst(value.width(),
                    n == 0 ? 0 : static_cast<uint32_t>(value.bits()[0]),
                    n == 0 ? 0 : static_cast<uint32_t>(value.xMask()[0]));
  }
  const ConstId id = pool.intern(value.width(), value.bits(), value.xMask());
  if (!id.isValid()) {
    fail("addConst(): constant pool is read-only: cannot add a new constant",
         absl::StatusCode::kFailedPrecondition);
    return ObjId();
  }
  return append(Op::kConst, value.width(), {}, id.rawIndex(), false);
}

ObjId ModuleBuilder::copyConst(const Module& from, ObjId obj)
{
  if (!ok()) {
    return ObjId();
  }
  if (!from.isValid(obj) || from.op(obj) != Op::kConst) {
    fail(absl::StrCat(
        "copyConst(): object ", obj.value(), " is not a constant"));
    return ObjId();
  }
  const ObjRecord& r = from.record(obj);
  return r.flags.inline_const
             ? addConst(r.width,
                        static_cast<uint32_t>(r.data),
                        static_cast<uint32_t>(r.fanin_begin))
             : append(Op::kConst, r.width, {}, r.data, false);  // pooled
}

ObjId ModuleBuilder::addRegisterOut(int width,
                                    RegClassId reg_class,
                                    Signedness signedness)
{
  if (!ok()) {
    return ObjId();
  }
  if (!reg_class.isValid()) {
    failInvalidRegClass("addRegisterOut()");
    return ObjId();
  }
  // The pair link (stateIn()) stays unset until addRegisterIn().
  const ObjId id = append(Op::kRegOut,
                          width,
                          {},
                          reg_class.rawIndex(),
                          signedness == Signedness::kSigned);
  ++open_pairs_;
  return id;
}

void ModuleBuilder::setRegisterClass(ObjId reg_out, RegClassId reg_class)
{
  if (!ok()) {
    return;
  }
  if (isPlaceholder(reg_out) || !module_.isValid(reg_out)
      || module_.op(reg_out) != Op::kRegOut) {
    fail(absl::StrCat("setRegisterClass(): object ",
                      reg_out.value(),
                      " is not a register output"));
    return;
  }
  if (!reg_class.isValid()) {
    failInvalidRegClass("setRegisterClass()");
    return;
  }
  module_.objs_[reg_out.value()].data = reg_class.rawIndex();
}

ObjId ModuleBuilder::addRegisterIn(ObjId reg_out, std::span<const ObjId> pins)
{
  if (!ok()) {
    return ObjId();
  }
  if (isPlaceholder(reg_out) || !module_.isValid(reg_out)
      || module_.op(reg_out) != Op::kRegOut) {
    fail(absl::StrCat("addRegisterIn(): object ",
                      reg_out.value(),
                      " is not a register output"));
    return ObjId();
  }
  if (module_.stateIn(reg_out).isValid()) {
    fail(absl::StrCat("addRegisterIn(): register output ",
                      reg_out.value(),
                      " already has an input"));
    return ObjId();
  }
  if (!checkFanins(pins)) {
    return ObjId();
  }
  const ObjId id = append(Op::kRegIn,
                          module_.width(reg_out),
                          pins,
                          reg_out.value(),
                          module_.isSigned(reg_out));
  module_.linkStateIn(reg_out, id);
  --open_pairs_;
  return id;
}

ObjId ModuleBuilder::addLoopOut(int width, Signedness signedness)
{
  if (!ok()) {
    return ObjId();
  }
  // The pair link (stateIn()) stays unset until addLoopIn().
  const ObjId id
      = append(Op::kLoopOut, width, {}, 0, signedness == Signedness::kSigned);
  ++open_pairs_;
  return id;
}

ObjId ModuleBuilder::addLoopIn(ObjId loop_out, ObjId driver)
{
  if (!ok()) {
    return ObjId();
  }
  if (isPlaceholder(loop_out) || !module_.isValid(loop_out)
      || module_.op(loop_out) != Op::kLoopOut) {
    fail(absl::StrCat(
        "addLoopIn(): object ", loop_out.value(), " is not a loop output"));
    return ObjId();
  }
  if (module_.stateIn(loop_out).isValid()) {
    fail(absl::StrCat("addLoopIn(): loop output ",
                      loop_out.value(),
                      " already has an input"));
    return ObjId();
  }
  if (!checkFanins(std::span(&driver, 1))) {
    return ObjId();
  }
  const int w = module_.width(loop_out);
  if (width(driver) != w) {
    fail(absl::StrCat("addLoopIn(): loop output ",
                      loop_out.value(),
                      " has width ",
                      w,
                      " but its driver has width ",
                      width(driver)));
    return ObjId();
  }
  const ObjId id = append(Op::kLoopIn,
                          w,
                          std::span(&driver, 1),
                          loop_out.value(),
                          module_.isSigned(loop_out));
  module_.linkStateIn(loop_out, id);
  --open_pairs_;
  return id;
}

ObjId ModuleBuilder::addPlaceholder(int width)
{
  assert(width >= 0);
  if (!ok()) {
    return ObjId();
  }
  const ObjIndex index = static_cast<ObjIndex>(placeholder_width_.size());
  if (index >= kPlaceholderBit - 1) {
    fail("too many placeholders");
    return ObjId();
  }
  placeholder_width_.push_back(width);
  placeholder_driver_.emplace_back();
  placeholder_has_piece_.push_back(0);
  return ObjId(kPlaceholderBit | index);
}

void ModuleBuilder::drivePlaceholder(ObjId placeholder, ObjId driver)
{
  if (!ok()) {
    return;
  }
  if (isPlaceholder(placeholder) && isKnown(placeholder) && isKnown(driver)) {
    const ObjIndex index = placeholderIndex(placeholder);
    if (width(driver) != placeholder_width_[index]) {
      fail(absl::StrCat("drivePlaceholder(): placeholder ",
                        index,
                        " has width ",
                        placeholder_width_[index],
                        " but its driver has width ",
                        width(driver)));
      return;
    }
  }
  drivePlaceholder(placeholder, 0, driver);
}

void ModuleBuilder::drivePlaceholder(ObjId placeholder, int lsb, ObjId driver)
{
  assert(lsb >= 0);
  if (!ok()) {
    return;
  }
  if (!isPlaceholder(placeholder) || !isKnown(placeholder)) {
    fail(absl::StrCat(
        "drivePlaceholder(): ", placeholder.value(), " is not a placeholder"));
    return;
  }
  if (!isKnown(driver)) {
    fail(absl::StrCat("drivePlaceholder(): unknown driver ", driver.value()));
    return;
  }
  const ObjIndex index = placeholderIndex(placeholder);
  const int placeholder_width = placeholder_width_[index];
  const int driver_width = width(driver);
  if (driver_width == 0 || lsb > placeholder_width
      || driver_width > placeholder_width - lsb) {
    fail(absl::StrCat("drivePlaceholder(): bits [",
                      lsb,
                      ", ",
                      int64_t{lsb} + driver_width,
                      ") are outside placeholder ",
                      index,
                      " of width ",
                      placeholder_width));
    return;
  }
  if (placeholder_driver_[index].isValid()) {
    fail(absl::StrCat(
        "drivePlaceholder(): placeholder ", index, " is already resolved"));
    return;
  }
  if (lsb == 0 && driver_width == placeholder_width
      && !placeholder_has_piece_[index]) {
    placeholder_driver_[index] = driver;  // the common, whole-width case
    return;
  }
  placeholder_has_piece_[index] = 1;
  pieces_.push_back(Piece{.placeholder = index, .lsb = lsb, .driver = driver});
  if (isPlaceholder(driver)) {
    has_placeholder_fanins_ = true;  // it becomes a Concat fanin
  }
}

void ModuleBuilder::setName(ObjId id, NameId name)
{
  if (!ok()) {
    return;
  }
  if (!isKnown(id)) {
    fail(absl::StrCat("setName(): unknown object ", id.value()));
    return;
  }
  if (isPlaceholder(id)) {
    placeholder_names_.emplace_back(placeholderIndex(id), name);
    return;
  }
  module_.assignName(id, name);
}

void ModuleBuilder::addObjAttributes(ObjId id, ObjAttributes attrs)
{
  if (!ok() || !attrs.any()) {
    return;
  }
  if (!isKnown(id)) {
    fail(absl::StrCat("addObjAttributes(): unknown object ", id.value()));
    return;
  }
  if (isPlaceholder(id)) {
    placeholder_attrs_.emplace_back(placeholderIndex(id), attrs);
    return;
  }
  module_.obj_attrs_[id.value()] |= attrs;
  module_.objs_[id.value()].flags.has_attr = true;
}

void ModuleBuilder::setObjSourceLoc(ObjId id, SourceLoc loc)
{
  if (!ok()) {
    return;
  }
  if (!isKnown(id)) {
    fail(absl::StrCat("setObjSourceLoc(): unknown object ", id.value()));
    return;
  }
  if (isPlaceholder(id)) {
    if (loc.isValid()) {
      placeholder_locs_.emplace_back(placeholderIndex(id), loc);
    }
    return;
  }
  module_.assignObjSourceLoc(id, loc);
}

int ModuleBuilder::width(ObjId id) const
{
  if (!isKnown(id)) {
    return 0;
  }
  return isPlaceholder(id) ? placeholder_width_[placeholderIndex(id)]
                           : module_.width(id);
}

bool ModuleBuilder::assemblePlaceholders()
{
  auto undriven = [&](ObjIndex index, int lsb, int width) {
    if (!allow_undriven_) {
      fail(absl::StrCat("bits [",
                        lsb,
                        ", ",
                        int64_t{lsb} + width,
                        ") of placeholder ",
                        index,
                        " are never driven"));
      return ObjId();
    }
    return append(Op::kUndriven, width, {}, 0, false);
  };

  // Piecewise placeholders: one LSB-first Concat each, gaps filled.
  std::ranges::sort(pieces_, [](const Piece& a, const Piece& b) {
    return a.placeholder != b.placeholder ? a.placeholder < b.placeholder
                                          : a.lsb < b.lsb;
  });
  std::vector<ObjId> parts;
  const int64_t num_pieces = std::ssize(pieces_);
  int64_t i = 0;
  while (i < num_pieces) {
    const ObjIndex index = pieces_[i].placeholder;
    const int placeholder_width = placeholder_width_[index];
    parts.clear();
    int next_bit = 0;
    for (; i < num_pieces && pieces_[i].placeholder == index; ++i) {
      const Piece& piece = pieces_[i];
      if (piece.lsb < next_bit) {
        fail(absl::StrCat("bit ",
                          piece.lsb,
                          " of placeholder ",
                          index,
                          " is driven more than once"));
        return false;
      }
      if (piece.lsb > next_bit) {
        const ObjId gap = undriven(index, next_bit, piece.lsb - next_bit);
        if (!gap.isValid()) {
          return false;
        }
        parts.push_back(gap);
      }
      parts.push_back(piece.driver);
      next_bit = piece.lsb + width(piece.driver);
    }
    if (next_bit < placeholder_width) {
      const ObjId gap = undriven(index, next_bit, placeholder_width - next_bit);
      if (!gap.isValid()) {
        return false;
      }
      parts.push_back(gap);
    }
    placeholder_driver_[index]
        = parts.size() == 1
              ? parts[0]
              : append(Op::kConcat, placeholder_width, parts, 0, false);
  }

  // Placeholders that nothing drives at all.
  if (!placeholder_width_.empty()) {
    std::vector<uint8_t> referenced;
    if (allow_undriven_) {
      referenced.assign(placeholder_width_.size(), 0);
      auto markIfPlaceholder = [&](ObjId id) {
        if (isPlaceholder(id)) {
          referenced[placeholderIndex(id)] = 1;
        }
      };
      for (const ObjId driver : placeholder_driver_) {
        markIfPlaceholder(driver);
      }
      for (const Piece& piece : pieces_) {
        markIfPlaceholder(piece.driver);
      }
      for (const ObjId fanin : module_.fanins_) {
        markIfPlaceholder(fanin);
      }
    }
    for (ObjIndex index = 0; index < placeholder_width_.size(); ++index) {
      if (placeholder_driver_[index].isValid()) {
        continue;
      }
      if (!allow_undriven_) {
        fail(absl::StrCat("placeholder ", index, " is never resolved"));
        return false;
      }
      if (referenced[index]) {
        placeholder_driver_[index]
            = append(Op::kUndriven, placeholder_width_[index], {}, 0, false);
      }
    }
  }
  return ok();
}

void ModuleBuilder::cutCombinationalLoops()
{
  std::vector<ObjRecord>& objs = module_.objs_;
  std::vector<ObjId>& fanins = module_.fanins_;
  const ObjIndex initial_size = static_cast<ObjIndex>(objs.size());

  enum class VisitState : uint8_t
  {
    kNew,
    kOpen,
    kDone,
  };
  std::vector<VisitState> state(initial_size, VisitState::kNew);
  std::vector<ObjId> loop_out_of(initial_size);
  struct Frame
  {
    ObjIndex id = 0;
    int next = 0;
  };
  std::vector<Frame> stack;

  for (ObjIndex root = 0; root < initial_size && ok(); ++root) {
    if (state[root] != VisitState::kNew || objs[root].op == Op::kInst) {
      continue;
    }
    state[root] = VisitState::kOpen;
    stack.push_back(Frame{.id = root, .next = 0});
    while (!stack.empty() && ok()) {
      const ObjIndex u = stack.back().id;
      const int count = module_.faninCount(ObjId(u));
      if (stack.back().next >= count) {
        state[u] = VisitState::kDone;
        stack.pop_back();
        continue;
      }
      const int k = stack.back().next++;
      const ObjIndex slot
          = Module::firstFaninSlot(objs[u]) + static_cast<ObjIndex>(k);
      const ObjId w = fanins[slot];
      if (!w.isValid() || w.value() >= initial_size) {
        continue;
      }
      ObjId target = w;
      if (state[target.value()] == VisitState::kNew
          && objs[w.value()].op == Op::kFan && module_.faninCount(w) == 1) {
        const ObjId owner = module_.fanin(w, 0);
        if (owner.isValid() && owner.value() < initial_size
            && state[owner.value()] == VisitState::kOpen) {
          target = owner;
        }
      }
      const VisitState target_state = state[target.value()];
      if (target_state == VisitState::kNew) {
        state[w.value()] = VisitState::kOpen;
        stack.push_back(Frame{.id = w.value(), .next = 0});
        continue;
      }
      if (target_state != VisitState::kOpen) {
        continue;
      }
      // Back edge: cut `u -> w` with a loop pair. A Fan's link to its owner
      // and an instance (width 0) are not wires; another edge gets cut.
      if (objs[u].op == Op::kFan || objs[w.value()].width == 0) {
        continue;
      }
      ObjId& loop_out = loop_out_of[w.value()];
      if (!loop_out.isValid()) {
        loop_out = addLoopOut(objs[w.value()].width,
                              objs[w.value()].flags.is_signed
                                  ? Signedness::kSigned
                                  : Signedness::kUnsigned);
        if (!loop_out.isValid()) {
          return;
        }
        if (!addLoopIn(loop_out, w).isValid()) {
          return;
        }
      }
      fanins[slot] = loop_out;
    }
  }
}

bool ModuleBuilder::checkPortNames()
{
  if (absl::Status status = lir::checkPortNames(module_); !status.ok()) {
    fail(std::string(status.message()));
    return false;
  }
  return true;
}

absl::StatusOr<Module> ModuleBuilder::finish() &&
{
  if (!ok()) {
    return error();
  }
  if (open_pairs_ != 0) {
    return absl::InvalidArgumentError(
        absl::StrCat(open_pairs_, " register or loop output(s) have no input"));
  }
  if (!assemblePlaceholders()) {
    return error();
  }

  absl::StatusOr<std::vector<ObjId>> final_driver = resolvePlaceholderChains();
  if (!final_driver.ok()) {
    return final_driver.status();
  }
  applyPlaceholderMetadata(*final_driver);
  if (!checkPortNames()) {
    return error();
  }

  if (has_placeholder_fanins_) {
    rewritePlaceholderFanins(*final_driver);
  }
  if (faninsAreOrdered()) {
    module_.ordered_ = true;
  } else {
    cutCombinationalLoops();
    if (!ok()) {
      return error();
    }
  }
  ++module_.revision_;
  return std::move(module_);
}

absl::StatusOr<std::vector<ObjId>> ModuleBuilder::resolvePlaceholderChains()
    const
{
  // Resolve every placeholder to a real object, following chains.
  const size_t num_placeholders = placeholder_width_.size();
  std::vector<ObjId> final_driver(num_placeholders);
  std::vector<ObjIndex> chain;
  for (ObjIndex i = 0; i < num_placeholders; ++i) {
    chain.clear();
    ObjId current(kPlaceholderBit | i);
    while (isPlaceholder(current)) {
      const ObjIndex index = placeholderIndex(current);
      if (final_driver[index].isValid()) {
        current = final_driver[index];
        break;
      }
      if (!placeholder_driver_[index].isValid()) {
        assert(allow_undriven_
               && "assemblePlaceholders() drives every placeholder");
        current = ObjId();
        break;
      }
      if (chain.size() > num_placeholders) {
        return absl::InvalidArgumentError(absl::StrCat(
            "placeholder ", i, " resolves to itself through a cycle"));
      }
      chain.push_back(index);
      current = placeholder_driver_[index];
    }
    for (const ObjIndex index : chain) {
      final_driver[index] = current;
    }
  }
  return final_driver;
}

void ModuleBuilder::applyPlaceholderMetadata(
    std::span<const ObjId> final_driver)
{
  for (const auto& [index, name] : placeholder_names_) {
    const ObjId driver = final_driver[index];
    if (driver.isValid() && !module_.objName(driver).isValid()) {
      module_.assignName(driver, name);
    }
  }
  for (const auto& [index, attrs] : placeholder_attrs_) {
    const ObjId driver = final_driver[index];
    if (driver.isValid()) {
      module_.obj_attrs_[driver.value()] |= attrs;
      module_.objs_[driver.value()].flags.has_attr = true;
    }
  }
  for (const auto& [index, loc] : placeholder_locs_) {
    const ObjId driver = final_driver[index];
    if (driver.isValid() && !module_.objSourceLoc(driver).isValid()) {
      module_.assignObjSourceLoc(driver, loc);
    }
  }
}

void ModuleBuilder::rewritePlaceholderFanins(
    std::span<const ObjId> final_driver)
{
  for (const ObjRecord& r : module_.objs_) {
    for (ObjId& fanin : Module::faninSpan(r, std::span(module_.fanins_))) {
      if (isPlaceholder(fanin)) {
        fanin = final_driver[placeholderIndex(fanin)];
      }
    }
  }
}

bool ModuleBuilder::faninsAreOrdered() const
{
  // Only resolved placeholders can point forward; otherwise ids are
  // topological by construction.
  const std::vector<ObjRecord>& objs = module_.objs_;
  bool ordered = true;
  for (ObjIndex id = 0; has_placeholder_fanins_ && id < objs.size() && ordered;
       ++id) {
    for (const ObjId fanin : module_.fanins(ObjId(id))) {
      if (fanin.value() >= id) {
        ordered = false;
        break;
      }
    }
  }
  return ordered;
}

ObjId ModuleBuilder::input(int port) const
{
  assert(port >= 0);
  return src_ != nullptr && port < std::ssize(module_.inputs_)
             ? module_.inputs_[port]
             : ObjId();
}

ObjId ModuleBuilder::copy(ObjId src_obj, std::span<const ObjId> fanins)
{
  if (!ok()) {
    return ObjId();
  }
  if (src_ == nullptr) {
    fail("copy() needs a builder made from a source module");
    return ObjId();
  }
  if (!src_->isValid(src_obj) || src_->isDead(src_obj)) {
    fail(absl::StrCat(
        "copy(): source object ", src_obj.value(), " does not exist"));
    return ObjId();
  }
  const ObjRecord& r = src_->record(src_obj);
  const ObjId id = copyByOp(r, src_obj, fanins);
  // A Pi is input(port): no mapping or metadata to copy.
  if (r.op == Op::kPi || !id.isValid()) {
    return id;
  }
  src_map_[src_obj.value()] = id;
  const NameId name = src_->objName(src_obj);
  if (name.isValid()) {
    setName(id, name);
  }
  addObjAttributes(id, src_->objAttributes(src_obj));
  if (const SourceLoc loc = src_->objSourceLoc(src_obj); loc.isValid()) {
    setObjSourceLoc(id, loc);
  }
  return id;
}

bool ModuleBuilder::copyTakesNoFanins(const ObjRecord& r,
                                      ObjId src_obj,
                                      std::span<const ObjId> fanins)
{
  if (!fanins.empty()) {
    fail(absl::StrCat("copy(): a '",
                      opName(r.op),
                      "' object (source ",
                      src_obj.value(),
                      ") takes no fanins"));
    return false;
  }
  return true;
}

ObjId ModuleBuilder::copyByOp(const ObjRecord& r,
                              ObjId src_obj,
                              std::span<const ObjId> fanins)
{
  const Signedness signedness
      = r.flags.is_signed ? Signedness::kSigned : Signedness::kUnsigned;
  switch (r.op) {
    case Op::kPi:
      return copyTakesNoFanins(r, src_obj, fanins)
                 ? input(src_->portIndex(src_obj))
                 : ObjId();
    case Op::kPo: {
      const int port = src_->portIndex(src_obj);
      if (fanins.size() != 1) {
        fail(absl::StrCat("copy(): output ", port, " takes one fanin"));
        return ObjId();
      }
      if (port != std::ssize(module_.outputs_)) {
        fail(absl::StrCat(
            "copy(): output ", port, " is copied out of port order"));
        return ObjId();
      }
      return addPo(src_->outputName(port), fanins[0]);
    }
    case Op::kConst:
      if (!copyTakesNoFanins(r, src_obj, fanins)) {
        return ObjId();
      }
      return r.flags.inline_const
                 ? addConst(r.width,
                            static_cast<uint32_t>(r.data),
                            static_cast<uint32_t>(r.fanin_begin))
                 : append(Op::kConst, r.width, {}, r.data, false);  // pooled
    case Op::kRegOut:
      if (!copyTakesNoFanins(r, src_obj, fanins)) {
        return ObjId();
      }
      return addRegisterOut(r.width, src_->regClassId(src_obj), signedness);
    case Op::kRegIn: {
      const ObjId reg_out = copyOf(src_->stateOut(src_obj));
      if (!reg_out.isValid()) {
        fail(absl::StrCat("copy(): the register output of source object ",
                          src_obj.value(),
                          " has not been copied"));
        return ObjId();
      }
      return addRegisterIn(reg_out, fanins);
    }
    case Op::kLoopOut:
      if (!copyTakesNoFanins(r, src_obj, fanins)) {
        return ObjId();
      }
      return addLoopOut(r.width, signedness);
    case Op::kLoopIn: {
      const ObjId loop_out = copyOf(src_->stateOut(src_obj));
      if (!loop_out.isValid()) {
        fail(absl::StrCat("copy(): the loop output of source object ",
                          src_obj.value(),
                          " has not been copied"));
        return ObjId();
      }
      if (fanins.size() != 1) {
        fail(absl::StrCat("copy(): loop input (source ",
                          src_obj.value(),
                          ") takes one fanin"));
        return ObjId();
      }
      return addLoopIn(loop_out, fanins[0]);
    }
    default:
      // Not a state input, so data fits 32 bits.
      return add(
          r.op, r.width, fanins, static_cast<uint32_t>(r.data), signedness);
  }
}

absl::StatusOr<IdRemap> ModuleBuilder::install(Design& design, ModId id) &&
{
  if (src_ == nullptr) {
    return absl::FailedPreconditionError(
        "install() needs a builder made from a source module");
  }
  if (!id.isValid()
      || id.value() >= static_cast<uint32_t>(design.numModules())) {
    return absl::InvalidArgumentError(
        absl::StrCat("install(): module ", id.value(), " does not exist"));
  }
  std::vector<ObjId> src_map = std::move(src_map_);
  absl::StatusOr<Module> built = std::move(*this).finish();
  if (!built.ok()) {
    return built.status();
  }
  Module& module = *built;
  const Module& old = design.module(id);
  if (!sameWidths(module, module.inputs(), old, old.inputs())) {
    return absl::InvalidArgumentError(absl::StrCat(
        "install(): the inputs of module ", id.value(), " would change"));
  }
  if (!sameWidths(module, module.outputs(), old, old.outputs())) {
    return absl::InvalidArgumentError(absl::StrCat(
        "install(): the outputs of module ", id.value(), " would change"));
  }
  if (design.options().check_well_formed) {
    if (absl::Status status = checkWellFormed(design, module); !status.ok()) {
      return status;
    }
  }
  // replaceModule() sets the revision to the old one plus one.
  if (absl::Status status = design.replaceModule(id, std::move(module));
      !status.ok()) {
    return status;
  }
  return IdRemap(std::move(src_map));
}

ObjId ModuleBuilder::append(Op op,
                            int width,
                            std::span<const ObjId> fanins,
                            ObjIndex data,
                            bool is_signed)
{
  // The builder's cap is lower than the Module's: ids with kPlaceholderBit
  // set are placeholders.
  ObjFlags flags{};
  flags.is_signed = is_signed;
  const ObjId id = module_.size() >= kMaxObjects
                       ? ObjId()
                       : module_.appendObject(op, width, fanins, data, flags);
  if (!id.isValid()) {
    failTooLarge();
  }
  return id;
}

bool ModuleBuilder::checkFanins(std::span<const ObjId> fanins)
{
  for (const ObjId fanin : fanins) {
    if (!isKnown(fanin)) {
      fail(absl::StrCat("fanin ",
                        fanin.value(),
                        " does not exist yet; use a placeholder for forward "
                        "references"));
      return false;
    }
    if (isPlaceholder(fanin)) {
      has_placeholder_fanins_ = true;
    }
  }
  return true;
}

bool ModuleBuilder::isKnown(ObjId id) const
{
  if (!id.isValid()) {
    return false;
  }
  if (isPlaceholder(id)) {
    return placeholderIndex(id) < placeholder_width_.size();
  }
  return module_.isValid(id);
}

ObjIndex ModuleBuilder::placeholderIndex(ObjId id) const
{
  return id.value() & ~kPlaceholderBit;
}

void ModuleBuilder::fail(std::string message, absl::StatusCode code)
{
  if (error_.empty()) {
    error_ = std::move(message);
    error_code_ = code;
  }
}

void ModuleBuilder::failInvalidRegClass(const char* what)
{
  fail(absl::StrCat(what,
                    ": invalid register class (its initial state may "
                    "need a new constant, and the constant pool is "
                    "read-only)"),
       absl::StatusCode::kFailedPrecondition);
}

void ModuleBuilder::failTooLarge()
{
  fail(absl::StrCat("module is too large: ",
                    module_.size(),
                    " objects and ",
                    module_.faninArenaSize(),
                    " fanin slots; the limits are ",
                    kMaxObjects,
                    " objects and ",
                    Module::kMaxFaninSlots,
                    " slots with ",
                    kObjIdBits,
                    "-bit object ids",
                    kObjIdBits == 64
                        ? ""
                        : " (split the module into a hierarchy, or build with "
                          "LIR_OBJ_ID_BITS=64)"),
       absl::StatusCode::kResourceExhausted);
}

}  // namespace syn::lir
