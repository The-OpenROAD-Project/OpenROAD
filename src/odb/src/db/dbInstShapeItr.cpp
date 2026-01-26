// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include <cassert>

#include "odb/db.h"
#include "odb/dbShape.h"
#include "odb/geom.h"

namespace odb {

dbInstShapeItr::dbInstShapeItr(bool expand_vias)
{
  state_ = kInit;
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
  state_ = kInit;
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
  state_ = kInit;
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

bool dbInstShapeItr::next(dbShape& shape)
{
next_state:

  switch (state_) {
    case kInit: {
      if (type_ == OBSTRUCTIONS) {
        boxes_ = master_->getObstructions();
        box_itr_ = boxes_.begin();
        state_ = kObsItr;
      } else {
        mterms_ = master_->getMTerms();
        mterm_itr_ = mterms_.begin();
        state_ = kMtermItr;
      }

      goto next_state;
    }

    case kMtermItr: {
      if (mterm_itr_ == mterms_.end()) {
        state_ = kPinsDone;
      } else {
        dbMTerm* mterm = *mterm_itr_;
        ++mterm_itr_;
        mpins_ = mterm->getMPins();
        mpin_itr_ = mpins_.begin();
        state_ = kMpinItr;
      }

      goto next_state;
    }

    case kMpinItr: {
      if (mpin_itr_ == mpins_.end()) {
        state_ = kMtermItr;
      } else {
        _mpin_ = *mpin_itr_;
        ++mpin_itr_;
        boxes_ = _mpin_->getGeometry();
        box_itr_ = boxes_.begin();
        state_ = kMboxItr;
      }

      goto next_state;
    }

    case kMboxItr: {
      if (box_itr_ == boxes_.end()) {
        state_ = kMpinItr;
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
        prev_state_ = kMboxItr;
        state_ = kViaBoxItr;
      }

      goto next_state;
    }

    case kObsItr: {
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
      prev_state_ = kObsItr;
      state_ = kViaBoxItr;
      goto next_state;
    }

    case kViaBoxItr: {
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

    case kPinsDone: {
      if (type_ == ALL) {
        type_ = OBSTRUCTIONS;
        state_ = kInit;
        goto next_state;
      }

      return false;
    }
  }

  return false;
}

}  // namespace odb
