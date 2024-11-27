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

#include <algorithm>
#include <iostream>
#include <memory>
#include <set>
#include <vector>

#include "db/obj/frShape.h"
#include "frBaseTypes.h"
#include "utl/Logger.h"

namespace drt {
class frLef58CutClass
{
 public:
  // getters
  void getName(std::string& in) const { in = name_; }
  std::string getName() const { return name_; }
  frCoord getViaWidth() const { return viaWidth_; }
  bool hasViaLength() const { return (viaLength_ == viaWidth_) ? false : true; }
  frCoord getViaLength() const { return viaLength_; }
  frUInt4 getNumCut() const { return numCut_; }
  // setters
  void setName(frString& in) { name_ = in; }
  void setViaWidth(frCoord in) { viaWidth_ = in; }
  void setViaLength(frCoord in) { viaLength_ = in; }
  void setNumCut(frUInt4 in) { numCut_ = in; }
  void report(utl::Logger* logger)
  {
    logger->report("CUTCLASS name {} viaWidth {} viaLength {} numCut {}",
                   name_,
                   viaWidth_,
                   viaLength_,
                   numCut_);
  }

 private:
  std::string name_;
  frCoord viaWidth_{0};
  frCoord viaLength_{0};
  frUInt4 numCut_{1};  // this value is not equal to #multi cuts, only used for
                       // calculating resistance, currently ignored in rule
                       // checking process
};

class frViaDef
{
 public:
  // constructors
  frViaDef() = default;
  frViaDef(const std::string& nameIn) : name_(nameIn) {}
  // operators
  bool operator==(const frViaDef& in) const
  {
    if (name_ != in.name_) {
      return false;
    }
    if (cutClassIdx_ != in.cutClassIdx_) {
      return false;
    }
    if (cutClass_ != in.cutClass_) {
      return false;
    }
    if (isDefault_ != in.isDefault_) {
      return false;
    }
    auto equalFigs = [](const std::vector<std::unique_ptr<frShape>>& val1,
                        const std::vector<std::unique_ptr<frShape>>& val2) {
      std::multiset<std::pair<frLayerNum, odb::Rect>> val1set;
      std::multiset<std::pair<frLayerNum, odb::Rect>> val2set;
      std::transform(val1.begin(),
                     val1.end(),
                     std::inserter(val1set, val1set.begin()),
                     [](const std::unique_ptr<frShape>& val) {
                       return std::make_pair(val->getLayerNum(),
                                             val->getBBox());
                     });
      std::transform(val2.begin(),
                     val2.end(),
                     std::inserter(val2set, val2set.begin()),
                     [](const std::unique_ptr<frShape>& val) {
                       return std::make_pair(val->getLayerNum(),
                                             val->getBBox());
                     });
      if (val1set.size() != val2set.size()) {
        return false;
      }
      auto it1 = val1set.begin();
      auto it2 = val2set.begin();
      while (it1 != val1set.end()) {
        if (*it1 != *it2) {
          return false;
        }
        it1++;
        it2++;
      }
      return true;
    };
    if (!equalFigs(layer1Figs_, in.layer1Figs_)) {
      return false;
    }
    if (!equalFigs(layer2Figs_, in.layer2Figs_)) {
      return false;
    }
    if (!equalFigs(cutFigs_, in.cutFigs_)) {
      return false;
    }
    return true;
  }
  bool operator!=(const frViaDef& in) const { return !(*this == in); }
  // getters
  void getName(std::string& nameIn) const { nameIn = name_; }
  std::string getName() const { return name_; }
  frLayerNum getLayer1Num() const
  {
    if (!layer1Figs_.empty()) {
      return (layer1Figs_.at(0))->getLayerNum();
    }
    std::cout << "Error: via does not have shape on layer 1" << std::endl;
    exit(1);
  }
  frLayerNum getLayer2Num() const
  {
    if (!layer2Figs_.empty()) {
      return (layer2Figs_.at(0))->getLayerNum();
    }
    std::cout << "Error: via does not have shape on layer 2" << std::endl;
    exit(1);
  }
  frLayerNum getCutLayerNum() const
  {
    if (!cutFigs_.empty()) {
      return (cutFigs_.at(0))->getLayerNum();
    }
    std::cout << "Error: via does not have shape on layer cut" << std::endl;
    exit(1);
  }
  const std::vector<std::unique_ptr<frShape>>& getLayer1Figs() const
  {
    return layer1Figs_;
  }
  const std::vector<std::unique_ptr<frShape>>& getLayer2Figs() const
  {
    return layer2Figs_;
  }
  const std::vector<std::unique_ptr<frShape>>& getCutFigs() const
  {
    return cutFigs_;
  }
  bool getDefault() const { return isDefault_; }
  int getNumCut() const { return cutFigs_.size(); }
  bool hasCutClass() const { return (cutClass_ != nullptr); }
  frLef58CutClass* getCutClass() const { return cutClass_; }
  int getCutClassIdx() const { return cutClassIdx_; }
  bool isAddedByRouter() const { return addedByRouter_; }
  bool isMultiCut() const { return (cutFigs_.size() > 1); }
  // setters
  void addLayer1Fig(std::unique_ptr<frShape> figIn)
  {
    Rect box = figIn->getBBox();
    layer1ShapeBox_.merge(box);
    layer1Figs_.push_back(std::move(figIn));
  }
  void addLayer2Fig(std::unique_ptr<frShape> figIn)
  {
    Rect box = figIn->getBBox();
    layer2ShapeBox_.merge(box);
    layer2Figs_.push_back(std::move(figIn));
  }
  void addCutFig(std::unique_ptr<frShape> figIn)
  {
    Rect box = figIn->getBBox();
    cutShapeBox_.merge(box);
    cutFigs_.push_back(std::move(figIn));
  }
  void setDefault(bool isDefaultIn) { isDefault_ = isDefaultIn; }
  void setCutClass(frLef58CutClass* in) { cutClass_ = in; }
  void setCutClassIdx(int in) { cutClassIdx_ = in; }
  void setAddedByRouter(bool in) { addedByRouter_ = in; }
  const Rect& getLayer1ShapeBox() const { return layer1ShapeBox_; }
  const Rect& getLayer2ShapeBox() const { return layer2ShapeBox_; }
  const Rect& getCutShapeBox() const { return cutShapeBox_; }
  const Rect& getShapeBox(frLayerNum lNum)
  {
    if (lNum == getLayer1Num()) {
      return layer1ShapeBox_;
    }
    if (lNum == getLayer2Num()) {
      return layer2ShapeBox_;
    }
    throw std::invalid_argument("Error: via does not have shape on layer "
                                + std::to_string(lNum));
  }
  void setId(int in) { id_ = in; }
  int getId() const { return id_; }

 private:
  int id_{0};
  std::string name_;
  bool isDefault_{false};
  std::vector<std::unique_ptr<frShape>> layer1Figs_;
  std::vector<std::unique_ptr<frShape>> layer2Figs_;
  std::vector<std::unique_ptr<frShape>> cutFigs_;
  frLef58CutClass* cutClass_{nullptr};
  int cutClassIdx_{-1};
  bool addedByRouter_{false};

  Rect layer1ShapeBox_;
  Rect layer2ShapeBox_;
  Rect cutShapeBox_;
};
}  // namespace drt
