// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include <string>
#include <vector>

#include "gtest/gtest.h"
#include "odb/db.h"
#include "odb/gdsin.h"
#include "odb/geom.h"
#include "tst/fixture.h"

namespace odb::gds {
namespace {

static const std::string prefix("_main/src/odb/test/");

using tst::Fixture;

TEST_F(Fixture, reader)
{
  GDSReader reader(&logger_);
  std::string path = getFilePath(prefix + "data/sky130_fd_sc_hd__inv_1.gds");

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

}  // namespace
}  // namespace odb::gds
