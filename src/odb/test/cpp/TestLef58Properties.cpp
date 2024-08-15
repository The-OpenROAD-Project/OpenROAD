#define BOOST_TEST_MODULE TestLef58Properties
#include <libgen.h>

#include <boost/test/included/unit_test.hpp>
#include <fstream>
#include <iostream>

#include "env.h"
#include "odb/db.h"
#include "odb/defin.h"
#include "odb/defout.h"
#include "odb/lefin.h"
#include "odb/lefout.h"
#include "utl/Logger.h"

namespace odb {

BOOST_AUTO_TEST_SUITE(test_suite)

BOOST_AUTO_TEST_CASE(lef58_class)
{
  utl::Logger* logger = new utl::Logger();
  dbDatabase* db1 = dbDatabase::create();
  db1->setLogger(logger);
  lefin lefParser(db1, logger, false);

  const char* libname = "gscl45nm.lef";
  std::string path = testTmpPath("data", "gscl45nm.lef");
  lefParser.createTechAndLib("tech", libname, path.c_str());

  odb::dbLib* dbLib = db1->findLib(libname);

  path = testTmpPath("data", "lef58class_gscl45nm.lef");
  lefParser.updateLib(dbLib, path.c_str());

  odb::dbMaster* endcap = db1->findMaster("ENDCAP_BOTTOMEDGE_NOT_A_REAL_CELL");
  BOOST_CHECK(endcap);

  BOOST_TEST(endcap->getType() == odb::dbMasterType::ENDCAP_LEF58_BOTTOMEDGE);
}

BOOST_AUTO_TEST_CASE(test_default)
{
  utl::Logger* logger = new utl::Logger();
  dbDatabase* db1 = dbDatabase::create();
  dbDatabase* db2 = dbDatabase::create();
  db1->setLogger(logger);
  db2->setLogger(logger);
  lefin lefParser(db1, logger, false);
  const char* libname = "gscl45nm.lef";

  std::string path = testTmpPath("data", "gscl45nm.lef");

  lefParser.createTechAndLib("tech", libname, path.c_str());

  path = testTmpPath("results", "TestLef58PropertiesDbRW");

  std::ofstream write;
  write.exceptions(std::ifstream::failbit | std::ifstream::badbit
                   | std::ios::eofbit);
  write.open(path, std::ios::binary);

  db1->write(write);

  std::ifstream read;
  read.exceptions(std::ifstream::failbit | std::ifstream::badbit
                  | std::ios::eofbit);
  read.open(path, std::ios::binary);

  db2->read(read);

  auto dbTech = db2->getTech();
  double distFactor = 2000;
  auto layer = dbTech->findLayer("metal1");
  BOOST_TEST(layer->getLef58Type() == odb::dbTechLayer::LEF58_TYPE::MIMCAP);
  auto rules = layer->getTechLayerSpacingEolRules();
  BOOST_TEST(rules.size() == 1);
  odb::dbTechLayerSpacingEolRule* rule
      = (odb::dbTechLayerSpacingEolRule*) *rules.begin();
  BOOST_TEST(rule->getEolSpace() == 1.3 * distFactor);
  BOOST_TEST(rule->getEolWidth() == 1.5 * distFactor);
  BOOST_TEST(rule->isWithinValid() == 1);
  BOOST_TEST(rule->getEolWithin() == 1.9 * distFactor);
  BOOST_TEST(rule->isSameMaskValid() == 1);
  BOOST_TEST(rule->isExceptExactWidthValid() == 1);
  BOOST_TEST(rule->getExactWidth() == 0.5 * distFactor);
  BOOST_TEST(rule->getOtherWidth() == 0.3 * distFactor);
  BOOST_TEST(rule->isParallelEdgeValid() == 1);
  BOOST_TEST(rule->getParSpace() == 0.2 * distFactor);
  BOOST_TEST(rule->getParWithin() == 9.1 * distFactor);
  BOOST_TEST(rule->isParPrlValid() == 1);
  BOOST_TEST(rule->getParPrl() == 81 * distFactor);
  BOOST_TEST(rule->isParMinLengthValid() == 1);
  BOOST_TEST(rule->getParMinLength() == -0.1 * distFactor);
  BOOST_TEST(rule->isTwoEdgesValid() == 1);
  BOOST_TEST(rule->isToConcaveCornerValid() == 0);

  auto wrongDir_rules = layer->getTechLayerWrongDirSpacingRules();
  BOOST_TEST(wrongDir_rules.size() == 1);
  odb::dbTechLayerWrongDirSpacingRule* wrongDir_rule
      = (odb::dbTechLayerWrongDirSpacingRule*) *wrongDir_rules.begin();
  BOOST_TEST(wrongDir_rule->getWrongdirSpace() == 0.12 * distFactor);
  BOOST_TEST(wrongDir_rule->isNoneolValid() == 1);
  BOOST_TEST(wrongDir_rule->getNoneolWidth() == 0.15 * distFactor);
  BOOST_TEST(wrongDir_rule->getPrlLength() == -0.05 * distFactor);
  BOOST_TEST(wrongDir_rule->isLengthValid() == 1);
  BOOST_TEST(wrongDir_rule->getLength() == 0.2 * distFactor);

  auto minStepRules = layer->getTechLayerMinStepRules();
  BOOST_TEST(minStepRules.size() == 4);
  auto itr = minStepRules.begin();
  odb::dbTechLayerMinStepRule* step_rule = (odb::dbTechLayerMinStepRule*) *itr;
  BOOST_TEST(step_rule->getMinStepLength() == 0.6 * distFactor);
  BOOST_TEST(step_rule->getMaxEdges() == 1);
  BOOST_TEST(step_rule->isMinAdjLength1Valid() == true);
  BOOST_TEST(step_rule->isMinAdjLength2Valid() == false);
  BOOST_TEST(step_rule->getMinAdjLength1() == 1.0 * distFactor);
  BOOST_TEST(step_rule->isConvexCorner());
  itr++;
  step_rule = (odb::dbTechLayerMinStepRule*) *itr;
  BOOST_TEST(step_rule->isMinAdjLength2Valid());
  BOOST_TEST(step_rule->getMinAdjLength2() == 0.15 * distFactor);
  itr++;
  step_rule = (odb::dbTechLayerMinStepRule*) *itr;
  BOOST_TEST(step_rule->isMinBetweenLengthValid());
  BOOST_TEST(step_rule->isExceptSameCorners());
  BOOST_TEST(step_rule->getMinBetweenLength() == 0.13 * distFactor);
  itr++;
  step_rule = (odb::dbTechLayerMinStepRule*) *itr;
  BOOST_TEST(step_rule->isNoBetweenEol());
  BOOST_TEST(step_rule->getEolWidth() == 0.5 * distFactor);

  auto corner_rules = layer->getTechLayerCornerSpacingRules();
  BOOST_TEST(corner_rules.size() == 1);
  odb::dbTechLayerCornerSpacingRule* corner_rule
      = (odb::dbTechLayerCornerSpacingRule*) *corner_rules.begin();
  BOOST_TEST(corner_rule->getType()
             == odb::dbTechLayerCornerSpacingRule::CONVEXCORNER);
  BOOST_TEST(corner_rule->isExceptEol());
  BOOST_TEST(corner_rule->isCornerToCorner());
  BOOST_TEST(corner_rule->getEolWidth() == 0.090 * distFactor);
  std::vector<std::pair<int, int>> spacing;
  std::vector<int> corner_width;
  corner_rule->getSpacingTable(spacing);
  corner_rule->getWidthTable(corner_width);
  BOOST_TEST(spacing.size() == 1);
  BOOST_TEST(spacing[0].first == 0.110 * distFactor);
  BOOST_TEST(spacing[0].second == 0.110 * distFactor);
  BOOST_TEST(corner_width[0] == 0);

  auto spacingTables = layer->getTechLayerSpacingTablePrlRules();
  BOOST_TEST(spacingTables.size() == 1);
  odb::dbTechLayerSpacingTablePrlRule* spacing_tbl_rule
      = (odb::dbTechLayerSpacingTablePrlRule*) *spacingTables.begin();
  BOOST_TEST(spacing_tbl_rule->isWrongDirection() == true);
  BOOST_TEST(spacing_tbl_rule->isSameMask() == false);
  BOOST_TEST(spacing_tbl_rule->isExceeptEol() == true);
  BOOST_TEST(spacing_tbl_rule->getEolWidth() == 0.090 * distFactor);
  std::vector<int> width;
  std::vector<int> length;
  std::vector<std::vector<int>> spacing_tbl;
  std::map<unsigned int, std::pair<int, int>> within;
  spacing_tbl_rule->getTable(width, length, spacing_tbl, within);
  BOOST_TEST(width.size() == 2);
  BOOST_TEST(length.size() == 2);
  BOOST_TEST(spacing_tbl.size() == 2);
  BOOST_TEST(within.size() == 1);
  BOOST_TEST(spacing_tbl_rule->getSpacing(0, 0) == 0.05 * distFactor);

  BOOST_TEST(layer->isRightWayOnGridOnly() == true);
  BOOST_TEST(layer->isRightWayOnGridOnlyCheckMask() == true);

  BOOST_TEST(layer->isRectOnly() == true);
  BOOST_TEST(layer->isRectOnlyExceptNonCorePins() == true);

  auto eolextrules = layer->getTechLayerEolExtensionRules();
  BOOST_TEST(eolextrules.size() == 1);
  odb::dbTechLayerEolExtensionRule* eolextrule
      = (odb::dbTechLayerEolExtensionRule*) *eolextrules.begin();
  BOOST_TEST(eolextrule->isParallelOnly());
  BOOST_TEST(eolextrule->getSpacing() == 0.1 * distFactor);
  std::vector<std::pair<int, int>> ext_tbl;
  eolextrule->getExtensionTable(ext_tbl);
  BOOST_TEST(ext_tbl.size() == 1);
  BOOST_TEST(ext_tbl[0].first == 0.11 * distFactor);
  BOOST_TEST(ext_tbl[0].second == 0.14 * distFactor);

  auto keepoutRules = layer->getTechLayerEolKeepOutRules();
  BOOST_TEST(keepoutRules.size() == 1);
  auto keepoutRule = (odb::dbTechLayerEolKeepOutRule*) *(keepoutRules.begin());
  BOOST_TEST(keepoutRule->getEolWidth() == 0.2 * distFactor);
  BOOST_TEST(keepoutRule->getBackwardExt() == 0.03 * distFactor);
  BOOST_TEST(keepoutRule->getForwardExt() == 0.1 * distFactor);
  BOOST_TEST(keepoutRule->getSideExt() == 0.05 * distFactor);
  BOOST_TEST(keepoutRule->isExceptWithin());
  BOOST_TEST(keepoutRule->getWithinLow() == -0.01 * distFactor);
  BOOST_TEST(keepoutRule->getWithinHigh() == 0.05 * distFactor);
  BOOST_TEST(keepoutRule->isClassValid());
  BOOST_TEST(keepoutRule->getClassName() == "EOL_WIDE");

  auto widthTableRules = layer->getTechLayerWidthTableRules();
  BOOST_TEST(widthTableRules.size() == 1);
  auto widthTableRule
      = (odb::dbTechLayerWidthTableRule*) *(widthTableRules.begin());
  BOOST_TEST(widthTableRule->isOrthogonal());
  BOOST_TEST(!widthTableRule->isWrongDirection());
  BOOST_TEST(widthTableRule->getWidthTable().size() == 5);
  BOOST_TEST(widthTableRule->getWidthTable().at(1) == 0.2 * distFactor);

  auto minCutRules = layer->getTechLayerMinCutRules();
  BOOST_TEST(minCutRules.size() == 1);
  auto minCutRule = (odb::dbTechLayerMinCutRule*) *(minCutRules.begin());
  BOOST_TEST(!minCutRule->isPerCutClass());
  BOOST_TEST(minCutRule->isFromAbove());
  BOOST_TEST(!minCutRule->isFromBelow());
  BOOST_TEST(!minCutRule->isLengthValid());
  BOOST_TEST(!minCutRule->isFullyEnclosed());
  BOOST_TEST(!minCutRule->isSameMetalOverlap());
  BOOST_TEST(minCutRule->isAreaValid());
  BOOST_TEST(!minCutRule->isAreaWithinDistValid());
  BOOST_TEST(minCutRule->isWithinCutDistValid());
  BOOST_TEST(minCutRule->getNumCuts() == 2);
  BOOST_TEST(minCutRule->getWithinCutDist() == 0.05 * distFactor);
  BOOST_TEST(minCutRule->getWidth() == 0.09 * distFactor);
  BOOST_TEST(minCutRule->getArea() == 2.0 * distFactor);

  auto cutLayer = dbTech->findLayer("via1");

  auto cutRules = cutLayer->getTechLayerCutClassRules();
  BOOST_TEST(cutRules.size() == 5);
  odb::dbTechLayerCutClassRule* cut_rule
      = (odb::dbTechLayerCutClassRule*) *cutRules.begin();
  BOOST_TEST(std::string(cut_rule->getName()) == "VA");
  BOOST_TEST(cut_rule->getWidth() == 0.15 * distFactor);
  BOOST_TEST((cutLayer->findTechLayerCutClassRule("VA") == cut_rule));

  auto cutSpacingRules = cutLayer->getTechLayerCutSpacingRules();
  BOOST_TEST(cutSpacingRules.size() == 3);
  int i = 0;
  for (odb::dbTechLayerCutSpacingRule* subRule : cutSpacingRules) {
    if (i == 1) {
      BOOST_TEST(subRule->getCutSpacing() == 0.3 * distFactor);
      BOOST_TEST(subRule->getType()
                 == odb::dbTechLayerCutSpacingRule::CutSpacingType::LAYER);
      BOOST_TEST(subRule->isSameMetal());
      BOOST_TEST(subRule->isStack());
      BOOST_TEST(std::string(subRule->getSecondLayer()->getName()) == "metal1");
    } else if (i == 2) {
      BOOST_TEST(subRule->getCutSpacing() == 0.2 * distFactor);
      BOOST_TEST(
          subRule->getType()
          == odb::dbTechLayerCutSpacingRule::CutSpacingType::ADJACENTCUTS);
      BOOST_TEST(subRule->getAdjacentCuts() == 3);
      BOOST_TEST(subRule->getTwoCuts() == 1);
    } else {
      BOOST_TEST(subRule->getCutSpacing() == 0.12 * distFactor);
      BOOST_TEST(subRule->getType()
                 == odb::dbTechLayerCutSpacingRule::CutSpacingType::MAXXY);
    }
    i++;
  }

  auto orths = cutLayer->getTechLayerCutSpacingTableOrthRules();
  auto defs = cutLayer->getTechLayerCutSpacingTableDefRules();
  BOOST_TEST(orths.size() == 1);
  BOOST_TEST(defs.size() == 1);
  odb::dbTechLayerCutSpacingTableOrthRule* orth
      = (odb::dbTechLayerCutSpacingTableOrthRule*) *orths.begin();
  odb::dbTechLayerCutSpacingTableDefRule* def
      = (odb::dbTechLayerCutSpacingTableDefRule*) *defs.begin();
  std::vector<std::pair<int, int>> table;
  orth->getSpacingTable(table);
  BOOST_TEST(table[0].first == 0.2 * distFactor);
  BOOST_TEST(table[0].second == 0.15 * distFactor);
  BOOST_TEST(table[1].first == 0.3 * distFactor);
  BOOST_TEST(table[1].second == 0.25 * distFactor);

  BOOST_TEST(def->getDefault() == 0.12 * distFactor);
  BOOST_TEST(def->isDefaultValid());
  BOOST_TEST(def->isSameMask());
  BOOST_TEST(def->isSameNet());
  BOOST_TEST(def->isLayerValid());
  BOOST_TEST(def->isNoStack());
  BOOST_TEST(std::string(def->getSecondLayer()->getName()) == "metal1");
  BOOST_TEST(!def->isSameMetal());
  BOOST_TEST(def->isPrlForAlignedCut());
  auto spacing1 = def->getSpacing("cls1", true, "cls3", false);
  auto spacing2 = def->getSpacing("cls1", false, "cls4", false);
  BOOST_TEST(spacing1 == 0.2 * distFactor);
  BOOST_TEST(spacing2 == 0.7 * distFactor);

  auto poly = dbTech->findLayer("poly");
  BOOST_TEST(poly->getLef58Type() == odb::dbTechLayer::NWELL);

  auto encRules = cutLayer->getTechLayerCutEnclosureRules();
  BOOST_TEST(encRules.size() == 1);
  odb::dbTechLayerCutEnclosureRule* encRule = *encRules.begin();
  BOOST_TEST(encRule->getFirstOverhang() == 0.05 * distFactor);
  BOOST_TEST(encRule->getSecondOverhang() == 0.05 * distFactor);
  BOOST_TEST(encRule->isWidthValid());
  BOOST_TEST(encRule->isIncludeAbutted());
  BOOST_TEST(encRule->isAbove());
  BOOST_TEST(encRule->isCutClassValid());
  BOOST_TEST(encRule->getCutClass()->getName() == "cls1");
  BOOST_TEST(!encRule->isBelow());
  BOOST_TEST(encRule->getMinWidth() == 0.2 * distFactor);

  auto aspRules = cutLayer->getTechLayerArraySpacingRules();
  BOOST_TEST(aspRules.size() == 1);
  odb::dbTechLayerArraySpacingRule* aspRule = *aspRules.begin();
  BOOST_TEST(!aspRule->isLongArray());
  BOOST_TEST(aspRule->isParallelOverlap());
  BOOST_TEST(aspRule->isViaWidthValid());
  BOOST_TEST(!aspRule->isWithinValid());
  BOOST_TEST(aspRule->getCutClass()->getName() == "VA");
  auto array_spacing_map = aspRule->getCutsArraySpacing();
  BOOST_TEST(array_spacing_map.size() == 1);
  BOOST_TEST(array_spacing_map[3] == 0.30 * distFactor);

  auto keepoutzoneRules = cutLayer->getTechLayerKeepOutZoneRules();
  BOOST_TEST(keepoutzoneRules.size() == 1);
  odb::dbTechLayerKeepOutZoneRule* kozrule = *keepoutzoneRules.begin();
  BOOST_TEST(!kozrule->isExceptAlignedEnd());
  BOOST_TEST(kozrule->isExceptAlignedSide());
  BOOST_TEST(kozrule->getFirstCutClass() == "cls1");
  BOOST_TEST(kozrule->getSecondCutClass() == "cls2");
  BOOST_TEST(kozrule->getAlignedSpacing() == 0);
  BOOST_TEST(kozrule->getEndSideExtension() == 1.0 * distFactor);
  BOOST_TEST(kozrule->getEndForwardExtension() == 2.0 * distFactor);
  BOOST_TEST(kozrule->getSideSideExtension() == 0.1 * distFactor);
  BOOST_TEST(kozrule->getSideForwardExtension() == 0.2 * distFactor);
  BOOST_TEST(kozrule->getSpiralExtension() == 0.05 * distFactor);

  auto maxSpacingRules = cutLayer->getTechLayerMaxSpacingRules();
  BOOST_TEST(maxSpacingRules.size() == 1);
  auto maxSpcRule
      = (odb::dbTechLayerMaxSpacingRule*) *(maxSpacingRules.begin());
  BOOST_TEST(maxSpcRule->getMaxSpacing() == 2 * distFactor);
  BOOST_TEST(maxSpcRule->hasCutClass());
  BOOST_TEST(maxSpcRule->getCutClass() == "VA");

  layer = dbTech->findLayer("contact");
  BOOST_TEST(layer->getLef58Type() == odb::dbTechLayer::LEF58_TYPE::HIGHR);
  layer = dbTech->findLayer("metal2");
  BOOST_TEST(layer->getLef58Type() == odb::dbTechLayer::LEF58_TYPE::TSVMETAL);

  auto viaMaps = dbTech->getMetalWidthViaMap();
  BOOST_TEST(viaMaps.size() == 1);
  auto viaMap = (dbMetalWidthViaMap*) (*viaMaps.begin());
  BOOST_TEST(viaMap->getCutLayer()->getName() == "via1");
  BOOST_TEST(!viaMap->isPgVia());
  BOOST_TEST(!viaMap->isViaCutClass());
  BOOST_TEST(viaMap->getBelowLayerWidthLow()
             == viaMap->getBelowLayerWidthHigh());
  BOOST_TEST(viaMap->getBelowLayerWidthHigh() == 0.5 * distFactor);
  BOOST_TEST(viaMap->getAboveLayerWidthLow()
             == viaMap->getAboveLayerWidthHigh());
  BOOST_TEST(viaMap->getAboveLayerWidthHigh() == 0.8 * distFactor);
  BOOST_TEST(viaMap->getViaName() == "M2_M1_via");

  layer = dbTech->findLayer("metal2");
  // Check LEF57_MINSTEP
  auto minStepRules_57 = layer->getTechLayerMinStepRules();
  BOOST_TEST(minStepRules_57.size() == 7);
  auto itr_57 = minStepRules_57.begin();
  odb::dbTechLayerMinStepRule* step_rule_57
      = (odb::dbTechLayerMinStepRule*) *itr_57;
  BOOST_TEST(step_rule_57->getMinStepLength() == 0.6 * distFactor);
  BOOST_TEST(step_rule_57->getMaxEdges() == 1);
  BOOST_TEST(step_rule_57->isMinAdjLength1Valid() == true);
  BOOST_TEST(step_rule_57->isMinAdjLength2Valid() == false);
  BOOST_TEST(step_rule_57->getMinAdjLength1() == 1.0 * distFactor);
  BOOST_TEST(step_rule_57->isConvexCorner());
  itr_57++;
  step_rule_57 = (odb::dbTechLayerMinStepRule*) *itr_57;
  BOOST_TEST(step_rule_57->isMinAdjLength2Valid());
  BOOST_TEST(step_rule_57->getMinAdjLength2() == 0.15 * distFactor);
  itr_57++;
  step_rule_57 = (odb::dbTechLayerMinStepRule*) *itr_57;
  BOOST_TEST(step_rule_57->isMinBetweenLengthValid());
  BOOST_TEST(step_rule_57->isExceptSameCorners());
  BOOST_TEST(step_rule_57->getMinBetweenLength() == 0.13 * distFactor);
  itr_57++;
  step_rule_57 = (odb::dbTechLayerMinStepRule*) *itr_57;
  BOOST_TEST(step_rule_57->isNoBetweenEol());
  BOOST_TEST(step_rule_57->getEolWidth() == 0.5 * distFactor);

  auto areaRules = layer->getTechLayerAreaRules();
  BOOST_TEST(areaRules.size() == 6);
  int cnt = 0;
  for (odb::dbTechLayerAreaRule* subRule : areaRules) {
    if (cnt == 0) {
      BOOST_TEST(subRule->getArea() == 0.044 * distFactor);
      BOOST_TEST(subRule->getMask() == 2);
    }
    if (cnt == 1) {
      BOOST_TEST(subRule->getArea() == 0.34 * distFactor);
      BOOST_TEST(subRule->getRectWidth() == 0.12 * distFactor);
    }
    if (cnt == 2) {
      BOOST_TEST(subRule->getArea() == 1.01 * distFactor);
      BOOST_TEST(subRule->getExceptMinWidth() == 0.09 * distFactor);
      BOOST_TEST(subRule->getExceptMinSize().first == 0.1 * distFactor);
      BOOST_TEST(subRule->getExceptMinSize().second == 0.3 * distFactor);
      BOOST_TEST(subRule->getExceptStep().first == 0.01 * distFactor);
      BOOST_TEST(subRule->getExceptStep().second == 0.02 * distFactor);
      BOOST_TEST(subRule->getExceptEdgeLength() == 0.8 * distFactor);
    }
    if (cnt == 3) {
      BOOST_TEST(subRule->getArea() == 0.101 * distFactor);
      BOOST_TEST(subRule->getTrimLayer()->getName() == "metal1");
      BOOST_TEST(subRule->getOverlap() == 1);
    }
    if (cnt == 4) {
      BOOST_TEST(subRule->getArea() == 2.34 * distFactor);
      BOOST_TEST(subRule->isExceptRectangle() == true);
    }
    if (cnt == 5) {
      BOOST_TEST(subRule->getArea() == 0.78 * distFactor);
      BOOST_TEST(subRule->getExceptEdgeLengths().first == 0.3 * distFactor);
      BOOST_TEST(subRule->getExceptEdgeLengths().second == 0.7 * distFactor);
    }
    cnt++;
  }

  layer = dbTech->findLayer("metal2");
  BOOST_TEST(layer->getPitchX() == 0.36 * distFactor);
  BOOST_TEST(layer->getPitchY() == 0.36 * distFactor);
  BOOST_TEST(layer->getFirstLastPitch() == 0.45 * distFactor);

  // Check LEF57_Spacing
  auto cutLayer_57 = dbTech->findLayer("via2");
  auto cutSpacingRules_57 = cutLayer_57->getTechLayerCutSpacingRules();
  BOOST_TEST(cutSpacingRules_57.size() == 3);
  int i_57 = 0;
  for (odb::dbTechLayerCutSpacingRule* subRule : cutSpacingRules_57) {
    if (i_57 == 1) {
      BOOST_TEST(subRule->getCutSpacing() == 0.3 * distFactor);
      BOOST_TEST(subRule->getType()
                 == odb::dbTechLayerCutSpacingRule::CutSpacingType::LAYER);
      BOOST_TEST(subRule->isSameMetal());
      BOOST_TEST(subRule->isStack());
      BOOST_TEST(std::string(subRule->getSecondLayer()->getName()) == "metal2");
    } else if (i_57 == 2) {
      BOOST_TEST(subRule->getCutSpacing() == 0.2 * distFactor);
      BOOST_TEST(
          subRule->getType()
          == odb::dbTechLayerCutSpacingRule::CutSpacingType::ADJACENTCUTS);
      BOOST_TEST(subRule->getAdjacentCuts() == 3);
      BOOST_TEST(subRule->getTwoCuts() == 1);
    } else {
      BOOST_TEST(subRule->getCutSpacing() == 0.12 * distFactor);
      BOOST_TEST(subRule->getType()
                 == odb::dbTechLayerCutSpacingRule::CutSpacingType::MAXXY);
    }
    i_57++;
  }

  // check LEF58_FORBIDDENSPACING
  layer = dbTech->findLayer("metal2");
  auto forbiddenSpacingRules = layer->getTechLayerForbiddenSpacingRules();
  BOOST_TEST(forbiddenSpacingRules.size() == 1);
  int c = 0;
  for (odb::dbTechLayerForbiddenSpacingRule* subRule : forbiddenSpacingRules) {
    if (c == 0) {
      BOOST_TEST(subRule->getForbiddenSpacing().first == 0.05 * distFactor);
      BOOST_TEST(subRule->getForbiddenSpacing().second == 0.2 * distFactor);
      BOOST_TEST(subRule->getWidth() == 0.05 * distFactor);
      BOOST_TEST(subRule->getWithin() == 0.15 * distFactor);
      BOOST_TEST(subRule->getPrl() == 0.015 * distFactor);
      BOOST_TEST(subRule->getTwoEdges() == 0.06 * distFactor);
    }
    c++;
  }

  layer = dbTech->findLayer("metal3");
  forbiddenSpacingRules = layer->getTechLayerForbiddenSpacingRules();
  BOOST_TEST(forbiddenSpacingRules.size() == 1);
  c = 0;
  for (odb::dbTechLayerForbiddenSpacingRule* subRule : forbiddenSpacingRules) {
    if (c == 0) {
      BOOST_TEST(subRule->getForbiddenSpacing().first == 0.1 * distFactor);
      BOOST_TEST(subRule->getForbiddenSpacing().second == 0.3 * distFactor);
      BOOST_TEST(subRule->getWidth() == 0.5 * distFactor);
      BOOST_TEST(subRule->getPrl() == 0.02 * distFactor);
      BOOST_TEST(subRule->getTwoEdges() == 0.12 * distFactor);
    }
    c++;
  }
  // check LEF58_TWOWIRESFORBIDDENSPACING
  layer = dbTech->findLayer("metal2");
  auto TWforbiddenSpacingRules = layer->getTechLayerTwoWiresForbiddenSpcRules();
  BOOST_TEST(TWforbiddenSpacingRules.size() == 2);
  c = 0;
  for (auto* subRule : TWforbiddenSpacingRules) {
    if (c == 0) {
      BOOST_TEST(subRule->getMinSpacing() == 0.16 * distFactor);
      BOOST_TEST(subRule->getMaxSpacing() == 0.2 * distFactor);
      BOOST_TEST(subRule->getMinSpanLength() == 0.05 * distFactor);
      BOOST_TEST(subRule->getMaxSpanLength() == 0.08 * distFactor);
      BOOST_TEST(subRule->getPrl() == 0);
      BOOST_TEST(subRule->isMinExactSpanLength());
      BOOST_TEST(subRule->isMaxExactSpanLength());
    } else {
      BOOST_TEST(subRule->getPrl() == -0.5 * distFactor);
      BOOST_TEST(!subRule->isMinExactSpanLength());
      BOOST_TEST(!subRule->isMaxExactSpanLength());
    }
    c++;
  }
}

BOOST_AUTO_TEST_SUITE_END()

}  // namespace odb
