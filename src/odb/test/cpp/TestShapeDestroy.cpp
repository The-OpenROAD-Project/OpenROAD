// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include <cstdint>

#include "gtest/gtest.h"
#include "helper.h"
#include "odb/db.h"
#include "odb/dbTypes.h"
#include "odb/geom.h"

namespace odb {
namespace {

// Destroying a shape must release everything the shape held in the block.
class ShapeDestroyFixture : public SimpleDbFixture
{
 protected:
  ShapeDestroyFixture()
  {
    create2LevetDbNoBTerms();
    block_ = db_->getChip()->getBlock();
    layer_ = db_->getTech()->findLayer("L1");
    net_ = block_->findNet("n1");

    // Place one instance so the block has a non-empty extent, then read the
    // bbox so its cache is valid before a test adds a shape.
    dbInst* inst = block_->findInst("i1");
    inst->setLocation(0, 0);
    inst->setPlacementStatus(dbPlacementStatus::PLACED);
    EXPECT_EQ(getBBox(), initial_);
  }

  Rect getBBox() { return block_->getBBox()->getBox(); }

  dbSBox* createFarSBox(dbSWire* swire)
  {
    dbSBox* sbox = dbSBox::create(swire,
                                  layer_,
                                  far_.xMin(),
                                  far_.yMin(),
                                  far_.xMax(),
                                  far_.yMax(),
                                  dbWireShapeType::STRIPE);
    EXPECT_EQ(getBBox(), grown_);
    return sbox;
  }

  dbObstruction* createFarObstruction()
  {
    return dbObstruction::create(
        block_, layer_, far_.xMin(), far_.yMin(), far_.xMax(), far_.yMax());
  }

  static void tag(dbObject* object) { dbIntProperty::create(object, "tag", 1); }

  // A table hands out its most recently freed slot first. So the replacement
  // gets the old id back only if the destroy freed it, and it must not
  // inherit the properties left on that slot.
  static void expectFreshSlot(dbObject* replacement, uint32_t old_id)
  {
    EXPECT_EQ(replacement->getId(), old_id);
    EXPECT_EQ(dbProperty::find(replacement, "tag"), nullptr);
  }

  dbBlock* block_ = nullptr;
  dbTechLayer* layer_ = nullptr;
  dbNet* net_ = nullptr;
  const Rect initial_{0, 0, 1000, 1000};
  const Rect far_{5000, 5000, 6000, 6000};
  const Rect grown_{0, 0, 6000, 6000};
};

TEST_F(ShapeDestroyFixture, sbox_destroy_restores_bbox)
{
  dbSWire* swire = dbSWire::create(net_, dbWireType::ROUTED);
  dbSBox* sbox = createFarSBox(swire);
  dbSBox::destroy(sbox);
  EXPECT_EQ(getBBox(), initial_);
}

TEST_F(ShapeDestroyFixture, swire_destroy_restores_bbox)
{
  dbSWire* swire = dbSWire::create(net_, dbWireType::ROUTED);
  createFarSBox(swire);
  dbSWire::destroy(swire);
  EXPECT_EQ(getBBox(), initial_);
}

TEST_F(ShapeDestroyFixture, net_destroy_swires_restores_bbox)
{
  dbSWire* swire = dbSWire::create(net_, dbWireType::ROUTED);
  createFarSBox(swire);
  net_->destroySWires();
  EXPECT_EQ(getBBox(), initial_);
}

TEST_F(ShapeDestroyFixture, net_destroy_restores_bbox)
{
  dbSWire* swire = dbSWire::create(net_, dbWireType::ROUTED);
  createFarSBox(swire);
  dbNet::destroy(net_);
  EXPECT_EQ(getBBox(), initial_);
}

TEST_F(ShapeDestroyFixture, obstruction_destroy_restores_bbox)
{
  dbObstruction* obs = createFarObstruction();
  EXPECT_EQ(getBBox(), grown_);
  dbObstruction::destroy(obs);
  EXPECT_EQ(getBBox(), initial_);
}

TEST_F(ShapeDestroyFixture, obstruction_destroy_frees_box)
{
  dbObstruction* obs = createFarObstruction();
  dbBox* box = obs->getBBox();
  const uint32_t box_id = box->getId();
  tag(box);
  dbObstruction::destroy(obs);
  expectFreshSlot(createFarObstruction()->getBBox(), box_id);
}

TEST_F(ShapeDestroyFixture, blockage_destroy_frees_box)
{
  dbBlockage* blockage = dbBlockage::create(block_, 0, 0, 100, 100);
  dbBox* box = blockage->getBBox();
  const uint32_t blockage_id = blockage->getId();
  const uint32_t box_id = box->getId();
  tag(blockage);
  tag(box);
  dbBlockage::destroy(blockage);
  blockage = dbBlockage::create(block_, 0, 0, 100, 100);
  expectFreshSlot(blockage, blockage_id);
  expectFreshSlot(blockage->getBBox(), box_id);
}

TEST_F(ShapeDestroyFixture, box_destroy_frees_bpin_box)
{
  dbBPin* bpin = dbBPin::create(dbBTerm::create(net_, "p"));
  dbBox* box = dbBox::create(bpin, layer_, 0, 0, 100, 100);
  const uint32_t box_id = box->getId();
  tag(box);
  dbBox::destroy(box);
  expectFreshSlot(dbBox::create(bpin, layer_, 0, 0, 100, 100), box_id);
}

TEST_F(ShapeDestroyFixture, box_destroy_frees_halo)
{
  dbInst* inst = block_->findInst("i2");
  dbBox* halo = dbBox::create(inst, 10, 10, 10, 10);
  const uint32_t halo_id = halo->getId();
  tag(halo);
  dbBox::destroy(halo);
  expectFreshSlot(dbBox::create(inst, 10, 10, 10, 10), halo_id);
}

}  // namespace
}  // namespace odb
