#define BOOST_TEST_MODULE TestGDSIn
#include <libgen.h>

#include <boost/test/included/unit_test.hpp>
#include <iostream>
#include <string>

#include "helper/env.h"
#include "odb/gdsin.h"
#include "odb/gdsout.h"
#include "utl/Logger.h"

namespace odb::gds {
namespace {

BOOST_AUTO_TEST_SUITE(test_suite)
BOOST_AUTO_TEST_CASE(reader)
{
  auto logger = std::make_unique<utl::Logger>();
  GDSReader reader(logger.get());
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

  BOOST_TEST(str->getGDSBoundarys().size() == 44);
  BOOST_TEST(str->getGDSBoxs().size() == 0);
  BOOST_TEST(str->getGDSNodes().size() == 0);
  BOOST_TEST(str->getGDSPaths().size() == 2);
  BOOST_TEST(str->getGDSSRefs().size() == 0);
  BOOST_TEST(str->getGDSTexts().size() == 8);

  auto paths_iter = str->getGDSPaths().begin();
  auto path0 = *paths_iter++;
  auto path1 = *paths_iter++;
  BOOST_TEST(path0->getLayer() == 68);
  BOOST_TEST(path0->getDatatype() == 20);
  BOOST_TEST(path1->getLayer() == 68);
  BOOST_TEST(path1->getDatatype() == 20);

  const std::vector<Point>& path0_xy = path0->getXY();
  const std::vector<Point>& path1_xy = path1->getXY();

  BOOST_TEST(path0_xy.size() == 2);
  BOOST_TEST(path1_xy.size() == 2);

  auto boundries_iter = str->getGDSBoundarys().begin();
  auto boundary0 = *boundries_iter++;
  const std::vector<Point>& boundary0_xy = boundary0->getXY();

  BOOST_TEST(boundary0_xy.size() == 5);
  BOOST_TEST(boundary0_xy[0] == Point(0, 0));
  BOOST_TEST(boundary0_xy[0] == Point(0, 0));
  BOOST_TEST(boundary0_xy[1] == Point(1380, 0));
  BOOST_TEST(boundary0_xy[2] == Point(1380, 2720));
  BOOST_TEST(boundary0_xy[3] == Point(0, 2720));
  BOOST_TEST(boundary0_xy[4] == Point(0, 0));

  auto texts_iter = str->getGDSTexts().begin();
  auto text0 = *texts_iter++;
  BOOST_TEST(text0->getLayer() == 67);
  BOOST_TEST(text0->getText().c_str() == "Y");
  BOOST_TEST(text0->getTransform()._mag == 0.17);
  BOOST_TEST(text0->getTransform()._angle == 0);
}

BOOST_AUTO_TEST_CASE(writer)
{
  auto logger = std::make_unique<utl::Logger>();
  GDSReader reader(logger.get());
  dbDatabase* db = dbDatabase::create();
  std::string path = testTmpPath("data", "sky130_fd_sc_hd__inv_1.gds");

  std::string outpath
      = testTmpPath("results", "sky130_fd_sc_hd__inv_1_temp.gds");

  printf("Running GDS Writer Test on GDS: %s\n", path.c_str());

  dbGDSLib* libOld = reader.read_gds(path, db);

  GDSWriter writer(logger.get());
  writer.write_gds(libOld, outpath);

  dbGDSLib* lib = reader.read_gds(outpath, db);

  BOOST_TEST(lib->getLibname() == "sky130_fd_sc_hd__inv_1");
  BOOST_TEST(lib->getUnits().first == 1e-3);
  BOOST_TEST(lib->getUnits().second == 1e-9);

  BOOST_TEST(lib->getGDSStructures().size() == 1);

  dbGDSStructure* str = lib->findGDSStructure("sky130_fd_sc_hd__inv_1");
  BOOST_TEST(str != nullptr);

  BOOST_TEST(str->getGDSBoundarys().size() == 44);
  BOOST_TEST(str->getGDSBoxs().size() == 0);
  BOOST_TEST(str->getGDSNodes().size() == 0);
  BOOST_TEST(str->getGDSPaths().size() == 2);
  BOOST_TEST(str->getGDSSRefs().size() == 0);
  BOOST_TEST(str->getGDSTexts().size() == 8);

  auto paths_iter = str->getGDSPaths().begin();
  auto path0 = *paths_iter++;
  auto path1 = *paths_iter++;
  BOOST_TEST(path0->getLayer() == 68);
  BOOST_TEST(path0->getDatatype() == 20);
  BOOST_TEST(path1->getLayer() == 68);
  BOOST_TEST(path1->getDatatype() == 20);

  const std::vector<Point>& path0_xy = path0->getXY();
  const std::vector<Point>& path1_xy = path1->getXY();

  BOOST_TEST(path0_xy.size() == 2);
  BOOST_TEST(path1_xy.size() == 2);

  auto boundries_iter = str->getGDSBoundarys().begin();
  auto boundary0 = *boundries_iter++;
  const std::vector<Point>& boundary0_xy = boundary0->getXY();

  BOOST_TEST(boundary0_xy.size() == 5);
  BOOST_TEST(boundary0_xy[0] == Point(0, 0));
  BOOST_TEST(boundary0_xy[0] == Point(0, 0));
  BOOST_TEST(boundary0_xy[1] == Point(1380, 0));
  BOOST_TEST(boundary0_xy[2] == Point(1380, 2720));
  BOOST_TEST(boundary0_xy[3] == Point(0, 2720));
  BOOST_TEST(boundary0_xy[4] == Point(0, 0));

  auto texts_iter = str->getGDSTexts().begin();
  auto text0 = *texts_iter++;
  BOOST_TEST(text0->getLayer() == 67);
  BOOST_TEST(text0->getText().c_str() == "Y");
  BOOST_TEST(text0->getTransform()._mag == 0.17);
  BOOST_TEST(text0->getTransform()._angle == 0);
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

  dbGDSBox* box = dbGDSBox::create(str1);
  box->setLayer(3);
  box->setDatatype(4);
  box->setXy({{0, 0}, {0, 1000}, {1000, 1000}, {1000, 0}});

  box->getPropattr().emplace_back(12, "test");

  dbGDSNode* node = dbGDSNode::create(str1);
  node->setLayer(6);
  node->setDatatype(7);
  node->setXy({{2, 3}, {4, 5}});

  dbGDSSRef* sref = dbGDSSRef::create(str3);
  sref->set_sName("str1");
  sref->setTransform(dbGDSSTrans(false, false, false, 2.0, 90));

  std::string outpath = testTmpPath("results", "edit_test_out.gds");

  stampGDSLib(lib);

  auto logger = std::make_unique<utl::Logger>();
  GDSWriter writer(logger.get());
  writer.write_gds(lib, outpath);

  GDSReader reader(logger.get());
  dbGDSLib* lib2 = reader.read_gds(outpath, db);

  BOOST_TEST(lib2->getLibname() == libname);
  BOOST_TEST(lib2->getGDSStructures().size() == 2);

  dbGDSStructure* str1_read = lib2->findGDSStructure("str1");
  BOOST_TEST(str1_read != nullptr);
  BOOST_TEST(str1_read->getGDSBoxs().size() == 1);
  BOOST_TEST(str1_read->getGDSNodes().size() == 1);

  dbGDSBox* box_read = *str1_read->getGDSBoxs().begin();
  BOOST_TEST(box_read->getLayer() == 3);
  BOOST_TEST(box_read->getDatatype() == 4);
  BOOST_TEST(box_read->getXY().size() == 4);
  BOOST_TEST(box_read->getPropattr().size() == 1);
  BOOST_TEST(box_read->getPropattr()[0].first == 12);
  BOOST_TEST(box_read->getPropattr()[0].second == "test");

  dbGDSNode* node_read = *str1_read->getGDSNodes().begin();
  BOOST_TEST(node_read->getLayer() == 6);
  BOOST_TEST(node_read->getDatatype() == 7);

  dbGDSStructure* str3_read = lib2->findGDSStructure("str3");
  BOOST_TEST(str3_read != nullptr);
  BOOST_TEST(str3_read->getGDSSRefs().size() == 1);

  dbGDSSRef* sref_read = *str3_read->getGDSSRefs().begin();
  BOOST_TEST(sref_read->get_sName() == "str1");
  BOOST_TEST(sref_read->getTransform()._mag == 2.0);
  BOOST_TEST(sref_read->getTransform()._angle == 90);

  dbGDSStructure* ref_str = sref_read->getStructure();
  BOOST_TEST(ref_str != nullptr);
  BOOST_TEST(ref_str == str1_read);
}

BOOST_AUTO_TEST_SUITE_END()

}  // namespace
}  // namespace odb::gds
