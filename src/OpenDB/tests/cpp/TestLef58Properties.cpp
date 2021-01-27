#define BOOST_TEST_MODULE example
#include <boost/test/included/unit_test.hpp>
#include "db.h"
#include <libgen.h>
#include "lefin.h"
#include "lefout.h"
#include "defin.h"
#include "defout.h"
#include "utility/Logger.h"

#include <iostream>
using namespace odb;
using namespace std;

BOOST_AUTO_TEST_SUITE( test_suite )

BOOST_AUTO_TEST_CASE( test_default )
{
    utl::Logger* logger = new utl::Logger(nullptr);
    dbDatabase*  db   = dbDatabase::create();
    db->setLogger(logger);
    lefin lefParser(db, logger, false);
    const char *libname = "gscl45nm.lef";
    
    std::string path = "/home/osama/Desktop/Workspace/Private/OpenROAD/src/OpenDB/tests/data/gscl45nm.lef";

    lefParser.createTechAndLib(libname, path.c_str());
    
    auto dbTech = db->getTech();
    double distFactor = 2000;
    auto layer = dbTech->findLayer("metal1");
    auto rules = layer->getEolSpacingRules();
    BOOST_ASSERT(rules.size() == 1);
    for (auto rule : rules)
    {
        BOOST_ASSERT(rule->getEolSpace() == 1.3 * distFactor);
        BOOST_ASSERT(rule->getEolWidth() == 1.5 * distFactor);
        BOOST_ASSERT(rule->isWithinValid() == 1);
        BOOST_ASSERT(rule->getEolWithin() == 1.9 * distFactor);
        BOOST_ASSERT(rule->isSameMaskValid() == 1);
        BOOST_ASSERT(rule->isExceptExactWidthValid() == 1);
        BOOST_ASSERT(rule->getExactWidth() == 0.5 * distFactor);
        BOOST_ASSERT(rule->getOtherWidth() == 0.3 * distFactor);
        BOOST_ASSERT(rule->isParallelEdgeValid() == 1 );
        BOOST_ASSERT(rule->getParSpace() == 0.2 * distFactor);
        BOOST_ASSERT(rule->getParWithin() == 9.1 * distFactor);
        BOOST_ASSERT(rule->isParPrlValid() == 1 );
        BOOST_ASSERT(rule->getParPrl() == 81 * distFactor);
        BOOST_ASSERT(rule->isParMinLengthValid() == 1);
        BOOST_ASSERT(rule->getParMinLength() == -0.1 * distFactor);
        BOOST_ASSERT(rule->isTwoEdgesValid() == 1);
        BOOST_ASSERT(rule->isToConcaveCornerValid() == 0);
    }
    
    auto step_rules = layer->getMinStepRules();
    BOOST_TEST(step_rules.size() == 1);
    odb::dbTechLayerMinStepRule* step_rule = (odb::dbTechLayerMinStepRule*) *step_rules.begin();
    BOOST_TEST(step_rule->getMinStepLength() == 0.6 * distFactor);
    BOOST_TEST(step_rule->getMaxEdges() == 1);
    BOOST_TEST(step_rule->isMinAdjLength1Valid() == true);
    BOOST_TEST(step_rule->isMinAdjLength2Valid() == false);
    BOOST_TEST(step_rule->getMinAdjLength1() == 1.0 * distFactor);

    auto corner_rules = layer->getCornerSpacingRules();
    BOOST_TEST(corner_rules.size() == 1);
    odb::dbTechLayerCornerSpacingRule* corner_rule = (odb::dbTechLayerCornerSpacingRule*) *corner_rules.begin();
    BOOST_TEST(corner_rule->getType() == odb::dbTechLayerCornerSpacingRule::CONVEXCORNER);
    BOOST_TEST(corner_rule->isExceptEol() == true);
    BOOST_TEST(corner_rule->getEolWidth() == 0.090 * distFactor);
    vector<pair<int,int>> spacing;
    corner_rule->getSpacingTable(spacing);
    BOOST_TEST(spacing.size() == 1 );
    BOOST_TEST(spacing[0].first == 0);
    BOOST_TEST(spacing[0].second == 0.110 * distFactor);

    auto spacingTables = layer->getSpacingTablePrlRules();
    BOOST_TEST(spacingTables.size() == 1);
    odb::dbTechLayerSpacingTablePrlRule* spacing_tbl_rule = (odb::dbTechLayerSpacingTablePrlRule*) *spacingTables.begin();
    BOOST_TEST(spacing_tbl_rule->isWrongDirection()==true);
    BOOST_TEST(spacing_tbl_rule->isSameMask()==false);
    BOOST_TEST(spacing_tbl_rule->isExceeptEol()==true);
    BOOST_TEST(spacing_tbl_rule->getEolWidth()==0.090*distFactor);
    vector<int> width;
    vector<int> length;
    vector<vector<int>> spacing_tbl;
    map<unsigned int,pair<int,int>> within;
    spacing_tbl_rule->getTable(width,length,spacing_tbl,within);
    BOOST_TEST(width.size()==2);
    BOOST_TEST(length.size()==2);
    BOOST_TEST(spacing_tbl.size()==2);
    BOOST_TEST(within.size()==1);
    BOOST_TEST(spacing_tbl_rule->getSpacing(0,0)==0.05*distFactor);

    auto rwogoRules = layer->getRightWayOnGridOnlyRules();
    BOOST_TEST(rwogoRules.size() == 1);
    odb::dbTechLayerRightWayOnGridOnlyRule* rwogo_rule = (odb::dbTechLayerRightWayOnGridOnlyRule*) *rwogoRules.begin();
    BOOST_TEST(rwogo_rule->isCheckMask()==true);

    auto rectOnlyRules = layer->getRectOnlyRules();
    BOOST_TEST(rectOnlyRules.size() == 1);
    odb::dbTechLayerRectOnlyRule* rect_only_rule = (odb::dbTechLayerRectOnlyRule*) *rectOnlyRules.begin();
    BOOST_TEST(rect_only_rule->isExceptNonCorePins()==true);

}

BOOST_AUTO_TEST_SUITE_END()
