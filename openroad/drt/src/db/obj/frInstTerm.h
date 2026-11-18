// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include <memory>
#include <vector>

#include "db/obj/frBlockObject.h"
#include "db/obj/frInst.h"
#include "db/obj/frMTerm.h"
#include "db/obj/frNet.h"
#include "db/obj/frShape.h"
#include "frBaseTypes.h"
#include "odb/geom.h"

namespace drt {
class frNet;
class frInst;
class frAccessPoint;

class frInstTerm : public frBlockObject
{
 public:
  // constructors
  frInstTerm(frInst* inst, frMTerm* term) : inst_(inst), term_(term) {}
  frInstTerm(const frInstTerm& in) = delete;
  // getters
  bool hasNet() const { return (net_); }
  frNet* getNet() const { return net_; }
  frInst* getInst() const { return inst_; }
  frMTerm* getTerm() const { return term_; }
  void addToNet(frNet* in) { net_ = in; }
  const std::vector<frAccessPoint*>& getAccessPoints() const { return ap_; }
  frAccessPoint* getAccessPoint(int idx) const { return ap_[idx]; }
  frString getName() const;
  // setters
  void setAPSize(int size) { ap_.resize(size, nullptr); }
  void setAccessPoint(int idx, frAccessPoint* in) { ap_[idx] = in; }
  void addAccessPoint(frAccessPoint* in) { ap_.push_back(in); }
  void setAccessPoints(const std::vector<frAccessPoint*>& in) { ap_ = in; }
  // others
  void clearAccessPoints() { ap_.clear(); }
  frBlockObjectEnum typeId() const override { return frcInstTerm; }
  frAccessPoint* getAccessPoint(frCoord x, frCoord y, frLayerNum lNum);
  bool hasAccessPoint(frCoord x, frCoord y, frLayerNum lNum);
  void getShapes(std::vector<frRect>& outShapes) const;
  odb::Rect getBBox() const;
  void setIndexInOwner(int in) { index_in_owner_ = in; }
  int getIndexInOwner() const { return index_in_owner_; }
  bool isStubborn() const { return is_stubborn_; }
  void setStubborn(bool in) { is_stubborn_ = in; }

 private:
  // Place this first so it is adjacent to "int id_" inherited from
  // frBlockObject, saving 8 bytes.
  int index_in_owner_{0};
  frInst* inst_;
  frMTerm* term_;
  frNet* net_{nullptr};
  std::vector<frAccessPoint*> ap_;  // follows pin index
  bool is_stubborn_{false};
};

}  // namespace drt
