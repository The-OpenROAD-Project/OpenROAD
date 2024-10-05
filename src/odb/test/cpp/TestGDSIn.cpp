#define BOOST_TEST_MODULE TestGDSIn
#include <libgen.h>

#include <boost/test/included/unit_test.hpp>
#include <iostream>
#include <string>

#include "helper/env.h"
#include "odb/gdsin.h"
#include "odb/gdsout.h"

namespace odb {
namespace gds {
namespace {

BOOST_AUTO_TEST_SUITE(test_suite)
BOOST_AUTO_TEST_CASE(reader)
{
  GDSReader reader;
  dbDatabase* db = dbDatabase::create();
  std::string path = testTmpPath("data", "sky130_fd_sc_hd__inv_1.gds");

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

  dbGDSText* text = (dbGDSText*) str->getElement(53);

  BOOST_TEST(text->getLayer() == 83);
  BOOST_TEST(text->getText().c_str() == "inv_1");
  BOOST_TEST(text->getTransform()._mag == 0.1);
  BOOST_TEST(text->getTransform()._angle == 90);
}

BOOST_AUTO_TEST_CASE(writer)
{
  GDSReader reader;
  dbDatabase* db = dbDatabase::create();
  std::string path = testTmpPath("data", "sky130_fd_sc_hd__inv_1.gds");

  std::string outpath
      = testTmpPath("results", "sky130_fd_sc_hd__inv_1_temp.gds");

  printf("Running GDS Writer Test on GDS: %s\n", path.c_str());

  dbGDSLib* libOld = reader.read_gds(path, db);
  GDSWriter writer;
  writer.write_gds(libOld, outpath);

  dbGDSLib* lib = reader.read_gds(outpath, db);

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

  dbGDSText* text = (dbGDSText*) str->getElement(53);

  BOOST_TEST(text->getLayer() == 83);
  BOOST_TEST(text->getText().c_str() == "inv_1");
  BOOST_TEST(text->getTransform()._mag == 0.1);
  BOOST_TEST(text->getTransform()._angle == 90);
}

BOOST_AUTO_TEST_CASE(edit)
{
  dbDatabase* db = dbDatabase::create();
  std::string libname = "test_lib";
  dbGDSLib* lib = createEmptyGDSLib(db, libname);

  dbGDSStructure* str1 = dbGDSStructure::create(lib, "str1");
  dbGDSStructure* str2 = dbGDSStructure::create(lib, "str2");
  dbGDSStructure* str3 = dbGDSStructure::create(lib, "str3");

  dbGDSStructure::destroy(str2);

  dbGDSBox* box = createEmptyGDSBox(db);
  box->setLayer(3);
  box->setDatatype(4);
  box->getXY().emplace_back(0, 0);
  box->getXY().emplace_back(0, 1000);
  box->getXY().emplace_back(1000, 1000);
  box->getXY().emplace_back(1000, 0);

  box->getPropattr().emplace_back(12, "test");

  str1->addElement(box);

  dbGDSNode* node = createEmptyGDSNode(db);
  node->setLayer(6);
  node->setDatatype(7);

  node->getXY().emplace_back(2, 3);
  node->getXY().emplace_back(4, 5);

  str1->addElement(node);

  dbGDSSRef* sref = createEmptyGDSSRef(db);
  sref->set_sName("str1");
  sref->setTransform(dbGDSSTrans(false, false, false, 2.0, 90));

  str3->addElement(sref);

  std::string outpath = testTmpPath("results", "edit_test_out.gds");

  stampGDSLib(lib);

  GDSWriter writer;
  writer.write_gds(lib, outpath);

  GDSReader reader;
  dbGDSLib* lib2 = reader.read_gds(outpath, db);

  BOOST_TEST(lib2->getLibname() == libname);
  BOOST_TEST(lib2->getGDSStructures().size() == 2);

  dbGDSStructure* str1_read = lib2->findGDSStructure("str1");
  BOOST_TEST(str1_read != nullptr);
  BOOST_TEST(str1_read->getNumElements() == 2);

  dbGDSBox* box_read = (dbGDSBox*) str1_read->getElement(0);
  BOOST_TEST(box_read->getLayer() == 3);
  BOOST_TEST(box_read->getDatatype() == 4);
  BOOST_TEST(box_read->getXY().size() == 4);
  BOOST_TEST(box_read->getPropattr().size() == 1);
  BOOST_TEST(box_read->getPropattr()[0].first == 12);
  BOOST_TEST(box_read->getPropattr()[0].second == "test");

  dbGDSNode* node_read = (dbGDSNode*) str1_read->getElement(1);
  BOOST_TEST(node_read->getLayer() == 6);
  BOOST_TEST(node_read->getDatatype() == 7);

  dbGDSStructure* str3_read = lib2->findGDSStructure("str3");
  BOOST_TEST(str3_read != nullptr);
  BOOST_TEST(str3_read->getNumElements() == 1);

  dbGDSSRef* sref_read = (dbGDSSRef*) str3_read->getElement(0);
  BOOST_TEST(sref_read->get_sName() == "str1");
  BOOST_TEST(sref_read->getTransform()._mag == 2.0);
  BOOST_TEST(sref_read->getTransform()._angle == 90);

  dbGDSStructure* ref_str = sref_read->getStructure();

  BOOST_TEST(ref_str != nullptr);
  BOOST_TEST(ref_str == str1_read);
}

BOOST_AUTO_TEST_SUITE_END()

}  // namespace
}  // namespace gds
}  // namespace odb
