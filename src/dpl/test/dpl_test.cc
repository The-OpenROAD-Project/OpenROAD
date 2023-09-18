#include <unistd.h>

#include <memory>

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
        lef_reader.createTechAndLib("tech",
                                    "isPlacedTestLibName",
                                    "sky130hd/sky130_fd_sc_hd_merged.lef"),
        &odb::dbLib::destroy);

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

}  // namespace dpl
