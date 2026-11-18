// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// L-IR module storage: a flat table of objects in which every object is also
// its own output value. Each object is one 16-byte ObjRecord; fanins live in
// one shared arena. A Module is created by ModuleBuilder and changed in place
// by the Editor. Concurrent reads are safe; mutation needs exclusive access.

#pragma once

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <optional>
#include <span>
#include <utility>
#include <vector>

#include "absl/base/attributes.h"
#include "absl/container/flat_hash_map.h"
#include "syn/lir/ConstView.h"
#include "syn/lir/Ids.h"
#include "syn/lir/ObjAttributes.h"
#include "syn/lir/Op.h"
#include "syn/lir/OperatorSpec.h"
#include "syn/lir/RegClassPool.h"
#include "syn/lir/RegPinTypes.h"
#include "syn/lir/SourceLoc.h"

namespace syn::lir {

class Design;

class ConstPool;

// Per-object flags, one byte in ObjRecord. The fields have no default
// initializers (ObjRecord stays trivial): value-initialize with ObjFlags{}.
struct ObjFlags
{
  bool is_signed : 1;  // the operation is signed (only if hasSignedFlag(op))
  bool dead : 1;       // removed; reclaimed by compact()
  // A kConst of at most kMaxInlineConstWidth bits, stored in the record:
  // `bits` in `data`, `x_mask` in `fanin_begin`. Small kConsts are always
  // inline.
  bool inline_const : 1;
  // Has an entry in the attribute table (Module::objAttributes). Dead
  // objects keep it until Editor::compact().
  bool has_attr : 1;
  // Has an entry in the source location table (Module::objSourceLoc). Dead
  // objects keep it until Editor::compact().
  bool has_loc : 1;
  // Reserved for persistent flags. Zero.
  uint8_t reserved : 1;
  // Scratch marks; prefer a pass-local vector. Only a pass that holds the
  // Editor exclusively may use them, and it must clear them before
  // returning. Not thread-safe.
  bool mark1 : 1;
  bool mark2 : 1;

  bool operator==(const ObjFlags&) const = default;
};
static_assert(sizeof(ObjFlags) == 1, "ObjFlags must stay one byte");

// fanin_count value meaning "too many to count here": the real count is
// stored in the fanin arena at fanin_begin, and the fanins follow it.
inline constexpr uint16_t kFaninCountEscape = 0xFFFF;

// fanin_begin and data are ObjIndex wide because they can hold the pair links
// (ObjIds) between state halves.
struct ObjRecord
{
  Op op;
  ObjFlags flags;
  uint16_t fanin_count;
  int32_t width;
  // Index into the fanin arena. Without fanins, it holds the `x_mask` of an
  // inline kConst, or a state output's link to its input (or kInvalidValue).
  ObjIndex fanin_begin;
  ObjIndex data;  // by op: port index, ConstId, inline kConst `bits`, ModId,
                  // slice lsb, LibCellId, RegClassId, state input's link to
                  // its output, ...
};
static_assert(sizeof(ObjRecord) == 8 + 2 * sizeof(ObjIndex),
              "ObjRecord must stay 16 bytes (24 with 64-bit object ids)");

// The ids 0..size-1 of a module, as a range for range-based for loops.
// Includes dead objects; callers skip them with Module::isDead.
class ObjRange
{
 public:
  class Iterator
  {
   public:
    using iterator_category = std::forward_iterator_tag;
    using value_type = ObjId;
    using difference_type = std::ptrdiff_t;
    using pointer = const ObjId*;
    using reference = ObjId;

    Iterator() = default;
    explicit Iterator(ObjIndex value) : value_(value) {}
    ObjId operator*() const { return ObjId(value_); }
    Iterator& operator++()
    {
      ++value_;
      return *this;
    }
    Iterator operator++(int)
    {
      Iterator old = *this;
      ++value_;
      return old;
    }
    friend bool operator==(Iterator a, Iterator b) = default;

   private:
    ObjIndex value_ = 0;
  };

  explicit ObjRange(ObjIndex size) : size_(size) {}
  Iterator begin() const { return Iterator(0); }
  Iterator end() const { return Iterator(size_); }
  ObjIndex size() const { return size_; }

 private:
  ObjIndex size_;
};

// Maps old object ids to new ones after Editor::compact() or a rebuild
// (ModuleBuilder::install). Removed objects, and objects a rebuild did not
// copy, map to an invalid id.
class IdRemap
{
 public:
  IdRemap() = default;
  explicit IdRemap(std::vector<ObjId> map) : map_(std::move(map)) {}
  IdRemap(IdRemap&&) noexcept = default;
  IdRemap& operator=(IdRemap&&) noexcept = default;
  IdRemap(const IdRemap&) = delete;
  IdRemap& operator=(const IdRemap&) = delete;

  // Every id maps to itself (nothing changed).
  static IdRemap identity()
  {
    IdRemap remap;
    remap.identity_ = true;
    return remap;
  }

  bool isIdentity() const { return identity_; }
  ObjId operator()(ObjId old_id) const
  {
    if (identity_) {
      return old_id;
    }
    return old_id.value() < map_.size() ? map_[old_id.value()] : ObjId();
  }

 private:
  std::vector<ObjId> map_;
  bool identity_ = false;
};

// Module attributes, set by importers and passes (ModuleBuilder::
// setModuleAttributes, Design::setModuleAttributes) and kept by rebuilds.
struct ModuleAttributes
{
  bool blackbox : 1 = false;        // interface only; never inlined
  bool keep_hierarchy : 1 = false;  // never inlined
  // Not optimized, and not copied by uniquify; implies frozen.
  bool dont_touch : 1 = false;
  // Derived, set by Design::markFrozenModules: the module is dont_touch, or
  // is instantiated by a dont_touch instance or from a frozen module.
  bool frozen_by_parent : 1 = false;

  bool operator==(const ModuleAttributes&) const = default;
};

// Object attributes: ObjAttributes (ObjAttributes.h), stored behind
// ObjFlags::has_attr; see objAttributes().

class Module
{
 public:
  // Capacity: object ids 0..kMaxObjects-1 (the all-ones id is invalid) and
  // fanin arena slots 0..kMaxFaninSlots-1. Appending past the limit fails
  // with an error (ResourceExhausted from the Editor).
  static constexpr ObjIndex kMaxObjects = ObjId::kInvalidValue;
  static constexpr ObjIndex kMaxFaninSlots = ObjId::kInvalidValue;
  // Test hook: lowers the capacity of every module. The defaults restore
  // the limits. Not thread-safe.
  static void setCapacityForTesting(ObjIndex max_objects = kMaxObjects,
                                    ObjIndex max_fanin_slots = kMaxFaninSlots);

  // Construction and copy.
  Module() = default;
  Module(Module&&) noexcept = default;
  Module& operator=(Module&&) noexcept = default;
  Module& operator=(const Module&) = delete;
  // Explicit deep copy.
  Module clone() const { return Module(*this); }

  // Size and liveness.
  // Number of object slots, including dead objects.
  ObjIndex size() const { return static_cast<ObjIndex>(objs_.size()); }
  ObjRange objects() const { return ObjRange(size()); }
  bool isValid(ObjId id) const { return id.value() < objs_.size(); }
  bool isDead(ObjId id) const { return record(id).flags.dead; }

  // Op, width, flags and signedness.
  const ObjRecord& record(ObjId id) const;
  Op op(ObjId id) const { return record(id).op; }
  int width(ObjId id) const { return record(id).width; }
  // The raw payload. Prefer the typed accessors below; raw access is for
  // checks and copies. ObjIndex wide; only a state input's link to its
  // output (an ObjId) can exceed 32 bits.
  ObjIndex data(ObjId id) const { return record(id).data; }
  bool isSigned(ObjId id) const { return record(id).flags.is_signed; }
  Signedness signedness(ObjId id) const;
  bool isInlineConst(ObjId id) const { return record(id).flags.inline_const; }

  // Fanins.
  int faninCount(ObjId id) const;
  std::span<const ObjId> fanins(ObjId id) const ABSL_ATTRIBUTE_LIFETIME_BOUND;
  ObjId fanin(ObjId id, int k) const;

  // Typed reads of the payload of each op (Op.h); each asserts the op.
  //
  // The value of a kConst object, inline or pooled. Always read constants
  // through this, never through data(). `pool` is the owning Design's pool.
  ConstView constValue(ObjId id, const ConstPool& pool) const;
  // Position of a kPi in inputs(), or of a kPo in outputs().
  int portIndex(ObjId port) const;
  // First bit of its fanin that a kSlice selects.
  int sliceLsb(ObjId slice) const;
  // Number of copies of its fanin that a kReplicate concatenates.
  int replicateCount(ObjId replicate) const;
  // Module that a kInst instantiates.
  ModId instModule(ObjId inst) const;
  // Liberty cell of a kCell.
  LibCellId cellId(ObjId cell) const;
  // Truth table of a kLut: a pooled constant of 2^faninCount bits.
  ConstId lutTable(ObjId lut) const;
  // Register class of a kRegOut, an id in regClasses() (see regClass()).
  RegClassId regClassId(ObjId reg_out) const;
  // Owner of a kFan (a kInst, kCell or kRegOut): its only fanin.
  ObjId fanOwner(ObjId fan) const;
  // Output of its owner that a kFan carries: an output port of the child
  // of a kInst, a Liberty output pin of a kCell or mapped kRegOut, or a
  // RegOutput of a generic kRegOut (see fanRegOutput()).
  int fanOutput(ObjId fan) const;
  // Output (QN or scan-out) that a kFan of a generic kRegOut carries.
  RegOutput fanRegOutput(ObjId fan) const;

  // Ports.
  // Pi objects in port order; portIndex(pi) is the index in this list.
  std::span<const ObjId> inputs() const ABSL_ATTRIBUTE_LIFETIME_BOUND;
  // Po objects in port order; portIndex(po) is the index in this list.
  std::span<const ObjId> outputs() const ABSL_ATTRIBUTE_LIFETIME_BOUND;
  // Port names are part of the interface: every port of a well-formed module
  // has one, unique among the module's inputs and outputs. They are stored
  // by port, not in the object name column, so naming the ports does not
  // allocate that column.
  NameId inputName(int port) const;
  NameId outputName(int port) const;

  // Object names, attributes and source locations.
  // Name of an object, or invalid if anonymous: a port's name; the instance
  // name of a kInst, kCell or mapped kRegOut (else a net name); otherwise the
  // net it drives (Q for a generic kRegOut). Best-effort paths, not checked
  // for uniqueness; names that must survive carry ObjAttributes.
  NameId objName(ObjId id) const;
  // True if the name of object `id` names a net: false for kInst, kCell and
  // a mapped kRegOut, whose names are instance names.
  bool nameIsNetName(ObjId id) const;
  // True if object `id` may take a net name carried over from another object:
  // its name names a net and is not its identity (not a kPi, kPo, kRegOut or
  // kLoopOut).
  bool takesNetName(ObjId id) const;
  // Attributes of object `id`; none for most objects.
  ObjAttributes objAttributes(ObjId id) const;
  // True if some object (dead ones included) has attributes.
  bool hasObjAttributes() const { return !obj_attrs_.empty(); }
  // Number of objects that have attributes, dead ones included.
  int64_t numObjAttributes() const { return std::ssize(obj_attrs_); }
  // Source location of object `id`; invalid for most objects.
  SourceLoc objSourceLoc(ObjId id) const;
  // True if some object, possibly a dead one, has a source location.
  bool hasObjSourceLocs() const { return !obj_locs_.empty(); }
  // Number of objects that have a source location, dead ones included.
  int64_t numObjSourceLocs() const { return std::ssize(obj_locs_); }

  // State pairs.
  // The input of a state output (kRegOut, kLoopOut), or an invalid id until
  // the input is added. The pair link is not a fanin: a state output has
  // none.
  ObjId stateIn(ObjId out) const;
  // The output of a state input (kRegIn, kLoopIn).
  ObjId stateOut(ObjId in) const;

  // Module name, attributes, revision and operator status.
  NameId name() const { return name_; }
  const ModuleAttributes& attributes() const { return attributes_; }
  // True if passes must leave the module alone: frozen by a parent or
  // dont_touch.
  bool isFrozen() const;
  SourceLoc sourceLoc() const { return source_loc_; }
  // Id order is topological: every fanin of an object has a smaller id (a
  // state output's pair link, stateIn(), is not a fanin). Set by
  // ModuleBuilder::finish() and compact(); cleared by an edit that breaks
  // it.
  bool isOrdered() const { return ordered_; }
  // Removed by Design::removeModule(): empty and anonymous; its ModId stays
  // reserved and no instance may refer to it.
  bool isRemoved() const { return removed_; }
  // Incremented by every structural change; analyses cache against it.
  int64_t revision() const { return revision_; }
  // Operator implementation record (OperatorSpec.h); empty for an
  // ordinary module. Set by Design::setOperatorImpl().
  const std::optional<OperatorImpl>& operatorImpl() const { return impl_; }
  bool isOperatorImpl() const { return impl_.has_value(); }
  // An implementation module not edited since its record was set: its
  // gates are still exactly its architecture's.
  bool isPristineOperator() const;

  // Pools, memory and checksums.
  // The module's register classes (regClassId() is an id in this pool). Per
  // module, so the module's owner adds classes freely.
  RegClassPool& regClasses() { return reg_classes_; }
  const RegClassPool& regClasses() const { return reg_classes_; }
  // Class of register `reg` (a RegOut or RegIn of this module).
  RegClass regClass(ObjId reg) const;
  // Slots used in the fanin arena, including escape headers and holes.
  size_t faninArenaSize() const { return fanins_.size(); }
  // Heap bytes held by this module (capacities, not sizes).
  size_t memoryBytes() const;
  // Deterministic checksum of object `id`, from its id, fanin ids and the
  // values (not handles) of its pooled data, resolved through `design`.
  uint64_t objChecksum(const Design& design, ObjId id) const;
  // Deterministic checksum of the module: metadata, ports and objects.
  uint64_t checksum(const Design& design) const;

 private:
  // copyModule(), setModuleAttributes(), setOperatorImpl(), removeModule()
  friend class Design;
  friend class Editor;
  friend class ModuleBuilder;

  Module(const Module&) = default;

  // Appends a record and its fanins. Clears flags.is_signed if `op` cannot
  // carry it. A state output takes no fanins and starts unlinked. Returns
  // an invalid id if the module or its fanin arena is full.
  ObjId appendObject(Op op,
                     int width,
                     std::span<const ObjId> fanins,
                     ObjIndex data,
                     ObjFlags flags);
  // Sets the pair link of state output `out` to its input `in` (stateIn()).
  void linkStateIn(ObjId out, ObjId in);
  // True if `extra_slots` more fanin slots fit in the arena.
  bool faninArenaFits(size_t extra_slots) const;
  // True if `extra_objects` more objects fit in the module.
  bool objectsFit(size_t extra_objects) const;
  // Appends an inline kConst; width <= kMaxInlineConstWidth. Bits above the
  // width are cleared.
  ObjId appendInlineConst(int width, uint32_t bits, uint32_t x_mask);
  // Arena index of fanin 0 of a record that has fanins.
  static ObjIndex firstFaninSlot(const ObjRecord& r);
  // The fanins of `r` in `arena`, a view of fanins_ (mutable for the
  // builder and the Editor).
  template <typename T>
  static std::span<T> faninSpan(const ObjRecord& r, std::span<T> arena);
  // Slots object `id` uses in the fanin arena: its fanins and, if it has
  // one, its escape header.
  size_t faninSlotsUsed(ObjId id) const;
  // Points `r` at the end of `arena` for `count` fanins: sets fanin_count
  // and fanin_begin, and appends the escape header if `count` needs one.
  // The caller then appends the fanins.
  static void appendFaninHeader(ObjRecord& r,
                                std::vector<ObjId>& arena,
                                size_t count);
  // Names object `id`: the port name of a Pi or Po, else the name column
  // entry (growing the column if needed).
  void assignName(ObjId id, NameId name);
  // Map entry of `id`, which must have one (has_attr).
  ObjAttributes findObjAttributes(ObjId id) const;
  // Sets (or, with no attribute set, removes) the entry of `id`, keeping
  // has_attr in step.
  void assignObjAttributes(ObjId id, ObjAttributes attrs);
  // Map entry of `id`, which must have one (has_loc).
  SourceLoc findObjSourceLoc(ObjId id) const;
  // Sets (or, when `!loc.isValid()`, removes) the source location of `id`,
  // keeping has_loc in step.
  void assignObjSourceLoc(ObjId id, SourceLoc loc);

  NameId name_;
  ModuleAttributes attributes_;
  SourceLoc source_loc_;
  bool ordered_ = false;
  bool removed_ = false;
  int64_t revision_ = 0;
  std::optional<OperatorImpl> impl_;
  std::vector<ObjRecord> objs_;
  std::vector<ObjId> fanins_;
  std::vector<ObjId> inputs_;
  std::vector<ObjId> outputs_;
  std::vector<NameId> input_names_;   // by input port
  std::vector<NameId> output_names_;  // by output port
  std::vector<NameId> obj_names_;  // cold; may be shorter than objs_; no ports
  // By object id; never iterated (see ObjAttributes and SourceLoc).
  absl::flat_hash_map<ObjIndex, ObjAttributes> obj_attrs_;
  absl::flat_hash_map<ObjIndex, SourceLoc> obj_locs_;
  RegClassPool reg_classes_;  // empty (no allocation) without registers
};

// Inline definitions.

inline const ObjRecord& Module::record(ObjId id) const
{
  assert(isValid(id));
  return objs_[id.value()];
}

inline Signedness Module::signedness(ObjId id) const
{
  return isSigned(id) ? Signedness::kSigned : Signedness::kUnsigned;
}

inline int Module::faninCount(ObjId id) const
{
  const ObjRecord& r = record(id);
  return r.fanin_count != kFaninCountEscape
             ? r.fanin_count
             : static_cast<int>(fanins_[r.fanin_begin].value());
}

inline std::span<const ObjId> Module::fanins(ObjId id) const
    ABSL_ATTRIBUTE_LIFETIME_BOUND
{
  return faninSpan(record(id), std::span<const ObjId>(fanins_));
}

inline ObjId Module::fanin(ObjId id, int k) const
{
  assert(k >= 0 && k < faninCount(id));
  return fanins(id)[k];
}

inline int Module::portIndex(ObjId port) const
{
  assert(op(port) == Op::kPi || op(port) == Op::kPo);
  return static_cast<int>(record(port).data);
}

inline int Module::sliceLsb(ObjId slice) const
{
  assert(op(slice) == Op::kSlice);
  return static_cast<int>(record(slice).data);
}

inline int Module::replicateCount(ObjId replicate) const
{
  assert(op(replicate) == Op::kReplicate);
  return static_cast<int>(record(replicate).data);
}

inline ModId Module::instModule(ObjId inst) const
{
  assert(op(inst) == Op::kInst);
  return ModId(static_cast<uint32_t>(record(inst).data));
}

inline LibCellId Module::cellId(ObjId cell) const
{
  assert(op(cell) == Op::kCell);
  return LibCellId(static_cast<uint32_t>(record(cell).data));
}

inline ConstId Module::lutTable(ObjId lut) const
{
  assert(op(lut) == Op::kLut);
  return ConstId(static_cast<uint32_t>(record(lut).data));
}

inline RegClassId Module::regClassId(ObjId reg_out) const
{
  assert(op(reg_out) == Op::kRegOut);
  return RegClassId(static_cast<uint32_t>(record(reg_out).data));
}

inline ObjId Module::fanOwner(ObjId fan) const
{
  assert(op(fan) == Op::kFan);
  return fanin(fan, 0);
}

inline int Module::fanOutput(ObjId fan) const
{
  assert(op(fanOwner(fan)) == Op::kInst || op(fanOwner(fan)) == Op::kCell
         || op(fanOwner(fan)) == Op::kRegOut);
  return static_cast<int>(record(fan).data);
}

inline RegOutput Module::fanRegOutput(ObjId fan) const
{
  assert(op(fanOwner(fan)) == Op::kRegOut
         && !regClass(fanOwner(fan)).isMapped());
  return static_cast<RegOutput>(record(fan).data);
}

inline std::span<const ObjId> Module::inputs() const
    ABSL_ATTRIBUTE_LIFETIME_BOUND
{
  return inputs_;
}

inline std::span<const ObjId> Module::outputs() const
    ABSL_ATTRIBUTE_LIFETIME_BOUND
{
  return outputs_;
}

inline NameId Module::inputName(int port) const
{
  assert(port >= 0 && port < std::ssize(input_names_));
  return input_names_[port];
}

inline NameId Module::outputName(int port) const
{
  assert(port >= 0 && port < std::ssize(output_names_));
  return output_names_[port];
}

inline NameId Module::objName(ObjId id) const
{
  const ObjRecord& r = record(id);
  if (r.op == Op::kPi) {
    return r.data < input_names_.size() ? input_names_[r.data] : NameId();
  }
  if (r.op == Op::kPo) {
    return r.data < output_names_.size() ? output_names_[r.data] : NameId();
  }
  return id.value() < obj_names_.size() ? obj_names_[id.value()] : NameId();
}

inline ObjAttributes Module::objAttributes(ObjId id) const
{
  return record(id).flags.has_attr ? findObjAttributes(id) : ObjAttributes();
}

inline SourceLoc Module::objSourceLoc(ObjId id) const
{
  return record(id).flags.has_loc ? findObjSourceLoc(id) : SourceLoc();
}

inline ObjId Module::stateIn(ObjId out) const
{
  assert(isStateOut(op(out)));
  return ObjId(record(out).fanin_begin);
}

inline ObjId Module::stateOut(ObjId in) const
{
  assert(isStateIn(op(in)));
  return ObjId(record(in).data);
}

inline bool Module::isFrozen() const
{
  return attributes_.frozen_by_parent || attributes_.dont_touch;
}

inline void Module::linkStateIn(ObjId out, ObjId in)
{
  assert(isStateOut(op(out)));
  objs_[out.value()].fanin_begin = in.value();
}

inline ObjIndex Module::firstFaninSlot(const ObjRecord& r)
{
  return r.fanin_begin + (r.fanin_count == kFaninCountEscape ? 1 : 0);
}

template <typename T>
inline std::span<T> Module::faninSpan(const ObjRecord& r, std::span<T> arena)
{
  if (r.fanin_count == 0) {
    return {};  // fanin_begin may hold other data (ObjRecord)
  }
  if (r.fanin_count != kFaninCountEscape) {
    return {arena.data() + r.fanin_begin, r.fanin_count};
  }
  return {arena.data() + r.fanin_begin + 1, arena[r.fanin_begin].value()};
}

}  // namespace syn::lir
