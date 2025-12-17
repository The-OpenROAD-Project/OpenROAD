// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include <cassert>

#include "odb/db.h"
#include "odb/dbShape.h"
#include "odb/geom.h"

namespace odb {

dbInstShapeItr::dbInstShapeItr(bool expand_vias)
{
  state_ = 0;
  inst_ = nullptr;
  master_ = nullptr;
  _mpin_ = nullptr;
  type_ = ALL;
  via_ = nullptr;
  expand_vias_ = expand_vias;
}

void dbInstShapeItr::begin(dbInst* inst, IteratorType type)
{
  inst_ = inst;
  master_ = inst_->getMaster();
  transform_ = inst_->getTransform();
  type_ = type;
  state_ = 0;
}

void dbInstShapeItr::begin(dbInst* inst,
                           IteratorType type,
                           const dbTransform& t)
{
  inst_ = inst;
  master_ = inst_->getMaster();
  transform_ = inst_->getTransform();
  transform_.concat(t);
  type_ = type;
  state_ = 0;
}

void dbInstShapeItr::getViaBox(dbBox* box, dbShape& shape)
{
  Rect b = box->getBox();
  b.moveDelta(via_pt_.getX(), via_pt_.getY());
  transform_.apply(b);
  shape.setViaBox(via_, box->getTechLayer(), b);
}

void dbInstShapeItr::getShape(dbBox* box, dbShape& shape)
{
  Rect r = box->getBox();
  transform_.apply(r);

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

  switch (state_) {
    case INIT: {
      if (type_ == OBSTRUCTIONS) {
        boxes_ = master_->getObstructions();
        box_itr_ = boxes_.begin();
        state_ = OBS_ITR;
      } else {
        mterms_ = master_->getMTerms();
        mterm_itr_ = mterms_.begin();
        state_ = MTERM_ITR;
      }

      goto next_state;
    }

    case MTERM_ITR: {
      if (mterm_itr_ == mterms_.end()) {
        state_ = PINS_DONE;
      } else {
        dbMTerm* mterm = *mterm_itr_;
        ++mterm_itr_;
        mpins_ = mterm->getMPins();
        mpin_itr_ = mpins_.begin();
        state_ = MPIN_ITR;
      }

      goto next_state;
    }

    case MPIN_ITR: {
      if (mpin_itr_ == mpins_.end()) {
        state_ = MTERM_ITR;
      } else {
        _mpin_ = *mpin_itr_;
        ++mpin_itr_;
        boxes_ = _mpin_->getGeometry();
        box_itr_ = boxes_.begin();
        state_ = MBOX_ITR;
      }

      goto next_state;
    }

    case MBOX_ITR: {
      if (box_itr_ == boxes_.end()) {
        state_ = MPIN_ITR;
      } else {
        dbBox* box = *box_itr_;
        ++box_itr_;

        if ((expand_vias_ == false) || (box->isVia() == false)) {
          getShape(box, shape);
          return true;
        }

        via_pt_ = box->getViaXY();
        via_ = box->getTechVia();
        assert(via_);
        via_boxes_ = via_->getBoxes();
        via_box_itr_ = via_boxes_.begin();
        prev_state_ = MBOX_ITR;
        state_ = VIA_BOX_ITR;
      }

      goto next_state;
    }

    case OBS_ITR: {
      if (box_itr_ == boxes_.end()) {
        return false;
      }
      dbBox* box = *box_itr_;
      ++box_itr_;

      if ((expand_vias_ == false) || (box->isVia() == false)) {
        getShape(box, shape);
        return true;
      }

      via_pt_ = box->getViaXY();
      via_ = box->getTechVia();
      assert(via_);
      via_boxes_ = via_->getBoxes();
      via_box_itr_ = via_boxes_.begin();
      prev_state_ = OBS_ITR;
      state_ = VIA_BOX_ITR;
      goto next_state;
    }

    case VIA_BOX_ITR: {
      if (via_box_itr_ == via_boxes_.end()) {
        state_ = prev_state_;
      } else {
        dbBox* box = *via_box_itr_;
        ++via_box_itr_;
        getViaBox(box, shape);
        return true;
      }

      goto next_state;
    }

    case PINS_DONE: {
      if (type_ == ALL) {
        type_ = OBSTRUCTIONS;
        state_ = INIT;
        goto next_state;
      }

      return false;
    }
  }

  return false;
}

}  // namespace odb
