// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include <filesystem>
#include <string>

#include "gtest/gtest.h"
#include "odb/db.h"
#include "odb/gdsUtil.h"
#include "odb/gdsin.h"
#include "odb/gdsout.h"
#include "odb/geom.h"
#include "tst/fixture.h"

namespace odb::gds {
namespace {

using tst::Fixture;

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
