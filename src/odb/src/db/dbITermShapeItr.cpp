// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "odb/ZException.h"
#include "odb/db.h"
#include "odb/dbShape.h"

namespace odb {

dbITermShapeItr::dbITermShapeItr(bool expand_vias)
{
  _mterm = nullptr;
  _state = 0;
  _iterm = nullptr;
  _mpin = nullptr;
  _via = nullptr;
  _via_x = 0;
  _via_y = 0;
  _expand_vias = expand_vias;
}

void dbITermShapeItr::begin(dbITerm* iterm)
{
  _iterm = iterm;
  dbInst* inst = iterm->getInst();
  _mterm = iterm->getMTerm();
  _transform = inst->getTransform();
  _state = 0;
}

void dbITermShapeItr::getShape(dbBox* box, dbShape& shape)
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
#define MPIN_ITR 1
#define MBOX_ITR 2
#define VIA_BOX_ITR 3

bool dbITermShapeItr::next(dbShape& shape)
{
next_state:

  switch (_state) {
    case INIT: {
      _mpins = _mterm->getMPins();
      _mpin_itr = _mpins.begin();
      _state = MPIN_ITR;
      goto next_state;
    }

    case MPIN_ITR: {
      if (_mpin_itr == _mpins.end()) {
        return false;
      }
      _mpin = *_mpin_itr;
      ++_mpin_itr;
      _boxes = _mpin->getGeometry();
      _box_itr = _boxes.begin();
      _state = MBOX_ITR;

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
        _state = VIA_BOX_ITR;
      }

      goto next_state;
    }

    case VIA_BOX_ITR: {
      if (_via_box_itr == _via_boxes.end()) {
        _state = MBOX_ITR;
      } else {
        dbBox* box = *_via_box_itr;
        ++_via_box_itr;
        getViaBox(box, shape);
        return true;
      }

      goto next_state;
    }
  }

  return false;
}

void dbITermShapeItr::getViaBox(dbBox* box, dbShape& shape)
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

}  // namespace odb
