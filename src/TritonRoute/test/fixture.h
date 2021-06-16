/* Author: Matt Liberty */
/*
 * Copyright (c) 2020, The Regents of the University of California
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

#include "frDesign.h"

using namespace fr;
// General Fixture for tests using db objects.
class Fixture
{
 public:
  Fixture();
  virtual ~Fixture() = default;

  void addLayer(frTechObject* tech,
                const char* name,
                frLayerTypeEnum type,
                frPrefRoutingDirEnum dir = frcNonePrefRoutingDir);

  void setupTech(frTechObject* tech);

  void makeDesign();

  frBlock* makeMacro(const char* name,
                     frCoord originX = 0,
                     frCoord originY = 0,
                     frCoord sizeX = 0,
                     frCoord sizeY = 0);

  frBlockage* makeMacroObs(frBlock* refBlock,
                           frCoord xl,
                           frCoord yl,
                           frCoord xh,
                           frCoord yh,
                           frLayerNum lNum = 2,
                           frCoord designRuleWidth = -1);

  frTerm* makeMacroPin(frBlock* refBlock,
                       std::string name,
                       frCoord xl,
                       frCoord yl,
                       frCoord xh,
                       frCoord yh,
                       frLayerNum lNum = 2);

  frInst* makeInst(const char* name, frBlock* refBlock, frCoord x, frCoord y);

  void makeCornerConstraint(frLayerNum layer_num,
                            frCoord eolWidth = -1,
                            frCornerTypeEnum type = frCornerTypeEnum::CONVEX);

  void makeSpacingConstraint(frLayerNum layer_num);

  void makeMinStepConstraint(frLayerNum layer_num);

  void makeMinStep58Constraint(frLayerNum layer_num);

  void makeRectOnlyConstraint(frLayerNum layer_num);

  void makeMinEnclosedAreaConstraint(frLayerNum layer_num);

  void makeSpacingEndOfLineConstraint(frLayerNum layer_num,
                                      frCoord par_space = -1,
                                      frCoord par_within = -1,
                                      bool two_edges = false);

  void makeLef58EolKeepOutConstraint(frLayerNum layer_num,
                                     bool cornerOnly = false,
                                     frCoord forward = 200,
                                     frCoord side = 50,
                                     frCoord backward = 0,
                                     frCoord width = 200);

  std::shared_ptr<frLef58SpacingEndOfLineConstraint>
  makeLef58SpacingEolConstraint(frLayerNum layer_num,
                                frCoord space = 200,
                                frCoord width = 200,
                                frCoord within = 50);

  std::shared_ptr<frLef58SpacingEndOfLineWithinParallelEdgeConstraint>
  makeLef58SpacingEolParEdgeConstraint(
      std::shared_ptr<frLef58SpacingEndOfLineConstraint> con,
      frCoord par_space,
      frCoord par_within,
      bool two_edges = false);

  std::shared_ptr<frLef58SpacingEndOfLineWithinMaxMinLengthConstraint>
  makeLef58SpacingEolMinMaxLenConstraint(
      std::shared_ptr<frLef58SpacingEndOfLineConstraint> con,
      frCoord min_max_length,
      bool max = true,
      bool two_sides = true);

  std::shared_ptr<frLef58SpacingEndOfLineWithinEncloseCutConstraint>
  makeLef58SpacingEolCutEncloseConstraint(
      std::shared_ptr<frLef58SpacingEndOfLineConstraint> con,
      frCoord encloseDist = 100,
      frCoord cutToMetalSpacing = 300,
      bool above = false,
      bool below = false,
      bool allCuts = false);

  frNet* makeNet(const char* name);

  frViaDef* makeViaDef(const char* name,
                       frLayerNum layer_num,
                       const frPoint& ll,
                       const frPoint& ur);

  frVia* makeVia(frViaDef* via, frNet* net, const frPoint& origin);

  void makePathseg(frNet* net,
                   frLayerNum layer_num,
                   const frPoint& begin,
                   const frPoint& end,
                   frUInt4 width = 100,
                   frEndStyleEnum begin_style = frcTruncateEndStyle,
                   frEndStyleEnum end_style = frcTruncateEndStyle);

  void makePathsegExt(frNet* net,
                      frLayerNum layer_num,
                      const frPoint& begin,
                      const frPoint& end,
                      frUInt4 width = 100)
  {
    makePathseg(net,
                layer_num,
                begin,
                end,
                width,
                frcExtendEndStyle,
                frcExtendEndStyle);
  }

  frSpacingTableInfluenceConstraint* makeSpacingTableInfluenceConstraint(
      frLayerNum layer_num,
      std::vector<frCoord> widthTbl,
      std::vector<std::pair<frCoord, frCoord>> valTbl);

  frLef58EolExtensionConstraint* makeEolExtensionConstraint(
      frLayerNum layer_num,
      frCoord spacing,
      std::vector<frCoord> eol,
      std::vector<frCoord> ext,
      bool parallelOnly = false);

  frSpacingTableTwConstraint* makeSpacingTableTwConstraint(
      frLayerNum layer_num,
      std::vector<frCoord> widthTbl,
      std::vector<frCoord> prlTbl,
      std::vector<std::vector<frCoord>> spacingTbl);
  void initRegionQuery();

  // Public data members are accessible from inside the test function
  std::unique_ptr<fr::Logger> logger;
  std::unique_ptr<frDesign> design;
  frUInt4 numBlockages, numTerms, numRefBlocks, numInsts;
};

// BOOST_TEST wants an operator<< for any type it compares.  We
// don't have those for enums and they are tedious to write.
// Just compare them as integers to avoid this requirement.
#define TEST_ENUM_EQUAL(L, R) \
  BOOST_TEST(static_cast<int>(L) == static_cast<int>(R))
