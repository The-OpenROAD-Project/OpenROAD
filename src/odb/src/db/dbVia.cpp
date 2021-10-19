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

#include "dbVia.h"

#include "db.h"
#include "dbBlock.h"
#include "dbBox.h"
#include "dbBoxItr.h"
#include "dbChip.h"
#include "dbDatabase.h"
#include "dbDiff.hpp"
#include "dbSet.h"
#include "dbTable.h"
#include "dbTable.hpp"
#include "dbTech.h"
#include "dbTechLayer.h"
#include "dbTechVia.h"
#include "dbTechViaGenerateRule.h"

namespace odb {

template class dbTable<_dbVia>;
static void create_via_boxes(_dbVia* via, const dbViaParams& P);

////////////////////////////////////////////////////////////////////
//
// _dbVia - Methods
//
////////////////////////////////////////////////////////////////////

bool _dbVia::operator==(const _dbVia& rhs) const
{
  if (_flags._is_rotated != rhs._flags._is_rotated)
    return false;

  if (_flags._is_tech_via != rhs._flags._is_tech_via)
    return false;

  if (_flags._has_params != rhs._flags._has_params)
    return false;

  if (_flags._orient != rhs._flags._orient)
    return false;

  if (_name && rhs._name) {
    if (strcmp(_name, rhs._name) != 0)
      return false;
  } else if (_name || rhs._name)
    return false;

  if (_pattern && rhs._pattern) {
    if (strcmp(_pattern, rhs._pattern) != 0)
      return false;
  } else if (_pattern || rhs._pattern)
    return false;

  if (_bbox != rhs._bbox)
    return false;

  if (_boxes != rhs._boxes)
    return false;

  if (_top != rhs._top)
    return false;

  if (_bottom != rhs._bottom)
    return false;

  if (_generate_rule != rhs._generate_rule)
    return false;

  if (_rotated_via_id != rhs._rotated_via_id)
    return false;

  if (_via_params != rhs._via_params)
    return false;

  return true;
}

void _dbVia::differences(dbDiff& diff,
                         const char* field,
                         const _dbVia& rhs) const
{
  _dbBlock* lhs_block = (_dbBlock*) getOwner();
  _dbBlock* rhs_block = (_dbBlock*) rhs.getOwner();
  DIFF_BEGIN
  DIFF_FIELD(_name);
  DIFF_FIELD(_flags._is_rotated);
  DIFF_FIELD(_flags._is_tech_via);
  DIFF_FIELD(_flags._has_params);
  DIFF_FIELD(_flags._orient);
  DIFF_FIELD(_pattern);
  DIFF_OBJECT(_bbox, lhs_block->_box_tbl, rhs_block->_box_tbl);
  DIFF_SET(_boxes, lhs_block->_box_itr, rhs_block->_box_itr);
  DIFF_FIELD(_top);
  DIFF_FIELD(_bottom);
  DIFF_FIELD(_generate_rule);
  DIFF_FIELD(_rotated_via_id);
  DIFF_STRUCT(_via_params);
  DIFF_END
}

void _dbVia::out(dbDiff& diff, char side, const char* field) const
{
  _dbBlock* block = (_dbBlock*) getOwner();
  DIFF_OUT_BEGIN
  DIFF_OUT_FIELD(_name);
  DIFF_OUT_FIELD(_flags._is_rotated);
  DIFF_OUT_FIELD(_flags._is_tech_via);
  DIFF_OUT_FIELD(_flags._has_params);
  DIFF_OUT_FIELD(_flags._orient);
  DIFF_OUT_FIELD(_pattern);
  DIFF_OUT_OBJECT(_bbox, block->_box_tbl);
  DIFF_OUT_SET(_boxes, block->_box_itr);
  DIFF_OUT_FIELD(_top);
  DIFF_OUT_FIELD(_bottom);
  DIFF_OUT_FIELD(_generate_rule);
  DIFF_OUT_FIELD(_rotated_via_id);
  DIFF_OUT_STRUCT(_via_params);
  DIFF_END
}

_dbVia::_dbVia(_dbDatabase*, const _dbVia& v)
    : _flags(v._flags),
      _name(NULL),
      _pattern(NULL),
      _bbox(v._bbox),
      _boxes(v._boxes),
      _top(v._top),
      _bottom(v._bottom),
      _generate_rule(v._generate_rule),
      _rotated_via_id(v._rotated_via_id),
      _via_params(v._via_params)
{
  if (v._name) {
    _name = strdup(v._name);
    ZALLOCATED(_name);
  }

  if (v._pattern) {
    _pattern = strdup(v._pattern);
    ZALLOCATED(_pattern);
  }
}

_dbVia::_dbVia(_dbDatabase*)
{
  _flags._is_rotated = 0;
  _flags._is_tech_via = 0;
  _flags._has_params = 0;
  _flags._orient = dbOrientType::R0;
  _flags._spare_bits = 0;
  _name = 0;
  _pattern = 0;
}

_dbVia::~_dbVia()
{
  if (_name)
    free((void*) _name);

  if (_pattern)
    free((void*) _pattern);
}

dbOStream& operator<<(dbOStream& stream, const _dbVia& v)
{
  uint* bit_field = (uint*) &v._flags;
  stream << *bit_field;
  stream << v._name;
  stream << v._pattern;
  stream << v._bbox;
  stream << v._boxes;
  stream << v._top;
  stream << v._bottom;
  stream << v._generate_rule;
  stream << v._rotated_via_id;
  stream << v._via_params;
  return stream;
}

dbIStream& operator>>(dbIStream& stream, _dbVia& v)
{
  uint* bit_field = (uint*) &v._flags;
  stream >> *bit_field;
  stream >> v._name;
  stream >> v._pattern;
  stream >> v._bbox;
  stream >> v._boxes;
  stream >> v._top;
  stream >> v._bottom;
  stream >> v._generate_rule;
  stream >> v._rotated_via_id;
  stream >> v._via_params;

  return stream;
}

////////////////////////////////////////////////////////////////////
//
// dbVia - Methods
//
////////////////////////////////////////////////////////////////////

std::string dbVia::getName()
{
  _dbVia* via = (_dbVia*) this;
  return via->_name;
}

const char* dbVia::getConstName()
{
  _dbVia* via = (_dbVia*) this;
  return via->_name;
}

std::string dbVia::getPattern()
{
  _dbVia* via = (_dbVia*) this;

  if (via->_pattern == 0) {
    return "";
  }

  return via->_pattern;
}

void dbVia::setPattern(const char* name)
{
  _dbVia* via = (_dbVia*) this;

  if (via->_pattern != 0) {
    return;
  }

  via->_pattern = strdup(name);
  ZALLOCATED(via->_pattern);
}

dbBlock* dbVia::getBlock()
{
  return (dbBlock*) getImpl()->getOwner();
}

dbBox* dbVia::getBBox()
{
  _dbVia* via = (_dbVia*) this;

  if (via->_bbox == 0)
    return NULL;

  _dbBlock* block = (_dbBlock*) via->getOwner();
  return (dbBox*) block->_box_tbl->getPtr(via->_bbox);
}

bool dbVia::isViaRotated()
{
  _dbVia* via = (_dbVia*) this;
  return via->_flags._is_rotated == 1;
}

dbOrientType dbVia::getOrient()
{
  _dbVia* via = (_dbVia*) this;
  dbOrientType o(via->_flags._orient);
  return o;
}

dbTechVia* dbVia::getTechVia()
{
  _dbVia* via = (_dbVia*) this;

  if ((via->_flags._is_rotated == 0) || (via->_flags._is_tech_via == 0))
    return NULL;

  _dbDatabase* db = via->getDatabase();
  _dbTech* tech = db->_tech_tbl->getPtr(db->_tech);
  _dbTechVia* v = tech->_via_tbl->getPtr(via->_rotated_via_id);
  return (dbTechVia*) v;
}

dbVia* dbVia::getBlockVia()
{
  _dbVia* via = (_dbVia*) this;

  if ((via->_flags._is_rotated == 0) || (via->_flags._is_tech_via == 1))
    return NULL;

  _dbBlock* block = (_dbBlock*) via->getOwner();
  _dbVia* v = block->_via_tbl->getPtr(via->_rotated_via_id);
  return (dbVia*) v;
}

dbSet<dbBox> dbVia::getBoxes()
{
  _dbVia* via = (_dbVia*) this;
  _dbBlock* block = (_dbBlock*) via->getOwner();
  return dbSet<dbBox>(via, block->_box_itr);
}

dbTechLayer* dbVia::getTopLayer()
{
  _dbVia* via = (_dbVia*) this;

  if (via->_top == 0)
    return NULL;

  _dbDatabase* db = via->getDatabase();
  _dbTech* tech = db->_tech_tbl->getPtr(db->_tech);
  return (dbTechLayer*) tech->_layer_tbl->getPtr(via->_top);
}

dbTechLayer* dbVia::getBottomLayer()
{
  _dbVia* via = (_dbVia*) this;

  if (via->_bottom == 0)
    return NULL;

  _dbDatabase* db = via->getDatabase();
  _dbTech* tech = db->_tech_tbl->getPtr(db->_tech);
  return (dbTechLayer*) tech->_layer_tbl->getPtr(via->_bottom);
}

bool dbVia::hasParams()
{
  _dbVia* via = (_dbVia*) this;
  return via->_flags._has_params == 1;
}

void dbVia::setViaGenerateRule(dbTechViaGenerateRule* rule)
{
  _dbVia* via = (_dbVia*) this;
  via->_generate_rule = rule->getImpl()->getOID();
}

dbTechViaGenerateRule* dbVia::getViaGenerateRule()
{
  _dbVia* via = (_dbVia*) this;

  if (via->_generate_rule == 0)
    return NULL;

  _dbDatabase* db = via->getDatabase();
  _dbTech* tech = db->_tech_tbl->getPtr(db->_tech);
  _dbTechViaGenerateRule* rule
      = tech->_via_generate_rule_tbl->getPtr(via->_generate_rule);
  return (dbTechViaGenerateRule*) rule;
}

void dbVia::setViaParams(const dbViaParams& params)
{
  _dbVia* via = (_dbVia*) this;
  _dbBlock* block = (_dbBlock*) via->getOwner();
  via->_flags._has_params = 1;

  // Clear previous boxes
  dbSet<dbBox> boxes = getBoxes();
  dbSet<dbBox>::iterator itr;

  for (itr = boxes.begin(); itr != boxes.end();) {
    dbSet<dbBox>::iterator n = ++itr;
    _dbBox* box = (_dbBox*) *itr;
    dbProperty::destroyProperties(box);
    block->_box_tbl->destroy(box);
    itr = n;
  }

  via->_boxes = 0U;
  via->_via_params = params;
  via->_top = params._top_layer;
  via->_bottom = params._bot_layer;
  create_via_boxes(via, params);
}

void dbVia::getViaParams(dbViaParams& params)
{
  _dbVia* via = (_dbVia*) this;

  if (via->_flags._has_params == 0)
    params = dbViaParams();
  else {
    params = via->_via_params;
    _dbDatabase* db = via->getDatabase();
    _dbTech* tech = db->_tech_tbl->getPtr(db->_tech);
    params._tech = (dbTech*) tech;
  }
}

dbVia* dbVia::create(dbBlock* block_, const char* name_)
{
  if (block_->findVia(name_))
    return NULL;

  _dbBlock* block = (_dbBlock*) block_;
  _dbVia* via = block->_via_tbl->create();
  via->_name = strdup(name_);
  ZALLOCATED(via->_name);
  return (dbVia*) via;
}

dbVia* dbVia::create(dbBlock* block,
                     const char* name,
                     dbVia* blk_via,
                     dbOrientType orient)
{
  _dbVia* via = (_dbVia*) dbVia::create(block, name);

  if (via == NULL)
    return NULL;

  via->_flags._is_rotated = 1;
  via->_flags._orient = orient;
  via->_rotated_via_id = blk_via->getId();

  dbTransform t(orient);
  dbSet<dbBox> boxes = blk_via->getBoxes();
  dbSet<dbBox>::iterator itr;

  for (itr = boxes.begin(); itr != boxes.end(); ++itr) {
    _dbBox* box = (_dbBox*) *itr;
    dbTechLayer* l = (dbTechLayer*) box->getTechLayer();
    Rect r = box->_shape._rect;
    t.apply(r);
    dbBox::create((dbVia*) via, l, r.xMin(), r.yMin(), r.xMax(), r.yMax());
  }

  return (dbVia*) via;
}

dbVia* dbVia::create(dbBlock* block,
                     const char* name,
                     dbTechVia* tech_via,
                     dbOrientType orient)
{
  _dbVia* via = (_dbVia*) dbVia::create(block, name);

  if (via == NULL)
    return NULL;

  via->_flags._is_rotated = 1;
  via->_flags._is_tech_via = 1;
  via->_flags._orient = orient;
  via->_rotated_via_id = tech_via->getId();

  dbTransform t(orient);
  dbSet<dbBox> boxes = tech_via->getBoxes();
  dbSet<dbBox>::iterator itr;

  for (itr = boxes.begin(); itr != boxes.end(); ++itr) {
    _dbBox* box = (_dbBox*) *itr;
    dbTechLayer* l = (dbTechLayer*) box->getTechLayer();
    Rect r = box->_shape._rect;
    t.apply(r);
    dbBox::create((dbVia*) via, l, r.xMin(), r.yMin(), r.xMax(), r.yMax());
  }

  return (dbVia*) via;
}

static dbVia* copyVia(dbBlock* block, dbVia* via, bool copyRotatedVia);

dbVia* copyVia(dbBlock* block_, dbVia* via_, bool copyRotatedVia)
{
  _dbBlock* block = (_dbBlock*) block_;
  _dbVia* via = (_dbVia*) via_;

  _dbVia* cvia = block->_via_tbl->create();

  cvia->_flags = via->_flags;
  cvia->_name = strdup(via->_name);
  ZALLOCATED(cvia->_name);

  if (via->_pattern) {
    cvia->_pattern = strdup(via->_pattern);
    ZALLOCATED(cvia->_pattern);
  }

  cvia->_top = via->_top;
  cvia->_bottom = via->_bottom;

  dbSet<dbBox> boxes = via_->getBoxes();
  dbSet<dbBox>::iterator itr;

  for (itr = boxes.begin(); itr != boxes.end(); ++itr) {
    dbBox* b = *itr;
    dbTechLayer* l = b->getTechLayer();
    dbBox::create((dbVia*) cvia, l, b->xMin(), b->yMin(), b->xMax(), b->yMax());
  }

  if (via->_flags._is_rotated) {
    if (via->_flags._is_tech_via)
      cvia->_rotated_via_id = via->_rotated_via_id;
    else {
      _dbVia* bv = (_dbVia*) via_->getBlockVia();
      _dbVia* cbv = (_dbVia*) block_->findVia(bv->_name);

      if (copyRotatedVia && (cbv == NULL))
        cbv = (_dbVia*) copyVia(block_, (dbVia*) bv, true);

      assert(cbv);
      cvia->_rotated_via_id = cbv->getOID();
    }
  }

  return (dbVia*) cvia;
}

dbVia* dbVia::copy(dbBlock* dst, dbVia* src)
{
  return copyVia(dst, src, true);
}

bool dbVia::copy(dbBlock* dst_, dbBlock* src_)
{
  dbSet<dbVia> vias = src_->getVias();
  dbSet<dbVia>::iterator itr;

  // copy non rotated via's first
  for (itr = vias.begin(); itr != vias.end(); ++itr) {
    dbVia* v = *itr;
    _dbVia* via = (_dbVia*) v;

    if (!v->isViaRotated())
      if (!dst_->findVia(via->_name))
        copyVia(dst_, v, false);
  }

  // copy rotated via's last
  for (itr = vias.begin(); itr != vias.end(); ++itr) {
    dbVia* v = *itr;
    _dbVia* via = (_dbVia*) v;

    if (v->isViaRotated())
      if (!dst_->findVia(via->_name))
        copyVia(dst_, v, false);
  }

  return true;
}

dbVia* dbVia::getVia(dbBlock* block_, uint dbid_)
{
  _dbBlock* block = (_dbBlock*) block_;
  return (dbVia*) block->_via_tbl->getPtr(dbid_);
}

void create_via_boxes(_dbVia* via, const dbViaParams& P)
{
  int rows = P.getNumCutRows();
  int cols = P.getNumCutCols();
  int row;
  int y = 0;
  int maxX = 0;
  int maxY = 0;
  std::vector<Rect> cutRects;

  for (row = 0; row < rows; ++row) {
    int col;
    int x = 0;

    for (col = 0; col < cols; ++col) {
      maxX = x + P.getXCutSize();
      maxY = y + P.getYCutSize();
      Rect r(x, y, maxX, maxY);
      cutRects.push_back(r);
      x = maxX;
      x += P.getXCutSpacing();
    }

    y = maxY;
    y += P.getYCutSpacing();
  }

  dbTechLayer* cut_layer = P.getCutLayer();

  int dx = maxX / 2;
  int dy = maxY / 2;
  std::vector<Rect>::iterator itr;

  for (itr = cutRects.begin(); itr != cutRects.end(); ++itr) {
    Rect& r = *itr;
    r.moveDelta(-dx, -dy);
    r.moveDelta(P.getXOrigin(), P.getYOrigin());
    dbBox::create(
        (dbVia*) via, cut_layer, r.xMin(), r.yMin(), r.xMax(), r.yMax());
  }

  int minX = -dx;
  int minY = -dy;
  maxX -= dx;
  maxY -= dy;

  int top_minX
      = minX - P.getXTopEnclosure() + P.getXOrigin() + P.getXTopOffset();
  int top_minY
      = minY - P.getYTopEnclosure() + P.getYOrigin() + P.getYTopOffset();
  int top_maxX
      = maxX + P.getXTopEnclosure() + P.getXOrigin() + P.getXTopOffset();
  int top_maxY
      = maxY + P.getYTopEnclosure() + P.getYOrigin() + P.getYTopOffset();
  dbBox::create(
      (dbVia*) via, P.getTopLayer(), top_minX, top_minY, top_maxX, top_maxY);

  int bot_minX
      = minX - P.getXBottomEnclosure() + P.getXOrigin() + P.getXBottomOffset();
  int bot_minY
      = minY - P.getYBottomEnclosure() + P.getYOrigin() + P.getYBottomOffset();
  int bot_maxX
      = maxX + P.getXBottomEnclosure() + P.getXOrigin() + P.getXBottomOffset();
  int bot_maxY
      = maxY + P.getYBottomEnclosure() + P.getYOrigin() + P.getYBottomOffset();
  dbBox::create(
      (dbVia*) via, P.getBottomLayer(), bot_minX, bot_minY, bot_maxX, bot_maxY);
}

}  // namespace odb
