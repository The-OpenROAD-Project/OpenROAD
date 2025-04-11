// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include <map>
#include <set>

#include "frBaseTypes.h"

namespace drt {
class frBlockObject
{
 public:
  virtual ~frBlockObject() = default;

  virtual frBlockObjectEnum typeId() const = 0;

  int getId() const { return id_; }
  void setId(int in) { id_ = in; }

  bool operator<(const frBlockObject& rhs) const { return id_ < rhs.id_; }

 private:
  int id_{-1};

  template <class Archive>
  void serialize(Archive& ar, const unsigned int version)
  {
    (ar) & id_;
  }

  friend class boost::serialization::access;
};

namespace internal {
// Don't use directly, use the sets below
struct frBlockObjectComp
{
  bool operator()(const frBlockObject* lhs, const frBlockObject* rhs) const
  {
    return *lhs < *rhs;
  }
};
}  // namespace internal

// A set that is compatible with keys derived from
// frBlockObject*
template <typename K>
using frOrderedIdSet = std::set<K, internal::frBlockObjectComp>;

// Legacy container for a frBlockObject* values; consider using the
// templated container.
using frBlockObjectSet = frOrderedIdSet<frBlockObject*>;

template <typename K, typename V>
using frOrderedIdMap = std::map<K, V, internal::frBlockObjectComp>;

// Legacy container for a frBlockObject* key; consider using
// using the <K, V> container.
template <typename V>
using frBlockObjectMap = frOrderedIdMap<frBlockObject*, V>;

}  // namespace drt
