/* Authors: Lutong Wang and Bangqi Xu */
/*
 * Copyright (c) 2019, The Regents of the University of California
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the University nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE REGENTS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _DR_MARKER_H_
#define _DR_MARKER_H_

#include "db/drObj/drBlockObject.h"

namespace fr {
class frConstraint;
class drNet;
class drMazeMarker : public drBlockObject
{
 public:
  // constructors
  drMazeMarker() : constraint_(nullptr), trigNets_(), cnt_(0) {}
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
  frConstraint* constraint_;
  std::map<drNet*, int> trigNets_;
  int cnt_;

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
}  // namespace fr
#endif
