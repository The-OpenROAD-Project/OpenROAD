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

#pragma once

#include "db/taObj/taBlockObject.h"
#include "db/taObj/taShape.h"
#include "db/taObj/taVia.h"

namespace drt {
class frGuide;

class taPin : public taBlockObject
{
 public:
  // constructors
  taPin() = default;

  // getters
  int getNextIrouteDir() const { return nextIrouteDir_; }
  bool hasPinCoord() const { return hasPinCoord_; }
  int getPinCoord() const { return pinCoord_; }
  frGuide* getGuide() const { return guide_; }
  const std::vector<std::unique_ptr<taPinFig>>& getFigs() const
  {
    return pinFigs_;
  }
  frCost getCost() const { return cost_; }
  int getNumAssigned() const { return numAssigned_; }
  void setNextIrouteDir(int in) { nextIrouteDir_ = in; }
  void setPinCoord(frCoord in)
  {
    hasPinCoord_ = true;
    pinCoord_ = in;
  }
  void setGuide(frGuide* in) { guide_ = in; }
  void addPinFig(std::unique_ptr<taPinFig> in)
  {
    in->addToPin(this);
    pinFigs_.push_back(std::move(in));
  }
  void setCost(frCost in) { cost_ = in; }
  void addCost(frCost in) { cost_ += in; }
  void addNumAssigned() { numAssigned_++; }
  // others
  frBlockObjectEnum typeId() const override { return tacPin; }
  bool operator<(const taPin& b) const
  {
    if (this->cost_ != b.cost_) {
      return this->getCost() > b.getCost();
    }
    return this->getId() < b.getId();
  }

 protected:
  frGuide* guide_{nullptr};
  std::vector<std::unique_ptr<taPinFig>> pinFigs_;
  int nextIrouteDir_{0};  // for nbr global guides
  bool hasPinCoord_{false};
  frCoord pinCoord_{0};  // for local guides and pin guides
  frCost cost_{0};
  int numAssigned_{0};
};

struct taPinComp
{
  bool operator()(const taPin* lhs, const taPin* rhs) const
  {
    return *lhs < *rhs;
  }
};
}  // namespace drt
