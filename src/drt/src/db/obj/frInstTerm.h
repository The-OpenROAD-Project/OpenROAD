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

#ifndef _FR_INSTTERM_H_
#define _FR_INSTTERM_H_

#include <memory>

#include "db/obj/frBlockObject.h"
#include "db/obj/frInst.h"
#include "db/obj/frMTerm.h"
#include "db/obj/frNet.h"
#include "frBaseTypes.h"

namespace fr {
class frNet;
class frInst;
class frAccessPoint;

class frInstTerm : public frBlockObject
{
 public:
  // constructors
  frInstTerm(frInst* inst, frMTerm* term)
      : inst_(inst), term_(term), net_(nullptr), ap_(), index_in_owner_(0)
  {
  }
  frInstTerm(const frInstTerm& in)
      : frBlockObject(),
        inst_(in.inst_),
        term_(in.term_),
        net_(in.net_),
        ap_(),
        index_in_owner_(0)
  {
  }
  // getters
  bool hasNet() const { return (net_); }
  frNet* getNet() const { return net_; }
  frInst* getInst() const { return inst_; }
  frMTerm* getTerm() const { return term_; }
  void addToNet(frNet* in) { net_ = in; }
  const std::vector<frAccessPoint*>& getAccessPoints() const { return ap_; }
  frAccessPoint* getAccessPoint(int idx) const { return ap_[idx]; }
  frString getName() const;
  // setters
  void setAPSize(int size) { ap_.resize(size, nullptr); }
  void setAccessPoint(int idx, frAccessPoint* in) { ap_[idx] = in; }
  void addAccessPoint(frAccessPoint* in) { ap_.push_back(in); }
  // others
  frBlockObjectEnum typeId() const override { return frcInstTerm; }
  frAccessPoint* getAccessPoint(frCoord x, frCoord y, frLayerNum lNum);
  bool hasAccessPoint(frCoord x, frCoord y, frLayerNum lNum);
  void getShapes(std::vector<frRect>& outShapes, bool updatedTransform = false);
  Rect getBBox(bool updatedTransform);
  void setIndexInOwner(int in) { index_in_owner_ = in; }
  int getIndexInOwner() const { return index_in_owner_; }

 private:
  frInst* inst_;
  frMTerm* term_;
  frNet* net_;
  std::vector<frAccessPoint*> ap_;  // follows pin index
  int index_in_owner_;
};

}  // namespace fr

#endif
