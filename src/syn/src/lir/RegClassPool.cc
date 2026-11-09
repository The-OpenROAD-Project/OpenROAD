// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include "syn/lir/RegClassPool.h"

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <utility>
#include <vector>

#include "syn/lir/Checksum.h"
#include "syn/lir/ConstPool.h"
#include "syn/lir/ConstView.h"
#include "syn/lir/Ids.h"
#include "syn/lir/LibCellTable.h"
#include "syn/lir/RegPinTypes.h"

namespace syn::lir {

RegInit RegInit::inlineValue(uint32_t value, uint32_t known)
{
  assert((value & ~known) == 0);
  RegInit init;
  init.value_ = value;
  init.known_ = known;
  return init;
}

RegInit RegInit::pooled(ConstId id)
{
  RegInit init;  // an invalid id is rejected by RegClassPool::intern()
  init.value_ = id.rawIndex();
  init.pooled_ = true;
  return init;
}

RegInit RegInit::fromConst(ConstView value, ConstPool& pool)
{
  const uint32_t width = value.width();
  if (width <= kMaxInlineConstWidth) {
    uint32_t known = 0;
    uint32_t bits = 0;
    for (uint32_t i = 0; i < width; ++i) {
      const LogicValue v = value.bit(i);
      if (v == LogicValue::kZero || v == LogicValue::kOne) {
        known |= 1u << i;
        if (v == LogicValue::kOne) {
          bits |= 1u << i;
        }
      }
    }
    return inlineValue(bits, known);
  }
  if (value.isFill(LogicValue::kX)) {
    return RegInit();
  }
  return pooled(pool.intern(width, value.bits(), value.xMask()));
}

LogicValue RegInit::bit(uint32_t index, const ConstPool& pool) const
{
  if (pooled_) {
    const ConstView v = pool.get(constId());
    if (index >= v.width()) {
      return LogicValue::kX;
    }
    return v.bit(index);
  }
  if (index >= 32 || ((known_ >> index) & 1u) == 0) {
    return LogicValue::kX;
  }
  return ((value_ >> index) & 1u) != 0 ? LogicValue::kOne : LogicValue::kZero;
}

uint64_t RegInit::checksum(const ConstPool& pool) const
{
  if (isNone()) {
    return 0;
  }
  if (pooled_) {
    return checksumCombine(checksumCombine(kChecksumSeed, 2),
                           pool.checksum(constId()));
  }
  uint64_t h = checksumCombine(kChecksumSeed, 1);
  h = checksumCombine(h, value_);
  h = checksumCombine(h, known_);
  return h;
}

RegClass RegClass::generic(RegFlags flags, RegPinMask pins, RegInit init)
{
  RegClass c;
  c.flags_ = flags;
  c.pins_ = pins;
  c.init_ = init;
  return c;
}

RegClass RegClass::mapped(LibCellId cell)
{
  RegClass c;
  c.mode_ = Mode::kMapped;
  c.cell_ = cell;
  return c;
}

RegClass RegClass::withPins(RegPinMask pins) const
{
  assert(isGeneric());
  RegClass c = *this;
  c.pins_ = pins;
  return c;
}

RegClass RegClass::withInit(RegInit init) const
{
  assert(isGeneric());
  RegClass c = *this;
  c.init_ = init;
  return c;
}

uint64_t RegClass::checksum(const ConstPool& consts,
                            const LibCellTable& cells) const
{
  uint64_t h = checksumCombine(kChecksumSeed, static_cast<uint64_t>(mode_));
  if (isMapped()) {
    return checksumCombine(h, cells.checksum(cell_));
  }
  const uint64_t flag_bits = (flags_.latch ? 1ULL : 0ULL)
                             | (flags_.sync_over_enable ? 2ULL : 0ULL)
                             | (flags_.set_over_reset ? 4ULL : 0ULL)
                             | (static_cast<uint64_t>(flags_.reserved) << 3u);
  h = checksumCombine(h, flag_bits);
  h = checksumCombine(h, pins_);
  h = checksumCombine(h, init_.checksum(consts));
  return h;
}

RegClassId RegClassPool::intern(const RegClass& reg_class)
{
  if (!reg_class.init().isValid()) {
    return RegClassId();
  }
  if (const RegClassId id = find(reg_class); id.isValid()) {
    return id;
  }
  const uint32_t id = static_cast<uint32_t>(classes_.size());
  classes_.push_back(reg_class);
  index_.emplace(reg_class, id);
  return RegClassId(id);
}

RegClass RegClassPool::get(RegClassId id) const
{
  assert(id.isValid() && id.rawIndex() < classes_.size());
  return classes_[id.rawIndex()];
}

RegClassId RegClassPool::find(const RegClass& reg_class) const
{
  const auto it = index_.find(reg_class);
  return it != index_.end() ? RegClassId(it->second) : RegClassId();
}

size_t RegClassPool::memoryBytes() const
{
  return classes_.capacity() * sizeof(RegClass)
         + index_.capacity() * (sizeof(std::pair<RegClass, uint32_t>) + 1);
}

}  // namespace syn::lir
