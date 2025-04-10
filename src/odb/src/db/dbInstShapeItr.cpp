// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "odb/ZException.h"
#include "odb/db.h"
#include "odb/dbShape.h"

namespace odb {

dbInstShapeItr::dbInstShapeItr(bool expand_vias)
{
  _state = 0;
  _inst = nullptr;
  _master = nullptr;
  _mpin = nullptr;
  _type = ALL;
  _via = nullptr;
  _via_x = 0;
  _via_y = 0;
  _expand_vias = expand_vias;
}

void dbInstShapeItr::begin(dbInst* inst, IteratorType type)
{
  _inst = inst;
  _master = _inst->getMaster();
  _transform = _inst->getTransform();
  _type = type;
  _state = 0;
}

void dbInstShapeItr::begin(dbInst* inst,
                           IteratorType type,
                           const dbTransform& t)
{
  _inst = inst;
  _master = _inst->getMaster();
  _transform = _inst->getTransform();
  _transform.concat(t);
  _type = type;
  _state = 0;
}

void dbInstShapeItr::getViaBox(dbBox* box, dbShape& shape)
{
  Rect b = box->getBox();
  int xmin = b.xMin() + _via_x;
  int ymin = b.yMin() + _via_y;
  int xmax = b.xMax() + _via_x;
  int ymax = b.yMax() + _via_y;
  Rect r(xmin, ymin, xmax, ymax);
  _transform.apply(r);
  shape.setViaBox(_via, box->getTechLayer(), r);
}

void dbInstShapeItr::getShape(dbBox* box, dbShape& shape)
{
  Rect r = box->getBox();
  _transform.apply(r);

  dbTechVia* via = box->getTechVia();

  if (via) {
    shape.setVia(via, r);
  } else {
    shape.setSegment(box->getTechLayer(), r);
  }
}

#define INIT 0
#define MTERM_ITR 1
#define MPIN_ITR 2
#define MBOX_ITR 3
#define OBS_ITR 4
#define VIA_BOX_ITR 5
#define PINS_DONE 6

bool dbInstShapeItr::next(dbShape& shape)
{
next_state:

  switch (_state) {
    case INIT: {
      if (_type == OBSTRUCTIONS) {
        _boxes = _master->getObstructions();
        _box_itr = _boxes.begin();
        _state = OBS_ITR;
      } else {
        _mterms = _master->getMTerms();
        _mterm_itr = _mterms.begin();
        _state = MTERM_ITR;
      }

      goto next_state;
    }

    case MTERM_ITR: {
      if (_mterm_itr == _mterms.end()) {
        _state = PINS_DONE;
      } else {
        dbMTerm* mterm = *_mterm_itr;
        ++_mterm_itr;
        _mpins = mterm->getMPins();
        _mpin_itr = _mpins.begin();
        _state = MPIN_ITR;
      }

      goto next_state;
    }

    case MPIN_ITR: {
      if (_mpin_itr == _mpins.end()) {
        _state = MTERM_ITR;
      } else {
        _mpin = *_mpin_itr;
        ++_mpin_itr;
        _boxes = _mpin->getGeometry();
        _box_itr = _boxes.begin();
        _state = MBOX_ITR;
      }

      goto next_state;
    }

    case MBOX_ITR: {
      if (_box_itr == _boxes.end()) {
        _state = MPIN_ITR;
      } else {
        dbBox* box = *_box_itr;
        ++_box_itr;

        if ((_expand_vias == false) || (box->isVia() == false)) {
          getShape(box, shape);
          return true;
        }

        box->getViaXY(_via_x, _via_y);
        _via = box->getTechVia();
        assert(_via);
        _via_boxes = _via->getBoxes();
        _via_box_itr = _via_boxes.begin();
        _prev_state = MBOX_ITR;
        _state = VIA_BOX_ITR;
      }

      goto next_state;
    }

    case OBS_ITR: {
      if (_box_itr == _boxes.end()) {
        return false;
      }
      dbBox* box = *_box_itr;
      ++_box_itr;

      if ((_expand_vias == false) || (box->isVia() == false)) {
        getShape(box, shape);
        return true;
      }

      box->getViaXY(_via_x, _via_y);
      _via = box->getTechVia();
      assert(_via);
      _via_boxes = _via->getBoxes();
      _via_box_itr = _via_boxes.begin();
      _prev_state = OBS_ITR;
      _state = VIA_BOX_ITR;
      goto next_state;
    }

    case VIA_BOX_ITR: {
      if (_via_box_itr == _via_boxes.end()) {
        _state = _prev_state;
      } else {
        dbBox* box = *_via_box_itr;
        ++_via_box_itr;
        getViaBox(box, shape);
        return true;
      }

      goto next_state;
    }

    case PINS_DONE: {
      if (_type == ALL) {
        _type = OBSTRUCTIONS;
        _state = INIT;
        goto next_state;
      }

      return false;
    }
  }

  return false;
}

}  // namespace odb
