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

dbInstShapeItr::dbInstShapeItr(bool expand_vias)
{
  _state = 0;
  _inst = NULL;
  _master = NULL;
  _mpin = NULL;
  _type = ALL;
  _via = NULL;
  _via_x = 0;
  _via_y = 0;
  _expand_vias = expand_vias;
}

void dbInstShapeItr::begin(dbInst* inst, IteratorType type)
{
  int x, y;
  _inst = inst;
  _inst->getOrigin(x, y);
  _master = _inst->getMaster();
  _transform = dbTransform(inst->getOrient(), Point(x, y));
  _type = type;
  _state = 0;
}

void dbInstShapeItr::begin(dbInst* inst,
                           IteratorType type,
                           const dbTransform& t)
{
  int x, y;
  _inst = inst;
  _inst->getOrigin(x, y);
  _master = _inst->getMaster();
  _transform = dbTransform(inst->getOrient(), Point(x, y));
  _transform.concat(t);
  _type = type;
  _state = 0;
}

void dbInstShapeItr::getViaBox(dbBox* box, dbShape& shape)
{
  Rect b;
  box->getBox(b);
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
  Rect r;
  box->getBox(r);
  _transform.apply(r);

  dbTechVia* via = box->getTechVia();

  if (via)
    shape.setVia(via, r);
  else
    shape.setSegment(box->getTechLayer(), r);
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
  ZASSERT(_inst);

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
      if (_mterm_itr == _mterms.end())
        _state = PINS_DONE;
      else {
        dbMTerm* mterm = *_mterm_itr;
        ++_mterm_itr;
        _mpins = mterm->getMPins();
        _mpin_itr = _mpins.begin();
        _state = MPIN_ITR;
      }

      goto next_state;
    }

    case MPIN_ITR: {
      if (_mpin_itr == _mpins.end())
        _state = MTERM_ITR;
      else {
        _mpin = *_mpin_itr;
        ++_mpin_itr;
        _boxes = _mpin->getGeometry();
        _box_itr = _boxes.begin();
        _state = MBOX_ITR;
      }

      goto next_state;
    }

    case MBOX_ITR: {
      if (_box_itr == _boxes.end())
        _state = MPIN_ITR;
      else {
        dbBox* box = *_box_itr;
        ++_box_itr;

        if ((_expand_vias == false) || (box->isVia() == false)) {
          getShape(box, shape);
          return true;
        }

        else {
          box->getViaXY(_via_x, _via_y);
          _via = box->getTechVia();
          assert(_via);
          _via_boxes = _via->getBoxes();
          _via_box_itr = _via_boxes.begin();
          _prev_state = MBOX_ITR;
          _state = VIA_BOX_ITR;
        }
      }

      goto next_state;
    }

    case OBS_ITR: {
      if (_box_itr == _boxes.end())
        return false;

      else {
        dbBox* box = *_box_itr;
        ++_box_itr;

        if ((_expand_vias == false) || (box->isVia() == false)) {
          getShape(box, shape);
          return true;
        }

        else {
          box->getViaXY(_via_x, _via_y);
          _via = box->getTechVia();
          assert(_via);
          _via_boxes = _via->getBoxes();
          _via_box_itr = _via_boxes.begin();
          _prev_state = OBS_ITR;
          _state = VIA_BOX_ITR;
        }
      }
      goto next_state;
    }

    case VIA_BOX_ITR: {
      if (_via_box_itr == _via_boxes.end())
        _state = _prev_state;
      else {
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
