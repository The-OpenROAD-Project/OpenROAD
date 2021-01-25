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

BOOST_AUTO_TEST_SUITE( test_suite )

BOOST_AUTO_TEST_CASE( test_default )
{
    utl::Logger* logger = new utl::Logger(nullptr);
    dbDatabase*  db   = dbDatabase::create();
    db->setLogger(logger);
    lefin lefParser(db, logger, false);
    const char *libname = "gscl45nm.lef";
    
    std::string path = std::string(std::getenv("BASE_DIR")) + "/data/gscl45nm.lef";

    lefParser.createTechAndLib(libname, path.c_str());
    
    auto dbTech = db->getTech();
    auto layers = dbTech->getLayers();

    double distFactor = 2000;
    for (auto &&layer : layers)
    {
        if(layer->getName() == "metal1"){
            auto rules = layer->getEolSpacingRules();
            BOOST_ASSERT(rules.size() == 1);
            for (auto &&rule : rules)
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
            for (auto rule : step_rules)
            {
                BOOST_TEST(rule->getMinStepLength() == 0.6 * distFactor);
            }
        }
    }
    
}

BOOST_AUTO_TEST_SUITE_END()
