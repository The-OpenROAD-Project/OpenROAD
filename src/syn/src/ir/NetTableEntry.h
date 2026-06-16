// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#pragma once

#include <cstdint>

namespace syn {

using NetTableId = uint32_t;
using MetadataId = uint32_t;

enum class EntryType : uint8_t
{
  kVoid,
  kPlaceholder,

  kTieHigh,
  kTieLow,
  kTieX,

  kBufferFine,
  kNotFine,
  kAndFine,
  kOrFine,
  kAndnotFine,
  kXorFine,
  kMuxFine,
  kAdcFine,

  kBufferWide,
  kNotWide,
  kAndWide,
  kOrWide,
  kAndnotWide,
  kXorWide,
  kMuxWide,
  kAdcWide,

  kEq,
  kULt,
  kSLt,
  kShl,
  kUShr,
  kSShr,
  kXShr,

  kMul,
  kUDiv,
  kUMod,
  kSDivTrunc,
  kSDivFloor,
  kSModTrunc,
  kSModFloor,

  kDff,
  kLoopBreaker,

  kInput,
  kDangling,
  kOutput,
  kName,
  kTarget,
  kOther,
};

class NetTableEntry
{
 public:
  static constexpr int kNetTableIdxBits = 7;
  static constexpr uint32_t kNetTableIdxMask = (1u << kNetTableIdxBits) - 1;
  static constexpr int kNetTableBlockSize = 1 << kNetTableIdxBits;

  EntryType entryType() const { return static_cast<EntryType>(entry_type_); }
  void setEntryType(EntryType type)
  {
    entry_type_ = static_cast<uint32_t>(type);
  }

  uint32_t objectIndex() const { return object_index_; }

 protected:
  static_assert(sizeof(EntryType) == 1, "EntryType must fit in entry_type_");

  uint32_t object_index_ : kNetTableIdxBits = 0;
  uint32_t reserved_ : 32 - kNetTableIdxBits - 8 = 0;
  uint32_t entry_type_ : 8 = 0;

 private:
  friend class NetTable;
  friend class NetTableBlock;
  friend class Graph;

  void setObjectIndex(uint32_t idx) { object_index_ = idx; }
};

}  // namespace syn
