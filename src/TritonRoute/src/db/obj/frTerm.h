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

#ifndef _FR_TERM_H_
#define _FR_TERM_H_

#include <memory>
#include "frBaseTypes.h"
#include "db/obj/frBlockObject.h"
#include "db/obj/frPin.h"

namespace fr {
  class frNet;
  class frInstTerm;

  class frTerm: public frBlockObject {
  public:
    // constructors
    frTerm(const frString &name): frBlockObject(), name(name), net(nullptr), pins(), type(frTermEnum::frcNormalTerm) {}
    frTerm(const frTerm &in): frBlockObject(), name(in.name), net(in.net), type(in.type) {
      for (auto &uPin: in.getPins()) {
        auto pin = uPin.get();
        auto tmp = std::make_unique<frPin>(*pin);
        addPin(std::move(tmp));
      }
    }
    frTerm(const frTerm &in, const frTransform &xform): frBlockObject(), name(in.name), net(in.net), type(in.type) {
      for (auto &uPin: in.getPins()) {
        auto pin = uPin.get();
        auto tmp = std::make_unique<frPin>(*pin, xform);
        addPin(std::move(tmp));
      }
    }
    // getters
    bool hasNet() const {
      return (net);
    }
    frNet* getNet() const {
      return net;
    }
    const frString& getName() const {
      return name;
    }
    const std::vector< std::unique_ptr<frPin> >& getPins() const {
      return pins;
    }
    // setters
    void addToNet(frNet* in) {
      net = in;
    }
    void addPin(std::unique_ptr<frPin> in) {
      in->setTerm(this);
      pins.push_back(std::move(in));
    }
    void setType(frTermEnum in) {
      type = in;
    }
    frTermEnum getType() const {
      return type;
    }
    // others
    frBlockObjectEnum typeId() const override {
      return frcTerm;
    }
  protected:
    frString name; // A, B, Z, VSS, VDD
    frNet* net; // set later, term in instTerm does not have net
    std::vector<std::unique_ptr<frPin> > pins; // set later
    frTermEnum type;
  };
}

#endif
