// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include <iostream>
#include <list>
#include <memory>

#include "db/infra/frPoint.h"
#include "db/obj/frBlockObject.h"
#include "frBaseTypes.h"
#include "odb/geom.h"

namespace drt {
class frNet;
class grNode;
class frNode : public frBlockObject
{
 public:
  // constructors
  frNode() = default;
  frNode(frNode& in) = delete;
  frNode(grNode& in);
  // setters
  void addToNet(frNet* in) { net_ = in; }
  void setLoc(const odb::Point& in) { loc_ = in; }
  void setLayerNum(frLayerNum in) { layerNum_ = in; }
  void setConnFig(frBlockObject* in) { connFig_ = in; }
  void setPin(frBlockObject* in) { pin_ = in; }
  void setType(frNodeTypeEnum in) { type_ = in; }
  void setParent(frNode* in) { parent_ = in; }
  void addChild(frNode* in)
  {
    bool exist = false;
    for (auto child : children_) {
      if (child == in) {
        exist = true;
      }
    }
    if (!exist) {
      children_.push_back(in);
    } else {
      std::cout << "Warning: child already exists\n";
    }
  }
  // void addChild1(frNode *in) {
  //   bool exist = false;
  //   for (auto child: children) {
  //     if (child == in) {
  //       exist = true;
  //     }
  //   }
  //   if (!exist) {
  //     children.push_back(in);
  //   } else {
  //     std::cout << "Warning1: child already exists\n";
  //   }
  // }
  // void addChild2(frNode *in) {
  //   bool exist = false;
  //   for (auto child: children) {
  //     if (child == in) {
  //       exist = true;
  //     }
  //   }
  //   if (!exist) {
  //     children.push_back(in);
  //   } else {
  //     std::cout << "Warning2: child already exists\n";
  //   }
  // }
  void clearChildren() { children_.clear(); }
  void removeChild(frNode* in)
  {
    for (auto it = children_.begin(); it != children_.end(); it++) {
      if (*it == in) {
        children_.erase(it);
        break;
      }
    }
  }
  void setIter(frListIter<std::unique_ptr<frNode>>& in) { iter_ = in; }
  void reset()
  {
    parent_ = nullptr;
    children_.clear();
  }

  // getters
  bool hasNet() { return net_; }
  frNet* getNet() { return net_; }
  odb::Point getLoc() { return loc_; }
  frLayerNum getLayerNum() { return layerNum_; }
  frBlockObject* getConnFig() { return connFig_; }
  frBlockObject* getPin() { return pin_; }
  frNodeTypeEnum getType() { return type_; }
  bool hasParent() { return parent_; }
  frNode* getParent() { return parent_; }
  std::list<frNode*>& getChildren() { return children_; }
  const std::list<frNode*>& getChildren() const { return children_; }
  frListIter<std::unique_ptr<frNode>> getIter() { return iter_; }

  frBlockObjectEnum typeId() const override { return frcNode; }

 protected:
  frNet* net_{nullptr};
  odb::Point loc_;                   // == prefAP bp if exist for pin
  frLayerNum layerNum_{0};           // == prefAP bp if exist for pin
  frBlockObject* connFig_{nullptr};  // wire / via / patch to parent
  frBlockObject* pin_{
      nullptr};  // term / instTerm / null if boundary pin or steiner
  frNodeTypeEnum type_{frNodeTypeEnum::frcSteiner};

  frNode* parent_{nullptr};
  std::list<frNode*> children_;

  frListIter<std::unique_ptr<frNode>> iter_;

  friend class grNode;
};
}  // namespace drt
