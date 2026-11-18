// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include <filesystem>
#include <fstream>
#include <map>
#include <sstream>
#include <string>

#include "gtest/gtest.h"
#include "odb/db.h"
#include "odb/dbTypes.h"
#include "odb/defout.h"
#include "tst/db_fixture.h"

namespace odb {
namespace {

class DefBlockagesTest : public tst::DbFixture
{
 protected:
  void SetUp() override
  {
    db_->setDbuPerMicron(1000);
    tech_ = dbTech::create(db_.get(), "tech");
    layer_ = dbTechLayer::create(tech_, "M1", dbTechLayerType::ROUTING);
    dbLib::create(db_.get(), "lib", tech_, ',');
    path_
        = std::filesystem::path(testing::TempDir())
          / (std::string(
                 testing::UnitTest::GetInstance()->current_test_info()->name())
             + ".def");
  }

  void TearDown() override { std::filesystem::remove(path_); }

  void checkExport(const DefOut::Version version, const bool except_pg)
  {
    dbChip* chip = dbChip::create(db_.get(), tech_);
    dbBlock* block = dbBlock::create(chip, "test");
    block->setDefUnits(1000);
    block->setDieArea(Rect(0, 0, 1000, 1000));
    dbObstruction* flagged
        = dbObstruction::create(block, layer_, 100, 100, 200, 200);
    flagged->setExceptPGNetsObstruction();
    dbObstruction::create(block, layer_, 300, 100, 400, 200);

    DefOut writer(&logger_);
    writer.setVersion(version);
    ASSERT_TRUE(writer.writeBlock(block, path_.c_str()));

    std::ifstream input(path_);
    std::ostringstream text;
    text << input.rdbuf();
    EXPECT_EQ(text.str().find("+ EXCEPTPGNET") != std::string::npos, except_pg);

    dbChip::destroy(chip);
    block = readDef(path_.string());
    ASSERT_EQ(block->getObstructions().size(), 2);
    for (dbObstruction* obs : block->getObstructions()) {
      EXPECT_EQ(obs->isExceptPGNetsObstruction(),
                except_pg && obs->getBBox()->xMin() == 100);
    }
  }

  dbTech* tech_ = nullptr;
  dbTechLayer* layer_ = nullptr;
  std::filesystem::path path_;
};

TEST_F(DefBlockagesTest, ReadExceptPGNet)
{
  std::ofstream output(path_);
  output << R"(VERSION 5.8 ;
DIVIDERCHAR "/" ;
BUSBITCHARS "[]" ;
DESIGN test ;
UNITS DISTANCE MICRONS 1000 ;
DIEAREA ( 0 0 ) ( 1000 1000 ) ;
BLOCKAGES 4 ;
  - LAYER M1 + EXCEPTPGNET
    RECT ( 100 100 ) ( 200 200 )
    RECT ( 300 100 ) ( 400 200 ) ;
  - LAYER M1 RECT ( 500 100 ) ( 600 200 ) ;
  - LAYER M1 + EXCEPTPGNET
    POLYGON ( 700 100 ) ( 800 100 ) ( 800 200 ) ( 700 200 ) ;
  - LAYER M1
    POLYGON ( 900 100 ) ( 1000 100 ) ( 1000 200 ) ( 900 200 ) ;
END BLOCKAGES
END DESIGN
)";
  output.close();

  dbBlock* block = readDef(path_.string());
  const std::map<int, bool> expected{
      {100, true}, {300, true}, {500, false}, {700, true}, {900, false}};
  ASSERT_EQ(block->getObstructions().size(), expected.size());
  for (dbObstruction* obs : block->getObstructions()) {
    EXPECT_EQ(obs->isExceptPGNetsObstruction(),
              expected.at(obs->getBBox()->xMin()));
  }
}

TEST_F(DefBlockagesTest, WriteExceptPGNet57)
{
  checkExport(DefOut::DEF_5_7, true);
}

TEST_F(DefBlockagesTest, WriteExceptPGNet58)
{
  checkExport(DefOut::DEF_5_8, true);
}

TEST_F(DefBlockagesTest, WriteExceptPGNet56)
{
  checkExport(DefOut::DEF_5_6, false);
}

}  // namespace
}  // namespace odb
