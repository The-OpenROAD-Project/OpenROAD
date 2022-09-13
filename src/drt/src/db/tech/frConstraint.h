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

#ifndef _FR_CONSTRAINT_H_
#define _FR_CONSTRAINT_H_

#include <algorithm>
#include <iterator>
#include <map>
#include <memory>
#include <utility>

#include "db/tech/frLookupTbl.h"
#include "frBaseTypes.h"
#include "frViaDef.h"
#include "frViaRuleGenerate.h"
#include "odb/db.h"
#include "utl/Logger.h"

namespace fr {
class frLayer;
namespace io {
class Parser;
}

using ForbiddenRanges = std::vector<std::pair<frCoord, frCoord>>;

enum class frLef58CornerSpacingExceptionEnum {
  NONE,
  EXCEPTSAMENET,
  EXCEPTSAMEMETAL
};

struct drEolSpacingConstraint {
  drEolSpacingConstraint(frCoord width = 0,
                         frCoord space = 0,
                         frCoord within = 0)
      : eolWidth(width), eolSpace(space), eolWithin(within) {}
  frCoord eolWidth;
  frCoord eolSpace;
  frCoord eolWithin;
};

// base type for design rule
class frConstraint {
 public:
  virtual ~frConstraint() {}
  virtual frConstraintTypeEnum typeId() const = 0;
  virtual void report(utl::Logger* logger) const = 0;
  void setLayer(frLayer* layer) { layer_ = layer; }
  void setId(int in) { id_ = in; }
  int getId() const { return id_; }
  std::string getViolName() const {
    switch (typeId()) {
      case frConstraintTypeEnum::frcShortConstraint:
        return "Short";
      case frConstraintTypeEnum::frcMinWidthConstraint:
        return "Min Width";

      case frConstraintTypeEnum::frcSpacingConstraint:
        return "Metal Spacing";

      case frConstraintTypeEnum::frcSpacingEndOfLineConstraint:
        return "EOL Spacing";

      case frConstraintTypeEnum::frcSpacingTablePrlConstraint:
        return "Metal Spacing";

      case frConstraintTypeEnum::frcCutSpacingConstraint:
        return "Cut Spacing";

      case frConstraintTypeEnum::frcMinStepConstraint:
        return "Min Step";

      case frConstraintTypeEnum::frcNonSufficientMetalConstraint:
        return "NS Metal";

      case frConstraintTypeEnum::frcSpacingSamenetConstraint:
        return "Metal Spacing";

      case frConstraintTypeEnum::frcOffGridConstraint:
        return "Off Grid";

      case frConstraintTypeEnum::frcMinEnclosedAreaConstraint:
        return "Min Hole";

      case frConstraintTypeEnum::frcAreaConstraint:
        return "Min Area";

      case frConstraintTypeEnum::frcLef58CornerSpacingConstraint:
        return "Corner Spacing";

      case frConstraintTypeEnum::frcLef58CutSpacingConstraint:
        return "Cut Spacing";

      case frConstraintTypeEnum::frcLef58RectOnlyConstraint:
        return "Rect Only";

      case frConstraintTypeEnum::frcLef58RightWayOnGridOnlyConstraint:
        return "RightWayOnGridOnly";

      case frConstraintTypeEnum::frcLef58MinStepConstraint:
        return "Min Step";

      case frConstraintTypeEnum::frcSpacingTableInfluenceConstraint:
        return "MetSpacingInf";

      case frConstraintTypeEnum::frcSpacingEndOfLineParallelEdgeConstraint:
        return "SpacingEOLParallelEdge";

      case frConstraintTypeEnum::frcSpacingTableConstraint:
        return "SpacingTable";

      case frConstraintTypeEnum::frcSpacingTableTwConstraint:
        return "SpacingTableTw";

      case frConstraintTypeEnum::frcLef58SpacingTableConstraint:
        return "Lef58SpacingTable";

      case frConstraintTypeEnum::frcLef58CutSpacingTableConstraint:
        return "Lef58CutSpacingTable";

      case frConstraintTypeEnum::frcLef58CutSpacingTablePrlConstraint:
        return "Lef58CutSpacingTablePrl";

      case frConstraintTypeEnum::frcLef58CutSpacingTableLayerConstraint:
        return "Lef58CutSpacingTableLayer";

      case frConstraintTypeEnum::frcLef58CutSpacingParallelWithinConstraint:
        return "Lef58CutSpacingParallelWithin";

      case frConstraintTypeEnum::frcLef58CutSpacingAdjacentCutsConstraint:
        return "Lef58CutSpacingAdjacentCuts";

      case frConstraintTypeEnum::frcLef58CutSpacingLayerConstraint:
        return "Lef58CutSpacingLayer";

      case frConstraintTypeEnum::frcMinimumcutConstraint:
        return "Minimum Cut";

      case frConstraintTypeEnum::frcLef58CornerSpacingConcaveCornerConstraint:
        return "Lef58CornerSpacingConcaveCorner";

      case frConstraintTypeEnum::frcLef58CornerSpacingConvexCornerConstraint:
        return "Lef58CornerSpacingConvexCorner";

      case frConstraintTypeEnum::frcLef58CornerSpacingSpacingConstraint:
        return "Lef58CornerSpacingSpacing";

      case frConstraintTypeEnum::frcLef58CornerSpacingSpacing1DConstraint:
        return "Lef58CornerSpacingSpacing1D";

      case frConstraintTypeEnum::frcLef58CornerSpacingSpacing2DConstraint:
        return "Lef58CornerSpacingSpacing2D";

      case frConstraintTypeEnum::frcLef58SpacingEndOfLineConstraint:
        return "Lef58SpacingEndOfLine";

      case frConstraintTypeEnum::frcLef58SpacingEndOfLineWithinConstraint:
        return "Lef58SpacingEndOfLineWithin";

      case frConstraintTypeEnum::
          frcLef58SpacingEndOfLineWithinEndToEndConstraint:
        return "Lef58SpacingEndOfLineWithinEndToEnd";

      case frConstraintTypeEnum::
          frcLef58SpacingEndOfLineWithinEncloseCutConstraint:
        return "Lef58SpacingEndOfLineWithinEncloseCut";

      case frConstraintTypeEnum::
          frcLef58SpacingEndOfLineWithinParallelEdgeConstraint:
        return "Lef58SpacingEndOfLineWithinParallelEdge";

      case frConstraintTypeEnum::
          frcLef58SpacingEndOfLineWithinMaxMinLengthConstraint:
        return "Lef58SpacingEndOfLineWithinMaxMinLength";

      case frConstraintTypeEnum::frcLef58CutClassConstraint:
        return "Lef58CutClass";

      case frConstraintTypeEnum::frcRecheckConstraint:
        return "Recheck";

      case frConstraintTypeEnum::frcLef58EolExtensionConstraint:
        return "Lef58EolExtension";

      case frConstraintTypeEnum::frcLef58EolKeepOutConstraint:
        return "Lef58EolKeepOut";
    }
    return "";
  }

 protected:
  int id_;
  frLayer* layer_;
  frConstraint() : id_(-1), layer_(nullptr) {}
};

class frLef58CutClassConstraint : public frConstraint {
 public:
  friend class io::Parser;
  // constructors;
  frLef58CutClassConstraint() {}
  // getters
  frCollection<std::shared_ptr<frLef58CutClass>> getCutClasses() const {
    frCollection<std::shared_ptr<frLef58CutClass>> sol;
    std::transform(cutClasses.begin(),
                   cutClasses.end(),
                   std::back_inserter(sol),
                   [](auto& kv) { return kv.second; });
    return sol;
  }
  // setters
  void addToCutClass(const std::shared_ptr<frLef58CutClass>& in) {
    cutClasses[in->getName()] = in;
  }
  // others
  frConstraintTypeEnum typeId() const override {
    return frConstraintTypeEnum::frcLef58CutClassConstraint;
  }
  void report(utl::Logger* logger) const override {
    logger->report("Cut class");
  }

 protected:
  std::map<frString, std::shared_ptr<frLef58CutClass>> cutClasses;
};

// recheck constraint for negative rules
class frRecheckConstraint : public frConstraint {
 public:
  frConstraintTypeEnum typeId() const override {
    return frConstraintTypeEnum::frcRecheckConstraint;
  }
  void report(utl::Logger* logger) const override { logger->report("Recheck"); }
};

// short

class frShortConstraint : public frConstraint {
 public:
  frConstraintTypeEnum typeId() const override {
    return frConstraintTypeEnum::frcShortConstraint;
  }
  void report(utl::Logger* logger) const override { logger->report("Short"); }
};

// NSMetal
class frNonSufficientMetalConstraint : public frConstraint {
 public:
  frConstraintTypeEnum typeId() const override {
    return frConstraintTypeEnum::frcNonSufficientMetalConstraint;
  }
  void report(utl::Logger* logger) const override { logger->report("NSMetal"); }
};

// offGrid
class frOffGridConstraint : public frConstraint {
 public:
  frConstraintTypeEnum typeId() const override {
    return frConstraintTypeEnum::frcOffGridConstraint;
  }
  void report(utl::Logger* logger) const override {
    logger->report("Off grid");
  }
};

// minHole
class frMinEnclosedAreaConstraint : public frConstraint {
 public:
  // constructor
  frMinEnclosedAreaConstraint() : rule_(nullptr) {}
  // getter
  odb::dbTechMinEncRule* getDbTechMinEncRule() const { return rule_; }
  frCoord getArea() const {
    frUInt4 minEnclosedArea;
    rule_->getEnclosure(minEnclosedArea);
    return minEnclosedArea;
  }
  bool hasWidth() const {
    frUInt4 width;
    return rule_->getEnclosureWidth(width);
  }
  frCoord getWidth() const {
    frUInt4 width;
    return (rule_->getEnclosureWidth(width)) ? width : -1;
  }
  // setter
  void setDbTechMinEncRule(odb::dbTechMinEncRule* ruleIn) { rule_ = ruleIn; }

  frConstraintTypeEnum typeId() const override {
    return frConstraintTypeEnum::frcMinEnclosedAreaConstraint;
  }

  void report(utl::Logger* logger) const override {
    logger->report("Min enclosed area {} width {}", getArea(), getWidth());
  }

 protected:
  odb::dbTechMinEncRule* rule_;
};

// LEF58_MINSTEP (currently only implement GF14 related API)
class frLef58MinStepConstraint : public frConstraint {
 public:
  // constructor
  frLef58MinStepConstraint()
      : rule_(nullptr),
        insideCorner(false),
        outsideCorner(false),
        step(false),
        maxLength(-1),
        convexCorner(false),
        exceptWithin(-1),
        concaveCorner(false),
        threeConcaveCorners(false),
        width(-1),
        minAdjLength2(-1),
        minBetweenLength(-1),
        exceptSameCorners(false),
        concaveCorners(false) {}
  // getter
  odb::dbTechLayerMinStepRule* getDbTechLayerMinStepRule() const {
    return rule_;
  }
  frCoord getMinStepLength() const { return rule_->getMinStepLength(); }
  bool hasMaxEdges() const { return rule_->isMaxEdgesValid(); }
  int getMaxEdges() const {
    return rule_->isMaxEdgesValid() ? rule_->getMaxEdges() : -1;
  }
  bool hasMinAdjacentLength() const { return rule_->isMinAdjLength1Valid(); }
  frCoord getMinAdjacentLength() const {
    return rule_->isMinAdjLength1Valid() ? rule_->getMinAdjLength1() : -1;
  }
  bool hasEolWidth() const { return rule_->isNoBetweenEol(); }
  frCoord getEolWidth() const {
    return rule_->isNoBetweenEol() ? rule_->getEolWidth() : -1;
  }

  // setter
  void setDbTechLayerMinStepRule(odb::dbTechLayerMinStepRule* ruleIn) {
    rule_ = ruleIn;
  }

  frConstraintTypeEnum typeId() const override {
    return frConstraintTypeEnum::frcLef58MinStepConstraint;
  }
  void report(utl::Logger* logger) const override {
    logger->report(
        "MINSTEP minStepLength {} insideCorner {} outsideCorner {} step {} "
        "maxLength {} maxEdges {} minAdjLength {} convexCorner {} exceptWithin "
        "{} concaveCorner {} threeConcaveCorners {} width {} minAdjLength2 {} "
        "minBetweenLength {} exceptSameCorners {} eolWidth {} concaveCorners "
        "{} ",
        getMinStepLength(),
        insideCorner,
        outsideCorner,
        step,
        maxLength,
        getMaxEdges(),
        getMinAdjacentLength(),
        convexCorner,
        exceptWithin,
        concaveCorner,
        threeConcaveCorners,
        width,
        minAdjLength2,
        minBetweenLength,
        exceptSameCorners,
        getEolWidth(),
        concaveCorners);
  }

 protected:
  odb::dbTechLayerMinStepRule* rule_;
  bool insideCorner;
  bool outsideCorner;
  bool step;
  frCoord maxLength;
  bool convexCorner;
  frCoord exceptWithin;
  bool concaveCorner;
  bool threeConcaveCorners;
  frCoord width;
  frCoord minAdjLength2;
  frCoord minBetweenLength;
  bool exceptSameCorners;
  bool concaveCorners;
};

// minStep
class frMinStepConstraint : public frConstraint {
 public:
  // constructor
  frMinStepConstraint()
      : layer_(nullptr),
        minstepType(frMinstepTypeEnum::UNKNOWN),
        insideCorner(false),
        outsideCorner(true),
        step(false) {}
  // getter
  frCoord getMinStepLength() const { return layer_->getMinStep(); }
  bool hasMaxLength() const { return layer_->hasMinStepMaxLength(); }
  frCoord getMaxLength() const {
    return (layer_->hasMinStepMaxLength()) ? layer_->getMinStepMaxLength() : -1;
  }
  bool hasMinstepType() const {
    return minstepType != frMinstepTypeEnum::UNKNOWN;
  }
  frMinstepTypeEnum getMinstepType() const { return minstepType; }
  bool hasInsideCorner() const { return insideCorner; }
  bool hasOutsideCorner() const { return outsideCorner; }
  bool hasStep() const { return step; }
  bool hasMaxEdges() const { return layer_->hasMinStepMaxEdges(); }
  int getMaxEdges() const {
    return (layer_->hasMinStepMaxEdges()) ? layer_->getMinStepMaxEdges() : -1;
  }
  // setter
  void setDbTechLayer(odb::dbTechLayer* layerIn) { layer_ = layerIn; }
  void setMinstepType(frMinstepTypeEnum in) { minstepType = in; }
  void setInsideCorner(bool in) { insideCorner = in; }
  void setOutsideCorner(bool in) { outsideCorner = in; }
  void setStep(bool in) { step = in; }

  frConstraintTypeEnum typeId() const override {
    return frConstraintTypeEnum::frcMinStepConstraint;
  }

  void report(utl::Logger* logger) const override {
    logger->report(
        "Min step length min {} type {} max {} "
        "insideCorner {} outsideCorner {} step {} maxEdges {}",
        getMinStepLength(),
        int(minstepType),
        getMaxLength(),
        insideCorner,
        outsideCorner,
        step,
        getMaxEdges());
  }

 protected:
  odb::dbTechLayer* layer_;
  frMinstepTypeEnum minstepType;
  bool insideCorner;
  bool outsideCorner;
  bool step;
};

// minimumcut
class frMinimumcutConstraint : public frConstraint {
 public:
  frMinimumcutConstraint() : rule_(nullptr) {}
  // getters
  odb::dbTechMinCutRule* getDbTechMinCutRule() const { return rule_; }
  int getNumCuts() const {
    frUInt4 numCuts, temp;
    return (rule_->getMinimumCuts(numCuts, temp)) ? numCuts : -1;
  }
  frCoord getWidth() const {
    frUInt4 width, temp;
    return (rule_->getMinimumCuts(temp, width)) ? width : -1;
  }
  bool hasWithin() const {
    frUInt4 within;
    return rule_->getCutDistance(within);
  }
  frCoord getCutDistance() const {
    frUInt4 within;
    return (rule_->getCutDistance(within)) ? within : -1;
  }
  bool hasConnection() const {
    return !(getConnection() == frMinimumcutConnectionEnum::UNKNOWN);
  }
  frMinimumcutConnectionEnum getConnection() const {
    return (rule_->isAboveOnly())
               ? frMinimumcutConnectionEnum::FROMABOVE
               : (rule_->isBelowOnly()) ? frMinimumcutConnectionEnum::FROMBELOW
                                        : frMinimumcutConnectionEnum::UNKNOWN;
  }
  bool hasLength() const {
    frUInt4 length, temp;
    return rule_->getLengthForCuts(length, temp);
  }
  frCoord getLength() const {
    frUInt4 length, temp;
    return (rule_->getLengthForCuts(length, temp)) ? length : -1;
  }
  frCoord getDistance() const {
    frUInt4 distance, temp;
    return (rule_->getLengthForCuts(temp, distance)) ? distance : -1;
  }
  // setters
  void setDbTechMinCutRule(odb::dbTechMinCutRule* ruleIn) { rule_ = ruleIn; }
  // others
  frConstraintTypeEnum typeId() const override {
    return frConstraintTypeEnum::frcMinimumcutConstraint;
  }
  void report(utl::Logger* logger) const override {
    logger->report(
        "Min cut numCuts {} width {} cutDistance {} "
        "connection {} length {} distance {}",
        getNumCuts(),
        getWidth(),
        getCutDistance(),
        getConnection(),
        getLength(),
        getDistance());
  }

 protected:
  odb::dbTechMinCutRule* rule_;
};

// minArea

class frAreaConstraint : public frConstraint {
 public:
  // constructor
  frAreaConstraint(frCoord minAreaIn) { minArea = minAreaIn; }
  // getter
  frCoord getMinArea() { return minArea; }
  // setter
  void setMinArea(frCoord minAreaIn) { minArea = minAreaIn; }

  frConstraintTypeEnum typeId() const override {
    return frConstraintTypeEnum::frcAreaConstraint;
  }
  void report(utl::Logger* logger) const override {
    logger->report("Area {}", minArea);
  }

 protected:
  frCoord minArea;
};

// minWidth
class frMinWidthConstraint : public frConstraint {
 public:
  // constructor
  frMinWidthConstraint(frCoord minWidthIn) { minWidth = minWidthIn; }
  // getter
  frCoord getMinWidth() { return minWidth; }
  // setter
  void set(frCoord minWidthIn) { minWidth = minWidthIn; }

  frConstraintTypeEnum typeId() const override {
    return frConstraintTypeEnum::frcMinWidthConstraint;
  }
  void report(utl::Logger* logger) const override {
    logger->report("Width {}", minWidth);
  }

 protected:
  frCoord minWidth;
};

class frLef58SpacingEndOfLineWithinEncloseCutConstraint : public frConstraint {
 public:
  // constructors
  frLef58SpacingEndOfLineWithinEncloseCutConstraint() : rule_(nullptr) {}
  // setters
  void setDbTechLayerSpacingEolRule(odb::dbTechLayerSpacingEolRule* ruleIn) {
    rule_ = ruleIn;
  }
  // getters
  odb::dbTechLayerSpacingEolRule* getDbTechLayerSpacingEolRule() const {
    return rule_;
  }
  bool isAboveOnly() const { return rule_->isAboveValid(); }
  bool isBelowOnly() const { return rule_->isBelowValid(); }
  bool isAboveAndBelow() const {
    return !(rule_->isAboveValid() ^ rule_->isBelowValid());
  }
  bool isAllCuts() const { return rule_->isAllCutsValid(); }
  frCoord getEncloseDist() const { return rule_->getEncloseDist(); }
  frCoord getCutToMetalSpace() const { return rule_->getCutToMetalSpace(); }
  // others
  frConstraintTypeEnum typeId() const override {
    return frConstraintTypeEnum::
        frcLef58SpacingEndOfLineWithinEncloseCutConstraint;
  }
  void report(utl::Logger* logger) const override {
    logger->report(
        "\t\tSPACING_WITHIN_ENCLOSECUT below {} above {} encloseDist "
        "{} cutToMetalSpace {} allCuts {}",
        isBelowOnly(),
        isAboveOnly(),
        getEncloseDist(),
        getCutToMetalSpace(),
        isAllCuts());
  }

 private:
  odb::dbTechLayerSpacingEolRule* rule_;
};

class frLef58SpacingEndOfLineWithinEndToEndConstraint : public frConstraint {
 public:
  // constructors
  frLef58SpacingEndOfLineWithinEndToEndConstraint()
      : rule_(nullptr), cutSpace(false) {}
  // getters
  odb::dbTechLayerSpacingEolRule* getDbTechLayerSpacingEolRule() const {
    return rule_;
  }
  frCoord getEndToEndSpace() const { return rule_->getEndToEndSpace(); }
  frCoord getOneCutSpace() const { return rule_->getOneCutSpace(); }
  frCoord getTwoCutSpace() const { return rule_->getTwoCutSpace(); }
  bool hasExtension() const { return rule_->isExtensionValid(); }
  frCoord getExtension() const {
    return (rule_->isExtensionValid()) ? rule_->getExtension() : 0;
  }
  frCoord getWrongDirExtension() const {
    return (rule_->isWrongDirExtensionValid()) ? rule_->getWrongDirExtension()
                                               : 0;
  }
  bool hasOtherEndWidth() const { return rule_->isOtherEndWidthValid(); }
  frCoord getOtherEndWidth() const {
    return (rule_->isOtherEndWidthValid()) ? rule_->getOtherEndWidth() : 0;
  }

  // setters
  void setDbTechLayerSpacingEolRule(odb::dbTechLayerSpacingEolRule* ruleIn) {
    rule_ = ruleIn;
  }

  // others
  frConstraintTypeEnum typeId() const override {
    return frConstraintTypeEnum::
        frcLef58SpacingEndOfLineWithinEndToEndConstraint;
  }
  void report(utl::Logger* logger) const override {
    logger->report(
        "\t\tSPACING_WITHIN_ENDTOEND endToEndSpace {} cutSpace {} oneCutSpace "
        "{} twoCutSpace {} hExtension {} extension {} wrongDirExtension {} "
        "hOtherEndWidth {} otherEndWidth {} ",
        getEndToEndSpace(),
        cutSpace,
        getOneCutSpace(),
        getTwoCutSpace(),
        hasExtension(),
        getExtension(),
        getWrongDirExtension(),
        hasOtherEndWidth(),
        getOtherEndWidth());
  }

 protected:
  odb::dbTechLayerSpacingEolRule* rule_;
  bool cutSpace;
};

class frLef58SpacingEndOfLineWithinParallelEdgeConstraint
    : public frConstraint {
 public:
  // constructors
  frLef58SpacingEndOfLineWithinParallelEdgeConstraint() : rule_(nullptr) {}
  // getters
  odb::dbTechLayerSpacingEolRule* getDbTechLayerSpacingEolRule() const {
    return rule_;
  }
  bool hasSubtractEolWidth() const { return rule_->isSubtractEolWidthValid(); }
  frCoord getParSpace() const { return rule_->getParSpace(); }
  frCoord getParWithin() const { return rule_->getParWithin(); }
  bool hasPrl() const { return rule_->isParPrlValid(); }
  frCoord getPrl() const {
    return (rule_->isParPrlValid()) ? rule_->getParPrl() : 0;
  }
  bool hasMinLength() const { return rule_->isParMinLengthValid(); }
  frCoord getMinLength() const {
    return (rule_->isParMinLengthValid()) ? rule_->getParMinLength() : 0;
  }
  bool hasTwoEdges() const { return rule_->isTwoEdgesValid(); }
  bool hasSameMetal() const { return rule_->isSameMetalValid(); }
  bool hasNonEolCornerOnly() const { return rule_->isNonEolCornerOnlyValid(); }
  bool hasParallelSameMask() const { return rule_->isParallelSameMaskValid(); }
  // setters
  void setDbTechLayerSpacingEolRule(odb::dbTechLayerSpacingEolRule* ruleIn) {
    rule_ = ruleIn;
  }

  // others
  frConstraintTypeEnum typeId() const override {
    return frConstraintTypeEnum::
        frcLef58SpacingEndOfLineWithinParallelEdgeConstraint;
  }
  void report(utl::Logger* logger) const override {
    logger->report(
        "\t\tSPACING_WITHIN_PARALLELEDGE subtractEolWidth {} parSpace {} "
        "parWithin {} hPrl {} prl {} hMinLength {} minLength {} twoEdges {} "
        "sameMetal {} nonEolCornerOnly {} parallelSameMask {} ",
        hasSubtractEolWidth(),
        getParSpace(),
        getParWithin(),
        hasPrl(),
        getPrl(),
        hasMinLength(),
        getMinLength(),
        hasTwoEdges(),
        hasSameMetal(),
        hasNonEolCornerOnly(),
        hasParallelSameMask());
  }

 protected:
  odb::dbTechLayerSpacingEolRule* rule_;
};

class frLef58SpacingEndOfLineWithinMaxMinLengthConstraint
    : public frConstraint {
 public:
  // constructors
  frLef58SpacingEndOfLineWithinMaxMinLengthConstraint() : rule_(nullptr) {}

  // getters
  odb::dbTechLayerSpacingEolRule* getDbTechLayerSpacingEolRule() const {
    return rule_;
  }
  frCoord getLength() const {
    return rule_->isMinLengthValid() ? rule_->getMinLength()
                                     : rule_->getMaxLength();
  }
  bool isMaxLength() const { return rule_->isMaxLengthValid(); }
  bool isTwoSides() const { return rule_->isTwoEdgesValid(); }

  // setters
  void setDbTechLayerSpacingEolRule(odb::dbTechLayerSpacingEolRule* ruleIn) {
    rule_ = ruleIn;
  }
  // others
  frConstraintTypeEnum typeId() const override {
    return frConstraintTypeEnum::
        frcLef58SpacingEndOfLineWithinMaxMinLengthConstraint;
  }
  void report(utl::Logger* logger) const override {
    logger->report(
        "\t\tSPACING_WITHIN_MAXMIN maxLength {} length {} twoSides {} ",
        isMaxLength(),
        getLength(),
        isTwoSides());
  }

 protected:
  odb::dbTechLayerSpacingEolRule* rule_;
};

class frLef58SpacingEndOfLineWithinConstraint : public frConstraint {
 public:
  // constructors
  frLef58SpacingEndOfLineWithinConstraint()
      : rule_(nullptr),
        endToEndConstraint(nullptr),
        parallelEdgeConstraint(nullptr) {}

  // getters
  odb::dbTechLayerSpacingEolRule* getDbTechLayerSpacingEolRule() const {
    return rule_;
  }
  bool hasOppositeWidth() const { return rule_->isOppositeWidthValid(); }
  frCoord getOppositeWidth() const {
    return rule_->isOppositeWidthValid() ? rule_->getOppositeWidth() : 0;
  }
  frCoord getEolWithin() const { return rule_->getEolWithin(); }
  frCoord getWrongDirWithin() const {
    return rule_->isWrongDirWithinValid() ? rule_->getWrongDirWithin() : 0;
  }
  frCoord getEndPrlSpacing() const {
    return rule_->isEndPrlSpacingValid() ? rule_->getEndPrlSpace() : 0;
  }
  frCoord getEndPrl() const {
    return rule_->isEndPrlSpacingValid() ? rule_->getEndPrl() : 0;
  }
  bool hasSameMask() const { return rule_->isSameMaskValid(); }
  bool hasExceptExactWidth() const {
    return false;  // skip for now
  }
  bool hasFillConcaveCorner() const {
    return false;  // skip for now
  }
  bool hasWithCut() const {
    return false;  // skip for now
  }
  bool hasEndPrlSpacing() const { return rule_->isEndPrlSpacingValid(); }
  bool hasEndToEndConstraint() const {
    return (endToEndConstraint) ? true : false;
  }
  std::shared_ptr<frLef58SpacingEndOfLineWithinEndToEndConstraint>
  getEndToEndConstraint() const {
    return endToEndConstraint;
  }
  bool hasMinMaxLength() const {
    return false;  // skip for now
  }
  bool hasEqualRectWidth() const {
    return false;  // skip for now
  }
  bool hasParallelEdgeConstraint() const {
    return (parallelEdgeConstraint) ? true : false;
  }
  std::shared_ptr<frLef58SpacingEndOfLineWithinParallelEdgeConstraint>
  getParallelEdgeConstraint() const {
    return parallelEdgeConstraint;
  }
  bool hasMaxMinLengthConstraint() const {
    return (maxMinLengthConstraint) ? true : false;
  }
  std::shared_ptr<frLef58SpacingEndOfLineWithinMaxMinLengthConstraint>
  getMaxMinLengthConstraint() const {
    return maxMinLengthConstraint;
  }
  bool hasEncloseCutConstraint() const {
    return (encloseCutConstraint) ? true : false;
  }
  std::shared_ptr<frLef58SpacingEndOfLineWithinEncloseCutConstraint>
  getEncloseCutConstraint() const {
    return encloseCutConstraint;
  }
  // setters
  void setDbTechLayerSpacingEolRule(odb::dbTechLayerSpacingEolRule* ruleIn) {
    rule_ = ruleIn;
  }
  void setEndToEndConstraint(
      const std::shared_ptr<frLef58SpacingEndOfLineWithinEndToEndConstraint>&
          in) {
    endToEndConstraint = in;
  }
  void setParallelEdgeConstraint(
      const std::shared_ptr<
          frLef58SpacingEndOfLineWithinParallelEdgeConstraint>& in) {
    parallelEdgeConstraint = in;
  }
  void setMaxMinLengthConstraint(
      const std::shared_ptr<
          frLef58SpacingEndOfLineWithinMaxMinLengthConstraint>& in) {
    maxMinLengthConstraint = in;
  }
  void setEncloseCutConstraint(
      const std::shared_ptr<frLef58SpacingEndOfLineWithinEncloseCutConstraint>&
          in) {
    encloseCutConstraint = in;
  }
  // others
  frConstraintTypeEnum typeId() const override {
    return frConstraintTypeEnum::frcLef58SpacingEndOfLineWithinConstraint;
  }
  void report(utl::Logger* logger) const override {
    logger->report(
        "\tSPACING_WITHIN hOppositeWidth {} oppositeWidth {} eolWithin {} "
        "wrongDirWithin {} sameMask {} endPrlSpacing {} endPrl {}",
        hasOppositeWidth(),
        getOppositeWidth(),
        getEolWithin(),
        getWrongDirWithin(),
        hasSameMask(),
        getEndPrlSpacing(),
        getEndPrl());
    if (endToEndConstraint != nullptr)
      endToEndConstraint->report(logger);
    if (parallelEdgeConstraint != nullptr)
      parallelEdgeConstraint->report(logger);
  }

 protected:
  odb::dbTechLayerSpacingEolRule* rule_;
  std::shared_ptr<frLef58SpacingEndOfLineWithinEndToEndConstraint>
      endToEndConstraint;
  std::shared_ptr<frLef58SpacingEndOfLineWithinParallelEdgeConstraint>
      parallelEdgeConstraint;
  std::shared_ptr<frLef58SpacingEndOfLineWithinMaxMinLengthConstraint>
      maxMinLengthConstraint;
  std::shared_ptr<frLef58SpacingEndOfLineWithinEncloseCutConstraint>
      encloseCutConstraint;
};

class frLef58SpacingEndOfLineConstraint : public frConstraint {
 public:
  // constructors
  frLef58SpacingEndOfLineConstraint()
      : rule_(nullptr), withinConstraint(nullptr) {}
  // getters
  odb::dbTechLayerSpacingEolRule* getDbTechLayerSpacingEolRule() const {
    return rule_;
  }
  frCoord getEolSpace() const { return rule_->getEolSpace(); }
  frCoord getEolWidth() const { return rule_->getEolWidth(); }
  bool hasExactWidth() const { return rule_->isExactWidthValid(); }
  bool hasWrongDirSpacing() const { return rule_->isWrongDirSpacingValid(); }
  frCoord getWrongDirSpace() const {
    return rule_->isWrongDirSpacingValid() ? rule_->getWrongDirSpace() : 0;
  }
  bool hasWithinConstraint() const { return (withinConstraint) ? true : false; }
  std::shared_ptr<frLef58SpacingEndOfLineWithinConstraint> getWithinConstraint()
      const {
    return withinConstraint;
  }
  bool hasToConcaveCornerConstraint() const { return false; }
  bool hasToNotchLengthConstraint() const { return false; }
  // setters
  void setDbTechLayerSpacingEolRule(odb::dbTechLayerSpacingEolRule* ruleIn) {
    rule_ = ruleIn;
  }
  void setWithinConstraint(
      const std::shared_ptr<frLef58SpacingEndOfLineWithinConstraint>& in) {
    withinConstraint = in;
  }
  // others
  frConstraintTypeEnum typeId() const override {
    return frConstraintTypeEnum::frcLef58SpacingEndOfLineConstraint;
  }
  void report(utl::Logger* logger) const override {
    logger->report(
        "SPACING eolSpace {} eolWidth {} exactWidth {} wrongDirSpacing {} "
        "wrongDirSpace {} ",
        getEolSpace(),
        getEolWidth(),
        hasExactWidth(),
        hasWrongDirSpacing(),
        getWrongDirSpace());
    if (withinConstraint != nullptr)
      withinConstraint->report(logger);
  }

 protected:
  odb::dbTechLayerSpacingEolRule* rule_;
  std::shared_ptr<frLef58SpacingEndOfLineWithinConstraint> withinConstraint;
};

class frLef58EolKeepOutConstraint : public frConstraint {
 public:
  // constructors
  frLef58EolKeepOutConstraint() : rule_(nullptr) {}
  // getters
  odb::dbTechLayerEolKeepOutRule* getDbTechLayerEolKeepOutRule() const {
    return rule_;
  }
  frCoord getBackwardExt() const { return rule_->getBackwardExt(); }
  frCoord getForwardExt() const { return rule_->getForwardExt(); }
  frCoord getSideExt() const { return rule_->getSideExt(); }
  frCoord getEolWidth() const { return rule_->getEolWidth(); }
  bool isCornerOnly() const { return rule_->isCornerOnly(); }
  bool isExceptWithin() const { return rule_->isExceptWithin(); }
  frCoord getWithinLow() const { return rule_->getWithinLow(); }
  frCoord getWithinHigh() const { return rule_->getWithinHigh(); }
  // setters
  void setDbTechLayerEolKeepOutRule(odb::dbTechLayerEolKeepOutRule* ruleIn) {
    rule_ = ruleIn;
  }
  // others
  frConstraintTypeEnum typeId() const override {
    return frConstraintTypeEnum::frcLef58EolKeepOutConstraint;
  }
  void report(utl::Logger* logger) const override {
    logger->report(
        "EOLKEEPOUT backwardExt {} sideExt {} forwardExt {} eolWidth {} "
        "cornerOnly {} exceptWithin {} withinLow {} withinHigh {}",
        getBackwardExt(),
        getSideExt(),
        getForwardExt(),
        getEolWidth(),
        isCornerOnly(),
        isExceptWithin(),
        getWithinLow(),
        getWithinHigh());
  }

 private:
  odb::dbTechLayerEolKeepOutRule* rule_;
};

class frLef58CornerSpacingSpacingConstraint;

// SPACING Constraints
class frSpacingConstraint : public frConstraint {
 public:
  frSpacingConstraint() : minSpacing(0) {}
  frSpacingConstraint(frCoord minSpacingIn) : minSpacing(minSpacingIn) {}

  // getter
  frCoord getMinSpacing() const { return minSpacing; }
  // setter
  void setMinSpacing(frCoord minSpacingIn) { minSpacing = minSpacingIn; }
  // check
  frConstraintTypeEnum typeId() const override {
    return frConstraintTypeEnum::frcSpacingConstraint;
  }
  void report(utl::Logger* logger) const override {
    logger->report("Spacing {}", minSpacing);
  }

 protected:
  frCoord minSpacing;
};

class frSpacingSamenetConstraint : public frSpacingConstraint {
 public:
  frSpacingSamenetConstraint(frCoord minSpacingIn,
                             odb::dbTechLayerSpacingRule* ruleIn)
      : frSpacingConstraint(minSpacingIn), rule_(ruleIn) {}

  // getter
  odb::dbTechLayerSpacingRule* getDbTechLayerSpacingRule() { return rule_; }
  bool hasPGonly() const { return rule_->getSameNetPgOnly(); }
  // setter
  void setDbTechLayerSpacingRule(odb::dbTechLayerSpacingRule* ruleIn) {
    rule_ = ruleIn;
  }
  // check
  frConstraintTypeEnum typeId() const override {
    return frConstraintTypeEnum::frcSpacingSamenetConstraint;
  }
  void report(utl::Logger* logger) const override {
    logger->report("Spacing same net pgonly {}", hasPGonly());
  }

 protected:
  odb::dbTechLayerSpacingRule* rule_;
};

class frSpacingTableInfluenceConstraint : public frConstraint {
 public:
  frSpacingTableInfluenceConstraint(
      const fr1DLookupTbl<frCoord, std::pair<frCoord, frCoord>>& in)
      : tbl(in) {}
  // getter
  const fr1DLookupTbl<frCoord, std::pair<frCoord, frCoord>>& getLookupTbl()
      const {
    return tbl;
  }
  std::pair<frCoord, frCoord> find(frCoord width) const {
    return tbl.find(width);
  }
  frCoord getMinWidth() const { return tbl.getMinRow(); }
  // setter
  void setLookupTbl(
      const fr1DLookupTbl<frCoord, std::pair<frCoord, frCoord>>& in) {
    tbl = in;
  }

  frConstraintTypeEnum typeId() const override {
    return frConstraintTypeEnum::frcSpacingTableInfluenceConstraint;
  }
  void report(utl::Logger* logger) const override {
    logger->report("Spacing table influence");
  }

 private:
  fr1DLookupTbl<frCoord, std::pair<frCoord, frCoord>> tbl;
};

// EOL spacing
class frSpacingEndOfLineConstraint : public frSpacingConstraint {
 public:
  // constructor
  frSpacingEndOfLineConstraint() : frSpacingConstraint(), rule_(nullptr) {}
  // getter
  odb::dbTechLayerSpacingRule* getDbTechLayerSpacingRule() const {
    return rule_;
  }
  frCoord getEolWidth() const {
    frUInt4 eolWidth, temp;
    bool tempB;
    bool hasSpacingEndOfLine
        = rule_->getEol(eolWidth, temp, tempB, temp, temp, tempB);
    return (hasSpacingEndOfLine) ? eolWidth : -1;
  }
  frCoord getEolWithin() const {
    frUInt4 eolWithin, temp;
    bool tempB;
    bool hasSpacingEndOfLine
        = rule_->getEol(temp, eolWithin, tempB, temp, temp, tempB);
    return (hasSpacingEndOfLine) ? eolWithin : -1;
  }
  frCoord getParSpace() const {
    frUInt4 parSpace, temp;
    bool hasSpacingParellelEdge, tempB;
    bool hasSpacingEndOfLine = rule_->getEol(
        temp, temp, hasSpacingParellelEdge, parSpace, temp, tempB);
    return (hasSpacingEndOfLine && hasSpacingParellelEdge) ? parSpace : -1;
  }
  frCoord getParWithin() const {
    frUInt4 parWithin, temp;
    bool hasSpacingParellelEdge, tempB;
    bool hasSpacingEndOfLine = rule_->getEol(
        temp, temp, hasSpacingParellelEdge, temp, parWithin, tempB);
    return (hasSpacingEndOfLine && hasSpacingParellelEdge) ? parWithin : -1;
  }
  bool hasParallelEdge() const {
    frUInt4 temp;
    bool hasSpacingParellelEdge, tempB;
    bool hasSpacingEndOfLine
        = rule_->getEol(temp, temp, hasSpacingParellelEdge, temp, temp, tempB);
    return (hasSpacingEndOfLine) ? hasSpacingParellelEdge : false;
  }
  bool hasTwoEdges() const {
    frUInt4 temp;
    bool hasSpacingTwoEdges, hasSpacingParellelEdge;
    bool hasSpacingEndOfLine = rule_->getEol(
        temp, temp, hasSpacingParellelEdge, temp, temp, hasSpacingTwoEdges);
    return (hasSpacingEndOfLine && hasSpacingParellelEdge) ? hasSpacingTwoEdges
                                                           : false;
  }
  bool hasEndOfLine() const {
    frUInt4 temp;
    bool tempB;
    return rule_->getEol(temp, temp, tempB, temp, temp, tempB);
  }
  // setter
  void setDbTechLayerSpacingRule(odb::dbTechLayerSpacingRule* ruleIn) {
    rule_ = ruleIn;
  }

  frConstraintTypeEnum typeId() const override {
    return frConstraintTypeEnum::frcSpacingEndOfLineConstraint;
  }
  void report(utl::Logger* logger) const override {
    logger->report(
        "Spacing EOL eolWidth {} eolWithin {} "
        "parSpace {} parWithin {} isTwoEdges {}",
        getEolWidth(),
        getEolWithin(),
        getParSpace(),
        getParWithin(),
        hasTwoEdges());
  }

 protected:
  odb::dbTechLayerSpacingRule* rule_;
};

class frLef58EolExtensionConstraint : public frSpacingConstraint {
 public:
  // constructors
  frLef58EolExtensionConstraint(const fr1DLookupTbl<frCoord, frCoord>& tbl)
      : frSpacingConstraint(), rule_(nullptr), extensionTbl(tbl) {}
  // setters
  void setDbTechLayerEolExtensionRule(
      odb::dbTechLayerEolExtensionRule* ruleIn) {
    rule_ = ruleIn;
  }

  // getters

  bool isParallelOnly() const { return rule_->isParallelOnly(); }

  fr1DLookupTbl<frCoord, frCoord> getExtensionTable() const {
    return extensionTbl;
  }

  // others

  frConstraintTypeEnum typeId() const override {
    return frConstraintTypeEnum::frcLef58EolExtensionConstraint;
  }

  void report(utl::Logger* logger) const override {
    logger->report("EOLEXTENSIONSPACING spacing {} parallelonly {} ",
                   minSpacing,
                   isParallelOnly());
  }

 private:
  odb::dbTechLayerEolExtensionRule* rule_;
  fr1DLookupTbl<frCoord, frCoord> extensionTbl;
};

// LEF58 cut spacing table
class frLef58CutSpacingTableConstraint : public frConstraint {
 public:
  // constructor
  frLef58CutSpacingTableConstraint(
      odb::dbTechLayerCutSpacingTableDefRule* dbRule)
      : db_rule_(dbRule),
        default_spacing_({0, 0}),
        default_center2center_(false),
        default_centerAndEdge_(false) {}
  // setter
  void setDefaultSpacing(const std::pair<frCoord, frCoord>& value) {
    default_spacing_ = value;
  }
  void setDefaultCenterToCenter(bool value) { default_center2center_ = value; }
  void setDefaultCenterAndEdge(bool value) { default_centerAndEdge_ = value; }
  // getter
  odb::dbTechLayerCutSpacingTableDefRule* getODBRule() const {
    return db_rule_;
  }
  void report(utl::Logger* logger) const override {
    logger->report(
        "CUTSPACINGTABLE lyr:{} lyr2:{}",
        db_rule_->getTechLayer()->getName(),
        db_rule_->isLayerValid() ? db_rule_->getSecondLayer()->getName() : "-");
  }
  std::pair<frCoord, frCoord> getDefaultSpacing() const {
    return default_spacing_;
  }
  bool getDefaultCenterToCenter() const { return default_center2center_; }
  bool getDefaultCenterAndEdge() const { return default_centerAndEdge_; }
  // others
  frConstraintTypeEnum typeId() const override {
    return frConstraintTypeEnum::frcLef58CutSpacingTableConstraint;
  }

 private:
  odb::dbTechLayerCutSpacingTableDefRule* db_rule_;
  std::pair<frCoord, frCoord> default_spacing_;
  bool default_center2center_, default_centerAndEdge_;
};

// new SPACINGTABLE Constraints
class frSpacingTablePrlConstraint : public frConstraint {
 public:
  // constructor
  frSpacingTablePrlConstraint(
      const fr2DLookupTbl<frCoord, frCoord, frCoord>& in)
      : tbl(in) {}
  // getter
  const fr2DLookupTbl<frCoord, frCoord, frCoord>& getLookupTbl() const {
    return tbl;
  }
  frCoord find(frCoord width, frCoord prl) const {
    return tbl.find(width, prl);
  }
  frCoord findMin() const { return tbl.findMin(); }
  frCoord findMax() const { return tbl.findMax(); }
  // setter
  void setLookupTbl(const fr2DLookupTbl<frCoord, frCoord, frCoord>& in) {
    tbl = in;
  }
  frConstraintTypeEnum typeId() const override {
    return frConstraintTypeEnum::frcSpacingTablePrlConstraint;
  }
  void report(utl::Logger* logger) const override {
    logger->report("Spacing table PRL");
  }

 protected:
  fr2DLookupTbl<frCoord, frCoord, frCoord> tbl;
};

struct frSpacingTableTwRowType {
  frSpacingTableTwRowType(frCoord in1, frCoord in2) : width(in1), prl(in2) {}
  frCoord width;
  frCoord prl;
};

// new SPACINGTABLE Constraints
class frSpacingTableTwConstraint : public frConstraint {
 public:
  // constructor
  frSpacingTableTwConstraint(
      const frCollection<frSpacingTableTwRowType>& rowsIn,
      const frCollection<frCollection<frCoord>>& spacingIn)
      : rows(rowsIn), spacingTbl(spacingIn) {}
  // getter
  frCoord find(frCoord width1, frCoord width2, frCoord prl) const;
  frCoord findMin() const { return spacingTbl.front().front(); }
  frCoord findMax() const { return spacingTbl.back().back(); }
  // setter
  void setSpacingTable(const frCollection<frSpacingTableTwRowType>& rowsIn,
                       const frCollection<frCollection<frCoord>>& spacingIn) {
    rows = rowsIn;
    spacingTbl = spacingIn;
  }

  frConstraintTypeEnum typeId() const override {
    return frConstraintTypeEnum::frcSpacingTableTwConstraint;
  }
  void report(utl::Logger* logger) const override {
    logger->report("Spacing table tw");
  }

 private:
  frCollection<frSpacingTableTwRowType> rows;
  frCollection<frCollection<frCoord>> spacingTbl;

  frUInt4 getIdx(frCoord width, frCoord prl) const {
    int sz = rows.size();
    for (int i = 0; i < sz; i++) {
      if (width <= rows[i].width)
        return std::max(0, i - 1);
      if (rows[i].prl != -1 && prl <= rows[i].prl)
        return std::max(0, i - 1);
    }
    return sz - 1;
  }
};

// original SPACINGTABLE Constraints
class frSpacingTableConstraint : public frConstraint {
 public:
  // constructor
  frSpacingTableConstraint(
      std::shared_ptr<fr2DLookupTbl<frCoord, frCoord, frCoord>>
          parallelRunLengthConstraintIn) {
    parallelRunLengthConstraint = parallelRunLengthConstraintIn;
  }
  // getter
  const std::shared_ptr<fr2DLookupTbl<frCoord, frCoord, frCoord>>
  getParallelRunLengthConstraint() const {
    return parallelRunLengthConstraint;
  }
  // setter
  void setParallelRunLengthConstraint(
      std::shared_ptr<fr2DLookupTbl<frCoord, frCoord, frCoord>>&
          parallelRunLengthConstraintIn) {
    parallelRunLengthConstraint = parallelRunLengthConstraintIn;
  }

  frConstraintTypeEnum typeId() const override {
    return frConstraintTypeEnum::frcSpacingTableConstraint;
  }
  void report(utl::Logger* logger) const override {
    logger->report("Spacing table");
  }

 protected:
  std::shared_ptr<fr2DLookupTbl<frCoord, frCoord, frCoord>>
      parallelRunLengthConstraint;
};

class frLef58SpacingTableConstraint : public frSpacingTableConstraint {
 public:
  // constructor
  frLef58SpacingTableConstraint(
      const std::shared_ptr<fr2DLookupTbl<frCoord, frCoord, frCoord>>&
          parallelRunLengthConstraintIn,
      const std::map<int, std::pair<frCoord, frCoord>>&
          exceptWithinConstraintIn)
      : frSpacingTableConstraint(parallelRunLengthConstraintIn),
        exceptWithinConstraint(exceptWithinConstraintIn) {}
  // getter
  odb::dbTechLayerSpacingTablePrlRule* getDbTechLayerSpacingTablePrlRule()
      const {
    return rule_;
  }
  bool hasExceptWithin(frCoord val) const {
    auto rowIdx = getParallelRunLengthConstraint()->getRowIdx(val);
    return (exceptWithinConstraint.find(rowIdx)
            != exceptWithinConstraint.end());
  }
  std::pair<frCoord, frCoord> getExceptWithin(frCoord val) const {
    auto rowIdx = getParallelRunLengthConstraint()->getRowIdx(val);
    return exceptWithinConstraint.at(rowIdx);
  }
  bool isWrongDirection() const { return rule_->isWrongDirection(); }
  bool isSameMask() const { return rule_->isSameMask(); }
  bool hasExceptEol() const { return rule_->isExceeptEol(); }
  frUInt4 getEolWidth() const { return rule_->getEolWidth(); }
  // setters
  void setExceptWithinConstraint(
      std::map<int, std::pair<frCoord, frCoord>>& exceptWithinConstraintIn) {
    exceptWithinConstraint = exceptWithinConstraintIn;
  }
  void setDbTechLayerSpacingTablePrlRule(
      odb::dbTechLayerSpacingTablePrlRule* ruleIn) {
    rule_ = ruleIn;
  }

  frConstraintTypeEnum typeId() const override {
    return frConstraintTypeEnum::frcLef58SpacingTableConstraint;
  }

  void report(utl::Logger* logger) const override {
    logger->report(
        "SPACINGTABLE wrongDirection {} sameMask {} exceptEol {} eolWidth {} ",
        isWrongDirection(),
        isSameMask(),
        hasExceptEol(),
        getEolWidth());
    logger->report("\texceptWithinConstraint");
    for (auto& [key, val] : exceptWithinConstraint)
      logger->report("\t{} ({} {})", key, val.first, val.second);
  }

 protected:
  std::map<frCoord, std::pair<frCoord, frCoord>> exceptWithinConstraint;
  odb::dbTechLayerSpacingTablePrlRule* rule_;
};

// ADJACENTCUTS
class frCutSpacingConstraint : public frConstraint {
 public:
  // constructor
  frCutSpacingConstraint() {}
  frCutSpacingConstraint(odb::dbTechLayerSpacingRule* ruleIn,
                         int twoCutsIn = -1) {
    rule_ = ruleIn;
    twoCuts = twoCutsIn;
  }
  // getter
  odb::dbTechLayerSpacingRule* getbTechLayerSpacingRule() const {
    return rule_;
  }
  bool hasCenterToCenter() const { return rule_->getCutCenterToCenter(); }
  bool hasSameNet() const { return rule_->getCutSameNet(); }
  frCutSpacingConstraint* getSameNetConstraint() { return sameNetConstraint; }
  bool hasStack() const { return rule_->getCutStacking(); }
  bool isLayer() const {
    odb::dbTechLayer* outly;
    frString secondLayerName = "";
    if (rule_->getCutLayer4Spacing(outly))
      secondLayerName = outly->getName();
    return !(secondLayerName.empty());
  }
  const frString getSecondLayerName() const {
    odb::dbTechLayer* outly;
    if (rule_->getCutLayer4Spacing(outly))
      return outly->getName();
    return "";
  }
  bool hasSecondLayer() const { return (secondLayerNum != -1); }
  frLayerNum getSecondLayerNum() const { return secondLayerNum; }
  bool isAdjacentCuts() const {
    frUInt4 within, spacing;
    bool except_same_pgnet;
    frUInt4 adjacentCuts = 0;
    return (rule_->getAdjacentCuts(
               adjacentCuts, within, spacing, except_same_pgnet))
               ? (adjacentCuts != 0)
               : false;
  }
  int getAdjacentCuts() const {
    frUInt4 within, spacing;
    bool except_same_pgnet;
    frUInt4 adjacentCuts = 0;
    if (rule_->getAdjacentCuts(
            adjacentCuts, within, spacing, except_same_pgnet))
      return (adjacentCuts == 0) ? -1 : adjacentCuts;
    return -1;
  }
  frCoord getCutWithin() const {
    frUInt4 within = 0;
    frUInt4 spacing;
    bool except_same_pgnet;
    frUInt4 adjacentCuts;
    if (rule_->getAdjacentCuts(
            adjacentCuts, within, spacing, except_same_pgnet))
      return (within == 0) ? -1 : within;
    return -1;
  }
  bool hasExceptSamePGNet() const { return rule_->getSameNetPgOnly(); }
  bool getExceptSamePGNet() const { return rule_->getSameNetPgOnly(); }
  bool isParallelOverlap() const { return rule_->getCutParallelOverlap(); }
  bool getParallelOverlap() const { return rule_->getCutParallelOverlap(); }
  bool isArea() const { return !(rule_->getCutArea() == 0); }
  frCoord getCutArea() const {
    return (rule_->getCutArea() == 0) ? -1 : rule_->getCutArea();
  }
  frCoord getCutSpacing() const {
    return (rule_ != nullptr) ? rule_->getSpacing() : -1;
  }
  frConstraintTypeEnum typeId() const override {
    return frConstraintTypeEnum::frcCutSpacingConstraint;
  }
  // LEF58 related
  bool isTwoCuts() const { return (twoCuts == -1); }
  int getTwoCuts() const { return twoCuts; }

  void report(utl::Logger* logger) const override {
    logger->report("Cut Spacing");
  }

  // setter
  // void setDbTechLayerSpacingRule(odb::dbTechLayerSpacingRuleIn *ruleIn) {
  // rule_ = ruleIn; }
  void setSecondLayerNum(int secondLayerNumIn) {
    secondLayerNum = secondLayerNumIn;
  }

  void setSameNetConstraint(frCutSpacingConstraint* in) {
    sameNetConstraint = in;
  }

 protected:
  odb::dbTechLayerSpacingRule* rule_ = nullptr;
  frCutSpacingConstraint* sameNetConstraint = nullptr;
  frLayerNum secondLayerNum = -1;
  // LEF58 related
  int twoCuts = -1;
};

// LEF58_SPACING for cut layer (new)
class frLef58CutSpacingConstraint : public frConstraint {
 public:
  // constructor
  frLef58CutSpacingConstraint()
      : rule_(nullptr),
        maxXY(false),
        secondLayerNum(-1),
        cutClassIdx(-1),
        cutWithin1(-1),
        cutWithin2(-1),
        exceptAllWithin(-1) {}
  // getters
  // is == what rules have; has == what derived from values
  odb::dbTechLayerCutSpacingRule* getDbTechLayerCutSpacingRule() const {
    return rule_;
  }
  frCoord getCutSpacing() const { return rule_->getCutSpacing(); }
  bool isSameMask() const { return rule_->isSameMask(); }
  bool isMaxXY() const { return maxXY; }
  bool isCenterToCenter() const { return rule_->isCenterToCenter(); }
  bool isSameNet() const { return rule_->isSameNet(); }
  bool isSameMetal() const { return rule_->isSameMetal(); }
  bool isSameVia() const { return rule_->isSameVia(); }
  std::string getSecondLayerName() const {
    return rule_->getSecondLayer()->getName();
  }
  bool hasSecondLayer() const {
    return (secondLayerNum != -1 || rule_->getSecondLayer()->getName() != "");
  }
  frLayerNum getSecondLayerNum() const { return secondLayerNum; }
  bool isStack() const { return rule_->isStack(); }
  bool hasOrthogonalSpacing() const {
    return rule_->isOrthogonalSpacingValid();
  }
  int getOrthogonalSpacing() const {
    return (rule_->isOrthogonalSpacingValid()) ? rule_->getOrthogonalSpacing()
                                               : -1;
  }
  std::string getCutClassName() const {
    return (rule_->getCutClass() != nullptr) ? rule_->getCutClass()->getName()
                                             : "";
  }
  bool hasCutClass() const {
    return (cutClassIdx != -1 || getCutClassName() != "");
  }
  int getCutClassIdx() const { return cutClassIdx; }
  bool isShortEdgeOnly() const {
    return (rule_->getCutClass() != nullptr) ? rule_->isShortEdgeOnly() : false;
  }
  bool hasPrl() const { return rule_->isPrlValid(); }
  frCoord getPrl() const {
    return (rule_->getCutClass() != nullptr && rule_->isPrlValid())
               ? rule_->getPrl()
               : -1;
  }
  bool isConcaveCorner() const { return rule_->isConcaveCorner(); }
  bool hasWidth() const { return rule_->isConcaveCornerWidth(); }
  frCoord getWidth() const {
    return (rule_->getCutClass() != nullptr && rule_->isConcaveCornerWidth())
               ? rule_->getWidth()
               : (rule_->getCutClass() != nullptr && rule_->isAboveWidthValid())
                     ? rule_->getAboveWidth()
                     : -1;
  }
  bool hasEnclosure() const { return (rule_->getEnclosure() != -1); }
  frCoord getEnclosure() const {
    return (rule_->getCutClass() != nullptr
            && (rule_->isConcaveCornerWidth()
                || rule_->isConcaveCornerParallel()))
               ? rule_->getEnclosure()
               : (rule_->getCutClass() != nullptr
                  && rule_->isAboveWidthEnclosureValid())
                     ? rule_->getAboveEnclosure()
                     : -1;
  }
  bool hasEdgeLength() const { return (rule_->getEdgeLength() != -1); }
  frCoord getEdgeLength() const {
    return (rule_->getCutClass() != nullptr
            && (rule_->isConcaveCornerWidth()
                || rule_->isConcaveCornerEdgeLength()))
               ? rule_->getEdgeLength()
               : -1;
  }
  bool hasParallel() const { return rule_->isConcaveCornerParallel(); }
  frCoord getParlength() const {
    return (rule_->getCutClass() != nullptr && rule_->isConcaveCornerParallel())
               ? rule_->getParLength()
               : -1;
  }
  frCoord getParWithin() const {
    return (rule_->getCutClass() != nullptr && rule_->isConcaveCornerParallel())
               ? rule_->getParWithin()
               : -1;
  }
  frCoord getEdgeEnclosure() const {
    return (rule_->getCutClass() != nullptr
            && rule_->isConcaveCornerEdgeLength())
               ? rule_->getEdgeEnclosure()
               : -1;
  }
  frCoord getAdjEnclosure() const {
    return (rule_->getCutClass() != nullptr
            && rule_->isConcaveCornerEdgeLength())
               ? rule_->getAdjEnclosure()
               : -1;
  }
  bool hasExtension() const { return rule_->isExtensionValid(); }
  frCoord getExtension() const {
    return (rule_->getCutClass() != nullptr && rule_->isExtensionValid())
               ? rule_->getExtension()
               : -1;
  }
  bool hasNonEolConvexCorner() const { return rule_->isNonEolConvexCorner(); }
  frCoord getEolWidth() const {
    return (rule_->getCutClass() != nullptr && rule_->isNonEolConvexCorner())
               ? rule_->getEolWidth()
               : -1;
  }
  bool hasMinLength() const { return rule_->isMinLengthValid(); }
  frCoord getMinLength() const {
    return (rule_->getCutClass() != nullptr && rule_->isMinLengthValid())
               ? rule_->getMinLength()
               : -1;
  }
  bool hasAboveWidth() const { return rule_->isAboveWidthValid(); }
  int getAboveWidth() const {
    return (rule_->getCutClass() != nullptr && rule_->isAboveWidthValid())
               ? rule_->getAboveWidth()
               : -1;
  }
  bool hasAboveWidthEnclosure() const {
    return rule_->isAboveWidthEnclosureValid();
  }
  int getAboveEnclosure() const { return rule_->getAboveEnclosure(); }
  bool isMaskOverlap() const {
    return (rule_->getCutClass() != nullptr) ? rule_->isMaskOverlap() : false;
  }
  bool isWrongDirection() const {
    return (rule_->getCutClass() != nullptr) ? rule_->isWrongDirection()
                                             : false;
  }
  bool hasAdjacentCuts() const { return (rule_->getAdjacentCuts() != -1); }
  int getAdjacentCuts() const { return rule_->getAdjacentCuts(); }
  bool hasExactAligned() const { return rule_->isExactAligned(); }
  int getExactAligned() const {
    return (rule_->isExactAligned()) ? rule_->getNumCuts() : -1;
  }
  bool hasTwoCuts() const { return rule_->isTwoCutsValid(); }
  int getTwoCuts() const {
    return (rule_->isTwoCutsValid()) ? rule_->getTwoCuts() : -1;
  }
  bool hasTwoCutsSpacing() const { return (rule_->getTwoCuts() != -1); }
  frCoord getTwoCutsSpacing() const { return rule_->getTwoCuts(); }
  bool isSameCut() const { return rule_->isSameCut(); }
  // cutWithin2 is always used as upper bound
  bool hasTwoCutWithin() const { return (cutWithin1 != -1); }
  frCoord getCutWithin() const { return rule_->getWithin(); }
  frCoord getCutWithin1() const { return cutWithin1; }
  frCoord getCutWithin2() const { return cutWithin2; }
  bool isExceptSamePGNet() const { return rule_->isExceptSamePgnet(); }
  bool hasExceptAllWithin() const { return (exceptAllWithin != -1); }
  frCoord getExceptAllWithin() const { return exceptAllWithin; }
  bool isAbove() const { return rule_->isAbove(); }
  bool isBelow() const { return rule_->isBelow(); }
  bool isToAll() const {
    return (rule_->getCutClass() != nullptr) ? rule_->isCutClassToAll() : false;
  }
  bool isNoPrl() const { return rule_->isNoPrl(); }
  bool isSideParallelOverlap() const { return rule_->isSideParallelOverlap(); }
  bool isParallelOverlap() const { return false; }
  bool isExceptSameNet() const { return rule_->isExceptSameNet(); }
  bool isExceptSameMetal() const { return rule_->isExceptSameMetal(); }
  bool isExceptSameMetalOverlap() const {
    return rule_->isExceptSameMetalOverlap();
  }
  bool isExceptSameVia() const { return rule_->isExceptSameVia(); }
  bool hasParallelWithin() const { return (rule_->getParWithin() != -1); }
  frCoord getWithin() const { return rule_->getWithin(); }
  bool isLongEdgeOnly() const { return rule_->isLongEdgeOnly(); }
  bool hasSameMetalSharedEdge() const { return (rule_->getParWithin() != -1); }
  bool isExceptTwoEdges() const { return rule_->isExceptTwoEdges(); }
  bool hasExceptSameVia() const { return rule_->isExceptSameVia(); }
  bool hasArea() const { return (rule_->getCutArea() != -1); }
  frCoord getCutArea() const { return rule_->getCutArea(); }
  // setters
  void setDbTechLayerCutSpacingRule(odb::dbTechLayerCutSpacingRule* ruleIn) {
    rule_ = ruleIn;
  }
  void setMaxXY(bool in) { maxXY = in; }
  void setSecondLayerNum(frLayerNum in) { secondLayerNum = in; }
  void setCutClassIdx(int in) { cutClassIdx = in; }
  void setCutWithin1(frCoord in) { cutWithin1 = in; }
  void setCutWithin2(frCoord in) { cutWithin2 = in; }
  void setExceptAllWithin(frCoord in) { exceptAllWithin = in; }

  // others
  frConstraintTypeEnum typeId() const override {
    return frConstraintTypeEnum::frcLef58CutSpacingConstraint;
  }

  void report(utl::Logger* logger) const override {
    logger->report(
        "CUTSPACING cutSpacing {} sameMask {} maxXY {} centerToCenter {} "
        "sameNet {} sameMetal {} sameVia {} secondLayerName {} secondLayerNum "
        "{} stack {} orthogonalSpacing {} cutClassName {} cutClassIdx {} "
        "shortEdgeOnly {} prl {} concaveCorner {} width {} enclosure {} "
        "edgeLength {} parLength {} parWithin {} edgeEnclosure {} adjEnclosure "
        "{} extension {} eolWidth {} minLength {} maskOverlap {} "
        "wrongDirection {} adjacentCuts {} exactAlignedCut {} twoCuts {} "
        "twoCutsSpacing {} sameCut {} cutWithin1 {} cutWithin2 {} "
        "exceptSamePGNet {} exceptAllWithin {} above {} below {} toAll {} "
        "noPrl {} sideParallelOverlap {} parallelOverlap {} exceptSameNet {} "
        "exceptSameMetal {} exceptSameMetalOverlap {} exceptSameVia {} within "
        "{} longEdgeOnly {} exceptTwoEdges {} numCut {} cutArea {} ",
        getCutSpacing(),
        isSameMask(),
        maxXY,
        isCenterToCenter(),
        isSameNet(),
        isSameMetal(),
        isSameVia(),
        getSecondLayerName(),
        secondLayerNum,
        isStack(),
        getOrthogonalSpacing(),
        getCutClassName(),
        cutClassIdx,
        isShortEdgeOnly(),
        getPrl(),
        isConcaveCorner(),
        getWidth(),
        getEnclosure(),
        getEdgeLength(),
        getParlength(),
        getParWithin(),
        getEdgeEnclosure(),
        getAdjEnclosure(),
        getExtension(),
        getEolWidth(),
        getMinLength(),
        isMaskOverlap(),
        isWrongDirection(),
        getAdjacentCuts(),
        getExactAligned(),
        getTwoCuts(),
        getTwoCutsSpacing(),
        isSameCut(),
        cutWithin1,
        cutWithin2,
        isExceptSamePGNet(),
        exceptAllWithin,
        isAbove(),
        isBelow(),
        isToAll(),
        isNoPrl(),
        isSideParallelOverlap(),
        isParallelOverlap(),
        isExceptSameNet(),
        isExceptSameMetal(),
        isExceptSameMetalOverlap(),
        isExceptSameVia(),
        getWithin(),
        isLongEdgeOnly(),
        isExceptTwoEdges(),
        rule_->getNumCuts(),
        getCutArea());
  }

 protected:
  odb::dbTechLayerCutSpacingRule* rule_;
  bool maxXY;
  frLayerNum secondLayerNum;
  int cutClassIdx;
  frCoord cutWithin1;
  frCoord cutWithin2;
  frCoord exceptAllWithin;
};

// LEF58_CORNERSPACING (new)
class frLef58CornerSpacingConstraint : public frConstraint {
 public:
  // constructor
  frLef58CornerSpacingConstraint(
      const fr1DLookupTbl<frCoord, std::pair<frCoord, frCoord>>& tblIn)
      : rule_(nullptr), tbl(tblIn), sameXY(false), cornerToCorner(false) {}

  // getters
  odb::dbTechLayerCornerSpacingRule* getDbTechLayerCornerSpacingRule() const {
    return rule_;
  }
  frConstraintTypeEnum typeId() const override {
    return frConstraintTypeEnum::frcLef58CornerSpacingConstraint;
  }
  frCornerTypeEnum getCornerType() const {
    return (rule_->getType()
            == odb::dbTechLayerCornerSpacingRule::CornerType::CONVEXCORNER)
               ? frCornerTypeEnum::CONVEX
               : frCornerTypeEnum::CONCAVE;
  }
  bool getSameMask() const {
    return (rule_->getType()
            == odb::dbTechLayerCornerSpacingRule::CornerType::CONVEXCORNER)
               ? rule_->isSameMask()
               : false;
  }
  bool hasCornerOnly() const { return rule_->isCornerOnly(); }
  frCoord getWithin() const {
    return (rule_->getType()
                == odb::dbTechLayerCornerSpacingRule::CornerType::CONVEXCORNER
            && rule_->isCornerOnly())
               ? rule_->getWithin()
               : -1;
  }
  bool hasExceptEol() const { return rule_->isExceptEol(); }
  frCoord getEolWidth() const {
    return (rule_->getType()
                == odb::dbTechLayerCornerSpacingRule::CornerType::CONVEXCORNER
            && rule_->isExceptEol())
               ? rule_->getEolWidth()
               : -1;
  }
  bool hasExceptJogLength() const { return rule_->isExceptJogLength(); }
  frCoord getLength() const {
    return (rule_->getType()
                == odb::dbTechLayerCornerSpacingRule::CornerType::CONVEXCORNER
            && rule_->isExceptJogLength() && rule_->isExceptEol())
               ? rule_->getJogLength()
               : -1;
  }
  bool hasEdgeLength() const {
    return (rule_->getType()
                == odb::dbTechLayerCornerSpacingRule::CornerType::CONVEXCORNER
            && rule_->isExceptJogLength() && rule_->isExceptEol())
               ? rule_->isEdgeLengthValid()
               : false;
  }
  bool getEdgeLength() const { return rule_->isEdgeLengthValid(); }
  bool hasIncludeLShape() const {
    return (rule_->getType()
                == odb::dbTechLayerCornerSpacingRule::CornerType::CONVEXCORNER
            && rule_->isExceptJogLength() && rule_->isExceptEol())
               ? rule_->isIncludeShape()
               : false;
  }
  bool getIncludeLShape() const {
    return (rule_->getType()
                == odb::dbTechLayerCornerSpacingRule::CornerType::CONVEXCORNER
            && rule_->isExceptJogLength() && rule_->isExceptEol())
               ? rule_->isIncludeShape()
               : false;
  }
  frCoord getMinLength() const {
    return (rule_->getType()
                != odb::dbTechLayerCornerSpacingRule::CornerType::CONVEXCORNER
            && rule_->isMinLengthValid())
               ? rule_->getMinLength()
               : -1;
  }
  bool hasExceptNotch() const {
    return (rule_->getType()
            != odb::dbTechLayerCornerSpacingRule::CornerType::CONVEXCORNER)
               ? rule_->isExceptNotch()
               : false;
  }
  bool getExceptNotch() const {
    return (rule_->getType()
            != odb::dbTechLayerCornerSpacingRule::CornerType::CONVEXCORNER)
               ? rule_->isExceptNotch()
               : false;
  }
  frCoord getNotchLength() const {
    return (rule_->getType()
                != odb::dbTechLayerCornerSpacingRule::CornerType::CONVEXCORNER
            && rule_->isExceptNotchLengthValid())
               ? rule_->getExceptNotchLength()
               : -1;
  }
  bool hasExceptSameNet() const { return rule_->isExceptSameNet(); }
  bool getExceptSameNet() const { return rule_->isExceptSameNet(); }
  bool hasExceptSameMetal() const { return rule_->isExceptSameMetal(); }
  bool getExceptSameMetal() const { return rule_->isExceptSameMetal(); }
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
  bool hasSameXY() const { return sameXY; }
  bool getSameXY() const { return sameXY; }
  bool isCornerToCorner() const { return cornerToCorner; }

  // setters
  void setDbTechLayerCornerSpacingRule(
      odb::dbTechLayerCornerSpacingRule* ruleIn) {
    rule_ = ruleIn;
  }
  void setLookupTbl(
      const fr1DLookupTbl<frCoord, std::pair<frCoord, frCoord>>& in) {
    tbl = in;
  }
  void setSameXY(bool in) { sameXY = in; }
  void setCornerToCorner(bool in) { cornerToCorner = in; }
  void report(utl::Logger* logger) const override {
    logger->report(
        "CORNERSPACING cornerType {} sameMask {} within {} eolWidth {} length "
        "{} edgeLength {} includeLShape {} minLength {} exceptNotch {} "
        "notchLength {} exceptSameNet {} exceptSameMetal {} sameXY {} "
        "cornerToCorner {}",
        getCornerType(),
        getSameMask(),
        getWithin(),
        getEolWidth(),
        getLength(),
        getEdgeLength(),
        getIncludeLShape(),
        getMinLength(),
        getExceptNotch(),
        getNotchLength(),
        getExceptSameNet(),
        getExceptSameMetal(),
        sameXY,
        cornerToCorner);

    std::string vals = "";
    std::string rows = "";
    for (auto row : tbl.getRows())
      rows = rows + std::to_string(row) + " ";
    for (auto val : tbl.getValues())
      vals = vals + "(" + std::to_string(val.first) + ","
             + std::to_string(val.second) + ") ";
    logger->report("\trowName: {}", tbl.getRowName());
    logger->report("\trows: {}", rows);
    logger->report("\tvals: {}", vals);
  }

 protected:
  odb::dbTechLayerCornerSpacingRule* rule_;
  fr1DLookupTbl<frCoord, std::pair<frCoord, frCoord>>
      tbl;      // horz / vert spacing
  bool sameXY;  // indicate whether horz spacing == vert spacing // for write
                // LEF some day
  bool cornerToCorner;
};

class frLef58CornerSpacingSpacingConstraint : public frConstraint {
 public:
  // constructor
  frLef58CornerSpacingSpacingConstraint(frCoord widthIn) : width(widthIn) {}
  // getter
  frCoord getWidth() { return width; }
  // setter
  void setWidth(frCoord widthIn) { width = widthIn; }

  frConstraintTypeEnum typeId() const override {
    return frConstraintTypeEnum::frcLef58CornerSpacingSpacingConstraint;
  }
  void report(utl::Logger* logger) const override {
    logger->report("58 Corner spacing spacing {}", width);
  }

 protected:
  frCoord width;
};

class frLef58CornerSpacingSpacing1DConstraint
    : public frLef58CornerSpacingSpacingConstraint {
 public:
  // constructor
  frLef58CornerSpacingSpacing1DConstraint(frCoord widthIn, frCoord spacingIn)
      : frLef58CornerSpacingSpacingConstraint(widthIn), spacing(spacingIn) {}
  // getter
  frCoord getSpacing() { return spacing; }
  // setter
  void setSpacing(frCoord spacingIn) { spacing = spacingIn; }

  frConstraintTypeEnum typeId() const override {
    return frConstraintTypeEnum::frcLef58CornerSpacingSpacing1DConstraint;
  }

  void report(utl::Logger* logger) const override {
    logger->report("58 Corner spacing 1D {}", spacing);
  }

 protected:
  frCoord spacing = -1;
};

class frLef58CornerSpacingSpacing2DConstraint
    : public frLef58CornerSpacingSpacingConstraint {
 public:
  // constructor
  frLef58CornerSpacingSpacing2DConstraint(frCoord widthIn,
                                          frCoord horizontalSpacingIn,
                                          frCoord verticalSpacingIn)
      : frLef58CornerSpacingSpacingConstraint(widthIn),
        horizontalSpacing(horizontalSpacingIn),
        verticalSpacing(verticalSpacingIn) {}
  // getter
  frCoord getHorizontalSpacing() { return horizontalSpacing; }
  frCoord getVerticalSpacing() { return verticalSpacing; }
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
  void report(utl::Logger* logger) const override {
    logger->report("58 Corner spacing spacing 2D h {} v {}",
                   horizontalSpacing,
                   verticalSpacing);
  }

 protected:
  frCoord horizontalSpacing = -1, verticalSpacing = -1;
};

class frLef58RectOnlyConstraint : public frConstraint {
 public:
  // constructor
  frLef58RectOnlyConstraint(bool exceptNonCorePinsIn = false)
      : exceptNonCorePins(exceptNonCorePinsIn) {}
  // getter
  bool isExceptNonCorePinsIn() { return exceptNonCorePins; }
  // setter
  void setExceptNonCorePins(bool exceptNonCorePinsIn) {
    exceptNonCorePins = exceptNonCorePinsIn;
  }

  frConstraintTypeEnum typeId() const override {
    return frConstraintTypeEnum::frcLef58RectOnlyConstraint;
  }
  void report(utl::Logger* logger) const override {
    logger->report("RECTONLY exceptNonCorePins {}", exceptNonCorePins);
  }

 protected:
  bool exceptNonCorePins;
};

class frLef58RightWayOnGridOnlyConstraint : public frConstraint {
 public:
  // constructor
  frLef58RightWayOnGridOnlyConstraint(bool checkMaskIn = false)
      : checkMask(checkMaskIn) {}
  // getter
  bool isCheckMask() { return checkMask; }
  // setter
  void setCheckMask(bool checkMaskIn) { checkMask = checkMaskIn; }

  frConstraintTypeEnum typeId() const override {
    return frConstraintTypeEnum::frcLef58RightWayOnGridOnlyConstraint;
  }
  void report(utl::Logger* logger) const override {
    logger->report("RIGHTWAYONGRIDONLY checkMask {}", checkMask);
  }

 protected:
  bool checkMask;
};

using namespace std;
class frNonDefaultRule {
  friend class FlexRP;
  friend class frTechObject;

 private:
  string name_;
  // each vector position is a metal layer
  vector<frCoord> widths_;
  vector<frCoord> spacings_;
  vector<frCoord> wireExtensions_;
  vector<drEolSpacingConstraint> drEolCons_;
  vector<int> minCuts_;  // min cuts per cut layer

  // vias for each layer
  vector<vector<frViaDef*>> vias_;
  vector<vector<frViaRuleGenerate*>> viasRules_;

  bool hardSpacing_ = false;

  // See comments in frTechObject's equivalent fields for the meaning
  std::vector<std::array<ForbiddenRanges, 8>> via2ViaForbiddenLen;
  std::vector<std::array<ForbiddenRanges, 4>> viaForbiddenTurnLen;

 public:
  frViaDef* getPrefVia(int z) {
    if (z >= (int) vias_.size() || vias_[z].empty()) {
      return nullptr;
    }
    return vias_[z][0];
  }

  void setWidth(frCoord w, int z) {
    if (z >= (int) widths_.size()) {
      widths_.resize(z + 1, 0);
    }
    widths_[z] = w;
  }

  void setSpacing(frCoord s, int z) {
    if (z >= (int) spacings_.size()) {
      spacings_.resize(z + 1, 0);
    }
    spacings_[z] = s;
  }

  void setMinCuts(int ncuts, int z) {
    if (z >= (int) minCuts_.size()) {
      minCuts_.resize(z + 1, 1);
    }
    minCuts_[z] = ncuts;
  }

  void setWireExtension(frCoord we, int z) {
    if (z >= (int) wireExtensions_.size()) {
      wireExtensions_.resize(z + 1, 0);
    }
    wireExtensions_[z] = we;
  }

  void setDrEolConstraint(drEolSpacingConstraint con, int z) {
    if (z >= (int) drEolCons_.size()) {
      drEolCons_.resize(z + 1, 0);
    }
    drEolCons_[z] = con;
  }

  void addVia(frViaDef* via, int z) {
    if (z >= (int) vias_.size()) {
      vias_.resize(z + 1, vector<frViaDef*>());
    }
    vias_[z].push_back(via);
  }

  void addViaRule(frViaRuleGenerate* via, int z) {
    if (z >= (int) viasRules_.size()) {
      viasRules_.resize(z + 1, vector<frViaRuleGenerate*>());
    }
    viasRules_[z].push_back(via);
  }

  void setHardSpacing(bool isHard) { hardSpacing_ = isHard; }

  void setName(const char* n) { name_ = string(n); }

  string getName() const { return name_; }

  frCoord getWidth(int z) const {
    if (z >= (int) widths_.size()) {
      return 0;
    }
    return widths_[z];
  }

  frCoord getSpacing(int z) const {
    if (z >= (int) spacings_.size()) {
      return 0;
    }
    return spacings_[z];
  }

  frCoord getWireExtension(int z) const {
    if (z >= (int) wireExtensions_.size()) {
      return 0;
    }
    return wireExtensions_[z];
  }

  drEolSpacingConstraint getDrEolSpacingConstraint(int z) const {
    if (z >= (int) drEolCons_.size()) {
      return drEolSpacingConstraint();
    }
    return drEolCons_[z];
  }

  int getMinCuts(int z) const {
    if (z >= (int) minCuts_.size()) {
      return 1;
    }
    return minCuts_[z];
  }

  const vector<frViaDef*>& getVias(int z) const { return vias_[z]; }

  const vector<frViaRuleGenerate*>& getViaRules(int z) const {
    return viasRules_[z];
  }

  bool isHardSpacing() const { return hardSpacing_; }
};
}  // namespace fr

#endif
