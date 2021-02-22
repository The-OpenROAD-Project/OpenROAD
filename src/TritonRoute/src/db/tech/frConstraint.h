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

#ifndef _FR_CONSTRAINT_H_
#define _FR_CONSTRAINT_H_

#include "frBaseTypes.h"
#include <map>
#include <iterator>
#include <algorithm>
#include <memory>
#include <utility>
#include "db/tech/frLookupTbl.h"
#include "frViaDef.h"
#include "frViaRuleGenerate.h"

namespace fr {
  enum class frLef58CornerSpacingExceptionEnum {
    NONE,
    EXCEPTSAMENET,
    EXCEPTSAMEMETAL
  };


  // base type for design rule
  class frConstraint {
  public:
    virtual ~frConstraint() {}
    virtual frConstraintTypeEnum typeId() const = 0;    
  protected:
    frConstraint() {}
  };

  class frLef58CutClassConstraint : public frConstraint {
  public:
    friend class io::Parser;
    // constructors;
    frLef58CutClassConstraint() {}
    // getters
    frCollection< std::shared_ptr<frLef58CutClass> > getCutClasses() const {
      frCollection<std::shared_ptr<frLef58CutClass> > sol;
      std::transform(cutClasses.begin(), cutClasses.end(), 
                     std::back_inserter(sol), 
                     [](auto &kv) {return kv.second;});
      return sol;
    }
    // setters
    void addToCutClass(const std::shared_ptr<frLef58CutClass> &in) {
      cutClasses[in->getName()] = in;
    }
    // others
    frConstraintTypeEnum typeId() const override {
      return frConstraintTypeEnum::frcLef58CutClassConstraint;
    }
  protected:
    std::map<frString, std::shared_ptr<frLef58CutClass> > cutClasses;
  };

  // recheck constraint for negative rules
  class frRecheckConstraint : public frConstraint {
  public:
    frConstraintTypeEnum typeId() const override {
      return frConstraintTypeEnum::frcRecheckConstraint;
    }
  };

  // short

  class frShortConstraint : public frConstraint {
  public:
    frConstraintTypeEnum typeId() const override {
      return frConstraintTypeEnum::frcShortConstraint;
    }
  };
  
  // NSMetal
  class frNonSufficientMetalConstraint : public frConstraint {
  public:
    frConstraintTypeEnum typeId() const override {
      return frConstraintTypeEnum::frcNonSufficientMetalConstraint;
    }
  };

  // offGrid
  class frOffGridConstraint : public frConstraint {
  public:
    frConstraintTypeEnum typeId() const override {
      return frConstraintTypeEnum::frcOffGridConstraint;
    }
  };

  // minHole
  class frMinEnclosedAreaConstraint : public frConstraint {
  public:
    // constructor
    frMinEnclosedAreaConstraint(frCoord areaIn) : area(areaIn), width(-1) {}
    // getter
    frCoord getArea() const {
      return area;
    }
    bool hasWidth() const {
      return (width != -1);
    }
    frCoord getWidth() const {
      return width;
    }
    // setter
    void setWidth(frCoord widthIn) {
      width = widthIn;
    }

    frConstraintTypeEnum typeId() const override {
      return frConstraintTypeEnum::frcMinEnclosedAreaConstraint;
    }

  protected:
    frCoord area, width;
  };

  // LEF58_MINSTEP (currently only implement GF14 related API)
  class frLef58MinStepConstraint : public frConstraint {
  public:
    // constructor
    frLef58MinStepConstraint() : minStepLength(-1), insideCorner(false), outsideCorner(false), step(false), maxLength(-1),
                                 maxEdges(-1), minAdjLength(-1), convexCorner(false), exceptWithin(-1),
                                 concaveCorner(false), threeConcaveCorners(false), width(-1), minAdjLength2(-1),
                                 minBetweenLength(-1), exceptSameCorners(false), eolWidth(-1), concaveCorners(false) {}
    // getter
    frCoord getMinStepLength() const {
      return minStepLength;
    }
    bool hasMaxEdges() const {
      return (maxEdges != -1);
    }
    int getMaxEdges() const {
      return maxEdges;
    }
    bool hasMinAdjacentLength() const {
      return (minAdjLength != -1);
    }
    frCoord getMinAdjacentLength() const {
      return minAdjLength;
    }
    bool hasEolWidth() const {
      return (eolWidth != -1);
    }
    frCoord getEolWidth() const {
      return eolWidth;
    }

    // setter
    void setMinStepLength(frCoord in) {
      minStepLength = in;
    }
    void setMaxEdges(int in) {
      maxEdges = in;
    }
    void setMinAdjacentLength(frCoord in) {
      minAdjLength = in;
    }
    void setEolWidth(frCoord in) {
      eolWidth = in;
    }

    frConstraintTypeEnum typeId() const override {
      return frConstraintTypeEnum::frcLef58MinStepConstraint;
    }

  protected:
    frCoord minStepLength;
    bool insideCorner;
    bool outsideCorner;
    bool step;
    frCoord maxLength;
    int maxEdges;
    frCoord minAdjLength;
    bool convexCorner;
    frCoord exceptWithin;
    bool concaveCorner;
    bool threeConcaveCorners;
    frCoord width;
    frCoord minAdjLength2;
    frCoord minBetweenLength;
    bool exceptSameCorners;
    frCoord eolWidth;
    bool concaveCorners;
  };

  // minStep
  class frMinStepConstraint : public frConstraint {
  public:
    // constructor
    frMinStepConstraint() : minStepLength(-1), 
                            minstepType(frMinstepTypeEnum::UNKNOWN),
                            maxLength(-1), 
                            insideCorner(false), 
                            outsideCorner(true), 
                            step(false),
                            maxEdges(-1) {}
    // getter
    frCoord getMinStepLength() const {
      return minStepLength;
    }
    bool hasMaxLength() const {
      return (maxLength != -1);
    }
    frCoord getMaxLength() const {
      return maxLength;
    }
    bool hasMinstepType() const {
      return minstepType != frMinstepTypeEnum::UNKNOWN;
    }
    frMinstepTypeEnum getMinstepType() const {
      return minstepType;
    }
    bool hasInsideCorner() const {
      return insideCorner;
    }
    bool hasOutsideCorner() const {
      return outsideCorner;
    }
    bool hasStep() const {
      return step;
    }
    bool hasMaxEdges() const {
      return (maxEdges != -1);
    }
    int getMaxEdges() const {
      return maxEdges;
    }
    // setter
    void setMinstepType(frMinstepTypeEnum in) {
      minstepType = in;
    }
    void setInsideCorner(bool in) {
      insideCorner = in;
    }
    void setOutsideCorner(bool in) {
      outsideCorner = in;
    }
    void setStep(bool in) {
      step = in;
    }
    void setMinStepLength(frCoord in) {
      minStepLength = in;
    }
    void setMaxLength(frCoord in) {
      maxLength = in;
    }
    void setMaxEdges(int in) {
      maxEdges = in;
    }

    frConstraintTypeEnum typeId() const override {
      return frConstraintTypeEnum::frcMinStepConstraint;
    }

  protected:
    frCoord minStepLength;
    frMinstepTypeEnum minstepType;
    frCoord maxLength;
    bool insideCorner;
    bool outsideCorner;
    bool step;
    int maxEdges;
  };

  // minimumcut
  class frMinimumcutConstraint: public frConstraint {
  public: 
    frMinimumcutConstraint(): numCuts(-1), width(-1), cutDistance(-1), 
                              connection(frMinimumcutConnectionEnum::UNKNOWN),
                              length(-1), distance(-1) {}
    // getters
    int getNumCuts() const {
      return numCuts;
    }
    frCoord getWidth() const {
      return width;
    }
    bool hasWithin() const {
      return !(cutDistance == -1);
    }
    frCoord getCutDistance() const {
      return cutDistance;
    }
    bool hasConnection() const {
      return !(connection == frMinimumcutConnectionEnum::UNKNOWN);
    }
    frMinimumcutConnectionEnum getConnection() const {
      return connection;
    }
    bool hasLength() const {
      return !(length == -1);
    }
    frCoord getLength() const {
      return length;
    }
    frCoord getDistance() const {
      return distance;
    }
    // setters
    void setNumCuts(int in) {
      numCuts = in;
    }
    void setWidth(frCoord in) {
      width = in;
    }
    void setWithin(frCoord in) {
      cutDistance = in;
    }
    void setConnection(frMinimumcutConnectionEnum in) {
      connection = in;
    }
    void setLength(frCoord in1, frCoord in2) {
      length = in1;
      distance = in2;
    }
    // others
    frConstraintTypeEnum typeId() const override {
      return frConstraintTypeEnum::frcMinimumcutConstraint;
    }
  protected:
    int                        numCuts;
    frCoord                    width;
    frCoord                    cutDistance;
    frMinimumcutConnectionEnum connection;
    frCoord                    length;
    frCoord                    distance;
  };

  // minArea

  class frAreaConstraint : public frConstraint {
  public:
    // constructor
    frAreaConstraint(frCoord minAreaIn) {
      minArea = minAreaIn;
    }
    // getter
    frCoord getMinArea() {
      return minArea;
    }
    // setter
    void setMinArea(frCoord minAreaIn) {
      minArea = minAreaIn;
    }

    frConstraintTypeEnum typeId() const override {
      return frConstraintTypeEnum::frcAreaConstraint;
    }
  protected:
    frCoord minArea;
  };

  // minWidth
  class frMinWidthConstraint: public frConstraint {
  public:
    // constructor
    frMinWidthConstraint(frCoord minWidthIn) {
      minWidth = minWidthIn;
    }
    // getter
    frCoord getMinWidth() {
      return minWidth;
    }
    // setter
    void set(frCoord minWidthIn) {
      minWidth = minWidthIn;
    }

    frConstraintTypeEnum typeId() const override {
      return frConstraintTypeEnum::frcMinWidthConstraint;
    }
  protected:
    frCoord minWidth;
  };

  class frLef58SpacingEndOfLineWithinEndToEndConstraint : public frConstraint {
  public:
    // constructors
    frLef58SpacingEndOfLineWithinEndToEndConstraint(): endToEndSpace(0), cutSpace(false), oneCutSpace(0),
                                                       twoCutSpace(0), hExtension(false), extension(0),
                                                       wrongDirExtension(false), hOtherEndWidth(false),
                                                       otherEndWidth(0) {}
    // getters
    frCoord getEndToEndSpace() const {
      return endToEndSpace;
    }
    frCoord getOneCutSpace() const {
      return oneCutSpace;
    }
    frCoord getTwoCutSpace() const {
      return twoCutSpace;
    }
    bool hasExtension() const {
      return hExtension;
    }
    frCoord getExtension() const {
      return extension;
    }
    frCoord getWrongDirExtension() const {
      return wrongDirExtension;
    }
    bool hasOtherEndWidth() const {
      return hOtherEndWidth;
    }
    frCoord getOtherEndWidth() const {
      return otherEndWidth;
    }
    
    // setters
    void setEndToEndSpace(frCoord in) {
      endToEndSpace = in;
    }
    void setCutSpace(frCoord one, frCoord two) {
      oneCutSpace = one;
      twoCutSpace = two;
    }
    void setExtension(frCoord extensionIn) {
      hExtension = true;
      extension = extensionIn;
      wrongDirExtension = extensionIn;
    }
    void setExtension(frCoord extensionIn, frCoord wrongDirExtensionIn) {
      hExtension = true;
      extension = extensionIn;
      wrongDirExtension = wrongDirExtensionIn;
    }
    void setOtherEndWidth(frCoord in) {
      hOtherEndWidth = true;
      otherEndWidth = in;
    }
    // others
    frConstraintTypeEnum typeId() const override {
      return frConstraintTypeEnum::frcLef58SpacingEndOfLineWithinEndToEndConstraint;
    }
  protected:
    frCoord    endToEndSpace;
    bool       cutSpace;
    frCoord    oneCutSpace;
    frCoord    twoCutSpace;
    bool       hExtension;
    frCoord    extension;
    frCoord    wrongDirExtension;
    bool       hOtherEndWidth;
    frCoord    otherEndWidth;
  };

  class frLef58SpacingEndOfLineWithinParallelEdgeConstraint : public frConstraint {
  public:
    // constructors
    frLef58SpacingEndOfLineWithinParallelEdgeConstraint(): subtractEolWidth(false), parSpace(0), parWithin(0),
                                                           hPrl(false), prl(0), hMinLength(false), minLength(0),
                                                           twoEdges(false), sameMetal(false), nonEolCornerOnly(false),
                                                           parallelSameMask(false) {}
    // getters
    bool hasSubtractEolWidth() const {
      return subtractEolWidth;
    }
    frCoord getParSpace() const {
      return parSpace;
    }
    frCoord getParWithin() const {
      return parWithin;
    }
    bool hasPrl() const {
      return hPrl;
    }
    frCoord getPrl() const {
      return prl;
    }
    bool hasMinLength() const {
      return hMinLength;
    }
    frCoord getMinLength() const {
      return minLength;
    }
    bool hasTwoEdges() const {
      return twoEdges;
    }
    bool hasSameMetal() const {
      return sameMetal;
    }
    bool hasNonEolCornerOnly() const {
      return nonEolCornerOnly;
    }
    bool hasParallelSameMask() const {
      return parallelSameMask;
    }
    // setters
    void setSubtractEolWidth(bool in) {
      subtractEolWidth = in;
    }
    void setPar(frCoord parSpaceIn, frCoord parWithinIn) {
      parSpace = parSpaceIn;
      parWithin = parWithinIn;
    }
    void setPrl(frCoord in) {
      hPrl = true;
      prl = in;
    }
    void setMinLength(frCoord in) {
      hMinLength = true;
      minLength = in;
    }
    void setTwoEdges(bool in) {
      twoEdges = in;
    }
    void setSameMetal(bool in) {
      sameMetal = in;
    }
    void setNonEolCornerOnly(bool in) {
      nonEolCornerOnly = in;
    }
    void setParallelSameMask(bool in) {
      parallelSameMask = in;
    }
    
    // others
    frConstraintTypeEnum typeId() const override {
      return frConstraintTypeEnum::frcLef58SpacingEndOfLineWithinParallelEdgeConstraint;
    }
  protected:
    bool    subtractEolWidth;
    frCoord parSpace;
    frCoord parWithin;
    bool    hPrl;
    frCoord prl;
    bool    hMinLength;
    frCoord minLength;
    bool    twoEdges;
    bool    sameMetal;
    bool    nonEolCornerOnly;
    bool    parallelSameMask;
  };

  class frLef58SpacingEndOfLineWithinMaxMinLengthConstraint : public frConstraint {
  public:
    // constructors
    frLef58SpacingEndOfLineWithinMaxMinLengthConstraint(): maxLength(false), length(0), twoSides(false) {}
    // getters
    frCoord getLength() const {
      return length;
    }
    bool isMaxLength() const {
      return maxLength;
    }
    // setters
    void setLength(bool maxLengthIn, frCoord lengthIn, bool twoSidesIn = false) {
      maxLength = maxLengthIn;
      length    = lengthIn;
      twoSides  = twoSidesIn;
    }
    // others
    frConstraintTypeEnum typeId() const override {
      return frConstraintTypeEnum::frcLef58SpacingEndOfLineWithinMaxMinLengthConstraint;
    }
  protected:
    bool    maxLength;
    frCoord length;
    bool    twoSides;
  };


  class frLef58SpacingEndOfLineWithinConstraint : public frConstraint {
  public:
    // constructors
    frLef58SpacingEndOfLineWithinConstraint(): hOppositeWidth(false), oppositeWidth(0), eolWithin(0),
                                               wrongDirWithin(false), sameMask(false), 
                                               endToEndConstraint(nullptr), parallelEdgeConstraint(nullptr) {}
    // getters
    bool hasOppositeWidth() const {
      return hOppositeWidth;
    }
    frCoord getOppositeWidth() const {
      return oppositeWidth;
    }
    frCoord getEolWithin() const {
      return eolWithin;
    }
    frCoord getWrongDirWithin() const {
      return wrongDirWithin;
    }
    bool hasSameMask() const {
      return sameMask;
    }
    bool hasExceptExactWidth() const {
      return false; // skip for now
    }
    bool hasFillConcaveCorner() const {
      return false; // skip for now
    }
    bool hasWithCut() const {
      return false; // skip for now
    }
    bool hasEndPrlSpacing() const {
      return false; // skip for now
    }
    bool hasEndToEndConstraint() const {
      return (endToEndConstraint) ? true : false;
    }
    std::shared_ptr<frLef58SpacingEndOfLineWithinEndToEndConstraint> getEndToEndConstraint() const {
      return endToEndConstraint;
    }
    bool hasMinMaxLength() const {
      return false; // skip for now
    }
    bool hasEqualRectWidth() const {
      return false; // skip for now
    }
    bool hasParallelEdgeConstraint() const {
      return (parallelEdgeConstraint) ? true : false;
    }
    std::shared_ptr<frLef58SpacingEndOfLineWithinParallelEdgeConstraint> getParallelEdgeConstraint() const {
      return parallelEdgeConstraint;
    }
    bool hasMaxMinLengthConstraint() const {
      return (maxMinLengthConstraint) ? true : false;
    }
    std::shared_ptr<frLef58SpacingEndOfLineWithinMaxMinLengthConstraint> getMaxMinLengthConstraint() const {
      return maxMinLengthConstraint;
    }
    bool hasEncloseCut() const {
      return false; // skip for now
    }
    // setters
    void setOppositeWidth(frCoord in) {
      hOppositeWidth = true;
      oppositeWidth = in;
    }
    void setEolWithin(frCoord in) {
      eolWithin = in;
      wrongDirWithin = in;
    }
    void setWrongDirWithin(frCoord in) {
      wrongDirWithin = in;
    }
    void setSameMask(bool in) {
      sameMask = in;
    }
    void setEndToEndConstraint(const std::shared_ptr<frLef58SpacingEndOfLineWithinEndToEndConstraint> &in) {
      endToEndConstraint = in;
    }
    void setParallelEdgeConstraint(const std::shared_ptr<frLef58SpacingEndOfLineWithinParallelEdgeConstraint> &in) {
      parallelEdgeConstraint = in;
    }
    void setMaxMinLengthConstraint(const std::shared_ptr<frLef58SpacingEndOfLineWithinMaxMinLengthConstraint> &in) {
      maxMinLengthConstraint = in;
    }
    // others
    frConstraintTypeEnum typeId() const override {
      return frConstraintTypeEnum::frcLef58SpacingEndOfLineWithinConstraint;
    }
  protected:
    bool                                                                 hOppositeWidth;
    frCoord                                                              oppositeWidth;
    frCoord                                                              eolWithin;
    frCoord                                                              wrongDirWithin;
    bool                                                                 sameMask;
    std::shared_ptr<frLef58SpacingEndOfLineWithinEndToEndConstraint>     endToEndConstraint;
    std::shared_ptr<frLef58SpacingEndOfLineWithinParallelEdgeConstraint> parallelEdgeConstraint;
    std::shared_ptr<frLef58SpacingEndOfLineWithinMaxMinLengthConstraint> maxMinLengthConstraint;
  };

  class frLef58SpacingEndOfLineConstraint : public frConstraint {
  public:
    // constructors
    frLef58SpacingEndOfLineConstraint(): eolSpace(0), eolWidth(0), exactWidth(false),
                                         wrongDirSpacing(false), wrongDirSpace(0),
                                         withinConstraint(nullptr) {}
    // getters
    frCoord getEolSpace() const {
      return eolSpace;
    }
    frCoord getEolWidth() const {
      return eolWidth;
    }
    bool hasExactWidth() const {
      return exactWidth;
    }
    bool hasWrongDirSpacing() const {
      return wrongDirSpacing;
    }
    frCoord getWrongDirSpace() const {
      return wrongDirSpace;
    }
    bool hasWithinConstraint() const {
      return (withinConstraint) ? true : false;
    }
    std::shared_ptr<frLef58SpacingEndOfLineWithinConstraint> getWithinConstraint() const {
      return withinConstraint;
    }
    bool hasToConcaveCornerConstraint() const {
      return false;
    }
    bool hasToNotchLengthConstraint() const {
      return false;
    }
    // setters
    void setEol(frCoord eolSpaceIn, frCoord eolWidthIn, bool exactWidthIn = false) {
      eolSpace = eolSpaceIn;
      eolWidth = eolWidthIn;
      exactWidth = exactWidthIn;
    }
    void setWrongDirSpace(bool in) {
      wrongDirSpacing = true;
      wrongDirSpace = in;
    }
    void setWithinConstraint(const std::shared_ptr<frLef58SpacingEndOfLineWithinConstraint> &in) {
      withinConstraint = in;
    }
    // others
    frConstraintTypeEnum typeId() const override {
      return frConstraintTypeEnum::frcLef58SpacingEndOfLineConstraint;
    }
  protected:
    frCoord                                                  eolSpace;
    frCoord                                                  eolWidth;
    bool                                                     exactWidth;
    bool                                                     wrongDirSpacing;
    frCoord                                                  wrongDirSpace;
    std::shared_ptr<frLef58SpacingEndOfLineWithinConstraint> withinConstraint;
  };

  class frLef58CornerSpacingSpacingConstraint;

  // SPACING Constraints
  class frSpacingConstraint : public frConstraint {
  public:
    frSpacingConstraint(): minSpacing(0) {}
    frSpacingConstraint(frCoord minSpacingIn): minSpacing(minSpacingIn) {}

    // getter
    frCoord getMinSpacing() const {
      return minSpacing;
    }
    // setter
    void setMinSpacing(frCoord minSpacingIn) {
      minSpacing = minSpacingIn;
    }
    // check
    frConstraintTypeEnum typeId() const override {
      return frConstraintTypeEnum::frcSpacingConstraint;
    }
  protected:
    frCoord minSpacing;
  };
  
  class frSpacingSamenetConstraint : public frSpacingConstraint {
  public:
    frSpacingSamenetConstraint(): frSpacingConstraint(), pgonly(false) {}
    frSpacingSamenetConstraint(frCoord minSpacingIn, bool pgonlyIn): frSpacingConstraint(minSpacingIn), pgonly(pgonlyIn) {}
    // getter
    bool hasPGonly() const {
      return pgonly;
    }
    // setter
    void setPGonly(bool in) {
      pgonly = in;
    }
    // check
    frConstraintTypeEnum typeId() const override {
      return frConstraintTypeEnum::frcSpacingSamenetConstraint;
    }
  protected:
    bool pgonly;
  };

  // EOL spacing
  class frSpacingEndOfLineConstraint : public frSpacingConstraint {
  public:
    // constructor
    frSpacingEndOfLineConstraint(): frSpacingConstraint(), 
                                    eolWidth(-1), eolWithin(-1), 
                                    parSpace(-1), parWithin(-1),
                                    isTwoEdges(false) {}
    // getter
    frCoord getEolWidth() const {
      return eolWidth;
    }
    frCoord getEolWithin() const {
      return eolWithin;
    }
    frCoord getParSpace() const {
      return parSpace;
    }
    frCoord getParWithin() const {
      return parWithin;
    }
    bool hasParallelEdge() const {
      return ((parSpace == -1) ? false : true);
    }
    bool hasTwoEdges() const {
      return isTwoEdges;
    }
    // setter
    void setEolWithin(frCoord eolWithinIn) {
      eolWithin = eolWithinIn;
    }
    void setEolWidth(frCoord eolWidthIn) {
      eolWidth = eolWidthIn;
    }
    void setParSpace(frCoord parSpaceIn) {
      parSpace = parSpaceIn;
    }
    void setParWithin(frCoord parWithinIn) {
      parWithin = parWithinIn;
    }
    void setTwoEdges(bool isTwoEdgesIn) {
      isTwoEdges = isTwoEdgesIn;
    }
    frConstraintTypeEnum typeId() const override {
      return frConstraintTypeEnum::frcSpacingEndOfLineConstraint;
    }

  protected:
    frCoord eolWidth, eolWithin;
    frCoord parSpace, parWithin;
    bool isTwoEdges;
  };


  class frLef58CutSpacingTableLayerConstraint : public frConstraint {
  public:
    // constructors
    frLef58CutSpacingTableLayerConstraint(): secondLayerNum(0), nonZeroEnc(false) {}
    // getters
    frLayerNum getSecondLayerNum() const {
      return secondLayerNum;
    }
    bool isNonZeroEnc() const {
      return nonZeroEnc;
    }
    // setters
    void setSecondLayerNum(frLayerNum in) {
      secondLayerNum = in;
    }
    void setNonZeroEnc(bool in) {
      nonZeroEnc = in;
    }
    // others
    frConstraintTypeEnum typeId() const override {
      return frConstraintTypeEnum::frcLef58CutSpacingTableLayerConstraint;
    }
  protected:
    frLayerNum secondLayerNum;
    bool       nonZeroEnc;
  };

  class frLef58CutSpacingTablePrlConstraint : public frConstraint {
  public:
    // constructors
    frLef58CutSpacingTablePrlConstraint(): prl(0), horizontal(false), vertical(false), 
                                           maxXY(false) {}
    // getters
    frCoord getPrl() const {
      return prl;
    }
    bool isHorizontal() const {
      return horizontal;
    }
    bool isVertical() const {
      return vertical;
    }
    bool isMaxXY() const {
      return maxXY;
    }
    // setters
    void setPrl(frCoord in) {
      prl = in;
    }
    void setHorizontal(bool in) {
      horizontal = in;
    }
    void setVertical(bool in) {
      vertical = in;
    }
    void setMaxXY(bool in) {
      maxXY = in;
    }
    // others
    frConstraintTypeEnum typeId() const override {
      return frConstraintTypeEnum::frcLef58CutSpacingTablePrlConstraint;
    }
  protected:
    frCoord prl;
    bool    horizontal;
    bool    vertical;
    bool    maxXY;
  };

  // LEF58 cut spacing table
  class frLef58CutSpacingTableConstraint : public frConstraint {
  public:
    // constructor
    frLef58CutSpacingTableConstraint(): cutClassHasAll(false), defaultCutSpacing(0) {}
    // getter
    std::shared_ptr<fr2DLookupTbl<frString, frString, std::pair<frCoord, frCoord> > > getCutClassTbl() const {
      return cutClassTbl;
    }
    bool hasPrlConstraint() const {
      return (prlConstraint) ? true : false;
    }
    std::shared_ptr<frLef58CutSpacingTablePrlConstraint> getPrlConstraint() const {
      return prlConstraint;
    }
    bool hasLayerConstraint() const {
      return (layerConstraint) ? true : false;
    }
    std::shared_ptr<frLef58CutSpacingTableLayerConstraint> getLayerConstraint() const {
      return layerConstraint;
    }
    bool hasAll() const {
      return cutClassHasAll;
    }
    frCoord getDefaultCutSpacing() const {
      return defaultCutSpacing;
    }
    // setter
    void setCutClassTbl(std::shared_ptr<fr2DLookupTbl<frString, frString, std::pair<frCoord, frCoord> > > &in) {
      cutClassTbl = in;
    }
    void setPrlConstraint(const std::shared_ptr<frLef58CutSpacingTablePrlConstraint> &in) {
      prlConstraint = in;
    }
    void setLayerConstraint(const std::shared_ptr<frLef58CutSpacingTableLayerConstraint> &in) {
      layerConstraint = in;
    }
    void setAll(bool in) {
      cutClassHasAll = in;
    }
    void setDefaultCutSpacing(frCoord in) {
      defaultCutSpacing = in;
    }
    // others
    frConstraintTypeEnum typeId() const override {
      return frConstraintTypeEnum::frcLef58CutSpacingTableConstraint;
    }
  protected:
    std::shared_ptr<fr2DLookupTbl<frString, frString, std::pair<frCoord, frCoord> > > cutClassTbl;
    std::shared_ptr<frLef58CutSpacingTablePrlConstraint>                              prlConstraint;
    std::shared_ptr<frLef58CutSpacingTableLayerConstraint>                            layerConstraint;
    bool                                                                              cutClassHasAll;
    frCoord                                                                           defaultCutSpacing;
  };

  // new SPACINGTABLE Constraints
  class frSpacingTablePrlConstraint : public frConstraint {
  public:
    // constructor
    frSpacingTablePrlConstraint(const fr2DLookupTbl<frCoord, frCoord, frCoord> &in): tbl(in) {}
    // getter
    const fr2DLookupTbl<frCoord, frCoord, frCoord>& getLookupTbl() const {
      return tbl;
    }
    frCoord find(frCoord width, frCoord prl) const {
      return tbl.find(width, prl);
    }
    frCoord findMin() const {
      return tbl.findMin();
    }
    frCoord findMax() const {
      return tbl.findMax();
    }
    // setter
    void setLookupTbl(const fr2DLookupTbl<frCoord, frCoord, frCoord> &in) {
      tbl = in;
    }
    frConstraintTypeEnum typeId() const override {
      return frConstraintTypeEnum::frcSpacingTablePrlConstraint;
    }
  protected:
    fr2DLookupTbl<frCoord, frCoord, frCoord> tbl;
  };

  struct frSpacingTableTwRowType {
    frSpacingTableTwRowType(frCoord in1, frCoord in2): width(in1), prl(in2) {}
    frCoord width;
    frCoord prl;
    bool operator<(const frSpacingTableTwRowType &b) const {
      return width < b.width || prl < b.prl;
    }
  };
  // new SPACINGTABLE Constraints
  class frSpacingTableTwConstraint : public frConstraint {
  public:
    // constructor
    frSpacingTableTwConstraint(const fr2DLookupTbl<frSpacingTableTwRowType, frSpacingTableTwRowType, frCoord> &in): tbl(in) {}
    // getter
    const fr2DLookupTbl<frSpacingTableTwRowType, frSpacingTableTwRowType, frCoord>& getLookupTbl() const {
      return tbl;
    }
    frCoord find(frCoord width1, frCoord width2, frCoord prl) const {
      return tbl.find(frSpacingTableTwRowType(width1, prl), frSpacingTableTwRowType(width2, prl));
    }
    frCoord findMin() const {
      return tbl.findMin();
    }
    // setter
    void setLookupTbl(const fr2DLookupTbl<frSpacingTableTwRowType, frSpacingTableTwRowType, frCoord> &in) {
      tbl = in;
    }
    frConstraintTypeEnum typeId() const override {
      return frConstraintTypeEnum::frcSpacingTableTwConstraint;
    }
  protected:
    fr2DLookupTbl<frSpacingTableTwRowType, frSpacingTableTwRowType, frCoord> tbl;
  };
  
  // original SPACINGTABLE Constraints
  class frSpacingTableConstraint : public frConstraint {
  public:
    // constructor
    frSpacingTableConstraint(std::shared_ptr<fr2DLookupTbl<frCoord, frCoord, frCoord> > parallelRunLengthConstraintIn) {
      parallelRunLengthConstraint = parallelRunLengthConstraintIn;
    }
    // getter
    std::shared_ptr<fr2DLookupTbl<frCoord, frCoord, frCoord> > getParallelRunLengthConstraint() {
      return parallelRunLengthConstraint;
    }
    // setter
    void setParallelRunLengthConstraint(std::shared_ptr<fr2DLookupTbl<frCoord, frCoord, frCoord> > &parallelRunLengthConstraintIn) {
      parallelRunLengthConstraint = parallelRunLengthConstraintIn;
    }

    frConstraintTypeEnum typeId() const override {
      return frConstraintTypeEnum::frcSpacingTableConstraint;
    }
  protected:
    std::shared_ptr<fr2DLookupTbl<frCoord, frCoord, frCoord> > parallelRunLengthConstraint;
  };
  
  class frLef58SpacingTableConstraint : public frSpacingTableConstraint {
  public:
    // constructor
    frLef58SpacingTableConstraint(const std::shared_ptr<fr2DLookupTbl<frCoord, frCoord, frCoord> > &parallelRunLengthConstraintIn,
                                  const std::map<int, std::pair<frCoord, frCoord> > &exceptWithinConstraintIn): 
      frSpacingTableConstraint(parallelRunLengthConstraintIn), exceptWithinConstraint(exceptWithinConstraintIn), 
      wrongDirection(false), sameMask(false), exceptEol(false), eolWidth(0) {}
    // getter
    bool hasExceptWithin(frCoord val) const {
      auto rowIdx = parallelRunLengthConstraint->getRowIdx(val);
      return (exceptWithinConstraint.find(rowIdx) != exceptWithinConstraint.end());
    }
    std::pair<frCoord, frCoord> getExceptWithin(frCoord val) const {
      auto rowIdx = parallelRunLengthConstraint->getRowIdx(val);
      return exceptWithinConstraint.at(rowIdx);
    }
    bool isWrongDirection() const {
      return wrongDirection;
    }
    bool isSameMask() const {
      return sameMask;
    }
    bool hasExceptEol() const {
      return exceptEol;
    }
    frUInt4 getEolWidth() const {
      return eolWidth;
    }
    // setters
    void setExceptWithinConstraint(std::map<int, std::pair<frCoord, frCoord> > &exceptWithinConstraintIn) {
      exceptWithinConstraint = exceptWithinConstraintIn;
    }
    void setWrongDirection(bool in) {
      wrongDirection = in;
    }
    void setSameMask(bool in) {
      sameMask = in;
    }
    void setEolWidth(frUInt4 in) {
      exceptEol = true;
      eolWidth = in;
    }

    frConstraintTypeEnum typeId() const override {
      return frConstraintTypeEnum::frcLef58SpacingTableConstraint;
    }
  protected:
    std::map<frCoord, std::pair<frCoord, frCoord> > exceptWithinConstraint;
    bool wrongDirection;
    bool sameMask;
    bool exceptEol;
    frUInt4 eolWidth;
  };

  // ADJACENTCUTS
  class frCutSpacingConstraint : public frConstraint {
  public:
  // constructor
  frCutSpacingConstraint() {}
  frCutSpacingConstraint(
    frCoord cutSpacingIn,
    bool centerToCenterIn,
    bool sameNetIn,
    frString secondLayerNameIn,
    bool stackIn,
    int adjacentCutsIn,
    frCoord cutWithinIn,
    bool isExceptSamePGNetIn,
    bool isParallelOverlapIn,
    frCoord cutAreaIn,
    int twoCutsIn = -1
  ) {
    cutSpacing = cutSpacingIn;
    centerToCenter = centerToCenterIn;
    sameNet = sameNetIn;
    secondLayerName = secondLayerNameIn;
    stack = stackIn;
    adjacentCuts = adjacentCutsIn;
    cutWithin = cutWithinIn;
    exceptSamePGNet = isExceptSamePGNetIn;
    parallelOverlap = isParallelOverlapIn;
    cutArea = cutAreaIn;
    twoCuts = twoCutsIn;
  }
  // getter
  bool hasCenterToCenter() const {
    return centerToCenter;
  }
  bool getCenterToCenter() const {
    return centerToCenter;
  }
  bool getSameNet() const {
    return sameNet;
  }
  bool hasSameNet() const {
    return sameNet;
  }
  frCutSpacingConstraint *getSameNetConstraint() {
    return sameNetConstraint;
  }
  bool getStack() const {
    return stack;
  }
  bool hasStack() const {
    return stack;
  }
  bool isLayer() const {
    return !(secondLayerName.empty());
  }
  const frString& getSecondLayerName() const {
    return secondLayerName;
  }
  bool hasSecondLayer() const {
    return (secondLayerNum != -1);
  }
  frLayerNum getSecondLayerNum() const {
    return secondLayerNum;
  }
  bool isAdjacentCuts() const {
    return (adjacentCuts != -1);
  }
  int getAdjacentCuts() const {
    return adjacentCuts;
  }
  frCoord getCutWithin() const {
    return cutWithin;
  }
  bool hasExceptSamePGNet() const {
    return exceptSamePGNet;
  }
  bool getExceptSamePGNet() const {
    return exceptSamePGNet;
  }
  bool isParallelOverlap() const {
    return parallelOverlap;
  }
  bool getParallelOverlap() const {
    return parallelOverlap;
  }
  bool isArea() const {
    return !(cutArea == -1);
  }
  bool getCutArea() const {
    return cutArea;
  }
  frCoord getCutSpacing() const {
    return cutSpacing;
  }
  frConstraintTypeEnum typeId() const override {
      return frConstraintTypeEnum::frcCutSpacingConstraint;
  }
  // LEF58 related
  bool isTwoCuts() const {
    return (twoCuts == -1);
  }
  int getTwoCuts() const {
    return twoCuts;
  }



  // setter

  void setSecondLayerNum(int secondLayerNumIn) {
    secondLayerNum = secondLayerNumIn;
  }

  void setSameNetConstraint(frCutSpacingConstraint *in) {
    sameNetConstraint = in;
  }

  protected:
    frCoord cutSpacing = -1;
    bool centerToCenter = false;
    bool sameNet = false;
    frCutSpacingConstraint *sameNetConstraint = nullptr;
    bool stack = false;
    bool exceptSamePGNet = false;
    bool parallelOverlap = false;
    frString secondLayerName;
    frLayerNum secondLayerNum = -1;
    int adjacentCuts = -1;
    frCoord cutWithin = -1;
    frCoord cutArea = -1;
    // LEF58 related
    int twoCuts = -1;

  };

  // LEF58_SPACING for cut layer (new)
  class frLef58CutSpacingConstraint : public frConstraint {
  public:
    // constructor
    frLef58CutSpacingConstraint(): cutSpacing(-1), sameMask(false), maxXY(false), centerToCenter(false), sameNet(false),
                                   sameMetal(false), sameVia(false), secondLayerName(""), secondLayerNum(-1),
                                   stack(false), orthogonalSpacing(-1), cutClassName(""), cutClassIdx(-1),
                                   shortEdgeOnly(false), prl(-1), concaveCorner(false), width(-1), 
                                   enclosure(-1), edgeLength(-1), parLength(-1), parWithin(-1), edgeEnclosure(-1),
                                   adjEnclosure(-1), extension(-1), eolWidth(-1), minLength(-1), maskOverlap(false),
                                   wrongDirection(false), adjacentCuts(-1), exactAlignedCut(-1), twoCuts(-1),
                                   twoCutsSpacing(-1), sameCut(false), cutWithin1(-1), cutWithin2(-1), 
                                   exceptSamePGNet(false), exceptAllWithin(-1), above(false), below(false),
                                   toAll(false), noPrl(false), sideParallelOverlap(false), parallelOverlap(false),
                                   exceptSameNet(false), exceptSameMetal(false), exceptSameMetalOverlap(false),
                                   exceptSameVia(false), within(-1), longEdgeOnly(false), exceptTwoEdges(false),
                                   numCut(-1), cutArea(-1) {}
    // getters
    // is == what rules have; has == what derived from values
    frCoord getCutSpacing() const {
      return cutSpacing;
    }
    bool isSameMask() const {
      return sameMask;
    }
    bool isMaxXY() const {
      return maxXY;
    }
    bool isCenterToCenter() const {
      return centerToCenter;
    }
    bool isSameNet() const {
      return sameNet;
    }
    bool isSameMetal() const {
      return sameMetal;
    }
    bool isSameVia() const {
      return sameVia;
    }
    std::string getSecondLayerName() const {
      return secondLayerName;
    }
    bool hasSecondLayer() const {
      return (secondLayerNum != -1 || secondLayerName != std::string(""));
    }
    frLayerNum getSecondLayerNum() const {
      return secondLayerNum;
    }
    bool isStack() const {
      return stack;
    }
    bool hasOrthogonalSpacing() const {
      return (orthogonalSpacing != -1);
    }
    std::string getCutClassName() const {
      return cutClassName;
    }
    bool hasCutClass() const {
      return (cutClassIdx != -1 || cutClassName != std::string(""));
    }
    int getCutClassIdx() const {
      return cutClassIdx;
    }
    bool isShortEdgeOnly() const {
      return shortEdgeOnly;
    }
    bool hasPrl() const {
      return (prl != -1);
    }
    frCoord getPrl() const {
      return prl;
    }
    bool isConcaveCorner() const {
      return concaveCorner;
    }
    bool hasWidth() const {
      return (width != -1);
    }
    frCoord getWidth() const {
      return width;
    }
    bool hasEnclosure() const {
      return (enclosure != -1);
    }
    frCoord getEnclosure() const {
      return enclosure;
    }
    bool hasEdgeLength() const {
      return (edgeLength != -1);
    }
    frCoord getEdgeLength() const {
      return edgeLength;
    }
    bool hasParallel() const {
      return (parLength != -1);
    }
    frCoord getParlength() const {
      return parLength;
    }
    frCoord getParWithin() const {
      return parWithin;
    }
    frCoord getEdgeEnclosure() const {
      return edgeEnclosure;
    }
    frCoord getAdjEnclosure() const {
      return adjEnclosure;
    }
    bool hasExtension() const {
      return (extension != -1);
    }
    frCoord getExtension() const {
      return extension;
    }
    bool hasNonEolConvexCorner() const {
      return (eolWidth != -1);
    }
    frCoord getEolWidth() const {
      return eolWidth;
    }
    bool hasMinLength() const {
      return (minLength != -1);
    }
    frCoord getMinLength() const {
      return minLength;
    }
    bool hasAboveWidth() const {
      return (width != -1);
    }
    bool isMaskOverlap() const {
      return maskOverlap;
    }
    bool isWrongDirection() const {
      return wrongDirection;
    }
    bool hasAdjacentCuts() const {
      return (adjacentCuts != -1);
    }
    int getAdjacentCuts() const {
      return adjacentCuts;
    }
    bool hasExactAligned() const {
      return (exactAlignedCut != -1);
    }
    int getExactAligned() const {
      return exactAlignedCut;
    }
    bool hasTwoCuts() const {
      return (twoCuts != -1);
    }
    int getTwoCuts() const {
      return twoCuts;
    }
    bool hasTwoCutsSpacing() const {
      return (twoCutsSpacing != -1);
    }
    frCoord getTwoCutsSpacing() const {
      return twoCutsSpacing;
    }
    bool isSameCut() const {
      return sameCut;
    }
    // cutWithin2 is always used as upper bound
    bool hasTwoCutWithin() const {
      return (cutWithin1 != -1);
    }
    frCoord getCutWithin() const {
      return cutWithin2;
    }
    frCoord getCutWithin1() const {
      return cutWithin1;
    }
    frCoord getCutWithin2() const {
      return cutWithin2;
    }
    bool isExceptSamePGNet() const {
      return exceptSamePGNet;
    }
    bool hasExceptAllWithin() const {
      return (exceptAllWithin != -1);
    }
    frCoord getExceptAllWithin() const {
      return exceptAllWithin;
    }
    bool isAbove() const {
      return above;
    }
    bool isBelow() const {
      return below;
    }
    bool isToAll() const {
      return toAll;
    }
    bool isNoPrl() const {
      return noPrl;
    }
    bool isSideParallelOverlap() const {
      return sideParallelOverlap;
    }
    bool isParallelOverlap() const {
      return parallelOverlap;
    }
    bool isExceptSameNet() const {
      return exceptSameNet;
    }
    bool isExceptSameMetal() const {
      return exceptSameMetal;
    }
    bool isExceptSameMetalOverlap() const {
      return exceptSameMetalOverlap;
    }
    bool isExceptSameVia() const {
      return exceptSameVia;
    }
    bool hasParallelWithin() const {
      return (within != -1);
    }
    frCoord getWithin() const {
      return within;
    }
    bool isLongEdgeOnly() const {
      return longEdgeOnly;
    }
    bool hasSameMetalSharedEdge() const {
      return (parWithin != -1);
    }
    bool isExceptTwoEdges() const {
      return exceptTwoEdges;
    }
    bool hasExceptSameVia() const {
      return (numCut != -1);
    }
    bool hasArea() const {
      return (cutArea != -1);
    }
    frCoord getCutArea() const {
      return cutArea;
    }
    // setters
    void setCutSpacing(frCoord in) {
      cutSpacing = in;
    }
    void setSameMask(bool in) {
      sameMask = in;
    }
    void setMaxXY(bool in) {
      maxXY = in;
    }
    void setCenterToCenter(bool in) {
      centerToCenter = in;
    }
    void setSameNet(bool in) {
      sameNet = in;
    }
    void setSameMetal(bool in) {
      sameMetal = in;
    }
    void setSameVia(bool in) {
      sameVia = in;
    }
    void setSecondLayerName(const std::string &in) {
      secondLayerName = in;
    }
    void setSecondLayerNum(frLayerNum in) {
      secondLayerNum = in;
    }
    void setStack(bool in) {
      stack = in;
    }
    void setOrthogonalSpacing(frCoord in) {
      orthogonalSpacing = in;
    }
    void setCutClassName(const std::string &in) {
      cutClassName = in;
    }
    void setCutClassIdx(int in) {
      cutClassIdx = in;
    }
    void setShortEdgeOnly(bool in) {
      shortEdgeOnly = in;
    }
    void setPrl(frCoord in) {
      prl = in;
    }
    void setConcaveCorner(bool in) {
      concaveCorner = in;
    }
    void setWidth(frCoord in) {
      width = in;
    }
    void setEnclosure(frCoord in) {
      enclosure = in;
    }
    void setEdgeLength(frCoord in) {
      edgeLength = in;
    }
    void setParLength(frCoord in) {
      parLength = in;
    }
    void setParWithin(frCoord in) {
      parWithin = in;
    }
    void setEdgeEnclosure(frCoord in) {
      edgeEnclosure = in;
    }
    void setAdjEnclosure(frCoord in) {
      adjEnclosure = in;
    }
    void setExtension(frCoord in) {
      extension = in;
    }
    void setEolWidth(frCoord in) {
      eolWidth = in;
    }
    void setMinLength(frCoord in) {
      minLength = in;
    }
    void setMaskOverlap(bool in) {
      maskOverlap = in;
    }
    void setWrongDirection(bool in) {
      wrongDirection = in;
    }
    void setAdjacentCuts(int in) {
      adjacentCuts = in;
    }
    void setExactAlignedCut(int in) {
      exactAlignedCut = in;
    }
    void setTwoCuts(int in) {
      twoCuts = in;
    }
    void setTwoCutsSpacing(frCoord in) {
      twoCutsSpacing = in;
    }
    void setSameCut(bool in) {
      sameCut = in;
    }
    void setCutWithin(frCoord in) {
      cutWithin2 = in;
    }
    void setCutWithin1(frCoord in) {
      cutWithin1 = in;
    }
    void setCutWithin2(frCoord in) {
      cutWithin2 = in;
    }
    void setExceptSamePGNet(bool in) {
      exceptSamePGNet = in;
    }
    void setExceptAllWithin(frCoord in) {
      exceptAllWithin = in;
    }
    void setAbove(bool in) {
      above = in;
    }
    void setBelow(bool in) {
      below = in;
    }
    void setToAll(bool in) {
      toAll = in;
    }
    void setNoPrl(bool in) {
      noPrl = in;
    }
    void setSideParallelOverlap(bool in) {
      sideParallelOverlap = in;
    }
    void setExceptSameNet(bool in) {
      exceptSameNet = in;
    }
    void setExceptSameMetal(bool in) {
      exceptSameMetal = in;
    }
    void setExceptSameMetalOverlap(bool in) {
      exceptSameMetalOverlap = in;
    }
    void setExceptSameVia(bool in) {
      exceptSameVia = in;
    }
    void setWithin(frCoord in) {
      within = in;
    }
    void setLongEdgeOnly(bool in) {
      longEdgeOnly = in;
    }
    void setExceptTwoEdges(bool in) {
      exceptTwoEdges = in;
    }
    void setNumCut(int in) {
      numCut = in;
    }
    void setCutArea(frCoord in) {
      cutArea = in;
    }

    // others
    frConstraintTypeEnum typeId() const override {
      return frConstraintTypeEnum::frcLef58CutSpacingConstraint;
    }

  protected:
    frCoord cutSpacing;
    bool sameMask;
    bool maxXY;
    bool centerToCenter;
    bool sameNet;
    bool sameMetal;
    bool sameVia;
    std::string secondLayerName;
    frLayerNum secondLayerNum;
    bool stack;
    frCoord orthogonalSpacing;
    std::string cutClassName;
    int cutClassIdx;
    bool shortEdgeOnly;
    frCoord prl;
    bool concaveCorner;
    frCoord width;
    frCoord enclosure;
    frCoord edgeLength;
    frCoord parLength;
    frCoord parWithin;
    frCoord edgeEnclosure;
    frCoord adjEnclosure;
    frCoord extension;
    frCoord eolWidth;
    frCoord minLength;
    bool maskOverlap;
    bool wrongDirection;
    int adjacentCuts;
    int exactAlignedCut;
    int twoCuts;
    frCoord twoCutsSpacing;
    bool sameCut;
    frCoord cutWithin1;
    frCoord cutWithin2;
    bool exceptSamePGNet;
    frCoord exceptAllWithin;
    bool above;
    bool below;
    bool toAll;
    bool noPrl;
    bool sideParallelOverlap;
    bool parallelOverlap;
    bool exceptSameNet;
    bool exceptSameMetal;
    bool exceptSameMetalOverlap;
    bool exceptSameVia;
    frCoord within;
    bool longEdgeOnly;
    bool exceptTwoEdges;
    int numCut;
    frCoord cutArea;
  };

  // LEF58_CORNERSPACING (new)
  class frLef58CornerSpacingConstraint : public frConstraint {
  public:
    // constructor
    frLef58CornerSpacingConstraint(const fr1DLookupTbl<frCoord, std::pair<frCoord, frCoord> > &tblIn) : 
                                       cornerType(frCornerTypeEnum::UNKNOWN), sameMask(false), within(-1),
                                       eolWidth(-1), length(-1), edgeLength(false), includeLShape(false),
                                       minLength(-1), exceptNotch(false), notchLength(-1), exceptSameNet(false),
                                       exceptSameMetal(false), tbl(tblIn), sameXY(false) {}

    // getters
    frConstraintTypeEnum typeId() const override {
      return frConstraintTypeEnum::frcLef58CornerSpacingConstraint;
    }
    frCornerTypeEnum getCornerType() const {
      return cornerType;
    }
    bool getSameMask() const {
      return sameMask;
    }
    bool hasCornerOnly() const {
      return (within != -1);
    }
    frCoord getWithin() const {
      return within;
    }
    bool hasExceptEol() const {
      return (eolWidth != -1);
    }
    frCoord getEolWidth() const {
      return eolWidth;
    }
    bool hasExceptJogLength() const {
      return (minLength != -1);
    }
    frCoord getLength() const {
      return length;
    }
    bool hasEdgeLength() const {
      return edgeLength;
    }
    bool getEdgeLength() const {
      return edgeLength;
    }
    bool hasIncludeLShape() const {
      return includeLShape;
    }
    bool getIncludeLShape() const {
      return includeLShape;
    }
    frCoord getMinLength() const {
      return minLength;
    }
    bool hasExceptNotch() const {
      return exceptNotch;
    }
    bool getExceptNotch() const {
      return exceptNotch;
    }
    frCoord getNotchLength() const {
      return notchLength;
    }
    bool hasExceptSameNet() const {
      return exceptSameNet;
    }
    bool getExceptSameNet() const {
      return exceptSameNet;
    }
    bool hasExceptSameMetal() const {
      return exceptSameMetal;
    }
    bool getExceptSameMetal() const {
      return exceptSameMetal;
    }
    frCoord find(frCoord width, bool isHorizontal = true) const {
      return (isHorizontal ? tbl.find(width).first : tbl.find(width).second);
    }
    frCoord findMin() const {
      return std::min(tbl.findMin().first, tbl.findMin().second);
    }
    frCoord findMax() const {
      return std::max(tbl.findMax().first, tbl.findMax().second);
    }
    frCoord findMax(bool isHorz) const {
      return (isHorz ? tbl.findMax().first : tbl.findMax().second);
    }
    bool hasSameXY() const {
      return sameXY;
    }
    bool getSameXY() const {
      return sameXY;
    }

    // setters
    void setCornerType(frCornerTypeEnum in) {
      cornerType = in;
    }
    void setSameMask(bool in) {
      sameMask = in;
    }
    void setWithin(frCoord in) {
      within = in;
    }
    void setEolWidth(frCoord in) {
      eolWidth = in;
    }
    void setLength(frCoord in) {
      length = in;
    }
    void setEdgeLength(bool in) {
      edgeLength = in;
    }
    void setIncludeLShape(bool in) {
      includeLShape = in;
    }
    void setMinLength(frCoord in) {
      minLength = in;
    }
    void setExceptNotch(bool in) {
      exceptNotch = in;
    }
    void setExceptNotchLength(frCoord in) {
      notchLength = in;
    }
    void setExceptSameNet(bool in) {
      exceptSameNet = in;
    }
    void setExceptSameMetal(bool in) {
      exceptSameMetal = in;
    }
    void setLookupTbl(const fr1DLookupTbl<frCoord, std::pair<frCoord, frCoord> > &in) {
      tbl = in;
    }
    void setSameXY(bool in) {
      sameXY = in;
    }

  protected:
    frCornerTypeEnum cornerType;
    bool sameMask;
    frCoord within;
    frCoord eolWidth;
    frCoord length;
    bool edgeLength;
    bool includeLShape;
    frCoord minLength;
    bool exceptNotch;
    frCoord notchLength;
    bool exceptSameNet;
    bool exceptSameMetal;
    fr1DLookupTbl<frCoord, std::pair<frCoord, frCoord> > tbl; // horz / vert spacing
    bool sameXY; // indicate whether horz spacing == vert spacing // for write LEF some day
  };


  class frLef58CornerSpacingSpacingConstraint : public frConstraint {
  public:
    // constructor
    frLef58CornerSpacingSpacingConstraint(frCoord widthIn)
      : width(widthIn)
    {}
    // getter
    frCoord getWidth() {
      return width;
    }
    // setter
    void setWidth(frCoord widthIn) {
      width = widthIn;
    }

    frConstraintTypeEnum typeId() const override {
      return frConstraintTypeEnum::frcLef58CornerSpacingSpacingConstraint;
    }
  protected:
    frCoord width;
  };

  class frLef58CornerSpacingSpacing1DConstraint : public frLef58CornerSpacingSpacingConstraint {
  public:
    // constructor
    frLef58CornerSpacingSpacing1DConstraint(frCoord widthIn, frCoord spacingIn)
      : frLef58CornerSpacingSpacingConstraint(widthIn), spacing(spacingIn)
    {}
    // getter
    frCoord getSpacing() {
      return spacing;
    }
    // setter
    void setSpacing(frCoord spacingIn) {
      spacing = spacingIn;
    }

    frConstraintTypeEnum typeId() const override {
      return frConstraintTypeEnum::frcLef58CornerSpacingSpacing1DConstraint;
    }
    
  protected:
    frCoord spacing = -1;
  };

  class frLef58CornerSpacingSpacing2DConstraint : public frLef58CornerSpacingSpacingConstraint {
  public:
    // constructor
    frLef58CornerSpacingSpacing2DConstraint(frCoord widthIn, frCoord horizontalSpacingIn, frCoord verticalSpacingIn)
      : frLef58CornerSpacingSpacingConstraint(widthIn), horizontalSpacing(horizontalSpacingIn), verticalSpacing(verticalSpacingIn)
    {}
    // getter
    frCoord getHorizontalSpacing() {
      return horizontalSpacing;
    }
    frCoord getVerticalSpacing() {
      return verticalSpacing;
    }
    // setter
    void setHorizontalSpacing(frCoord horizontalSpacingIn) {
      horizontalSpacing = horizontalSpacingIn;
    }
    void setVerticalSpacing(frCoord verticalSpacingIn) {
      verticalSpacing = verticalSpacingIn;
    }

    frConstraintTypeEnum typeId() const override {
      return frConstraintTypeEnum::frcLef58CornerSpacingSpacing2DConstraint;
    }
  protected:
    frCoord horizontalSpacing = -1, verticalSpacing = -1;
  };

  class frLef58RectOnlyConstraint : public frConstraint {
  public:
    // constructor
    frLef58RectOnlyConstraint(bool exceptNonCorePinsIn = false) : exceptNonCorePins(exceptNonCorePinsIn) {

    }
    // getter
    bool isExceptNonCorePinsIn() {
      return exceptNonCorePins;
    }
    // setter
    void setExceptNonCorePins(bool exceptNonCorePinsIn) {
      exceptNonCorePins = exceptNonCorePinsIn;
    }

    frConstraintTypeEnum typeId() const override {
      return frConstraintTypeEnum::frcLef58RectOnlyConstraint;
    }
  protected:
    bool exceptNonCorePins;
  };

  class frLef58RightWayOnGridOnlyConstraint : public frConstraint {
  public:
    // constructor
    frLef58RightWayOnGridOnlyConstraint(bool checkMaskIn = false) : checkMask(checkMaskIn) {

    }
    // getter
    bool isCheckMask() {
      return checkMask;
    }
    // setter
    void setCheckMask(bool checkMaskIn) {
      checkMask = checkMaskIn;
    }

    frConstraintTypeEnum typeId() const override {
      return frConstraintTypeEnum::frcLef58RightWayOnGridOnlyConstraint;
    }
  protected:
    bool checkMask;
  };

      using namespace std;
  class frNonDefaultRule{
      friend class FlexRP;
      friend class frTechObject;
      private:
      
         string name_;
        //each vector position is a metal layer
        vector<frCoord> widths_; 
        vector<frCoord> spacings_;
        vector<frCoord> wireExtensions_;
        vector<int> minCuts_; //min cuts per cut layer 

        //vias for each layer
        vector<vector<frViaDef*>> vias_;
        vector<vector<frViaRuleGenerate*>> viasRules_;  
        
        bool hardSpacing_ = false;
        
        std::vector<std::vector<std::vector<std::pair<frCoord, frCoord>>>> via2ViaForbiddenLen;
        std::vector<std::vector<std::vector<std::pair<frCoord, frCoord>>>> viaForbiddenTurnLen;
        
  public:

    frViaDef* getPrefVia(int z){
        if (z >= vias_.size() || vias_[z].empty()) 
          return nullptr;
        return vias_[z][0];
    }
    void setWidth(frCoord w, int z){
        if (z >= widths_.size()) 
          widths_.resize(z+1, 0);
        widths_[z] = w;
    }
    
    void setSpacing(frCoord s, int z){
        if (z >= spacings_.size()) 
          spacings_.resize(z+1, 0);
        spacings_[z] = s;
    }
    
    void setMinCuts(int ncuts, int z){
        if (z >= minCuts_.size())
          minCuts_.resize(z+1, 1);
        minCuts_[z] = ncuts;
    }

    void setWireExtension(frCoord we, int z){
        if (z >= wireExtensions_.size()) 
          wireExtensions_.resize(z+1, 0);
        wireExtensions_[z] = we;
    }

    void addVia(frViaDef* via, int z){
        if (z >= vias_.size()) 
          vias_.resize(z+1, vector<frViaDef*>());
        vias_[z].push_back(via);
    }

    void addViaRule(frViaRuleGenerate* via, int z){
        if (z >= viasRules_.size()) 
          viasRules_.resize(z+1, vector<frViaRuleGenerate*>());
        viasRules_[z].push_back(via);
    }
    
    void setHardSpacing(bool isHard){
        hardSpacing_ = isHard;
    }
    
    void setName(const char* n){
        name_ = string(n);
    }
    
    string getName() const{
        return name_;
    }
    
    frCoord getWidth(int z){
        if (z >= widths_.size()) 
          return 0;
        return widths_[z];
    }
    
    frCoord getSpacing(int z){
        if (z >= spacings_.size()) 
          return 0;
        return spacings_[z];
    }
    
    frCoord getWireExtension(int z){
        if (z >= wireExtensions_.size()) 
          return 0;
        return wireExtensions_[z];
    }
    
    int getMinCuts(int z){
        if (z >= minCuts_.size()) 
          return 1;
        return minCuts_[z];
    }
    
    const vector<frViaDef*>& getVias(int z){
        return vias_[z];
    }
    
    const vector<frViaRuleGenerate*>& getViaRules(int z){
        return viasRules_[z];
    }
    
    bool isHardSpacing(){
        return hardSpacing_;
    }
    
    
  };
}

#endif
