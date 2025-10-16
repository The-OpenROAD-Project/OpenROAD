// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include <iostream>
#include <list>
#include <memory>

#include "db/grObj/grBlockObject.h"
#include "db/infra/frPoint.h"
#include "db/obj/frBlockObject.h"
#include "db/obj/frNode.h"
#include "frBaseTypes.h"

namespace drt {
class frNet;
class grNet;

class grNode : public grBlockObject
{
 public:
  // constructor
  grNode() = default;
  grNode(grNode& in)
      : grBlockObject(),
        net_(in.net_),
        loc_(in.loc_),
        layerNum_(in.layerNum_),
        connFig_(in.connFig_),
        pin_(in.pin_),
        type_(in.type_),
        parent_(in.parent_),
        children_(in.children_)
  {
  }
  grNode(frNode& in)
      : loc_(in.loc_), layerNum_(in.layerNum_), pin_(in.pin_), type_(in.type_)
  {
  }

  // setters
  void addToNet(grNet* in) { net_ = in; }
  void setLoc(const odb::Point& in) { loc_ = in; }
  void setLayerNum(frLayerNum in) { layerNum_ = in; }
  void setConnFig(grBlockObject* in) { connFig_ = in; }
  void setPin(frBlockObject* in) { pin_ = in; }
  void setType(frNodeTypeEnum in) { type_ = in; }
  void setParent(grNode* in) { parent_ = in; }
  void addChild(grNode* in)
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
      std::cout << "Warning: grNode child already exists\n";
    }
  }
  void clearChildren() { children_.clear(); }
  void removeChild(grNode* in)
  {
    for (auto it = children_.begin(); it != children_.end(); it++) {
      if (*it == in) {
        children_.erase(it);
        break;
      }
    }
  }
  void setIter(frListIter<std::unique_ptr<grNode>>& in) { iter_ = in; }
  void reset()
  {
    parent_ = nullptr;
    children_.clear();
  }

  // getters
  bool hasNet() { return (net_ != nullptr); }
  grNet* getNet() { return net_; }
  odb::Point getLoc() { return loc_; }
  frLayerNum getLayerNum() { return layerNum_; }
  grBlockObject* getConnFig() { return connFig_; }
  frBlockObject* getPin() { return pin_; }
  frNodeTypeEnum getType() { return type_; }
  bool hasParent() { return (parent_ != nullptr); }
  grNode* getParent() { return parent_; }
  bool hasChildren() { return (!children_.empty()); }
  std::list<grNode*>& getChildren() { return children_; }
  const std::list<grNode*>& getChildren() const { return children_; }
  frListIter<std::unique_ptr<grNode>> getIter() { return iter_; }

  frBlockObjectEnum typeId() const override { return grcNode; }

 protected:
  grNet* net_{nullptr};
  odb::Point loc_;
  frLayerNum layerNum_{0};
  grBlockObject* connFig_{nullptr};  // wire / via / patch to parent
  frBlockObject* pin_{
      nullptr};  // term / instTerm / null if boundary pin or steiner
  frNodeTypeEnum type_{frNodeTypeEnum::frcSteiner};

  // frNode *fNode; // corresponding frNode
  grNode* parent_{nullptr};
  std::list<grNode*> children_;

  frListIter<std::unique_ptr<grNode>> iter_;

  friend class frNode;
};
}  // namespace drt
