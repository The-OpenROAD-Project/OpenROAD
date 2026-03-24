// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "odb/dbViaParams.h"

#include <cassert>

#include "odb/db.h"

namespace odb {

////////////////////////////////////////////////////////////////////
//
// _dbViaParams - Methods
//
////////////////////////////////////////////////////////////////////

bool _dbViaParams::operator==(const _dbViaParams& rhs) const
{
  if (x_cut_size_ != rhs.x_cut_size_) {
    return false;
  }

  if (y_cut_size_ != rhs.y_cut_size_) {
    return false;
  }

  if (x_cut_spacing_ != rhs.x_cut_spacing_) {
    return false;
  }

  if (y_cut_spacing_ != rhs.y_cut_spacing_) {
    return false;
  }

  if (x_top_enclosure_ != rhs.x_top_enclosure_) {
    return false;
  }

  if (y_top_enclosure_ != rhs.y_top_enclosure_) {
    return false;
  }

  if (x_bot_enclosure_ != rhs.x_bot_enclosure_) {
    return false;
  }

  if (y_bot_enclosure_ != rhs.y_bot_enclosure_) {
    return false;
  }

  if (num_cut_rows_ != rhs.num_cut_rows_) {
    return false;
  }

  if (num_cut_cols_ != rhs.num_cut_cols_) {
    return false;
  }

  if (x_origin_ != rhs.x_origin_) {
    return false;
  }

  if (y_origin_ != rhs.y_origin_) {
    return false;
  }

  if (x_top_offset_ != rhs.x_top_offset_) {
    return false;
  }

  if (y_top_offset_ != rhs.y_top_offset_) {
    return false;
  }

  if (x_bot_offset_ != rhs.x_bot_offset_) {
    return false;
  }

  if (y_bot_offset_ != rhs.y_bot_offset_) {
    return false;
  }

  if (top_layer_ != rhs.top_layer_) {
    return false;
  }

  if (cut_layer_ != rhs.cut_layer_) {
    return false;
  }

  if (bot_layer_ != rhs.bot_layer_) {
    return false;
  }

  return true;
}

dbOStream& operator<<(dbOStream& stream, const _dbViaParams& v)
{
  stream << v.x_cut_size_;
  stream << v.y_cut_size_;
  stream << v.x_cut_spacing_;
  stream << v.y_cut_spacing_;
  stream << v.x_top_enclosure_;
  stream << v.y_top_enclosure_;
  stream << v.x_bot_enclosure_;
  stream << v.y_bot_enclosure_;
  stream << v.num_cut_rows_;
  stream << v.num_cut_cols_;
  stream << v.x_origin_;
  stream << v.y_origin_;
  stream << v.x_top_offset_;
  stream << v.y_top_offset_;
  stream << v.x_bot_offset_;
  stream << v.y_bot_offset_;
  stream << v.top_layer_;
  stream << v.cut_layer_;
  stream << v.bot_layer_;
  return stream;
}

dbIStream& operator>>(dbIStream& stream, _dbViaParams& v)
{
  stream >> v.x_cut_size_;
  stream >> v.y_cut_size_;
  stream >> v.x_cut_spacing_;
  stream >> v.y_cut_spacing_;
  stream >> v.x_top_enclosure_;
  stream >> v.y_top_enclosure_;
  stream >> v.x_bot_enclosure_;
  stream >> v.y_bot_enclosure_;
  stream >> v.num_cut_rows_;
  stream >> v.num_cut_cols_;
  stream >> v.x_origin_;
  stream >> v.y_origin_;
  stream >> v.x_top_offset_;
  stream >> v.y_top_offset_;
  stream >> v.x_bot_offset_;
  stream >> v.y_bot_offset_;
  stream >> v.top_layer_;
  stream >> v.cut_layer_;
  stream >> v.bot_layer_;
  return stream;
}

////////////////////////////////////////////////////////////////////
//
// dbViaParams - Methods
//
////////////////////////////////////////////////////////////////////

dbViaParams::dbViaParams() : _tech(nullptr)
{
}

dbViaParams::dbViaParams(const _dbViaParams& p)
    : _dbViaParams(p), _tech(nullptr)
{
}

int dbViaParams::getXCutSize() const
{
  return x_cut_size_;
}

int dbViaParams::getYCutSize() const
{
  return y_cut_size_;
}

int dbViaParams::getXCutSpacing() const
{
  return x_cut_spacing_;
}

int dbViaParams::getYCutSpacing() const
{
  return y_cut_spacing_;
}

int dbViaParams::getXTopEnclosure() const
{
  return x_top_enclosure_;
}

int dbViaParams::getYTopEnclosure() const
{
  return y_top_enclosure_;
}

int dbViaParams::getXBottomEnclosure() const
{
  return x_bot_enclosure_;
}

int dbViaParams::getYBottomEnclosure() const
{
  return y_bot_enclosure_;
}

int dbViaParams::getNumCutRows() const
{
  return num_cut_rows_;
}

int dbViaParams::getNumCutCols() const
{
  return num_cut_cols_;
}

int dbViaParams::getXOrigin() const
{
  return x_origin_;
}

int dbViaParams::getYOrigin() const
{
  return y_origin_;
}

int dbViaParams::getXTopOffset() const
{
  return x_top_offset_;
}

int dbViaParams::getYTopOffset() const
{
  return y_top_offset_;
}

int dbViaParams::getXBottomOffset() const
{
  return x_bot_offset_;
}

int dbViaParams::getYBottomOffset() const
{
  return y_bot_offset_;
}

dbTechLayer* dbViaParams::getTopLayer() const
{
  if (_tech == nullptr) {
    return nullptr;
  }

  return dbTechLayer::getTechLayer(_tech, top_layer_);
}

dbTechLayer* dbViaParams::getCutLayer() const
{
  if (_tech == nullptr) {
    return nullptr;
  }

  return dbTechLayer::getTechLayer(_tech, cut_layer_);
}

dbTechLayer* dbViaParams::getBottomLayer() const
{
  if (_tech == nullptr) {
    return nullptr;
  }

  return dbTechLayer::getTechLayer(_tech, bot_layer_);
}

void dbViaParams::setXCutSize(int value)
{
  x_cut_size_ = value;
}

void dbViaParams::setYCutSize(int value)
{
  y_cut_size_ = value;
}

void dbViaParams::setXCutSpacing(int value)
{
  x_cut_spacing_ = value;
}

void dbViaParams::setYCutSpacing(int value)
{
  y_cut_spacing_ = value;
}

void dbViaParams::setXTopEnclosure(int value)
{
  x_top_enclosure_ = value;
}

void dbViaParams::setYTopEnclosure(int value)
{
  y_top_enclosure_ = value;
}

void dbViaParams::setXBottomEnclosure(int value)
{
  x_bot_enclosure_ = value;
}

void dbViaParams::setYBottomEnclosure(int value)
{
  y_bot_enclosure_ = value;
}

void dbViaParams::setNumCutRows(int value)
{
  num_cut_rows_ = value;
}

void dbViaParams::setNumCutCols(int value)
{
  num_cut_cols_ = value;
}

void dbViaParams::setXOrigin(int value)
{
  x_origin_ = value;
}

void dbViaParams::setYOrigin(int value)
{
  y_origin_ = value;
}

void dbViaParams::setXTopOffset(int value)
{
  x_top_offset_ = value;
}

void dbViaParams::setYTopOffset(int value)
{
  y_top_offset_ = value;
}

void dbViaParams::setXBottomOffset(int value)
{
  x_bot_offset_ = value;
}

void dbViaParams::setYBottomOffset(int value)
{
  y_bot_offset_ = value;
}

void dbViaParams::setTopLayer(dbTechLayer* layer)
{
  if (_tech == nullptr) {
    _tech = layer->getTech();
  }

  assert(_tech == layer->getTech());
  top_layer_ = layer->getId();
}

void dbViaParams::setCutLayer(dbTechLayer* layer)
{
  if (_tech == nullptr) {
    _tech = layer->getTech();
  }

  assert(_tech == layer->getTech());
  cut_layer_ = layer->getId();
}

void dbViaParams::setBottomLayer(dbTechLayer* layer)
{
  if (_tech == nullptr) {
    _tech = layer->getTech();
  }

  assert(_tech == layer->getTech());
  bot_layer_ = layer->getId();
}

}  // namespace odb
