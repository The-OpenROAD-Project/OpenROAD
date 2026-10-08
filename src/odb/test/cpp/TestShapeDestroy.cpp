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
  dbObstruction* obs = dbObstruction::create(
      block_, layer_, far_.xMin(), far_.yMin(), far_.xMax(), far_.yMax());
  EXPECT_EQ(getBBox(), grown_);
  dbObstruction::destroy(obs);
  EXPECT_EQ(getBBox(), initial_);
}

TEST_F(ShapeDestroyFixture, obstruction_destroy_frees_box)
{
  dbObstruction* obs = dbObstruction::create(
      block_, layer_, far_.xMin(), far_.yMin(), far_.xMax(), far_.yMax());
  const uint32_t box_id = obs->getBBox()->getId();
  dbObstruction::destroy(obs);
  // The box table hands out the most recently freed slot first, so the new
  // obstruction only gets box_id back if the destroy freed it.
  obs = dbObstruction::create(
      block_, layer_, far_.xMin(), far_.yMin(), far_.xMax(), far_.yMax());
  EXPECT_EQ(obs->getBBox()->getId(), box_id);
}

}  // namespace
}  // namespace odb
