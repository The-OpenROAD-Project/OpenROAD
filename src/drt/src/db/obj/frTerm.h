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

#ifndef _FR_TERM_H_
#define _FR_TERM_H_

#include <memory>

#include "db/obj/frBlockObject.h"
#include "db/obj/frNet.h"
#include "frBaseTypes.h"

namespace fr {
class frTerm : public frBlockObject
{
 public:
  virtual ~frTerm() {}

  // getters
  virtual bool hasNet() const = 0;
  virtual frNet* getNet() const = 0;
  const frString& getName() const { return name_; }
  // setters
  void setType(dbSigType in) { type_ = in; }
  dbSigType getType() const { return type_; }
  void setDirection(dbIoType in) { direction_ = in; }
  dbIoType getDirection() const { return direction_; }
  // others
  virtual frBlockObjectEnum typeId() const = 0;
  void setIndexInOwner(int value) { index_in_owner_ = value; }
  int getIndexInOwner() { return index_in_owner_; }
  virtual frAccessPoint* getAccessPoint(frCoord x,
                                        frCoord y,
                                        frLayerNum lNum,
                                        int pinAccessIdx) = 0;
  bool hasAccessPoint(frCoord x, frCoord y, frLayerNum lNum, int pinAccessIdx)
  {
    return getAccessPoint(x, y, lNum, pinAccessIdx) != nullptr;
  }
  // fills outShapes with copies of the pinFigs
  virtual void getShapes(std::vector<frRect>& outShapes) = 0;
  const Rect getBBox() const { return bbox_; }

 protected:
  // constructors
  frTerm(const frString& name)
      : frBlockObject(),
        name_(name),
        net_(nullptr),
        type_(dbSigType::SIGNAL),
        direction_(dbIoType::INPUT),
        index_in_owner_(0),
        bbox_()
  {
  }

  frString name_;  // A, B, Z, VSS, VDD
  frNet* net_;  // set later, term in instTerm does not have net
  dbSigType type_;
  dbIoType direction_;
  int index_in_owner_;
  Rect bbox_;

};

}  // namespace fr

#endif
