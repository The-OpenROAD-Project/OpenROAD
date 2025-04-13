// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023-2025, The OpenROAD Authors

#include "distributed/paUpdate.h"

#include <fstream>
#include <memory>
#include <string>
#include <utility>
#include <vector>

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
