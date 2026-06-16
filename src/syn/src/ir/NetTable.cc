// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include "syn/ir/NetTable.h"

#include <cassert>
#include <cstdint>
#include <utility>

#include "syn/ir/NetTableEntry.h"

namespace syn {

static_assert(sizeof(NetTableEntry) == 4, "NetTableEntry should be 32 bits");

NetTable::NetTable() : size_(0), next_free_(0)
{
  addBlock();
}

NetTable::~NetTable()
{
  for (auto* block : blocks_) {
    delete block;
  }
}

NetTable::NetTable(NetTable&& other) noexcept
    : blocks_(std::move(other.blocks_)),
      size_(other.size_),
      next_free_(other.next_free_)
{
  other.size_ = 0;
  other.next_free_ = 0;
}

NetTable& NetTable::operator=(NetTable&& other) noexcept
{
  if (this != &other) {
    for (auto* block : blocks_) {
      delete block;
    }
    blocks_ = std::move(other.blocks_);
    size_ = other.size_;
    next_free_ = other.next_free_;
    other.size_ = 0;
    other.next_free_ = 0;
  }
  return *this;
}

void NetTable::addBlock()
{
  uint32_t block_index = static_cast<uint32_t>(blocks_.size());
  blocks_.push_back(new NetTableBlock(block_index));
}

NetTableId NetTable::allocateSlots(uint32_t count)
{
  assert(count > 0);
  NetTableId result = next_free_;

  // Allocate count slots, adding blocks as needed.
  const uint32_t last_id = next_free_ + count - 1;
  const uint32_t last_blk_idx = last_id >> NetTableEntry::kNetTableIdxBits;
  while (last_blk_idx >= blocks_.size()) {
    addBlock();
  }

  next_free_ += count;
  size_ += count;
  return result;
}

void* NetTable::pointer(NetTableId id)
{
  uint32_t blk_idx = id >> NetTableEntry::kNetTableIdxBits;
  uint32_t obj_idx = id & NetTableEntry::kNetTableIdxMask;
  assert(blk_idx < blocks_.size());
  return blocks_[blk_idx]->slotPtr(obj_idx);
}

const void* NetTable::pointer(NetTableId id) const
{
  uint32_t blk_idx = id >> NetTableEntry::kNetTableIdxBits;
  uint32_t obj_idx = id & NetTableEntry::kNetTableIdxMask;
  assert(blk_idx < blocks_.size());
  return blocks_[blk_idx]->slotPtr(obj_idx);
}

}  // namespace syn
