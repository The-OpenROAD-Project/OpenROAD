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

#ifndef _FR_RPIN_H_
#define _FR_RPIN_H_

#include "db/infra/frBox.h"
#include "db/obj/frBlockObject.h"
#include "frBaseTypes.h"
// #include "db/obj/frAccess.h"

namespace fr {
class frNet;
class frAccessPoint;
// serve the same purpose as drPin and grPin, but on fr level
class frRPin : public frBlockObject
{
 public:
  // constructors
  frRPin() : frBlockObject(), term(nullptr), accessPoint(nullptr), net(nullptr)
  {
  }
  // setters
  void setFrTerm(frBlockObject* in) { term = in; }
  void setAccessPoint(frAccessPoint*& in) { accessPoint = std::move(in); }
  void addToNet(frNet* in) { net = in; }
  // getters
  bool hasFrTerm() const { return (term); }
  frBlockObject* getFrTerm() const { return term; }
  frAccessPoint* getAccessPoint() const { return accessPoint; }
  frNet* getNet() const { return net; }

  // utility
  void getBBox(frBox& in);
  frLayerNum getLayerNum();

  // others
  frBlockObjectEnum typeId() const override { return frcRPin; }

 protected:
  frBlockObject* term;         // either frTerm or frInstTerm
  frAccessPoint* accessPoint;  // pref AP for frTerm and frInstTerm
  frNet* net;

 private:
  template <class Archive>
  void serialize(Archive& ar, const unsigned int version)
  {
    (ar) & boost::serialization::base_object<frBlockObject>(*this);
    (ar) & term;
    (ar) & accessPoint;
    (ar) & net;
  }

  friend class boost::serialization::access;
};
}  // namespace fr

#endif
