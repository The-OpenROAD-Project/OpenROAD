// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include <iostream>
#include <memory>

#include "db/infra/frPoint.h"
#include "db/obj/frBlockObject.h"
#include "frBaseTypes.h"

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
  void addToNet(frNet* in) { net = in; }
  void setLoc(const Point& in) { loc = in; }
  void setLayerNum(frLayerNum in) { layerNum = in; }
  void setConnFig(frBlockObject* in) { connFig = in; }
  void setPin(frBlockObject* in) { pin = in; }
  void setType(frNodeTypeEnum in) { type = in; }
  void setParent(frNode* in) { parent = in; }
  void addChild(frNode* in)
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
  void clearChildren() { children.clear(); }
  void removeChild(frNode* in)
  {
    for (auto it = children.begin(); it != children.end(); it++) {
      if (*it == in) {
        children.erase(it);
        break;
      }
    }
  }
  void setIter(frListIter<std::unique_ptr<frNode>>& in) { iter = in; }
  void reset()
  {
    parent = nullptr;
    children.clear();
  }

  // getters
  bool hasNet() { return (net != nullptr); }
  frNet* getNet() { return net; }
  Point getLoc() { return loc; }
  frLayerNum getLayerNum() { return layerNum; }
  frBlockObject* getConnFig() { return connFig; }
  frBlockObject* getPin() { return pin; }
  frNodeTypeEnum getType() { return type; }
  bool hasParent() { return (parent != nullptr); }
  frNode* getParent() { return parent; }
  std::list<frNode*>& getChildren() { return children; }
  const std::list<frNode*>& getChildren() const { return children; }
  frListIter<std::unique_ptr<frNode>> getIter() { return iter; }

  frBlockObjectEnum typeId() const override { return frcNode; }

 protected:
  frNet* net{nullptr};
  Point loc;                        // == prefAP bp if exist for pin
  frLayerNum layerNum{0};           // == prefAP bp if exist for pin
  frBlockObject* connFig{nullptr};  // wire / via / patch to parent
  frBlockObject* pin{
      nullptr};  // term / instTerm / null if boundary pin or steiner
  frNodeTypeEnum type{frNodeTypeEnum::frcSteiner};

  frNode* parent{nullptr};
  std::list<frNode*> children;

  frListIter<std::unique_ptr<frNode>> iter;

  friend class grNode;
};
}  // namespace drt
