// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "db/drObj/drAccessPattern.h"
#include "db/drObj/drBlockObject.h"
#include "db/obj/frBTerm.h"
#include "db/obj/frInstTerm.h"
#include "db/obj/frTerm.h"

namespace drt {

class drNet;

class drPin : public drBlockObject
{
 public:
  // setters
  void setFrTerm(frBlockObject* in) { term_ = in; }
  void addAccessPattern(std::unique_ptr<drAccessPattern> in)
  {
    in->setPin(this);
    accessPatterns_.push_back(std::move(in));
  }
  void setNet(drNet* in) { net_ = in; }
  // getters
  bool hasFrTerm() const { return term_; }
  frBlockObject* getFrTerm() const { return term_; }
  const std::vector<std::unique_ptr<drAccessPattern>>& getAccessPatterns() const
  {
    return accessPatterns_;
  }
  drNet* getNet() const { return net_; }
  bool isInstPin() { return hasFrTerm() && term_->typeId() == frcInstTerm; }
  std::pair<FlexMazeIdx, FlexMazeIdx> getAPBbox();
  // others
  frBlockObjectEnum typeId() const override { return drcPin; }
  std::string getName()
  {
    if (hasFrTerm()) {
      if (term_->typeId() == frcInstTerm) {
        return static_cast<frInstTerm*>(term_)->getName();
      }
      return static_cast<frTerm*>(term_)->getName();
    }
    return "";
  }

 private:
  frBlockObject* term_{nullptr};  // either frTerm or frInstTerm
  std::vector<std::unique_ptr<drAccessPattern>> accessPatterns_;
  drNet* net_{nullptr};

  template <class Archive>
  void serialize(Archive& ar, unsigned int version);
  friend class boost::serialization::access;
};

}  // namespace drt
