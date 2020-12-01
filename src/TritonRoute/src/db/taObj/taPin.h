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

#ifndef _TA_PIN_H_
#define _TA_PIN_H_

#include "db/taObj/taBlockObject.h"
#include "db/taObj/taShape.h"
#include "db/taObj/taVia.h"

namespace fr{
  class frGuide;
  class taPin: public taBlockObject {
  public:
    // constructors
    taPin(): taBlockObject(), guide(nullptr), pinFigs(), wlen_helper(0), pin(false), wlen_helper2(0), cost(0), numAssigned(0) {}
    // getters
    int getWlenHelper() const {
      return wlen_helper;
    }
    bool hasWlenHelper2() const {
      return pin;
    }
    int getWlenHelper2() const {
      return wlen_helper2;
    }
    frGuide* getGuide() const {
      return guide;
    }
    const std::vector<std::unique_ptr<taPinFig> >& getFigs() const {
      return pinFigs;
    }
    frCost getCost() const {
      return cost;
    }
    int getNumAssigned() const {
      return numAssigned;
    }
    void setWlenHelper(int in) {
      wlen_helper = in;
    }
    void setWlenHelper2(frCoord in) {
      pin = true;
      wlen_helper2 = in;
    }
    void setGuide(frGuide* in) {
      guide = in;
    }
    void addPinFig(std::unique_ptr<taPinFig> in) {
      in->addToPin(this);
      pinFigs.push_back(std::move(in));
    }
    void setCost(frCost in) {
      cost = in;
    }
    void addCost(frCost in) {
      cost += in;
    }
    void addNumAssigned() {
      numAssigned++;
    }
    // others
    frBlockObjectEnum typeId() const override {
      return tacPin;
    }
    bool operator<(const taPin &b) const {
      if (this->cost != b.cost) {
        return this->getCost() > b.getCost();
      } else {
        return this->getId() < b.getId();
      }
    }
  protected:
    frGuide*                                guide;
    std::vector<std::unique_ptr<taPinFig> > pinFigs;
    int                                     wlen_helper; // for nbr global guides
    bool                                    pin;
    frCoord                                 wlen_helper2; // for local guides and pin guides
    frCost                                  cost;
    int                                     numAssigned;
  };
  struct taPinComp {
    bool operator()(const taPin* lhs, const taPin* rhs) const {
      return *lhs < *rhs;
    }
  };
}
#endif
