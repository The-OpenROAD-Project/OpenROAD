#include <libgen.h>

#include <cstdio>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

#include "gtest/gtest.h"
#include "odb/db.h"
#include "odb/gdsUtil.h"
#include "odb/gdsin.h"
#include "odb/gdsout.h"
#include "odb/geom.h"
#include "tst/fixture.h"
#include "utl/Logger.h"

namespace odb::gds {
namespace {

static const std::string prefix("_main/src/odb/test/");

using tst::Fixture;

TEST_F(Fixture, reader)
{
  GDSReader reader(&logger_);
  std::string path = getFilePath(prefix + "data/sky130_fd_sc_hd__inv_1.gds");

  printf("Running GDS reader test on file: %s\n", path.c_str());

  dbGDSLib* lib = reader.read_gds(path, db_.get());

  EXPECT_EQ(lib->getLibname(), "sky130_fd_sc_hd__inv_1");
  EXPECT_EQ(lib->getUnits().first, 1e-3);
  EXPECT_EQ(lib->getUnits().second, 1e-9);

  EXPECT_EQ(lib->getGDSStructures().size(), 1);

  dbGDSStructure* str = lib->findGDSStructure("sky130_fd_sc_hd__inv_1");
  ASSERT_NE(str, nullptr);

  EXPECT_EQ(str->getGDSBoundaries().size(), 44);
  EXPECT_EQ(str->getGDSBoxs().size(), 0);
  EXPECT_EQ(str->getGDSPaths().size(), 2);
  EXPECT_EQ(str->getGDSSRefs().size(), 0);
  EXPECT_EQ(str->getGDSTexts().size(), 8);

  auto paths_iter = str->getGDSPaths().begin();
  auto path0 = *paths_iter++;
  auto path1 = *paths_iter++;
  EXPECT_EQ(path0->getLayer(), 68);
  EXPECT_EQ(path0->getDatatype(), 20);
  EXPECT_EQ(path1->getLayer(), 68);
  EXPECT_EQ(path1->getDatatype(), 20);

  const std::vector<Point>& path0_xy = path0->getXY();
  const std::vector<Point>& path1_xy = path1->getXY();

  EXPECT_EQ(path0_xy.size(), 2);
  EXPECT_EQ(path1_xy.size(), 2);

  auto boundries_iter = str->getGDSBoundaries().begin();
  auto boundary0 = *boundries_iter++;
  const std::vector<Point>& boundary0_xy = boundary0->getXY();

  EXPECT_EQ(boundary0_xy.size(), 5);
  EXPECT_EQ(boundary0_xy[0], Point(0, 0));
  EXPECT_EQ(boundary0_xy[0], Point(0, 0));
  EXPECT_EQ(boundary0_xy[1], Point(1380, 0));
  EXPECT_EQ(boundary0_xy[2], Point(1380, 2720));
  EXPECT_EQ(boundary0_xy[3], Point(0, 2720));
  EXPECT_EQ(boundary0_xy[4], Point(0, 0));

  auto texts_iter = str->getGDSTexts().begin();
  auto text0 = *texts_iter++;
  EXPECT_EQ(text0->getLayer(), 67);
  EXPECT_EQ(text0->getText(), "Y");
  EXPECT_EQ(text0->getTransform().mag_, 0.17);
  EXPECT_EQ(text0->getTransform().angle_, 0);
}

TEST_F(Fixture, writer)
{
  GDSReader reader(&logger_);
  std::string path = getFilePath(prefix + "data/sky130_fd_sc_hd__inv_1.gds");

  printf("Running GDS Writer Test on GDS: %s\n", path.c_str());

  dbGDSLib* libOld = reader.read_gds(path, db_.get());

  std::filesystem::create_directory("results");
  const char* outpath = "results/sky130_fd_sc_hd__inv_1_temp.gds";

  GDSWriter writer(&logger_);
  writer.write_gds(libOld, outpath);

  dbGDSLib* lib = reader.read_gds(outpath, db_.get());

  EXPECT_EQ(lib->getLibname(), "sky130_fd_sc_hd__inv_1");
  EXPECT_EQ(lib->getUnits().first, 1e-3);
  EXPECT_EQ(lib->getUnits().second, 1e-9);

  EXPECT_EQ(lib->getGDSStructures().size(), 1);

  dbGDSStructure* str = lib->findGDSStructure("sky130_fd_sc_hd__inv_1");
  ASSERT_NE(str, nullptr);

  EXPECT_EQ(str->getGDSBoundaries().size(), 44);
  EXPECT_EQ(str->getGDSBoxs().size(), 0);
  EXPECT_EQ(str->getGDSPaths().size(), 2);
  EXPECT_EQ(str->getGDSSRefs().size(), 0);
  EXPECT_EQ(str->getGDSTexts().size(), 8);

  auto paths_iter = str->getGDSPaths().begin();
  auto path0 = *paths_iter++;
  auto path1 = *paths_iter++;
  EXPECT_EQ(path0->getLayer(), 68);
  EXPECT_EQ(path0->getDatatype(), 20);
  EXPECT_EQ(path1->getLayer(), 68);
  EXPECT_EQ(path1->getDatatype(), 20);

  const std::vector<Point>& path0_xy = path0->getXY();
  const std::vector<Point>& path1_xy = path1->getXY();

  EXPECT_EQ(path0_xy.size(), 2);
  EXPECT_EQ(path1_xy.size(), 2);

  auto boundries_iter = str->getGDSBoundaries().begin();
  auto boundary0 = *boundries_iter++;
  const std::vector<Point>& boundary0_xy = boundary0->getXY();

  EXPECT_EQ(boundary0_xy.size(), 5);
  EXPECT_EQ(boundary0_xy[0], Point(0, 0));
  EXPECT_EQ(boundary0_xy[0], Point(0, 0));
  EXPECT_EQ(boundary0_xy[1], Point(1380, 0));
  EXPECT_EQ(boundary0_xy[2], Point(1380, 2720));
  EXPECT_EQ(boundary0_xy[3], Point(0, 2720));
  EXPECT_EQ(boundary0_xy[4], Point(0, 0));

  auto texts_iter = str->getGDSTexts().begin();
  auto text0 = *texts_iter++;
  EXPECT_EQ(text0->getLayer(), 67);
  EXPECT_EQ(text0->getText(), "Y");
  EXPECT_EQ(text0->getTransform().mag_, 0.17);
  EXPECT_EQ(text0->getTransform().angle_, 0);
}

TEST_F(Fixture, edit)
{
  std::string libname = "test_lib";
  dbGDSLib* lib = createEmptyGDSLib(db_.get(), libname);

  dbGDSStructure* str1 = dbGDSStructure::create(lib, "str1");
  dbGDSStructure* str2 = dbGDSStructure::create(lib, "str2");
  dbGDSStructure* str3 = dbGDSStructure::create(lib, "str3");

  dbGDSStructure::destroy(str2);

  dbGDSBox* box = dbGDSBox::create(str1);
  box->setLayer(3);
  box->setDatatype(4);
  box->setBounds({0, 0, 1000, 1000});

  box->getPropattr().emplace_back(12, "test");

  dbGDSSRef* sref = dbGDSSRef::create(str3, str1);
  sref->setTransform(dbGDSSTrans(false, 2.0, 90));

  std::filesystem::create_directory("results");
  const char* outpath = "results/edit_test_out.gds";

  GDSWriter writer(&logger_);
  writer.write_gds(lib, outpath);

  GDSReader reader(&logger_);
  dbGDSLib* lib2 = reader.read_gds(outpath, db_.get());

  EXPECT_EQ(lib2->getLibname(), libname);
  EXPECT_EQ(lib2->getGDSStructures().size(), 2);

  dbGDSStructure* str1_read = lib2->findGDSStructure("str1");
  ASSERT_NE(str1_read, nullptr);
  EXPECT_EQ(str1_read->getGDSBoxs().size(), 1);

  dbGDSBox* box_read = *str1_read->getGDSBoxs().begin();
  EXPECT_EQ(box_read->getLayer(), 3);
  EXPECT_EQ(box_read->getDatatype(), 4);
  EXPECT_EQ(box_read->getPropattr().size(), 1);
  EXPECT_EQ(box_read->getPropattr()[0].first, 12);
  EXPECT_EQ(box_read->getPropattr()[0].second, "test");

  dbGDSStructure* str3_read = lib2->findGDSStructure("str3");
  ASSERT_NE(str3_read, nullptr);
  EXPECT_EQ(str3_read->getGDSSRefs().size(), 1);

  dbGDSSRef* sref_read = *str3_read->getGDSSRefs().begin();
  EXPECT_STREQ(sref_read->getStructure()->getName(), "str1");
  EXPECT_EQ(sref_read->getTransform().mag_, 2.0);
  EXPECT_EQ(sref_read->getTransform().angle_, 90);

  dbGDSStructure* ref_str = sref_read->getStructure();
  EXPECT_NE(ref_str, nullptr);
  EXPECT_EQ(ref_str, str1_read);
}

}  // namespace
}  // namespace odb::gds
