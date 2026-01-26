// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2020-2025, The OpenROAD Authors

// Generator Code Begin Cpp
#include "dbTechLayerCutSpacingTableDefRule.h"

#include <cstdint>
#include <cstring>
#include <map>
#include <string>
#include <tuple>

#include "dbCore.h"
#include "dbDatabase.h"
#include "dbTable.h"
#include "dbTechLayer.h"
#include "odb/db.h"
// User Code Begin Includes
#include "dbTech.h"
#include "dbTechLayerCutClassRule.h"
// User Code End Includes
namespace odb {
template class dbTable<_dbTechLayerCutSpacingTableDefRule>;

bool _dbTechLayerCutSpacingTableDefRule::operator==(
    const _dbTechLayerCutSpacingTableDefRule& rhs) const
{
  if (flags_.default_valid != rhs.flags_.default_valid) {
    return false;
  }
  if (flags_.same_mask != rhs.flags_.same_mask) {
    return false;
  }
  if (flags_.same_net != rhs.flags_.same_net) {
    return false;
  }
  if (flags_.same_metal != rhs.flags_.same_metal) {
    return false;
  }
  if (flags_.same_via != rhs.flags_.same_via) {
    return false;
  }
  if (flags_.layer_valid != rhs.flags_.layer_valid) {
    return false;
  }
  if (flags_.no_stack != rhs.flags_.no_stack) {
    return false;
  }
  if (flags_.non_zero_enclosure != rhs.flags_.non_zero_enclosure) {
    return false;
  }
  if (flags_.prl_for_aligned_cut != rhs.flags_.prl_for_aligned_cut) {
    return false;
  }
  if (flags_.center_to_center_valid != rhs.flags_.center_to_center_valid) {
    return false;
  }
  if (flags_.center_and_edge_valid != rhs.flags_.center_and_edge_valid) {
    return false;
  }
  if (flags_.no_prl != rhs.flags_.no_prl) {
    return false;
  }
  if (flags_.prl_valid != rhs.flags_.prl_valid) {
    return false;
  }
  if (flags_.max_x_y != rhs.flags_.max_x_y) {
    return false;
  }
  if (flags_.end_extension_valid != rhs.flags_.end_extension_valid) {
    return false;
  }
  if (flags_.side_extension_valid != rhs.flags_.side_extension_valid) {
    return false;
  }
  if (flags_.exact_aligned_spacing_valid
      != rhs.flags_.exact_aligned_spacing_valid) {
    return false;
  }
  if (flags_.horizontal != rhs.flags_.horizontal) {
    return false;
  }
  if (flags_.prl_horizontal != rhs.flags_.prl_horizontal) {
    return false;
  }
  if (flags_.vertical != rhs.flags_.vertical) {
    return false;
  }
  if (flags_.prl_vertical != rhs.flags_.prl_vertical) {
    return false;
  }
  if (flags_.non_opposite_enclosure_spacing_valid
      != rhs.flags_.non_opposite_enclosure_spacing_valid) {
    return false;
  }
  if (flags_.opposite_enclosure_resize_spacing_valid
      != rhs.flags_.opposite_enclosure_resize_spacing_valid) {
    return false;
  }
  if (default_ != rhs.default_) {
    return false;
  }
  if (second_layer_ != rhs.second_layer_) {
    return false;
  }
  if (prl_ != rhs.prl_) {
    return false;
  }
  if (extension_ != rhs.extension_) {
    return false;
  }

  return true;
}

bool _dbTechLayerCutSpacingTableDefRule::operator<(
    const _dbTechLayerCutSpacingTableDefRule& rhs) const
{
  return true;
}

_dbTechLayerCutSpacingTableDefRule::_dbTechLayerCutSpacingTableDefRule(
    _dbDatabase* db)
{
  flags_ = {};
  default_ = 0;
  prl_ = 0;
  extension_ = 0;
}

dbIStream& operator>>(dbIStream& stream,
                      _dbTechLayerCutSpacingTableDefRule& obj)
{
  uint32_t flags_bit_field;
  stream >> flags_bit_field;
  static_assert(sizeof(obj.flags_) == sizeof(flags_bit_field));
  std::memcpy(&obj.flags_, &flags_bit_field, sizeof(flags_bit_field));
  stream >> obj.default_;
  stream >> obj.second_layer_;
  stream >> obj.prl_for_aligned_cut_tbl_;
  stream >> obj.center_to_center_tbl_;
  stream >> obj.center_and_edge_tbl_;
  stream >> obj.prl_;
  stream >> obj.prl_tbl_;
  stream >> obj.extension_;
  stream >> obj.end_extension_tbl_;
  stream >> obj.side_extension_tbl_;
  stream >> obj.exact_aligned_spacing_tbl_;
  stream >> obj.non_opp_enc_spacing_tbl_;
  stream >> obj.opp_enc_spacing_tbl_;
  stream >> obj.spacing_tbl_;
  stream >> obj.row_map_;
  stream >> obj.col_map_;
  return stream;
}

dbOStream& operator<<(dbOStream& stream,
                      const _dbTechLayerCutSpacingTableDefRule& obj)
{
  uint32_t flags_bit_field;
  static_assert(sizeof(obj.flags_) == sizeof(flags_bit_field));
  std::memcpy(&flags_bit_field, &obj.flags_, sizeof(obj.flags_));
  stream << flags_bit_field;
  stream << obj.default_;
  stream << obj.second_layer_;
  stream << obj.prl_for_aligned_cut_tbl_;
  stream << obj.center_to_center_tbl_;
  stream << obj.center_and_edge_tbl_;
  stream << obj.prl_;
  stream << obj.prl_tbl_;
  stream << obj.extension_;
  stream << obj.end_extension_tbl_;
  stream << obj.side_extension_tbl_;
  stream << obj.exact_aligned_spacing_tbl_;
  stream << obj.non_opp_enc_spacing_tbl_;
  stream << obj.opp_enc_spacing_tbl_;
  stream << obj.spacing_tbl_;
  stream << obj.row_map_;
  stream << obj.col_map_;
  return stream;
}

void _dbTechLayerCutSpacingTableDefRule::collectMemInfo(MemInfo& info)
{
  info.cnt++;
  info.size += sizeof(*this);

  // User Code Begin collectMemInfo
  info.children["prl_for_aligned_cut_tbl_"].add(prl_for_aligned_cut_tbl_);
  info.children["center_to_center_tbl_"].add(center_to_center_tbl_);
  info.children["center_and_edge_tbl_"].add(center_and_edge_tbl_);
  info.children["prl_tbl_"].add(prl_tbl_);
  info.children["end_extension_tbl_"].add(end_extension_tbl_);
  info.children["side_extension_tbl_"].add(side_extension_tbl_);
  info.children["exact_aligned_spacing_tbl_"].add(exact_aligned_spacing_tbl_);
  info.children["non_opp_enc_spacing_tbl_"].add(non_opp_enc_spacing_tbl_);
  info.children["opp_enc_spacing_tbl_"].add(opp_enc_spacing_tbl_);
  info.children["spacing_tbl_"].add(spacing_tbl_);
  info.children["row_map_"].add(row_map_);
  info.children["col_map_"].add(col_map_);
  // User Code End collectMemInfo
}

////////////////////////////////////////////////////////////////////
//
// dbTechLayerCutSpacingTableDefRule - Methods
//
////////////////////////////////////////////////////////////////////

void dbTechLayerCutSpacingTableDefRule::setDefault(int spacing)
{
  _dbTechLayerCutSpacingTableDefRule* obj
      = (_dbTechLayerCutSpacingTableDefRule*) this;

  obj->default_ = spacing;
}

int dbTechLayerCutSpacingTableDefRule::getDefault() const
{
  _dbTechLayerCutSpacingTableDefRule* obj
      = (_dbTechLayerCutSpacingTableDefRule*) this;
  return obj->default_;
}

void dbTechLayerCutSpacingTableDefRule::setSecondLayer(
    dbTechLayer* second_layer)
{
  _dbTechLayerCutSpacingTableDefRule* obj
      = (_dbTechLayerCutSpacingTableDefRule*) this;

  obj->second_layer_ = second_layer->getImpl()->getOID();
}

void dbTechLayerCutSpacingTableDefRule::setPrl(int prl)
{
  _dbTechLayerCutSpacingTableDefRule* obj
      = (_dbTechLayerCutSpacingTableDefRule*) this;

  obj->prl_ = prl;
}

int dbTechLayerCutSpacingTableDefRule::getPrl() const
{
  _dbTechLayerCutSpacingTableDefRule* obj
      = (_dbTechLayerCutSpacingTableDefRule*) this;
  return obj->prl_;
}

void dbTechLayerCutSpacingTableDefRule::setExtension(int extension)
{
  _dbTechLayerCutSpacingTableDefRule* obj
      = (_dbTechLayerCutSpacingTableDefRule*) this;

  obj->extension_ = extension;
}

int dbTechLayerCutSpacingTableDefRule::getExtension() const
{
  _dbTechLayerCutSpacingTableDefRule* obj
      = (_dbTechLayerCutSpacingTableDefRule*) this;
  return obj->extension_;
}

void dbTechLayerCutSpacingTableDefRule::setDefaultValid(bool default_valid)
{
  _dbTechLayerCutSpacingTableDefRule* obj
      = (_dbTechLayerCutSpacingTableDefRule*) this;

  obj->flags_.default_valid = default_valid;
}

bool dbTechLayerCutSpacingTableDefRule::isDefaultValid() const
{
  _dbTechLayerCutSpacingTableDefRule* obj
      = (_dbTechLayerCutSpacingTableDefRule*) this;

  return obj->flags_.default_valid;
}

void dbTechLayerCutSpacingTableDefRule::setSameMask(bool same_mask)
{
  _dbTechLayerCutSpacingTableDefRule* obj
      = (_dbTechLayerCutSpacingTableDefRule*) this;

  obj->flags_.same_mask = same_mask;
}

bool dbTechLayerCutSpacingTableDefRule::isSameMask() const
{
  _dbTechLayerCutSpacingTableDefRule* obj
      = (_dbTechLayerCutSpacingTableDefRule*) this;

  return obj->flags_.same_mask;
}

void dbTechLayerCutSpacingTableDefRule::setSameNet(bool same_net)
{
  _dbTechLayerCutSpacingTableDefRule* obj
      = (_dbTechLayerCutSpacingTableDefRule*) this;

  obj->flags_.same_net = same_net;
}

bool dbTechLayerCutSpacingTableDefRule::isSameNet() const
{
  _dbTechLayerCutSpacingTableDefRule* obj
      = (_dbTechLayerCutSpacingTableDefRule*) this;

  return obj->flags_.same_net;
}

void dbTechLayerCutSpacingTableDefRule::setSameMetal(bool same_metal)
{
  _dbTechLayerCutSpacingTableDefRule* obj
      = (_dbTechLayerCutSpacingTableDefRule*) this;

  obj->flags_.same_metal = same_metal;
}

bool dbTechLayerCutSpacingTableDefRule::isSameMetal() const
{
  _dbTechLayerCutSpacingTableDefRule* obj
      = (_dbTechLayerCutSpacingTableDefRule*) this;

  return obj->flags_.same_metal;
}

void dbTechLayerCutSpacingTableDefRule::setSameVia(bool same_via)
{
  _dbTechLayerCutSpacingTableDefRule* obj
      = (_dbTechLayerCutSpacingTableDefRule*) this;

  obj->flags_.same_via = same_via;
}

bool dbTechLayerCutSpacingTableDefRule::isSameVia() const
{
  _dbTechLayerCutSpacingTableDefRule* obj
      = (_dbTechLayerCutSpacingTableDefRule*) this;

  return obj->flags_.same_via;
}

void dbTechLayerCutSpacingTableDefRule::setLayerValid(bool layer_valid)
{
  _dbTechLayerCutSpacingTableDefRule* obj
      = (_dbTechLayerCutSpacingTableDefRule*) this;

  obj->flags_.layer_valid = layer_valid;
}

bool dbTechLayerCutSpacingTableDefRule::isLayerValid() const
{
  _dbTechLayerCutSpacingTableDefRule* obj
      = (_dbTechLayerCutSpacingTableDefRule*) this;

  return obj->flags_.layer_valid;
}

void dbTechLayerCutSpacingTableDefRule::setNoStack(bool no_stack)
{
  _dbTechLayerCutSpacingTableDefRule* obj
      = (_dbTechLayerCutSpacingTableDefRule*) this;

  obj->flags_.no_stack = no_stack;
}

bool dbTechLayerCutSpacingTableDefRule::isNoStack() const
{
  _dbTechLayerCutSpacingTableDefRule* obj
      = (_dbTechLayerCutSpacingTableDefRule*) this;

  return obj->flags_.no_stack;
}

void dbTechLayerCutSpacingTableDefRule::setNonZeroEnclosure(
    bool non_zero_enclosure)
{
  _dbTechLayerCutSpacingTableDefRule* obj
      = (_dbTechLayerCutSpacingTableDefRule*) this;

  obj->flags_.non_zero_enclosure = non_zero_enclosure;
}

bool dbTechLayerCutSpacingTableDefRule::isNonZeroEnclosure() const
{
  _dbTechLayerCutSpacingTableDefRule* obj
      = (_dbTechLayerCutSpacingTableDefRule*) this;

  return obj->flags_.non_zero_enclosure;
}

void dbTechLayerCutSpacingTableDefRule::setPrlForAlignedCut(
    bool prl_for_aligned_cut)
{
  _dbTechLayerCutSpacingTableDefRule* obj
      = (_dbTechLayerCutSpacingTableDefRule*) this;

  obj->flags_.prl_for_aligned_cut = prl_for_aligned_cut;
}

bool dbTechLayerCutSpacingTableDefRule::isPrlForAlignedCut() const
{
  _dbTechLayerCutSpacingTableDefRule* obj
      = (_dbTechLayerCutSpacingTableDefRule*) this;

  return obj->flags_.prl_for_aligned_cut;
}

void dbTechLayerCutSpacingTableDefRule::setCenterToCenterValid(
    bool center_to_center_valid)
{
  _dbTechLayerCutSpacingTableDefRule* obj
      = (_dbTechLayerCutSpacingTableDefRule*) this;

  obj->flags_.center_to_center_valid = center_to_center_valid;
}

bool dbTechLayerCutSpacingTableDefRule::isCenterToCenterValid() const
{
  _dbTechLayerCutSpacingTableDefRule* obj
      = (_dbTechLayerCutSpacingTableDefRule*) this;

  return obj->flags_.center_to_center_valid;
}

void dbTechLayerCutSpacingTableDefRule::setCenterAndEdgeValid(
    bool center_and_edge_valid)
{
  _dbTechLayerCutSpacingTableDefRule* obj
      = (_dbTechLayerCutSpacingTableDefRule*) this;

  obj->flags_.center_and_edge_valid = center_and_edge_valid;
}

bool dbTechLayerCutSpacingTableDefRule::isCenterAndEdgeValid() const
{
  _dbTechLayerCutSpacingTableDefRule* obj
      = (_dbTechLayerCutSpacingTableDefRule*) this;

  return obj->flags_.center_and_edge_valid;
}

void dbTechLayerCutSpacingTableDefRule::setNoPrl(bool no_prl)
{
  _dbTechLayerCutSpacingTableDefRule* obj
      = (_dbTechLayerCutSpacingTableDefRule*) this;

  obj->flags_.no_prl = no_prl;
}

bool dbTechLayerCutSpacingTableDefRule::isNoPrl() const
{
  _dbTechLayerCutSpacingTableDefRule* obj
      = (_dbTechLayerCutSpacingTableDefRule*) this;

  return obj->flags_.no_prl;
}

void dbTechLayerCutSpacingTableDefRule::setPrlValid(bool prl_valid)
{
  _dbTechLayerCutSpacingTableDefRule* obj
      = (_dbTechLayerCutSpacingTableDefRule*) this;

  obj->flags_.prl_valid = prl_valid;
}

bool dbTechLayerCutSpacingTableDefRule::isPrlValid() const
{
  _dbTechLayerCutSpacingTableDefRule* obj
      = (_dbTechLayerCutSpacingTableDefRule*) this;

  return obj->flags_.prl_valid;
}

void dbTechLayerCutSpacingTableDefRule::setMaxXY(bool max_x_y)
{
  _dbTechLayerCutSpacingTableDefRule* obj
      = (_dbTechLayerCutSpacingTableDefRule*) this;

  obj->flags_.max_x_y = max_x_y;
}

bool dbTechLayerCutSpacingTableDefRule::isMaxXY() const
{
  _dbTechLayerCutSpacingTableDefRule* obj
      = (_dbTechLayerCutSpacingTableDefRule*) this;

  return obj->flags_.max_x_y;
}

void dbTechLayerCutSpacingTableDefRule::setEndExtensionValid(
    bool end_extension_valid)
{
  _dbTechLayerCutSpacingTableDefRule* obj
      = (_dbTechLayerCutSpacingTableDefRule*) this;

  obj->flags_.end_extension_valid = end_extension_valid;
}

bool dbTechLayerCutSpacingTableDefRule::isEndExtensionValid() const
{
  _dbTechLayerCutSpacingTableDefRule* obj
      = (_dbTechLayerCutSpacingTableDefRule*) this;

  return obj->flags_.end_extension_valid;
}

void dbTechLayerCutSpacingTableDefRule::setSideExtensionValid(
    bool side_extension_valid)
{
  _dbTechLayerCutSpacingTableDefRule* obj
      = (_dbTechLayerCutSpacingTableDefRule*) this;

  obj->flags_.side_extension_valid = side_extension_valid;
}

bool dbTechLayerCutSpacingTableDefRule::isSideExtensionValid() const
{
  _dbTechLayerCutSpacingTableDefRule* obj
      = (_dbTechLayerCutSpacingTableDefRule*) this;

  return obj->flags_.side_extension_valid;
}

void dbTechLayerCutSpacingTableDefRule::setExactAlignedSpacingValid(
    bool exact_aligned_spacing_valid)
{
  _dbTechLayerCutSpacingTableDefRule* obj
      = (_dbTechLayerCutSpacingTableDefRule*) this;

  obj->flags_.exact_aligned_spacing_valid = exact_aligned_spacing_valid;
}

bool dbTechLayerCutSpacingTableDefRule::isExactAlignedSpacingValid() const
{
  _dbTechLayerCutSpacingTableDefRule* obj
      = (_dbTechLayerCutSpacingTableDefRule*) this;

  return obj->flags_.exact_aligned_spacing_valid;
}

void dbTechLayerCutSpacingTableDefRule::setHorizontal(bool horizontal)
{
  _dbTechLayerCutSpacingTableDefRule* obj
      = (_dbTechLayerCutSpacingTableDefRule*) this;

  obj->flags_.horizontal = horizontal;
}

bool dbTechLayerCutSpacingTableDefRule::isHorizontal() const
{
  _dbTechLayerCutSpacingTableDefRule* obj
      = (_dbTechLayerCutSpacingTableDefRule*) this;

  return obj->flags_.horizontal;
}

void dbTechLayerCutSpacingTableDefRule::setPrlHorizontal(bool prl_horizontal)
{
  _dbTechLayerCutSpacingTableDefRule* obj
      = (_dbTechLayerCutSpacingTableDefRule*) this;

  obj->flags_.prl_horizontal = prl_horizontal;
}

bool dbTechLayerCutSpacingTableDefRule::isPrlHorizontal() const
{
  _dbTechLayerCutSpacingTableDefRule* obj
      = (_dbTechLayerCutSpacingTableDefRule*) this;

  return obj->flags_.prl_horizontal;
}

void dbTechLayerCutSpacingTableDefRule::setVertical(bool vertical)
{
  _dbTechLayerCutSpacingTableDefRule* obj
      = (_dbTechLayerCutSpacingTableDefRule*) this;

  obj->flags_.vertical = vertical;
}

bool dbTechLayerCutSpacingTableDefRule::isVertical() const
{
  _dbTechLayerCutSpacingTableDefRule* obj
      = (_dbTechLayerCutSpacingTableDefRule*) this;

  return obj->flags_.vertical;
}

void dbTechLayerCutSpacingTableDefRule::setPrlVertical(bool prl_vertical)
{
  _dbTechLayerCutSpacingTableDefRule* obj
      = (_dbTechLayerCutSpacingTableDefRule*) this;

  obj->flags_.prl_vertical = prl_vertical;
}

bool dbTechLayerCutSpacingTableDefRule::isPrlVertical() const
{
  _dbTechLayerCutSpacingTableDefRule* obj
      = (_dbTechLayerCutSpacingTableDefRule*) this;

  return obj->flags_.prl_vertical;
}

void dbTechLayerCutSpacingTableDefRule::setNonOppositeEnclosureSpacingValid(
    bool non_opposite_enclosure_spacing_valid)
{
  _dbTechLayerCutSpacingTableDefRule* obj
      = (_dbTechLayerCutSpacingTableDefRule*) this;

  obj->flags_.non_opposite_enclosure_spacing_valid
      = non_opposite_enclosure_spacing_valid;
}

bool dbTechLayerCutSpacingTableDefRule::isNonOppositeEnclosureSpacingValid()
    const
{
  _dbTechLayerCutSpacingTableDefRule* obj
      = (_dbTechLayerCutSpacingTableDefRule*) this;

  return obj->flags_.non_opposite_enclosure_spacing_valid;
}

void dbTechLayerCutSpacingTableDefRule::setOppositeEnclosureResizeSpacingValid(
    bool opposite_enclosure_resize_spacing_valid)
{
  _dbTechLayerCutSpacingTableDefRule* obj
      = (_dbTechLayerCutSpacingTableDefRule*) this;

  obj->flags_.opposite_enclosure_resize_spacing_valid
      = opposite_enclosure_resize_spacing_valid;
}

bool dbTechLayerCutSpacingTableDefRule::isOppositeEnclosureResizeSpacingValid()
    const
{
  _dbTechLayerCutSpacingTableDefRule* obj
      = (_dbTechLayerCutSpacingTableDefRule*) this;

  return obj->flags_.opposite_enclosure_resize_spacing_valid;
}

// User Code Begin dbTechLayerCutSpacingTableDefRulePublicMethods
void dbTechLayerCutSpacingTableDefRule::addPrlForAlignedCutEntry(
    const std::string& from,
    const std::string& to)
{
  _dbTechLayerCutSpacingTableDefRule* obj
      = (_dbTechLayerCutSpacingTableDefRule*) this;
  obj->prl_for_aligned_cut_tbl_.push_back({from, to});
}

void dbTechLayerCutSpacingTableDefRule::addCenterToCenterEntry(
    const std::string& from,
    const std::string& to)
{
  _dbTechLayerCutSpacingTableDefRule* obj
      = (_dbTechLayerCutSpacingTableDefRule*) this;
  obj->center_to_center_tbl_.push_back({from, to});
}

void dbTechLayerCutSpacingTableDefRule::addCenterAndEdgeEntry(
    const std::string& from,
    const std::string& to)
{
  _dbTechLayerCutSpacingTableDefRule* obj
      = (_dbTechLayerCutSpacingTableDefRule*) this;
  obj->center_and_edge_tbl_.push_back({from, to});
}

void dbTechLayerCutSpacingTableDefRule::addPrlEntry(const std::string& from,
                                                    const std::string& to,
                                                    int ccPrl)
{
  _dbTechLayerCutSpacingTableDefRule* obj
      = (_dbTechLayerCutSpacingTableDefRule*) this;
  obj->prl_tbl_.push_back({from, to, ccPrl});
}

void dbTechLayerCutSpacingTableDefRule::addEndExtensionEntry(
    const std::string& cls,
    int ext)
{
  _dbTechLayerCutSpacingTableDefRule* obj
      = (_dbTechLayerCutSpacingTableDefRule*) this;
  obj->end_extension_tbl_.push_back({cls, ext});
}

void dbTechLayerCutSpacingTableDefRule::addSideExtensionEntry(
    const std::string& cls,
    int ext)
{
  _dbTechLayerCutSpacingTableDefRule* obj
      = (_dbTechLayerCutSpacingTableDefRule*) this;
  obj->side_extension_tbl_.push_back({cls, ext});
}

void dbTechLayerCutSpacingTableDefRule::addExactElignedEntry(
    const std::string& cls,
    int spacing)
{
  _dbTechLayerCutSpacingTableDefRule* obj
      = (_dbTechLayerCutSpacingTableDefRule*) this;
  obj->exact_aligned_spacing_tbl_.push_back({cls, spacing});
}

void dbTechLayerCutSpacingTableDefRule::addNonOppEncSpacingEntry(
    const std::string& cls,
    int spacing)
{
  _dbTechLayerCutSpacingTableDefRule* obj
      = (_dbTechLayerCutSpacingTableDefRule*) this;
  obj->non_opp_enc_spacing_tbl_.push_back({cls, spacing});
}

void dbTechLayerCutSpacingTableDefRule::addOppEncSpacingEntry(
    const std::string& cls,
    int rsz1,
    int rsz2,
    int spacing)
{
  _dbTechLayerCutSpacingTableDefRule* obj
      = (_dbTechLayerCutSpacingTableDefRule*) this;
  obj->opp_enc_spacing_tbl_.push_back({cls, rsz1, rsz2, spacing});
}

dbTechLayer* dbTechLayerCutSpacingTableDefRule::getSecondLayer() const
{
  _dbTechLayerCutSpacingTableDefRule* obj
      = (_dbTechLayerCutSpacingTableDefRule*) this;
  if (obj->second_layer_ == 0) {
    return nullptr;
  }
  _dbTechLayer* layer = (_dbTechLayer*) obj->getOwner();
  _dbTech* _tech = (_dbTech*) layer->getOwner();
  return (dbTechLayer*) _tech->layer_tbl_->getPtr(obj->second_layer_);
}

void dbTechLayerCutSpacingTableDefRule::setSpacingTable(
    const std::vector<std::vector<std::pair<int, int>>>& table,
    const std::map<std::string, uint32_t>& row_map,
    const std::map<std::string, uint32_t>& col_map)
{
  _dbTechLayerCutSpacingTableDefRule* obj
      = (_dbTechLayerCutSpacingTableDefRule*) this;
  obj->row_map_ = row_map;
  obj->col_map_ = col_map;
  for (const auto& spacing : table) {
    dbVector<std::pair<int, int>> tmp;
    tmp = spacing;
    obj->spacing_tbl_.push_back(tmp);
  }
}

void dbTechLayerCutSpacingTableDefRule::getSpacingTable(
    std::vector<std::vector<std::pair<int, int>>>& table,
    std::map<std::string, uint32_t>& row_map,
    std::map<std::string, uint32_t>& col_map)
{
  _dbTechLayerCutSpacingTableDefRule* obj
      = (_dbTechLayerCutSpacingTableDefRule*) this;
  row_map = obj->row_map_;
  col_map = obj->col_map_;
  table.clear();
  for (const auto& spacing : obj->spacing_tbl_) {
    table.push_back(spacing);
  }
}

int dbTechLayerCutSpacingTableDefRule::getMaxSpacing(std::string cutClass,
                                                     bool SIDE) const
{
  _dbTechLayerCutSpacingTableDefRule* obj
      = (_dbTechLayerCutSpacingTableDefRule*) this;
  if (SIDE) {
    cutClass += "/SIDE";
  } else {
    cutClass += "/END";
  }

  if (obj->col_map_.find(cutClass) == obj->col_map_.end()) {
    return obj->default_;
  }
  auto colIdx = obj->col_map_[cutClass];
  auto spc = 0;
  for (const auto& row : obj->spacing_tbl_) {
    spc = std::max({spc, row[colIdx].first, row[colIdx].second});
  }
  return spc;
}

int dbTechLayerCutSpacingTableDefRule::getMaxSpacing(
    const std::string& cutClass1,
    const std::string& cutClass2,
    LOOKUP_STRATEGY strategy) const
{
  auto spc1 = getSpacing(cutClass1, true, cutClass2, true, strategy);
  auto spc2 = getSpacing(cutClass1, true, cutClass2, false, strategy);
  auto spc3 = getSpacing(cutClass1, false, cutClass2, true, strategy);
  auto spc4 = getSpacing(cutClass1, false, cutClass2, false, strategy);

  return std::max({spc1, spc2, spc3, spc4});
}

bool dbTechLayerCutSpacingTableDefRule::isCenterToCenter(
    const std::string& cutClass1,
    const std::string& cutClass2)
{
  _dbTechLayerCutSpacingTableDefRule* obj
      = (_dbTechLayerCutSpacingTableDefRule*) this;
  for (auto& [class1, class2] : obj->center_to_center_tbl_) {
    if ((class1 == cutClass1 || class1 == "ALL")
        && (class2 == cutClass2 || class2 == "ALL")) {
      return true;
    }
    if ((class1 == cutClass2 || class1 == "ALL")
        && (class2 == cutClass1 || class2 == "ALL")) {
      return true;
    }
  }
  return false;
}

int dbTechLayerCutSpacingTableDefRule::getExactAlignedSpacing(
    const std::string& cutClass) const
{
  _dbTechLayerCutSpacingTableDefRule* obj
      = (_dbTechLayerCutSpacingTableDefRule*) this;
  for (auto& [cls, spc] : obj->exact_aligned_spacing_tbl_) {
    if (cls == cutClass) {
      return spc;
    }
  }
  return -1;
}

bool dbTechLayerCutSpacingTableDefRule::isPrlForAlignedCutClasses(
    const std::string& cutClass1,
    const std::string& cutClass2)
{
  _dbTechLayerCutSpacingTableDefRule* obj
      = (_dbTechLayerCutSpacingTableDefRule*) this;
  for (auto& [class1, class2] : obj->prl_for_aligned_cut_tbl_) {
    if ((class1 == cutClass1 || class1 == "ALL")
        && (class2 == cutClass2 || class2 == "ALL")) {
      return true;
    }
  }
  return false;
}

int dbTechLayerCutSpacingTableDefRule::getPrlEntry(const std::string& cutClass1,
                                                   const std::string& cutClass2)
{
  _dbTechLayerCutSpacingTableDefRule* obj
      = (_dbTechLayerCutSpacingTableDefRule*) this;
  for (const auto& [class1, class2, prl] : obj->prl_tbl_) {
    if ((class1 == cutClass1 || class1 == "ALL")
        && (class2 == cutClass2 || class2 == "ALL")) {
      return prl;
    }
    if ((class1 == cutClass2 || class1 == "ALL")
        && (class2 == cutClass1 || class2 == "ALL")) {
      return prl;
    }
  }
  return obj->prl_;
}

bool dbTechLayerCutSpacingTableDefRule::isCenterAndEdge(
    const std::string& cutClass1,
    const std::string& cutClass2)
{
  _dbTechLayerCutSpacingTableDefRule* obj
      = (_dbTechLayerCutSpacingTableDefRule*) this;
  for (auto& [class1, class2] : obj->center_and_edge_tbl_) {
    if ((class1 == cutClass1 || class1 == "ALL")
        && (class2 == cutClass2 || class2 == "ALL")) {
      return true;
    }
    if ((class1 == cutClass2 || class1 == "ALL")
        && (class2 == cutClass1 || class2 == "ALL")) {
      return true;
    }
  }
  return false;
}

int dbTechLayerCutSpacingTableDefRule::getSpacing(
    std::string c1,
    bool SIDE1,
    std::string c2,
    bool SIDE2,
    LOOKUP_STRATEGY strategy) const
{
  _dbTechLayerCutSpacingTableDefRule* obj
      = (_dbTechLayerCutSpacingTableDefRule*) this;
  if (SIDE1) {
    c1 += "/SIDE";
  } else {
    c1 += "/END";
  }

  if (SIDE2) {
    c2 += "/SIDE";
  } else {
    c2 += "/END";
  }
  std::pair<int, int> res;
  if (obj->row_map_.find(c2) != obj->row_map_.end()
      && obj->col_map_.find(c1) != obj->col_map_.end()) {
    res = obj->spacing_tbl_[obj->row_map_[c2]][obj->col_map_[c1]];
  } else {
    res = {obj->default_, obj->default_};
  }
  switch (strategy) {
    case FIRST:
      return res.first;
    case SECOND:
      return res.second;
    case MAX:
      return std::max(res.first, res.second);
    case MIN:
      return std::min(res.first, res.second);
    default:
      return 0;
  }
}

dbTechLayer* dbTechLayerCutSpacingTableDefRule::getTechLayer() const
{
  _dbTechLayerCutSpacingTableDefRule* obj
      = (_dbTechLayerCutSpacingTableDefRule*) this;
  return (odb::dbTechLayer*) obj->getOwner();
}

dbTechLayerCutSpacingTableDefRule* dbTechLayerCutSpacingTableDefRule::create(
    dbTechLayer* parent)
{
  _dbTechLayer* _parent = (_dbTechLayer*) parent;
  _dbTechLayerCutSpacingTableDefRule* newrule
      = _parent->cut_spacing_table_def_tbl_->create();
  return ((dbTechLayerCutSpacingTableDefRule*) newrule);
}

dbTechLayerCutSpacingTableDefRule*
dbTechLayerCutSpacingTableDefRule::getTechLayerCutSpacingTableDefSubRule(
    dbTechLayer* parent,
    uint32_t dbid)
{
  _dbTechLayer* _parent = (_dbTechLayer*) parent;
  return (dbTechLayerCutSpacingTableDefRule*)
      _parent->cut_spacing_table_def_tbl_->getPtr(dbid);
}
void dbTechLayerCutSpacingTableDefRule::destroy(
    dbTechLayerCutSpacingTableDefRule* rule)
{
  _dbTechLayer* _parent = (_dbTechLayer*) rule->getImpl()->getOwner();
  dbProperty::destroyProperties(rule);
  _parent->cut_spacing_table_def_tbl_->destroy(
      (_dbTechLayerCutSpacingTableDefRule*) rule);
}

// User Code End dbTechLayerCutSpacingTableDefRulePublicMethods
}  // namespace odb
// Generator Code End Cpp
