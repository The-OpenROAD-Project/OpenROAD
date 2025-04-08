// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include "db/grObj/grAccessPattern.h"
#include "db/grObj/grBlockObject.h"

namespace drt {
class grNet;
class grPin : public grBlockObject
{
 public:
  // setters
  void setFrTerm(frBlockObject* in) { term = in; }
  void setAccessPattern(grAccessPattern* in) { accessPattern = in; }
  void setNet(grNet* in) { net = in; }

  // getters
  bool hasFrTerm() const { return (term); }
  frBlockObject* getFrTerm() const { return term; }
  grAccessPattern* getAccessPattern() const { return accessPattern; }
  grNet* getNet() const { return net; }

  // others
  frBlockObjectEnum typeId() const override { return grcPin; }

 protected:
  frBlockObject* term{nullptr};  // either frTerm or frInstTerm
  grAccessPattern* accessPattern{nullptr};
  grNet* net{nullptr};
};
}  // namespace drt
