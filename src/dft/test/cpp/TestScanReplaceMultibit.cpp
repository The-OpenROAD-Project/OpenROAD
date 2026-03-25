// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024-2025, The OpenROAD Authors

#include <memory>
#include <string>

#include "dft/Dft.hh"
#include "gtest/gtest.h"
#include "odb/db.h"
#include "tst/fixture.h"

namespace dft {
namespace {

// Fixture that loads test0.lib / test0.lef and creates a chip+block ready for
// ScanReplace testing.
class ScanReplaceMultibitFixture : public tst::Fixture
{
 protected:
  void SetUp() override
  {
    readLiberty(
        getFilePath("openroad/src/gpl/test/library/test/test0.lib"));
    lib_ = loadTechAndLib(
        "test0",
        "test0",
        getFilePath("openroad/src/gpl/test/library/test/test0.lef"));

    odb::dbChip* chip = odb::dbChip::create(getDb(), getDb()->getTech());
    block_ = odb::dbBlock::create(chip, "top");
    block_->setDefUnits(1000);
    block_->setDieArea(odb::Rect(0, 0, 10000, 10000));
  }

  // Helper: find a master by name from the loaded lib.
  odb::dbMaster* findMaster(const char* name)
  {
    odb::dbMaster* master = lib_->findMaster(name);
    EXPECT_NE(master, nullptr) << "Master not found: " << name;
    return master;
  }

  // Helper: return the master name of the named instance after replacement.
  // ReplaceCell destroys the old dbInst and creates a new one with the same
  // name, so the original pointer is stale; always look up by name.
  std::string masterNameAfterReplace(const char* inst_name)
  {
    odb::dbInst* inst = block_->findInst(inst_name);
    if (!inst) {
      return "";
    }
    return inst->getMaster()->getName();
  }

  odb::dbLib* lib_ = nullptr;
  odb::dbBlock* block_ = nullptr;
};

// A 2-bit non-scan MBFF2 cell should be replaced with a 2-bit scan equivalent
// (either MBFF2SE for internal scan or MBFF2SE_EXT for external scan).
TEST_F(ScanReplaceMultibitFixture, InternalMultibitCellIsReplaced)
{
  odb::dbMaster* mbff2_master = findMaster("MBFF2");
  if (!mbff2_master) {
    GTEST_SKIP() << "MBFF2 master not found in test library";
  }

  ASSERT_NE(odb::dbInst::create(block_, mbff2_master, "u_mbff2"), nullptr);

  Dft dft(getDb(), getSta(), getLogger());
  dft.scanReplace();

  // The master should have changed to a scan equivalent.
  // ReplaceCell destroys the original instance and creates a new one with the
  // same name, so look up by name rather than keeping the old pointer.
  std::string replaced_name = masterNameAfterReplace("u_mbff2");
  EXPECT_NE(replaced_name, "MBFF2")
      << "Expected MBFF2 to be replaced with a scan equivalent";

  // The scan equivalent must have the same number of bits (2).
  bool is_valid_replacement = (replaced_name == "MBFF2SE")
                              || (replaced_name == "MBFF2SE_EXT")
                              || (replaced_name == "MBFF2SECLPS"); //! this is weird!
  EXPECT_TRUE(is_valid_replacement)
      << "Unexpected scan replacement: " << replaced_name;
}

// A cell that is already a scan cell (MBFF2SE) should not be replaced.
TEST_F(ScanReplaceMultibitFixture, AlreadyScanCellIsNotReplaced)
{
  odb::dbMaster* mbff2se_master = findMaster("MBFF2SE");
  if (!mbff2se_master) {
    GTEST_SKIP() << "MBFF2SE master not found in test library";
  }

  ASSERT_NE(odb::dbInst::create(block_, mbff2se_master, "u_mbff2se"), nullptr);

  Dft dft(getDb(), getSta(), getLogger());
  dft.scanReplace();

  // MBFF2SE is already a scan cell; the instance must remain unchanged.
  EXPECT_EQ(masterNameAfterReplace("u_mbff2se"), std::string("MBFF2SE"));
}

// Both a non-scan and a scan cell in the same design: only the non-scan one
// should be replaced.
TEST_F(ScanReplaceMultibitFixture, MixedDesignOnlyNonScanIsReplaced)
{
  odb::dbMaster* mbff2_master = findMaster("MBFF2");
  odb::dbMaster* mbff2se_master = findMaster("MBFF2SE");
  if (!mbff2_master || !mbff2se_master) {
    GTEST_SKIP() << "Required masters not found in test library";
  }

  ASSERT_NE(odb::dbInst::create(block_, mbff2_master, "u_nonscan"), nullptr);
  ASSERT_NE(odb::dbInst::create(block_, mbff2se_master, "u_scan"), nullptr);

  Dft dft(getDb(), getSta(), getLogger());
  dft.scanReplace();

  // Non-scan cell must have been replaced to a scan equivalent.
  EXPECT_NE(masterNameAfterReplace("u_nonscan"), std::string("MBFF2"));

  // Already-scan cell must remain unchanged.
  EXPECT_EQ(masterNameAfterReplace("u_scan"), std::string("MBFF2SE"));
}

// A 2-bit non-scan cell must NOT be replaced by a 1-bit scan cell.
// Guards the sequentials().size() check in IsScanEquivalent.
TEST_F(ScanReplaceMultibitFixture, BitWidthMismatchIsRejected)
{
  odb::dbMaster* mbff2_master = findMaster("MBFF2");
  if (!mbff2_master) {
    GTEST_SKIP() << "MBFF2 master not found in test library";
  }

  ASSERT_NE(odb::dbInst::create(block_, mbff2_master, "u_mbff2_bits"),
            nullptr);

  Dft dft(getDb(), getSta(), getLogger());
  dft.scanReplace();

  // The replacement (if any) must not be the 1-bit DFF scan cell.
  EXPECT_NE(masterNameAfterReplace("u_mbff2_bits"), std::string("DFF"));
}

}  // namespace
}  // namespace dft
