// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// RTL source location (file and 1-based line) of a Module or an object. The
// file path is interned in the Design's NameInterner, so a SourceLoc is an
// 8-byte value. Modules store object locations sparsely (ObjFlags::has_loc).

#pragma once

#include <cstdint>
#include <type_traits>
#include <utility>

#include "syn/lir/Checksum.h"
#include "syn/lir/Ids.h"
#include "syn/lir/NameInterner.h"

namespace syn::lir {

class SourceLoc
{
 public:
  constexpr SourceLoc() = default;
  // An invalid `file` gives the empty location SourceLoc{} (line 0).
  explicit constexpr SourceLoc(NameId file, uint32_t line = 0)
      : file_(file), line_(file.isValid() ? line : 0)
  {
  }

  constexpr NameId file() const { return file_; }
  constexpr uint32_t line() const { return line_; }
  constexpr bool isValid() const { return file_.isValid(); }

  // Deterministic checksum (0 if invalid); `file()` is read from `names`.
  uint64_t checksum(const NameInterner& names) const
  {
    if (!isValid()) {
      return 0;
    }
    return checksumCombine(names.checksum(file_), line_);
  }

  constexpr bool operator==(const SourceLoc&) const = default;

  template <typename H>
  friend H AbslHashValue(H h, SourceLoc loc)
  {
    return H::combine(std::move(h), loc.file_, loc.line_);
  }

 private:
  NameId file_;
  uint32_t line_ = 0;
};

static_assert(sizeof(SourceLoc) == 8, "SourceLoc must stay 8 bytes");
static_assert(std::is_trivially_copyable_v<SourceLoc>);

}  // namespace syn::lir
