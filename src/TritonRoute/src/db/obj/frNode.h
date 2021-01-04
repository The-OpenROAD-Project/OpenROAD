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

#ifndef _FR_NODE_H_
#define _FR_NODE_H_

#include <memory>
#include <iostream>
#include "frBaseTypes.h"
#include "db/obj/frBlockObject.h"
#include "db/infra/frPoint.h"

namespace fr {
  class frNet;
  class grNode;  
  class frNode: public frBlockObject {
  public:
    // constructors
    frNode(): frBlockObject(), net(nullptr), loc(), layerNum(0), connFig(nullptr), pin(nullptr), type(frNodeTypeEnum::frcSteiner),
              parent(nullptr), children() {}
    frNode(frNode &in): frBlockObject(), net(in.net), loc(in.loc), layerNum(in.layerNum), connFig(in.connFig),
                              pin(in.pin), type(in.type) {}
    frNode(grNode &in);
    // setters
    void addToNet(frNet *in) {
      net = in;
    }
    void setLoc(const frPoint &in) {
      loc = in;
    }
    void setLayerNum(frLayerNum in) {
      layerNum = in;
    }
    void setConnFig(frBlockObject *in) {
      connFig = in;
    }
    void setPin(frBlockObject *in) {
      pin = in;
    }
    void setType(frNodeTypeEnum in) {
      type = in;
    }
    void setParent(frNode *in) {
      parent = in;
    }
    void addChild(frNode *in) {
      bool exist = false;
      for (auto child: children) {
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
    void clearChildren() {
      children.clear();
    }
    void removeChild(frNode* in) {
      for (auto it = children.begin(); it != children.end(); it++) {
        if (*it == in) {
          children.erase(it);
          break;
        }
      }
    }
    void setIter(frListIter<std::unique_ptr<frNode> > &in) {
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
    frNet* getNet() {
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
    frBlockObject* getConnFig() {
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
    frNode* getParent() {
      return parent;
    }
    std::list<frNode*>& getChildren() {
      return children;
    }
    const std::list<frNode*>& getChildren() const {
      return children;
    }
    frListIter<std::unique_ptr<frNode> > getIter() {
      return iter;
    }

    frBlockObjectEnum typeId() const override {
      return frcNode;
    }

  protected:
    frNet *net;
    frPoint loc;  // == prefAP bp if exist for pin
    frLayerNum layerNum; // == prefAP bp if exist for pin
    frBlockObject *connFig; // wire / via / patch to parent
    frBlockObject *pin; // term / instTerm / null if boundary pin or steiner
    frNodeTypeEnum type;

    frNode *parent;
    std::list<frNode*> children;

    frListIter<std::unique_ptr<frNode> > iter;

    friend class grNode;
  };
}


#endif