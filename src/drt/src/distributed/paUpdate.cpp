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

#include "distributed/paUpdate.h"

#include <fstream>

#include "distributed/frArchive.h"
#include "serialization.h"

namespace drt {

void paUpdate::serialize(const paUpdate& update, const std::string& path)
{
  std::ofstream file(path.c_str());
  frOArchive ar(file);
  registerTypes(ar);
  ar << update;
  file.close();
}
void paUpdate::deserialize(frDesign* design,
                           paUpdate& update,
                           const std::string& path)
{
  std::ifstream file(path);
  frIArchive ar(file);
  ar.setDesign(design);
  registerTypes(ar);
  ar >> update;
  file.close();
}

template <class Archive>
void paUpdate::serialize(Archive& ar, const unsigned int version)
{
  if (is_loading(ar)) {
    int sz = 0;
    // pin_access_;
    (ar) & sz;
    while (sz--) {
      frBlockObject* obj;
      serializeBlockObject(ar, obj);
      frPin* pin = (frPin*) obj;
      std::vector<std::unique_ptr<frPinAccess>> pa;
      (ar) & pa;
      pin_access_.emplace_back(pin, std::move(pa));
    }

    // inst_rows_
    (ar) & sz;
    while (sz--) {
      inst_rows_.emplace_back();
      int innerSz = 0;
      (ar) & innerSz;
      while (innerSz--) {
        frBlockObject* obj;
        serializeBlockObject(ar, obj);
        inst_rows_.back().push_back((frInst*) obj);
      }
    }
    // group_results_
    (ar) & sz;
    while (sz--) {
      frBlockObject* obj;
      serializeBlockObject(ar, obj);
      frInstTerm* term = (frInstTerm*) obj;
      int innerSz = 0;
      (ar) & innerSz;
      std::vector<frAccessPoint*> aps;
      while (innerSz--) {
        serializeBlockObject(ar, obj);
        aps.push_back((frAccessPoint*) obj);
      }
      group_results_.emplace_back(term, aps);
    }

  } else {
    // pin_access_
    int sz = pin_access_.size();
    (ar) & sz;
    for (auto& [pin, pa] : pin_access_) {
      frBlockObject* obj = pin;
      serializeBlockObject(ar, obj);
      (ar) & pa;
    }
    // inst_rows_
    sz = inst_rows_.size();
    (ar) & sz;
    for (const auto& row : inst_rows_) {
      sz = row.size();
      (ar) & sz;
      for (auto inst : row) {
        frBlockObject* obj = (frBlockObject*) inst;
        serializeBlockObject(ar, obj);
      }
    }
    // group_results_
    sz = group_results_.size();
    (ar) & sz;
    for (const auto& [term, aps] : group_results_) {
      frBlockObject* obj = term;
      serializeBlockObject(ar, obj);
      sz = aps.size();
      (ar) & sz;
      for (auto ap : aps) {
        obj = ap;
        serializeBlockObject(ar, obj);
      }
    }
  }
}

template void paUpdate::serialize<frIArchive>(frIArchive& ar,
                                              const unsigned int file_version);

template void paUpdate::serialize<frOArchive>(frOArchive& ar,
                                              const unsigned int file_version);

}  // namespace drt
