// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include <map>

#include "db/drObj/drBlockObject.h"

namespace drt {
class frConstraint;
class drNet;
class drMazeMarker : public drBlockObject
{
 public:
  // constructors
  drMazeMarker() = default;
  // setters
  void setConstraint(frConstraint* in) { constraint_ = in; }
  void addTrigNet(drNet* in)
  {
    auto it = trigNets_.find(in);
    if (it == trigNets_.end()) {
      trigNets_[in] = 1;
    } else {
      ++(trigNets_[in]);
    }
    cnt_++;
  }
  bool subTrigNet(drNet* in)
  {
    auto it = trigNets_.find(in);
    if (it != trigNets_.end()) {
      if (it->second == 1) {
        trigNets_.erase(it);
      } else {
        --(it->second);
      }
      --cnt_;
    }
    return (cnt_) ? true : false;
  }
  // getters
  frConstraint* getConstraint() const { return constraint_; }
  drNet* getTrigNet() const { return trigNets_.cbegin()->first; }
  const std::map<drNet*, int>& getTrigNets() const { return trigNets_; }
  int getCnt() const { return cnt_; }
  // others
  frBlockObjectEnum typeId() const override { return drcMazeMarker; }
  bool operator<(const drMazeMarker& b) const
  {
    return (constraint_ < b.constraint_);
  }

 private:
  frConstraint* constraint_{nullptr};
  std::map<drNet*, int> trigNets_;
  int cnt_{0};

  template <class Archive>
  void serialize(Archive& ar, const unsigned int version)
  {
    (ar) & boost::serialization::base_object<drBlockObject>(*this);
    (ar) & constraint_;
    (ar) & trigNets_;
    (ar) & cnt_;
  }

  friend class boost::serialization::access;
};
}  // namespace drt
