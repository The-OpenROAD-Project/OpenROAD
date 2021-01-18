#define BOOST_TEST_MODULE example
#include <boost/test/included/unit_test.hpp>
#include "db.h"
#include <libgen.h>
#include "lefin.h"
#include "lefout.h"
#include "defin.h"
#include "defout.h"
#include "cdl.h"


#include <iostream>
using namespace odb;

BOOST_AUTO_TEST_SUITE( test_suite )

BOOST_AUTO_TEST_CASE( test_default )
{
    dbDatabase*  db   = dbDatabase::create();
    lefin lefParser(db, false);
    const char *libname = "Nangate45";
    
    std::string lefpath = std::string(std::getenv("BASE_DIR")) + "/data/Nangate45/Nangate45.lef";
    std::string defpath = std::string(std::getenv("BASE_DIR")) + "/data/gcd/gcd.def";

    lefParser.createTechAndLib(libname, lefpath.c_str());
    

    std::vector<odb::dbLib *> libs;
    for (dbLib *lib : db->getLibs()) {
        libs.push_back(lib);
    }

    defin defParser(db);
    auto chip = defParser.createChip(libs, defpath.c_str());
    auto block = chip->getBlock();
    std::string cdlpath = std::string(std::getenv("BASE_DIR")) + "/results/netlist.cdl";
    cdl::dumpNetLists(block, cdlpath.c_str());    
}

BOOST_AUTO_TEST_SUITE_END()
