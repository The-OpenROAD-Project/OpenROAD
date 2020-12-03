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
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE REGENTS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _FR_INSTTERM_H_
#define _FR_INSTTERM_H_

#include <memory>
#include "frBaseTypes.h"
#include "db/obj/frBlockObject.h"
#include "db/obj/frTerm.h"

namespace fr {
  class frNet;
  class frInst;
  class frAccessPoint;

  class frInstTerm: public frBlockObject {
  public:
    // constructors
    frInstTerm(frInst* inst, frTerm* term): inst(inst), term(term), net(nullptr), ap() {}
    frInstTerm(const frInstTerm &in): frBlockObject(), inst(in.inst), term(in.term), 
                                      net(in.net), ap() {}
    // getters
    bool hasNet() const {
      return (net);
    }
    frNet* getNet() const {
      return net;
    }
    frInst* getInst() const {
      return inst;
    }
    frTerm* getTerm() const {
      return term;
    }
    void addToNet(frNet* in) {
      net = in;
    }
    const std::vector<frAccessPoint*>& getAccessPoints() const {
      return ap;
    }
    frAccessPoint* getAccessPoint(int idx = 0) const {
      return ap[idx];
    }
    // setters
    void setAPSize(int size) {
      ap.resize(size, nullptr);
    }
    void setAccessPoint(int idx, frAccessPoint *in) {
      ap[idx] = in;
    }
    void addAccessPoint(frAccessPoint* in) {
      ap.push_back(in);
    }
    // others
    frBlockObjectEnum typeId() const override {
      return frcInstTerm;
    }
  protected:
    frInst* inst;
    frTerm* term;
    frNet*  net;
    std::vector<frAccessPoint*> ap; // follows pin index

  };
}

#endif
