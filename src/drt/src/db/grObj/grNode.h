// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include <iostream>
#include <memory>

#include "db/grObj/grBlockObject.h"
#include "db/infra/frPoint.h"
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
        net(in.net),
        loc(in.loc),
        layerNum(in.layerNum),
        connFig(in.connFig),
        pin(in.pin),
        type(in.type),
        parent(in.parent),
        children(in.children)
  {
  }
  grNode(frNode& in)
      : loc(in.loc), layerNum(in.layerNum), pin(in.pin), type(in.type)
  {
  }

  // setters
  void addToNet(grNet* in) { net = in; }
  void setLoc(const Point& in) { loc = in; }
  void setLayerNum(frLayerNum in) { layerNum = in; }
  void setConnFig(grBlockObject* in) { connFig = in; }
  void setPin(frBlockObject* in) { pin = in; }
  void setType(frNodeTypeEnum in) { type = in; }
  void setParent(grNode* in) { parent = in; }
  void addChild(grNode* in)
  {
    bool exist = false;
    for (auto child : children) {
      if (child == in) {
        exist = true;
      }
    }
    if (!exist) {
      children.push_back(in);
    } else {
      std::cout << "Warning: grNode child already exists\n";
    }
  }
  void clearChildren() { children.clear(); }
  void removeChild(grNode* in)
  {
    for (auto it = children.begin(); it != children.end(); it++) {
      if (*it == in) {
        children.erase(it);
        break;
      }
    }
  }
  void setIter(frListIter<std::unique_ptr<grNode>>& in) { iter = in; }
  void reset()
  {
    parent = nullptr;
    children.clear();
  }

  // getters
  bool hasNet() { return (net != nullptr); }
  grNet* getNet() { return net; }
  Point getLoc() { return loc; }
  frLayerNum getLayerNum() { return layerNum; }
  grBlockObject* getConnFig() { return connFig; }
  frBlockObject* getPin() { return pin; }
  frNodeTypeEnum getType() { return type; }
  bool hasParent() { return (parent != nullptr); }
  grNode* getParent() { return parent; }
  bool hasChildren() { return (!children.empty()); }
  std::list<grNode*>& getChildren() { return children; }
  const std::list<grNode*>& getChildren() const { return children; }
  frListIter<std::unique_ptr<grNode>> getIter() { return iter; }

  frBlockObjectEnum typeId() const override { return grcNode; }

 protected:
  grNet* net{nullptr};
  Point loc;
  frLayerNum layerNum{0};
  grBlockObject* connFig{nullptr};  // wire / via / patch to parent
  frBlockObject* pin{
      nullptr};  // term / instTerm / null if boundary pin or steiner
  frNodeTypeEnum type{frNodeTypeEnum::frcSteiner};

  // frNode *fNode; // corresponding frNode
  grNode* parent{nullptr};
  std::list<grNode*> children;

  frListIter<std::unique_ptr<grNode>> iter;

  friend class frNode;
};
}  // namespace drt
