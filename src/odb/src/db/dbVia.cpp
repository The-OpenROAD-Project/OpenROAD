// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "dbVia.h"

#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

#include "dbBlock.h"
#include "dbBox.h"
#include "dbBoxItr.h"
#include "dbChip.h"
#include "dbCommon.h"
#include "dbCore.h"
#include "dbDatabase.h"
#include "dbTable.h"
#include "dbTech.h"
#include "dbTechLayer.h"
#include "dbTechVia.h"
#include "dbTechViaGenerateRule.h"
#include "odb/db.h"
#include "odb/dbSet.h"
#include "odb/dbTypes.h"
#include "odb/geom.h"

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
  if (flags_.is_rotated != rhs.flags_.is_rotated) {
    return false;
  }

  if (flags_.is_tech_via != rhs.flags_.is_tech_via) {
    return false;
  }

  if (flags_.has_params != rhs.flags_.has_params) {
    return false;
  }

  if (flags_.orient != rhs.flags_.orient) {
    return false;
  }

  if (flags_.default_via != rhs.flags_.default_via) {
    return false;
  }

  if (name_ && rhs.name_) {
    if (strcmp(name_, rhs.name_) != 0) {
      return false;
    }
  } else if (name_ || rhs.name_) {
    return false;
  }

  if (pattern_ && rhs.pattern_) {
    if (strcmp(pattern_, rhs.pattern_) != 0) {
      return false;
    }
  } else if (pattern_ || rhs.pattern_) {
    return false;
  }

  if (bbox_ != rhs.bbox_) {
    return false;
  }

  if (boxes_ != rhs.boxes_) {
    return false;
  }

  if (top_ != rhs.top_) {
    return false;
  }

  if (bottom_ != rhs.bottom_) {
    return false;
  }

  if (generate_rule_ != rhs.generate_rule_) {
    return false;
  }

  if (rotated_via_id_ != rhs.rotated_via_id_) {
    return false;
  }

  if (via_params_ != rhs.via_params_) {
    return false;
  }

  return true;
}

_dbVia::_dbVia(_dbDatabase*, const _dbVia& v)
    : flags_(v.flags_),
      name_(nullptr),
      pattern_(nullptr),
      bbox_(v.bbox_),
      boxes_(v.boxes_),
      top_(v.top_),
      bottom_(v.bottom_),
      generate_rule_(v.generate_rule_),
      rotated_via_id_(v.rotated_via_id_),
      via_params_(v.via_params_)
{
  if (v.name_) {
    name_ = safe_strdup(v.name_);
  }

  if (v.pattern_) {
    pattern_ = safe_strdup(v.pattern_);
  }
}

_dbVia::_dbVia(_dbDatabase*)
{
  flags_.is_rotated = 0;
  flags_.is_tech_via = 0;
  flags_.has_params = 0;
  flags_.orient = dbOrientType::R0;
  flags_.default_via = false;
  flags_.spare_bits = 0;
  name_ = nullptr;
  pattern_ = nullptr;
}

_dbVia::~_dbVia()
{
  if (name_) {
    free((void*) name_);
  }

  if (pattern_) {
    free((void*) pattern_);
  }
}

_dbTech* _dbVia::getTech()
{
  _dbBlock* block = (_dbBlock*) getOwner();
  return block->getTech();
}

dbOStream& operator<<(dbOStream& stream, const _dbVia& v)
{
  uint32_t* bit_field = (uint32_t*) &v.flags_;
  stream << *bit_field;
  stream << v.name_;
  stream << v.pattern_;
  stream << v.bbox_;
  stream << v.boxes_;
  stream << v.top_;
  stream << v.bottom_;
  stream << v.generate_rule_;
  stream << v.rotated_via_id_;
  stream << v.via_params_;
  return stream;
}

dbIStream& operator>>(dbIStream& stream, _dbVia& v)
{
  uint32_t* bit_field = (uint32_t*) &v.flags_;
  stream >> *bit_field;
  stream >> v.name_;
  stream >> v.pattern_;
  stream >> v.bbox_;
  stream >> v.boxes_;
  stream >> v.top_;
  stream >> v.bottom_;
  stream >> v.generate_rule_;
  stream >> v.rotated_via_id_;
  stream >> v.via_params_;

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
  return via->name_;
}

const char* dbVia::getConstName()
{
  _dbVia* via = (_dbVia*) this;
  return via->name_;
}

std::string dbVia::getPattern()
{
  _dbVia* via = (_dbVia*) this;

  if (via->pattern_ == nullptr) {
    return "";
  }

  return via->pattern_;
}

void dbVia::setPattern(const char* name)
{
  _dbVia* via = (_dbVia*) this;

  if (via->pattern_ != nullptr) {
    return;
  }

  via->pattern_ = safe_strdup(name);
}

dbBlock* dbVia::getBlock()
{
  return (dbBlock*) getImpl()->getOwner();
}

dbBox* dbVia::getBBox()
{
  _dbVia* via = (_dbVia*) this;

  if (via->bbox_ == 0) {
    return nullptr;
  }

  _dbBlock* block = (_dbBlock*) via->getOwner();
  return (dbBox*) block->box_tbl_->getPtr(via->bbox_);
}

bool dbVia::isViaRotated()
{
  _dbVia* via = (_dbVia*) this;
  return via->flags_.is_rotated == 1;
}

dbOrientType dbVia::getOrient()
{
  _dbVia* via = (_dbVia*) this;
  dbOrientType o(via->flags_.orient);
  return o;
}

dbTechVia* dbVia::getTechVia()
{
  _dbVia* via = (_dbVia*) this;

  if ((via->flags_.is_rotated == 0) || (via->flags_.is_tech_via == 0)) {
    return nullptr;
  }

  _dbTech* tech = via->getTech();
  _dbTechVia* v = tech->via_tbl_->getPtr(via->rotated_via_id_);
  return (dbTechVia*) v;
}

dbVia* dbVia::getBlockVia()
{
  _dbVia* via = (_dbVia*) this;

  if ((via->flags_.is_rotated == 0) || (via->flags_.is_tech_via == 1)) {
    return nullptr;
  }

  _dbBlock* block = (_dbBlock*) via->getOwner();
  _dbVia* v = block->via_tbl_->getPtr(via->rotated_via_id_);
  return (dbVia*) v;
}

dbSet<dbBox> dbVia::getBoxes()
{
  _dbVia* via = (_dbVia*) this;
  _dbBlock* block = (_dbBlock*) via->getOwner();
  return dbSet<dbBox>(via, block->box_itr_);
}

dbTechLayer* dbVia::getTopLayer()
{
  _dbVia* via = (_dbVia*) this;

  if (via->top_ == 0) {
    return nullptr;
  }

  _dbTech* tech = via->getTech();
  return (dbTechLayer*) tech->layer_tbl_->getPtr(via->top_);
}

dbTechLayer* dbVia::getBottomLayer()
{
  _dbVia* via = (_dbVia*) this;

  if (via->bottom_ == 0) {
    return nullptr;
  }

  _dbTech* tech = via->getTech();
  return (dbTechLayer*) tech->layer_tbl_->getPtr(via->bottom_);
}

bool dbVia::hasParams()
{
  _dbVia* via = (_dbVia*) this;
  return via->flags_.has_params == 1;
}

void dbVia::setViaGenerateRule(dbTechViaGenerateRule* rule)
{
  _dbVia* via = (_dbVia*) this;
  via->generate_rule_ = rule->getImpl()->getOID();
}

dbTechViaGenerateRule* dbVia::getViaGenerateRule()
{
  _dbVia* via = (_dbVia*) this;

  if (via->generate_rule_ == 0) {
    return nullptr;
  }

  _dbTech* tech = via->getTech();
  auto rule = tech->via_generate_rule_tbl_->getPtr(via->generate_rule_);
  return (dbTechViaGenerateRule*) rule;
}

void dbVia::setViaParams(const dbViaParams& params)
{
  _dbVia* via = (_dbVia*) this;
  _dbBlock* block = (_dbBlock*) via->getOwner();
  via->flags_.has_params = 1;

  // Clear previous boxes
  dbSet<dbBox> boxes = getBoxes();
  dbSet<dbBox>::iterator itr;

  for (itr = boxes.begin(); itr != boxes.end();) {
    dbSet<dbBox>::iterator cur = itr++;
    _dbBox* box = (_dbBox*) *cur;
    dbProperty::destroyProperties(box);
    block->box_tbl_->destroy(box);
  }

  via->boxes_ = 0U;
  via->via_params_ = params;
  via->top_ = params.top_layer_;
  via->bottom_ = params.bot_layer_;
  create_via_boxes(via, params);
}

dbViaParams dbVia::getViaParams()
{
  dbViaParams params;

  _dbVia* via = (_dbVia*) this;

  if (via->flags_.has_params == 0) {
    params = dbViaParams();
  } else {
    params = via->via_params_;
    params._tech = (dbTech*) via->getTech();
  }

  return params;
}

void dbVia::setDefault(bool val)
{
  _dbVia* via = (_dbVia*) this;
  via->flags_.default_via = val;
}

bool dbVia::isDefault()
{
  _dbVia* via = (_dbVia*) this;
  return via->flags_.default_via;
}

dbVia* dbVia::create(dbBlock* block_, const char* name_)
{
  if (block_->findVia(name_)) {
    return nullptr;
  }

  _dbBlock* block = (_dbBlock*) block_;
  _dbVia* via = block->via_tbl_->create();
  via->name_ = safe_strdup(name_);
  return (dbVia*) via;
}

dbVia* dbVia::create(dbBlock* block,
                     const char* name,
                     dbVia* blk_via,
                     dbOrientType orient)
{
  _dbVia* via = (_dbVia*) dbVia::create(block, name);

  if (via == nullptr) {
    return nullptr;
  }

  via->flags_.is_rotated = 1;
  via->flags_.orient = orient;
  via->rotated_via_id_ = blk_via->getId();

  dbTransform t(orient);

  for (dbBox* box : blk_via->getBoxes()) {
    dbTechLayer* l = box->getTechLayer();
    Rect r = ((_dbBox*) box)->shape_.rect;
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

  if (via == nullptr) {
    return nullptr;
  }

  via->flags_.is_rotated = 1;
  via->flags_.is_tech_via = 1;
  via->flags_.orient = orient;
  via->rotated_via_id_ = tech_via->getId();

  dbTransform t(orient);

  for (dbBox* box : tech_via->getBoxes()) {
    dbTechLayer* l = box->getTechLayer();
    Rect r = ((_dbBox*) box)->shape_.rect;
    t.apply(r);
    dbBox::create((dbVia*) via, l, r.xMin(), r.yMin(), r.xMax(), r.yMax());
  }

  return (dbVia*) via;
}

static dbVia* copyVia(dbBlock* block_, dbVia* via_, bool copyRotatedVia)
{
  _dbBlock* block = (_dbBlock*) block_;
  _dbVia* via = (_dbVia*) via_;

  _dbVia* cvia = block->via_tbl_->create();

  cvia->flags_ = via->flags_;
  cvia->name_ = safe_strdup(via->name_);

  if (via->pattern_) {
    cvia->pattern_ = safe_strdup(via->pattern_);
  }

  cvia->top_ = via->top_;
  cvia->bottom_ = via->bottom_;

  for (dbBox* b : via_->getBoxes()) {
    dbTechLayer* l = b->getTechLayer();
    dbBox::create((dbVia*) cvia, l, b->xMin(), b->yMin(), b->xMax(), b->yMax());
  }

  if (via->flags_.is_rotated) {
    if (via->flags_.is_tech_via) {
      cvia->rotated_via_id_ = via->rotated_via_id_;
    } else {
      _dbVia* bv = (_dbVia*) via_->getBlockVia();
      _dbVia* cbv = (_dbVia*) block_->findVia(bv->name_);

      if (copyRotatedVia && (cbv == nullptr)) {
        cbv = (_dbVia*) copyVia(block_, (dbVia*) bv, true);
      }

      assert(cbv);
      cvia->rotated_via_id_ = cbv->getOID();
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
  // copy non rotated via's first
  for (dbVia* v : src_->getVias()) {
    _dbVia* via = (_dbVia*) v;

    if (!v->isViaRotated()) {
      if (!dst_->findVia(via->name_)) {
        copyVia(dst_, v, false);
      }
    }
  }

  // copy rotated via's last
  for (dbVia* v : src_->getVias()) {
    _dbVia* via = (_dbVia*) v;

    if (v->isViaRotated()) {
      if (!dst_->findVia(via->name_)) {
        copyVia(dst_, v, false);
      }
    }
  }

  return true;
}

dbVia* dbVia::getVia(dbBlock* block_, uint32_t dbid_)
{
  _dbBlock* block = (_dbBlock*) block_;
  return (dbVia*) block->via_tbl_->getPtr(dbid_);
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

void _dbVia::collectMemInfo(MemInfo& info)
{
  info.cnt++;
  info.size += sizeof(*this);

  info.children["name"].add(name_);
  info.children["pattern"].add(pattern_);
}

}  // namespace odb
