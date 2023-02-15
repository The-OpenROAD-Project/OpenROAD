#include <unistd.h>

#include "dpl/Opendp.h"
#include "gtest/gtest.h"
#include "odb/db.h"
#include "odb/lefin.h"
#include "utl/Logger.h"

namespace dpl {

TEST(Dpl, IsPlaced)
{
  // Arrange
  utl::Logger logger;
  odb::dbDatabase* db = odb::dbDatabase::create();

  odb::lefin lef_reader(db, &logger, /*ignore_non_routing_layers=*/false);
  odb::dbLib* lib = lef_reader.createTechAndLib(
      "isPlacedTestLibName", "sky130hd/sky130_fd_sc_hd_merged.lef");

  ASSERT_NE(lib, nullptr);

  odb::dbChip* chip = odb::dbChip::create(db);
  odb::dbBlock* block = odb::dbBlock::create(chip, "top");
  block->setDefUnits(lib->getTech()->getLefUnits());
  block->setDieArea(odb::Rect(0, 0, 1000, 1000));

  odb::dbMaster* and_gate = lib->findMaster("sky130_fd_sc_hd__and2_1");
  odb::dbInst* and_placed = odb::dbInst::create(block, and_gate, "and_1");
  odb::dbInst* and_unplaced = odb::dbInst::create(block, and_gate, "and_2");

  and_placed->setPlacementStatus(odb::dbPlacementStatus::PLACED);
  and_unplaced->setPlacementStatus(odb::dbPlacementStatus::UNPLACED);

  Cell placed, unplaced;
  placed.db_inst_ = and_placed;
  unplaced.db_inst_ = and_unplaced;

  // Act & Assert
  ASSERT_TRUE(Opendp::isPlaced(&placed));
  ASSERT_FALSE(Opendp::isPlaced(&unplaced));
}

}  // namespace dpl
