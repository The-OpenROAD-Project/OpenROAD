#define BOOST_TEST_MODULE TestGDSIn
#include <libgen.h>
#include <boost/test/included/unit_test.hpp>
#include <iostream>
#include <string>

#include "odb/gdsin.h"
#include "odb/gdsout.h"
#include "helper/env.h"

namespace odb {
namespace {

BOOST_AUTO_TEST_SUITE(test_suite)
BOOST_AUTO_TEST_CASE(reader)
{
  GDSReader reader;
  dbDatabase* db = dbDatabase::create();
  std::string path = "../../../../../src/odb/test/data/sky130_fd_sc_hd__inv_1.gds";

  printf("Running GDS reader test on file: %s\n", path.c_str());

  dbGDSLib* lib = reader.read_gds(path, db);
  
  BOOST_TEST(lib->getLibname() == "sky130_fd_sc_hd__inv_1");
  BOOST_TEST(lib->getUnits().first == 1e-3);
  BOOST_TEST(lib->getUnits().second == 1e-9);
  
  BOOST_TEST(lib->getGDSStructures().size() == 1);

  dbGDSStructure* str = lib->findGDSStructure("sky130_fd_sc_hd__inv_1");
  BOOST_TEST(str != nullptr);
  BOOST_TEST(str->getNumElements() == 54);

  dbGDSElement* el = str->getElement(0);
  BOOST_TEST(el->getLayer() == 236);
  BOOST_TEST(el->getDatatype() == 0);

  std::vector<Point>& xy = el->getXY();

  BOOST_TEST(xy.size() == 5);
  BOOST_TEST((xy[0].x() == 0 && xy[0].y() == 0));
  BOOST_TEST((xy[1].x() == 1380 && xy[1].y() == 0));
  BOOST_TEST((xy[2].x() == 1380 && xy[2].y() == 2720));
  BOOST_TEST((xy[3].x() == 0 && xy[3].y() == 2720));
  BOOST_TEST((xy[4].x() == 0 && xy[4].y() == 0));

  dbGDSText* text = (dbGDSText*)str->getElement(53);

  BOOST_TEST(text->getLayer() == 83);
  BOOST_TEST(text->getText().c_str() == "inv_1");
  BOOST_TEST(text->get_sTrans()._mag == 0.1);
  BOOST_TEST(text->get_sTrans()._angle == 90);
}

BOOST_AUTO_TEST_CASE(writer)
{
  GDSReader reader;
  dbDatabase* db = dbDatabase::create();
  std::string path = "../../../../../src/odb/test/data/sky130_fd_sc_hd__inv_1.gds";

  printf("Running GDS Writer Test on GDS: %s\n", path.c_str());

  dbGDSLib* libOld = reader.read_gds(path, db);
  GDSWriter writer;
  writer.write_gds(libOld, "sky130_fd_sc_hd__inv_1_temp.gds");

  dbGDSLib* lib = reader.read_gds("sky130_fd_sc_hd__inv_1_temp.gds", db);

  BOOST_TEST(lib->getLibname() == "sky130_fd_sc_hd__inv_1");
  BOOST_TEST(lib->getUnits().first == 1e-3);
  BOOST_TEST(lib->getUnits().second == 1e-9);
  
  BOOST_TEST(lib->getGDSStructures().size() == 1);

  dbGDSStructure* str = lib->findGDSStructure("sky130_fd_sc_hd__inv_1");
  BOOST_TEST(str != nullptr);
  BOOST_TEST(str->getNumElements() == 54);

  dbGDSElement* el = str->getElement(0);
  BOOST_TEST(el->getLayer() == 236);
  BOOST_TEST(el->getDatatype() == 0);

  std::vector<Point>& xy = el->getXY();

  BOOST_TEST(xy.size() == 5);
  BOOST_TEST((xy[0].x() == 0 && xy[0].y() == 0));
  BOOST_TEST((xy[1].x() == 1380 && xy[1].y() == 0));
  BOOST_TEST((xy[2].x() == 1380 && xy[2].y() == 2720));
  BOOST_TEST((xy[3].x() == 0 && xy[3].y() == 2720));
  BOOST_TEST((xy[4].x() == 0 && xy[4].y() == 0));

  dbGDSText* text = (dbGDSText*)str->getElement(53);

  BOOST_TEST(text->getLayer() == 83);
  BOOST_TEST(text->getText().c_str() == "inv_1");
  BOOST_TEST(text->get_sTrans()._mag == 0.1);
  BOOST_TEST(text->get_sTrans()._angle == 90);
}

// int main(int argc, char* argv[])
// {
//   if(argc != 3)
//   {
//     std::cerr << "Usage: " << argv[0] << " <input file> <output file>" << std::endl;
//     return 1;
//   }
//   odb::GDSReader reader;
//   odb::dbDatabase* db = odb::dbDatabase::create();
//   odb::dbGDSLib* lib = reader.read_gds(argv[1], db);
//   std::cout << "Library: " << lib->getLibname() << std::endl;
//   std::cout << "Units: " << lib->getUnits().first << " " << lib->getUnits().second << std::endl;

//   odb::GDSWriter writer;
//   writer.write_gds(lib, argv[2]);

//   delete lib;
// }

BOOST_AUTO_TEST_SUITE_END()

} // namespace
} // namespace odb