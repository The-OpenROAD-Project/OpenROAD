// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "odb/dbShape.h"

#include <cstring>
#include <vector>

#include "dbTechLayer.h"
#include "dbTechVia.h"
#include "dbVia.h"
#include "odb/db.h"
#include "odb/geom.h"

namespace odb {

bool dbShape::operator<(const dbShape& rhs)
{
  if (type_ < rhs.type_) {
    return true;
  }

  if (type_ > rhs.type_) {
    return false;
  }

  switch (type_) {
    case VIA: {
      _dbVia* lhs_via = (_dbVia*) via_;
      _dbVia* rhs_via = (_dbVia*) rhs.via_;

      int r = strcmp(lhs_via->name_, rhs_via->name_);

      if (r < 0) {
        return true;
      }

      if (r > 0) {
        return false;
      }

      break;
    }

    case TECH_VIA: {
      _dbTechVia* lhs_via = (_dbTechVia*) via_;
      _dbTechVia* rhs_via = (_dbTechVia*) rhs.via_;

      int r = strcmp(lhs_via->name_, rhs_via->name_);

      if (r < 0) {
        return true;
      }

      if (r > 0) {
        return false;
      }
      break;
    }

    case SEGMENT: {
      _dbTechLayer* lhs_lay = (_dbTechLayer*) layer_;
      _dbTechLayer* rhs_lay = (_dbTechLayer*) rhs.layer_;

      int r = strcmp(lhs_lay->name_, rhs_lay->name_);

      if (r < 0) {
        return true;
      }

      if (r > 0) {
        return false;
      }
      break;
    }

    case TECH_VIA_BOX: {
      _dbTechVia* lhs_via = (_dbTechVia*) via_;
      _dbTechVia* rhs_via = (_dbTechVia*) rhs.via_;

      int r = strcmp(lhs_via->name_, rhs_via->name_);

      if (r < 0) {
        return true;
      }

      if (r > 0) {
        return false;
      }

      _dbTechLayer* lhs_lay = (_dbTechLayer*) layer_;
      _dbTechLayer* rhs_lay = (_dbTechLayer*) rhs.layer_;

      r = strcmp(lhs_lay->name_, rhs_lay->name_);

      if (r < 0) {
        return true;
      }

      if (r > 0) {
        return false;
      }
      break;
    }

    case VIA_BOX: {
      _dbVia* lhs_via = (_dbVia*) via_;
      _dbVia* rhs_via = (_dbVia*) rhs.via_;

      int r = strcmp(lhs_via->name_, rhs_via->name_);

      if (r < 0) {
        return true;
      }

      if (r > 0) {
        return false;
      }

      _dbTechLayer* lhs_lay = (_dbTechLayer*) layer_;
      _dbTechLayer* rhs_lay = (_dbTechLayer*) rhs.layer_;

      r = strcmp(lhs_lay->name_, rhs_lay->name_);

      if (r < 0) {
        return true;
      }

      if (r > 0) {
        return false;
      }
      break;
    }
  }

  return rect_ < rhs.rect_;
}

void dbShape::getViaXY(int& x, int& y) const
{
  switch (type_) {
    case VIA: {
      dbVia* v = getVia();
      dbBox* b = v->getBBox();
      int xmin = b->xMin();
      int ymin = b->yMin();
      x = rect_.xMin() - xmin;
      y = rect_.yMin() - ymin;
      break;
    }

    case TECH_VIA: {
      dbTechVia* v = getTechVia();
      dbBox* b = v->getBBox();
      int xmin = b->xMin();
      int ymin = b->yMin();
      x = rect_.xMin() - xmin;
      y = rect_.yMin() - ymin;
      break;
    }
    default:
      break;  // Wall
  }
}

Point dbShape::getViaXY() const
{
  int x;
  int y;
  getViaXY(x, y);
  return {x, y};
}

void dbShape::getViaBoxes(const dbShape& via, std::vector<dbShape>& shapes)
{
  if (via.getTechVia()) {
    int x, y;
    via.getViaXY(x, y);
    shapes.clear();
    dbTechVia* v = via.getTechVia();

    for (dbBox* box : v->getBoxes()) {
      Rect b = box->getBox();
      b.moveDelta(x, y);
      dbShape shape(v, box->getTechLayer(), b);
      shapes.push_back(shape);
    }
  } else if (via.getVia()) {
    int x, y;
    via.getViaXY(x, y);
    shapes.clear();
    dbVia* v = via.getVia();

    for (dbBox* box : v->getBoxes()) {
      Rect b = box->getBox();
      b.moveDelta(x, y);
      dbShape shape(v, box->getTechLayer(), b);
      shapes.push_back(shape);
    }
  }
}

}  // namespace odb
