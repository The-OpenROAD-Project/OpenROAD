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

struct frBlockObjectComp
{
  bool operator()(const frBlockObject* lhs, const frBlockObject* rhs) const
  {
    return *lhs < *rhs;
  }
};

using frBlockObjectSet = std::set<frBlockObject*, frBlockObjectComp>;
template <typename T>
using frBlockObjectMap = std::map<frBlockObject*, T, frBlockObjectComp>;
}  // namespace drt
