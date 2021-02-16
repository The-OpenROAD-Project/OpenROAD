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

#ifndef _FR_VIADEF_H_
#define _FR_VIADEF_H_

#include <vector>
#include <memory>
#include "frBaseTypes.h"
#include "db/obj/frShape.h"

namespace fr{
  class frLef58CutClass {
  public:
    // constructors
    frLef58CutClass(): name(""), viaWidth(0), viaLength(0), numCut(1) {}
    // getters
    void getName(std::string &in) const {
      in = name;
    }
    std::string getName() const {
      return name;
    }
    frCoord getViaWidth() const {
      return viaWidth;
    }
    bool hasViaLength() const {
      return (viaLength == viaWidth) ? false : true;
    }
    frCoord getViaLength() const {
      return viaLength;
    }
    frUInt4 getNumCut() const {
      return numCut;
    }
    // setters
    void setName(frString &in) {
      name = in;
    }
    void setViaWidth(frCoord in) {
      viaWidth = in;
    }
    void setViaLength(frCoord in) {
      viaLength = in;
    }
    void setNumCut(frUInt4 in) {
      numCut = in;
    }
  protected:
    std::string name;
    frCoord  viaWidth;
    frCoord  viaLength;
    frUInt4  numCut; // this value is not equal to #multi cuts, only used for calculating resistance, currently ignored in rule checking process
  };


  class frViaDef {
  public:
    // constructors
    frViaDef(): name(), isDefault(false), layer1Figs(), layer2Figs(), cutFigs(), cutClass(nullptr), cutClassIdx(-1), addedByRouter(false),
                layer1ShapeBox(), layer2ShapeBox(), cutShapeBox(){}
    frViaDef(const std::string &nameIn): name(nameIn), isDefault(false), layer1Figs(),
                                     layer2Figs(), cutFigs(), cutClass(nullptr), cutClassIdx(-1), addedByRouter(false),
                                     layer1ShapeBox(), layer2ShapeBox(), cutShapeBox(){}
    // getters
    void getName(std::string &nameIn) const {
      nameIn = name;
    }
    std::string getName() const {
      return name;
    }
    frLayerNum getLayer1Num() const {
      if (layer1Figs.size()) {
        return (layer1Figs.at(0))->getLayerNum();
      } else {
        std::cout <<"Error: via does not have shape on layer 1" <<std::endl;
        exit(1);
      }
    }
    frLayerNum getLayer2Num() const {
      if (layer2Figs.size()) {
        return (layer2Figs.at(0))->getLayerNum();
      } else {
        std::cout <<"Error: via does not have shape on layer 2" <<std::endl;
        exit(1);
      }
    }
    frLayerNum getCutLayerNum() const {
      if (cutFigs.size()) {
        return (cutFigs.at(0))->getLayerNum();
      } else {
        std::cout <<"Error: via does not have shape on layer cut" <<std::endl;
        exit(1);
      }
    }
    const std::vector<std::unique_ptr<frShape> >& getLayer1Figs() const {
      return layer1Figs;
    }
    const std::vector<std::unique_ptr<frShape> >& getLayer2Figs() const {
      return layer2Figs;
    }
    const std::vector<std::unique_ptr<frShape> >& getCutFigs() const {
      return cutFigs;
    }
    bool getDefault() const {
      return isDefault;
    }
    int getNumCut() const {
      return cutFigs.size();
    }
    bool hasCutClass() const {
      return (cutClass != nullptr);
    }
    frLef58CutClass* getCutClass() const {
      return cutClass;
    }
    int getCutClassIdx() const {
      return cutClassIdx;
    }
    bool isAddedByRouter() const {
      return addedByRouter;
    }
    bool isMultiCut() const {
      return (cutFigs.size() > 1) ? true : false;
    }
    // setters
    void addLayer1Fig(std::unique_ptr<frShape> figIn) {
      layer1Figs.push_back(std::move(figIn));
    }
    void addLayer2Fig(std::unique_ptr<frShape> figIn) {
      layer2Figs.push_back(std::move(figIn));
    }
    void addCutFig(std::unique_ptr<frShape> figIn) {
      cutFigs.push_back(std::move(figIn));
    }
    void setDefault(bool isDefaultIn) {
      isDefault = isDefaultIn;
    }
    void setCutClass(frLef58CutClass *in) {
      cutClass = in;
    }
    void setCutClassIdx(int in) {
      cutClassIdx = in;
    }
    void setAddedByRouter(bool in) {
      addedByRouter = in;
    }
    void calculatedShapeBoxes(){
        frBox box;
        frCoord xl = 0, yl = 0, xh = 0, yh = 0;
        for (auto &fig: layer1Figs) {
            fig->getBBox(box);
            xl = std::min(xl, box.left());
            yl = std::min(yl, box.bottom());
            xh = std::max(xh, box.right());
            yh = std::max(yh, box.top());
        }
        layer1ShapeBox.set(xl, yl, xh, yh);
//        std::cout << "\nl1: " << layer1ShapeBox.left() << " " << layer1ShapeBox.right() << " " <<
//                layer1ShapeBox.bottom() << " " << layer1ShapeBox.top()<<  "\n";
        xl = 0; yl = 0; xh = 0; yh = 0;
        for (auto &fig: layer2Figs) {
            fig->getBBox(box);
            xl = std::min(xl, box.left());
            yl = std::min(yl, box.bottom());
            xh = std::max(xh, box.right());
            yh = std::max(yh, box.top());
        }
        layer2ShapeBox.set(xl, yl, xh, yh);
//        std::cout << "\nl2: " << layer2ShapeBox.left() << " " << layer2ShapeBox.right() << " " <<
//                layer2ShapeBox.bottom() << " " << layer2ShapeBox.top()<<  "\n";
        xl = 0; yl = 0; xh = 0; yh = 0;
        for (auto &fig: cutFigs) {
            fig->getBBox(box);
            xl = std::min(xl, box.left());
            yl = std::min(yl, box.bottom());
            xh = std::max(xh, box.right());
            yh = std::max(yh, box.top());
        }
        cutShapeBox.set(xl, yl, xh, yh);
//        std::cout << "\ncut: " << cutShapeBox.left() << " " << cutShapeBox.right() << " " <<
//                cutShapeBox.bottom() << " " << cutShapeBox.top()<<  "\n";
    }
    const frBox& getLayer1ShapeBox(){
        return layer1ShapeBox;
    }
    const frBox& getLayer2ShapeBox(){
        return layer2ShapeBox;
    }
    const frBox& getCutShapeBox(){
        return cutShapeBox;
    }
  protected:
    std::string                              name;
    bool                                     isDefault;
    std::vector< std::unique_ptr<frShape> >  layer1Figs;
    std::vector< std::unique_ptr<frShape> >  layer2Figs;
    std::vector< std::unique_ptr<frShape> >  cutFigs;
    frLef58CutClass*                         cutClass;
    int                                      cutClassIdx;
    bool                                     addedByRouter;
    
    frBox                                    layer1ShapeBox;
    frBox                                    layer2ShapeBox;
    frBox                                    cutShapeBox;
//    frBox                                    layer1SpacingBox;
//    frBox                                    layer2SpacingBox;
//    frBox                                    cutSpacingBox;
  };
}
#endif
