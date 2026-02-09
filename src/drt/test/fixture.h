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

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "db/obj/frBlockage.h"
#include "db/obj/frInst.h"
#include "db/obj/frVia.h"
#include "db/tech/frConstraint.h"
#include "db/tech/frTechObject.h"
#include "db/tech/frViaDef.h"
#include "frBaseTypes.h"
#include "frDesign.h"
#include "global.h"
#include "gtest/gtest.h"
#include "odb/db.h"
#include "odb/dbTypes.h"
#include "utl/Logger.h"

namespace odb {
class dbTechLayerCutSpacingTableDefRule;
}

namespace drt {

// General Fixture for tests using db objects.
class Fixture : public ::testing::Test
{
 protected:
  Fixture();

  void addLayer(frTechObject* tech,
                const char* name,
                odb::dbTechLayerType type,
                odb::dbTechLayerDir dir = odb::dbTechLayerDir::NONE);

  odb::dbInst* createDummyInst(odb::dbMaster* master);

  void setupTech(frTechObject* tech);

  void makeDesign();

  std::pair<frMaster*, odb::dbMaster*> makeMacro(const char* name,
                                                 frCoord originX = 0,
                                                 frCoord originY = 0,
                                                 frCoord sizeX = 0,
                                                 frCoord sizeY = 0);

  frBlockage* makeMacroObs(frMaster* master,
                           frCoord xl,
                           frCoord yl,
                           frCoord xh,
                           frCoord yh,
                           frLayerNum lNum = 2,
                           frCoord designRuleWidth = -1);

  frTerm* makeMacroPin(frMaster* master,
                       const std::string& name,
                       frCoord xl,
                       frCoord yl,
                       frCoord xh,
                       frCoord yh,
                       frLayerNum lNum = 2);

  frInst* makeInst(const char* name,
                   frMaster* master,
                   odb::dbMaster* db_master);

  frLef58CornerSpacingConstraint* makeCornerConstraint(
      frLayerNum layer_num,
      frCoord eolWidth = -1,
      frCornerTypeEnum type = frCornerTypeEnum::CONVEX);

  void makeSpacingConstraint(frLayerNum layer_num);

  void makeMetalWidthViaMap(frLayerNum layer_num,
                            odb::dbMetalWidthViaMap* rule);

  void makeKeepOutZoneRule(frLayerNum layer_num,
                           odb::dbTechLayerKeepOutZoneRule* dbRule);

  void makeMinStepConstraint(frLayerNum layer_num);

  frLef58MinStepConstraint* makeMinStep58Constraint(frLayerNum layer_num);

  void makeRectOnlyConstraint(frLayerNum layer_num);

  void makeMinEnclosedAreaConstraint(frLayerNum layer_num);

  void makeSpacingEndOfLineConstraint(frLayerNum layer_num,
                                      frCoord par_space = -1,
                                      frCoord par_within = -1,
                                      bool two_edges = false);

  void makeLef58EolKeepOutConstraint(frLayerNum layer_num,
                                     bool cornerOnly = false,
                                     bool exceptWithin = false,
                                     frCoord withinLow = -10,
                                     frCoord withinHigh = 10,
                                     frCoord forward = 200,
                                     frCoord side = 50,
                                     frCoord backward = 0,
                                     frCoord width = 200);

  frLef58SpacingEndOfLineConstraint* makeLef58SpacingEolConstraint(
      frLayerNum layer_num,
      frCoord space = 200,
      frCoord width = 200,
      frCoord within = 50,
      frCoord end_prl_spacing = 0,
      frCoord end_prl = 0);

  frSpacingRangeConstraint* makeSpacingRangeConstraint(frLayerNum layer_num,
                                                       frCoord spacing,
                                                       frCoord minWidth,
                                                       frCoord maxWidth);

  std::shared_ptr<frLef58SpacingEndOfLineWithinParallelEdgeConstraint>
  makeLef58SpacingEolParEdgeConstraint(frLef58SpacingEndOfLineConstraint* con,
                                       frCoord par_space,
                                       frCoord par_within,
                                       bool two_edges = false);

  std::shared_ptr<frLef58SpacingEndOfLineWithinMaxMinLengthConstraint>
  makeLef58SpacingEolMinMaxLenConstraint(frLef58SpacingEndOfLineConstraint* con,
                                         frCoord min_max_length,
                                         bool max = true,
                                         bool two_sides = true);

  std::shared_ptr<frLef58SpacingEndOfLineWithinEncloseCutConstraint>
  makeLef58SpacingEolCutEncloseConstraint(
      frLef58SpacingEndOfLineConstraint* con,
      frCoord encloseDist = 100,
      frCoord cutToMetalSpacing = 300,
      bool above = false,
      bool below = false,
      bool allCuts = false);

  void makeCutClass(frLayerNum layer_num,
                    std::string name,
                    frCoord width,
                    frCoord height);

  void makeLef58CutSpcTbl(frLayerNum layer_num,
                          odb::dbTechLayerCutSpacingTableDefRule* dbRule);
  void makeLef58TwoWiresForbiddenSpc(
      frLayerNum layer_num,
      odb::dbTechLayerTwoWiresForbiddenSpcRule* dbRule);
  void makeLef58ForbiddenSpc(frLayerNum layer_num,
                             odb::dbTechLayerForbiddenSpacingRule* dbRule);

  frLef58EnclosureConstraint* makeLef58EnclosureConstrainut(
      frLayerNum layer_num,
      int cut_class_idx,
      frCoord width,
      frCoord firstOverhang,
      frCoord secondOverhang);
  void makeMinimumCut(frLayerNum layerNum,
                      frCoord width,
                      frCoord length,
                      frCoord distance,
                      frMinimumcutConnectionEnum connection
                      = frMinimumcutConnectionEnum::UNKNOWN);

  frNet* makeNet(const char* name);

  frViaDef* makeViaDef(const char* name,
                       frLayerNum layer_num,
                       const odb::Point& ll,
                       const odb::Point& ur);

  frVia* makeVia(frViaDef* via, frNet* net, const odb::Point& origin);

  void makePathseg(frNet* net,
                   frLayerNum layer_num,
                   const odb::Point& begin,
                   const odb::Point& end,
                   frUInt4 width = 100,
                   frEndStyleEnum begin_style = frcTruncateEndStyle,
                   frEndStyleEnum end_style = frcTruncateEndStyle);

  void makePathsegExt(frNet* net,
                      frLayerNum layer_num,
                      const odb::Point& begin,
                      const odb::Point& end,
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
      const std::vector<frCoord>& widthTbl,
      const std::vector<std::pair<frCoord, frCoord>>& valTbl);

  frLef58EolExtensionConstraint* makeEolExtensionConstraint(
      frLayerNum layer_num,
      frCoord spacing,
      const std::vector<frCoord>& eol,
      const std::vector<frCoord>& ext,
      bool parallelOnly = false);

  frSpacingTableTwConstraint* makeSpacingTableTwConstraint(
      frLayerNum layer_num,
      const std::vector<frCoord>& widthTbl,
      const std::vector<frCoord>& prlTbl,
      const std::vector<std::vector<frCoord>>& spacingTbl);

  frLef58WidthTableOrthConstraint* makeWidthTblOrthConstraint(
      frLayerNum layer_num,
      frCoord horz_spc,
      frCoord vert_spc);
  void initRegionQuery();
  frLef58CutSpacingConstraint* makeLef58CutSpacingConstraint_parallelOverlap(
      frLayerNum layer_num,
      frCoord spacing);
  frLef58CutSpacingConstraint* makeLef58CutSpacingConstraint_adjacentCut(
      frLayerNum layer_num,
      frCoord spacing,
      int adjacent_cuts,
      int two_cuts,
      frCoord within);
  void makeLef58WrongDirSpcConstraint(
      frLayerNum layer_num,
      odb::dbTechLayerWrongDirSpacingRule* dbRule);
  void makeSpacingTableOrthConstraint(frLayerNum layer_num,
                                      frCoord within,
                                      frCoord spc);
  // Public data members are accessible from inside the test function
  std::unique_ptr<utl::Logger> logger;
  std::unique_ptr<RouterConfiguration> router_cfg;
  std::unique_ptr<frDesign> design;
  frUInt4 numBlockages, numTerms, numMasters, numInsts;
  odb::dbTech* db_tech;

 private:
  odb::dbDatabase* db_;
};

}  // namespace drt
