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

#include "dbShape.h"

#include "db.h"
#include "dbTechLayer.h"
#include "dbTechVia.h"
#include "dbVia.h"

namespace odb {

bool dbShape::operator<(const dbShape& rhs)
{
  if (_type < rhs._type)
    return true;

  if (_type > rhs._type)
    return false;

  switch (_type) {
    case VIA: {
      _dbVia* lhs_via = (_dbVia*) _via;
      _dbVia* rhs_via = (_dbVia*) rhs._via;

      int r = strcmp(lhs_via->_name, rhs_via->_name);

      if (r < 0)
        return true;

      if (r > 0)
        return false;

      break;
    }

    case TECH_VIA: {
      _dbTechVia* lhs_via = (_dbTechVia*) _via;
      _dbTechVia* rhs_via = (_dbTechVia*) rhs._via;

      int r = strcmp(lhs_via->_name, rhs_via->_name);

      if (r < 0)
        return true;

      if (r > 0)
        return false;
      break;
    }

    case SEGMENT: {
      _dbTechLayer* lhs_lay = (_dbTechLayer*) _layer;
      _dbTechLayer* rhs_lay = (_dbTechLayer*) rhs._layer;

      int r = strcmp(lhs_lay->_name, rhs_lay->_name);

      if (r < 0)
        return true;

      if (r > 0)
        return false;
      break;
    }

    case TECH_VIA_BOX: {
      _dbTechVia* lhs_via = (_dbTechVia*) _via;
      _dbTechVia* rhs_via = (_dbTechVia*) rhs._via;

      int r = strcmp(lhs_via->_name, rhs_via->_name);

      if (r < 0)
        return true;

      if (r > 0)
        return false;

      _dbTechLayer* lhs_lay = (_dbTechLayer*) _layer;
      _dbTechLayer* rhs_lay = (_dbTechLayer*) rhs._layer;

      r = strcmp(lhs_lay->_name, rhs_lay->_name);

      if (r < 0)
        return true;

      if (r > 0)
        return false;
      break;
    }

    case VIA_BOX: {
      _dbVia* lhs_via = (_dbVia*) _via;
      _dbVia* rhs_via = (_dbVia*) rhs._via;

      int r = strcmp(lhs_via->_name, rhs_via->_name);

      if (r < 0)
        return true;

      if (r > 0)
        return false;

      _dbTechLayer* lhs_lay = (_dbTechLayer*) _layer;
      _dbTechLayer* rhs_lay = (_dbTechLayer*) rhs._layer;

      r = strcmp(lhs_lay->_name, rhs_lay->_name);

      if (r < 0)
        return true;

      if (r > 0)
        return false;
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

void dbShape::getViaBoxes(const dbShape& via, std::vector<dbShape>& shapes)
{
  if (via.getTechVia()) {
    int x, y;
    via.getViaXY(x, y);
    shapes.clear();
    dbTechVia* v = via.getTechVia();
    dbSet<dbBox> boxes;
    boxes = v->getBoxes();
    dbSet<dbBox>::iterator itr;

    for (itr = boxes.begin(); itr != boxes.end(); ++itr) {
      dbBox* box = *itr;
      Rect b;
      box->getBox(b);
      int xmin = b.xMin() + x;
      int ymin = b.yMin() + y;
      int xmax = b.xMax() + x;
      int ymax = b.yMax() + y;
      Rect r(xmin, ymin, xmax, ymax);
      dbShape shape(v, box->getTechLayer(), r);
      shapes.push_back(shape);
    }
  }

  else if (via.getVia()) {
    int x, y;
    via.getViaXY(x, y);
    shapes.clear();
    dbVia* v = via.getVia();
    dbSet<dbBox> boxes;
    boxes = v->getBoxes();
    dbSet<dbBox>::iterator itr;

    for (itr = boxes.begin(); itr != boxes.end(); ++itr) {
      dbBox* box = *itr;
      Rect b;
      box->getBox(b);
      int xmin = b.xMin() + x;
      int ymin = b.yMin() + y;
      int xmax = b.xMax() + x;
      int ymax = b.yMax() + y;
      Rect r(xmin, ymin, xmax, ymax);
      dbShape shape(v, box->getTechLayer(), r);
      shapes.push_back(shape);
    }
  }
}

}  // namespace odb
