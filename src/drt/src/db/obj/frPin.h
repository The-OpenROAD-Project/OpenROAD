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

#include <iostream>

#include "db/obj/frAccess.h"
#include "db/obj/frBlockObject.h"
#include "db/obj/frShape.h"
#include "frBaseTypes.h"

namespace drt {
class frTerm;

class frPin : public frBlockObject
{
 public:
  frPin(const frPin& in) = delete;
  frPin& operator=(const frPin&) = delete;

  // getters
  const std::vector<std::unique_ptr<frPinFig>>& getFigs() const
  {
    return pinFigs_;
  }

  int getNumPinAccess() const { return aps_.size(); }
  bool hasPinAccess() const { return !aps_.empty(); }
  frPinAccess* getPinAccess(int idx) const { return aps_[idx].get(); }

  // setters
  void addPinFig(std::unique_ptr<frPinFig> in)
  {
    in->addToPin(this);
    pinFigs_.push_back(std::move(in));
  }
  void addPinAccess(std::unique_ptr<frPinAccess> in)
  {
    in->setId(aps_.size());
    in->setPin(this);
    aps_.push_back(std::move(in));
  }
  void setPinAccess(int idx, std::unique_ptr<frPinAccess> in)
  {
    in->setId(idx);
    in->setPin(this);
    aps_[idx] = std::move(in);
  }
  void clearPinAccess() { aps_.clear(); }

 protected:
  frPin() = default;

  std::vector<std::unique_ptr<frPinFig>> pinFigs_;
  std::vector<std::unique_ptr<frPinAccess>> aps_;
};

}  // namespace drt
