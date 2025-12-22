// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include <cassert>

#include "odb/db.h"
#include "odb/dbShape.h"
#include "odb/dbTypes.h"
#include "odb/geom.h"

namespace odb {

static bool isFiltered(unsigned filter, unsigned mask)
{
  return (filter & mask) == mask;
}

dbHierInstShapeItr::dbHierInstShapeItr(dbShapeItrCallback* callback)
{
  callback_ = callback;
  assert(callback);
}

void dbHierInstShapeItr::iterate(dbInst* inst, unsigned filter)
{
  transforms_.clear();
  transforms_.emplace_back();
  iterate_inst(inst, filter, 0);
}

void dbHierInstShapeItr::push_transform(const dbTransform& t)
{
  dbTransform top = transforms_.back();
  top.concat(t);
  transforms_.push_back(top);
}

bool dbHierInstShapeItr::iterate_leaf(dbInst* inst, unsigned filter, int level)
{
  if (isFiltered(filter, INST_OBS | INST_VIA | INST_PIN)) {
    return true;
  }

  callback_->beginInst(inst, level);

  push_transform(inst->getTransform());
  dbMaster* master = inst->getMaster();

  if (!isFiltered(filter, INST_OBS | INST_VIA)) {
    callback_->beginObstructions(master);
    const bool filter_via = isFiltered(filter, INST_VIA);
    const bool filter_obs = isFiltered(filter, INST_OBS);

    dbShape s;

    for (dbBox* box : master->getObstructions()) {
      if (box->isVia()) {
        if (!filter_via) {
          getShape(box, s);

          if (!callback_->nextBoxShape(box, s)) {
            transforms_.pop_back();
            callback_->endObstructions();
            callback_->endInst();
            return false;
          }
        }

      } else {
        if (!filter_obs) {
          getShape(box, s);

          if (!callback_->nextBoxShape(box, s)) {
            transforms_.pop_back();
            callback_->endObstructions();
            callback_->endInst();
            return false;
          }
        }
      }
    }

    callback_->endObstructions();
  }

  if (!isFiltered(filter, INST_PIN)) {
    dbShape s;

    for (dbMTerm* mterm : master->getMTerms()) {
      callback_->beginMTerm(mterm);
      for (dbMPin* pin : mterm->getMPins()) {
        callback_->beginMPin(pin);
        for (dbBox* box : pin->getGeometry()) {
          getShape(box, s);

          if (!callback_->nextBoxShape(box, s)) {
            transforms_.pop_back();
            callback_->endMPin();
            callback_->endMTerm();
            callback_->endInst();
            return false;
          }
        }
        callback_->endMPin();
      }
      callback_->endMTerm();
    }
  }

  transforms_.pop_back();
  callback_->endInst();
  return true;
}

bool dbHierInstShapeItr::iterate_inst(dbInst* inst, unsigned filter, int level)
{
  if (!inst->isHierarchical()) {
    return iterate_leaf(inst, filter, level);
  }

  callback_->beginInst(inst, level);

  push_transform(inst->getTransform());
  dbBlock* child = inst->getChild();
  dbShape shape;

  if (!isFiltered(filter, BLOCK_PIN)) {
    for (dbBTerm* bterm : child->getBTerms()) {
      for (dbBPin* pin : bterm->getBPins()) {
        callback_->beginBPin(pin);

        for (dbBox* box : pin->getBoxes()) {
          getShape(box, shape);
          if (!callback_->nextBoxShape(box, shape)) {
            transforms_.pop_back();
            callback_->endBPin();
            callback_->endInst();
            return false;
          }
        }

        callback_->endBPin();
      }
    }
  }

  if (!isFiltered(filter, BLOCK_OBS)) {
    for (dbObstruction* obs : child->getObstructions()) {
      dbBox* box = obs->getBBox();
      getShape(box, shape);
      callback_->beginObstruction(obs);

      if (!callback_->nextBoxShape(box, shape)) {
        transforms_.pop_back();
        callback_->endObstruction();
        callback_->endInst();
        return false;
      }

      callback_->endObstruction();
    }
  }

  for (dbInst* inst : child->getInsts()) {
    if (!iterate_inst(inst, filter, ++level)) {
      transforms_.pop_back();
      return false;
    }
  }

  for (dbNet* net : child->getNets()) {
    callback_->beginNet(net);

    bool draw_segments;
    bool draw_vias;

    if (!drawNet(filter, net, draw_vias, draw_segments)) {
      continue;
    }

    if (!isFiltered(filter, NET_SWIRE)) {
      if (!iterate_swires(filter, net, draw_vias, draw_segments)) {
        transforms_.pop_back();
        callback_->endNet();
        callback_->endInst();
        return false;
      }
    }

    if (!isFiltered(filter, NET_WIRE)) {
      if (!iterate_wire(filter, net, draw_vias, draw_segments)) {
        transforms_.pop_back();
        callback_->endNet();
        callback_->endInst();
        return false;
      }
    }

    callback_->endNet();
  }

  transforms_.pop_back();
  callback_->endInst();
  return true;
}

bool dbHierInstShapeItr::drawNet(unsigned filter,
                                 dbNet* net,
                                 bool& draw_via,
                                 bool& draw_segment)
{
  dbSigType type = net->getSigType();
  draw_via = false;
  draw_segment = false;

  switch (type.getValue()) {
    case dbSigType::SCAN:
    case dbSigType::ANALOG:
    case dbSigType::TIEOFF:
    case dbSigType::SIGNAL: {
      if (isFiltered(filter, SIGNAL_WIRE | SIGNAL_VIA)) {
        return false;
      }

      if (!isFiltered(filter, SIGNAL_WIRE)) {
        draw_segment = true;
      }

      if (!isFiltered(filter, SIGNAL_VIA)) {
        draw_via = true;
      }

      return true;
    }

    case dbSigType::POWER:
    case dbSigType::GROUND: {
      if (isFiltered(filter, POWER_WIRE | POWER_VIA)) {
        return false;
      }

      if (!isFiltered(filter, POWER_WIRE)) {
        draw_segment = true;
      }

      if (!isFiltered(filter, POWER_VIA)) {
        draw_via = true;
      }

      return true;
    }

    case dbSigType::CLOCK: {
      if (isFiltered(filter, CLOCK_WIRE | CLOCK_VIA)) {
        return false;
      }

      if (!isFiltered(filter, CLOCK_WIRE)) {
        draw_segment = true;
      }

      if (!isFiltered(filter, CLOCK_VIA)) {
        draw_via = true;
      }

      return true;
    }

    case dbSigType::RESET: {
      if (isFiltered(filter, RESET_WIRE | RESET_VIA)) {
        return false;
      }

      if (!isFiltered(filter, RESET_WIRE)) {
        draw_segment = true;
      }

      if (!isFiltered(filter, RESET_VIA)) {
        draw_via = true;
      }

      return true;
    }
  }

  draw_via = true;
  draw_segment = true;
  return true;
}

bool dbHierInstShapeItr::iterate_swires(unsigned filter,
                                        dbNet* net,
                                        bool draw_vias,
                                        bool draw_segments)
{
  for (dbSWire* swire : net->getSWires()) {
    if (!iterate_swire(filter, swire, draw_vias, draw_segments)) {
      return false;
    }
  }

  return true;
}

bool dbHierInstShapeItr::iterate_swire(unsigned filter,
                                       dbSWire* swire,
                                       bool draw_vias,
                                       bool draw_segments)
{
  callback_->beginSWire(swire);

  if (isFiltered(filter, NET_SBOX)) {
    callback_->endSWire();
    return true;
  }

  dbShape shape;

  for (dbBox* box : swire->getWires()) {
    if (box->isVia()) {
      if (draw_vias == true) {
        getShape(box, shape);

        if (!callback_->nextBoxShape(box, shape)) {
          callback_->endSWire();
          return false;
        }
      }
    } else {
      if (draw_segments == true) {
        getShape(box, shape);

        if (!callback_->nextBoxShape(box, shape)) {
          callback_->endSWire();
          return false;
        }
      }
    }
  }

  callback_->endSWire();
  return true;
}

bool dbHierInstShapeItr::iterate_wire(unsigned filter,
                                      dbNet* net,
                                      bool draw_vias,
                                      bool draw_segments)
{
  dbWire* wire = net->getWire();

  if (wire == nullptr) {
    return true;
  }

  callback_->beginWire(wire);

  if (isFiltered(filter, NET_WIRE_SHAPE)) {
    callback_->endWire();
    return true;
  }

  dbShape shape;
  dbWireShapeItr itr;

  for (itr.begin(wire); itr.next(shape);) {
    if (shape.isVia()) {
      if (draw_vias == true) {
        transform(shape);

        if (!callback_->nextWireShape(wire, itr.getShapeId(), shape)) {
          callback_->endWire();
          return false;
        }
      }
    } else {
      if (draw_segments == true) {
        transform(shape);

        if (!callback_->nextWireShape(wire, itr.getShapeId(), shape)) {
          callback_->endWire();
          return false;
        }
      }
    }
  }

  callback_->endWire();
  return true;
}

void dbHierInstShapeItr::transform(dbShape& shape)
{
  dbTransform& t = transforms_.back();
  t.apply(shape.rect_);
}

void dbHierInstShapeItr::getShape(dbBox* box, dbShape& shape)
{
  Rect r = box->getBox();
  dbTransform& t = transforms_.back();
  t.apply(r);

  if (!box->isVia()) {
    shape.setSegment(box->getTechLayer(), r);
  } else {
    dbTechVia* tech_via = box->getTechVia();

    if (tech_via) {
      shape.setVia(tech_via, r);
    } else {
      dbVia* via = box->getBlockVia();
      assert(via);
      shape.setVia(via, r);
    }
  }
}

}  // namespace odb
