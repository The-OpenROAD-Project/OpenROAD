// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include <fstream>
#include <string>

#include "gtest/gtest.h"
#include "odb/db.h"
#include "odb/lefin.h"
#include "tst/db_fixture.h"
#include "utl/Logger.h"

namespace odb {
namespace {

class LefInRounding : public tst::DbFixture
{
 protected:
  // Writes a tech LEF with a single routing layer M1, whose remaining
  // statements are layer_body, reads it and returns the log output.
  std::string readTech(const int dbu, const std::string& layer_body)
  {
    const std::string path
        = testing::TempDir() + "/"
          + testing::UnitTest::GetInstance()->current_test_info()->name()
          + ".lef";
    std::ofstream lef(path);
    lef << "VERSION 5.8 ;\n"
        << "UNITS\n"
        << "  DATABASE MICRONS " << dbu << " ;\n"
        << "END UNITS\n"
        << "LAYER M1\n"
        << "  TYPE ROUTING ;\n"
        << "  DIRECTION HORIZONTAL ;\n"
        << "  PITCH 0.2 ;\n"
        << layer_body << "END M1\n"
        << "END LIBRARY\n";
    lef.close();

    lefin reader(db_.get(), &logger_, /*ignore_non_routing_layers=*/false);
    logger_.redirectStringBegin();
    tech_ = reader.createTech("tech", path.c_str());
    return logger_.redirectStringEnd();
  }

  dbTechLayer* m1() const { return tech_->findLayer("M1"); }

  dbTech* tech_ = nullptr;
};

TEST_F(LefInRounding, OnGridValuesDoNotWarn)
{
  // 0.0125 is off the 1000 DBU grid as a distance, but exact as an area.
  const std::string log = readTech(1000,
                                   "  WIDTH 0.05 ;\n"
                                   "  SPACING 0.05 ;\n"
                                   "  AREA 0.0125 ;\n");

  EXPECT_EQ(logger_.getWarningCount(), 0);
  EXPECT_EQ(log.find("ODB-1220"), std::string::npos);
  EXPECT_EQ(m1()->getWidth(), 50);
}

TEST_F(LefInRounding, OffGridDistanceSuggestsFinerDbu)
{
  const std::string log = readTech(1000, "  WIDTH 0.0125 ;\n");

  EXPECT_EQ(logger_.getWarningCount(), 1);
  EXPECT_NE(log.find("[WARNING ODB-1220]"), std::string::npos);
  EXPECT_NE(
      log.find("rounded 1 conversions that are not on the 1000 DBU/micron "
               "grid (first: 0.0125 um -> 13 DBU near line"),
      std::string::npos)
      << log;
  EXPECT_NE(log.find("2000 DBU/micron would represent them exactly."),
            std::string::npos)
      << log;
  EXPECT_EQ(m1()->getWidth(), 13);
}

TEST_F(LefInRounding, OffGridAreaSuggestsFinerDbu)
{
  const std::string log = readTech(1000,
                                   "  WIDTH 0.05 ;\n"
                                   "  AREA 0.0021875 ;\n");

  EXPECT_EQ(logger_.getWarningCount(), 1);
  EXPECT_NE(
      log.find("rounded 1 conversions that are not on the 1000 DBU/micron "
               "grid (first: 0.0021875 um^2 -> 2188 DBU^2 near line"),
      std::string::npos)
      << log;
  EXPECT_NE(log.find("2000 DBU/micron would represent them exactly."),
            std::string::npos)
      << log;
}

TEST_F(LefInRounding, HintRepresentsEveryRoundedValue)
{
  // 0.05025 is exact at 4000, 8000 and 20000; 0.0501 at 10000 and 20000.
  // Neither value's own smallest exact grid works for the other.
  const std::string log = readTech(1000,
                                   "  WIDTH 0.05025 ;\n"
                                   "  OFFSET 0.0501 ;\n");

  EXPECT_EQ(logger_.getWarningCount(), 1);
  EXPECT_NE(
      log.find("rounded 2 conversions that are not on the 1000 DBU/micron "
               "grid (first: 0.05025 um -> 50 DBU near line"),
      std::string::npos)
      << log;
  EXPECT_NE(log.find("20000 DBU/micron would represent them exactly."),
            std::string::npos)
      << log;
}

TEST_F(LefInRounding, NoSupportedDbuRepresentsValue)
{
  const std::string log = readTech(1000, "  WIDTH 0.05001 ;\n");

  EXPECT_EQ(logger_.getWarningCount(), 1);
  EXPECT_NE(
      log.find("rounded 1 conversions that are not on the 1000 DBU/micron "
               "grid (first: 0.05001 um -> 50 DBU near line"),
      std::string::npos)
      << log;
  EXPECT_NE(log.find("No supported DBU/micron represents them exactly."),
            std::string::npos)
      << log;
}

TEST_F(LefInRounding, FreePdk45HasNoWarnings)
{
  const std::string path = getFilePath("_main/test/Nangate45/Nangate45.lef");
  lefin reader(db_.get(), &logger_, /*ignore_non_routing_layers=*/false);
  logger_.redirectStringBegin();
  dbLib* lib = reader.createTechAndLib("ng45", "ng45", path.c_str());
  const std::string log = logger_.redirectStringEnd();

  ASSERT_NE(lib, nullptr);
  EXPECT_EQ(logger_.getWarningCount(), 0);
  EXPECT_EQ(log.find("WARNING"), std::string::npos);
}

}  // namespace
}  // namespace odb
