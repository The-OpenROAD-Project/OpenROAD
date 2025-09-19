// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include "db/grObj/grAccessPattern.h"
#include "db/grObj/grBlockObject.h"
#include "db/obj/frBlockObject.h"
#include "frBaseTypes.h"

namespace drt {
class grNet;
class grPin : public grBlockObject
{
 public:
  // setters
  void setFrTerm(frBlockObject* in) { term_ = in; }
  void setAccessPattern(grAccessPattern* in) { accessPattern_ = in; }
  void setNet(grNet* in) { net_ = in; }

  // getters
  bool hasFrTerm() const { return term_; }
  frBlockObject* getFrTerm() const { return term_; }
  grAccessPattern* getAccessPattern() const { return accessPattern_; }
  grNet* getNet() const { return net_; }

  // others
  frBlockObjectEnum typeId() const override { return grcPin; }

 protected:
  frBlockObject* term_{nullptr};  // either frTerm or frInstTerm
  grAccessPattern* accessPattern_{nullptr};
  grNet* net_{nullptr};
};
}  // namespace drt
