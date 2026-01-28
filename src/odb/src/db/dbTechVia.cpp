// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "dbTechVia.h"

#include <string.h>  // NOLINT(modernize-deprecated-headers): for strdup()

#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

#include "dbBox.h"
#include "dbBoxItr.h"
#include "dbCommon.h"
#include "dbCore.h"
#include "dbDatabase.h"
#include "dbTable.h"
#include "dbTech.h"
#include "dbTechLayer.h"
#include "dbTechNonDefaultRule.h"
#include "dbTechViaGenerateRule.h"
#include "odb/db.h"
#include "odb/dbSet.h"
#include "odb/dbViaParams.h"
#include "odb/geom.h"

namespace odb {

template class dbTable<_dbTechVia>;
static void create_via_boxes(_dbTechVia* via, const dbViaParams& P);

bool _dbTechVia::operator==(const _dbTechVia& rhs) const
{
  if (flags_.default_via != rhs.flags_.default_via) {
    return false;
  }

  if (flags_.top_of_stack != rhs.flags_.top_of_stack) {
    return false;
  }

  if (resistance_ != rhs.resistance_) {
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

  if (non_default_rule_ != rhs.non_default_rule_) {
    return false;
  }

  if (generate_rule_ != rhs.generate_rule_) {
    return false;
  }

  if (via_params_ != rhs.via_params_) {
    return false;
  }

  if (next_entry_ != rhs.next_entry_) {
    return false;
  }

  return true;
}

////////////////////////////////////////////////////////////////////
//
// _dbTechVia - Methods
//
////////////////////////////////////////////////////////////////////

_dbTechVia::_dbTechVia(_dbDatabase*, const _dbTechVia& v)
    : flags_(v.flags_),
      resistance_(v.resistance_),
      name_(nullptr),
      pattern_(nullptr),
      bbox_(v.bbox_),
      boxes_(v.boxes_),
      top_(v.top_),
      bottom_(v.bottom_),
      non_default_rule_(v.non_default_rule_),
      generate_rule_(v.generate_rule_),
      via_params_(v.via_params_),
      next_entry_(v.next_entry_)
{
  if (v.name_) {
    name_ = safe_strdup(v.name_);
  }

  if (v.pattern_) {
    pattern_ = safe_strdup(v.pattern_);
  }
}

_dbTechVia::_dbTechVia(_dbDatabase*)
{
  flags_.default_via = 0;
  flags_.top_of_stack = 0;
  flags_.has_params = 0;
  flags_.spare_bits = 0;
  resistance_ = 0.0;
  name_ = nullptr;
  pattern_ = nullptr;
}

_dbTechVia::~_dbTechVia()
{
  if (name_) {
    free((void*) name_);
  }

  if (pattern_) {
    free((void*) pattern_);
  }
}

dbOStream& operator<<(dbOStream& stream, const _dbTechVia& via)
{
  uint32_t* bit_field = (uint32_t*) &via.flags_;
  stream << *bit_field;
  stream << via.resistance_;
  stream << via.name_;
  stream << via.bbox_;
  stream << via.boxes_;
  stream << via.top_;
  stream << via.bottom_;
  stream << via.non_default_rule_;
  stream << via.generate_rule_;
  stream << via.via_params_;
  stream << via.pattern_;
  stream << via.next_entry_;
  return stream;
}

dbIStream& operator>>(dbIStream& stream, _dbTechVia& via)
{
  uint32_t* bit_field = (uint32_t*) &via.flags_;
  stream >> *bit_field;
  stream >> via.resistance_;
  stream >> via.name_;
  stream >> via.bbox_;
  stream >> via.boxes_;
  stream >> via.top_;
  stream >> via.bottom_;
  stream >> via.non_default_rule_;
  stream >> via.generate_rule_;
  stream >> via.via_params_;
  stream >> via.pattern_;
  stream >> via.next_entry_;

  return stream;
}

////////////////////////////////////////////////////////////////////
//
// dbTechVia - Methods
//
////////////////////////////////////////////////////////////////////

std::string dbTechVia::getName()
{
  _dbTechVia* via = (_dbTechVia*) this;
  return via->name_;
}

const char* dbTechVia::getConstName()
{
  _dbTechVia* via = (_dbTechVia*) this;
  return via->name_;
}

bool dbTechVia::isDefault()
{
  _dbTechVia* via = (_dbTechVia*) this;
  return via->flags_.default_via == 1;
}

void dbTechVia::setDefault()
{
  _dbTechVia* via = (_dbTechVia*) this;
  via->flags_.default_via = 1;
}

bool dbTechVia::isTopOfStack()
{
  _dbTechVia* via = (_dbTechVia*) this;
  return via->flags_.top_of_stack == 1;
}

void dbTechVia::setTopOfStack()
{
  _dbTechVia* via = (_dbTechVia*) this;
  via->flags_.top_of_stack = 1;
}

double dbTechVia::getResistance()
{
  _dbTechVia* via = (_dbTechVia*) this;
  return via->resistance_;
}

void dbTechVia::setResistance(double resistance)
{
  _dbTechVia* via = (_dbTechVia*) this;
  via->resistance_ = resistance;
}

dbTech* dbTechVia::getTech()
{
  return (dbTech*) getImpl()->getOwner();
}

dbBox* dbTechVia::getBBox()
{
  _dbTechVia* via = (_dbTechVia*) this;

  if (via->bbox_ == 0) {
    return nullptr;
  }

  _dbTech* tech = (_dbTech*) via->getOwner();
  return (dbBox*) tech->box_tbl_->getPtr(via->bbox_);
}

dbSet<dbBox> dbTechVia::getBoxes()
{
  _dbTechVia* via = (_dbTechVia*) this;
  _dbTech* tech = (_dbTech*) via->getOwner();
  return dbSet<dbBox>(via, tech->box_itr_);
}

dbTechLayer* dbTechVia::getTopLayer()
{
  _dbTechVia* via = (_dbTechVia*) this;

  if (via->top_ == 0) {
    return nullptr;
  }

  _dbTech* tech = (_dbTech*) via->getOwner();
  return (dbTechLayer*) tech->layer_tbl_->getPtr(via->top_);
}

dbTechLayer* dbTechVia::getBottomLayer()
{
  _dbTechVia* via = (_dbTechVia*) this;

  if (via->bottom_ == 0) {
    return nullptr;
  }

  _dbTech* tech = (_dbTech*) via->getOwner();
  return (dbTechLayer*) tech->layer_tbl_->getPtr(via->bottom_);
}

dbTechNonDefaultRule* dbTechVia::getNonDefaultRule()
{
  _dbTechVia* via = (_dbTechVia*) this;

  if (via->non_default_rule_ == 0) {
    return nullptr;
  }

  _dbTech* tech = (_dbTech*) via->getOwner();
  return (dbTechNonDefaultRule*) tech->non_default_rule_tbl_->getPtr(
      via->non_default_rule_);
}

bool dbTechVia::hasParams()
{
  _dbTechVia* via = (_dbTechVia*) this;
  return via->flags_.has_params == 1;
}

void dbTechVia::setViaGenerateRule(dbTechViaGenerateRule* rule)
{
  _dbTechVia* via = (_dbTechVia*) this;
  via->generate_rule_ = rule->getImpl()->getOID();
}

dbTechViaGenerateRule* dbTechVia::getViaGenerateRule()
{
  _dbTechVia* via = (_dbTechVia*) this;

  if (via->generate_rule_ == 0) {
    return nullptr;
  }

  _dbTech* tech = (_dbTech*) via->getOwner();
  _dbTechViaGenerateRule* rule
      = tech->via_generate_rule_tbl_->getPtr(via->generate_rule_);
  return (dbTechViaGenerateRule*) rule;
}

std::string dbTechVia::getPattern()
{
  _dbTechVia* via = (_dbTechVia*) this;

  if (via->pattern_ == nullptr) {
    return "";
  }

  return via->pattern_;
}

void dbTechVia::setPattern(const char* pattern)
{
  _dbTechVia* via = (_dbTechVia*) this;

  if (via->pattern_) {
    free((void*) via->pattern_);
  }

  via->pattern_ = strdup(pattern);
}

void dbTechVia::setViaParams(const dbViaParams& params)
{
  _dbTechVia* via = (_dbTechVia*) this;
  _dbTech* tech = (_dbTech*) via->getOwner();
  via->flags_.has_params = 1;

  // Clear previous boxes
  dbSet<dbBox> boxes = getBoxes();
  dbSet<dbBox>::iterator itr;

  for (itr = boxes.begin(); itr != boxes.end();) {
    dbSet<dbBox>::iterator cur = itr++;
    _dbBox* box = (_dbBox*) *cur;
    dbProperty::destroyProperties(box);
    tech->box_tbl_->destroy(box);
  }

  via->boxes_ = 0U;
  via->via_params_ = params;
  via->top_ = params.top_layer_;
  via->bottom_ = params.bot_layer_;
  create_via_boxes(via, params);
}

dbViaParams dbTechVia::getViaParams()
{
  _dbTechVia* via = (_dbTechVia*) this;

  dbViaParams params;
  if (via->flags_.has_params == 0) {
    params = dbViaParams();
  } else {
    params = via->via_params_;
    _dbTech* tech = (_dbTech*) via->getOwner();
    params._tech = (dbTech*) tech;
  }
  return params;
}

dbTechVia* dbTechVia::create(dbTech* tech_, const char* name_)
{
  if (tech_->findVia(name_)) {
    return nullptr;
  }

  _dbTech* tech = (_dbTech*) tech_;
  _dbTechVia* via = tech->via_tbl_->create();
  via->name_ = safe_strdup(name_);
  tech->via_hash_.insert(via);
  tech->via_cnt_++;
  return (dbTechVia*) via;
}

dbTechVia* dbTechVia::clone(dbTechNonDefaultRule* rule_,
                            dbTechVia* invia_,
                            const char* new_name)
{
  _dbTechNonDefaultRule* rule = (_dbTechNonDefaultRule*) rule_;
  _dbTechVia* _invia = (_dbTechVia*) invia_;

  dbTech* tech_ = (dbTech*) rule->getOwner();

  if (tech_->findVia(new_name)) {
    return nullptr;
  }

  _dbTech* tech = (_dbTech*) tech_;
  _dbTechVia* via = tech->via_tbl_->create();
  via->name_ = safe_strdup(new_name);

  via->flags_ = _invia->flags_;
  via->resistance_ = _invia->resistance_;
  via->bbox_ = _invia->bbox_;
  via->boxes_ = _invia->boxes_;
  via->top_ = _invia->top_;
  via->bottom_ = _invia->bottom_;
  via->non_default_rule_ = (rule) ? rule->getOID() : 0;
  if (rule) {
    via->flags_.default_via
        = 0;  // DEFAULT via not allowed for non-default rule
  }

  tech->via_cnt_++;
  tech->via_hash_.insert(via);
  rule->vias_.push_back(via->getOID());
  return (dbTechVia*) via;
}

dbTechVia* dbTechVia::create(dbTechNonDefaultRule* rule_, const char* name_)
{
  _dbTechNonDefaultRule* rule = (_dbTechNonDefaultRule*) rule_;

  dbTech* tech_ = (dbTech*) rule->getOwner();

  if (tech_->findVia(name_)) {
    return nullptr;
  }

  _dbTech* tech = (_dbTech*) tech_;
  _dbTechVia* via = tech->via_tbl_->create();
  via->name_ = safe_strdup(name_);
  tech->via_cnt_++;
  via->non_default_rule_ = rule->getOID();
  tech->via_hash_.insert(via);
  rule->vias_.push_back(via->getOID());
  return (dbTechVia*) via;
}

dbTechVia* dbTechVia::getTechVia(dbTech* tech_, uint32_t dbid_)
{
  _dbTech* tech = (_dbTech*) tech_;
  return (dbTechVia*) tech->via_tbl_->getPtr(dbid_);
}

void create_via_boxes(_dbTechVia* via, const dbViaParams& P)
{
  const int rows = P.getNumCutRows();
  const int cols = P.getNumCutCols();
  int y = 0;
  int maxX = 0;
  int maxY = 0;
  std::vector<Rect> cutRects;

  for (int row = 0; row < rows; ++row) {
    int x = 0;

    for (int col = 0; col < cols; ++col) {
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

  const int dx = maxX / 2;
  const int dy = maxY / 2;

  for (Rect& r : cutRects) {
    r.moveDelta(-dx, -dy);
    r.moveDelta(P.getXOrigin(), P.getYOrigin());
    dbBox::create(
        (dbTechVia*) via, cut_layer, r.xMin(), r.yMin(), r.xMax(), r.yMax());
  }

  int minX = -dx;
  int minY = -dy;
  maxX -= dx;
  maxY -= dy;

  const int top_minX
      = minX - P.getXTopEnclosure() + P.getXOrigin() + P.getXTopOffset();
  const int top_minY
      = minY - P.getYTopEnclosure() + P.getYOrigin() + P.getYTopOffset();
  const int top_maxX
      = maxX + P.getXTopEnclosure() + P.getXOrigin() + P.getXTopOffset();
  const int top_maxY
      = maxY + P.getYTopEnclosure() + P.getYOrigin() + P.getYTopOffset();
  dbBox::create((dbTechVia*) via,
                P.getTopLayer(),
                top_minX,
                top_minY,
                top_maxX,
                top_maxY);

  const int bot_minX
      = minX - P.getXBottomEnclosure() + P.getXOrigin() + P.getXBottomOffset();
  const int bot_minY
      = minY - P.getYBottomEnclosure() + P.getYOrigin() + P.getYBottomOffset();
  const int bot_maxX
      = maxX + P.getXBottomEnclosure() + P.getXOrigin() + P.getXBottomOffset();
  const int bot_maxY
      = maxY + P.getYBottomEnclosure() + P.getYOrigin() + P.getYBottomOffset();
  dbBox::create((dbTechVia*) via,
                P.getBottomLayer(),
                bot_minX,
                bot_minY,
                bot_maxX,
                bot_maxY);
}

void _dbTechVia::collectMemInfo(MemInfo& info)
{
  info.cnt++;
  info.size += sizeof(*this);

  info.children["name"].add(name_);
  info.children["pattern"].add(pattern_);
}

}  // namespace odb
