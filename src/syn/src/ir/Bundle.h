// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#pragma once

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <iterator>
#include <vector>

#include "syn/ir/Const.h"
#include "syn/ir/Net.h"

namespace syn {

class BundleView;

class Bundle
{
 public:
  Bundle() : tag_(kEmpty), width_(0), net_(Net::zero()) {}

  Bundle(Net net) : tag_(kSingle), width_(1), net_(net) {}

  Bundle(Net base, uint32_t width)
      : tag_(width > 1 ? kConsecutive : kSingle), width_(width), net_(base)
  {
    assert(width >= 1);
  }

  // Construct from MSB-first initializer list: Bundle({msb, ..., lsb}).
  Bundle(std::initializer_list<Net> nets)
      : Bundle(
            fromVec(std::vector<Net>(std::make_reverse_iterator(nets.end()),
                                     std::make_reverse_iterator(nets.begin()))))
  {
  }

  Bundle(BundleView bv);

  Bundle(const Bundle& other);
  Bundle& operator=(const Bundle& other);

  Bundle(Bundle&& other) noexcept;
  Bundle& operator=(Bundle&& other) noexcept;

  ~Bundle();

  uint32_t width() const { return width_; }

  bool empty() const { return width_ == 0; }

  Net asNet() const
  {
    assert(width_ == 1);
    return (*this)[0];
  }

  Net msb() const
  {
    assert(width_ > 0);
    return (*this)[width_ - 1];
  }

  Net lsb() const
  {
    assert(width_ > 0);
    return (*this)[0];
  }

  Net operator[](uint32_t i) const;
  Net& mutableNet(uint32_t i);

  Bundle slice(uint32_t offset, uint32_t width) const;
  Bundle concat(const Bundle& other) const;
  void append(const Bundle& other);
  void append(BundleView other);

  Bundle zeroExtend(uint32_t width) const;
  Bundle signExtend(uint32_t width) const;

  static Bundle zero(uint32_t width);
  static Bundle ones(uint32_t width);
  static Bundle undef(uint32_t width);
  static Bundle sentinel(uint32_t width);
  static Bundle fromVec(std::vector<Net> nets);
  static Bundle fromConst(const Const& c);

  bool isConst() const;
  Const toConst() const;

  // Returns heap-allocated bytes owned by this Bundle (vector storage in
  // kGeneric mode).
  size_t heapBytes() const;

  template <typename F>
  void visit(F&& fn) const
  {
    for (uint32_t i = 0; i < width_; ++i) {
      fn((*this)[i]);
    }
  }

  template <typename F>
  void visitMut(F&& fn)
  {
    switch (tag_) {
      case kEmpty:
        break;
      case kSingle:
        fn(net_);
        break;
      case kConsecutive:
        toGeneric();
        [[fallthrough]];
      case kGeneric:
        for (auto& n : vec()) {
          fn(n);
        }
        break;
    }
  }

 private:
  enum Tag : uint8_t
  {
    kEmpty,
    kSingle,
    kConsecutive,
    kGeneric,
  };

  void toGeneric();
  std::vector<Net>& vec();
  const std::vector<Net>& vec() const;
  void destroyIfGeneric();

  Tag tag_;
  uint32_t width_;
  union
  {
    Net net_;
    alignas(std::vector<Net>) char vec_storage_[sizeof(std::vector<Net>)];
  };
};

class BundleView
{
 public:
  BundleView(Net net) : net_(net), Bundle_(nullptr), offset_(0), width_(1) {}

  BundleView(const Bundle& Bundle)
      : net_(Net::zero()), Bundle_(&Bundle), offset_(0), width_(Bundle.width())
  {
  }

  BundleView(const Bundle& Bundle, uint32_t offset, uint32_t len)
      : net_(Net::zero()), Bundle_(&Bundle), offset_(offset), width_(len)
  {
  }

  // Consecutive nets starting at base.
  BundleView(Net base, uint32_t len)
      : net_(base), Bundle_(nullptr), offset_(0), width_(len)
  {
  }

  uint32_t width() const { return width_; }
  bool empty() const { return width_ == 0; }

  Net msb() const { return (*this)[width() - 1]; }

  Net lsb() const { return (*this)[0]; }

  Net asNet() const
  {
    assert(width_ == 1);
    return (*this)[0];
  }

  Net operator[](uint32_t i) const
  {
    if (Bundle_) {
      return (*Bundle_)[offset_ + i];
    }
    return Net(net_.id_ + i);
  }

  // Iterator for range-based for.
  class Iterator
  {
   public:
    Iterator(const BundleView* bv, uint32_t i) : bv_(bv), i_(i) {}
    Net operator*() const { return (*bv_)[i_]; }
    Iterator& operator++()
    {
      i_++;
      return *this;
    }
    bool operator!=(const Iterator& o) const { return i_ != o.i_; }

   private:
    const BundleView* bv_;
    uint32_t i_;
  };

  Iterator begin() const { return Iterator(this, 0); }
  Iterator end() const { return Iterator(this, width_); }

  BundleView slice(uint32_t offset, uint32_t len) const
  {
    if (Bundle_) {
      return BundleView(*Bundle_, offset_ + offset, len);
    }
    if (len == 0) {
      return BundleView(Net::zero(), nullptr, 0, 0);
    }
    return BundleView(Net(net_.id_ + offset), nullptr, 0, len);
  }

  bool isConst() const;

 private:
  BundleView(Net net, const Bundle* Bundle, uint32_t offset, uint32_t len)
      : net_(net), Bundle_(Bundle), offset_(offset), width_(len)
  {
  }

  Net net_;
  const Bundle* Bundle_;
  uint32_t offset_;
  uint32_t width_;
};

}  // namespace syn
