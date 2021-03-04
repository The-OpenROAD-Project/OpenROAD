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
#include "frInst.h"

namespace fr {
  class frNet;
  class frInst;
  class frAccessPoint;

  class frInstTerm: public frBlockObject {
  public:
    // constructors
    frInstTerm(frInst* inst, frTerm* term): inst_(inst), term_(term), net_(nullptr), ap_() {}
    frInstTerm(const frInstTerm &in): frBlockObject(), inst_(in.inst_), term_(in.term_), 
                                      net_(in.net_), ap_(){}
    // getters
    bool hasNet() const {
      return (net_);
    }
    frNet* getNet() const {
      return net_;
    }
    frInst* getInst() const {
      return inst_;
    }
    frTerm* getTerm() const {
      return term_;
    }
    void addToNet(frNet* in) {
      net_ = in;
    }
    
    std::string getFullName();
    
    const std::vector<frAccessPoint*>& getAccessPoints() const {
      return ap_;
    }
    frAccessPoint* getAccessPoint(int idx = 0) const {
      return ap_[idx];
    }
    frString getName() const;
    // setters
    void setAPSize(int size) {
      ap_.resize(size, nullptr);
    }
    void setAccessPoint(int idx, frAccessPoint *in) {
      ap_[idx] = in;
    }
    void addAccessPoint(frAccessPoint* in) {
      ap_.push_back(in);
    }
    // others
    frBlockObjectEnum typeId() const override {
      return frcInstTerm;
    }
  protected:
    frInst* inst_;
    frTerm* term_;
    frNet*  net_;
    std::vector<frAccessPoint*> ap_; // follows pin index
  };
}

#endif
