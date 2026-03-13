// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include <filesystem>
#include <string>
#include <vector>

#include "gtest/gtest.h"
#include "odb/db.h"
#include "odb/gdsin.h"
#include "odb/gdsout.h"
#include "odb/geom.h"
#include "tst/fixture.h"
#include "utl/Logger.h"

namespace odb::gds {
namespace {

static const std::string prefix("_main/src/odb/test/");

using tst::Fixture;

TEST_F(Fixture, writer)
{
  GDSReader reader(&logger_);
  std::string path = getFilePath(prefix + "data/sky130_fd_sc_hd__inv_1.gds");

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

// Verify GDS string records are padded to even byte length (GDS spec).
// Odd-length strings like "A" (1 byte) must be null-padded to 2 bytes.
TEST_F(Fixture, odd_length_string_padding)
{
  // Create a lib with a TEXT element whose string has odd length
  dbGDSLib* lib = dbGDSLib::create(db_.get(), "pad_test");
  lib->setUnits(1e-3, 1e-9);
  dbGDSStructure* str = dbGDSStructure::create(lib, "TOP");
  dbGDSText* txt = dbGDSText::create(str);
  txt->setLayer(1);
  txt->setDatatype(0);
  txt->setText("A");  // 1 byte = odd
  txt->setOrigin(Point(0, 0));

  // Also add a 3-char (odd) string
  dbGDSText* txt3 = dbGDSText::create(str);
  txt3->setLayer(1);
  txt3->setDatatype(0);
  txt3->setText("VDD");  // 3 bytes = odd
  txt3->setOrigin(Point(100, 0));

  // Write and re-read
  std::filesystem::create_directory("results");
  const char* outpath = "results/pad_test.gds";
  GDSWriter writer(&logger_);
  writer.write_gds(lib, outpath);

  GDSReader reader(&logger_);
  dbGDSLib* lib2 = reader.read_gds(outpath, db_.get());

  dbGDSStructure* str2 = lib2->findGDSStructure("TOP");
  ASSERT_NE(str2, nullptr);
  EXPECT_EQ(str2->getGDSTexts().size(), 2);

  auto it = str2->getGDSTexts().begin();
  EXPECT_EQ((*it)->getText(), "A");
  ++it;
  EXPECT_EQ((*it)->getText(), "VDD");

  // Verify file size is even (all records must be even-length)
  auto fsize = std::filesystem::file_size(outpath);
  EXPECT_EQ(fsize % 2, 0) << "GDS file size should be even (all records padded)";
}

}  // namespace
}  // namespace odb::gds
