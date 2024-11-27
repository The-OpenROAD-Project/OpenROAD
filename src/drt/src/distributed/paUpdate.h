/* Authors: Osama */
/*
 * Copyright (c) 2023, The Regents of the University of California
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
#include <boost/serialization/access.hpp>

#include "db/obj/frAccess.h"
namespace drt {
class frInst;
class frInstTerm;
class frDesign;
class paUpdate
{
 public:
  paUpdate() = default;

  void addPinAccess(frPin* pin, std::vector<std::unique_ptr<frPinAccess>> pa)
  {
    pin_access_.emplace_back(pin, std::move(pa));
  }
  void setInstRows(const std::vector<std::vector<frInst*>>& in)
  {
    inst_rows_ = in;
  }
  void setGroupResults(
      const std::vector<std::pair<frInstTerm*, std::vector<frAccessPoint*>>>&
          in)
  {
    group_results_ = in;
  }
  void addGroupResult(
      const std::pair<frInstTerm*, std::vector<frAccessPoint*>>& in)
  {
    group_results_.push_back(in);
  }
  std::vector<std::pair<frPin*, std::vector<std::unique_ptr<frPinAccess>>>>&
  getPinAccess()
  {
    return pin_access_;
  }
  const std::vector<std::vector<frInst*>>& getInstRows() const
  {
    return inst_rows_;
  }
  const std::vector<std::pair<frInstTerm*, std::vector<frAccessPoint*>>>&
  getGroupResults() const
  {
    return group_results_;
  }
  static void serialize(const paUpdate& update, const std::string& path);
  static void deserialize(frDesign* design,
                          paUpdate& update,
                          const std::string& path);

 private:
  std::vector<std::pair<frPin*, std::vector<std::unique_ptr<frPinAccess>>>>
      pin_access_;
  std::vector<std::vector<frInst*>> inst_rows_;
  std::vector<std::pair<frInstTerm*, std::vector<frAccessPoint*>>>
      group_results_;

  template <class Archive>
  void serialize(Archive& ar, unsigned int version);

  friend class boost::serialization::access;
};
}  // namespace drt
