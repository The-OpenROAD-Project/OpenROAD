// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include "frBaseTypes.h"

namespace drt {
class frEndStyle
{
 public:
  // constructor
  frEndStyle() = default;
  frEndStyle(const frEndStyle& styleIn) = default;
  frEndStyle(frEndStyleEnum styleIn) : style_(styleIn) {}
  // setters
  void set(frEndStyleEnum styleIn) { style_ = styleIn; }
  void set(const frEndStyle& styleIn) { style_ = styleIn.style_; }
  // getters
  operator frEndStyleEnum() const { return style_; }

 private:
  frEndStyleEnum style_{frcExtendEndStyle};

  template <class Archive>
  void serialize(Archive& ar, const unsigned int version)
  {
    (ar) & style_;
  }

  friend class boost::serialization::access;
};

class frSegStyle
{
 public:
  // constructor
  frSegStyle() = default;
  frSegStyle(const frSegStyle& in) = default;
  // setters
  void setWidth(frUInt4 widthIn) { width_ = widthIn; }
  void setBeginStyle(const frEndStyle& style, frUInt4 ext = 0)
  {
    beginStyle_.set(style);
    beginExt_ = ext;
  }
  void setEndStyle(const frEndStyle& style, frUInt4 ext = 0)
  {
    endStyle_.set(style);
    endExt_ = ext;
  }
  void setBeginExt(const frUInt4& in) { beginExt_ = in; }
  void setEndExt(const frUInt4& in) { endExt_ = in; }
  // getters
  frUInt4 getWidth() const { return width_; }
  frUInt4 getBeginExt() const { return beginExt_; }
  frEndStyle getBeginStyle() const { return beginStyle_; }
  frUInt4 getEndExt() const { return endExt_; }
  frEndStyle getEndStyle() const { return endStyle_; }

 private:
  frUInt4 beginExt_{0};
  frUInt4 endExt_{0};
  frUInt4 width_{0};
  frEndStyle beginStyle_;
  frEndStyle endStyle_;

  template <class Archive>
  void serialize(Archive& ar, const unsigned int version)
  {
    (ar) & beginExt_;
    (ar) & endExt_;
    (ar) & width_;
    (ar) & beginStyle_;
    (ar) & endStyle_;
  }

  friend class boost::serialization::access;
};

}  // namespace drt
