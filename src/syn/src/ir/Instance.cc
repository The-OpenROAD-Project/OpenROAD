// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include "syn/ir/Instance.h"

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <utility>

#include "sta/Liberty.hh"
#include "sta/PortDirection.hh"
#include "syn/ir/Bundle.h"
#include "syn/ir/Net.h"
#include "syn/ir/NetTable.h"
#include "syn/ir/NetTableEntry.h"

namespace syn {

static_assert(sizeof(PlaceholderEntry) <= NetTableBlock::kSlotSize,
              "PlaceholderEntry must fit in a slot");

// Check all inline types fit in a slot
#define SYN_INLINE_SIZE(Type) \
  static_assert(sizeof(Type) <= NetTableBlock::kSlotSize);

SYN_ALL_INLINE_CASES(SYN_INLINE_SIZE)

#undef SYN_INLINE_SIZE

// Check all heap types don't fit in a slot
#define SYN_HEAP_SIZE(Type) \
  static_assert(sizeof(Type) > NetTableBlock::kSlotSize);

SYN_ALL_HEAP_CASES(SYN_HEAP_SIZE)

#undef SYN_HEAP_SIZE

// ============================================================
// Instance::destroy() — dispatch destructor on entryType()
// ============================================================

#define SYN_DISPATCH_DESTROY(Type)     \
  case EntryType::k##Type:             \
    static_cast<Type*>(this)->~Type(); \
    return;

void Instance::destroy()
{
  switch (entryType()) {
    SYN_ALL_INSTANCE_CASES(SYN_DISPATCH_DESTROY)
    default:
      break;
  }
}

#undef SYN_DISPATCH_DESTROY

uint32_t Instance::outputWidth() const
{
  switch (entryType()) {
    case EntryType::kTieHigh:
    case EntryType::kTieLow:
    case EntryType::kTieX:
    case EntryType::kBufferFine:
    case EntryType::kNotFine:
    case EntryType::kAndFine:
    case EntryType::kOrFine:
    case EntryType::kAndnotFine:
    case EntryType::kXorFine:
    case EntryType::kMuxFine:
    case EntryType::kEq:
    case EntryType::kULt:
    case EntryType::kSLt:
      return 1;

    case EntryType::kAdcFine:
      return 2;

    case EntryType::kBufferWide:
      return static_cast<const BufferWide*>(this)->a().width();
    case EntryType::kNotWide:
      return static_cast<const NotWide*>(this)->a().width();
    case EntryType::kAndWide:
      return static_cast<const AndWide*>(this)->a().width();
    case EntryType::kOrWide:
      return static_cast<const OrWide*>(this)->a().width();
    case EntryType::kAndnotWide:
      return static_cast<const AndnotWide*>(this)->a().width();
    case EntryType::kXorWide:
      return static_cast<const XorWide*>(this)->a().width();
    case EntryType::kMuxWide:
      return static_cast<const MuxWide*>(this)->a().width();
    case EntryType::kAdcWide:
      return static_cast<const AdcWide*>(this)->a().width() + 1;

    case EntryType::kShl:
      return static_cast<const Shl*>(this)->a().width();
    case EntryType::kUShr:
      return static_cast<const UShr*>(this)->a().width();
    case EntryType::kSShr:
      return static_cast<const SShr*>(this)->a().width();
    case EntryType::kXShr:
      return static_cast<const XShr*>(this)->a().width();

    case EntryType::kMul:
      return static_cast<const Mul*>(this)->a().width();
    case EntryType::kUDiv:
      return static_cast<const UDiv*>(this)->a().width();
    case EntryType::kUMod:
      return static_cast<const UMod*>(this)->a().width();
    case EntryType::kSDivTrunc:
      return static_cast<const SDivTrunc*>(this)->a().width();
    case EntryType::kSDivFloor:
      return static_cast<const SDivFloor*>(this)->a().width();
    case EntryType::kSModTrunc:
      return static_cast<const SModTrunc*>(this)->a().width();
    case EntryType::kSModFloor:
      return static_cast<const SModFloor*>(this)->a().width();

    case EntryType::kDff:
      return static_cast<const Dff*>(this)->data().width();

    case EntryType::kLoopBreaker:
      return static_cast<const LoopBreaker*>(this)->a().width();

    case EntryType::kInput:
      return static_cast<const Input*>(this)->outputWidth();

    case EntryType::kDangling:
      return static_cast<const Dangling*>(this)->outputWidth();

    case EntryType::kOther:
      return static_cast<const Other*>(this)->outputWidth();

    case EntryType::kTarget: {
      sta::LibertyCell* cell = static_cast<const Target*>(this)->cell();
      uint32_t width = 0;
      sta::LibertyCellPortIterator port_iter(cell);
      while (port_iter.hasNext()) {
        sta::LibertyPort* port = port_iter.next();
        if (port->direction()->isOutput()) {
          if (port->isBus()) {
            width += std::abs(port->toIndex() - port->fromIndex()) + 1;
          } else {
            width += 1;
          }
        }
      }
      return width;
    }

    case EntryType::kOutput:
    case EntryType::kName:
    default:
      return 0;
  }
}

// ============================================================
// Instance::sizeOf() — dispatch on entryType()
// ============================================================

#define SYN_DISPATCH_SIZEOF(Type) \
  case EntryType::k##Type:        \
    return sizeof(Type);

uint32_t Instance::sizeOf() const
{
  switch (entryType()) {
    SYN_ALL_INSTANCE_CASES(SYN_DISPATCH_SIZEOF)
    default:
      return kSlotSize;
  }
}

#undef SYN_DISPATCH_SIZEOF

// ============================================================
// Instance::heapBytes() — heap-allocated memory owned by members
// ============================================================

void Instance::heapBytes(size_t& name_bytes, size_t& vector_bytes) const
{
  name_bytes = 0;
  vector_bytes = 0;

  switch (entryType()) {
    case EntryType::kTieHigh:
    case EntryType::kTieLow:
    case EntryType::kTieX:
    case EntryType::kBufferFine:
    case EntryType::kNotFine:
    case EntryType::kAndFine:
    case EntryType::kOrFine:
    case EntryType::kAndnotFine:
    case EntryType::kXorFine:
    case EntryType::kMuxFine:
    case EntryType::kAdcFine:
      return;

    case EntryType::kBufferWide:
      vector_bytes = static_cast<const BufferWide*>(this)->a().heapBytes();
      return;
    case EntryType::kNotWide:
      vector_bytes = static_cast<const NotWide*>(this)->a().heapBytes();
      return;

    case EntryType::kAndWide:
      vector_bytes = static_cast<const AndWide*>(this)->a().heapBytes()
                     + static_cast<const AndWide*>(this)->b().heapBytes();
      return;
    case EntryType::kOrWide:
      vector_bytes = static_cast<const OrWide*>(this)->a().heapBytes()
                     + static_cast<const OrWide*>(this)->b().heapBytes();
      return;
    case EntryType::kAndnotWide:
      vector_bytes = static_cast<const AndnotWide*>(this)->a().heapBytes()
                     + static_cast<const AndnotWide*>(this)->b().heapBytes();
      return;
    case EntryType::kXorWide:
      vector_bytes = static_cast<const XorWide*>(this)->a().heapBytes()
                     + static_cast<const XorWide*>(this)->b().heapBytes();
      return;
    case EntryType::kMuxWide: {
      auto* m = static_cast<const MuxWide*>(this);
      vector_bytes = m->a().heapBytes() + m->b().heapBytes();
      return;
    }
    case EntryType::kAdcWide: {
      auto* a = static_cast<const AdcWide*>(this);
      vector_bytes = a->a().heapBytes() + a->b().heapBytes();
      return;
    }

    case EntryType::kEq:
      vector_bytes = static_cast<const Eq*>(this)->a().heapBytes()
                     + static_cast<const Eq*>(this)->b().heapBytes();
      return;
    case EntryType::kULt:
      vector_bytes = static_cast<const ULt*>(this)->a().heapBytes()
                     + static_cast<const ULt*>(this)->b().heapBytes();
      return;
    case EntryType::kSLt:
      vector_bytes = static_cast<const SLt*>(this)->a().heapBytes()
                     + static_cast<const SLt*>(this)->b().heapBytes();
      return;

    case EntryType::kShl:
      vector_bytes = static_cast<const Shl*>(this)->a().heapBytes()
                     + static_cast<const Shl*>(this)->b().heapBytes();
      return;
    case EntryType::kUShr:
      vector_bytes = static_cast<const UShr*>(this)->a().heapBytes()
                     + static_cast<const UShr*>(this)->b().heapBytes();
      return;
    case EntryType::kSShr:
      vector_bytes = static_cast<const SShr*>(this)->a().heapBytes()
                     + static_cast<const SShr*>(this)->b().heapBytes();
      return;
    case EntryType::kXShr:
      vector_bytes = static_cast<const XShr*>(this)->a().heapBytes()
                     + static_cast<const XShr*>(this)->b().heapBytes();
      return;

    case EntryType::kMul:
      vector_bytes = static_cast<const Mul*>(this)->a().heapBytes()
                     + static_cast<const Mul*>(this)->b().heapBytes();
      return;
    case EntryType::kUDiv:
      vector_bytes = static_cast<const UDiv*>(this)->a().heapBytes()
                     + static_cast<const UDiv*>(this)->b().heapBytes();
      return;
    case EntryType::kUMod:
      vector_bytes = static_cast<const UMod*>(this)->a().heapBytes()
                     + static_cast<const UMod*>(this)->b().heapBytes();
      return;
    case EntryType::kSDivTrunc:
      vector_bytes = static_cast<const SDivTrunc*>(this)->a().heapBytes()
                     + static_cast<const SDivTrunc*>(this)->b().heapBytes();
      return;
    case EntryType::kSDivFloor:
      vector_bytes = static_cast<const SDivFloor*>(this)->a().heapBytes()
                     + static_cast<const SDivFloor*>(this)->b().heapBytes();
      return;
    case EntryType::kSModTrunc:
      vector_bytes = static_cast<const SModTrunc*>(this)->a().heapBytes()
                     + static_cast<const SModTrunc*>(this)->b().heapBytes();
      return;
    case EntryType::kSModFloor:
      vector_bytes = static_cast<const SModFloor*>(this)->a().heapBytes()
                     + static_cast<const SModFloor*>(this)->b().heapBytes();
      return;

    case EntryType::kDff: {
      auto* d = static_cast<const Dff*>(this);
      vector_bytes = d->data().heapBytes();
      return;
    }

    case EntryType::kLoopBreaker:
      vector_bytes = static_cast<const LoopBreaker*>(this)->a().heapBytes();
      return;

    case EntryType::kInput:
      name_bytes = static_cast<const Input*>(this)->name().capacity();
      return;

    case EntryType::kDangling:
      return;

    case EntryType::kOutput: {
      auto* o = static_cast<const Output*>(this);
      name_bytes = o->name().capacity();
      vector_bytes = o->value().heapBytes();
      return;
    }
    case EntryType::kName: {
      auto* n = static_cast<const Name*>(this);
      name_bytes = n->nameStr().capacity();
      vector_bytes = n->value().heapBytes();
      return;
    }
    case EntryType::kTarget: {
      auto* t = static_cast<const Target*>(this);
      vector_bytes = t->inputs().heapBytes();
      return;
    }
    case EntryType::kOther: {
      auto* o = static_cast<const Other*>(this);
      name_bytes = o->cellType().capacity();
      vector_bytes = o->ports().capacity() * sizeof(Other::Port);
      for (auto& port : o->ports()) {
        name_bytes += port.name.capacity();
        vector_bytes += port.value.heapBytes();
      }
      return;
    }

    default:
      return;
  }
}

// ============================================================
// Fine/wide operations
// ============================================================

// --- Buffer ---

BundleView Buffer::a() const
{
  if (is<BufferFine>()) {
    return static_cast<const BufferFine*>(this)->a();
  }
  return static_cast<const BufferWide*>(this)->a();
}

void Buffer::setA(uint64_t index, Net net)
{
  if (is<BufferFine>()) {
    assert(index == 0);
    static_cast<BufferFine*>(this)->setA(net);
  } else {
    static_cast<BufferWide*>(this)->setA(index, net);
  }
}

void BufferWide::setA(uint64_t index, Net net)
{
  a_.mutableNet(index) = net;
}

size_t Buffer::plan(const Bundle& a)
{
  if (a.width() == 1) {
    return sizeof(BufferFine);
  }
  return sizeof(BufferWide);
}

void Buffer::construct(void* mem, Bundle a)
{
  if (a.width() == 1) {
    new (mem) BufferFine(a[0]);
  } else {
    new (mem) BufferWide(std::move(a));
  }
}

// --- Not ---

BundleView Not::a() const
{
  if (is<NotFine>()) {
    return static_cast<const NotFine*>(this)->a();
  }
  return static_cast<const NotWide*>(this)->a();
}

size_t Not::plan(const Bundle& a)
{
  if (a.width() == 1) {
    return sizeof(NotFine);
  }
  return sizeof(NotWide);
}

void Not::construct(void* mem, Bundle a)
{
  if (a.width() == 1) {
    new (mem) NotFine(a[0]);
  } else {
    new (mem) NotWide(std::move(a));
  }
}

// --- And ---

BundleView And::a() const
{
  if (is<AndFine>()) {
    return static_cast<const AndFine*>(this)->a();
  }
  return static_cast<const AndWide*>(this)->a();
}

BundleView And::b() const
{
  if (is<AndFine>()) {
    return static_cast<const AndFine*>(this)->b();
  }
  return static_cast<const AndWide*>(this)->b();
}

size_t And::plan(const Bundle& a, const Bundle& b)
{
  if (a.width() == 1 && b.width() == 1) {
    return sizeof(AndFine);
  }
  return sizeof(AndWide);
}

void And::construct(void* mem, Bundle a, Bundle b)
{
  if (a.width() == 1 && b.width() == 1) {
    new (mem) AndFine(a[0], b[0]);
  } else {
    new (mem) AndWide(std::move(a), std::move(b));
  }
}

// --- Or ---

BundleView Or::a() const
{
  if (is<OrFine>()) {
    return static_cast<const OrFine*>(this)->a();
  }
  return static_cast<const OrWide*>(this)->a();
}

BundleView Or::b() const
{
  if (is<OrFine>()) {
    return static_cast<const OrFine*>(this)->b();
  }
  return static_cast<const OrWide*>(this)->b();
}

size_t Or::plan(const Bundle& a, const Bundle& b)
{
  if (a.width() == 1 && b.width() == 1) {
    return sizeof(OrFine);
  }
  return sizeof(OrWide);
}

void Or::construct(void* mem, Bundle a, Bundle b)
{
  if (a.width() == 1 && b.width() == 1) {
    new (mem) OrFine(a[0], b[0]);
  } else {
    new (mem) OrWide(std::move(a), std::move(b));
  }
}

// --- Andnot ---

BundleView Andnot::a() const
{
  if (is<AndnotFine>()) {
    return static_cast<const AndnotFine*>(this)->a();
  }
  return static_cast<const AndnotWide*>(this)->a();
}

BundleView Andnot::b() const
{
  if (is<AndnotFine>()) {
    return static_cast<const AndnotFine*>(this)->b();
  }
  return static_cast<const AndnotWide*>(this)->b();
}

size_t Andnot::plan(const Bundle& a, const Bundle& b)
{
  if (a.width() == 1 && b.width() == 1) {
    return sizeof(AndnotFine);
  }
  return sizeof(AndnotWide);
}

void Andnot::construct(void* mem, Bundle a, Bundle b)
{
  if (a.width() == 1 && b.width() == 1) {
    new (mem) AndnotFine(a[0], b[0]);
  } else {
    new (mem) AndnotWide(std::move(a), std::move(b));
  }
}

// --- Xor ---

BundleView Xor::a() const
{
  if (is<XorFine>()) {
    return static_cast<const XorFine*>(this)->a();
  }
  return static_cast<const XorWide*>(this)->a();
}

BundleView Xor::b() const
{
  if (is<XorFine>()) {
    return static_cast<const XorFine*>(this)->b();
  }
  return static_cast<const XorWide*>(this)->b();
}

size_t Xor::plan(const Bundle& a, const Bundle& b)
{
  if (a.width() == 1 && b.width() == 1) {
    return sizeof(XorFine);
  }
  return sizeof(XorWide);
}

void Xor::construct(void* mem, Bundle a, Bundle b)
{
  if (a.width() == 1 && b.width() == 1) {
    new (mem) XorFine(a[0], b[0]);
  } else {
    new (mem) XorWide(std::move(a), std::move(b));
  }
}

// --- Mux ---

Net Mux::sel() const
{
  if (is<MuxFine>()) {
    return static_cast<const MuxFine*>(this)->sel();
  }
  return static_cast<const MuxWide*>(this)->sel();
}

BundleView Mux::a() const
{
  if (is<MuxFine>()) {
    return static_cast<const MuxFine*>(this)->a();
  }
  return static_cast<const MuxWide*>(this)->a();
}

BundleView Mux::b() const
{
  if (is<MuxFine>()) {
    return static_cast<const MuxFine*>(this)->b();
  }
  return static_cast<const MuxWide*>(this)->b();
}

size_t Mux::plan(Net /*sel*/, const Bundle& a, const Bundle& b)
{
  if (a.width() == 1 && b.width() == 1) {
    return sizeof(MuxFine);
  }
  return sizeof(MuxWide);
}

void Mux::construct(void* mem, Net sel, Bundle a, Bundle b)
{
  if (a.width() == 1 && b.width() == 1) {
    new (mem) MuxFine(sel, a[0], b[0]);
  } else {
    new (mem) MuxWide(sel, std::move(a), std::move(b));
  }
}

// --- Adc ---

BundleView Adc::a() const
{
  if (is<AdcFine>()) {
    return static_cast<const AdcFine*>(this)->a();
  }
  return static_cast<const AdcWide*>(this)->a();
}

BundleView Adc::b() const
{
  if (is<AdcFine>()) {
    return static_cast<const AdcFine*>(this)->b();
  }
  return static_cast<const AdcWide*>(this)->b();
}

Net Adc::cin() const
{
  if (is<AdcFine>()) {
    return static_cast<const AdcFine*>(this)->cin();
  }
  return static_cast<const AdcWide*>(this)->cin();
}

size_t Adc::plan(const Bundle& a, const Bundle& b, Net /*cin*/)
{
  if (a.width() == 1 && b.width() == 1) {
    return sizeof(AdcFine);
  }
  return sizeof(AdcWide);
}

void Adc::construct(void* mem, Bundle a, Bundle b, Net cin)
{
  if (a.width() == 1 && b.width() == 1) {
    new (mem) AdcFine(a[0], b[0], cin);
  } else {
    new (mem) AdcWide(std::move(a), std::move(b), cin);
  }
}

// ============================================================
// Instance::hasEffects() — dispatch on entryType()
// ============================================================

bool Instance::hasEffects() const
{
  switch (entryType()) {
    case EntryType::kOutput:
    case EntryType::kOther:
      return true;
    case EntryType::kTarget:
      return static_cast<const Target*>(this)->cell()->isMacro();
    case EntryType::kName:
      return !static_cast<const Name*>(this)->tentative();
    default:
      return false;
  }
}

bool Instance::isSliceable() const
{
  return is<Buffer>() || is<Not>() || is<And>() || is<Or>() || is<Andnot>()
         || is<Xor>() || is<Mux>() || is<LoopBreaker>() || is<Dangling>();
}

bool Instance::hasState() const
{
  switch (entryType()) {
    case EntryType::kDff:
    case EntryType::kOther:
      return true;
    case EntryType::kTarget:
      return static_cast<const Target*>(this)->cell()->hasSequentials();
    default:
      return false;
  }
}

bool Instance::isMapped() const
{
  switch (entryType()) {
    case EntryType::kTarget:
    case EntryType::kInput:
    case EntryType::kOutput:
    case EntryType::kOther:
    case EntryType::kLoopBreaker:
      return true;
    default:
      return false;
  }
}

bool Instance::isInline() const
{
  return sizeOf() <= kSlotSize;
}

// ============================================================
// Instance::baseIndex() / setBaseIndex() — dispatch to heap types
// ============================================================

#define SYN_HEAP_CASE_GET(Type) \
  case EntryType::k##Type:      \
    return static_cast<const Type*>(this)->baseIndex();
#define SYN_HEAP_CASE_SET(Type)                 \
  case EntryType::k##Type:                      \
    static_cast<Type*>(this)->setBaseIndex(id); \
    break;

NetTableId Instance::baseIndex() const
{
  switch (entryType()) {
    SYN_ALL_HEAP_CASES(SYN_HEAP_CASE_GET)
    default:
      return 0;
  }
}

void Instance::setBaseIndex(NetTableId id)
{
  switch (entryType()) {
    SYN_ALL_HEAP_CASES(SYN_HEAP_CASE_SET)
    default:
      break;
  }
}

#undef SYN_HEAP_CASE_GET
#undef SYN_HEAP_CASE_SET

}  // namespace syn
