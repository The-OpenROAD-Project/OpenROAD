// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include "syn/ir/Bundle.h"

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <utility>
#include <vector>

#include "syn/ir/Const.h"
#include "syn/ir/Net.h"

namespace syn {

Bundle::Bundle(BundleView bv) : tag_(kEmpty), width_(0), net_(Net::zero())
{
  if (bv.empty()) {
    return;
  }
  if (bv.width() == 1) {
    tag_ = kSingle;
    width_ = 1;
    net_ = bv[0];
    return;
  }
  std::vector<Net> nets;
  nets.reserve(bv.width());
  for (uint32_t i = 0; i < bv.width(); i++) {
    nets.push_back(bv[i]);
  }
  *this = Bundle::fromVec(std::move(nets));
}

Bundle::Bundle(const Bundle& other) : tag_(other.tag_), width_(other.width_)
{
  if (tag_ == kGeneric) {
    new (vec_storage_) std::vector<Net>(other.vec());
  } else {
    net_ = other.net_;
  }
}

Bundle& Bundle::operator=(const Bundle& other)
{
  if (this == &other) {
    return *this;
  }
  destroyIfGeneric();
  tag_ = other.tag_;
  width_ = other.width_;
  if (tag_ == kGeneric) {
    new (vec_storage_) std::vector<Net>(other.vec());
  } else {
    net_ = other.net_;
  }
  return *this;
}

Bundle::Bundle(Bundle&& other) noexcept : tag_(other.tag_), width_(other.width_)
{
  if (tag_ == kGeneric) {
    new (vec_storage_) std::vector<Net>(std::move(other.vec()));
  } else {
    net_ = other.net_;
  }
  other.tag_ = kEmpty;
  other.width_ = 0;
}

Bundle& Bundle::operator=(Bundle&& other) noexcept
{
  if (this == &other) {
    return *this;
  }
  destroyIfGeneric();
  tag_ = other.tag_;
  width_ = other.width_;
  if (tag_ == kGeneric) {
    new (vec_storage_) std::vector<Net>(std::move(other.vec()));
  } else {
    net_ = other.net_;
  }
  other.tag_ = kEmpty;
  other.width_ = 0;
  return *this;
}

Bundle::~Bundle()
{
  destroyIfGeneric();
}

Net Bundle::operator[](uint32_t i) const
{
  assert(i < width_);
  switch (tag_) {
    case kEmpty:
      assert(false);
      return Net::zero();
    case kSingle:
      return net_;
    case kConsecutive:
      return Net(net_.id_ + i);
    case kGeneric:
      return vec()[i];
  }
  return Net::zero();
}

Net& Bundle::mutableNet(uint32_t i)
{
  assert(i < width_);
  if (tag_ == kSingle) {
    return net_;
  }
  toGeneric();
  return vec()[i];
}

Bundle Bundle::slice(uint32_t offset, uint32_t width) const
{
  assert(offset + width <= width_);
  if (width == 0) {
    return Bundle();
  }
  if (width == 1) {
    return Bundle((*this)[offset]);
  }
  switch (tag_) {
    case kConsecutive:
      return Bundle(Net(net_.id_ + offset), width);
    default:
      break;
  }
  std::vector<Net> result;
  result.reserve(width);
  for (uint32_t i = 0; i < width; ++i) {
    result.push_back((*this)[offset + i]);
  }
  return fromVec(std::move(result));
}

Bundle Bundle::concat(const Bundle& other) const
{
  if (empty()) {
    return other;
  }
  if (other.empty()) {
    return *this;
  }
  if (tag_ == kConsecutive && other.tag_ == kConsecutive
      && net_.id_ + width_ == other.net_.id_) {
    return Bundle(net_, width_ + other.width_);
  }
  std::vector<Net> result;
  result.reserve(width_ + other.width_);
  for (uint32_t i = 0; i < width_; ++i) {
    result.push_back((*this)[i]);
  }
  for (uint32_t i = 0; i < other.width_; ++i) {
    result.push_back(other[i]);
  }
  return fromVec(std::move(result));
}

void Bundle::append(const Bundle& other)
{
  if (other.empty()) {
    return;
  }
  if (empty()) {
    *this = other;
    return;
  }
  if ((tag_ == kConsecutive || tag_ == kSingle)
      && (other.tag_ == kConsecutive || other.tag_ == kSingle)
      && net_.id_ + width_ == other.net_.id_) {
    tag_ = kConsecutive;
    width_ += other.width_;
    return;
  }
  toGeneric();
  auto& v = vec();
  v.reserve(v.size() + other.width_);
  for (uint32_t i = 0; i < other.width_; ++i) {
    v.push_back(other[i]);
  }
  width_ += other.width_;
}

void Bundle::append(BundleView other)
{
  if (other.empty()) {
    return;
  }
  if (empty()) {
    *this = Bundle(other);
    return;
  }
  toGeneric();
  auto& v = vec();
  v.reserve(v.size() + other.width());
  for (uint32_t i = 0; i < other.width(); ++i) {
    v.push_back(other[i]);
  }
  width_ += other.width();
}

Bundle Bundle::zeroExtend(uint32_t width) const
{
  assert(width >= width_);
  if (width == width_) {
    return *this;
  }
  return concat(Bundle::zero(width - width_));
}

Bundle Bundle::signExtend(uint32_t width) const
{
  assert(width >= width_);
  assert(width_ > 0);
  if (width == width_) {
    return *this;
  }
  Net msb = (*this)[width_ - 1];
  std::vector<Net> result;
  result.reserve(width);
  for (uint32_t i = 0; i < width_; ++i) {
    result.push_back((*this)[i]);
  }
  for (uint32_t i = width_; i < width; ++i) {
    result.push_back(msb);
  }
  return fromVec(std::move(result));
}

Bundle Bundle::zero(uint32_t width)
{
  if (width == 0) {
    return Bundle();
  }
  if (width == 1) {
    return Bundle(Net::zero());
  }
  std::vector<Net> result(width, Net::zero());
  return fromVec(std::move(result));
}

Bundle Bundle::ones(uint32_t width)
{
  if (width == 0) {
    return Bundle();
  }
  if (width == 1) {
    return Bundle(Net::one());
  }
  std::vector<Net> result(width, Net::one());
  return fromVec(std::move(result));
}

Bundle Bundle::undef(uint32_t width)
{
  if (width == 0) {
    return Bundle();
  }
  if (width == 1) {
    return Bundle(Net::undef());
  }
  std::vector<Net> result(width, Net::undef());
  return fromVec(std::move(result));
}

Bundle Bundle::sentinel(uint32_t width)
{
  if (width == 0) {
    return Bundle();
  }
  if (width == 1) {
    return Bundle(Net::sentinel());
  }
  std::vector<Net> result(width, Net::sentinel());
  return fromVec(std::move(result));
}

Bundle Bundle::fromVec(std::vector<Net> nets)
{
  if (nets.empty()) {
    return Bundle();
  }
  if (nets.size() == 1) {
    return Bundle(nets[0]);
  }
  bool consecutive = true;
  for (size_t i = 1; i < nets.size(); ++i) {
    if (nets[i].id_ != nets[0].id_ + i) {
      consecutive = false;
      break;
    }
  }
  if (consecutive) {
    return Bundle(nets[0], static_cast<uint32_t>(nets.size()));
  }
  Bundle v;
  v.tag_ = kGeneric;
  v.width_ = static_cast<uint32_t>(nets.size());
  new (v.vec_storage_) std::vector<Net>(std::move(nets));
  return v;
}

void Bundle::toGeneric()
{
  if (tag_ == kGeneric) {
    return;
  }
  std::vector<Net> nets;
  nets.reserve(width_);
  for (uint32_t i = 0; i < width_; ++i) {
    nets.push_back((*this)[i]);
  }
  tag_ = kGeneric;
  new (vec_storage_) std::vector<Net>(std::move(nets));
}

std::vector<Net>& Bundle::vec()
{
  assert(tag_ == kGeneric);
  return *reinterpret_cast<std::vector<Net>*>(vec_storage_);
}

const std::vector<Net>& Bundle::vec() const
{
  assert(tag_ == kGeneric);
  return *reinterpret_cast<const std::vector<Net>*>(vec_storage_);
}

void Bundle::destroyIfGeneric()
{
  if (tag_ == kGeneric) {
    vec().~vector();
    tag_ = kEmpty;
  }
}

bool Bundle::isConst() const
{
  for (uint32_t i = 0; i < width_; i++) {
    if (!(*this)[i].isConst()) {
      return false;
    }
  }
  return true;
}

Const Bundle::toConst() const
{
  std::vector<Trit> bits;
  bits.reserve(width_);
  for (uint32_t i = 0; i < width_; i++) {
    bits.push_back((*this)[i].constTrit());
  }
  return Const::from(std::move(bits));
}

size_t Bundle::heapBytes() const
{
  if (tag_ == kGeneric) {
    return vec().capacity() * sizeof(Net);
  }
  return 0;
}

Bundle Bundle::fromConst(const Const& c)
{
  std::vector<Net> nets;
  nets.reserve(c.width());
  for (uint32_t i = 0; i < c.width(); i++) {
    switch (c[i]) {
      case Trit::Zero:
        nets.push_back(Net::zero());
        break;
      case Trit::One:
        nets.push_back(Net::one());
        break;
      case Trit::Undef:
        nets.push_back(Net::undef());
        break;
    }
  }
  return fromVec(std::move(nets));
}

Bundle Net::repeated(uint32_t times) const
{
  return Bundle::fromVec(std::vector<Net>(times, *this));
}

bool BundleView::isConst() const
{
  for (auto net : *this) {
    if (!net.isConst()) {
      return false;
    }
  }
  return true;
}

}  // namespace syn
