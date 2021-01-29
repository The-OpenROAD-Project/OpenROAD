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

BOOST_AUTO_TEST_SUITE( test_suite )

BOOST_AUTO_TEST_CASE( test_default )
{
    utl::Logger* logger = new utl::Logger();
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
            BOOST_TEST (rules.size() == 1);
            for (auto &&rule : rules)
            {
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
           }
            
        }
    }
    
}

BOOST_AUTO_TEST_SUITE_END()
