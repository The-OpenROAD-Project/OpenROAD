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

namespace drt {
class frLayer;
namespace io {
class Parser;
}

using ForbiddenRanges = std::vector<std::pair<frCoord, frCoord>>;

enum class frLef58CornerSpacingExceptionEnum
{
  NONE,
  EXCEPTSAMENET,
  EXCEPTSAMEMETAL
};

struct drEolSpacingConstraint
{
  drEolSpacingConstraint(frCoord width = 0,
                         frCoord space = 0,
                         frCoord within = 0)
      : eolWidth(width), eolSpace(space), eolWithin(within)
  {
  }
  frCoord eolWidth;
  frCoord eolSpace;
  frCoord eolWithin;
};

// base type for design rule
class frConstraint
{
 public:
  virtual ~frConstraint() = default;
  virtual frConstraintTypeEnum typeId() const = 0;
  virtual void report(utl::Logger* logger) const = 0;
  void setLayer(frLayer* layer) { layer_ = layer; }
  void setId(int in) { id_ = in; }
  int getId() const { return id_; }
  std::string getViolName() const;

 protected:
  int id_{-1};
  frLayer* layer_{nullptr};
};

class frLef58CutClassConstraint : public frConstraint
{
 public:
  // getters
  frCollection<std::shared_ptr<frLef58CutClass>> getCutClasses() const
  {
    frCollection<std::shared_ptr<frLef58CutClass>> sol;
    std::transform(cutClasses_.begin(),
                   cutClasses_.end(),
                   std::back_inserter(sol),
                   [](auto& kv) { return kv.second; });
    return sol;
  }
  // setters
  void addToCutClass(const std::shared_ptr<frLef58CutClass>& in)
  {
    cutClasses_[in->getName()] = in;
  }
  // others
  frConstraintTypeEnum typeId() const override
  {
    return frConstraintTypeEnum::frcLef58CutClassConstraint;
  }
  void report(utl::Logger* logger) const override
  {
    logger->report("Cut class");
  }

 private:
  std::map<frString, std::shared_ptr<frLef58CutClass>> cutClasses_;

  friend class io::Parser;
};

// recheck constraint for negative rules
class frRecheckConstraint : public frConstraint
{
 public:
  frConstraintTypeEnum typeId() const override
  {
    return frConstraintTypeEnum::frcRecheckConstraint;
  }
  void report(utl::Logger* logger) const override { logger->report("Recheck"); }
};

// short

class frShortConstraint : public frConstraint
{
 public:
  frConstraintTypeEnum typeId() const override
  {
    return frConstraintTypeEnum::frcShortConstraint;
  }
  void report(utl::Logger* logger) const override { logger->report("Short"); }
};

// NSMetal
class frNonSufficientMetalConstraint : public frConstraint
{
 public:
  frConstraintTypeEnum typeId() const override
  {
    return frConstraintTypeEnum::frcNonSufficientMetalConstraint;
  }
  void report(utl::Logger* logger) const override { logger->report("NSMetal"); }
};

// offGrid
class frOffGridConstraint : public frConstraint
{
 public:
  frConstraintTypeEnum typeId() const override
  {
    return frConstraintTypeEnum::frcOffGridConstraint;
  }
  void report(utl::Logger* logger) const override
  {
    logger->report("Off grid");
  }
};

// minHole
class frMinEnclosedAreaConstraint : public frConstraint
{
 public:
  // constructor
  frMinEnclosedAreaConstraint(frCoord areaIn) : area_(areaIn) {}
  // getter
  frCoord getArea() const { return area_; }
  bool hasWidth() const { return (width_ != -1); }
  frCoord getWidth() const { return width_; }
  // setter
  void setWidth(frCoord widthIn) { width_ = widthIn; }

  frConstraintTypeEnum typeId() const override
  {
    return frConstraintTypeEnum::frcMinEnclosedAreaConstraint;
  }

  void report(utl::Logger* logger) const override
  {
    logger->report("Min enclosed area {} width {}", area_, width_);
  }

 protected:
  frCoord area_;
  frCoord width_{-1};
};

// LEF58_MINSTEP (currently only implement GF14 related API)
class frLef58MinStepConstraint : public frConstraint
{
 public:
  // getter
  frCoord getMinStepLength() const { return minStepLength_; }
  bool hasMaxEdges() const { return (maxEdges_ != -1); }
  int getMaxEdges() const { return maxEdges_; }
  bool hasMinAdjacentLength() const { return (minAdjLength_ != -1); }
  frCoord getMinAdjacentLength() const { return minAdjLength_; }
  bool hasEolWidth() const { return (eolWidth_ != -1); }
  frCoord getEolWidth() const { return eolWidth_; }
  frCoord getNoAdjEol() const { return noAdjEol_; }
  bool isExceptRectangle() const { return exceptRectangle_; }

  // setter
  void setMinStepLength(frCoord in) { minStepLength_ = in; }
  void setMaxEdges(int in) { maxEdges_ = in; }
  void setMinAdjacentLength(frCoord in) { minAdjLength_ = in; }
  void setEolWidth(frCoord in) { eolWidth_ = in; }

  frConstraintTypeEnum typeId() const override
  {
    return frConstraintTypeEnum::frcLef58MinStepConstraint;
  }
  void report(utl::Logger* logger) const override;
  void setNoAdjEol(frCoord value) { noAdjEol_ = value; }
  void setExceptRectangle(bool value) { exceptRectangle_ = value; }

 protected:
  frCoord minStepLength_{-1};
  bool insideCorner_{false};
  bool outsideCorner_{false};
  bool step_{false};
  frCoord maxLength_{-1};
  int maxEdges_{-1};
  frCoord minAdjLength_{-1};
  bool convexCorner_{false};
  frCoord exceptWithin_{-1};
  bool concaveCorner_{false};
  bool threeConcaveCorners_{false};
  frCoord width_{-1};
  frCoord minAdjLength2_{-1};
  frCoord minBetweenLength_{-1};
  bool exceptSameCorners_{false};
  frCoord eolWidth_{-1};
  bool concaveCorners_{false};
  frCoord noAdjEol_{-1};
  bool exceptRectangle_{false};
};

// minStep
class frMinStepConstraint : public frConstraint
{
 public:
  // getter
  frCoord getMinStepLength() const { return minStepLength_; }
  bool hasMaxLength() const { return (maxLength_ != -1); }
  frCoord getMaxLength() const { return maxLength_; }
  bool hasMinstepType() const
  {
    return minstepType_ != frMinstepTypeEnum::UNKNOWN;
  }
  frMinstepTypeEnum getMinstepType() const { return minstepType_; }
  bool hasInsideCorner() const { return insideCorner_; }
  bool hasOutsideCorner() const { return outsideCorner_; }
  bool hasStep() const { return step_; }
  bool hasMaxEdges() const { return (maxEdges_ != -1); }
  int getMaxEdges() const { return maxEdges_; }
  // setter
  void setMinstepType(frMinstepTypeEnum in) { minstepType_ = in; }
  void setInsideCorner(bool in) { insideCorner_ = in; }
  void setOutsideCorner(bool in) { outsideCorner_ = in; }
  void setStep(bool in) { step_ = in; }
  void setMinStepLength(frCoord in) { minStepLength_ = in; }
  void setMaxLength(frCoord in) { maxLength_ = in; }
  void setMaxEdges(int in) { maxEdges_ = in; }

  frConstraintTypeEnum typeId() const override
  {
    return frConstraintTypeEnum::frcMinStepConstraint;
  }

  void report(utl::Logger* logger) const override
  {
    logger->report(
        "Min step length min {} type {} max {} "
        "insideCorner {} outsideCorner {} step {} maxEdges {}",
        minStepLength_,
        int(minstepType_),
        maxLength_,
        insideCorner_,
        outsideCorner_,
        step_,
        maxEdges_);
  }

 protected:
  frCoord minStepLength_{-1};
  frMinstepTypeEnum minstepType_{frMinstepTypeEnum::UNKNOWN};
  frCoord maxLength_{-1};
  bool insideCorner_{false};
  bool outsideCorner_{true};
  bool step_{false};
  int maxEdges_{-1};
};

// minimumcut
class frMinimumcutConstraint : public frConstraint
{
 public:
  // getters
  int getNumCuts() const { return numCuts_; }
  frCoord getWidth() const { return width_; }
  bool hasWithin() const { return !(cutDistance_ == -1); }
  frCoord getCutDistance() const { return cutDistance_; }
  bool hasConnection() const
  {
    return !(connection_ == frMinimumcutConnectionEnum::UNKNOWN);
  }
  frMinimumcutConnectionEnum getConnection() const { return connection_; }
  bool hasLength() const { return !(length_ == -1); }
  frCoord getLength() const { return length_; }
  frCoord getDistance() const { return distance_; }
  // setters
  void setNumCuts(int in) { numCuts_ = in; }
  void setWidth(frCoord in) { width_ = in; }
  void setWithin(frCoord in) { cutDistance_ = in; }
  void setConnection(frMinimumcutConnectionEnum in) { connection_ = in; }
  void setLength(frCoord in1, frCoord in2)
  {
    length_ = in1;
    distance_ = in2;
  }
  // others
  frConstraintTypeEnum typeId() const override
  {
    return frConstraintTypeEnum::frcMinimumcutConstraint;
  }
  void report(utl::Logger* logger) const override
  {
    logger->report(
        "Min cut numCuts {} width {} cutDistance {} "
        "connection {} length {} distance {}",
        numCuts_,
        width_,
        cutDistance_,
        connection_,
        length_,
        distance_);
  }

 protected:
  int numCuts_{-1};
  frCoord width_{-1};
  frCoord cutDistance_{-1};
  frMinimumcutConnectionEnum connection_{frMinimumcutConnectionEnum::UNKNOWN};
  frCoord length_{-1};
  frCoord distance_{-1};
};

// LEF58_MINIMUMCUT
class frLef58MinimumcutConstraint : public frConstraint
{
 public:
  frLef58MinimumcutConstraint(odb::dbTechLayerMinCutRule* rule) : db_rule_(rule)
  {
  }
  // getter
  odb::dbTechLayerMinCutRule* getODBRule() const { return db_rule_; }
  // others
  frConstraintTypeEnum typeId() const override
  {
    return frConstraintTypeEnum::frcLef58MinimumCutConstraint;
  }
  void report(utl::Logger* logger) const override
  {
    logger->report("LEF58_MINIMUMCUT");
  }

 private:
  odb::dbTechLayerMinCutRule* db_rule_;
};

// minArea

class frAreaConstraint : public frConstraint
{
 public:
  // constructor
  frAreaConstraint(frCoord minAreaIn) : minArea(minAreaIn) {}
  // getter
  frCoord getMinArea() { return minArea; }
  // setter
  void setMinArea(frCoord minAreaIn) { minArea = minAreaIn; }

  frConstraintTypeEnum typeId() const override
  {
    return frConstraintTypeEnum::frcAreaConstraint;
  }
  void report(utl::Logger* logger) const override
  {
    logger->report("Area {}", minArea);
  }

 protected:
  frCoord minArea;
};

// minWidth
class frMinWidthConstraint : public frConstraint
{
 public:
  // constructor
  frMinWidthConstraint(frCoord minWidthIn) : minWidth(minWidthIn) {}
  // getter
  frCoord getMinWidth() { return minWidth; }
  // setter
  void set(frCoord minWidthIn) { minWidth = minWidthIn; }

  frConstraintTypeEnum typeId() const override
  {
    return frConstraintTypeEnum::frcMinWidthConstraint;
  }
  void report(utl::Logger* logger) const override
  {
    logger->report("Width {}", minWidth);
  }

 protected:
  frCoord minWidth;
};

class frOrthSpacingTableConstraint : public frConstraint
{
 public:
  frOrthSpacingTableConstraint(const std::vector<std::pair<int, int>>& spc_tbl)
      : spc_tbl_(spc_tbl)
  {
  }

  const std::vector<std::pair<int, int>>& getSpacingTable() const
  {
    return spc_tbl_;
  }
  frConstraintTypeEnum typeId() const override
  {
    return frConstraintTypeEnum::frcSpacingTableOrth;
  }
  void report(utl::Logger* logger) const override
  {
    logger->report("SPACINGTABLE ORTHOGONAL");
  }

 private:
  std::vector<std::pair<int, int>> spc_tbl_;
};

class frLef58SpacingEndOfLineWithinEncloseCutConstraint : public frConstraint
{
 public:
  // constructors
  frLef58SpacingEndOfLineWithinEncloseCutConstraint(frCoord encloseDistIn,
                                                    frCoord cutToMetalSpaceIn)
      : encloseDist(encloseDistIn), cutToMetalSpace(cutToMetalSpaceIn)
  {
  }
  // setters
  void setBelow(bool value) { below = value; }
  void setAbove(bool value) { above = value; }
  void setAllCuts(bool value) { allCuts = value; }
  void setEncloseDist(frCoord value) { encloseDist = value; }
  void setCutToMetalSpace(frCoord value) { cutToMetalSpace = value; }
  // getters
  bool isAboveOnly() const { return above; }
  bool isBelowOnly() const { return below; }
  bool isAboveAndBelow() const { return !(above ^ below); }
  bool isAllCuts() const { return allCuts; }
  frCoord getEncloseDist() const { return encloseDist; }
  frCoord getCutToMetalSpace() const { return cutToMetalSpace; }
  // others
  frConstraintTypeEnum typeId() const override
  {
    return frConstraintTypeEnum::
        frcLef58SpacingEndOfLineWithinEncloseCutConstraint;
  }
  void report(utl::Logger* logger) const override
  {
    logger->report(
        "\t\tSPACING_WITHIN_ENCLOSECUT below {} above {} encloseDist "
        "{} cutToMetalSpace {} allCuts {}",
        below,
        above,
        encloseDist,
        cutToMetalSpace,
        allCuts);
  }

 private:
  bool below{false};
  bool above{false};
  frCoord encloseDist;
  frCoord cutToMetalSpace;
  bool allCuts{false};
};

class frLef58SpacingEndOfLineWithinEndToEndConstraint : public frConstraint
{
 public:
  // getters
  frCoord getEndToEndSpace() const { return endToEndSpace_; }
  frCoord getOneCutSpace() const { return oneCutSpace_; }
  frCoord getTwoCutSpace() const { return twoCutSpace_; }
  bool hasExtension() const { return hExtension_; }
  frCoord getExtension() const { return extension_; }
  frCoord getWrongDirExtension() const { return wrongDirExtension_; }
  bool hasOtherEndWidth() const { return hOtherEndWidth_; }
  frCoord getOtherEndWidth() const { return otherEndWidth_; }

  // setters
  void setEndToEndSpace(frCoord in) { endToEndSpace_ = in; }
  void setCutSpace(frCoord one, frCoord two)
  {
    oneCutSpace_ = one;
    twoCutSpace_ = two;
  }
  void setExtension(frCoord extensionIn)
  {
    hExtension_ = true;
    extension_ = extensionIn;
    wrongDirExtension_ = extensionIn;
  }
  void setExtension(frCoord extensionIn, frCoord wrongDirExtensionIn)
  {
    hExtension_ = true;
    extension_ = extensionIn;
    wrongDirExtension_ = wrongDirExtensionIn;
  }
  void setOtherEndWidth(frCoord in)
  {
    hOtherEndWidth_ = true;
    otherEndWidth_ = in;
  }
  // others
  frConstraintTypeEnum typeId() const override
  {
    return frConstraintTypeEnum::
        frcLef58SpacingEndOfLineWithinEndToEndConstraint;
  }
  void report(utl::Logger* logger) const override
  {
    logger->report(
        "\t\tSPACING_WITHIN_ENDTOEND endToEndSpace {} cutSpace {} oneCutSpace "
        "{} twoCutSpace {} hExtension {} extension {} wrongDirExtension {} "
        "hOtherEndWidth {} otherEndWidth {} ",
        endToEndSpace_,
        cutSpace_,
        oneCutSpace_,
        twoCutSpace_,
        hExtension_,
        extension_,
        wrongDirExtension_,
        hOtherEndWidth_,
        otherEndWidth_);
  }

 protected:
  frCoord endToEndSpace_{0};
  bool cutSpace_{false};
  frCoord oneCutSpace_{0};
  frCoord twoCutSpace_{0};
  bool hExtension_{false};
  frCoord extension_{0};
  frCoord wrongDirExtension_{false};
  bool hOtherEndWidth_{false};
  frCoord otherEndWidth_{0};
};

class frLef58SpacingEndOfLineWithinParallelEdgeConstraint : public frConstraint
{
 public:
  // getters
  bool hasSubtractEolWidth() const { return subtractEolWidth_; }
  frCoord getParSpace() const { return parSpace_; }
  frCoord getParWithin() const { return parWithin_; }
  bool hasPrl() const { return hPrl_; }
  frCoord getPrl() const { return prl_; }
  bool hasMinLength() const { return hMinLength_; }
  frCoord getMinLength() const { return minLength_; }
  bool hasTwoEdges() const { return twoEdges_; }
  bool hasSameMetal() const { return sameMetal_; }
  bool hasNonEolCornerOnly() const { return nonEolCornerOnly_; }
  bool hasParallelSameMask() const { return parallelSameMask_; }
  // setters
  void setSubtractEolWidth(bool in) { subtractEolWidth_ = in; }
  void setPar(frCoord parSpaceIn, frCoord parWithinIn)
  {
    parSpace_ = parSpaceIn;
    parWithin_ = parWithinIn;
  }
  void setPrl(frCoord in)
  {
    hPrl_ = true;
    prl_ = in;
  }
  void setMinLength(frCoord in)
  {
    hMinLength_ = true;
    minLength_ = in;
  }
  void setTwoEdges(bool in) { twoEdges_ = in; }
  void setSameMetal(bool in) { sameMetal_ = in; }
  void setNonEolCornerOnly(bool in) { nonEolCornerOnly_ = in; }
  void setParallelSameMask(bool in) { parallelSameMask_ = in; }

  // others
  frConstraintTypeEnum typeId() const override
  {
    return frConstraintTypeEnum::
        frcLef58SpacingEndOfLineWithinParallelEdgeConstraint;
  }
  void report(utl::Logger* logger) const override
  {
    logger->report(
        "\t\tSPACING_WITHIN_PARALLELEDGE subtractEolWidth {} parSpace {} "
        "parWithin {} hPrl {} prl {} hMinLength {} minLength {} twoEdges {} "
        "sameMetal {} nonEolCornerOnly {} parallelSameMask {} ",
        subtractEolWidth_,
        parSpace_,
        parWithin_,
        hPrl_,
        prl_,
        hMinLength_,
        minLength_,
        twoEdges_,
        sameMetal_,
        nonEolCornerOnly_,
        parallelSameMask_);
  }

 protected:
  bool subtractEolWidth_{false};
  frCoord parSpace_{0};
  frCoord parWithin_{0};
  bool hPrl_{false};
  frCoord prl_{0};
  bool hMinLength_{false};
  frCoord minLength_{0};
  bool twoEdges_{false};
  bool sameMetal_{false};
  bool nonEolCornerOnly_{false};
  bool parallelSameMask_{false};
};

class frLef58SpacingEndOfLineWithinMaxMinLengthConstraint : public frConstraint
{
 public:
  // getters
  frCoord getLength() const { return length_; }
  bool isMaxLength() const { return maxLength_; }
  bool isTwoSides() const { return twoSides_; }

  // setters
  void setLength(bool maxLengthIn, frCoord lengthIn, bool twoSidesIn = false)
  {
    maxLength_ = maxLengthIn;
    length_ = lengthIn;
    twoSides_ = twoSidesIn;
  }
  // others
  frConstraintTypeEnum typeId() const override
  {
    return frConstraintTypeEnum::
        frcLef58SpacingEndOfLineWithinMaxMinLengthConstraint;
  }
  void report(utl::Logger* logger) const override
  {
    logger->report(
        "\t\tSPACING_WITHIN_MAXMIN maxLength {} length {} twoSides {} ",
        maxLength_,
        length_,
        twoSides_);
  }

 protected:
  bool maxLength_{false};
  frCoord length_{0};
  bool twoSides_{false};
};

class frLef58SpacingEndOfLineWithinConstraint : public frConstraint
{
 public:
  // getters
  bool hasOppositeWidth() const { return hOppositeWidth_; }
  frCoord getOppositeWidth() const { return oppositeWidth_; }
  frCoord getEolWithin() const { return sameMask_ ? 0 : eolWithin_; }
  frCoord getWrongDirWithin() const { return wrongDirWithin_; }
  frCoord getEndPrlSpacing() const { return endPrlSpacing_; }
  frCoord getEndPrl() const { return endPrl_; }
  bool hasSameMask() const { return sameMask_; }
  bool hasExceptExactWidth() const
  {
    return false;  // skip for now
  }
  bool hasFillConcaveCorner() const
  {
    return false;  // skip for now
  }
  bool hasWithCut() const
  {
    return false;  // skip for now
  }
  bool hasEndPrlSpacing() const { return endPrlSpacing_; }
  bool hasEndToEndConstraint() const
  {
    return (endToEndConstraint_) ? true : false;
  }
  std::shared_ptr<frLef58SpacingEndOfLineWithinEndToEndConstraint>
  getEndToEndConstraint() const
  {
    return endToEndConstraint_;
  }
  bool hasMinMaxLength() const
  {
    return false;  // skip for now
  }
  bool hasEqualRectWidth() const
  {
    return false;  // skip for now
  }
  bool hasParallelEdgeConstraint() const
  {
    return (parallelEdgeConstraint_) ? true : false;
  }
  std::shared_ptr<frLef58SpacingEndOfLineWithinParallelEdgeConstraint>
  getParallelEdgeConstraint() const
  {
    return parallelEdgeConstraint_;
  }
  bool hasMaxMinLengthConstraint() const
  {
    return (maxMinLengthConstraint_) ? true : false;
  }
  std::shared_ptr<frLef58SpacingEndOfLineWithinMaxMinLengthConstraint>
  getMaxMinLengthConstraint() const
  {
    return maxMinLengthConstraint_;
  }
  bool hasEncloseCutConstraint() const
  {
    return (encloseCutConstraint_) ? true : false;
  }
  std::shared_ptr<frLef58SpacingEndOfLineWithinEncloseCutConstraint>
  getEncloseCutConstraint() const
  {
    return encloseCutConstraint_;
  }
  // setters
  void setOppositeWidth(frCoord in)
  {
    hOppositeWidth_ = true;
    oppositeWidth_ = in;
  }
  void setEolWithin(frCoord in)
  {
    eolWithin_ = in;
    wrongDirWithin_ = in;
  }
  void setEndPrl(frCoord endPrlSpacingIn, frCoord endPrlIn)
  {
    endPrlSpacing_ = endPrlSpacingIn;
    endPrl_ = endPrlIn;
  }
  void setWrongDirWithin(frCoord in) { wrongDirWithin_ = in; }
  void setSameMask(bool in) { sameMask_ = in; }
  void setEndToEndConstraint(
      const std::shared_ptr<frLef58SpacingEndOfLineWithinEndToEndConstraint>&
          in)
  {
    endToEndConstraint_ = in;
  }
  void setParallelEdgeConstraint(
      const std::shared_ptr<
          frLef58SpacingEndOfLineWithinParallelEdgeConstraint>& in)
  {
    parallelEdgeConstraint_ = in;
  }
  void setMaxMinLengthConstraint(
      const std::shared_ptr<
          frLef58SpacingEndOfLineWithinMaxMinLengthConstraint>& in)
  {
    maxMinLengthConstraint_ = in;
  }
  void setEncloseCutConstraint(
      const std::shared_ptr<frLef58SpacingEndOfLineWithinEncloseCutConstraint>&
          in)
  {
    encloseCutConstraint_ = in;
  }
  // others
  frConstraintTypeEnum typeId() const override
  {
    return frConstraintTypeEnum::frcLef58SpacingEndOfLineWithinConstraint;
  }
  void report(utl::Logger* logger) const override
  {
    logger->report(
        "\tSPACING_WITHIN hOppositeWidth {} oppositeWidth {} eolWithin {} "
        "wrongDirWithin {} sameMask {} endPrlSpacing {} endPrl {}",
        hOppositeWidth_,
        oppositeWidth_,
        eolWithin_,
        wrongDirWithin_,
        sameMask_,
        endPrlSpacing_,
        endPrl_);
    if (endToEndConstraint_ != nullptr) {
      endToEndConstraint_->report(logger);
    }
    if (parallelEdgeConstraint_ != nullptr) {
      parallelEdgeConstraint_->report(logger);
    }
  }

 protected:
  bool hOppositeWidth_{false};
  frCoord oppositeWidth_{0};
  frCoord eolWithin_{0};
  frCoord wrongDirWithin_{0};
  frCoord endPrlSpacing_{0};
  frCoord endPrl_{0};
  bool sameMask_{false};
  std::shared_ptr<frLef58SpacingEndOfLineWithinEndToEndConstraint>
      endToEndConstraint_;
  std::shared_ptr<frLef58SpacingEndOfLineWithinParallelEdgeConstraint>
      parallelEdgeConstraint_;
  std::shared_ptr<frLef58SpacingEndOfLineWithinMaxMinLengthConstraint>
      maxMinLengthConstraint_;
  std::shared_ptr<frLef58SpacingEndOfLineWithinEncloseCutConstraint>
      encloseCutConstraint_;
};

class frLef58SpacingEndOfLineConstraint : public frConstraint
{
 public:
  // getters
  frCoord getEolSpace() const { return eolSpace_; }
  frCoord getEolWidth() const { return eolWidth_; }
  bool hasExactWidth() const { return exactWidth_; }
  bool hasWrongDirSpacing() const { return wrongDirSpacing_; }
  frCoord getWrongDirSpace() const { return wrongDirSpace_; }
  bool hasWithinConstraint() const
  {
    return (withinConstraint_) ? true : false;
  }
  std::shared_ptr<frLef58SpacingEndOfLineWithinConstraint> getWithinConstraint()
      const
  {
    return withinConstraint_;
  }
  bool hasToConcaveCornerConstraint() const { return false; }
  bool hasToNotchLengthConstraint() const { return false; }
  // setters
  void setEol(frCoord eolSpaceIn, frCoord eolWidthIn, bool exactWidthIn = false)
  {
    eolSpace_ = eolSpaceIn;
    eolWidth_ = eolWidthIn;
    exactWidth_ = exactWidthIn;
  }
  void setWrongDirSpace(frCoord in)
  {
    wrongDirSpacing_ = true;
    wrongDirSpace_ = in;
  }
  void setWithinConstraint(
      const std::shared_ptr<frLef58SpacingEndOfLineWithinConstraint>& in)
  {
    withinConstraint_ = in;
  }
  // others
  frConstraintTypeEnum typeId() const override
  {
    return frConstraintTypeEnum::frcLef58SpacingEndOfLineConstraint;
  }
  void report(utl::Logger* logger) const override
  {
    logger->report(
        "SPACING eolSpace {} eolWidth {} exactWidth {} wrongDirSpacing {} "
        "wrongDirSpace {} ",
        eolSpace_,
        eolWidth_,
        exactWidth_,
        wrongDirSpacing_,
        wrongDirSpace_);
    if (withinConstraint_ != nullptr) {
      withinConstraint_->report(logger);
    }
  }

 protected:
  frCoord eolSpace_{0};
  frCoord eolWidth_{0};
  bool exactWidth_{false};
  bool wrongDirSpacing_{false};
  frCoord wrongDirSpace_{0};
  std::shared_ptr<frLef58SpacingEndOfLineWithinConstraint> withinConstraint_;
};

class frLef58SpacingWrongDirConstraint : public frConstraint
{
 public:
  frLef58SpacingWrongDirConstraint(odb::dbTechLayerWrongDirSpacingRule* rule)
      : db_rule_(rule)
  {
  }
  // getter
  odb::dbTechLayerWrongDirSpacingRule* getODBRule() const { return db_rule_; }
  // others
  frConstraintTypeEnum typeId() const override
  {
    return frConstraintTypeEnum::frcLef58SpacingWrongDirConstraint;
  }
  void report(utl::Logger* logger) const override
  {
    logger->report("LEF58_SPACING WRONGDIRECTION");
  }

 private:
  odb::dbTechLayerWrongDirSpacingRule* db_rule_;
};

class frLef58EolKeepOutConstraint : public frConstraint
{
 public:
  // getters
  frCoord getBackwardExt() const { return backwardExt_; }
  frCoord getForwardExt() const { return forwardExt_; }
  frCoord getSideExt() const { return sideExt_; }
  frCoord getEolWidth() const { return eolWidth_; }
  bool isCornerOnly() const { return cornerOnly_; }
  bool isExceptWithin() const { return exceptWithin_; }
  frCoord getWithinLow() const { return withinLow_; }
  frCoord getWithinHigh() const { return withinHigh_; }
  // setters
  void setBackwardExt(frCoord value) { backwardExt_ = value; }
  void setForwardExt(frCoord value) { forwardExt_ = value; }
  void setSideExt(frCoord value) { sideExt_ = value; }
  void setEolWidth(frCoord value) { eolWidth_ = value; }
  void setCornerOnly(bool value) { cornerOnly_ = value; }
  void setExceptWithin(bool value) { exceptWithin_ = value; }
  void setWithinLow(frCoord value) { withinLow_ = value; }
  void setWithinHigh(frCoord value) { withinHigh_ = value; }
  // others
  frConstraintTypeEnum typeId() const override
  {
    return frConstraintTypeEnum::frcLef58EolKeepOutConstraint;
  }
  void report(utl::Logger* logger) const override
  {
    logger->report(
        "EOLKEEPOUT backwardExt {} sideExt {} forwardExt {} eolWidth {} "
        "cornerOnly {} exceptWithin {} withinLow {} withinHigh {}",
        backwardExt_,
        sideExt_,
        forwardExt_,
        eolWidth_,
        cornerOnly_,
        exceptWithin_,
        withinLow_,
        withinHigh_);
  }

 private:
  frCoord backwardExt_{0};
  frCoord sideExt_{0};
  frCoord forwardExt_{0};
  frCoord eolWidth_{0};
  bool cornerOnly_{false};
  bool exceptWithin_{false};
  frCoord withinLow_{0};
  frCoord withinHigh_{0};
};

class frLef58CornerSpacingSpacingConstraint;

// SPACING Constraints
class frSpacingConstraint : public frConstraint
{
 public:
  frSpacingConstraint() = default;
  frSpacingConstraint(frCoord minSpacingIn) : minSpacing_(minSpacingIn) {}

  // getter
  frCoord getMinSpacing() const { return minSpacing_; }
  // setter
  void setMinSpacing(frCoord minSpacingIn) { minSpacing_ = minSpacingIn; }
  // check
  frConstraintTypeEnum typeId() const override
  {
    return frConstraintTypeEnum::frcSpacingConstraint;
  }
  void report(utl::Logger* logger) const override
  {
    logger->report("Spacing {}", minSpacing_);
  }

 protected:
  frCoord minSpacing_{0};
};

class frSpacingSamenetConstraint : public frSpacingConstraint
{
 public:
  frSpacingSamenetConstraint() = default;
  frSpacingSamenetConstraint(frCoord minSpacingIn, bool pgonlyIn)
      : frSpacingConstraint(minSpacingIn), pgonly_(pgonlyIn)
  {
  }
  // getter
  bool hasPGonly() const { return pgonly_; }
  // setter
  void setPGonly(bool in) { pgonly_ = in; }
  // check
  frConstraintTypeEnum typeId() const override
  {
    return frConstraintTypeEnum::frcSpacingSamenetConstraint;
  }
  void report(utl::Logger* logger) const override
  {
    logger->report("Spacing same net pgonly {}", pgonly_);
  }

 protected:
  bool pgonly_{false};
};

class frSpacingTableInfluenceConstraint : public frConstraint
{
 public:
  frSpacingTableInfluenceConstraint(
      const fr1DLookupTbl<frCoord, std::pair<frCoord, frCoord>>& in)
      : tbl(in)
  {
  }
  // getter
  const fr1DLookupTbl<frCoord, std::pair<frCoord, frCoord>>& getLookupTbl()
      const
  {
    return tbl;
  }
  std::pair<frCoord, frCoord> find(frCoord width) const
  {
    return tbl.find(width);
  }
  frCoord getMinWidth() const { return tbl.getMinRow(); }
  // setter
  void setLookupTbl(
      const fr1DLookupTbl<frCoord, std::pair<frCoord, frCoord>>& in)
  {
    tbl = in;
  }

  frConstraintTypeEnum typeId() const override
  {
    return frConstraintTypeEnum::frcSpacingTableInfluenceConstraint;
  }
  void report(utl::Logger* logger) const override
  {
    logger->report("Spacing table influence");
  }

 private:
  fr1DLookupTbl<frCoord, std::pair<frCoord, frCoord>> tbl;
};
// range spacing
class frSpacingRangeConstraint : public frSpacingConstraint
{
 public:
  // getters
  frCoord getMinWidth() const { return minWidth; }
  frCoord getMaxWidth() const { return minWidth; }
  // setters
  void setMinWidth(frCoord in) { minWidth = in; }
  void setMaxWidth(frCoord in) { maxWidth = in; }
  // others
  frConstraintTypeEnum typeId() const override
  {
    return frConstraintTypeEnum::frcSpacingRangeConstraint;
  }
  void report(utl::Logger* logger) const override
  {
    logger->report("Spacing RANGE minWidth {} maxWidth {}", minWidth, maxWidth);
  }
  bool inRange(frCoord width) const
  {
    return width >= minWidth && width <= maxWidth;
  }

 private:
  frCoord minWidth{0};
  frCoord maxWidth{0};
};

//
// EOL spacing
class frSpacingEndOfLineConstraint : public frSpacingConstraint
{
 public:
  // getter
  frCoord getEolWidth() const { return eolWidth; }
  frCoord getEolWithin() const { return eolWithin; }
  frCoord getParSpace() const { return parSpace; }
  frCoord getParWithin() const { return parWithin; }
  bool hasParallelEdge() const { return ((parSpace == -1) ? false : true); }
  bool hasTwoEdges() const { return isTwoEdges; }
  // setter
  void setEolWithin(frCoord eolWithinIn) { eolWithin = eolWithinIn; }
  void setEolWidth(frCoord eolWidthIn) { eolWidth = eolWidthIn; }
  void setParSpace(frCoord parSpaceIn) { parSpace = parSpaceIn; }
  void setParWithin(frCoord parWithinIn) { parWithin = parWithinIn; }
  void setTwoEdges(bool isTwoEdgesIn) { isTwoEdges = isTwoEdgesIn; }
  frConstraintTypeEnum typeId() const override
  {
    return frConstraintTypeEnum::frcSpacingEndOfLineConstraint;
  }
  void report(utl::Logger* logger) const override
  {
    logger->report(
        "Spacing EOL eolWidth {} eolWithin {} "
        "parSpace {} parWithin {} isTwoEdges {}",
        eolWidth,
        eolWithin,
        parSpace,
        parWithin,
        isTwoEdges);
  }

 protected:
  frCoord eolWidth{-1};
  frCoord eolWithin{-1};
  frCoord parSpace{-1};
  frCoord parWithin{-1};
  bool isTwoEdges{false};
};

class frLef58EolExtensionConstraint : public frSpacingConstraint
{
 public:
  // constructors
  frLef58EolExtensionConstraint(const fr1DLookupTbl<frCoord, frCoord>& tbl)
      : extensionTbl_(tbl)
  {
  }
  // setters

  void setParallelOnly(bool value) { parallelOnly_ = value; }

  // getters

  bool isParallelOnly() const { return parallelOnly_; }

  fr1DLookupTbl<frCoord, frCoord> getExtensionTable() const
  {
    return extensionTbl_;
  }

  // others

  frConstraintTypeEnum typeId() const override
  {
    return frConstraintTypeEnum::frcLef58EolExtensionConstraint;
  }

  void report(utl::Logger* logger) const override
  {
    logger->report("EOLEXTENSIONSPACING spacing {} parallelonly {} ",
                   minSpacing_,
                   parallelOnly_);
  }

 private:
  bool parallelOnly_{false};
  fr1DLookupTbl<frCoord, frCoord> extensionTbl_;
};

// LEF58 cut spacing table
class frLef58CutSpacingTableConstraint : public frConstraint
{
 public:
  // constructor
  frLef58CutSpacingTableConstraint(
      odb::dbTechLayerCutSpacingTableDefRule* dbRule)
      : db_rule_(dbRule)
  {
  }
  // setter
  void setDefaultSpacing(const std::pair<frCoord, frCoord>& value)
  {
    default_spacing_ = value;
  }
  void setDefaultCenterToCenter(bool value) { default_center2center_ = value; }
  void setDefaultCenterAndEdge(bool value) { default_centerAndEdge_ = value; }
  // getter
  odb::dbTechLayerCutSpacingTableDefRule* getODBRule() const
  {
    return db_rule_;
  }
  void report(utl::Logger* logger) const override
  {
    logger->report(
        "CUTSPACINGTABLE lyr:{} lyr2:{}",
        db_rule_->getTechLayer()->getName(),
        db_rule_->isLayerValid() ? db_rule_->getSecondLayer()->getName() : "-");
  }
  std::pair<frCoord, frCoord> getDefaultSpacing() const
  {
    return default_spacing_;
  }
  bool getDefaultCenterToCenter() const { return default_center2center_; }
  bool getDefaultCenterAndEdge() const { return default_centerAndEdge_; }
  // others
  frConstraintTypeEnum typeId() const override
  {
    return frConstraintTypeEnum::frcLef58CutSpacingTableConstraint;
  }

 private:
  odb::dbTechLayerCutSpacingTableDefRule* db_rule_;
  std::pair<frCoord, frCoord> default_spacing_{0, 0};
  bool default_center2center_{false};
  bool default_centerAndEdge_{false};
};

// new SPACINGTABLE Constraints
class frSpacingTablePrlConstraint : public frConstraint
{
 public:
  // constructor
  frSpacingTablePrlConstraint(
      const fr2DLookupTbl<frCoord, frCoord, frCoord>& in)
      : tbl_(in)
  {
  }
  // getter
  const fr2DLookupTbl<frCoord, frCoord, frCoord>& getLookupTbl() const
  {
    return tbl_;
  }
  frCoord find(frCoord width, frCoord prl) const
  {
    return tbl_.find(width, prl);
  }
  frCoord findMin() const { return tbl_.findMin(); }
  frCoord findMax() const { return tbl_.findMax(); }
  // setter
  void setLookupTbl(const fr2DLookupTbl<frCoord, frCoord, frCoord>& in)
  {
    tbl_ = in;
  }
  frConstraintTypeEnum typeId() const override
  {
    return frConstraintTypeEnum::frcSpacingTablePrlConstraint;
  }
  void report(utl::Logger* logger) const override
  {
    logger->report("Spacing table PRL");
  }

 protected:
  fr2DLookupTbl<frCoord, frCoord, frCoord> tbl_;
};

struct frSpacingTableTwRowType
{
  frSpacingTableTwRowType(frCoord in1, frCoord in2) : width(in1), prl(in2) {}
  frCoord width;
  frCoord prl;
};

// new SPACINGTABLE Constraints
class frSpacingTableTwConstraint : public frConstraint
{
 public:
  // constructor
  frSpacingTableTwConstraint(
      const frCollection<frSpacingTableTwRowType>& rowsIn,
      const frCollection<frCollection<frCoord>>& spacingIn)
      : rows_(rowsIn), spacingTbl_(spacingIn)
  {
  }
  // getter
  frCoord find(frCoord width1, frCoord width2, frCoord prl) const;
  frCoord findMin() const { return spacingTbl_.front().front(); }
  frCoord findMax() const { return spacingTbl_.back().back(); }
  // setter
  void setSpacingTable(const frCollection<frSpacingTableTwRowType>& rowsIn,
                       const frCollection<frCollection<frCoord>>& spacingIn)
  {
    rows_ = rowsIn;
    spacingTbl_ = spacingIn;
  }

  frConstraintTypeEnum typeId() const override
  {
    return frConstraintTypeEnum::frcSpacingTableTwConstraint;
  }
  void report(utl::Logger* logger) const override
  {
    logger->report("Spacing table tw");
  }

 private:
  frUInt4 getIdx(frCoord width, frCoord prl) const
  {
    int sz = rows_.size();
    for (int i = 0; i < sz; i++) {
      if (width <= rows_[i].width) {
        return std::max(0, i - 1);
      }
      if (rows_[i].prl != -1 && prl <= rows_[i].prl) {
        return std::max(0, i - 1);
      }
    }
    return sz - 1;
  }

  frCollection<frSpacingTableTwRowType> rows_;
  frCollection<frCollection<frCoord>> spacingTbl_;
};

// original SPACINGTABLE Constraints
class frSpacingTableConstraint : public frConstraint
{
 public:
  using Table = fr2DLookupTbl<frCoord, frCoord, frCoord>;

  // constructor
  frSpacingTableConstraint(std::shared_ptr<Table> parallelRunLengthConstraintIn)
      : parallelRunLengthConstraint_(std::move(parallelRunLengthConstraintIn))
  {
  }
  // getter
  std::shared_ptr<Table> getParallelRunLengthConstraint() const
  {
    return parallelRunLengthConstraint_;
  }
  // setter
  void setParallelRunLengthConstraint(
      std::shared_ptr<fr2DLookupTbl<frCoord, frCoord, frCoord>>&
          parallelRunLengthConstraintIn)
  {
    parallelRunLengthConstraint_ = parallelRunLengthConstraintIn;
  }

  frConstraintTypeEnum typeId() const override
  {
    return frConstraintTypeEnum::frcSpacingTableConstraint;
  }
  void report(utl::Logger* logger) const override
  {
    logger->report("Spacing table");
  }

 protected:
  std::shared_ptr<Table> parallelRunLengthConstraint_;
};

class frLef58SpacingTableConstraint : public frSpacingTableConstraint
{
 public:
  // constructor
  frLef58SpacingTableConstraint(
      const std::shared_ptr<fr2DLookupTbl<frCoord, frCoord, frCoord>>&
          parallelRunLengthConstraintIn,
      const std::map<int, std::pair<frCoord, frCoord>>&
          exceptWithinConstraintIn)
      : frSpacingTableConstraint(parallelRunLengthConstraintIn),
        exceptWithinConstraint_(exceptWithinConstraintIn)
  {
  }
  // getter
  bool hasExceptWithin(frCoord val) const
  {
    auto rowIdx = getParallelRunLengthConstraint()->getRowIdx(val);
    return (exceptWithinConstraint_.find(rowIdx)
            != exceptWithinConstraint_.end());
  }
  std::pair<frCoord, frCoord> getExceptWithin(frCoord val) const
  {
    auto rowIdx = getParallelRunLengthConstraint()->getRowIdx(val);
    return exceptWithinConstraint_.at(rowIdx);
  }
  bool isWrongDirection() const { return wrongDirection_; }
  bool isSameMask() const { return sameMask_; }
  bool hasExceptEol() const { return exceptEol_; }
  frUInt4 getEolWidth() const { return eolWidth_; }
  // setters
  void setExceptWithinConstraint(
      std::map<int, std::pair<frCoord, frCoord>>& exceptWithinConstraintIn)
  {
    exceptWithinConstraint_ = exceptWithinConstraintIn;
  }
  void setWrongDirection(bool in) { wrongDirection_ = in; }
  void setSameMask(bool in) { sameMask_ = in; }
  void setEolWidth(frUInt4 in)
  {
    exceptEol_ = true;
    eolWidth_ = in;
  }

  frConstraintTypeEnum typeId() const override
  {
    return frConstraintTypeEnum::frcLef58SpacingTableConstraint;
  }

  void report(utl::Logger* logger) const override
  {
    logger->report(
        "SPACINGTABLE wrongDirection {} sameMask {} exceptEol {} eolWidth {} ",
        wrongDirection_,
        sameMask_,
        exceptEol_,
        eolWidth_);
    logger->report("\texceptWithinConstraint");
    for (auto& [key, val] : exceptWithinConstraint_) {
      logger->report("\t{} ({} {})", key, val.first, val.second);
    }
  }

 protected:
  std::map<frCoord, std::pair<frCoord, frCoord>> exceptWithinConstraint_;
  bool wrongDirection_{false};
  bool sameMask_{false};
  bool exceptEol_{false};
  frUInt4 eolWidth_{0};
};
// LEF58_TWOWIRESFORBIDDENSPACING
class frLef58TwoWiresForbiddenSpcConstraint : public frConstraint
{
 public:
  frLef58TwoWiresForbiddenSpcConstraint(
      odb::dbTechLayerTwoWiresForbiddenSpcRule* db_rule)
      : db_rule_(db_rule)
  {
  }
  // getters
  odb::dbTechLayerTwoWiresForbiddenSpcRule* getODBRule() const
  {
    return db_rule_;
  }
  // setters
  void setODBRule(odb::dbTechLayerTwoWiresForbiddenSpcRule* in)
  {
    db_rule_ = in;
  }
  // others
  frConstraintTypeEnum typeId() const override
  {
    return frConstraintTypeEnum::frcLef58TwoWiresForbiddenSpcConstraint;
  }
  bool isValidForMinSpanLength(frCoord width)
  {
    return db_rule_->isMinExactSpanLength()
               ? (width == db_rule_->getMinSpanLength())
               : (width >= db_rule_->getMinSpanLength());
  }
  bool isValidForMaxSpanLength(frCoord width)
  {
    return db_rule_->isMaxExactSpanLength()
               ? (width == db_rule_->getMaxSpanLength())
               : (width <= db_rule_->getMaxSpanLength());
  }
  bool isForbiddenSpacing(frCoord spc)
  {
    return spc >= db_rule_->getMinSpacing() && spc <= db_rule_->getMaxSpacing();
  }
  bool isValidPrl(frCoord prl) { return prl > db_rule_->getPrl(); }
  void report(utl::Logger* logger) const override
  {
    logger->report("LEF58_TWOWIRESFORBIDDENSPACING");
  }

 private:
  odb::dbTechLayerTwoWiresForbiddenSpcRule* db_rule_;
};
// LEF58_FORBIDDENSPACING
class frLef58ForbiddenSpcConstraint : public frConstraint
{
 public:
  frLef58ForbiddenSpcConstraint(odb::dbTechLayerForbiddenSpacingRule* db_rule)
      : db_rule_(db_rule)
  {
  }
  // getters
  odb::dbTechLayerForbiddenSpacingRule* getODBRule() const { return db_rule_; }
  frCoord getMinSpc() const { return db_rule_->getForbiddenSpacing().first; }
  frCoord getMaxSpc() const { return db_rule_->getForbiddenSpacing().second; }
  frCoord getTwoEdgesWithin() const { return db_rule_->getTwoEdges(); }
  // setters
  void setODBRule(odb::dbTechLayerForbiddenSpacingRule* in) { db_rule_ = in; }
  // others
  bool isPrlValid(frCoord prl) const { return prl > db_rule_->getPrl(); }
  bool isWidthValid(frCoord width) const
  {
    return width < db_rule_->getWidth();
  }
  bool isForbiddenSpc(frCoord spc) const
  {
    return spc >= getMinSpc() && spc <= getMaxSpc();
  }
  frConstraintTypeEnum typeId() const override
  {
    return frConstraintTypeEnum::frcLef58ForbiddenSpcConstraint;
  }

  void report(utl::Logger* logger) const override
  {
    logger->report("LEF58_FORBIDDENSPACING");
  }

 private:
  odb::dbTechLayerForbiddenSpacingRule* db_rule_;
};
// ADJACENTCUTS
class frCutSpacingConstraint : public frConstraint
{
 public:
  // constructor
  frCutSpacingConstraint(frCoord cutSpacingIn,
                         bool centerToCenterIn,
                         bool sameNetIn,
                         const frString& secondLayerNameIn,
                         bool stackIn,
                         int adjacentCutsIn,
                         frCoord cutWithinIn,
                         bool isExceptSamePGNetIn,
                         bool isParallelOverlapIn,
                         frCoord cutAreaIn,
                         int twoCutsIn = -1)
  {
    cutSpacing_ = cutSpacingIn;
    centerToCenter_ = centerToCenterIn;
    sameNet_ = sameNetIn;
    secondLayerName_ = secondLayerNameIn;
    stack_ = stackIn;
    adjacentCuts_ = adjacentCutsIn;
    cutWithin_ = cutWithinIn;
    exceptSamePGNet_ = isExceptSamePGNetIn;
    parallelOverlap_ = isParallelOverlapIn;
    cutArea_ = cutAreaIn;
    twoCuts_ = twoCutsIn;
  }
  // getter
  bool hasCenterToCenter() const { return centerToCenter_; }
  bool getCenterToCenter() const { return centerToCenter_; }
  bool getSameNet() const { return sameNet_; }
  bool hasSameNet() const { return sameNet_; }
  frCutSpacingConstraint* getSameNetConstraint() { return sameNetConstraint_; }
  bool getStack() const { return stack_; }
  bool hasStack() const { return stack_; }
  bool isLayer() const { return !(secondLayerName_.empty()); }
  const frString& getSecondLayerName() const { return secondLayerName_; }
  bool hasSecondLayer() const { return (secondLayerNum_ != -1); }
  frLayerNum getSecondLayerNum() const { return secondLayerNum_; }
  bool isAdjacentCuts() const { return (adjacentCuts_ != -1); }
  int getAdjacentCuts() const { return adjacentCuts_; }
  frCoord getCutWithin() const { return cutWithin_; }
  bool hasExceptSamePGNet() const { return exceptSamePGNet_; }
  bool getExceptSamePGNet() const { return exceptSamePGNet_; }
  bool isParallelOverlap() const { return parallelOverlap_; }
  bool getParallelOverlap() const { return parallelOverlap_; }
  bool isArea() const { return !(cutArea_ == -1); }
  bool getCutArea() const { return cutArea_; }
  frCoord getCutSpacing() const { return cutSpacing_; }
  frConstraintTypeEnum typeId() const override
  {
    return frConstraintTypeEnum::frcCutSpacingConstraint;
  }
  // LEF58 related
  bool isTwoCuts() const { return (twoCuts_ == -1); }
  int getTwoCuts() const { return twoCuts_; }

  void report(utl::Logger* logger) const override
  {
    logger->report("Cut Spacing");
  }

  // setter

  void setSecondLayerNum(int secondLayerNumIn)
  {
    secondLayerNum_ = secondLayerNumIn;
  }

  void setSameNetConstraint(frCutSpacingConstraint* in)
  {
    sameNetConstraint_ = in;
  }

 protected:
  frCoord cutSpacing_ = -1;
  bool centerToCenter_ = false;
  bool sameNet_ = false;
  frCutSpacingConstraint* sameNetConstraint_ = nullptr;
  bool stack_ = false;
  bool exceptSamePGNet_ = false;
  bool parallelOverlap_ = false;
  frString secondLayerName_;
  frLayerNum secondLayerNum_ = -1;
  int adjacentCuts_ = -1;
  frCoord cutWithin_ = -1;
  frCoord cutArea_ = -1;
  // LEF58 related
  int twoCuts_ = -1;
};

// LEF58_SPACING for cut layer (new)
class frLef58CutSpacingConstraint : public frConstraint
{
 public:
  // getters
  // is == what rules have; has == what derived from values
  frCoord getCutSpacing() const { return cutSpacing_; }
  bool isSameMask() const { return sameMask_; }
  bool isMaxXY() const { return maxXY_; }
  bool isCenterToCenter() const { return centerToCenter_; }
  bool isSameNet() const { return sameNet_; }
  bool isSameMetal() const { return sameMetal_; }
  bool isSameVia() const { return sameVia_; }
  std::string getSecondLayerName() const { return secondLayerName_; }
  bool hasSecondLayer() const
  {
    return secondLayerNum_ != -1 || !secondLayerName_.empty();
  }
  frLayerNum getSecondLayerNum() const { return secondLayerNum_; }
  bool isStack() const { return stack_; }
  bool hasOrthogonalSpacing() const { return (orthogonalSpacing_ != -1); }
  std::string getCutClassName() const { return cutClassName_; }
  bool hasCutClass() const
  {
    return cutClassIdx_ != -1 || !cutClassName_.empty();
  }
  int getCutClassIdx() const { return cutClassIdx_; }
  bool isShortEdgeOnly() const { return shortEdgeOnly_; }
  bool hasPrl() const { return (prl_ != -1); }
  frCoord getPrl() const { return prl_; }
  bool isConcaveCorner() const { return concaveCorner_; }
  bool hasWidth() const { return (width_ != -1); }
  frCoord getWidth() const { return width_; }
  bool hasEnclosure() const { return (enclosure_ != -1); }
  frCoord getEnclosure() const { return enclosure_; }
  bool hasEdgeLength() const { return (edgeLength_ != -1); }
  frCoord getEdgeLength() const { return edgeLength_; }
  bool hasParallel() const { return (parLength_ != -1); }
  frCoord getParlength() const { return parLength_; }
  frCoord getParWithin() const { return parWithin_; }
  frCoord getEdgeEnclosure() const { return edgeEnclosure_; }
  frCoord getAdjEnclosure() const { return adjEnclosure_; }
  bool hasExtension() const { return (extension_ != -1); }
  frCoord getExtension() const { return extension_; }
  bool hasNonEolConvexCorner() const { return (eolWidth_ != -1); }
  frCoord getEolWidth() const { return eolWidth_; }
  bool hasMinLength() const { return (minLength_ != -1); }
  frCoord getMinLength() const { return minLength_; }
  bool hasAboveWidth() const { return (width_ != -1); }
  bool isMaskOverlap() const { return maskOverlap_; }
  bool isWrongDirection() const { return wrongDirection_; }
  bool hasAdjacentCuts() const { return (adjacentCuts_ != -1); }
  int getAdjacentCuts() const { return adjacentCuts_; }
  bool hasExactAligned() const { return (exactAlignedCut_ != -1); }
  int getExactAligned() const { return exactAlignedCut_; }
  bool hasTwoCuts() const { return (twoCuts_ != -1); }
  int getTwoCuts() const { return twoCuts_; }
  bool hasTwoCutsSpacing() const { return (twoCutsSpacing_ != -1); }
  frCoord getTwoCutsSpacing() const { return twoCutsSpacing_; }
  bool isSameCut() const { return sameCut_; }
  // cutWithin2 is always used as upper bound
  bool hasTwoCutWithin() const { return (cutWithin1_ != -1); }
  frCoord getCutWithin() const { return cutWithin2_; }
  frCoord getCutWithin1() const { return cutWithin1_; }
  frCoord getCutWithin2() const { return cutWithin2_; }
  bool isExceptSamePGNet() const { return exceptSamePGNet_; }
  bool hasExceptAllWithin() const { return (exceptAllWithin_ != -1); }
  frCoord getExceptAllWithin() const { return exceptAllWithin_; }
  bool isAbove() const { return above_; }
  bool isBelow() const { return below_; }
  bool isToAll() const { return toAll_; }
  bool isNoPrl() const { return noPrl_; }
  bool isSideParallelOverlap() const { return sideParallelOverlap_; }
  bool isParallelOverlap() const { return parallelOverlap_; }
  bool isExceptSameNet() const { return exceptSameNet_; }
  bool isExceptSameMetal() const { return exceptSameMetal_; }
  bool isExceptSameMetalOverlap() const { return exceptSameMetalOverlap_; }
  bool isExceptSameVia() const { return exceptSameVia_; }
  bool hasParallelWithin() const { return (within_ != -1); }
  frCoord getWithin() const { return within_; }
  bool isLongEdgeOnly() const { return longEdgeOnly_; }
  bool hasSameMetalSharedEdge() const { return (parWithin_ != -1); }
  bool isExceptTwoEdges() const { return exceptTwoEdges_; }
  bool hasExceptSameVia() const { return (numCut_ != -1); }
  bool hasArea() const { return (cutArea_ != -1); }
  frCoord getCutArea() const { return cutArea_; }
  // setters
  void setCutSpacing(frCoord in) { cutSpacing_ = in; }
  void setSameMask(bool in) { sameMask_ = in; }
  void setMaxXY(bool in) { maxXY_ = in; }
  void setCenterToCenter(bool in) { centerToCenter_ = in; }
  void setSameNet(bool in) { sameNet_ = in; }
  void setSameMetal(bool in) { sameMetal_ = in; }
  void setSameVia(bool in) { sameVia_ = in; }
  void setSecondLayerName(const std::string& in) { secondLayerName_ = in; }
  void setSecondLayerNum(frLayerNum in) { secondLayerNum_ = in; }
  void setStack(bool in) { stack_ = in; }
  void setOrthogonalSpacing(frCoord in) { orthogonalSpacing_ = in; }
  void setCutClassName(const std::string& in) { cutClassName_ = in; }
  void setCutClassIdx(int in) { cutClassIdx_ = in; }
  void setShortEdgeOnly(bool in) { shortEdgeOnly_ = in; }
  void setPrl(frCoord in) { prl_ = in; }
  void setConcaveCorner(bool in) { concaveCorner_ = in; }
  void setWidth(frCoord in) { width_ = in; }
  void setEnclosure(frCoord in) { enclosure_ = in; }
  void setEdgeLength(frCoord in) { edgeLength_ = in; }
  void setParLength(frCoord in) { parLength_ = in; }
  void setParWithin(frCoord in) { parWithin_ = in; }
  void setEdgeEnclosure(frCoord in) { edgeEnclosure_ = in; }
  void setAdjEnclosure(frCoord in) { adjEnclosure_ = in; }
  void setExtension(frCoord in) { extension_ = in; }
  void setEolWidth(frCoord in) { eolWidth_ = in; }
  void setMinLength(frCoord in) { minLength_ = in; }
  void setMaskOverlap(bool in) { maskOverlap_ = in; }
  void setWrongDirection(bool in) { wrongDirection_ = in; }
  void setAdjacentCuts(int in) { adjacentCuts_ = in; }
  void setExactAlignedCut(int in) { exactAlignedCut_ = in; }
  void setTwoCuts(int in) { twoCuts_ = in; }
  void setTwoCutsSpacing(frCoord in) { twoCutsSpacing_ = in; }
  void setSameCut(bool in) { sameCut_ = in; }
  void setCutWithin(frCoord in) { cutWithin2_ = in; }
  void setCutWithin1(frCoord in) { cutWithin1_ = in; }
  void setCutWithin2(frCoord in) { cutWithin2_ = in; }
  void setExceptSamePGNet(bool in) { exceptSamePGNet_ = in; }
  void setExceptAllWithin(frCoord in) { exceptAllWithin_ = in; }
  void setAbove(bool in) { above_ = in; }
  void setBelow(bool in) { below_ = in; }
  void setToAll(bool in) { toAll_ = in; }
  void setNoPrl(bool in) { noPrl_ = in; }
  void setSideParallelOverlap(bool in) { sideParallelOverlap_ = in; }
  void setParallelOverlap(bool in) { parallelOverlap_ = in; }
  void setExceptSameNet(bool in) { exceptSameNet_ = in; }
  void setExceptSameMetal(bool in) { exceptSameMetal_ = in; }
  void setExceptSameMetalOverlap(bool in) { exceptSameMetalOverlap_ = in; }
  void setExceptSameVia(bool in) { exceptSameVia_ = in; }
  void setWithin(frCoord in) { within_ = in; }
  void setLongEdgeOnly(bool in) { longEdgeOnly_ = in; }
  void setExceptTwoEdges(bool in) { exceptTwoEdges_ = in; }
  void setNumCut(int in) { numCut_ = in; }
  void setCutArea(frCoord in) { cutArea_ = in; }

  // others
  frConstraintTypeEnum typeId() const override
  {
    return frConstraintTypeEnum::frcLef58CutSpacingConstraint;
  }

  void report(utl::Logger* logger) const override
  {
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
        cutSpacing_,
        sameMask_,
        maxXY_,
        centerToCenter_,
        sameNet_,
        sameMetal_,
        sameVia_,
        secondLayerName_,
        secondLayerNum_,
        stack_,
        orthogonalSpacing_,
        cutClassName_,
        cutClassIdx_,
        shortEdgeOnly_,
        prl_,
        concaveCorner_,
        width_,
        enclosure_,
        edgeLength_,
        parLength_,
        parWithin_,
        edgeEnclosure_,
        adjEnclosure_,
        extension_,
        eolWidth_,
        minLength_,
        maskOverlap_,
        wrongDirection_,
        adjacentCuts_,
        exactAlignedCut_,
        twoCuts_,
        twoCutsSpacing_,
        sameCut_,
        cutWithin1_,
        cutWithin2_,
        exceptSamePGNet_,
        exceptAllWithin_,
        above_,
        below_,
        toAll_,
        noPrl_,
        sideParallelOverlap_,
        parallelOverlap_,
        exceptSameNet_,
        exceptSameMetal_,
        exceptSameMetalOverlap_,
        exceptSameVia_,
        within_,
        longEdgeOnly_,
        exceptTwoEdges_,
        numCut_,
        cutArea_);
  }

 protected:
  frCoord cutSpacing_{-1};
  bool sameMask_{false};
  bool maxXY_{false};
  bool centerToCenter_{false};
  bool sameNet_{false};
  bool sameMetal_{false};
  bool sameVia_{false};
  std::string secondLayerName_;
  frLayerNum secondLayerNum_{-1};
  bool stack_{false};
  frCoord orthogonalSpacing_{-1};
  std::string cutClassName_;
  int cutClassIdx_{-1};
  bool shortEdgeOnly_{false};
  frCoord prl_{-1};
  bool concaveCorner_{false};
  frCoord width_{-1};
  frCoord enclosure_{-1};
  frCoord edgeLength_{-1};
  frCoord parLength_{-1};
  frCoord parWithin_{-1};
  frCoord edgeEnclosure_{-1};
  frCoord adjEnclosure_{-1};
  frCoord extension_{-1};
  frCoord eolWidth_{-1};
  frCoord minLength_{-1};
  bool maskOverlap_{false};
  bool wrongDirection_{false};
  int adjacentCuts_{-1};
  int exactAlignedCut_{-1};
  int twoCuts_{-1};
  frCoord twoCutsSpacing_{-1};
  bool sameCut_{false};
  frCoord cutWithin1_{-1};
  frCoord cutWithin2_{-1};
  bool exceptSamePGNet_{false};
  frCoord exceptAllWithin_{-1};
  bool above_{false};
  bool below_{false};
  bool toAll_{false};
  bool noPrl_{false};
  bool sideParallelOverlap_{false};
  bool parallelOverlap_{false};
  bool exceptSameNet_{false};
  bool exceptSameMetal_{false};
  bool exceptSameMetalOverlap_{false};
  bool exceptSameVia_{false};
  frCoord within_{-1};
  bool longEdgeOnly_{false};
  bool exceptTwoEdges_{false};
  int numCut_{-1};
  frCoord cutArea_{-1};
};

// LEF58_CORNERSPACING (new)
class frLef58CornerSpacingConstraint : public frConstraint
{
 public:
  // constructor
  frLef58CornerSpacingConstraint(
      const fr1DLookupTbl<frCoord, std::pair<frCoord, frCoord>>& tblIn)
      : tbl_(tblIn)
  {
  }

  // getters
  frConstraintTypeEnum typeId() const override
  {
    return frConstraintTypeEnum::frcLef58CornerSpacingConstraint;
  }
  frCornerTypeEnum getCornerType() const { return cornerType_; }
  bool getSameMask() const { return sameMask_; }
  bool hasCornerOnly() const { return (within_ != -1); }
  frCoord getWithin() const { return within_; }
  bool hasExceptEol() const { return (eolWidth_ != -1); }
  frCoord getEolWidth() const { return eolWidth_; }
  bool hasExceptJogLength() const { return (minLength_ != -1); }
  frCoord getLength() const { return length_; }
  bool hasEdgeLength() const { return edgeLength_; }
  bool getEdgeLength() const { return edgeLength_; }
  bool hasIncludeLShape() const { return includeLShape_; }
  bool getIncludeLShape() const { return includeLShape_; }
  frCoord getMinLength() const { return minLength_; }
  bool hasExceptNotch() const { return exceptNotch_; }
  bool getExceptNotch() const { return exceptNotch_; }
  frCoord getNotchLength() const { return notchLength_; }
  bool hasExceptSameNet() const { return exceptSameNet_; }
  bool getExceptSameNet() const { return exceptSameNet_; }
  bool hasExceptSameMetal() const { return exceptSameMetal_; }
  bool getExceptSameMetal() const { return exceptSameMetal_; }
  frCoord find(frCoord width, bool isHorizontal = true) const
  {
    return (isHorizontal ? tbl_.find(width).first : tbl_.find(width).second);
  }
  frCoord findMin() const
  {
    return std::min(tbl_.findMin().first, tbl_.findMin().second);
  }
  frCoord findMax() const
  {
    return std::max(tbl_.findMax().first, tbl_.findMax().second);
  }
  frCoord findMax(bool isHorz) const
  {
    return (isHorz ? tbl_.findMax().first : tbl_.findMax().second);
  }
  bool hasSameXY() const { return sameXY_; }
  bool getSameXY() const { return sameXY_; }
  bool isCornerToCorner() const { return cornerToCorner_; }

  // setters
  void setCornerType(frCornerTypeEnum in) { cornerType_ = in; }
  void setSameMask(bool in) { sameMask_ = in; }
  void setWithin(frCoord in) { within_ = in; }
  void setEolWidth(frCoord in) { eolWidth_ = in; }
  void setLength(frCoord in) { length_ = in; }
  void setEdgeLength(bool in) { edgeLength_ = in; }
  void setIncludeLShape(bool in) { includeLShape_ = in; }
  void setMinLength(frCoord in) { minLength_ = in; }
  void setExceptNotch(bool in) { exceptNotch_ = in; }
  void setExceptNotchLength(frCoord in) { notchLength_ = in; }
  void setExceptSameNet(bool in) { exceptSameNet_ = in; }
  void setExceptSameMetal(bool in) { exceptSameMetal_ = in; }
  void setLookupTbl(
      const fr1DLookupTbl<frCoord, std::pair<frCoord, frCoord>>& in)
  {
    tbl_ = in;
  }
  void setSameXY(bool in) { sameXY_ = in; }
  void setCornerToCorner(bool in) { cornerToCorner_ = in; }
  void report(utl::Logger* logger) const override
  {
    logger->report(
        "CORNERSPACING cornerType {} sameMask {} within {} eolWidth {} length "
        "{} edgeLength {} includeLShape {} minLength {} exceptNotch {} "
        "notchLength {} exceptSameNet {} exceptSameMetal {} sameXY {} "
        "cornerToCorner {}",
        cornerType_,
        sameMask_,
        within_,
        eolWidth_,
        length_,
        edgeLength_,
        includeLShape_,
        minLength_,
        exceptNotch_,
        notchLength_,
        exceptSameNet_,
        exceptSameMetal_,
        sameXY_,
        cornerToCorner_);

    std::string vals;
    std::string rows;
    for (auto row : tbl_.getRows()) {
      rows += std::to_string(row) + " ";
    }
    for (const auto& val : tbl_.getValues()) {
      vals += "(" + std::to_string(val.first) + "," + std::to_string(val.second)
              + ") ";
    }
    logger->report("\trowName: {}", tbl_.getRowName());
    logger->report("\trows: {}", rows);
    logger->report("\tvals: {}", vals);
  }

 protected:
  frCornerTypeEnum cornerType_{frCornerTypeEnum::UNKNOWN};
  bool sameMask_{false};
  frCoord within_{-1};
  frCoord eolWidth_{-1};
  frCoord length_{-1};
  bool edgeLength_{false};
  bool includeLShape_{false};
  frCoord minLength_{-1};
  bool exceptNotch_{false};
  frCoord notchLength_{-1};
  bool exceptSameNet_{false};
  bool exceptSameMetal_{false};
  // horz / vert spacing
  fr1DLookupTbl<frCoord, std::pair<frCoord, frCoord>> tbl_;
  // indicate whether horz spacing == vert spacing for write LEF some day
  bool sameXY_{false};
  bool cornerToCorner_{false};
};

class frLef58CornerSpacingSpacingConstraint : public frConstraint
{
 public:
  // constructor
  frLef58CornerSpacingSpacingConstraint(frCoord widthIn) : width_(widthIn) {}
  // getter
  frCoord getWidth() { return width_; }
  // setter
  void setWidth(frCoord widthIn) { width_ = widthIn; }

  frConstraintTypeEnum typeId() const override
  {
    return frConstraintTypeEnum::frcLef58CornerSpacingSpacingConstraint;
  }
  void report(utl::Logger* logger) const override
  {
    logger->report("58 Corner spacing spacing {}", width_);
  }

 protected:
  frCoord width_;
};

class frLef58CornerSpacingSpacing1DConstraint
    : public frLef58CornerSpacingSpacingConstraint
{
 public:
  // constructor
  frLef58CornerSpacingSpacing1DConstraint(frCoord widthIn, frCoord spacingIn)
      : frLef58CornerSpacingSpacingConstraint(widthIn), spacing_(spacingIn)
  {
  }
  // getter
  frCoord getSpacing() { return spacing_; }
  // setter
  void setSpacing(frCoord spacingIn) { spacing_ = spacingIn; }

  frConstraintTypeEnum typeId() const override
  {
    return frConstraintTypeEnum::frcLef58CornerSpacingSpacing1DConstraint;
  }

  void report(utl::Logger* logger) const override
  {
    logger->report("58 Corner spacing 1D {}", spacing_);
  }

 protected:
  frCoord spacing_{-1};
};

class frLef58CornerSpacingSpacing2DConstraint
    : public frLef58CornerSpacingSpacingConstraint
{
 public:
  // constructor
  frLef58CornerSpacingSpacing2DConstraint(frCoord widthIn,
                                          frCoord horizontalSpacingIn,
                                          frCoord verticalSpacingIn)
      : frLef58CornerSpacingSpacingConstraint(widthIn),
        horizontalSpacing_(horizontalSpacingIn),
        verticalSpacing_(verticalSpacingIn)
  {
  }
  // getter
  frCoord getHorizontalSpacing() { return horizontalSpacing_; }
  frCoord getVerticalSpacing() { return verticalSpacing_; }
  // setter
  void setHorizontalSpacing(frCoord horizontalSpacingIn)
  {
    horizontalSpacing_ = horizontalSpacingIn;
  }
  void setVerticalSpacing(frCoord verticalSpacingIn)
  {
    verticalSpacing_ = verticalSpacingIn;
  }

  frConstraintTypeEnum typeId() const override
  {
    return frConstraintTypeEnum::frcLef58CornerSpacingSpacing2DConstraint;
  }
  void report(utl::Logger* logger) const override
  {
    logger->report("58 Corner spacing spacing 2D h {} v {}",
                   horizontalSpacing_,
                   verticalSpacing_);
  }

 protected:
  frCoord horizontalSpacing_{-1};
  frCoord verticalSpacing_{-1};
};

class frLef58RectOnlyConstraint : public frConstraint
{
 public:
  // constructor
  frLef58RectOnlyConstraint(bool exceptNonCorePinsIn = false)
      : exceptNonCorePins_(exceptNonCorePinsIn)
  {
  }
  // getter
  bool isExceptNonCorePinsIn() { return exceptNonCorePins_; }
  // setter
  void setExceptNonCorePins(bool exceptNonCorePinsIn)
  {
    exceptNonCorePins_ = exceptNonCorePinsIn;
  }

  frConstraintTypeEnum typeId() const override
  {
    return frConstraintTypeEnum::frcLef58RectOnlyConstraint;
  }
  void report(utl::Logger* logger) const override
  {
    logger->report("RECTONLY exceptNonCorePins {}", exceptNonCorePins_);
  }

 protected:
  bool exceptNonCorePins_;
};

class frLef58RightWayOnGridOnlyConstraint : public frConstraint
{
 public:
  // constructor
  frLef58RightWayOnGridOnlyConstraint(bool checkMaskIn = false)
      : checkMask_(checkMaskIn)
  {
  }
  // getter
  bool isCheckMask() { return checkMask_; }
  // setter
  void setCheckMask(bool checkMaskIn) { checkMask_ = checkMaskIn; }

  frConstraintTypeEnum typeId() const override
  {
    return frConstraintTypeEnum::frcLef58RightWayOnGridOnlyConstraint;
  }
  void report(utl::Logger* logger) const override
  {
    logger->report("RIGHTWAYONGRIDONLY checkMask {}", checkMask_);
  }

 protected:
  bool checkMask_;
};

class frMetalWidthViaConstraint : public frConstraint
{
 public:
  frMetalWidthViaConstraint(odb::dbMetalWidthViaMap* rule) : dbRule_(rule) {}
  odb::dbMetalWidthViaMap* getDbRule() const { return dbRule_; }
  frConstraintTypeEnum typeId() const override
  {
    return frConstraintTypeEnum::frcMetalWidthViaConstraint;
  }
  void report(utl::Logger* logger) const override
  {
    logger->report("METALWIDTHVIAMAP");
  }

 private:
  odb::dbMetalWidthViaMap* dbRule_;
};

class frLef58AreaConstraint : public frConstraint
{
 public:
  // constructor
  frLef58AreaConstraint(odb::dbTechLayerAreaRule* dbRule) : db_rule_(dbRule) {}
  // getter
  odb::dbTechLayerAreaRule* getODBRule() const { return db_rule_; }

  // others
  frConstraintTypeEnum typeId() const override
  {
    return frConstraintTypeEnum::frcLef58AreaConstraint;
  }

  void report(utl::Logger* logger) const override
  {
    auto trim_layer = db_rule_->getTrimLayer();
    std::string trim_layer_name
        = trim_layer != nullptr ? db_rule_->getTrimLayer()->getName() : "";
    logger->report(
        "LEF58 AREA rule: area {}, exceptMinWidth {}, exceptEdgeLength {}, "
        "exceptEdgeLengths ({} {}), exceptMinSize ({} {}), exceptStep ({} {}), "
        "mask {}, rectWidth {}, exceptRectangle {}, overlap {}, trimLayer {}",
        db_rule_->getArea(),
        db_rule_->getExceptMinWidth(),
        db_rule_->getExceptEdgeLength(),
        db_rule_->getExceptEdgeLengths().first,
        db_rule_->getExceptEdgeLengths().second,
        db_rule_->getExceptMinSize().first,
        db_rule_->getExceptMinSize().second,
        db_rule_->getExceptStep().first,
        db_rule_->getExceptStep().second,
        db_rule_->getMask(),
        db_rule_->getRectWidth(),
        db_rule_->isExceptRectangle(),
        db_rule_->getOverlap(),
        trim_layer_name);
  }

 private:
  odb::dbTechLayerAreaRule* db_rule_;
};

// LEF58_KEEPOUTZONE rule
class frLef58KeepOutZoneConstraint : public frConstraint
{
 public:
  frLef58KeepOutZoneConstraint(odb::dbTechLayerKeepOutZoneRule* rule)
      : db_rule_(rule)
  {
  }
  // getter
  odb::dbTechLayerKeepOutZoneRule* getODBRule() const { return db_rule_; }
  // others
  frConstraintTypeEnum typeId() const override
  {
    return frConstraintTypeEnum::frcLef58KeepOutZoneConstraint;
  }
  void report(utl::Logger* logger) const override
  {
    logger->report("LEF58_KEEPOUTZONE");
  }

 private:
  odb::dbTechLayerKeepOutZoneRule* db_rule_;
};

class frLef58EnclosureConstraint : public frConstraint
{
 public:
  frLef58EnclosureConstraint(odb::dbTechLayerCutEnclosureRule* ruleIn)
      : db_rule_(ruleIn), cut_class_idx_(-1)
  {
  }
  void setCutClassIdx(int in) { cut_class_idx_ = in; }
  int getCutClassIdx() const { return cut_class_idx_; }
  bool isAboveOnly() const { return db_rule_->isAbove(); }
  bool isBelowOnly() const { return db_rule_->isBelow(); }
  bool isValidOverhang(frCoord endOverhang, frCoord sideOverhang) const
  {
    if (db_rule_->getType() == odb::dbTechLayerCutEnclosureRule::ENDSIDE) {
      return endOverhang >= db_rule_->getFirstOverhang()
             && sideOverhang >= db_rule_->getSecondOverhang();
    }
    return (endOverhang >= db_rule_->getFirstOverhang()
            && sideOverhang >= db_rule_->getSecondOverhang())
           || (endOverhang >= db_rule_->getSecondOverhang()
               && sideOverhang >= db_rule_->getFirstOverhang());
  }
  frCoord getWidth() const { return db_rule_->getMinWidth(); }
  bool isEol() const
  {
    return db_rule_->getType() == odb::dbTechLayerCutEnclosureRule::EOL;
  }
  bool isEolOnly() const { return db_rule_->isEolOnly(); }
  frCoord getFirstOverhang() const { return db_rule_->getFirstOverhang(); }
  frCoord getSecondOverhang() const { return db_rule_->getSecondOverhang(); }
  frCoord getEolLength() const { return db_rule_->getEolWidth(); }
  void report(utl::Logger* logger) const override
  {
    logger->report("LEF58_ENCLOSURE");
  }
  frConstraintTypeEnum typeId() const override
  {
    return frConstraintTypeEnum::frcLef58EnclosureConstraint;
  }

 private:
  odb::dbTechLayerCutEnclosureRule* db_rule_;
  int cut_class_idx_;
};

// LEF58_MAXSPACING rule
class frLef58MaxSpacingConstraint : public frConstraint
{
 public:
  frLef58MaxSpacingConstraint(odb::dbTechLayerMaxSpacingRule* ruleIn)
      : db_rule_(ruleIn)
  {
  }
  void setCutClassIdx(int in) { cut_class_idx_ = in; }
  int getCutClassIdx() const { return cut_class_idx_; }
  frCoord getMaxSpacing() const { return db_rule_->getMaxSpacing(); }
  std::string getCutClass() const { return db_rule_->getCutClass(); }
  bool hasCutClass() const { return db_rule_->hasCutClass(); }

  void report(utl::Logger* logger) const override
  {
    logger->report("LEF58_MAXSPACING");
  }
  // typeId
  frConstraintTypeEnum typeId() const override
  {
    return frConstraintTypeEnum::frcLef58MaxSpacingConstraint;
  }

 private:
  odb::dbTechLayerMaxSpacingRule* db_rule_{nullptr};
  int cut_class_idx_{-1};
};

class frLef58WidthTableOrthConstraint : public frConstraint
{
 public:
  frLef58WidthTableOrthConstraint(const frCoord horz_spc,
                                  const frCoord vert_spc)
      : horz_spc_(horz_spc), vert_spc_(vert_spc)
  {
  }
  frCoord getHorzSpc() const { return horz_spc_; }
  frCoord getVertSpc() const { return vert_spc_; }
  void report(utl::Logger* logger) const override
  {
    logger->report("LEF58_WIDTHTABLE ORTH");
  }
  // typeId
  frConstraintTypeEnum typeId() const override
  {
    return frConstraintTypeEnum::frcLef58WidthTableOrth;
  }

 private:
  frCoord horz_spc_;
  frCoord vert_spc_;
};

class frNonDefaultRule
{
 public:
  frViaDef* getPrefVia(int z)
  {
    if (z >= (int) vias_.size() || vias_[z].empty()) {
      return nullptr;
    }
    return vias_[z][0];
  }

  void setWidth(frCoord w, int z)
  {
    if (z >= (int) widths_.size()) {
      widths_.resize(z + 1, 0);
    }
    widths_[z] = w;
  }

  void setSpacing(frCoord s, int z)
  {
    if (z >= (int) spacings_.size()) {
      spacings_.resize(z + 1, 0);
    }
    spacings_[z] = s;
  }

  void setMinCuts(int ncuts, int z)
  {
    if (z >= (int) minCuts_.size()) {
      minCuts_.resize(z + 1, 1);
    }
    minCuts_[z] = ncuts;
  }

  void setWireExtension(frCoord we, int z)
  {
    if (z >= (int) wireExtensions_.size()) {
      wireExtensions_.resize(z + 1, 0);
    }
    wireExtensions_[z] = we;
  }

  void setDrEolConstraint(drEolSpacingConstraint con, int z)
  {
    if (z >= (int) drEolCons_.size()) {
      drEolCons_.resize(z + 1, 0);
    }
    drEolCons_[z] = con;
  }

  void addVia(frViaDef* via, int z)
  {
    if (z >= (int) vias_.size()) {
      vias_.resize(z + 1, std::vector<frViaDef*>());
    }
    vias_[z].push_back(via);
  }

  void addViaRule(frViaRuleGenerate* via, int z)
  {
    if (z >= (int) viasRules_.size()) {
      viasRules_.resize(z + 1, std::vector<frViaRuleGenerate*>());
    }
    viasRules_[z].push_back(via);
  }

  void setHardSpacing(bool isHard) { hardSpacing_ = isHard; }

  void setName(const char* n) { name_ = std::string(n); }

  std::string getName() const { return name_; }

  frCoord getWidth(int z) const
  {
    if (z >= (int) widths_.size()) {
      return 0;
    }
    return widths_[z];
  }

  frCoord getSpacing(int z) const
  {
    if (z >= (int) spacings_.size()) {
      return 0;
    }
    return spacings_[z];
  }

  frCoord getWireExtension(int z) const
  {
    if (z >= (int) wireExtensions_.size()) {
      return 0;
    }
    return wireExtensions_[z];
  }

  drEolSpacingConstraint getDrEolSpacingConstraint(int z) const
  {
    if (z >= (int) drEolCons_.size()) {
      return drEolSpacingConstraint();
    }
    return drEolCons_[z];
  }

  int getMinCuts(int z) const
  {
    if (z >= (int) minCuts_.size()) {
      return 1;
    }
    return minCuts_[z];
  }

  const std::vector<frViaDef*>& getVias(int z) const { return vias_[z]; }

  const std::vector<frViaRuleGenerate*>& getViaRules(int z) const
  {
    return viasRules_[z];
  }

  bool isHardSpacing() const { return hardSpacing_; }

 private:
  std::string name_;
  // each vector position is a metal layer
  std::vector<frCoord> widths_;
  std::vector<frCoord> spacings_;
  std::vector<frCoord> wireExtensions_;
  std::vector<drEolSpacingConstraint> drEolCons_;
  std::vector<int> minCuts_;  // min cuts per cut layer

  // vias for each layer
  std::vector<std::vector<frViaDef*>> vias_;
  std::vector<std::vector<frViaRuleGenerate*>> viasRules_;

  bool hardSpacing_ = false;

  // See comments in frTechObject's equivalent fields for the meaning
  std::vector<std::array<ForbiddenRanges, 8>> via2ViaForbiddenLen_;
  std::vector<std::array<ForbiddenRanges, 4>> viaForbiddenTurnLen_;

  friend class FlexRP;
  friend class frTechObject;
};

}  // namespace drt
