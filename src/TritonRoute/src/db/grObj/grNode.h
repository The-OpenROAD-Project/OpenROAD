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
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE REGENTS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _GR_NODE_H_
#define _GR_NODE_H_

#include <memory>
#include <iostream>
#include "frBaseTypes.h"
// #include "db/obj/frNode.h"
#include "db/grObj/grBlockObject.h"

namespace fr {
  class frNet;
  class grNet;
  class frNode;

  class grNode: public grBlockObject {
  public:
    // constructor
    grNode(): grBlockObject(), net(nullptr), loc(), layerNum(0), connFig(nullptr), pin(nullptr),
              type(frNodeTypeEnum::frcSteiner), /*fNode(nullptr),*/ parent(nullptr), children() {}
    grNode(grNode &in): grBlockObject(), net(in.net), loc(in.loc), layerNum(in.layerNum), connFig(in.connFig),
                        pin(in.pin), type(in.type), parent(in.parent), children(in.children) {}
    grNode(frNode &in): grBlockObject(), net(nullptr), loc(in.loc), layerNum(in.layerNum), connFig(nullptr),
                        pin(in.pin), type(in.type), parent(nullptr), children() {}

    // setters
    void addToNet(grNet *in) {
      net = in;
    }
    void setLoc(const frPoint &in) {
      loc = in;
    }
    void setLayerNum(frLayerNum in) {
      layerNum = in;
    }
    void setConnFig(grBlockObject *in) {
      connFig = in;
    }
    void setPin(frBlockObject *in) {
      pin = in;
    }
    void setType(frNodeTypeEnum in) {
      type = in;
    }
    void setParent(grNode *in) {
      parent = in;
    }
    void addChild(grNode *in) {
      bool exist = false;
      for (auto child: children) {
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
    void clearChildren() {
      children.clear();
    }
    void removeChild(grNode* in) {
      for (auto it = children.begin(); it != children.end(); it++) {
        if (*it == in) {
          children.erase(it);
          break;
        }
      }
    }
    void setIter(frListIter<std::unique_ptr<grNode> > &in) {
      iter = in;
    }
    void reset() {
      parent = nullptr;
      children.clear();
    }

    // getters
    bool hasNet() {
      return (net != nullptr);
    }
    grNet* getNet() {
      return net;
    }
    void getLoc(frPoint &in) {
      in = loc;
    }
    frPoint getLoc() {
      return loc;
    }
    frLayerNum getLayerNum() {
      return layerNum;
    }
    grBlockObject* getConnFig() {
      return connFig;
    }
    frBlockObject* getPin() {
      return pin;
    }
    frNodeTypeEnum getType() {
      return type;
    }
    bool hasParent() {
      return (parent != nullptr);
    }
    grNode* getParent() {
      return parent;
    }
    bool hasChildren() {
      return (!children.empty());
    }
    std::list<grNode*>& getChildren() {
      return children;
    }
    const std::list<grNode*>& getChildren() const {
      return children;
    }
    frListIter<std::unique_ptr<grNode> > getIter() {
      return iter;
    }

    frBlockObjectEnum typeId() const override {
      return grcNode;
    }

  protected:
    grNet *net;
    frPoint loc;
    frLayerNum layerNum;
    grBlockObject *connFig; // wire / via / patch to parent
    frBlockObject *pin; // term / instTerm / null if boundary pin or steiner
    frNodeTypeEnum type;

    // frNode *fNode; // corresponding frNode
    grNode *parent;
    std::list<grNode*> children;

    frListIter<std::unique_ptr<grNode> > iter;

    friend class frNode;
  };
}

#endif