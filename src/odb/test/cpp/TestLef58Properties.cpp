#include <libgen.h>

#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <string>
#include <utility>
#include <vector>

#include "gtest/gtest.h"
#include "odb/db.h"
#include "odb/dbTypes.h"
#include "odb/defin.h"
#include "odb/defout.h"
#include "odb/lefin.h"
#include "odb/lefout.h"
#include "tst/fixture.h"

namespace odb {

using Fixture = tst::Fixture;

static const std::string prefix("_main/src/odb/test/");

TEST_F(Fixture, lef58_class)
{
  const char* libname = "gscl45nm.lef";
  loadTechAndLib("tech", libname, prefix + "data/gscl45nm.lef");

  odb::dbLib* dbLib = db_->findLib(libname);

  updateLib(dbLib, prefix + "data/lef58class_gscl45nm.lef");

  odb::dbMaster* endcap = db_->findMaster("ENDCAP_BOTTOMEDGE_NOT_A_REAL_CELL");
  EXPECT_TRUE(endcap);

  EXPECT_EQ(endcap->getType(), odb::dbMasterType::ENDCAP_LEF58_BOTTOMEDGE);
  EXPECT_EQ(endcap->getEdgeTypes().size(), 2);

  auto edge_type_itr = endcap->getEdgeTypes().begin();
  EXPECT_EQ((*edge_type_itr)->getEdgeDir(),
            odb::dbMasterEdgeType::EdgeDir::LEFT);
  EXPECT_EQ((*edge_type_itr)->getEdgeType(), "TYPE1");
  ++edge_type_itr;
  EXPECT_EQ((*edge_type_itr)->getEdgeDir(),
            odb::dbMasterEdgeType::EdgeDir::RIGHT);
  EXPECT_EQ((*edge_type_itr)->getEdgeType(), "TYPE2");
}

TEST_F(Fixture, test_default)
{
  const char* libname = "gscl45nm.lef";

  std::string path = prefix + "data/gscl45nm.lef";

  loadTechAndLib("tech", libname, path);

  std::filesystem::create_directory("results");
  path = "results/TestLef58PropertiesDbRW";

  std::ofstream write;
  write.exceptions(std::ifstream::failbit | std::ifstream::badbit
                   | std::ios::eofbit);
  write.open(path, std::ios::binary);

  db_->write(write);

  std::ifstream read;
  read.exceptions(std::ifstream::failbit | std::ifstream::badbit
                  | std::ios::eofbit);
  read.open(path, std::ios::binary);

  dbDatabase* db2 = dbDatabase::create();
  db2->setLogger(&logger_);
  db2->read(read);

  auto dbTech = db2->getTech();
  double distFactor = 2000;
  auto layer = dbTech->findLayer("metal1");
  EXPECT_EQ(layer->getLef58Type(), odb::dbTechLayer::LEF58_TYPE::MIMCAP);
  auto rules = layer->getTechLayerSpacingEolRules();
  EXPECT_EQ(rules.size(), 1);
  odb::dbTechLayerSpacingEolRule* rule
      = (odb::dbTechLayerSpacingEolRule*) *rules.begin();
  EXPECT_EQ(rule->getEolSpace(), 1.3 * distFactor);
  EXPECT_EQ(rule->getEolWidth(), 1.5 * distFactor);
  EXPECT_EQ(rule->isWithinValid(), 1);
  EXPECT_EQ(rule->getEolWithin(), 1.9 * distFactor);
  EXPECT_EQ(rule->isSameMaskValid(), 1);
  EXPECT_EQ(rule->isExceptExactWidthValid(), 1);
  EXPECT_EQ(rule->getExactWidth(), 0.5 * distFactor);
  EXPECT_EQ(rule->getOtherWidth(), 0.3 * distFactor);
  EXPECT_EQ(rule->isParallelEdgeValid(), 1);
  EXPECT_EQ(rule->getParSpace(), 0.2 * distFactor);
  EXPECT_EQ(rule->getParWithin(), 9.1 * distFactor);
  EXPECT_EQ(rule->isParPrlValid(), 1);
  EXPECT_EQ(rule->getParPrl(), 81 * distFactor);
  EXPECT_EQ(rule->isParMinLengthValid(), 1);
  EXPECT_EQ(rule->getParMinLength(), -0.1 * distFactor);
  EXPECT_EQ(rule->isTwoEdgesValid(), 1);
  EXPECT_EQ(rule->isToConcaveCornerValid(), 0);

  auto wrongDir_rules = layer->getTechLayerWrongDirSpacingRules();
  EXPECT_EQ(wrongDir_rules.size(), 1);
  odb::dbTechLayerWrongDirSpacingRule* wrongDir_rule
      = (odb::dbTechLayerWrongDirSpacingRule*) *wrongDir_rules.begin();
  EXPECT_EQ(wrongDir_rule->getWrongdirSpace(), 0.12 * distFactor);
  EXPECT_EQ(wrongDir_rule->isNoneolValid(), 1);
  EXPECT_EQ(wrongDir_rule->getNoneolWidth(), 0.15 * distFactor);
  EXPECT_EQ(wrongDir_rule->getPrlLength(), -0.05 * distFactor);
  EXPECT_EQ(wrongDir_rule->isLengthValid(), 1);
  EXPECT_EQ(wrongDir_rule->getLength(), 0.2 * distFactor);

  auto minStepRules = layer->getTechLayerMinStepRules();
  EXPECT_EQ(minStepRules.size(), 4);
  auto itr = minStepRules.begin();
  odb::dbTechLayerMinStepRule* step_rule = (odb::dbTechLayerMinStepRule*) *itr;
  EXPECT_EQ(step_rule->getMinStepLength(), 0.6 * distFactor);
  EXPECT_EQ(step_rule->getMaxEdges(), 1);
  EXPECT_TRUE(step_rule->isMinAdjLength1Valid());
  EXPECT_FALSE(step_rule->isMinAdjLength2Valid());
  EXPECT_EQ(step_rule->getMinAdjLength1(), 1.0 * distFactor);
  EXPECT_TRUE(step_rule->isConvexCorner());
  itr++;
  step_rule = (odb::dbTechLayerMinStepRule*) *itr;
  EXPECT_TRUE(step_rule->isMinAdjLength2Valid());
  EXPECT_EQ(step_rule->getMinAdjLength2(), 0.15 * distFactor);
  itr++;
  step_rule = (odb::dbTechLayerMinStepRule*) *itr;
  EXPECT_TRUE(step_rule->isMinBetweenLengthValid());
  EXPECT_TRUE(step_rule->isExceptSameCorners());
  EXPECT_EQ(step_rule->getMinBetweenLength(), 0.13 * distFactor);
  itr++;
  step_rule = (odb::dbTechLayerMinStepRule*) *itr;
  EXPECT_TRUE(step_rule->isNoBetweenEol());
  EXPECT_EQ(step_rule->getEolWidth(), 0.5 * distFactor);

  auto corner_rules = layer->getTechLayerCornerSpacingRules();
  EXPECT_EQ(corner_rules.size(), 1);
  odb::dbTechLayerCornerSpacingRule* corner_rule
      = (odb::dbTechLayerCornerSpacingRule*) *corner_rules.begin();
  EXPECT_EQ(corner_rule->getType(),
            odb::dbTechLayerCornerSpacingRule::CONVEXCORNER);
  EXPECT_TRUE(corner_rule->isExceptEol());
  EXPECT_TRUE(corner_rule->isCornerToCorner());
  EXPECT_EQ(corner_rule->getEolWidth(), 0.090 * distFactor);
  std::vector<std::pair<int, int>> spacing;
  std::vector<int> corner_width;
  corner_rule->getSpacingTable(spacing);
  corner_rule->getWidthTable(corner_width);
  EXPECT_EQ(spacing.size(), 1);
  EXPECT_EQ(spacing[0].first, 0.110 * distFactor);
  EXPECT_EQ(spacing[0].second, 0.110 * distFactor);
  EXPECT_EQ(corner_width[0], 0);

  auto spacingTables = layer->getTechLayerSpacingTablePrlRules();
  EXPECT_EQ(spacingTables.size(), 1);
  odb::dbTechLayerSpacingTablePrlRule* spacing_tbl_rule
      = (odb::dbTechLayerSpacingTablePrlRule*) *spacingTables.begin();
  EXPECT_TRUE(spacing_tbl_rule->isWrongDirection());
  EXPECT_FALSE(spacing_tbl_rule->isSameMask());
  EXPECT_TRUE(spacing_tbl_rule->isExceeptEol());
  EXPECT_EQ(spacing_tbl_rule->getEolWidth(), 0.090 * distFactor);
  std::vector<int> width;
  std::vector<int> length;
  std::vector<std::vector<int>> spacing_tbl;
  std::map<unsigned int, std::pair<int, int>> within;
  spacing_tbl_rule->getTable(width, length, spacing_tbl, within);
  EXPECT_EQ(width.size(), 2);
  EXPECT_EQ(length.size(), 2);
  EXPECT_EQ(spacing_tbl.size(), 2);
  EXPECT_EQ(within.size(), 1);
  EXPECT_EQ(spacing_tbl_rule->getSpacing(0, 0), 0.05 * distFactor);

  EXPECT_TRUE(layer->isRightWayOnGridOnly());
  EXPECT_TRUE(layer->isRightWayOnGridOnlyCheckMask());

  EXPECT_TRUE(layer->isRectOnly());
  EXPECT_TRUE(layer->isRectOnlyExceptNonCorePins());

  auto eolextrules = layer->getTechLayerEolExtensionRules();
  EXPECT_EQ(eolextrules.size(), 1);
  odb::dbTechLayerEolExtensionRule* eolextrule
      = (odb::dbTechLayerEolExtensionRule*) *eolextrules.begin();
  EXPECT_TRUE(eolextrule->isParallelOnly());
  EXPECT_EQ(eolextrule->getSpacing(), 0.1 * distFactor);
  std::vector<std::pair<int, int>> ext_tbl;
  eolextrule->getExtensionTable(ext_tbl);
  EXPECT_EQ(ext_tbl.size(), 1);
  EXPECT_EQ(ext_tbl[0].first, 0.11 * distFactor);
  EXPECT_EQ(ext_tbl[0].second, 0.14 * distFactor);

  auto keepoutRules = layer->getTechLayerEolKeepOutRules();
  EXPECT_EQ(keepoutRules.size(), 1);
  auto keepoutRule = (odb::dbTechLayerEolKeepOutRule*) *(keepoutRules.begin());
  EXPECT_EQ(keepoutRule->getEolWidth(), 0.2 * distFactor);
  EXPECT_EQ(keepoutRule->getBackwardExt(), 0.03 * distFactor);
  EXPECT_EQ(keepoutRule->getForwardExt(), 0.1 * distFactor);
  EXPECT_EQ(keepoutRule->getSideExt(), 0.05 * distFactor);
  EXPECT_TRUE(keepoutRule->isExceptWithin());
  EXPECT_EQ(keepoutRule->getWithinLow(), -0.01 * distFactor);
  EXPECT_EQ(keepoutRule->getWithinHigh(), 0.05 * distFactor);
  EXPECT_TRUE(keepoutRule->isClassValid());
  EXPECT_EQ(keepoutRule->getClassName(), "EOL_WIDE");

  auto widthTableRules = layer->getTechLayerWidthTableRules();
  EXPECT_EQ(widthTableRules.size(), 1);
  auto widthTableRule
      = (odb::dbTechLayerWidthTableRule*) *(widthTableRules.begin());
  EXPECT_TRUE(widthTableRule->isOrthogonal());
  EXPECT_TRUE(!widthTableRule->isWrongDirection());
  EXPECT_EQ(widthTableRule->getWidthTable().size(), 5);
  EXPECT_EQ(widthTableRule->getWidthTable().at(1), 0.2 * distFactor);

  auto minCutRules = layer->getTechLayerMinCutRules();
  EXPECT_EQ(minCutRules.size(), 1);
  auto minCutRule = (odb::dbTechLayerMinCutRule*) *(minCutRules.begin());
  EXPECT_TRUE(!minCutRule->isPerCutClass());
  EXPECT_TRUE(minCutRule->isFromAbove());
  EXPECT_TRUE(!minCutRule->isFromBelow());
  EXPECT_TRUE(!minCutRule->isLengthValid());
  EXPECT_TRUE(!minCutRule->isFullyEnclosed());
  EXPECT_TRUE(!minCutRule->isSameMetalOverlap());
  EXPECT_TRUE(minCutRule->isAreaValid());
  EXPECT_TRUE(!minCutRule->isAreaWithinDistValid());
  EXPECT_TRUE(minCutRule->isWithinCutDistValid());
  EXPECT_EQ(minCutRule->getNumCuts(), 2);
  EXPECT_EQ(minCutRule->getWithinCutDist(), 0.05 * distFactor);
  EXPECT_EQ(minCutRule->getWidth(), 0.09 * distFactor);
  EXPECT_EQ(minCutRule->getArea(), 2.0 * distFactor);

  auto cutLayer = dbTech->findLayer("via1");

  auto cutRules = cutLayer->getTechLayerCutClassRules();
  EXPECT_EQ(cutRules.size(), 5);
  odb::dbTechLayerCutClassRule* cut_rule
      = (odb::dbTechLayerCutClassRule*) *cutRules.begin();
  EXPECT_EQ(std::string(cut_rule->getName()), "VA");
  EXPECT_EQ(cut_rule->getWidth(), 0.15 * distFactor);
  EXPECT_EQ(cutLayer->findTechLayerCutClassRule("VA"), cut_rule);

  auto cutSpacingRules = cutLayer->getTechLayerCutSpacingRules();
  EXPECT_EQ(cutSpacingRules.size(), 3);
  int i = 0;
  for (odb::dbTechLayerCutSpacingRule* subRule : cutSpacingRules) {
    if (i == 1) {
      EXPECT_EQ(subRule->getCutSpacing(), 0.3 * distFactor);
      EXPECT_EQ(subRule->getType(),
                odb::dbTechLayerCutSpacingRule::CutSpacingType::LAYER);
      EXPECT_TRUE(subRule->isSameMetal());
      EXPECT_TRUE(subRule->isStack());
      EXPECT_EQ(std::string(subRule->getSecondLayer()->getName()), "metal1");
    } else if (i == 2) {
      EXPECT_EQ(subRule->getCutSpacing(), 0.2 * distFactor);
      EXPECT_EQ(subRule->getType(),
                odb::dbTechLayerCutSpacingRule::CutSpacingType::ADJACENTCUTS);
      EXPECT_EQ(subRule->getAdjacentCuts(), 3);
      EXPECT_EQ(subRule->getTwoCuts(), 1);
    } else {
      EXPECT_EQ(subRule->getCutSpacing(), 0.12 * distFactor);
      EXPECT_EQ(subRule->getType(),
                odb::dbTechLayerCutSpacingRule::CutSpacingType::MAXXY);
    }
    i++;
  }

  auto orths = cutLayer->getTechLayerCutSpacingTableOrthRules();
  auto defs = cutLayer->getTechLayerCutSpacingTableDefRules();
  EXPECT_EQ(orths.size(), 1);
  EXPECT_EQ(defs.size(), 1);
  odb::dbTechLayerCutSpacingTableOrthRule* orth
      = (odb::dbTechLayerCutSpacingTableOrthRule*) *orths.begin();
  odb::dbTechLayerCutSpacingTableDefRule* def
      = (odb::dbTechLayerCutSpacingTableDefRule*) *defs.begin();
  std::vector<std::pair<int, int>> table;
  orth->getSpacingTable(table);
  EXPECT_EQ(table[0].first, 0.2 * distFactor);
  EXPECT_EQ(table[0].second, 0.15 * distFactor);
  EXPECT_EQ(table[1].first, 0.3 * distFactor);
  EXPECT_EQ(table[1].second, 0.25 * distFactor);

  EXPECT_EQ(def->getDefault(), 0.12 * distFactor);
  EXPECT_TRUE(def->isDefaultValid());
  EXPECT_TRUE(def->isSameMask());
  EXPECT_TRUE(def->isSameNet());
  EXPECT_TRUE(def->isLayerValid());
  EXPECT_TRUE(def->isNoStack());
  EXPECT_EQ(std::string(def->getSecondLayer()->getName()), "metal1");
  EXPECT_TRUE(!def->isSameMetal());
  EXPECT_TRUE(def->isPrlForAlignedCut());
  auto spacing1 = def->getSpacing("cls1", true, "cls3", false);
  auto spacing2 = def->getSpacing("cls1", false, "cls4", false);
  EXPECT_EQ(spacing1, 0.2 * distFactor);
  EXPECT_EQ(spacing2, 0.7 * distFactor);

  auto poly = dbTech->findLayer("poly");
  EXPECT_EQ(poly->getLef58Type(), odb::dbTechLayer::NWELL);

  auto encRules = cutLayer->getTechLayerCutEnclosureRules();
  EXPECT_EQ(encRules.size(), 1);
  odb::dbTechLayerCutEnclosureRule* encRule = *encRules.begin();
  EXPECT_EQ(encRule->getFirstOverhang(), 0.05 * distFactor);
  EXPECT_EQ(encRule->getSecondOverhang(), 0.05 * distFactor);
  EXPECT_TRUE(encRule->isWidthValid());
  EXPECT_TRUE(encRule->isIncludeAbutted());
  EXPECT_TRUE(encRule->isAbove());
  EXPECT_TRUE(encRule->isCutClassValid());
  EXPECT_STREQ(encRule->getCutClass()->getName(), "cls1");
  EXPECT_TRUE(!encRule->isBelow());
  EXPECT_EQ(encRule->getMinWidth(), 0.2 * distFactor);

  auto aspRules = cutLayer->getTechLayerArraySpacingRules();
  EXPECT_EQ(aspRules.size(), 1);
  odb::dbTechLayerArraySpacingRule* aspRule = *aspRules.begin();
  EXPECT_TRUE(!aspRule->isLongArray());
  EXPECT_TRUE(aspRule->isParallelOverlap());
  EXPECT_TRUE(aspRule->isViaWidthValid());
  EXPECT_TRUE(!aspRule->isWithinValid());
  EXPECT_STREQ(aspRule->getCutClass()->getName(), "VA");
  auto array_spacing_map = aspRule->getCutsArraySpacing();
  EXPECT_EQ(array_spacing_map.size(), 1);
  EXPECT_EQ(array_spacing_map[3], 0.30 * distFactor);

  auto keepoutzoneRules = cutLayer->getTechLayerKeepOutZoneRules();
  EXPECT_EQ(keepoutzoneRules.size(), 1);
  odb::dbTechLayerKeepOutZoneRule* kozrule = *keepoutzoneRules.begin();
  EXPECT_TRUE(!kozrule->isExceptAlignedEnd());
  EXPECT_TRUE(kozrule->isExceptAlignedSide());
  EXPECT_EQ(kozrule->getFirstCutClass(), "cls1");
  EXPECT_EQ(kozrule->getSecondCutClass(), "cls2");
  EXPECT_EQ(kozrule->getAlignedSpacing(), 0);
  EXPECT_EQ(kozrule->getEndSideExtension(), 1.0 * distFactor);
  EXPECT_EQ(kozrule->getEndForwardExtension(), 2.0 * distFactor);
  EXPECT_EQ(kozrule->getSideSideExtension(), 0.1 * distFactor);
  EXPECT_EQ(kozrule->getSideForwardExtension(), 0.2 * distFactor);
  EXPECT_EQ(kozrule->getSpiralExtension(), 0.05 * distFactor);

  auto maxSpacingRules = cutLayer->getTechLayerMaxSpacingRules();
  EXPECT_EQ(maxSpacingRules.size(), 1);
  auto maxSpcRule
      = (odb::dbTechLayerMaxSpacingRule*) *(maxSpacingRules.begin());
  EXPECT_EQ(maxSpcRule->getMaxSpacing(), 2 * distFactor);
  EXPECT_TRUE(maxSpcRule->hasCutClass());
  EXPECT_EQ(maxSpcRule->getCutClass(), "VA");

  layer = dbTech->findLayer("contact");
  EXPECT_EQ(layer->getLef58Type(), odb::dbTechLayer::LEF58_TYPE::HIGHR);
  layer = dbTech->findLayer("metal2");
  EXPECT_EQ(layer->getLef58Type(), odb::dbTechLayer::LEF58_TYPE::TSVMETAL);

  auto viaMaps = dbTech->getMetalWidthViaMap();
  EXPECT_EQ(viaMaps.size(), 1);
  auto viaMap = (dbMetalWidthViaMap*) (*viaMaps.begin());
  EXPECT_EQ(viaMap->getCutLayer()->getName(), "via1");
  EXPECT_TRUE(!viaMap->isPgVia());
  EXPECT_TRUE(!viaMap->isViaCutClass());
  EXPECT_EQ(viaMap->getBelowLayerWidthLow(), viaMap->getBelowLayerWidthHigh());
  EXPECT_EQ(viaMap->getBelowLayerWidthHigh(), 0.5 * distFactor);
  EXPECT_EQ(viaMap->getAboveLayerWidthLow(), viaMap->getAboveLayerWidthHigh());
  EXPECT_EQ(viaMap->getAboveLayerWidthHigh(), 0.8 * distFactor);
  EXPECT_EQ(viaMap->getViaName(), "M2_M1_via");

  layer = dbTech->findLayer("metal2");
  // Check LEF57_MINSTEP
  auto minStepRules_57 = layer->getTechLayerMinStepRules();
  EXPECT_EQ(minStepRules_57.size(), 7);
  auto itr_57 = minStepRules_57.begin();
  odb::dbTechLayerMinStepRule* step_rule_57
      = (odb::dbTechLayerMinStepRule*) *itr_57;
  EXPECT_EQ(step_rule_57->getMinStepLength(), 0.6 * distFactor);
  EXPECT_EQ(step_rule_57->getMaxEdges(), 1);
  EXPECT_TRUE(step_rule_57->isMinAdjLength1Valid());
  EXPECT_FALSE(step_rule_57->isMinAdjLength2Valid());
  EXPECT_EQ(step_rule_57->getMinAdjLength1(), 1.0 * distFactor);
  EXPECT_TRUE(step_rule_57->isConvexCorner());
  itr_57++;
  step_rule_57 = (odb::dbTechLayerMinStepRule*) *itr_57;
  EXPECT_TRUE(step_rule_57->isMinAdjLength2Valid());
  EXPECT_EQ(step_rule_57->getMinAdjLength2(), 0.15 * distFactor);
  itr_57++;
  step_rule_57 = (odb::dbTechLayerMinStepRule*) *itr_57;
  EXPECT_TRUE(step_rule_57->isMinBetweenLengthValid());
  EXPECT_TRUE(step_rule_57->isExceptSameCorners());
  EXPECT_EQ(step_rule_57->getMinBetweenLength(), 0.13 * distFactor);
  itr_57++;
  step_rule_57 = (odb::dbTechLayerMinStepRule*) *itr_57;
  EXPECT_TRUE(step_rule_57->isNoBetweenEol());
  EXPECT_EQ(step_rule_57->getEolWidth(), 0.5 * distFactor);

  auto areaRules = layer->getTechLayerAreaRules();
  EXPECT_EQ(areaRules.size(), 6);
  int cnt = 0;
  for (odb::dbTechLayerAreaRule* subRule : areaRules) {
    if (cnt == 0) {
      EXPECT_EQ(subRule->getArea(), 0.044 * distFactor);
      EXPECT_EQ(subRule->getMask(), 2);
    }
    if (cnt == 1) {
      EXPECT_EQ(subRule->getArea(), 0.34 * distFactor);
      EXPECT_EQ(subRule->getRectWidth(), 0.12 * distFactor);
    }
    if (cnt == 2) {
      EXPECT_EQ(subRule->getArea(), 1.01 * distFactor);
      EXPECT_EQ(subRule->getExceptMinWidth(), 0.09 * distFactor);
      EXPECT_EQ(subRule->getExceptMinSize().first, 0.1 * distFactor);
      EXPECT_EQ(subRule->getExceptMinSize().second, 0.3 * distFactor);
      EXPECT_EQ(subRule->getExceptStep().first, 0.01 * distFactor);
      EXPECT_EQ(subRule->getExceptStep().second, 0.02 * distFactor);
      EXPECT_EQ(subRule->getExceptEdgeLength(), 0.8 * distFactor);
    }
    if (cnt == 3) {
      EXPECT_EQ(subRule->getArea(), 0.101 * distFactor);
      EXPECT_EQ(subRule->getTrimLayer()->getName(), "metal1");
      EXPECT_EQ(subRule->getOverlap(), 1);
    }
    if (cnt == 4) {
      EXPECT_EQ(subRule->getArea(), 2.34 * distFactor);
      EXPECT_TRUE(subRule->isExceptRectangle());
    }
    if (cnt == 5) {
      EXPECT_EQ(subRule->getArea(), 0.78 * distFactor);
      EXPECT_EQ(subRule->getExceptEdgeLengths().first, 0.3 * distFactor);
      EXPECT_EQ(subRule->getExceptEdgeLengths().second, 0.7 * distFactor);
    }
    cnt++;
  }

  layer = dbTech->findLayer("metal2");
  EXPECT_EQ(layer->getPitchX(), 0.36 * distFactor);
  EXPECT_EQ(layer->getPitchY(), 0.36 * distFactor);
  EXPECT_EQ(layer->getFirstLastPitch(), 0.45 * distFactor);

  // Check LEF57_Spacing
  auto cutLayer_57 = dbTech->findLayer("via2");
  auto cutSpacingRules_57 = cutLayer_57->getTechLayerCutSpacingRules();
  EXPECT_EQ(cutSpacingRules_57.size(), 3);
  int i_57 = 0;
  for (odb::dbTechLayerCutSpacingRule* subRule : cutSpacingRules_57) {
    if (i_57 == 1) {
      EXPECT_EQ(subRule->getCutSpacing(), 0.3 * distFactor);
      EXPECT_EQ(subRule->getType(),
                odb::dbTechLayerCutSpacingRule::CutSpacingType::LAYER);
      EXPECT_TRUE(subRule->isSameMetal());
      EXPECT_TRUE(subRule->isStack());
      EXPECT_EQ(std::string(subRule->getSecondLayer()->getName()), "metal2");
    } else if (i_57 == 2) {
      EXPECT_EQ(subRule->getCutSpacing(), 0.2 * distFactor);
      EXPECT_EQ(subRule->getType(),
                odb::dbTechLayerCutSpacingRule::CutSpacingType::ADJACENTCUTS);
      EXPECT_EQ(subRule->getAdjacentCuts(), 3);
      EXPECT_EQ(subRule->getTwoCuts(), 1);
    } else {
      EXPECT_EQ(subRule->getCutSpacing(), 0.12 * distFactor);
      EXPECT_EQ(subRule->getType(),
                odb::dbTechLayerCutSpacingRule::CutSpacingType::MAXXY);
    }
    i_57++;
  }

  // check LEF58_FORBIDDENSPACING
  layer = dbTech->findLayer("metal2");
  auto forbiddenSpacingRules = layer->getTechLayerForbiddenSpacingRules();
  EXPECT_EQ(forbiddenSpacingRules.size(), 1);
  int c = 0;
  for (odb::dbTechLayerForbiddenSpacingRule* subRule : forbiddenSpacingRules) {
    if (c == 0) {
      EXPECT_EQ(subRule->getForbiddenSpacing().first, 0.05 * distFactor);
      EXPECT_EQ(subRule->getForbiddenSpacing().second, 0.2 * distFactor);
      EXPECT_EQ(subRule->getWidth(), 0.05 * distFactor);
      EXPECT_EQ(subRule->getWithin(), 0.15 * distFactor);
      EXPECT_EQ(subRule->getPrl(), 0.015 * distFactor);
      EXPECT_EQ(subRule->getTwoEdges(), 0.06 * distFactor);
    }
    c++;
  }

  layer = dbTech->findLayer("metal3");
  forbiddenSpacingRules = layer->getTechLayerForbiddenSpacingRules();
  EXPECT_EQ(forbiddenSpacingRules.size(), 1);
  c = 0;
  for (odb::dbTechLayerForbiddenSpacingRule* subRule : forbiddenSpacingRules) {
    if (c == 0) {
      EXPECT_EQ(subRule->getForbiddenSpacing().first, 0.1 * distFactor);
      EXPECT_EQ(subRule->getForbiddenSpacing().second, 0.3 * distFactor);
      EXPECT_EQ(subRule->getWidth(), 0.5 * distFactor);
      EXPECT_EQ(subRule->getPrl(), 0.02 * distFactor);
      EXPECT_EQ(subRule->getTwoEdges(), 0.12 * distFactor);
    }
    c++;
  }
  // check LEF58_TWOWIRESFORBIDDENSPACING
  layer = dbTech->findLayer("metal2");
  auto TWforbiddenSpacingRules = layer->getTechLayerTwoWiresForbiddenSpcRules();
  EXPECT_EQ(TWforbiddenSpacingRules.size(), 2);
  c = 0;
  for (auto* subRule : TWforbiddenSpacingRules) {
    if (c == 0) {
      EXPECT_EQ(subRule->getMinSpacing(), 0.16 * distFactor);
      EXPECT_EQ(subRule->getMaxSpacing(), 0.2 * distFactor);
      EXPECT_EQ(subRule->getMinSpanLength(), 0.05 * distFactor);
      EXPECT_EQ(subRule->getMaxSpanLength(), 0.08 * distFactor);
      EXPECT_EQ(subRule->getPrl(), 0);
      EXPECT_TRUE(subRule->isMinExactSpanLength());
      EXPECT_TRUE(subRule->isMaxExactSpanLength());
    } else {
      EXPECT_EQ(subRule->getPrl(), -0.5 * distFactor);
      EXPECT_TRUE(!subRule->isMinExactSpanLength());
      EXPECT_TRUE(!subRule->isMaxExactSpanLength());
    }
    c++;
  }

  // LEF58_CELLEDGESPACINGTABLE
  auto cell_edge_spacing_tbl = dbTech->getCellEdgeSpacingTable();
  EXPECT_EQ(cell_edge_spacing_tbl.size(), 4);
  auto edge_spc_it = cell_edge_spacing_tbl.begin();
  auto edge_spc = *edge_spc_it;
  EXPECT_EQ(edge_spc->getFirstEdgeType(), "GROUP1");
  EXPECT_EQ(edge_spc->getSecondEdgeType(), "GROUP2");
  EXPECT_EQ(edge_spc->getSpacing(), 0.1 * 2000);
  EXPECT_TRUE(!edge_spc->isExact() && edge_spc->isExceptAbutted()
              && !edge_spc->isExceptNonFillerInBetween()
              && !edge_spc->isOptional() && !edge_spc->isSoft());
  edge_spc = *(++edge_spc_it);
  EXPECT_EQ(edge_spc->getFirstEdgeType(), "GROUP1");
  EXPECT_EQ(edge_spc->getSecondEdgeType(), "GROUP1");
  EXPECT_EQ(edge_spc->getSpacing(), 0.2 * 2000);
  EXPECT_TRUE(!edge_spc->isExact() && !edge_spc->isExceptAbutted()
              && !edge_spc->isExceptNonFillerInBetween()
              && edge_spc->isOptional() && !edge_spc->isSoft());
  edge_spc = *(++edge_spc_it);
  EXPECT_EQ(edge_spc->getFirstEdgeType(), "GROUP2");
  EXPECT_EQ(edge_spc->getSecondEdgeType(), "DEFAULT");
  EXPECT_EQ(edge_spc->getSpacing(), 0.3 * 2000);
  EXPECT_TRUE(edge_spc->isExact() && !edge_spc->isExceptAbutted()
              && !edge_spc->isExceptNonFillerInBetween()
              && !edge_spc->isOptional() && !edge_spc->isSoft());
  edge_spc = *(++edge_spc_it);
  EXPECT_EQ(edge_spc->getFirstEdgeType(), "GROUP2");
  EXPECT_EQ(edge_spc->getSecondEdgeType(), "GROUP2");
  EXPECT_EQ(edge_spc->getSpacing(), 0.4 * 2000);
  EXPECT_TRUE(!edge_spc->isExact() && !edge_spc->isExceptAbutted()
              && !edge_spc->isExceptNonFillerInBetween()
              && !edge_spc->isOptional() && !edge_spc->isSoft());

  // LEF57_ANTENNAGATEPLUSDIFF
  layer = dbTech->findLayer("via1");
  EXPECT_TRUE(layer->hasDefaultAntennaRule());
  EXPECT_EQ(layer->getDefaultAntennaRule()->getGatePlusDiffFactor(), 2.0);

  layer = dbTech->findLayer("metal2");
  EXPECT_TRUE(layer->hasDefaultAntennaRule());
  EXPECT_EQ(layer->getDefaultAntennaRule()->getGatePlusDiffFactor(), 4.0);

  // LEF58_VOLTAGESPACINGTABLE
  layer = dbTech->findLayer("metal2");
  auto voltage_spacing_table = layer->getTechLayerVoltageSpacings();
  EXPECT_EQ(voltage_spacing_table.size(), 2);
  auto voltage_spacing_itr = voltage_spacing_table.begin();
  auto voltage_spacing = *voltage_spacing_itr;
  EXPECT_FALSE(voltage_spacing->isTocutBelow());
  EXPECT_FALSE(voltage_spacing->isTocutAbove());
  auto voltage_table = voltage_spacing->getTable();
  EXPECT_EQ(voltage_table.size(), 5);
  EXPECT_EQ(voltage_table.at(1.2), 150);
  EXPECT_EQ(voltage_table.at(1.8), 150);
  EXPECT_EQ(voltage_table.at(2.5), 150);
  EXPECT_EQ(voltage_table.at(3.3), 300);
  EXPECT_EQ(voltage_table.at(5.0), 300);
  voltage_spacing = *(++voltage_spacing_itr);
  EXPECT_TRUE(voltage_spacing->isTocutBelow());
  EXPECT_TRUE(voltage_spacing->isTocutAbove());
  voltage_table = voltage_spacing->getTable();
  EXPECT_EQ(voltage_table.size(), 3);
  EXPECT_EQ(voltage_table.at(1.2), 150);
  EXPECT_EQ(voltage_table.at(3.3), 300);
  EXPECT_EQ(voltage_table.at(5.0), 600);

  layer = dbTech->findLayer("metal1");
  voltage_spacing_table = layer->getTechLayerVoltageSpacings();
  EXPECT_EQ(voltage_spacing_table.size(), 1);
  voltage_spacing_itr = voltage_spacing_table.begin();
  voltage_spacing = *voltage_spacing_itr;
  EXPECT_FALSE(voltage_spacing->isTocutBelow());
  EXPECT_FALSE(voltage_spacing->isTocutAbove());
  voltage_table = voltage_spacing->getTable();
  EXPECT_EQ(voltage_table.size(), 5);
  EXPECT_EQ(voltage_table.at(1.2), 130);
  EXPECT_EQ(voltage_table.at(1.8), 130);
  EXPECT_EQ(voltage_table.at(2.5), 130);
  EXPECT_EQ(voltage_table.at(3.3), 260);
  EXPECT_EQ(voltage_table.at(5.0), 520);

  layer = dbTech->findLayer("metal3");
  voltage_spacing_table = layer->getTechLayerVoltageSpacings();
  EXPECT_EQ(voltage_spacing_table.size(), 1);
  voltage_spacing_itr = voltage_spacing_table.begin();
  voltage_spacing = *voltage_spacing_itr;
  EXPECT_TRUE(voltage_spacing->isTocutBelow());
  EXPECT_FALSE(voltage_spacing->isTocutAbove());
  voltage_table = voltage_spacing->getTable();
  EXPECT_EQ(voltage_table.size(), 5);
  EXPECT_EQ(voltage_table.at(1.2), 140);
  EXPECT_EQ(voltage_table.at(1.8), 140);
  EXPECT_EQ(voltage_table.at(2.5), 140);
  EXPECT_EQ(voltage_table.at(3.3), 280);
  EXPECT_EQ(voltage_table.at(5.0), 420);

  // LEF58_MINWIDTH
  layer = dbTech->findLayer("metal3");
  EXPECT_EQ(layer->getMinWidth(), 140);
  EXPECT_EQ(layer->getWrongWayMinWidth(), 140);

  layer = dbTech->findLayer("metal4");
  EXPECT_EQ(layer->getMinWidth(), 280);
  EXPECT_EQ(layer->getWrongWayMinWidth(), 560);

  layer = dbTech->findLayer("metal5");
  EXPECT_EQ(layer->getMinWidth(), 280);
  EXPECT_EQ(layer->getWrongWayMinWidth(), 560);
}
}  // namespace odb
