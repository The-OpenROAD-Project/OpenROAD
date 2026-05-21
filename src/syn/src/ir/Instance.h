// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#pragma once

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include "syn/ir/Bundle.h"
#include "syn/ir/Const.h"
#include "syn/ir/ControlNet.h"
#include "syn/ir/Net.h"
#include "syn/ir/NetTableEntry.h"

namespace sta {
class LibertyCell;
}

namespace syn {

// Base class for all instances in the graph.
class Instance : public NetTableEntry
{
 public:
  MetadataId metadataId() const { return metadata_id_; }
  void setMetadataId(MetadataId id) { metadata_id_ = id; }

  // Destroy this instance by dispatching on entryType() to call the correct
  // derived destructor.  Instance has no virtual destructor for size reasons.
  void destroy();

  // Returns the output width of this instance by dispatching on entryType().
  uint32_t outputWidth() const;

  // Returns true if this instance has observable effects (I/O, names, etc.).
  bool hasEffects() const;

  // Returns true if this instance has state. This function is conservative
  // on some instance types, e.g. always returns true for Other
  bool hasState() const;

  // Returns true if this instance represents a bitwise operation, and as
  // such can be sliced into smaller parts
  bool isSliceable() const;

  // Returns true if this instance is expected as a functional component
  // in a fully mapped design (I/O, Target, Other, LoopBreaker); does not
  // return true for Name
  bool isMapped() const;

  // Check if this instance is of type T (or a subclass of T).
  // e.g. is<Buffer>() matches both BufferFine and BufferWide.
  template <typename T>
  bool is() const;

  // Assert is<T>() and return the casted pointer.
  template <typename T>
  const T* as() const
  {
    assert(is<T>());
    return static_cast<const T*>(this);
  }
  template <typename T>
  T* as()
  {
    assert(is<T>());
    return static_cast<T*>(this);
  }

  // Return the casted pointer if is<T>(), otherwise nullptr.
  template <typename T>
  const T* try_as() const
  {
    return is<T>() ? static_cast<const T*>(this) : nullptr;
  }
  template <typename T>
  T* try_as()
  {
    return is<T>() ? static_cast<T*>(this) : nullptr;
  }

  // Return a Bundle containing all input nets.
  Bundle inputs() const
  {
    std::vector<Net> nets;
    visit([&](Net net) { nets.push_back(net); });
    return Bundle::fromVec(std::move(nets));
  }

  // Visit all input Nets.
  template <typename F>
  void visit(F&& fn) const;

  // Visit all input Nets mutably.
  template <typename F>
  void visitMut(F&& fn);

  // Visit the input Nets connected to output bit `i`.
  // For bitwise ops, visits only the corresponding input bit(s).
  // For non-bitwise ops, visits all inputs.
  template <typename F>
  void visitSlice(uint32_t i, F&& fn) const;

  // Mutable version of visitSlice.
  template <typename F>
  void visitSliceMut(uint32_t i, F&& fn);

  // Returns true if this instance is stored inline in a table slot.
  bool isInline() const;

  // Get/set the base NetTableId (dispatches to concrete heap type).
  NetTableId baseIndex() const;
  void setBaseIndex(NetTableId id);

 private:
  friend class Graph;

  uint32_t sizeOf() const;
  void heapBytes(size_t& name_bytes, size_t& vector_bytes) const;

 protected:
  Instance() = default;

  MetadataId metadata_id_{0};
};

// Macro for heap-allocated instance types that need a base_index_ field.
#define SYN_HEAP_INSTANCE_FIELDS   \
  NetTableId baseIndex() const     \
  {                                \
    return base_index_;            \
  }                                \
  void setBaseIndex(NetTableId id) \
  {                                \
    base_index_ = id;              \
  }                                \
                                   \
 private:                          \
  NetTableId base_index_ = 0;      \
                                   \
 public:

// Placeholder entry references a heap-allocated instance and a bit offset.
class PlaceholderEntry : public NetTableEntry
{
 public:
  PlaceholderEntry(Instance* inst, uint32_t offset)
      : offset_(offset), instance_(inst)
  {
    setEntryType(EntryType::kPlaceholder);
  }

  Instance* instance() const { return instance_; }
  void setInstance(Instance* inst) { instance_ = inst; }

  uint32_t offset() const { return offset_; }
  void setOffset(uint32_t off) { offset_ = off; }

 private:
  uint32_t offset_;     // bit offset within instance's output
  Instance* instance_;  // pointer last to avoid alignment padding
};

static_assert(sizeof(PlaceholderEntry) == 16);

inline constexpr size_t kSlotSize = sizeof(PlaceholderEntry);

// ============================================================
// Tie-off instances (always single-bit, no fine/wide)
// ============================================================

class TieHigh : public Instance
{
 public:
  TieHigh() { setEntryType(EntryType::kTieHigh); }
  template <typename F>
  void visit(F&&) const
  {
  }
  template <typename F>
  void visitMut(F&&)
  {
  }
};

class TieLow : public Instance
{
 public:
  TieLow() { setEntryType(EntryType::kTieLow); }
  template <typename F>
  void visit(F&&) const
  {
  }
  template <typename F>
  void visitMut(F&&)
  {
  }
};

class TieX : public Instance
{
 public:
  TieX() { setEntryType(EntryType::kTieX); }
  template <typename F>
  void visit(F&&) const
  {
  }
  template <typename F>
  void visitMut(F&&)
  {
  }
};

// ============================================================
// Operations with fine/wide variants
//
// Each family declares: base class, Fine subclass, Wide subclass,
// then defines plan()/construct() on the base after both are complete.
// ============================================================

// --- Buffer ---

class Buffer : public Instance
{
 public:
  BundleView a() const;
  void setA(uint64_t index, Net net);
  static size_t plan(const Bundle& a);
  static void construct(void* mem, Bundle a);

 protected:
  Buffer() = default;
};

class BufferFine : public Buffer
{
 public:
  friend class Instance;
  BufferFine() : a_(Net::zero()) { setEntryType(EntryType::kBufferFine); }
  explicit BufferFine(Net a) : a_(a) { setEntryType(EntryType::kBufferFine); }
  Net a() const { return a_; }
  void setA(Net net) { a_ = net; }
  template <typename F>
  void visit(F&& fn) const
  {
    fn(a_);
  }
  template <typename F>
  void visitMut(F&& fn)
  {
    fn(a_);
  }

 private:
  Net a_;
};

class BufferWide : public Buffer
{
 public:
  friend class Instance;
  BufferWide() { setEntryType(EntryType::kBufferWide); }
  explicit BufferWide(Bundle a) : a_(std::move(a))
  {
    setEntryType(EntryType::kBufferWide);
  }
  const Bundle& a() const { return a_; }
  void setA(uint64_t index, Net net);
  template <typename F>
  void visit(F&& fn) const
  {
    a_.visit(fn);
  }
  template <typename F>
  void visitMut(F&& fn)
  {
    a_.visitMut(fn);
  }

  SYN_HEAP_INSTANCE_FIELDS

 private:
  Bundle a_;
};

// --- Not ---

class Not : public Instance
{
 public:
  BundleView a() const;
  static size_t plan(const Bundle& a);
  static void construct(void* mem, Bundle a);

 protected:
  Not() = default;
};

class NotFine : public Not
{
 public:
  friend class Instance;
  NotFine() : a_(Net::zero()) { setEntryType(EntryType::kNotFine); }
  explicit NotFine(Net a) : a_(a) { setEntryType(EntryType::kNotFine); }
  Net a() const { return a_; }
  template <typename F>
  void visit(F&& fn) const
  {
    fn(a_);
  }
  template <typename F>
  void visitMut(F&& fn)
  {
    fn(a_);
  }

 private:
  Net a_;
};

class NotWide : public Not
{
 public:
  friend class Instance;
  NotWide() { setEntryType(EntryType::kNotWide); }
  explicit NotWide(Bundle a) : a_(std::move(a))
  {
    setEntryType(EntryType::kNotWide);
  }
  const Bundle& a() const { return a_; }
  template <typename F>
  void visit(F&& fn) const
  {
    a_.visit(fn);
  }
  template <typename F>
  void visitMut(F&& fn)
  {
    a_.visitMut(fn);
  }

  SYN_HEAP_INSTANCE_FIELDS

 private:
  Bundle a_;
};

// --- And ---

class And : public Instance
{
 public:
  BundleView a() const;
  BundleView b() const;
  static size_t plan(const Bundle& a, const Bundle& b);
  static void construct(void* mem, Bundle a, Bundle b);

 protected:
  And() = default;
};

class AndFine : public And
{
 public:
  friend class Instance;
  AndFine() : a_(Net::zero()), b_(Net::zero())
  {
    setEntryType(EntryType::kAndFine);
  }
  AndFine(Net a, Net b) : a_(a), b_(b) { setEntryType(EntryType::kAndFine); }
  Net a() const { return a_; }
  Net b() const { return b_; }
  template <typename F>
  void visit(F&& fn) const
  {
    fn(a_);
    fn(b_);
  }
  template <typename F>
  void visitMut(F&& fn)
  {
    fn(a_);
    fn(b_);
  }

 private:
  Net a_;
  Net b_;
};

class AndWide : public And
{
 public:
  friend class Instance;
  AndWide() { setEntryType(EntryType::kAndWide); }
  AndWide(Bundle a, Bundle b) : a_(std::move(a)), b_(std::move(b))
  {
    setEntryType(EntryType::kAndWide);
  }
  const Bundle& a() const { return a_; }
  const Bundle& b() const { return b_; }
  template <typename F>
  void visit(F&& fn) const
  {
    a_.visit(fn);
    b_.visit(fn);
  }
  template <typename F>
  void visitMut(F&& fn)
  {
    a_.visitMut(fn);
    b_.visitMut(fn);
  }

  SYN_HEAP_INSTANCE_FIELDS

 private:
  Bundle a_;
  Bundle b_;
};

// --- Or ---

class Or : public Instance
{
 public:
  BundleView a() const;
  BundleView b() const;
  static size_t plan(const Bundle& a, const Bundle& b);
  static void construct(void* mem, Bundle a, Bundle b);

 protected:
  Or() = default;
};

class OrFine : public Or
{
 public:
  friend class Instance;
  OrFine() : a_(Net::zero()), b_(Net::zero())
  {
    setEntryType(EntryType::kOrFine);
  }
  OrFine(Net a, Net b) : a_(a), b_(b) { setEntryType(EntryType::kOrFine); }
  Net a() const { return a_; }
  Net b() const { return b_; }
  template <typename F>
  void visit(F&& fn) const
  {
    fn(a_);
    fn(b_);
  }
  template <typename F>
  void visitMut(F&& fn)
  {
    fn(a_);
    fn(b_);
  }

 private:
  Net a_;
  Net b_;
};

class OrWide : public Or
{
 public:
  friend class Instance;
  OrWide() { setEntryType(EntryType::kOrWide); }
  OrWide(Bundle a, Bundle b) : a_(std::move(a)), b_(std::move(b))
  {
    setEntryType(EntryType::kOrWide);
  }
  const Bundle& a() const { return a_; }
  const Bundle& b() const { return b_; }
  template <typename F>
  void visit(F&& fn) const
  {
    a_.visit(fn);
    b_.visit(fn);
  }
  template <typename F>
  void visitMut(F&& fn)
  {
    a_.visitMut(fn);
    b_.visitMut(fn);
  }

  SYN_HEAP_INSTANCE_FIELDS

 private:
  Bundle a_;
  Bundle b_;
};

// --- Andnot ---

class Andnot : public Instance
{
 public:
  BundleView a() const;
  BundleView b() const;
  static size_t plan(const Bundle& a, const Bundle& b);
  static void construct(void* mem, Bundle a, Bundle b);

 protected:
  Andnot() = default;
};

class AndnotFine : public Andnot
{
 public:
  friend class Instance;
  AndnotFine() : a_(Net::zero()), b_(Net::zero())
  {
    setEntryType(EntryType::kAndnotFine);
  }
  AndnotFine(Net a, Net b) : a_(a), b_(b)
  {
    setEntryType(EntryType::kAndnotFine);
  }
  Net a() const { return a_; }
  Net b() const { return b_; }
  template <typename F>
  void visit(F&& fn) const
  {
    fn(a_);
    fn(b_);
  }
  template <typename F>
  void visitMut(F&& fn)
  {
    fn(a_);
    fn(b_);
  }

 private:
  Net a_;
  Net b_;
};

class AndnotWide : public Andnot
{
 public:
  friend class Instance;
  AndnotWide() { setEntryType(EntryType::kAndnotWide); }
  AndnotWide(Bundle a, Bundle b) : a_(std::move(a)), b_(std::move(b))
  {
    setEntryType(EntryType::kAndnotWide);
  }
  const Bundle& a() const { return a_; }
  const Bundle& b() const { return b_; }
  template <typename F>
  void visit(F&& fn) const
  {
    a_.visit(fn);
    b_.visit(fn);
  }
  template <typename F>
  void visitMut(F&& fn)
  {
    a_.visitMut(fn);
    b_.visitMut(fn);
  }

  SYN_HEAP_INSTANCE_FIELDS

 private:
  Bundle a_;
  Bundle b_;
};

// --- Xor ---

class Xor : public Instance
{
 public:
  BundleView a() const;
  BundleView b() const;
  static size_t plan(const Bundle& a, const Bundle& b);
  static void construct(void* mem, Bundle a, Bundle b);

 protected:
  Xor() = default;
};

class XorFine : public Xor
{
 public:
  friend class Instance;
  XorFine() : a_(Net::zero()), b_(Net::zero())
  {
    setEntryType(EntryType::kXorFine);
  }
  XorFine(Net a, Net b) : a_(a), b_(b) { setEntryType(EntryType::kXorFine); }
  Net a() const { return a_; }
  Net b() const { return b_; }
  template <typename F>
  void visit(F&& fn) const
  {
    fn(a_);
    fn(b_);
  }
  template <typename F>
  void visitMut(F&& fn)
  {
    fn(a_);
    fn(b_);
  }

 private:
  Net a_;
  Net b_;
};

class XorWide : public Xor
{
 public:
  friend class Instance;
  XorWide() { setEntryType(EntryType::kXorWide); }
  XorWide(Bundle a, Bundle b) : a_(std::move(a)), b_(std::move(b))
  {
    setEntryType(EntryType::kXorWide);
  }
  const Bundle& a() const { return a_; }
  const Bundle& b() const { return b_; }
  template <typename F>
  void visit(F&& fn) const
  {
    a_.visit(fn);
    b_.visit(fn);
  }
  template <typename F>
  void visitMut(F&& fn)
  {
    a_.visitMut(fn);
    b_.visitMut(fn);
  }

  SYN_HEAP_INSTANCE_FIELDS

 private:
  Bundle a_;
  Bundle b_;
};

// --- Mux ---

class Mux : public Instance
{
 public:
  Net sel() const;
  BundleView a() const;
  BundleView b() const;
  static size_t plan(Net sel, const Bundle& a, const Bundle& b);
  static void construct(void* mem, Net sel, Bundle a, Bundle b);

 protected:
  Mux() = default;
};

class MuxFine : public Mux
{
 public:
  friend class Instance;
  MuxFine() : sel_(Net::zero()), a_(Net::zero()), b_(Net::zero())
  {
    setEntryType(EntryType::kMuxFine);
  }
  MuxFine(Net sel, Net a, Net b) : sel_(sel), a_(a), b_(b)
  {
    setEntryType(EntryType::kMuxFine);
  }
  Net sel() const { return sel_; }
  Net a() const { return a_; }
  Net b() const { return b_; }
  template <typename F>
  void visit(F&& fn) const
  {
    fn(sel_);
    fn(a_);
    fn(b_);
  }
  template <typename F>
  void visitMut(F&& fn)
  {
    fn(sel_);
    fn(a_);
    fn(b_);
  }

  SYN_HEAP_INSTANCE_FIELDS

 private:
  Net sel_;
  Net a_;
  Net b_;
};

class MuxWide : public Mux
{
 public:
  friend class Instance;
  MuxWide() : sel_(Net::zero()) { setEntryType(EntryType::kMuxWide); }
  MuxWide(Net sel, Bundle a, Bundle b)
      : sel_(sel), a_(std::move(a)), b_(std::move(b))
  {
    setEntryType(EntryType::kMuxWide);
  }
  Net sel() const { return sel_; }
  const Bundle& a() const { return a_; }
  const Bundle& b() const { return b_; }
  template <typename F>
  void visit(F&& fn) const
  {
    fn(sel_);
    a_.visit(fn);
    b_.visit(fn);
  }
  template <typename F>
  void visitMut(F&& fn)
  {
    fn(sel_);
    a_.visitMut(fn);
    b_.visitMut(fn);
  }

  SYN_HEAP_INSTANCE_FIELDS

 private:
  Net sel_;
  Bundle a_;
  Bundle b_;
};

// --- Adc (add with carry) ---

class Adc : public Instance
{
 public:
  BundleView a() const;
  BundleView b() const;
  Net cin() const;
  static size_t plan(const Bundle& a, const Bundle& b, Net cin);
  static void construct(void* mem, Bundle a, Bundle b, Net cin);

 protected:
  Adc() = default;
};

class AdcFine : public Adc
{
 public:
  AdcFine() : a_(Net::zero()), b_(Net::zero()), cin_(Net::zero())
  {
    setEntryType(EntryType::kAdcFine);
  }
  AdcFine(Net a, Net b, Net cin) : a_(a), b_(b), cin_(cin)
  {
    setEntryType(EntryType::kAdcFine);
  }
  Net a() const { return a_; }
  Net b() const { return b_; }
  Net cin() const { return cin_; }
  template <typename F>
  void visit(F&& fn) const
  {
    fn(a_);
    fn(b_);
    fn(cin_);
  }
  template <typename F>
  void visitMut(F&& fn)
  {
    fn(a_);
    fn(b_);
    fn(cin_);
  }

  SYN_HEAP_INSTANCE_FIELDS

 private:
  Net a_;
  Net b_;
  Net cin_;
};

class AdcWide : public Adc
{
 public:
  AdcWide() : cin_(Net::zero()) { setEntryType(EntryType::kAdcWide); }
  AdcWide(Bundle a, Bundle b, Net cin)
      : a_(std::move(a)), b_(std::move(b)), cin_(cin)
  {
    setEntryType(EntryType::kAdcWide);
  }
  const Bundle& a() const { return a_; }
  const Bundle& b() const { return b_; }
  Net cin() const { return cin_; }
  template <typename F>
  void visit(F&& fn) const
  {
    a_.visit(fn);
    b_.visit(fn);
    fn(cin_);
  }
  template <typename F>
  void visitMut(F&& fn)
  {
    a_.visitMut(fn);
    b_.visitMut(fn);
    fn(cin_);
  }

  SYN_HEAP_INSTANCE_FIELDS

 private:
  Bundle a_;
  Bundle b_;
  Net cin_;
};

// ============================================================
// Operations without fine/wide split
// ============================================================

class Eq : public Instance
{
 public:
  Eq() { setEntryType(EntryType::kEq); }
  Eq(Bundle a, Bundle b) : a_(std::move(a)), b_(std::move(b))
  {
    setEntryType(EntryType::kEq);
  }
  const Bundle& a() const { return a_; }
  const Bundle& b() const { return b_; }
  template <typename F>
  void visit(F&& fn) const
  {
    a_.visit(fn);
    b_.visit(fn);
  }
  template <typename F>
  void visitMut(F&& fn)
  {
    a_.visitMut(fn);
    b_.visitMut(fn);
  }

  SYN_HEAP_INSTANCE_FIELDS

 private:
  Bundle a_;
  Bundle b_;
};

class ULt : public Instance
{
 public:
  ULt() { setEntryType(EntryType::kULt); }
  ULt(Bundle a, Bundle b) : a_(std::move(a)), b_(std::move(b))
  {
    setEntryType(EntryType::kULt);
  }
  const Bundle& a() const { return a_; }
  const Bundle& b() const { return b_; }
  template <typename F>
  void visit(F&& fn) const
  {
    a_.visit(fn);
    b_.visit(fn);
  }
  template <typename F>
  void visitMut(F&& fn)
  {
    a_.visitMut(fn);
    b_.visitMut(fn);
  }

  SYN_HEAP_INSTANCE_FIELDS

 private:
  Bundle a_;
  Bundle b_;
};

class SLt : public Instance
{
 public:
  SLt() { setEntryType(EntryType::kSLt); }
  SLt(Bundle a, Bundle b) : a_(std::move(a)), b_(std::move(b))
  {
    setEntryType(EntryType::kSLt);
  }
  const Bundle& a() const { return a_; }
  const Bundle& b() const { return b_; }
  template <typename F>
  void visit(F&& fn) const
  {
    a_.visit(fn);
    b_.visit(fn);
  }
  template <typename F>
  void visitMut(F&& fn)
  {
    a_.visitMut(fn);
    b_.visitMut(fn);
  }

  SYN_HEAP_INSTANCE_FIELDS

 private:
  Bundle a_;
  Bundle b_;
};

// --- Shift operations ---

class Shl : public Instance
{
 public:
  Shl() { setEntryType(EntryType::kShl); }
  Shl(Bundle a, Bundle b, uint32_t stride)
      : a_(std::move(a)), b_(std::move(b)), stride_(stride)
  {
    setEntryType(EntryType::kShl);
  }
  const Bundle& a() const { return a_; }
  const Bundle& b() const { return b_; }
  uint32_t stride() const { return stride_; }
  template <typename F>
  void visit(F&& fn) const
  {
    a_.visit(fn);
    b_.visit(fn);
  }
  template <typename F>
  void visitMut(F&& fn)
  {
    a_.visitMut(fn);
    b_.visitMut(fn);
  }

  SYN_HEAP_INSTANCE_FIELDS

 private:
  Bundle a_;
  Bundle b_;
  uint32_t stride_;
};

class UShr : public Instance
{
 public:
  UShr() { setEntryType(EntryType::kUShr); }
  UShr(Bundle a, Bundle b, uint32_t stride)
      : a_(std::move(a)), b_(std::move(b)), stride_(stride)
  {
    setEntryType(EntryType::kUShr);
  }
  const Bundle& a() const { return a_; }
  const Bundle& b() const { return b_; }
  uint32_t stride() const { return stride_; }
  template <typename F>
  void visit(F&& fn) const
  {
    a_.visit(fn);
    b_.visit(fn);
  }
  template <typename F>
  void visitMut(F&& fn)
  {
    a_.visitMut(fn);
    b_.visitMut(fn);
  }

  SYN_HEAP_INSTANCE_FIELDS

 private:
  Bundle a_;
  Bundle b_;
  uint32_t stride_;
};

class SShr : public Instance
{
 public:
  SShr() { setEntryType(EntryType::kSShr); }
  SShr(Bundle a, Bundle b, uint32_t stride)
      : a_(std::move(a)), b_(std::move(b)), stride_(stride)
  {
    setEntryType(EntryType::kSShr);
  }
  const Bundle& a() const { return a_; }
  const Bundle& b() const { return b_; }
  uint32_t stride() const { return stride_; }
  template <typename F>
  void visit(F&& fn) const
  {
    a_.visit(fn);
    b_.visit(fn);
  }
  template <typename F>
  void visitMut(F&& fn)
  {
    a_.visitMut(fn);
    b_.visitMut(fn);
  }

  SYN_HEAP_INSTANCE_FIELDS

 private:
  Bundle a_;
  Bundle b_;
  uint32_t stride_;
};

class XShr : public Instance
{
 public:
  XShr() { setEntryType(EntryType::kXShr); }
  XShr(Bundle a, Bundle b, uint32_t stride)
      : a_(std::move(a)), b_(std::move(b)), stride_(stride)
  {
    setEntryType(EntryType::kXShr);
  }
  const Bundle& a() const { return a_; }
  const Bundle& b() const { return b_; }
  uint32_t stride() const { return stride_; }
  template <typename F>
  void visit(F&& fn) const
  {
    a_.visit(fn);
    b_.visit(fn);
  }
  template <typename F>
  void visitMut(F&& fn)
  {
    a_.visitMut(fn);
    b_.visitMut(fn);
  }

  SYN_HEAP_INSTANCE_FIELDS

 private:
  Bundle a_;
  Bundle b_;
  uint32_t stride_;
};

// --- Arithmetic operations ---

class Mul : public Instance
{
 public:
  Mul() { setEntryType(EntryType::kMul); }
  Mul(Bundle a, Bundle b) : a_(std::move(a)), b_(std::move(b))
  {
    setEntryType(EntryType::kMul);
  }
  const Bundle& a() const { return a_; }
  const Bundle& b() const { return b_; }
  template <typename F>
  void visit(F&& fn) const
  {
    a_.visit(fn);
    b_.visit(fn);
  }
  template <typename F>
  void visitMut(F&& fn)
  {
    a_.visitMut(fn);
    b_.visitMut(fn);
  }

  SYN_HEAP_INSTANCE_FIELDS

 private:
  Bundle a_;
  Bundle b_;
};

class UDiv : public Instance
{
 public:
  UDiv() { setEntryType(EntryType::kUDiv); }
  UDiv(Bundle a, Bundle b) : a_(std::move(a)), b_(std::move(b))
  {
    setEntryType(EntryType::kUDiv);
  }
  const Bundle& a() const { return a_; }
  const Bundle& b() const { return b_; }
  template <typename F>
  void visit(F&& fn) const
  {
    a_.visit(fn);
    b_.visit(fn);
  }
  template <typename F>
  void visitMut(F&& fn)
  {
    a_.visitMut(fn);
    b_.visitMut(fn);
  }

  SYN_HEAP_INSTANCE_FIELDS

 private:
  Bundle a_;
  Bundle b_;
};

class UMod : public Instance
{
 public:
  UMod() { setEntryType(EntryType::kUMod); }
  UMod(Bundle a, Bundle b) : a_(std::move(a)), b_(std::move(b))
  {
    setEntryType(EntryType::kUMod);
  }
  const Bundle& a() const { return a_; }
  const Bundle& b() const { return b_; }
  template <typename F>
  void visit(F&& fn) const
  {
    a_.visit(fn);
    b_.visit(fn);
  }
  template <typename F>
  void visitMut(F&& fn)
  {
    a_.visitMut(fn);
    b_.visitMut(fn);
  }

  SYN_HEAP_INSTANCE_FIELDS

 private:
  Bundle a_;
  Bundle b_;
};

class SDivTrunc : public Instance
{
 public:
  SDivTrunc() { setEntryType(EntryType::kSDivTrunc); }
  SDivTrunc(Bundle a, Bundle b) : a_(std::move(a)), b_(std::move(b))
  {
    setEntryType(EntryType::kSDivTrunc);
  }
  const Bundle& a() const { return a_; }
  const Bundle& b() const { return b_; }
  template <typename F>
  void visit(F&& fn) const
  {
    a_.visit(fn);
    b_.visit(fn);
  }
  template <typename F>
  void visitMut(F&& fn)
  {
    a_.visitMut(fn);
    b_.visitMut(fn);
  }

  SYN_HEAP_INSTANCE_FIELDS

 private:
  Bundle a_;
  Bundle b_;
};

class SDivFloor : public Instance
{
 public:
  SDivFloor() { setEntryType(EntryType::kSDivFloor); }
  SDivFloor(Bundle a, Bundle b) : a_(std::move(a)), b_(std::move(b))
  {
    setEntryType(EntryType::kSDivFloor);
  }
  const Bundle& a() const { return a_; }
  const Bundle& b() const { return b_; }
  template <typename F>
  void visit(F&& fn) const
  {
    a_.visit(fn);
    b_.visit(fn);
  }
  template <typename F>
  void visitMut(F&& fn)
  {
    a_.visitMut(fn);
    b_.visitMut(fn);
  }

  SYN_HEAP_INSTANCE_FIELDS

 private:
  Bundle a_;
  Bundle b_;
};

class SModTrunc : public Instance
{
 public:
  SModTrunc() { setEntryType(EntryType::kSModTrunc); }
  SModTrunc(Bundle a, Bundle b) : a_(std::move(a)), b_(std::move(b))
  {
    setEntryType(EntryType::kSModTrunc);
  }
  const Bundle& a() const { return a_; }
  const Bundle& b() const { return b_; }
  template <typename F>
  void visit(F&& fn) const
  {
    a_.visit(fn);
    b_.visit(fn);
  }
  template <typename F>
  void visitMut(F&& fn)
  {
    a_.visitMut(fn);
    b_.visitMut(fn);
  }

  SYN_HEAP_INSTANCE_FIELDS

 private:
  Bundle a_;
  Bundle b_;
};

class SModFloor : public Instance
{
 public:
  SModFloor() { setEntryType(EntryType::kSModFloor); }
  SModFloor(Bundle a, Bundle b) : a_(std::move(a)), b_(std::move(b))
  {
    setEntryType(EntryType::kSModFloor);
  }
  const Bundle& a() const { return a_; }
  const Bundle& b() const { return b_; }
  template <typename F>
  void visit(F&& fn) const
  {
    a_.visit(fn);
    b_.visit(fn);
  }
  template <typename F>
  void visitMut(F&& fn)
  {
    a_.visitMut(fn);
    b_.visitMut(fn);
  }

  SYN_HEAP_INSTANCE_FIELDS

 private:
  Bundle a_;
  Bundle b_;
};

// --- Dff ---

class Dff : public Instance
{
 public:
  Dff() { setEntryType(EntryType::kDff); }
  Dff(Bundle data,
      ControlNet clock,
      ControlNet clear,
      ControlNet reset,
      ControlNet enable,
      Const init_value,
      Const reset_value,
      Const clear_value)
      : data_(std::move(data)),
        clock_(clock),
        clear_(clear),
        reset_(reset),
        enable_(enable),
        init_value_(std::move(init_value)),
        reset_value_(std::move(reset_value)),
        clear_value_(std::move(clear_value))
  {
    setEntryType(EntryType::kDff);
  }

  const Bundle& data() const { return data_; }
  ControlNet clock() const { return clock_; }
  ControlNet clear() const { return clear_; }
  ControlNet reset() const { return reset_; }
  ControlNet enable() const { return enable_; }
  const Const& initValue() const { return init_value_; }
  const Const& resetValue() const { return reset_value_; }
  const Const& clearValue() const { return clear_value_; }

  void setClock(ControlNet cn) { clock_ = cn; }
  void setClear(ControlNet cn) { clear_ = cn; }
  void setReset(ControlNet cn) { reset_ = cn; }
  void setEnable(ControlNet cn) { enable_ = cn; }

  template <typename F>
  void visit(F&& fn) const
  {
    data_.visit(fn);
    fn(clock_.net_);
    fn(clear_.net_);
    fn(reset_.net_);
    fn(enable_.net_);
  }
  template <typename F>
  void visitMut(F&& fn)
  {
    data_.visitMut(fn);
    fn(clock_.net_);
    fn(clear_.net_);
    fn(reset_.net_);
    fn(enable_.net_);
  }

  SYN_HEAP_INSTANCE_FIELDS

 private:
  Bundle data_;
  ControlNet clock_;
  ControlNet clear_;
  ControlNet reset_;
  ControlNet enable_ = ControlNet::one();
  Const init_value_;
  Const reset_value_;
  Const clear_value_;
};

// --- LoopBreaker ---

class LoopBreaker : public Instance
{
 public:
  friend class Instance;
  LoopBreaker() { setEntryType(EntryType::kLoopBreaker); }
  explicit LoopBreaker(Bundle a) : a_(std::move(a))
  {
    setEntryType(EntryType::kLoopBreaker);
  }

  const Bundle& a() const { return a_; }
  template <typename F>
  void visit(F&& fn) const
  {
    a_.visit(fn);
  }
  template <typename F>
  void visitMut(F&& fn)
  {
    a_.visitMut(fn);
  }

  SYN_HEAP_INSTANCE_FIELDS

 private:
  Bundle a_;
};

// ============================================================
// Non-operation instances
// ============================================================

class Input : public Instance
{
 public:
  Input() { setEntryType(EntryType::kInput); }
  Input(std::string name, uint32_t width)
      : name_(std::move(name)), width_(width)
  {
    setEntryType(EntryType::kInput);
  }

  const std::string& name() const { return name_; }
  uint32_t outputWidth() const { return width_; }
  template <typename F>
  void visit(F&&) const
  {
  }
  template <typename F>
  void visitMut(F&&)
  {
  }

  SYN_HEAP_INSTANCE_FIELDS

 private:
  std::string name_;
  uint32_t width_;
};

class Dangling : public Instance
{
 public:
  Dangling() { setEntryType(EntryType::kDangling); }
  explicit Dangling(uint32_t width) : width_(width)
  {
    setEntryType(EntryType::kDangling);
  }

  uint32_t outputWidth() const { return width_; }
  template <typename F>
  void visit(F&&) const
  {
  }
  template <typename F>
  void visitMut(F&&)
  {
  }

 private:
  uint32_t width_ = 0;
};

class Output : public Instance
{
 public:
  Output() { setEntryType(EntryType::kOutput); }
  Output(std::string name, Bundle value)
      : name_(std::move(name)), value_(std::move(value))
  {
    setEntryType(EntryType::kOutput);
  }

  const std::string& name() const { return name_; }
  const Bundle& value() const { return value_; }
  template <typename F>
  void visit(F&& fn) const
  {
    value_.visit(fn);
  }
  template <typename F>
  void visitMut(F&& fn)
  {
    value_.visitMut(fn);
  }

  SYN_HEAP_INSTANCE_FIELDS

 private:
  std::string name_;
  Bundle value_;
};

class Name : public Instance
{
 public:
  Name() { setEntryType(EntryType::kName); }
  Name(std::string name, Bundle value)
      : name_(std::move(name)),
        value_(std::move(value)),
        from_(0),
        to_(value_.width()),
        tentative_(false),
        is_vector_(value_.width() > 1)
  {
    setEntryType(EntryType::kName);
  }

  Name(std::string name,
       Bundle value,
       uint32_t from,
       uint32_t to,
       bool tentative,
       bool is_vector)
      : name_(std::move(name)),
        value_(std::move(value)),
        from_(from),
        to_(to),
        tentative_(tentative),
        is_vector_(is_vector)
  {
    setEntryType(EntryType::kName);
  }

  const std::string& nameStr() const { return name_; }
  const Bundle& value() const { return value_; }
  uint32_t from() const { return from_; }
  uint32_t to() const { return to_; }
  bool tentative() const { return tentative_; }
  void setTentative(bool t) { tentative_ = t; }
  bool isVector() const { return is_vector_; }

  template <typename F>
  void visit(F&& fn) const
  {
    value_.visit(fn);
  }
  template <typename F>
  void visitMut(F&& fn)
  {
    value_.visitMut(fn);
  }

  SYN_HEAP_INSTANCE_FIELDS

 private:
  std::string name_;
  Bundle value_;
  uint32_t from_;
  uint32_t to_;
  bool tentative_;
  bool is_vector_;
};

class Target : public Instance
{
 public:
  Target() { setEntryType(EntryType::kTarget); }
  Target(sta::LibertyCell* cell, Bundle inputs)
      : cell_(cell), inputs_(std::move(inputs))
  {
    setEntryType(EntryType::kTarget);
  }

  sta::LibertyCell* cell() const { return cell_; }
  const Bundle& inputs() const { return inputs_; }

  template <typename F>
  void visit(F&& fn) const
  {
    inputs_.visit(fn);
  }
  template <typename F>
  void visitMut(F&& fn)
  {
    inputs_.visitMut(fn);
  }

  SYN_HEAP_INSTANCE_FIELDS

 private:
  sta::LibertyCell* cell_ = nullptr;
  Bundle inputs_;
};

class Other : public Instance
{
 public:
  struct Port
  {
    std::string name;
    enum Direction
    {
      kInput,
      kOutput,
      kInOut
    } direction;
    uint32_t width;  // port width
    Bundle value;    // for inputs: the driving signal
                     // for outputs: empty (output nets come from the graph)
  };

  Other() { setEntryType(EntryType::kOther); }
  Other(std::string cell_type, std::vector<Port> ports)
      : cell_type_(std::move(cell_type)), ports_(std::move(ports))
  {
    setEntryType(EntryType::kOther);
    for (auto& port : ports_) {
      if (port.direction == Port::kOutput) {
        output_width_ += port.width;
      }
    }
  }

  const std::string& cellType() const { return cell_type_; }
  const std::vector<Port>& ports() const { return ports_; }
  uint32_t outputWidth() const { return output_width_; }

  template <typename F>
  void visit(F&& fn) const
  {
    for (auto& port : ports_) {
      if (port.direction == Port::kInput || port.direction == Port::kInOut) {
        port.value.visit(fn);
      }
    }
  }
  template <typename F>
  void visitMut(F&& fn)
  {
    for (auto& port : ports_) {
      if (port.direction == Port::kInput || port.direction == Port::kInOut) {
        port.value.visitMut(fn);
      }
    }
  }

  SYN_HEAP_INSTANCE_FIELDS

 private:
  std::string cell_type_;
  std::vector<Port> ports_;
  uint32_t output_width_ = 0;
};

// All instance entry types, partitioned by storage class.
// Inline types fit in a NetTableEntry slot; heap types are heap-allocated and
// referenced by a PlaceholderEntry. Each macro takes M(Type); the corresponding
// EntryType enumerator is k##Type by convention.
// REQUIRES: every Type listed has a corresponding EntryType::k##Type value.
// These are intentionally not #undef'd so Instance.cc can reuse them.
#define SYN_ALL_INLINE_CASES(M) \
  M(TieHigh)                    \
  M(TieLow)                     \
  M(TieX)                       \
  M(BufferFine)                 \
  M(NotFine)                    \
  M(AndFine)                    \
  M(OrFine)                     \
  M(AndnotFine)                 \
  M(XorFine)                    \
  M(Dangling)

#define SYN_ALL_HEAP_CASES(M) \
  M(BufferWide)               \
  M(NotWide)                  \
  M(AndWide)                  \
  M(OrWide)                   \
  M(AndnotWide)               \
  M(XorWide)                  \
  M(MuxFine)                  \
  M(MuxWide)                  \
  M(AdcFine)                  \
  M(AdcWide)                  \
  M(Eq)                       \
  M(ULt)                      \
  M(SLt)                      \
  M(Shl)                      \
  M(UShr)                     \
  M(SShr)                     \
  M(XShr)                     \
  M(Mul)                      \
  M(UDiv)                     \
  M(UMod)                     \
  M(SDivTrunc)                \
  M(SDivFloor)                \
  M(SModTrunc)                \
  M(SModFloor)                \
  M(Dff)                      \
  M(LoopBreaker)              \
  M(Input)                    \
  M(Output)                   \
  M(Name)                     \
  M(Target)                   \
  M(Other)

#define SYN_ALL_INSTANCE_CASES(M) \
  SYN_ALL_INLINE_CASES(M)         \
  SYN_ALL_HEAP_CASES(M)

#define SYN_DISPATCH_VISIT(Type)               \
  case EntryType::k##Type:                     \
    static_cast<const Type*>(this)->visit(fn); \
    break;

template <typename F>
void Instance::visit(F&& fn) const
{
  switch (entryType()) {
    SYN_ALL_INSTANCE_CASES(SYN_DISPATCH_VISIT)
    default:
      break;
  }
}

#undef SYN_DISPATCH_VISIT

#define SYN_DISPATCH_VISIT_MUT(Type)        \
  case EntryType::k##Type:                  \
    static_cast<Type*>(this)->visitMut(fn); \
    break;

template <typename F>
void Instance::visitMut(F&& fn)
{
  switch (entryType()) {
    SYN_ALL_INSTANCE_CASES(SYN_DISPATCH_VISIT_MUT)
    default:
      break;
  }
}

#undef SYN_DISPATCH_VISIT_MUT

#define SYN_DISPATCH_IS(Type) \
  case EntryType::k##Type:    \
    return std::is_base_of_v<T, Type>;

template <typename T>
bool Instance::is() const
{
  switch (entryType()) {
    SYN_ALL_INSTANCE_CASES(SYN_DISPATCH_IS)
    default:
      return false;
  }
}

#undef SYN_DISPATCH_IS

template <typename F>
void Instance::visitSlice(uint32_t i, F&& fn) const
{
  switch (entryType()) {
    case EntryType::kBufferFine:
    case EntryType::kBufferWide:
      fn(static_cast<const Buffer*>(this)->a()[i]);
      return;
    case EntryType::kNotFine:
    case EntryType::kNotWide:
      fn(static_cast<const Not*>(this)->a()[i]);
      return;
    case EntryType::kAndFine:
    case EntryType::kAndWide: {
      auto* op = static_cast<const And*>(this);
      fn(op->a()[i]);
      fn(op->b()[i]);
      return;
    }
    case EntryType::kOrFine:
    case EntryType::kOrWide: {
      auto* op = static_cast<const Or*>(this);
      fn(op->a()[i]);
      fn(op->b()[i]);
      return;
    }
    case EntryType::kAndnotFine:
    case EntryType::kAndnotWide: {
      auto* op = static_cast<const Andnot*>(this);
      fn(op->a()[i]);
      fn(op->b()[i]);
      return;
    }
    case EntryType::kXorFine:
    case EntryType::kXorWide: {
      auto* op = static_cast<const Xor*>(this);
      fn(op->a()[i]);
      fn(op->b()[i]);
      return;
    }
    case EntryType::kMuxFine:
    case EntryType::kMuxWide: {
      auto* op = static_cast<const Mux*>(this);
      fn(op->sel());
      fn(op->a()[i]);
      fn(op->b()[i]);
      return;
    }
    case EntryType::kLoopBreaker:
      fn(static_cast<const LoopBreaker*>(this)->a()[i]);
      return;
    default:
      break;
  }
  // Non-bitwise: all inputs are connected to every output bit.
  visit(fn);
}

template <typename F>
void Instance::visitSliceMut(uint32_t i, F&& fn)
{
  switch (entryType()) {
    case EntryType::kBufferFine: {
      auto& a = static_cast<BufferFine*>(this)->a_;
      fn(a);
      return;
    }
    case EntryType::kBufferWide: {
      auto& a = static_cast<BufferWide*>(this)->a_.mutableNet(i);
      fn(a);
      return;
    }
    case EntryType::kNotFine: {
      auto& a = static_cast<NotFine*>(this)->a_;
      fn(a);
      return;
    }
    case EntryType::kNotWide: {
      auto& a = static_cast<NotWide*>(this)->a_.mutableNet(i);
      fn(a);
      return;
    }
    case EntryType::kAndFine: {
      auto* f = static_cast<AndFine*>(this);
      fn(f->a_);
      fn(f->b_);
      return;
    }
    case EntryType::kAndWide: {
      auto* w = static_cast<AndWide*>(this);
      fn(w->a_.mutableNet(i));
      fn(w->b_.mutableNet(i));
      return;
    }
    case EntryType::kOrFine: {
      auto* f = static_cast<OrFine*>(this);
      fn(f->a_);
      fn(f->b_);
      return;
    }
    case EntryType::kOrWide: {
      auto* w = static_cast<OrWide*>(this);
      fn(w->a_.mutableNet(i));
      fn(w->b_.mutableNet(i));
      return;
    }
    case EntryType::kAndnotFine: {
      auto* f = static_cast<AndnotFine*>(this);
      fn(f->a_);
      fn(f->b_);
      return;
    }
    case EntryType::kAndnotWide: {
      auto* w = static_cast<AndnotWide*>(this);
      fn(w->a_.mutableNet(i));
      fn(w->b_.mutableNet(i));
      return;
    }
    case EntryType::kXorFine: {
      auto* f = static_cast<XorFine*>(this);
      fn(f->a_);
      fn(f->b_);
      return;
    }
    case EntryType::kXorWide: {
      auto* w = static_cast<XorWide*>(this);
      fn(w->a_.mutableNet(i));
      fn(w->b_.mutableNet(i));
      return;
    }
    case EntryType::kMuxFine: {
      auto* f = static_cast<MuxFine*>(this);
      fn(f->sel_);
      fn(f->a_);
      fn(f->b_);
      return;
    }
    case EntryType::kMuxWide: {
      auto* w = static_cast<MuxWide*>(this);
      fn(w->sel_);
      fn(w->a_.mutableNet(i));
      fn(w->b_.mutableNet(i));
      return;
    }
    case EntryType::kLoopBreaker: {
      auto& a = static_cast<LoopBreaker*>(this)->a_.mutableNet(i);
      fn(a);
      return;
    }
    default:
      break;
  }
  visitMut(fn);
}

}  // namespace syn
