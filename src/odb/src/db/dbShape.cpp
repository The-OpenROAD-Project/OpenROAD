// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "odb/dbShape.h"

#include <vector>

#include "dbTechLayer.h"
#include "dbTechVia.h"
#include "dbVia.h"
#include "odb/db.h"

namespace odb {

bool dbShape::operator<(const dbShape& rhs)
{
  if (_type < rhs._type) {
    return true;
  }

  if (_type > rhs._type) {
    return false;
  }

  switch (_type) {
    case VIA: {
      _dbVia* lhs_via = (_dbVia*) _via;
      _dbVia* rhs_via = (_dbVia*) rhs._via;

      int r = strcmp(lhs_via->_name, rhs_via->_name);

      if (r < 0) {
        return true;
      }

      if (r > 0) {
        return false;
      }

      break;
    }

    case TECH_VIA: {
      _dbTechVia* lhs_via = (_dbTechVia*) _via;
      _dbTechVia* rhs_via = (_dbTechVia*) rhs._via;

      int r = strcmp(lhs_via->_name, rhs_via->_name);

      if (r < 0) {
        return true;
      }

      if (r > 0) {
        return false;
      }
      break;
    }

    case SEGMENT: {
      _dbTechLayer* lhs_lay = (_dbTechLayer*) _layer;
      _dbTechLayer* rhs_lay = (_dbTechLayer*) rhs._layer;

      int r = strcmp(lhs_lay->_name, rhs_lay->_name);

      if (r < 0) {
        return true;
      }

      if (r > 0) {
        return false;
      }
      break;
    }

    case TECH_VIA_BOX: {
      _dbTechVia* lhs_via = (_dbTechVia*) _via;
      _dbTechVia* rhs_via = (_dbTechVia*) rhs._via;

      int r = strcmp(lhs_via->_name, rhs_via->_name);

      if (r < 0) {
        return true;
      }

      if (r > 0) {
        return false;
      }

      _dbTechLayer* lhs_lay = (_dbTechLayer*) _layer;
      _dbTechLayer* rhs_lay = (_dbTechLayer*) rhs._layer;

      r = strcmp(lhs_lay->_name, rhs_lay->_name);

      if (r < 0) {
        return true;
      }

      if (r > 0) {
        return false;
      }
      break;
    }

    case VIA_BOX: {
      _dbVia* lhs_via = (_dbVia*) _via;
      _dbVia* rhs_via = (_dbVia*) rhs._via;

      int r = strcmp(lhs_via->_name, rhs_via->_name);

      if (r < 0) {
        return true;
      }

      if (r > 0) {
        return false;
      }

      _dbTechLayer* lhs_lay = (_dbTechLayer*) _layer;
      _dbTechLayer* rhs_lay = (_dbTechLayer*) rhs._layer;

      r = strcmp(lhs_lay->_name, rhs_lay->_name);

      if (r < 0) {
        return true;
      }

      if (r > 0) {
        return false;
      }
      break;
    }
  }

  return _rect < rhs._rect;
}

void dbShape::getViaXY(int& x, int& y) const
{
  switch (_type) {
    case VIA: {
      dbVia* v = getVia();
      dbBox* b = v->getBBox();
      int xmin = b->xMin();
      int ymin = b->yMin();
      x = _rect.xMin() - xmin;
      y = _rect.yMin() - ymin;
      break;
    }

    case TECH_VIA: {
      dbTechVia* v = getTechVia();
      dbBox* b = v->getBBox();
      int xmin = b->xMin();
      int ymin = b->yMin();
      x = _rect.xMin() - xmin;
      y = _rect.yMin() - ymin;
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
