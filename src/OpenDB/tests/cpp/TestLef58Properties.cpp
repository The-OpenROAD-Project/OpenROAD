#define BOOST_TEST_MODULE TestLef58Properties
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
    utl::Logger* logger = new utl::Logger();
    dbDatabase*  db1   = dbDatabase::create();
    dbDatabase*  db2   = dbDatabase::create();
    db1->setLogger(logger);
    db2->setLogger(logger);
    lefin lefParser(db1, logger, false);
    const char *libname = "gscl45nm.lef";

    std::string path = std::string(std::getenv("BASE_DIR")) + "/data/gscl45nm.lef";

    lefParser.createTechAndLib(libname, path.c_str());

    FILE *write,*read;
    path = std::string(std::getenv("BASE_DIR")) + "/results/TestLef58PropertiesDbRW";
    write=fopen(path.c_str(),"w");
    db1->write(write);
    read=fopen(path.c_str(),"r");
    db2->read(read);
    
    auto dbTech = db2->getTech();
    double distFactor = 2000;
    auto layer = dbTech->findLayer("metal1");
    
    auto rules = layer->getEolSpacingRules();
    BOOST_TEST (rules.size() == 1);
    odb::dbTechLayerSpacingEolRule* rule = (odb::dbTechLayerSpacingEolRule*) *rules.begin();
    BOOST_TEST (rule->getEolSpace() == 1.3 * distFactor);
    BOOST_TEST (rule->getEolWidth() == 1.5 * distFactor);
    BOOST_TEST (rule->isWithinValid() == 1);
    BOOST_TEST (rule->getEolWithin() == 1.9 * distFactor);
    BOOST_TEST (rule->isSameMaskValid() == 1);
    BOOST_TEST (rule->isExceptExactWidthValid() == 1);
    BOOST_TEST (rule->getExactWidth() == 0.5 * distFactor);
    BOOST_TEST (rule->getOtherWidth() == 0.3 * distFactor);
    BOOST_TEST (rule->isParallelEdgeValid() == 1 );
    BOOST_TEST (rule->getParSpace() == 0.2 * distFactor);
    BOOST_TEST (rule->getParWithin() == 9.1 * distFactor);
    BOOST_TEST (rule->isParPrlValid() == 1 );
    BOOST_TEST (rule->getParPrl() == 81 * distFactor);
    BOOST_TEST (rule->isParMinLengthValid() == 1);
    BOOST_TEST (rule->getParMinLength() == -0.1 * distFactor);
    BOOST_TEST (rule->isTwoEdgesValid() == 1);
    BOOST_TEST (rule->isToConcaveCornerValid() == 0);
    
    auto minStepRules = layer->getMinStepRules();
    BOOST_TEST(minStepRules.size() == 1);
    odb::dbTechLayerMinStepRule* minStepRule = (odb::dbTechLayerMinStepRule*) *minStepRules.begin();
    BOOST_TEST(minStepRule->getTechLayerMinStepSubRules().size() == 1);
    odb::dbTechLayerMinStepSubRule* step_rule = (odb::dbTechLayerMinStepSubRule*) *minStepRule->getTechLayerMinStepSubRules().begin();
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

    auto cutLayer = dbTech->findLayer("via1");

    // auto cutRules = cutLayer->getCutClassRules();
    // BOOST_TEST(cutRules.size() == 1);
    // odb::dbTechLayerCutClassRule* cut_rule = (odb::dbTechLayerCutClassRule*) *cutRules.begin();
    // auto subRules = cut_rule->getTechLayerCutClassSubRules();
    // BOOST_TEST(subRules.size() == 1);
    // odb::dbTechLayerCutClassSubRule* sub_rule = (odb::dbTechLayerCutClassSubRule*) *subRules.begin();
    // BOOST_TEST(std::string(sub_rule->getClassName())=="VA");
    // BOOST_TEST(sub_rule->getWidth()==0.15*distFactor);

    auto cutSpacingRules = cutLayer->getCutSpacingRules();
    BOOST_TEST(cutSpacingRules.size() == 1);
    odb::dbTechLayerCutSpacingRule* cut_spacing_rule = (odb::dbTechLayerCutSpacingRule*) *cutSpacingRules.begin();
    auto subSpacingRules = cut_spacing_rule->getTechLayerCutSpacingSubRules();
    BOOST_TEST(subSpacingRules.size()==2);
    int i = 0;
    for(auto subRule:subSpacingRules)
    {
        if(i)
        {
            BOOST_TEST(subRule->getCutSpacing()==0.3*distFactor);
            BOOST_TEST(subRule->getType()==odb::dbTechLayerCutSpacingSubRule::CutSpacingType::LAYER);
            BOOST_TEST(subRule->isSameMetal());
            BOOST_TEST(subRule->isStack());
            BOOST_TEST(std::string(subRule->getSecondLayerName())=="via2");
        }else
        {
            BOOST_TEST(subRule->getCutSpacing()==0.12*distFactor);
            BOOST_TEST(subRule->getType()==odb::dbTechLayerCutSpacingSubRule::CutSpacingType::MAXXY);
        }
        i++;
    }

    auto cutTables = cutLayer->getCutSpacingTableRules();
    BOOST_TEST(cutTables.size()==1);
    odb::dbTechLayerCutSpacingTableRule* cutTable = (odb::dbTechLayerCutSpacingTableRule*) *cutTables.begin();
    auto orths = cutTable->getTechLayerCutSpacingTableOrthSubRules();
    auto defs = cutTable->getTechLayerCutSpacingTableDefSubRules();
    BOOST_TEST(orths.size()==1);
    BOOST_TEST(defs.size()==1);
    odb::dbTechLayerCutSpacingTableOrthSubRule* orth = (odb::dbTechLayerCutSpacingTableOrthSubRule*) *orths.begin();
    odb::dbTechLayerCutSpacingTableDefSubRule* def = (odb::dbTechLayerCutSpacingTableDefSubRule*) *defs.begin();
    std::vector<std::pair<int,int>> table;
    orth->getSpacingTable(table);
    BOOST_TEST(table[0].first == 0.2*distFactor);
    BOOST_TEST(table[0].second == 0.15*distFactor);
    BOOST_TEST(table[1].first == 0.3*distFactor);
    BOOST_TEST(table[1].second == 0.25*distFactor);

    BOOST_TEST(def->getDefault()==0.12*distFactor);
    BOOST_TEST(def->isDefaultValid());
    BOOST_TEST(def->isSameMask());
    BOOST_TEST(def->isSameNet());
    BOOST_TEST(def->isLayerValid());
    BOOST_TEST(def->isNoStack());
    BOOST_TEST(std::string(def->getSecondLayerName())=="via2");
    BOOST_TEST(!def->isSameMetal());
    BOOST_TEST(def->isPrlForAlignedCut());
    std::vector<std::pair<char*,char*>> prlFACtbl;
    def->getPrlForAlignedCutTable(prlFACtbl);
    BOOST_TEST(prlFACtbl.size()==2);
    BOOST_TEST(std::string(prlFACtbl[0].first) == "cls1");
    BOOST_TEST(std::string(prlFACtbl[0].second) == "cls2");
    BOOST_TEST(std::string(prlFACtbl[1].first) == "cls3");
    BOOST_TEST(std::string(prlFACtbl[1].second) == "cls4");
    auto spacing1 = def->getSpacing("cls1",true,"cls3",false);
    auto spacing2 = def->getSpacing("cls1",false,"cls4",false);
    BOOST_TEST(spacing1.first==0.1*distFactor );
    BOOST_TEST(spacing1.second==0.2*distFactor );
    BOOST_TEST(spacing2.first==0.7*distFactor );
    BOOST_TEST(spacing2.second==def->getDefault());

}

BOOST_AUTO_TEST_SUITE_END()
