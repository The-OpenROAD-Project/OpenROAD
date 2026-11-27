// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include "syn/lir/Module.h"

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <span>
#include <utility>
#include <vector>

#include "syn/lir/Checksum.h"
#include "syn/lir/ConstPool.h"
#include "syn/lir/ConstView.h"
#include "syn/lir/Design.h"
#include "syn/lir/Ids.h"
#include "syn/lir/LibCellTable.h"
#include "syn/lir/NameInterner.h"
#include "syn/lir/ObjAttributes.h"
#include "syn/lir/Op.h"
#include "syn/lir/RegClassPool.h"
#include "syn/lir/SourceLoc.h"

namespace syn::lir {

namespace {

// Module::setCapacityForTesting().
ObjIndex g_max_objects = Module::kMaxObjects;
ObjIndex g_max_fanin_slots = Module::kMaxFaninSlots;

}  // namespace

void Module::setCapacityForTesting(ObjIndex max_objects,
                                   ObjIndex max_fanin_slots)
{
  g_max_objects = max_objects < kMaxObjects ? max_objects : kMaxObjects;
  g_max_fanin_slots
      = max_fanin_slots < kMaxFaninSlots ? max_fanin_slots : kMaxFaninSlots;
}

bool Module::faninArenaFits(size_t extra_slots) const
{
  return extra_slots <= g_max_fanin_slots
         && fanins_.size() <= g_max_fanin_slots - extra_slots;
}

bool Module::objectsFit(size_t extra_objects) const
{
  return extra_objects <= g_max_objects
         && objs_.size() <= g_max_objects - extra_objects;
}

ConstView Module::constValue(ObjId id, const ConstPool& pool) const
{
  const ObjRecord& r = record(id);
  assert(r.op == Op::kConst);
  if (r.flags.inline_const) {
    return ConstView::fromInline(r.width,
                                 static_cast<uint32_t>(r.data),
                                 static_cast<uint32_t>(r.fanin_begin));
  }
  return pool.get(ConstId(static_cast<uint32_t>(r.data)));
}

bool Module::isPristineOperator() const
{
  return impl_.has_value() && !impl_->modified
         && revision_ == impl_->generated_revision;
}

size_t Module::faninSlotsUsed(ObjId id) const
{
  const ObjRecord& r = record(id);
  return r.fanin_count == kFaninCountEscape
             ? 1 + static_cast<size_t>(faninCount(id))
             : r.fanin_count;
}

void Module::appendFaninHeader(ObjRecord& r,
                               std::vector<ObjId>& arena,
                               size_t count)
{
  assert(count <= static_cast<size_t>(std::numeric_limits<int>::max()));
  r.fanin_begin = static_cast<ObjIndex>(arena.size());
  if (count < kFaninCountEscape) {
    r.fanin_count = static_cast<uint16_t>(count);
  } else {
    r.fanin_count = kFaninCountEscape;
    arena.emplace_back(static_cast<ObjIndex>(count));
  }
}

ObjId Module::appendObject(Op op,
                           int width,
                           std::span<const ObjId> fanins,
                           ObjIndex data,
                           ObjFlags flags)
{
  assert(width >= 0);
  // +1: room for an escape header.
  if (objs_.size() >= g_max_objects
      || fanins.size() > static_cast<size_t>(std::numeric_limits<int>::max())
      || !faninArenaFits(fanins.size() + 1)) {
    return ObjId();
  }
  if (!hasSignedFlag(op)) {
    flags.is_signed = false;  // canonical: the flag means nothing on `op`
  }
  // `fanins` may be a span into `fanins_` (e.g. `module.fanins(other)`).
  // Capture its index offset before growing `fanins_` can reallocate it.
  const auto arena_begin = reinterpret_cast<uintptr_t>(fanins_.data());
  const auto arena_end
      = reinterpret_cast<uintptr_t>(fanins_.data() + fanins_.size());
  const auto span_begin = reinterpret_cast<uintptr_t>(fanins.data());
  const bool aliases_arena
      = !fanins.empty() && span_begin >= arena_begin && span_begin < arena_end;
  const size_t alias_offset
      = aliases_arena ? static_cast<size_t>(fanins.data() - fanins_.data()) : 0;

  ObjRecord r;
  r.op = op;
  r.flags = flags;
  r.width = width;
  r.data = data;
  appendFaninHeader(r, fanins_, fanins.size());
  // A state output has no fanins; fanin_begin holds its pair link instead.
  assert(fanins.empty() || !isStateOut(op));
  if (isStateOut(op)) {
    r.fanin_begin = ObjId::kInvalidValue;
  }
  if (aliases_arena) {
    const size_t dest = fanins_.size();
    fanins_.resize(dest + fanins.size());
    std::copy_n(
        fanins_.data() + alias_offset, fanins.size(), fanins_.data() + dest);
  } else {
    fanins_.insert(fanins_.end(), fanins.begin(), fanins.end());
  }
  const ObjId id(static_cast<ObjIndex>(objs_.size()));
  objs_.push_back(r);
  return id;
}

ObjId Module::appendInlineConst(int width, uint32_t bits, uint32_t x_mask)
{
  assert(width >= 0 && width <= kMaxInlineConstWidth);
  ObjFlags flags{};
  flags.inline_const = true;
  const ObjId id = appendObject(Op::kConst, width, {}, 0, flags);
  if (id.isValid()) {
    // width <= 32; width 0 gives an empty mask.
    const uint32_t mask
        = width == 32 ? ~uint32_t{0}
                      : (uint32_t{1} << static_cast<uint32_t>(width)) - 1;
    ObjRecord& r = objs_[id.value()];
    const uint32_t x = x_mask & mask;
    r.data = (bits & mask) & ~x;
    r.fanin_begin = x;
  }
  return id;
}

size_t Module::memoryBytes() const
{
  return objs_.capacity() * sizeof(ObjRecord)
         + fanins_.capacity() * sizeof(ObjId)
         + inputs_.capacity() * sizeof(ObjId)
         + outputs_.capacity() * sizeof(ObjId)
         + input_names_.capacity() * sizeof(NameId)
         + output_names_.capacity() * sizeof(NameId)
         + obj_names_.capacity() * sizeof(NameId)
         + obj_attrs_.bucket_count()
               * (sizeof(std::pair<const ObjIndex, ObjAttributes>) + 1)
         + obj_locs_.bucket_count()
               * (sizeof(std::pair<const ObjIndex, SourceLoc>) + 1)
         + reg_classes_.memoryBytes();
}

RegClass Module::regClass(ObjId reg) const
{
  assert(isValid(reg));
  const ObjId out = op(reg) == Op::kRegIn ? stateOut(reg) : reg;
  return reg_classes_.get(regClassId(out));
}

bool Module::nameIsNetName(ObjId id) const
{
  switch (op(id)) {
    case Op::kInst:
    case Op::kCell:
      return false;
    case Op::kRegOut:
      return !regClass(id).isMapped();
    default:
      return true;
  }
}

bool Module::takesNetName(ObjId id) const
{
  switch (op(id)) {
    case Op::kPi:
    case Op::kPo:
    case Op::kRegOut:
    case Op::kLoopOut:
      return false;
    default:
      return nameIsNetName(id);
  }
}

uint64_t Module::objChecksum(const Design& design, ObjId id) const
{
  assert(isValid(id));
  uint64_t hash = checksumCombine(kChecksumSeed, id.value());
  if (isDead(id)) {
    return checksumCombine(hash, 0);
  }
  hash = checksumCombine(hash, 1);
  const ObjRecord& rec = record(id);
  hash = checksumCombine(hash, static_cast<uint64_t>(rec.op));
  hash = checksumCombine(hash, static_cast<uint32_t>(rec.width));
  hash = checksumCombine(hash, rec.flags.is_signed ? 1 : 0);

  const std::span<const ObjId> fanin_ids = fanins(id);
  hash = checksumCombine(hash, fanin_ids.size());
  for (const ObjId fanin : fanin_ids) {
    hash = checksumCombine(hash, fanin.value());
  }
  if (isStateOut(rec.op)) {
    hash = checksumCombine(hash, stateIn(id).value());  // not a fanin
  }

  hash = checksumCombine(hash, design.names().checksum(objName(id)));
  hash = checksumCombine(hash, objAttributes(id).checksum());
  hash = checksumCombine(hash, objSourceLoc(id).checksum(design.names()));

  switch (rec.op) {
    case Op::kConst:
      hash = checksumCombine(hash, constValue(id, design.consts()).checksum());
      break;
    case Op::kLut:
      hash = checksumCombine(hash, design.consts().checksum(lutTable(id)));
      break;
    case Op::kRegOut:
      hash = checksumCombine(
          hash, regClass(id).checksum(design.consts(), design.cells()));
      break;
    case Op::kCell:
      hash = checksumCombine(hash, design.cells().checksum(cellId(id)));
      break;
    default:
      hash = checksumCombine(hash, rec.data);
      break;
  }
  return hash;
}

uint64_t Module::checksum(const Design& design) const
{
  uint64_t hash = checksumCombine(kChecksumSeed, isRemoved() ? 0 : 1);
  if (isRemoved()) {
    return hash;
  }
  hash = checksumCombine(hash, design.names().checksum(name_));
  const uint64_t attr_bits = (attributes_.blackbox ? 1ULL : 0ULL)
                             | (attributes_.keep_hierarchy ? 2ULL : 0ULL)
                             | (attributes_.dont_touch ? 4ULL : 0ULL)
                             | (attributes_.frozen_by_parent ? 8ULL : 0ULL);
  hash = checksumCombine(hash, attr_bits);
  hash = checksumCombine(hash, source_loc_.checksum(design.names()));
  hash = checksumCombine(hash, impl_.has_value() ? 1 : 0);
  if (impl_.has_value()) {
    hash = checksumCombine(
        hash, design.operators().checksum(impl_->key, design.consts()));
    hash = checksumCombine(hash, isPristineOperator() ? 1 : 0);
  }
  hash = checksumCombine(hash, inputs_.size());
  const int num_inputs = static_cast<int>(inputs_.size());
  for (int i = 0; i < num_inputs; ++i) {
    hash = checksumCombine(hash, inputs_[i].value());
    hash = checksumCombine(hash, design.names().checksum(inputName(i)));
  }
  hash = checksumCombine(hash, outputs_.size());
  const int num_outputs = static_cast<int>(outputs_.size());
  for (int i = 0; i < num_outputs; ++i) {
    hash = checksumCombine(hash, outputs_[i].value());
    hash = checksumCombine(hash, design.names().checksum(outputName(i)));
  }
  hash = checksumCombine(hash, size());
  for (const ObjId id : objects()) {
    hash = checksumCombine(hash, objChecksum(design, id));
  }
  return hash;
}

void Module::assignName(ObjId id, NameId name)
{
  const ObjRecord& r = record(id);
  if (r.op == Op::kPi) {
    input_names_[r.data] = name;
    return;
  }
  if (r.op == Op::kPo) {
    output_names_[r.data] = name;
    return;
  }
  if (id.value() >= obj_names_.size()) {
    obj_names_.resize(size());
  }
  obj_names_[id.value()] = name;
}

ObjAttributes Module::findObjAttributes(ObjId id) const
{
  // A flag without an entry reads as no attributes; checkWellFormed
  // reports it.
  const auto it = obj_attrs_.find(id.value());
  return it != obj_attrs_.end() ? it->second : ObjAttributes();
}

void Module::assignObjAttributes(ObjId id, ObjAttributes attrs)
{
  if (attrs.any()) {
    obj_attrs_[id.value()] = attrs;
  } else {
    obj_attrs_.erase(id.value());
  }
  objs_[id.value()].flags.has_attr = attrs.any();
}

SourceLoc Module::findObjSourceLoc(ObjId id) const
{
  const auto it = obj_locs_.find(id.value());
  return it != obj_locs_.end() ? it->second : SourceLoc();
}

void Module::assignObjSourceLoc(ObjId id, SourceLoc loc)
{
  if (loc.isValid()) {
    obj_locs_[id.value()] = loc;
  } else {
    obj_locs_.erase(id.value());
  }
  objs_[id.value()].flags.has_loc = loc.isValid();
}

}  // namespace syn::lir
