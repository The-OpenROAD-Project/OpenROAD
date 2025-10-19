// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2020-2025, The OpenROAD Authors

// Generator Code Begin Cpp
#include "dbTechLayerCutSpacingRule.h"

#include <cstdint>
#include <cstring>

#include "dbDatabase.h"
#include "dbTable.h"
#include "dbTable.hpp"
#include "dbTechLayer.h"
#include "odb/db.h"
// User Code Begin Includes
#include "dbTech.h"
#include "dbTechLayerCutClassRule.h"
// User Code End Includes
namespace odb {
template class dbTable<_dbTechLayerCutSpacingRule>;

bool _dbTechLayerCutSpacingRule::operator==(
    const _dbTechLayerCutSpacingRule& rhs) const
{
  if (flags_.center_to_center_ != rhs.flags_.center_to_center_) {
    return false;
  }
  if (flags_.same_net_ != rhs.flags_.same_net_) {
    return false;
  }
  if (flags_.same_metal_ != rhs.flags_.same_metal_) {
    return false;
  }
  if (flags_.same_via_ != rhs.flags_.same_via_) {
    return false;
  }
  if (flags_.cut_spacing_type_ != rhs.flags_.cut_spacing_type_) {
    return false;
  }
  if (flags_.stack_ != rhs.flags_.stack_) {
    return false;
  }
  if (flags_.orthogonal_spacing_valid_
      != rhs.flags_.orthogonal_spacing_valid_) {
    return false;
  }
  if (flags_.above_width_enclosure_valid_
      != rhs.flags_.above_width_enclosure_valid_) {
    return false;
  }
  if (flags_.short_edge_only_ != rhs.flags_.short_edge_only_) {
    return false;
  }
  if (flags_.concave_corner_width_ != rhs.flags_.concave_corner_width_) {
    return false;
  }
  if (flags_.concave_corner_parallel_ != rhs.flags_.concave_corner_parallel_) {
    return false;
  }
  if (flags_.concave_corner_edge_length_
      != rhs.flags_.concave_corner_edge_length_) {
    return false;
  }
  if (flags_.concave_corner_ != rhs.flags_.concave_corner_) {
    return false;
  }
  if (flags_.extension_valid_ != rhs.flags_.extension_valid_) {
    return false;
  }
  if (flags_.non_eol_convex_corner_ != rhs.flags_.non_eol_convex_corner_) {
    return false;
  }
  if (flags_.eol_width_valid_ != rhs.flags_.eol_width_valid_) {
    return false;
  }
  if (flags_.min_length_valid_ != rhs.flags_.min_length_valid_) {
    return false;
  }
  if (flags_.above_width_valid_ != rhs.flags_.above_width_valid_) {
    return false;
  }
  if (flags_.mask_overlap_ != rhs.flags_.mask_overlap_) {
    return false;
  }
  if (flags_.wrong_direction_ != rhs.flags_.wrong_direction_) {
    return false;
  }
  if (flags_.adjacent_cuts_ != rhs.flags_.adjacent_cuts_) {
    return false;
  }
  if (flags_.exact_aligned_ != rhs.flags_.exact_aligned_) {
    return false;
  }
  if (flags_.cut_class_to_all_ != rhs.flags_.cut_class_to_all_) {
    return false;
  }
  if (flags_.no_prl_ != rhs.flags_.no_prl_) {
    return false;
  }
  if (flags_.same_mask_ != rhs.flags_.same_mask_) {
    return false;
  }
  if (flags_.except_same_pgnet_ != rhs.flags_.except_same_pgnet_) {
    return false;
  }
  if (flags_.side_parallel_overlap_ != rhs.flags_.side_parallel_overlap_) {
    return false;
  }
  if (flags_.except_same_net_ != rhs.flags_.except_same_net_) {
    return false;
  }
  if (flags_.except_same_metal_ != rhs.flags_.except_same_metal_) {
    return false;
  }
  if (flags_.except_same_metal_overlap_
      != rhs.flags_.except_same_metal_overlap_) {
    return false;
  }
  if (flags_.except_same_via_ != rhs.flags_.except_same_via_) {
    return false;
  }
  if (flags_.above_ != rhs.flags_.above_) {
    return false;
  }
  if (flags_.except_two_edges_ != rhs.flags_.except_two_edges_) {
    return false;
  }
  if (flags_.two_cuts_valid_ != rhs.flags_.two_cuts_valid_) {
    return false;
  }
  if (flags_.same_cut_ != rhs.flags_.same_cut_) {
    return false;
  }
  if (flags_.long_edge_only_ != rhs.flags_.long_edge_only_) {
    return false;
  }
  if (flags_.prl_valid_ != rhs.flags_.prl_valid_) {
    return false;
  }
  if (flags_.below_ != rhs.flags_.below_) {
    return false;
  }
  if (flags_.par_within_enclosure_valid_
      != rhs.flags_.par_within_enclosure_valid_) {
    return false;
  }
  if (cut_spacing_ != rhs.cut_spacing_) {
    return false;
  }
  if (second_layer_ != rhs.second_layer_) {
    return false;
  }
  if (orthogonal_spacing_ != rhs.orthogonal_spacing_) {
    return false;
  }
  if (width_ != rhs.width_) {
    return false;
  }
  if (enclosure_ != rhs.enclosure_) {
    return false;
  }
  if (edge_length_ != rhs.edge_length_) {
    return false;
  }
  if (par_within_ != rhs.par_within_) {
    return false;
  }
  if (par_enclosure_ != rhs.par_enclosure_) {
    return false;
  }
  if (edge_enclosure_ != rhs.edge_enclosure_) {
    return false;
  }
  if (adj_enclosure_ != rhs.adj_enclosure_) {
    return false;
  }
  if (above_enclosure_ != rhs.above_enclosure_) {
    return false;
  }
  if (above_width_ != rhs.above_width_) {
    return false;
  }
  if (min_length_ != rhs.min_length_) {
    return false;
  }
  if (extension_ != rhs.extension_) {
    return false;
  }
  if (eol_width_ != rhs.eol_width_) {
    return false;
  }
  if (num_cuts_ != rhs.num_cuts_) {
    return false;
  }
  if (within_ != rhs.within_) {
    return false;
  }
  if (second_within_ != rhs.second_within_) {
    return false;
  }
  if (cut_class_ != rhs.cut_class_) {
    return false;
  }
  if (two_cuts_ != rhs.two_cuts_) {
    return false;
  }
  if (prl_ != rhs.prl_) {
    return false;
  }
  if (par_length_ != rhs.par_length_) {
    return false;
  }
  if (cut_area_ != rhs.cut_area_) {
    return false;
  }

  return true;
}

bool _dbTechLayerCutSpacingRule::operator<(
    const _dbTechLayerCutSpacingRule& rhs) const
{
  return true;
}

_dbTechLayerCutSpacingRule::_dbTechLayerCutSpacingRule(_dbDatabase* db)
{
  flags_ = {};
  cut_spacing_ = 0;
  orthogonal_spacing_ = 0;
  width_ = 0;
  enclosure_ = 0;
  edge_length_ = 0;
  par_within_ = 0;
  par_enclosure_ = 0;
  edge_enclosure_ = 0;
  adj_enclosure_ = 0;
  above_enclosure_ = 0;
  above_width_ = 0;
  min_length_ = 0;
  extension_ = 0;
  eol_width_ = 0;
  num_cuts_ = 0;
  within_ = 0;
  second_within_ = 0;
  two_cuts_ = 0;
  prl_ = 0;
  par_length_ = 0;
  cut_area_ = 0;
}

dbIStream& operator>>(dbIStream& stream, _dbTechLayerCutSpacingRule& obj)
{
  uint64_t flags_bit_field;
  stream >> flags_bit_field;
  static_assert(sizeof(obj.flags_) == sizeof(flags_bit_field));
  std::memcpy(&obj.flags_, &flags_bit_field, sizeof(flags_bit_field));
  stream >> obj.cut_spacing_;
  stream >> obj.second_layer_;
  stream >> obj.orthogonal_spacing_;
  stream >> obj.width_;
  stream >> obj.enclosure_;
  stream >> obj.edge_length_;
  stream >> obj.par_within_;
  stream >> obj.par_enclosure_;
  stream >> obj.edge_enclosure_;
  stream >> obj.adj_enclosure_;
  stream >> obj.above_enclosure_;
  stream >> obj.above_width_;
  stream >> obj.min_length_;
  stream >> obj.extension_;
  stream >> obj.eol_width_;
  stream >> obj.num_cuts_;
  stream >> obj.within_;
  stream >> obj.second_within_;
  stream >> obj.cut_class_;
  stream >> obj.two_cuts_;
  stream >> obj.prl_;
  stream >> obj.par_length_;
  stream >> obj.cut_area_;
  return stream;
}

dbOStream& operator<<(dbOStream& stream, const _dbTechLayerCutSpacingRule& obj)
{
  uint64_t flags_bit_field;
  static_assert(sizeof(obj.flags_) == sizeof(flags_bit_field));
  std::memcpy(&flags_bit_field, &obj.flags_, sizeof(obj.flags_));
  stream << flags_bit_field;
  stream << obj.cut_spacing_;
  stream << obj.second_layer_;
  stream << obj.orthogonal_spacing_;
  stream << obj.width_;
  stream << obj.enclosure_;
  stream << obj.edge_length_;
  stream << obj.par_within_;
  stream << obj.par_enclosure_;
  stream << obj.edge_enclosure_;
  stream << obj.adj_enclosure_;
  stream << obj.above_enclosure_;
  stream << obj.above_width_;
  stream << obj.min_length_;
  stream << obj.extension_;
  stream << obj.eol_width_;
  stream << obj.num_cuts_;
  stream << obj.within_;
  stream << obj.second_within_;
  stream << obj.cut_class_;
  stream << obj.two_cuts_;
  stream << obj.prl_;
  stream << obj.par_length_;
  stream << obj.cut_area_;
  return stream;
}

void _dbTechLayerCutSpacingRule::collectMemInfo(MemInfo& info)
{
  info.cnt++;
  info.size += sizeof(*this);
}

////////////////////////////////////////////////////////////////////
//
// dbTechLayerCutSpacingRule - Methods
//
////////////////////////////////////////////////////////////////////

void dbTechLayerCutSpacingRule::setCutSpacing(int cut_spacing)
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;

  obj->cut_spacing_ = cut_spacing;
}

int dbTechLayerCutSpacingRule::getCutSpacing() const
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;
  return obj->cut_spacing_;
}

void dbTechLayerCutSpacingRule::setSecondLayer(dbTechLayer* second_layer)
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;

  obj->second_layer_ = second_layer->getImpl()->getOID();
}

void dbTechLayerCutSpacingRule::setOrthogonalSpacing(int orthogonal_spacing)
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;

  obj->orthogonal_spacing_ = orthogonal_spacing;
}

int dbTechLayerCutSpacingRule::getOrthogonalSpacing() const
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;
  return obj->orthogonal_spacing_;
}

void dbTechLayerCutSpacingRule::setWidth(int width)
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;

  obj->width_ = width;
}

int dbTechLayerCutSpacingRule::getWidth() const
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;
  return obj->width_;
}

void dbTechLayerCutSpacingRule::setEnclosure(int enclosure)
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;

  obj->enclosure_ = enclosure;
}

int dbTechLayerCutSpacingRule::getEnclosure() const
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;
  return obj->enclosure_;
}

void dbTechLayerCutSpacingRule::setEdgeLength(int edge_length)
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;

  obj->edge_length_ = edge_length;
}

int dbTechLayerCutSpacingRule::getEdgeLength() const
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;
  return obj->edge_length_;
}

void dbTechLayerCutSpacingRule::setParWithin(int par_within)
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;

  obj->par_within_ = par_within;
}

int dbTechLayerCutSpacingRule::getParWithin() const
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;
  return obj->par_within_;
}

void dbTechLayerCutSpacingRule::setParEnclosure(int par_enclosure)
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;

  obj->par_enclosure_ = par_enclosure;
}

int dbTechLayerCutSpacingRule::getParEnclosure() const
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;
  return obj->par_enclosure_;
}

void dbTechLayerCutSpacingRule::setEdgeEnclosure(int edge_enclosure)
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;

  obj->edge_enclosure_ = edge_enclosure;
}

int dbTechLayerCutSpacingRule::getEdgeEnclosure() const
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;
  return obj->edge_enclosure_;
}

void dbTechLayerCutSpacingRule::setAdjEnclosure(int adj_enclosure)
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;

  obj->adj_enclosure_ = adj_enclosure;
}

int dbTechLayerCutSpacingRule::getAdjEnclosure() const
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;
  return obj->adj_enclosure_;
}

void dbTechLayerCutSpacingRule::setAboveEnclosure(int above_enclosure)
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;

  obj->above_enclosure_ = above_enclosure;
}

int dbTechLayerCutSpacingRule::getAboveEnclosure() const
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;
  return obj->above_enclosure_;
}

void dbTechLayerCutSpacingRule::setAboveWidth(int above_width)
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;

  obj->above_width_ = above_width;
}

int dbTechLayerCutSpacingRule::getAboveWidth() const
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;
  return obj->above_width_;
}

void dbTechLayerCutSpacingRule::setMinLength(int min_length)
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;

  obj->min_length_ = min_length;
}

int dbTechLayerCutSpacingRule::getMinLength() const
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;
  return obj->min_length_;
}

void dbTechLayerCutSpacingRule::setExtension(int extension)
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;

  obj->extension_ = extension;
}

int dbTechLayerCutSpacingRule::getExtension() const
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;
  return obj->extension_;
}

void dbTechLayerCutSpacingRule::setEolWidth(int eol_width)
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;

  obj->eol_width_ = eol_width;
}

int dbTechLayerCutSpacingRule::getEolWidth() const
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;
  return obj->eol_width_;
}

void dbTechLayerCutSpacingRule::setNumCuts(uint num_cuts)
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;

  obj->num_cuts_ = num_cuts;
}

uint dbTechLayerCutSpacingRule::getNumCuts() const
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;
  return obj->num_cuts_;
}

void dbTechLayerCutSpacingRule::setWithin(int within)
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;

  obj->within_ = within;
}

int dbTechLayerCutSpacingRule::getWithin() const
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;
  return obj->within_;
}

void dbTechLayerCutSpacingRule::setSecondWithin(int second_within)
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;

  obj->second_within_ = second_within;
}

int dbTechLayerCutSpacingRule::getSecondWithin() const
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;
  return obj->second_within_;
}

void dbTechLayerCutSpacingRule::setCutClass(dbTechLayerCutClassRule* cut_class)
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;

  obj->cut_class_ = cut_class->getImpl()->getOID();
}

void dbTechLayerCutSpacingRule::setTwoCuts(uint two_cuts)
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;

  obj->two_cuts_ = two_cuts;
}

uint dbTechLayerCutSpacingRule::getTwoCuts() const
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;
  return obj->two_cuts_;
}

void dbTechLayerCutSpacingRule::setPrl(uint prl)
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;

  obj->prl_ = prl;
}

uint dbTechLayerCutSpacingRule::getPrl() const
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;
  return obj->prl_;
}

void dbTechLayerCutSpacingRule::setParLength(uint par_length)
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;

  obj->par_length_ = par_length;
}

uint dbTechLayerCutSpacingRule::getParLength() const
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;
  return obj->par_length_;
}

void dbTechLayerCutSpacingRule::setCutArea(int cut_area)
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;

  obj->cut_area_ = cut_area;
}

int dbTechLayerCutSpacingRule::getCutArea() const
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;
  return obj->cut_area_;
}

void dbTechLayerCutSpacingRule::setCenterToCenter(bool center_to_center)
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;

  obj->flags_.center_to_center_ = center_to_center;
}

bool dbTechLayerCutSpacingRule::isCenterToCenter() const
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;

  return obj->flags_.center_to_center_;
}

void dbTechLayerCutSpacingRule::setSameNet(bool same_net)
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;

  obj->flags_.same_net_ = same_net;
}

bool dbTechLayerCutSpacingRule::isSameNet() const
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;

  return obj->flags_.same_net_;
}

void dbTechLayerCutSpacingRule::setSameMetal(bool same_metal)
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;

  obj->flags_.same_metal_ = same_metal;
}

bool dbTechLayerCutSpacingRule::isSameMetal() const
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;

  return obj->flags_.same_metal_;
}

void dbTechLayerCutSpacingRule::setSameVia(bool same_via)
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;

  obj->flags_.same_via_ = same_via;
}

bool dbTechLayerCutSpacingRule::isSameVia() const
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;

  return obj->flags_.same_via_;
}

void dbTechLayerCutSpacingRule::setStack(bool stack)
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;

  obj->flags_.stack_ = stack;
}

bool dbTechLayerCutSpacingRule::isStack() const
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;

  return obj->flags_.stack_;
}

void dbTechLayerCutSpacingRule::setOrthogonalSpacingValid(
    bool orthogonal_spacing_valid)
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;

  obj->flags_.orthogonal_spacing_valid_ = orthogonal_spacing_valid;
}

bool dbTechLayerCutSpacingRule::isOrthogonalSpacingValid() const
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;

  return obj->flags_.orthogonal_spacing_valid_;
}

void dbTechLayerCutSpacingRule::setAboveWidthEnclosureValid(
    bool above_width_enclosure_valid)
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;

  obj->flags_.above_width_enclosure_valid_ = above_width_enclosure_valid;
}

bool dbTechLayerCutSpacingRule::isAboveWidthEnclosureValid() const
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;

  return obj->flags_.above_width_enclosure_valid_;
}

void dbTechLayerCutSpacingRule::setShortEdgeOnly(bool short_edge_only)
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;

  obj->flags_.short_edge_only_ = short_edge_only;
}

bool dbTechLayerCutSpacingRule::isShortEdgeOnly() const
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;

  return obj->flags_.short_edge_only_;
}

void dbTechLayerCutSpacingRule::setConcaveCornerWidth(bool concave_corner_width)
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;

  obj->flags_.concave_corner_width_ = concave_corner_width;
}

bool dbTechLayerCutSpacingRule::isConcaveCornerWidth() const
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;

  return obj->flags_.concave_corner_width_;
}

void dbTechLayerCutSpacingRule::setConcaveCornerParallel(
    bool concave_corner_parallel)
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;

  obj->flags_.concave_corner_parallel_ = concave_corner_parallel;
}

bool dbTechLayerCutSpacingRule::isConcaveCornerParallel() const
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;

  return obj->flags_.concave_corner_parallel_;
}

void dbTechLayerCutSpacingRule::setConcaveCornerEdgeLength(
    bool concave_corner_edge_length)
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;

  obj->flags_.concave_corner_edge_length_ = concave_corner_edge_length;
}

bool dbTechLayerCutSpacingRule::isConcaveCornerEdgeLength() const
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;

  return obj->flags_.concave_corner_edge_length_;
}

void dbTechLayerCutSpacingRule::setConcaveCorner(bool concave_corner)
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;

  obj->flags_.concave_corner_ = concave_corner;
}

bool dbTechLayerCutSpacingRule::isConcaveCorner() const
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;

  return obj->flags_.concave_corner_;
}

void dbTechLayerCutSpacingRule::setExtensionValid(bool extension_valid)
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;

  obj->flags_.extension_valid_ = extension_valid;
}

bool dbTechLayerCutSpacingRule::isExtensionValid() const
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;

  return obj->flags_.extension_valid_;
}

void dbTechLayerCutSpacingRule::setNonEolConvexCorner(
    bool non_eol_convex_corner)
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;

  obj->flags_.non_eol_convex_corner_ = non_eol_convex_corner;
}

bool dbTechLayerCutSpacingRule::isNonEolConvexCorner() const
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;

  return obj->flags_.non_eol_convex_corner_;
}

void dbTechLayerCutSpacingRule::setEolWidthValid(bool eol_width_valid)
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;

  obj->flags_.eol_width_valid_ = eol_width_valid;
}

bool dbTechLayerCutSpacingRule::isEolWidthValid() const
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;

  return obj->flags_.eol_width_valid_;
}

void dbTechLayerCutSpacingRule::setMinLengthValid(bool min_length_valid)
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;

  obj->flags_.min_length_valid_ = min_length_valid;
}

bool dbTechLayerCutSpacingRule::isMinLengthValid() const
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;

  return obj->flags_.min_length_valid_;
}

void dbTechLayerCutSpacingRule::setAboveWidthValid(bool above_width_valid)
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;

  obj->flags_.above_width_valid_ = above_width_valid;
}

bool dbTechLayerCutSpacingRule::isAboveWidthValid() const
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;

  return obj->flags_.above_width_valid_;
}

void dbTechLayerCutSpacingRule::setMaskOverlap(bool mask_overlap)
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;

  obj->flags_.mask_overlap_ = mask_overlap;
}

bool dbTechLayerCutSpacingRule::isMaskOverlap() const
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;

  return obj->flags_.mask_overlap_;
}

void dbTechLayerCutSpacingRule::setWrongDirection(bool wrong_direction)
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;

  obj->flags_.wrong_direction_ = wrong_direction;
}

bool dbTechLayerCutSpacingRule::isWrongDirection() const
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;

  return obj->flags_.wrong_direction_;
}

void dbTechLayerCutSpacingRule::setAdjacentCuts(uint adjacent_cuts)
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;

  obj->flags_.adjacent_cuts_ = adjacent_cuts;
}

uint dbTechLayerCutSpacingRule::getAdjacentCuts() const
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;

  return obj->flags_.adjacent_cuts_;
}

void dbTechLayerCutSpacingRule::setExactAligned(bool exact_aligned)
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;

  obj->flags_.exact_aligned_ = exact_aligned;
}

bool dbTechLayerCutSpacingRule::isExactAligned() const
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;

  return obj->flags_.exact_aligned_;
}

void dbTechLayerCutSpacingRule::setCutClassToAll(bool cut_class_to_all)
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;

  obj->flags_.cut_class_to_all_ = cut_class_to_all;
}

bool dbTechLayerCutSpacingRule::isCutClassToAll() const
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;

  return obj->flags_.cut_class_to_all_;
}

void dbTechLayerCutSpacingRule::setNoPrl(bool no_prl)
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;

  obj->flags_.no_prl_ = no_prl;
}

bool dbTechLayerCutSpacingRule::isNoPrl() const
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;

  return obj->flags_.no_prl_;
}

void dbTechLayerCutSpacingRule::setSameMask(bool same_mask)
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;

  obj->flags_.same_mask_ = same_mask;
}

bool dbTechLayerCutSpacingRule::isSameMask() const
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;

  return obj->flags_.same_mask_;
}

void dbTechLayerCutSpacingRule::setExceptSamePgnet(bool except_same_pgnet)
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;

  obj->flags_.except_same_pgnet_ = except_same_pgnet;
}

bool dbTechLayerCutSpacingRule::isExceptSamePgnet() const
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;

  return obj->flags_.except_same_pgnet_;
}

void dbTechLayerCutSpacingRule::setSideParallelOverlap(
    bool side_parallel_overlap)
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;

  obj->flags_.side_parallel_overlap_ = side_parallel_overlap;
}

bool dbTechLayerCutSpacingRule::isSideParallelOverlap() const
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;

  return obj->flags_.side_parallel_overlap_;
}

void dbTechLayerCutSpacingRule::setExceptSameNet(bool except_same_net)
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;

  obj->flags_.except_same_net_ = except_same_net;
}

bool dbTechLayerCutSpacingRule::isExceptSameNet() const
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;

  return obj->flags_.except_same_net_;
}

void dbTechLayerCutSpacingRule::setExceptSameMetal(bool except_same_metal)
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;

  obj->flags_.except_same_metal_ = except_same_metal;
}

bool dbTechLayerCutSpacingRule::isExceptSameMetal() const
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;

  return obj->flags_.except_same_metal_;
}

void dbTechLayerCutSpacingRule::setExceptSameMetalOverlap(
    bool except_same_metal_overlap)
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;

  obj->flags_.except_same_metal_overlap_ = except_same_metal_overlap;
}

bool dbTechLayerCutSpacingRule::isExceptSameMetalOverlap() const
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;

  return obj->flags_.except_same_metal_overlap_;
}

void dbTechLayerCutSpacingRule::setExceptSameVia(bool except_same_via)
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;

  obj->flags_.except_same_via_ = except_same_via;
}

bool dbTechLayerCutSpacingRule::isExceptSameVia() const
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;

  return obj->flags_.except_same_via_;
}

void dbTechLayerCutSpacingRule::setAbove(bool above)
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;

  obj->flags_.above_ = above;
}

bool dbTechLayerCutSpacingRule::isAbove() const
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;

  return obj->flags_.above_;
}

void dbTechLayerCutSpacingRule::setExceptTwoEdges(bool except_two_edges)
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;

  obj->flags_.except_two_edges_ = except_two_edges;
}

bool dbTechLayerCutSpacingRule::isExceptTwoEdges() const
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;

  return obj->flags_.except_two_edges_;
}

void dbTechLayerCutSpacingRule::setTwoCutsValid(bool two_cuts_valid)
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;

  obj->flags_.two_cuts_valid_ = two_cuts_valid;
}

bool dbTechLayerCutSpacingRule::isTwoCutsValid() const
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;

  return obj->flags_.two_cuts_valid_;
}

void dbTechLayerCutSpacingRule::setSameCut(bool same_cut)
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;

  obj->flags_.same_cut_ = same_cut;
}

bool dbTechLayerCutSpacingRule::isSameCut() const
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;

  return obj->flags_.same_cut_;
}

void dbTechLayerCutSpacingRule::setLongEdgeOnly(bool long_edge_only)
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;

  obj->flags_.long_edge_only_ = long_edge_only;
}

bool dbTechLayerCutSpacingRule::isLongEdgeOnly() const
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;

  return obj->flags_.long_edge_only_;
}

void dbTechLayerCutSpacingRule::setPrlValid(bool prl_valid)
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;

  obj->flags_.prl_valid_ = prl_valid;
}

bool dbTechLayerCutSpacingRule::isPrlValid() const
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;

  return obj->flags_.prl_valid_;
}

void dbTechLayerCutSpacingRule::setBelow(bool below)
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;

  obj->flags_.below_ = below;
}

bool dbTechLayerCutSpacingRule::isBelow() const
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;

  return obj->flags_.below_;
}

void dbTechLayerCutSpacingRule::setParWithinEnclosureValid(
    bool par_within_enclosure_valid)
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;

  obj->flags_.par_within_enclosure_valid_ = par_within_enclosure_valid;
}

bool dbTechLayerCutSpacingRule::isParWithinEnclosureValid() const
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;

  return obj->flags_.par_within_enclosure_valid_;
}

// User Code Begin dbTechLayerCutSpacingRulePublicMethods
dbTechLayerCutClassRule* dbTechLayerCutSpacingRule::getCutClass() const
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;
  if (obj->cut_class_ == 0) {
    return nullptr;
  }
  _dbTechLayer* layer = (_dbTechLayer*) obj->getOwner();
  return (dbTechLayerCutClassRule*) layer->cut_class_rules_tbl_->getPtr(
      obj->cut_class_);
}

dbTechLayer* dbTechLayerCutSpacingRule::getSecondLayer() const
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;
  if (obj->second_layer_ == 0) {
    return nullptr;
  }
  _dbTechLayer* layer = (_dbTechLayer*) obj->getOwner();
  _dbTech* _tech = (_dbTech*) layer->getOwner();
  return (dbTechLayer*) _tech->_layer_tbl->getPtr(obj->second_layer_);
}

dbTechLayer* dbTechLayerCutSpacingRule::getTechLayer() const
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;
  return (odb::dbTechLayer*) obj->getOwner();
}

void dbTechLayerCutSpacingRule::setType(CutSpacingType _type)
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;

  obj->flags_.cut_spacing_type_ = (uint) _type;
}

dbTechLayerCutSpacingRule::CutSpacingType dbTechLayerCutSpacingRule::getType()
    const
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;

  return (dbTechLayerCutSpacingRule::CutSpacingType)
      obj->flags_.cut_spacing_type_;
}

dbTechLayerCutSpacingRule* dbTechLayerCutSpacingRule::create(
    dbTechLayer* _layer)
{
  _dbTechLayer* layer = (_dbTechLayer*) _layer;
  _dbTechLayerCutSpacingRule* newrule = layer->cut_spacing_rules_tbl_->create();
  return ((dbTechLayerCutSpacingRule*) newrule);
}

dbTechLayerCutSpacingRule*
dbTechLayerCutSpacingRule::getTechLayerCutSpacingRule(dbTechLayer* inly,
                                                      uint dbid)
{
  _dbTechLayer* layer = (_dbTechLayer*) inly;
  return (dbTechLayerCutSpacingRule*) layer->cut_spacing_rules_tbl_->getPtr(
      dbid);
}
void dbTechLayerCutSpacingRule::destroy(dbTechLayerCutSpacingRule* rule)
{
  _dbTechLayer* layer = (_dbTechLayer*) rule->getImpl()->getOwner();
  dbProperty::destroyProperties(rule);
  layer->cut_spacing_rules_tbl_->destroy((_dbTechLayerCutSpacingRule*) rule);
}
// User Code End dbTechLayerCutSpacingRulePublicMethods
}  // namespace odb
// Generator Code End Cpp
