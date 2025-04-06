// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include "frBaseTypes.h"

namespace drt {
class FlexMazeIdx
{
 public:
  FlexMazeIdx() = default;
  FlexMazeIdx(frMIdx xIn, frMIdx yIn, frMIdx zIn)
      : xIdx_(xIn), yIdx_(yIn), zIdx_(zIn)
  {
  }
  // getters
  frMIdx x() const { return xIdx_; }
  frMIdx y() const { return yIdx_; }
  frMIdx z() const { return zIdx_; }
  bool empty() const { return (xIdx_ == -1 && yIdx_ == -1 && zIdx_ == -1); }
  // setters
  void set(frMIdx xIn, frMIdx yIn, frMIdx zIn)
  {
    xIdx_ = xIn;
    yIdx_ = yIn;
    zIdx_ = zIn;
  }
  void setX(frMIdx xIn) { xIdx_ = xIn; }
  void setY(frMIdx yIn) { yIdx_ = yIn; }
  void setZ(frMIdx zIn) { zIdx_ = zIn; }
  void set(const FlexMazeIdx& in)
  {
    xIdx_ = in.x();
    yIdx_ = in.y();
    zIdx_ = in.z();
  }
  // others
  bool operator<(const FlexMazeIdx& rhs) const
  {
    if (xIdx_ != rhs.x()) {
      return xIdx_ < rhs.x();
    }
    if (yIdx_ != rhs.y()) {
      return yIdx_ < rhs.y();
    }
    return zIdx_ < rhs.z();
  }
  bool operator==(const FlexMazeIdx& rhs) const
  {
    return (xIdx_ == rhs.xIdx_ && yIdx_ == rhs.yIdx_ && zIdx_ == rhs.zIdx_);
  }

  friend std::ostream& operator<<(std::ostream& os, const FlexMazeIdx& mIdx)
  {
    os << "(" << mIdx.x() << ", " << mIdx.y() << ", " << mIdx.z() << ")";
    return os;
  }

 private:
  frMIdx xIdx_ = -1;
  frMIdx yIdx_ = -1;
  frMIdx zIdx_ = -1;

  template <class Archive>
  void serialize(Archive& ar, const unsigned int version)
  {
    (ar) & xIdx_;
    (ar) & yIdx_;
    (ar) & zIdx_;
  }
  friend class boost::serialization::access;
};
}  // namespace drt
