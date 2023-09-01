#include <unistd.h>

#include <memory>
#include <stdexcept>
#include <vector>

#include "dpl/Opendp.h"
#include "gtest/gtest.h"
#include "odb/db.h"
#include "odb/lefin.h"
#include "utl/Logger.h"

namespace dpl {

class OpendpTest : public ::testing::Test
{
 protected:
  template <class T>
  using OdbUniquePtr = std::unique_ptr<T, void (*)(T*)>;

  void SetUp() override
  {
    db_ = OdbUniquePtr<odb::dbDatabase>(odb::dbDatabase::create(),
                                        &odb::dbDatabase::destroy);
    odb::lefin lef_reader(
        db_.get(), &logger_, /*ignore_non_routing_layers=*/false);

    lib_ = OdbUniquePtr<odb::dbLib>(
        lef_reader.createTechAndLib(
            "tech", "OpenDpTest", "sky130hd/sky130hd.tlef"),
        &odb::dbLib::destroy);

    lef_reader.updateLib(lib_.get(), "sky130hd/sky130_fd_sc_hd_merged.lef");

    chip_ = OdbUniquePtr<odb::dbChip>(odb::dbChip::create(db_.get()),
                                      &odb::dbChip::destroy);
    block_ = OdbUniquePtr<odb::dbBlock>(
        odb::dbBlock::create(chip_.get(), "top"), &odb::dbBlock::destroy);
    block_->setDefUnits(lib_->getTech()->getLefUnits());
    block_->setDieArea(odb::Rect(0, 0, 1000, 1000));
  }

  utl::Logger logger_;
  OdbUniquePtr<odb::dbDatabase> db_{nullptr, &odb::dbDatabase::destroy};
  OdbUniquePtr<odb::dbLib> lib_{nullptr, &odb::dbLib::destroy};
  OdbUniquePtr<odb::dbChip> chip_{nullptr, &odb::dbChip::destroy};
  OdbUniquePtr<odb::dbBlock> block_{nullptr, &odb::dbBlock::destroy};
};

TEST_F(OpendpTest, IsPlaced)
{
  odb::dbMaster* and_gate = lib_->findMaster("sky130_fd_sc_hd__and2_1");
  auto and_placed = OdbUniquePtr<odb::dbInst>(
      odb::dbInst::create(block_.get(), and_gate, "and_1"),
      &odb::dbInst::destroy);

  and_placed->setPlacementStatus(odb::dbPlacementStatus::PLACED);

  Cell placed;
  placed.db_inst_ = and_placed.get();

  // Act & Assert
  ASSERT_TRUE(Opendp::isPlaced(&placed));
}

TEST_F(OpendpTest, AbuttedCellsWithGlobalPaddingFailPlacementChecks)
{
  odb::dbMaster* and_gate = lib_->findMaster("sky130_fd_sc_hd__and2_1");
  odb::dbSite* site = and_gate->getSite();
  int and_gate_site_count = and_gate->getWidth() / site->getWidth();

  odb::dbRow::create(block_.get(),
                     "row1",
                     and_gate->getSite(),
                     /*origin_x*/ 0,
                     /*origin_y*/ 0,
                     odb::dbOrientType::R0,
                     odb::dbRowDir::HORIZONTAL,
                     /*num_sites*/ and_gate_site_count * 25,  // 25 AND2 gates
                     site->getWidth());

  auto and1_placed = OdbUniquePtr<odb::dbInst>(
      odb::dbInst::create(block_.get(), and_gate, "and_1"),
      &odb::dbInst::destroy);
  auto and2_placed = OdbUniquePtr<odb::dbInst>(
      odb::dbInst::create(block_.get(), and_gate, "and_2"),
      &odb::dbInst::destroy);

  Opendp opendp;
  opendp.init(db_.get(), &logger_);
  opendp.setPaddingGlobal(/* left */ 1, /* right */ 1);

  /*
  -----------------------------------------------------
  | and1_placed | and2_placed | ... | empty_sites | ...
  -----------------------------------------------------
  */
  and1_placed->setLocation(0, 0);
  and2_placed->setLocation(and1_placed->getBBox()->getWidth(), 0);
  and1_placed->setPlacementStatus(odb::dbPlacementStatus::PLACED);
  and2_placed->setPlacementStatus(odb::dbPlacementStatus::PLACED);

  // Act & Assert
  EXPECT_THROW(opendp.checkPlacement(/* verbose */ true,
                                     /*disallow_one_site_gaps*/ false,
                                     /*report_file_name*/ ""),
               std::runtime_error);
}

TEST_F(OpendpTest, CoreTypeCellsPlacedByFillerDoNotTriggerPlacementChecks)
{
  odb::dbMaster* and_gate = lib_->findMaster("sky130_fd_sc_hd__and2_1");
  odb::dbSite* site = and_gate->getSite();
  int and_gate_site_count = and_gate->getWidth() / site->getWidth();

  odb::dbRow::create(block_.get(),
                     "row1",
                     and_gate->getSite(),
                     /*origin_x*/ 0,
                     /*origin_y*/ 0,
                     odb::dbOrientType::R0,
                     odb::dbRowDir::HORIZONTAL,
                     /*num_sites*/ and_gate_site_count * 25,  // 25 AND2 gates
                     site->getWidth());

  std::vector<odb::dbMaster*> filler_list{and_gate};

  Opendp opendp;
  opendp.init(db_.get(), &logger_);
  opendp.setPaddingGlobal(/* left */ 1, /* right */ 1);

  /*
  -------------------------------------------------------------------
  | and1_placed (used as filler) | and2_placed (used as filler) | ...
  -------------------------------------------------------------------
  */
  opendp.fillerPlacement(&filler_list, "filler_");

  // Act & Assert
  EXPECT_NO_THROW(opendp.checkPlacement(/* verbose */ true,
                                        /*disallow_one_site_gaps*/ false,
                                        /*report_file_name*/ ""));
}

TEST_F(OpendpTest, CoreTypeCellsPlacedByFillerAreRemovedByRemoveFiller)
{
  odb::dbMaster* and_gate = lib_->findMaster("sky130_fd_sc_hd__and2_1");
  odb::dbSite* site = and_gate->getSite();
  int and_gate_site_count = and_gate->getWidth() / site->getWidth();

  odb::dbRow::create(block_.get(),
                     "row1",
                     and_gate->getSite(),
                     /*origin_x*/ 0,
                     /*origin_y*/ 0,
                     odb::dbOrientType::R0,
                     odb::dbRowDir::HORIZONTAL,
                     /*num_sites*/ and_gate_site_count * 25,  // 25 AND2 gates
                     site->getWidth());

  std::vector<odb::dbMaster*> filler_list{and_gate};

  Opendp opendp;
  opendp.init(db_.get(), &logger_);
  opendp.setPaddingGlobal(/* left */ 1, /* right */ 1);

  /*
  -------------------------------------------------------------------
  | and1_placed (used as filler) | and2_placed (used as filler) | ...
  -------------------------------------------------------------------
  */

  // Act & Assert
  opendp.fillerPlacement(&filler_list, "filler_");
  EXPECT_EQ(block_->getInsts().size(), 25);

  opendp.removeFillers();
  EXPECT_EQ(block_->getInsts().size(), 0);
}

}  // namespace dpl
