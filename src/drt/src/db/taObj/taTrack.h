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

#ifndef _TA_TRACK_H_
#define _TA_TRACK_H_

#include <boost/icl/interval_map.hpp>

#include "db/taObj/taBlockObject.h"

namespace fr {
class frTrackPattern;
class taPinFig;
class taTrack : public taBlockObject
{
 public:
  // constructors
  taTrack()
      : trackLoc_(0),
        planar_(),
        via1_(),
        via2_(),
        costPlanar_(),
        costVia1_(),
        costVia2_()
  {
  }
  // getters
  frCoord getTrackLoc() const { return trackLoc_; }
  frUInt4 getCost(frCoord x1, frCoord x2, int type, taPinFig* fig) const;
  // setters
  void setTrackLoc(frCoord loc) { trackLoc_ = loc; }
  // type 0/1/2: planar/down/up
  void addToIntv(taPinFig* obj, frCoord x1, frCoord x2, int type)
  {
    std::set<taPinFig*> tmpS{obj};
    switch (type) {
      case 0:
        planar_ += std::make_pair(boost::icl::interval<frCoord>::closed(x1, x2),
                                  tmpS);
        break;
      case 1:
        via1_ += std::make_pair(boost::icl::interval<frCoord>::closed(x1, x2),
                                tmpS);
        break;
      case 2:
        via2_ += std::make_pair(boost::icl::interval<frCoord>::closed(x1, x2),
                                tmpS);
        break;
      default:;
    }
  }
  void removeFromIntv(taPinFig* obj, frCoord x1, frCoord x2, int type)
  {
    std::set<taPinFig*> tmpS{obj};
    switch (type) {
      case 0:
        planar_ -= std::make_pair(boost::icl::interval<frCoord>::closed(x1, x2),
                                  tmpS);
        break;
      case 1:
        via1_ -= std::make_pair(boost::icl::interval<frCoord>::closed(x1, x2),
                                tmpS);
        break;
      case 2:
        via2_ -= std::make_pair(boost::icl::interval<frCoord>::closed(x1, x2),
                                tmpS);
        break;
      default:;
    }
  }
  // type 0/1/2: planar/down/up
  void addToCost(frBlockObject* obj, frCoord x1, frCoord x2, int type)
  {
    std::set<frBlockObject*> tmpS{obj};
    switch (type) {
      case 0:
        costPlanar_ += std::make_pair(
            boost::icl::interval<frCoord>::closed(x1, x2), tmpS);
        break;
      case 1:
        costVia1_ += std::make_pair(
            boost::icl::interval<frCoord>::closed(x1, x2), tmpS);
        break;
      case 2:
        costVia2_ += std::make_pair(
            boost::icl::interval<frCoord>::closed(x1, x2), tmpS);
        break;
      default:;
    }
  }
  void removeFromCost(frBlockObject* obj, frCoord x1, frCoord x2, int type)
  {
    std::set<frBlockObject*> tmpS{obj};
    switch (type) {
      case 0:
        costPlanar_ -= std::make_pair(
            boost::icl::interval<frCoord>::closed(x1, x2), tmpS);
        break;
      case 1:
        costVia1_ -= std::make_pair(
            boost::icl::interval<frCoord>::closed(x1, x2), tmpS);
        break;
      case 2:
        costVia2_ -= std::make_pair(
            boost::icl::interval<frCoord>::closed(x1, x2), tmpS);
        break;
      default:;
    }
  }
  // others
  frBlockObjectEnum typeId() const override { return tacTrack; }
  bool operator<(const taTrack& b) const { return trackLoc_ < b.trackLoc_; }

 protected:
  frCoord trackLoc_;
  // intervals, ps bp-->ep, viaLoc
  boost::icl::interval_map<frCoord, std::set<taPinFig*>> planar_;
  boost::icl::interval_map<frCoord, std::set<taPinFig*>> via1_;  // down via
  boost::icl::interval_map<frCoord, std::set<taPinFig*>> via2_;  // up via
  // block others bp, ep, viaLoc
  boost::icl::interval_map<frCoord, std::set<frBlockObject*>>
      costPlanar_;  // all cost to others
  boost::icl::interval_map<frCoord, std::set<frBlockObject*>>
      costVia1_;  // all cost to others
  boost::icl::interval_map<frCoord, std::set<frBlockObject*>>
      costVia2_;  // all cost to others
};
}  // namespace fr
#endif
