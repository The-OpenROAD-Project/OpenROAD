// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#pragma once

#include <cassert>
#include <cstdint>
#include <cstring>
#include <vector>

#include "syn/ir/Net.h"
#include "syn/ir/NetTableEntry.h"

namespace syn {

class NetTableBlock
{
 public:
  explicit NetTableBlock(uint32_t block_index) : block_index_(block_index)
  {
    std::memset(slots_, 0, sizeof(slots_));
  }

  uint32_t index() const { return block_index_; }

  NetTableId slotId(const NetTableEntry* entry) const
  {
    return (block_index_ << NetTableEntry::kNetTableIdxBits)
           | entry->objectIndex();
  }

  static const NetTableBlock* blockOf(const NetTableEntry* entry)
  {
    uint32_t obj_idx = entry->objectIndex();
    return reinterpret_cast<const NetTableBlock*>(
        reinterpret_cast<const char*>(entry) - obj_idx * kSlotSize);
  }

  void* slotPtr(uint32_t idx)
  {
    assert(idx < NetTableEntry::kNetTableBlockSize);
    return &slots_[idx];
  }

  const void* slotPtr(uint32_t idx) const
  {
    assert(idx < NetTableEntry::kNetTableBlockSize);
    return &slots_[idx];
  }

  static constexpr size_t kSlotSize = 16;

 private:
  // Each slot holds either an inline instance or a PlaceholderEntry (16 bytes).
  alignas(8) char slots_[NetTableEntry::kNetTableBlockSize][kSlotSize];
  uint32_t block_index_;
};

class NetTable
{
 public:
  NetTable();
  ~NetTable();

  NetTable(const NetTable&) = delete;
  NetTable& operator=(const NetTable&) = delete;

  NetTable(NetTable&& other) noexcept;
  NetTable& operator=(NetTable&& other) noexcept;

  // Allocate a contiguous run of slots. Returns the NetTableId of the first.
  NetTableId allocateSlots(uint32_t count);

  // Access a slot by id.
  void* pointer(NetTableId id);
  const void* pointer(NetTableId id) const;

  NetTableEntry& entry(NetTableId id)
  {
    return *static_cast<NetTableEntry*>(pointer(id));
  }

  const NetTableEntry& entry(NetTableId id) const
  {
    return *static_cast<const NetTableEntry*>(pointer(id));
  }

  size_t size() const { return size_; }

  // Total bytes allocated for slot blocks.
  size_t memoryBytes() const { return blocks_.size() * sizeof(NetTableBlock); }

  // Iterate all allocated slot IDs.
  template <typename F>
  void forEachId(F&& fn) const
  {
    for (size_t b = 0; b < blocks_.size(); ++b) {
      uint32_t base = static_cast<uint32_t>(b)
                      << NetTableEntry::kNetTableIdxBits;
      for (uint32_t s = 0; s < NetTableEntry::kNetTableBlockSize; ++s) {
        fn(base + s);
      }
    }
  }

 private:
  void addBlock();

  std::vector<NetTableBlock*> blocks_;
  size_t size_;
  NetTableId next_free_;
};

}  // namespace syn
