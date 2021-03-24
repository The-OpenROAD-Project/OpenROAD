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

#ifndef _GR_PIN_H_
#define _GR_PIN_H_

#include "db/grObj/grAccessPattern.h"
#include "db/grObj/grBlockObject.h"

namespace fr {
class grNet;
class grPin : public grBlockObject
{
 public:
  // constructors
  grPin() : grBlockObject(), term(nullptr), accessPattern(nullptr), net(nullptr)
  {
  }
  // setters
  void setFrTerm(frBlockObject* in) { term = in; }
  void setAccessPattern(grAccessPattern* in) { accessPattern = in; }
  void setNet(grNet* in) { net = in; }

  // getters
  bool hasFrTerm() const { return (term); }
  frBlockObject* getFrTerm() const { return term; }
  grAccessPattern* getAccessPattern() const { return accessPattern; }
  grNet* getNet() const { return net; }

  // others
  frBlockObjectEnum typeId() const override { return grcPin; }

 protected:
  frBlockObject* term;  // either frTerm or frInstTerm
  grAccessPattern* accessPattern;
  grNet* net;
};
}  // namespace fr

#endif