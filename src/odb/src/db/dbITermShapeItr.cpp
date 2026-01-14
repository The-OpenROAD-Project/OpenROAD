// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include <cassert>

#include "odb/db.h"
#include "odb/dbShape.h"
#include "odb/geom.h"

namespace odb {

dbITermShapeItr::dbITermShapeItr(bool expand_vias)
{
  mterm_ = nullptr;
  state_ = kInit;
  iterm_ = nullptr;
  mpin_ = nullptr;
  via_ = nullptr;
  expand_vias_ = expand_vias;
}

void dbITermShapeItr::begin(dbITerm* iterm)
{
  iterm_ = iterm;
  dbInst* inst = iterm->getInst();
  mterm_ = iterm->getMTerm();
  transform_ = inst->getTransform();
  state_ = kInit;
}

void dbITermShapeItr::getShape(dbBox* box, dbShape& shape)
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

bool dbITermShapeItr::next(dbShape& shape)
{
next_state:

  switch (state_) {
    case kInit: {
      mpins_ = mterm_->getMPins();
      mpin_itr_ = mpins_.begin();
      state_ = kMpinItr;
      goto next_state;
    }

    case kMpinItr: {
      if (mpin_itr_ == mpins_.end()) {
        return false;
      }
      mpin_ = *mpin_itr_;
      ++mpin_itr_;
      boxes_ = mpin_->getGeometry();
      box_itr_ = boxes_.begin();
      state_ = kMboxItr;

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
        state_ = kViaBoxItr;
      }

      goto next_state;
    }

    case kViaBoxItr: {
      if (via_box_itr_ == via_boxes_.end()) {
        state_ = kMboxItr;
      } else {
        dbBox* box = *via_box_itr_;
        ++via_box_itr_;
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
  b.moveDelta(via_pt_.getX(), via_pt_.getY());
  transform_.apply(b);
  shape.setViaBox(via_, box->getTechLayer(), b);
}

}  // namespace odb
