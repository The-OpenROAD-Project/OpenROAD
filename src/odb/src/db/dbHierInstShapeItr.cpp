// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "odb/ZException.h"
#include "odb/db.h"
#include "odb/dbShape.h"

namespace odb {

inline bool isFiltered(unsigned filter, unsigned mask)
{
  return (filter & mask) == mask;
}

dbHierInstShapeItr::dbHierInstShapeItr(dbShapeItrCallback* callback)
{
  _callback = callback;
  assert(callback);
}

void dbHierInstShapeItr::iterate(dbInst* inst, unsigned filter)
{
  _transforms.clear();
  _transforms.push_back(dbTransform());
  iterate_inst(inst, filter, 0);
}

void dbHierInstShapeItr::push_transform(dbTransform t)
{
  dbTransform top = _transforms.back();
  top.concat(t);
  _transforms.push_back(top);
}

bool dbHierInstShapeItr::iterate_leaf(dbInst* inst, unsigned filter, int level)
{
  if (isFiltered(filter, INST_OBS | INST_VIA | INST_PIN)) {
    return true;
  }

  _callback->beginInst(inst, level);

  push_transform(inst->getTransform());
  dbMaster* master = inst->getMaster();

  if (!isFiltered(filter, INST_OBS | INST_VIA)) {
    _callback->beginObstructions(master);
    const bool filter_via = isFiltered(filter, INST_VIA);
    const bool filter_obs = isFiltered(filter, INST_OBS);

    dbShape s;

    for (dbBox* box : master->getObstructions()) {
      if (box->isVia()) {
        if (!filter_via) {
          getShape(box, s);

          if (!_callback->nextBoxShape(box, s)) {
            _transforms.pop_back();
            _callback->endObstructions();
            _callback->endInst();
            return false;
          }
        }

      } else {
        if (!filter_obs) {
          getShape(box, s);

          if (!_callback->nextBoxShape(box, s)) {
            _transforms.pop_back();
            _callback->endObstructions();
            _callback->endInst();
            return false;
          }
        }
      }
    }

    _callback->endObstructions();
  }

  if (!isFiltered(filter, INST_PIN)) {
    dbShape s;

    for (dbMTerm* mterm : master->getMTerms()) {
      _callback->beginMTerm(mterm);
      for (dbMPin* pin : mterm->getMPins()) {
        _callback->beginMPin(pin);
        for (dbBox* box : pin->getGeometry()) {
          getShape(box, s);

          if (!_callback->nextBoxShape(box, s)) {
            _transforms.pop_back();
            _callback->endMPin();
            _callback->endMTerm();
            _callback->endInst();
            return false;
          }
        }
        _callback->endMPin();
      }
      _callback->endMTerm();
    }
  }

  _transforms.pop_back();
  _callback->endInst();
  return true;
}

bool dbHierInstShapeItr::iterate_inst(dbInst* inst, unsigned filter, int level)
{
  if (!inst->isHierarchical()) {
    return iterate_leaf(inst, filter, level);
  }

  _callback->beginInst(inst, level);

  push_transform(inst->getTransform());
  dbBlock* child = inst->getChild();
  dbShape shape;

  if (!isFiltered(filter, BLOCK_PIN)) {
    for (dbBTerm* bterm : child->getBTerms()) {
      for (dbBPin* pin : bterm->getBPins()) {
        _callback->beginBPin(pin);

        for (dbBox* box : pin->getBoxes()) {
          getShape(box, shape);
          if (!_callback->nextBoxShape(box, shape)) {
            _transforms.pop_back();
            _callback->endBPin();
            _callback->endInst();
            return false;
          }
        }

        _callback->endBPin();
      }
    }
  }

  if (!isFiltered(filter, BLOCK_OBS)) {
    for (dbObstruction* obs : child->getObstructions()) {
      dbBox* box = obs->getBBox();
      getShape(box, shape);
      _callback->beginObstruction(obs);

      if (!_callback->nextBoxShape(box, shape)) {
        _transforms.pop_back();
        _callback->endObstruction();
        _callback->endInst();
        return false;
      }

      _callback->endObstruction();
    }
  }

  for (dbInst* inst : child->getInsts()) {
    if (!iterate_inst(inst, filter, ++level)) {
      _transforms.pop_back();
      return false;
    }
  }

  for (dbNet* net : child->getNets()) {
    _callback->beginNet(net);

    bool draw_segments;
    bool draw_vias;

    if (!drawNet(filter, net, draw_vias, draw_segments)) {
      continue;
    }

    if (!isFiltered(filter, NET_SWIRE)) {
      if (!iterate_swires(filter, net, draw_vias, draw_segments)) {
        _transforms.pop_back();
        _callback->endNet();
        _callback->endInst();
        return false;
      }
    }

    if (!isFiltered(filter, NET_WIRE)) {
      if (!iterate_wire(filter, net, draw_vias, draw_segments)) {
        _transforms.pop_back();
        _callback->endNet();
        _callback->endInst();
        return false;
      }
    }

    _callback->endNet();
  }

  _transforms.pop_back();
  _callback->endInst();
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
  _callback->beginSWire(swire);

  if (isFiltered(filter, NET_SBOX)) {
    _callback->endSWire();
    return true;
  }

  dbShape shape;

  for (dbBox* box : swire->getWires()) {
    if (box->isVia()) {
      if (draw_vias == true) {
        getShape(box, shape);

        if (!_callback->nextBoxShape(box, shape)) {
          _callback->endSWire();
          return false;
        }
      }
    } else {
      if (draw_segments == true) {
        getShape(box, shape);

        if (!_callback->nextBoxShape(box, shape)) {
          _callback->endSWire();
          return false;
        }
      }
    }
  }

  _callback->endSWire();
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

  _callback->beginWire(wire);

  if (isFiltered(filter, NET_WIRE_SHAPE)) {
    _callback->endWire();
    return true;
  }

  dbShape shape;
  dbWireShapeItr itr;

  for (itr.begin(wire); itr.next(shape);) {
    if (shape.isVia()) {
      if (draw_vias == true) {
        transform(shape);

        if (!_callback->nextWireShape(wire, itr.getShapeId(), shape)) {
          _callback->endWire();
          return false;
        }
      }
    } else {
      if (draw_segments == true) {
        transform(shape);

        if (!_callback->nextWireShape(wire, itr.getShapeId(), shape)) {
          _callback->endWire();
          return false;
        }
      }
    }
  }

  _callback->endWire();
  return true;
}

void dbHierInstShapeItr::transform(dbShape& shape)
{
  dbTransform& t = _transforms.back();
  t.apply(shape._rect);
}

void dbHierInstShapeItr::getShape(dbBox* box, dbShape& shape)
{
  Rect r = box->getBox();
  dbTransform& t = _transforms.back();
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
