///////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// Copyright (c) 2019, Nefelus Inc
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// * Redistributions of source code must retain the above copyright notice, this
//   list of conditions and the following disclaimer.
//
// * Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
//
// * Neither the name of the copyright holder nor the names of its
//   contributors may be used to endorse or promote products derived from
//   this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.

#include "ZException.h"
#include "db.h"
#include "dbShape.h"

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
  if (isFiltered(filter, INST_OBS | INST_VIA | INST_PIN))
    return true;

  _callback->beginInst(inst, level);

  int x, y;
  inst->getOrigin(x, y);
  push_transform(dbTransform(inst->getOrient(), Point(x, y)));
  dbMaster* master = inst->getMaster();

  if (!isFiltered(filter, INST_OBS | INST_VIA)) {
    _callback->beginObstructions(master);
    dbSet<dbBox> boxes = master->getObstructions();
    dbSet<dbBox>::iterator itr;
    bool filter_via = isFiltered(filter, INST_VIA);
    bool filter_obs = isFiltered(filter, INST_OBS);

    dbShape s;

    for (itr = boxes.begin(); itr != boxes.end(); ++itr) {
      dbBox* box = *itr;

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
    dbSet<dbMTerm> mterms = master->getMTerms();
    dbSet<dbMTerm>::iterator mitr;
    dbShape s;

    for (mitr = mterms.begin(); mitr != mterms.end(); ++mitr) {
      dbMTerm* mterm = *mitr;
      _callback->beginMTerm(mterm);

      dbSet<dbMPin> mpins = mterm->getMPins();
      dbSet<dbMPin>::iterator pitr;

      for (pitr = mpins.begin(); pitr != mpins.end(); ++pitr) {
        dbMPin* pin = *pitr;
        _callback->beginMPin(pin);

        dbSet<dbBox> geoms = pin->getGeometry();
        dbSet<dbBox>::iterator gitr;

        for (gitr = geoms.begin(); gitr != geoms.end(); ++gitr) {
          dbBox* box = *gitr;
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
  if (!inst->isHierarchical())
    return iterate_leaf(inst, filter, level);

  _callback->beginInst(inst, level);

  int x, y;
  inst->getOrigin(x, y);
  push_transform(dbTransform(inst->getOrient(), Point(x, y)));
  dbBlock* child = inst->getChild();
  dbShape shape;

  if (!isFiltered(filter, BLOCK_PIN)) {
    dbSet<dbBTerm> bterms = child->getBTerms();
    dbSet<dbBTerm>::iterator bitr;

    for (bitr = bterms.begin(); bitr != bterms.end(); ++bitr) {
      dbBTerm* bterm = *bitr;
      dbSet<dbBPin> bpins = bterm->getBPins();
      dbSet<dbBPin>::iterator pitr;

      for (pitr = bpins.begin(); pitr != bpins.end(); ++pitr) {
        dbBPin* pin = *pitr;
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
    dbSet<dbObstruction> obstructions = child->getObstructions();
    dbSet<dbObstruction>::iterator oitr;

    for (oitr = obstructions.begin(); oitr != obstructions.end(); ++oitr) {
      dbObstruction* obs = *oitr;
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

  dbSet<dbInst> insts = child->getInsts();
  dbSet<dbInst>::iterator iitr;

  for (iitr = insts.begin(); iitr != insts.end(); ++iitr) {
    dbInst* inst = *iitr;

    if (!iterate_inst(inst, filter, ++level)) {
      _transforms.pop_back();
      return false;
    }
  }

  dbSet<dbNet> nets = child->getNets();
  dbSet<dbNet>::iterator nitr;

  for (nitr = nets.begin(); nitr != nets.end(); ++nitr) {
    dbNet* net = *nitr;

    _callback->beginNet(net);

    bool draw_segments;
    bool draw_vias;

    if (!drawNet(filter, net, draw_vias, draw_segments))
      continue;

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

  switch (type) {
    case dbSigType::SCAN:
    case dbSigType::ANALOG:
    case dbSigType::TIEOFF:
    case dbSigType::SIGNAL: {
      if (isFiltered(filter, SIGNAL_WIRE | SIGNAL_VIA))
        return false;

      if (!isFiltered(filter, SIGNAL_WIRE))
        draw_segment = true;

      if (!isFiltered(filter, SIGNAL_VIA))
        draw_via = true;

      return true;
    }

    case dbSigType::POWER:
    case dbSigType::GROUND: {
      if (isFiltered(filter, POWER_WIRE | POWER_VIA))
        return false;

      if (!isFiltered(filter, POWER_WIRE))
        draw_segment = true;

      if (!isFiltered(filter, POWER_VIA))
        draw_via = true;

      return true;
    }

    case dbSigType::CLOCK: {
      if (isFiltered(filter, CLOCK_WIRE | CLOCK_VIA))
        return false;

      if (!isFiltered(filter, CLOCK_WIRE))
        draw_segment = true;

      if (!isFiltered(filter, CLOCK_VIA))
        draw_via = true;

      return true;
    }

    case dbSigType::RESET: {
      if (isFiltered(filter, RESET_WIRE | RESET_VIA))
        return false;

      if (!isFiltered(filter, RESET_WIRE))
        draw_segment = true;

      if (!isFiltered(filter, RESET_VIA))
        draw_via = true;

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
  dbSet<dbSWire> swires = net->getSWires();
  dbSet<dbSWire>::iterator itr;

  for (itr = swires.begin(); itr != swires.end(); ++itr) {
    dbSWire* swire = *itr;

    if (!iterate_swire(filter, swire, draw_vias, draw_segments))
      return false;
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

  dbSet<dbSBox> boxes = swire->getWires();
  dbSet<dbSBox>::iterator itr;
  dbShape shape;

  for (itr = boxes.begin(); itr != boxes.end(); ++itr) {
    dbBox* box = *itr;

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

  if (wire == NULL)
    return true;

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
  Rect r;
  box->getBox(r);
  dbTransform& t = _transforms.back();
  t.apply(r);

  if (!box->isVia()) {
    shape.setSegment(box->getTechLayer(), r);
  } else {
    dbTechVia* tech_via = box->getTechVia();

    if (tech_via)
      shape.setVia(tech_via, r);
    else {
      dbVia* via = box->getBlockVia();
      assert(via);
      shape.setVia(via, r);
    }
  }
}

}  // namespace odb
