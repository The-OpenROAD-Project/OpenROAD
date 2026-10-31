// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// Object attributes: optimization restrictions on one object, set by
// importers (ODB do-not-touch, `keep`) and by SDC constraints. The Editor
// refuses edits they forbid, and Editor::sweep keeps every object that has
// any. Few objects have attributes; ObjFlags::has_attr marks those that do.
// To list them, scan the objects in id order (Module.h).

#pragma once

#include <cstdint>
#include <type_traits>

#include "syn/lir/Checksum.h"

namespace syn::lir {

// Which optimizations may cross the ports of an instance, from least to most
// restrictive.
enum class BoundaryOpt : uint8_t
{
  // Every boundary optimization is allowed (the default).
  kFull = 0,
  // Only constants may cross the ports, so the instance may be specialized
  // for constant inputs. No port removal or merging, no inverter pushing, no
  // logic or register moves across.
  kConstOnly = 1,
  // Nothing crosses and the ports stay as they are (e.g. UPF elements).
  kNone = 2,
};

class ObjAttributes
{
 public:
  constexpr ObjAttributes() = default;

  // Never removed, replaced, rewired, renamed or resized.
  constexpr bool dontTouch() const { return dont_touch_; }
  // Only the master may change, within its family of equivalent cells.
  constexpr bool sizeOnly() const { return size_only_; }
  // Referenced by a timing constraint: keeps its name, fanins and readers,
  // but may be resized.
  constexpr bool isConstrained() const { return constrained_; }
  // Its value must stay: never removed or replaced, and its readers keep it
  // as driver. Its fanin cone may change, and it may gain readers.
  constexpr bool keepValue() const { return keep_value_; }
  // kInst only: the instance is never inlined. It may still be rewired or
  // removed. A boundary restriction implies it.
  constexpr bool keepHierarchy() const { return hier_ >= kKept; }
  // kInst only: which optimizations may cross the instance's ports.
  constexpr BoundaryOpt boundaryOpt() const
  {
    return hier_ <= kKept ? BoundaryOpt::kFull
                          : static_cast<BoundaryOpt>(hier_ - kKept);
  }

  // Setters return *this, so attributes chain:
  //   ObjAttributes().setSizeOnly().setConstrained()
  constexpr ObjAttributes& setDontTouch(bool on = true)
  {
    dont_touch_ = on;
    return *this;
  }
  constexpr ObjAttributes& setSizeOnly(bool on = true)
  {
    size_only_ = on;
    return *this;
  }
  constexpr ObjAttributes& setConstrained(bool on = true)
  {
    constrained_ = on;
    return *this;
  }
  constexpr ObjAttributes& setKeepValue(bool on = true)
  {
    keep_value_ = on;
    return *this;
  }
  // On: kept, keeping any boundary restriction. Off: inlinable, which also
  // drops the boundary restriction (a restricted instance is always kept).
  constexpr ObjAttributes& setKeepHierarchy(bool on = true)
  {
    if (!on) {
      hier_ = kInlinable;
    } else if (hier_ < kKept) {
      hier_ = kKept;
    }
    return *this;
  }
  // A restriction also keeps the instance; kFull lowers a restricted
  // instance to kept (not to inlinable).
  constexpr ObjAttributes& setBoundaryOpt(BoundaryOpt level)
  {
    if (level != BoundaryOpt::kFull) {
      hier_ = static_cast<uint8_t>(kKept + static_cast<uint8_t>(level));
    } else if (hier_ > kKept) {
      hier_ = kKept;
    }
    return *this;
  }

  // True if any attribute is set (the object needs a map entry).
  constexpr bool any() const
  {
    return dont_touch_ || size_only_ || constrained_ || keep_value_
           || hier_ != kInlinable;
  }
  // May be removed or replaced, and its readers may be moved to another
  // driver.
  constexpr bool removable() const
  {
    return !dont_touch_ && !size_only_ && !constrained_ && !keep_value_;
  }
  // Its fanins (and data other than the master) may change.
  constexpr bool restructurable() const
  {
    return !dont_touch_ && !size_only_ && !constrained_;
  }
  // Its name may change.
  constexpr bool renamable() const { return restructurable(); }
  // Its master (kCell LibCellId, RegOut RegClassId) may change.
  constexpr bool resizable() const { return !dont_touch_; }

  // Adds the attributes of `other` (the stricter hierarchy level wins).
  constexpr ObjAttributes& operator|=(const ObjAttributes& other)
  {
    dont_touch_ = dont_touch_ || other.dont_touch_;
    size_only_ = size_only_ || other.size_only_;
    constrained_ = constrained_ || other.constrained_;
    keep_value_ = keep_value_ || other.keep_value_;
    hier_ = hier_ > other.hier_ ? hier_ : other.hier_;
    return *this;
  }
  constexpr bool operator==(const ObjAttributes&) const = default;

  // Deterministic checksum of the attributes (0 if none are set).
  constexpr uint64_t checksum() const
  {
    if (!any()) {
      return 0;
    }
    const uint64_t bits
        = (dont_touch_ ? 1ULL : 0ULL) | (size_only_ ? 2ULL : 0ULL)
          | (constrained_ ? 4ULL : 0ULL) | (keep_value_ ? 8ULL : 0ULL)
          | (static_cast<uint64_t>(hier_) << 4u);
    return checksumCombine(kChecksumSeed, bits);
  }

 private:
  // keep_hierarchy and the boundary level in one field, ordered by
  // strictness so merging takes the maximum.
  static constexpr uint8_t kInlinable = 0;
  static constexpr uint8_t kKept = 1;  // + BoundaryOpt: 1 full, 2, 3

  bool dont_touch_ : 1 = false;
  bool size_only_ : 1 = false;
  bool constrained_ : 1 = false;
  bool keep_value_ : 1 = false;
  uint8_t hier_ : 2 = kInlinable;
};

static_assert(sizeof(ObjAttributes) == 1, "ObjAttributes must stay one byte");
static_assert(std::is_trivially_copyable_v<ObjAttributes>);

}  // namespace syn::lir
