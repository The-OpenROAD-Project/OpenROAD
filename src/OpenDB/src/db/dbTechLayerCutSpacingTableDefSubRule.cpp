///////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// Copyright (c) 2020, OpenRoad Project
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

// Generator Code Begin 1
#include "dbTechLayerCutSpacingTableDefSubRule.h"

#include "db.h"
#include "dbDatabase.h"
#include "dbDiff.hpp"
#include "dbTable.h"
#include "dbTable.hpp"
#include "dbTechLayer.h"
// User Code Begin includes
#include "dbTechLayerCutSpacingTableRule.h"
// User Code End includes
namespace odb {

// User Code Begin definitions
// User Code End definitions
template class dbTable<_dbTechLayerCutSpacingTableDefSubRule>;

bool _dbTechLayerCutSpacingTableDefSubRule::operator==(
    const _dbTechLayerCutSpacingTableDefSubRule& rhs) const
{
  if (_flags._default_valid != rhs._flags._default_valid)
    return false;

  if (_flags._same_mask != rhs._flags._same_mask)
    return false;

  if (_flags._same_net != rhs._flags._same_net)
    return false;

  if (_flags._same_metal != rhs._flags._same_metal)
    return false;

  if (_flags._same_via != rhs._flags._same_via)
    return false;

  if (_flags._layer_valid != rhs._flags._layer_valid)
    return false;

  if (_flags._no_stack != rhs._flags._no_stack)
    return false;

  if (_flags._non_zero_enclosure != rhs._flags._non_zero_enclosure)
    return false;

  if (_flags._prl_for_aligned_cut != rhs._flags._prl_for_aligned_cut)
    return false;

  if (_flags._center_to_center_valid != rhs._flags._center_to_center_valid)
    return false;

  if (_flags._center_and_edge_valid != rhs._flags._center_and_edge_valid)
    return false;

  if (_flags._no_prl != rhs._flags._no_prl)
    return false;

  if (_flags._prl_valid != rhs._flags._prl_valid)
    return false;

  if (_flags._max_x_y != rhs._flags._max_x_y)
    return false;

  if (_flags._end_extension_valid != rhs._flags._end_extension_valid)
    return false;

  if (_flags._side_extension_valid != rhs._flags._side_extension_valid)
    return false;

  if (_flags._exact_aligned_spacing_valid
      != rhs._flags._exact_aligned_spacing_valid)
    return false;

  if (_flags._horizontal != rhs._flags._horizontal)
    return false;

  if (_flags._vertical != rhs._flags._vertical)
    return false;

  if (_flags._non_opposite_enclosure_spacing_valid
      != rhs._flags._non_opposite_enclosure_spacing_valid)
    return false;

  if (_flags._opposite_enclosure_resize_spacing_valid
      != rhs._flags._opposite_enclosure_resize_spacing_valid)
    return false;

  if (_default != rhs._default)
    return false;

  if (_second_layer_name != rhs._second_layer_name)
    return false;

  if (_prl != rhs._prl)
    return false;

  if (_extension != rhs._extension)
    return false;

  // User Code Begin ==
  // User Code End ==
  return true;
}
bool _dbTechLayerCutSpacingTableDefSubRule::operator<(
    const _dbTechLayerCutSpacingTableDefSubRule& rhs) const
{
  // User Code Begin <
  // User Code End <
  return true;
}
void _dbTechLayerCutSpacingTableDefSubRule::differences(
    dbDiff&                                      diff,
    const char*                                  field,
    const _dbTechLayerCutSpacingTableDefSubRule& rhs) const
{
  DIFF_BEGIN

  DIFF_FIELD(_flags._default_valid);
  DIFF_FIELD(_flags._same_mask);
  DIFF_FIELD(_flags._same_net);
  DIFF_FIELD(_flags._same_metal);
  DIFF_FIELD(_flags._same_via);
  DIFF_FIELD(_flags._layer_valid);
  DIFF_FIELD(_flags._no_stack);
  DIFF_FIELD(_flags._non_zero_enclosure);
  DIFF_FIELD(_flags._prl_for_aligned_cut);
  DIFF_FIELD(_flags._center_to_center_valid);
  DIFF_FIELD(_flags._center_and_edge_valid);
  DIFF_FIELD(_flags._no_prl);
  DIFF_FIELD(_flags._prl_valid);
  DIFF_FIELD(_flags._max_x_y);
  DIFF_FIELD(_flags._end_extension_valid);
  DIFF_FIELD(_flags._side_extension_valid);
  DIFF_FIELD(_flags._exact_aligned_spacing_valid);
  DIFF_FIELD(_flags._horizontal);
  DIFF_FIELD(_flags._vertical);
  DIFF_FIELD(_flags._non_opposite_enclosure_spacing_valid);
  DIFF_FIELD(_flags._opposite_enclosure_resize_spacing_valid);
  DIFF_FIELD(_default);
  DIFF_FIELD(_second_layer_name);
  DIFF_FIELD(_prl);
  DIFF_FIELD(_extension);
  // User Code Begin differences
  // User Code End differences
  DIFF_END
}
void _dbTechLayerCutSpacingTableDefSubRule::out(dbDiff&     diff,
                                                char        side,
                                                const char* field) const
{
  DIFF_OUT_BEGIN
  DIFF_OUT_FIELD(_flags._default_valid);
  DIFF_OUT_FIELD(_flags._same_mask);
  DIFF_OUT_FIELD(_flags._same_net);
  DIFF_OUT_FIELD(_flags._same_metal);
  DIFF_OUT_FIELD(_flags._same_via);
  DIFF_OUT_FIELD(_flags._layer_valid);
  DIFF_OUT_FIELD(_flags._no_stack);
  DIFF_OUT_FIELD(_flags._non_zero_enclosure);
  DIFF_OUT_FIELD(_flags._prl_for_aligned_cut);
  DIFF_OUT_FIELD(_flags._center_to_center_valid);
  DIFF_OUT_FIELD(_flags._center_and_edge_valid);
  DIFF_OUT_FIELD(_flags._no_prl);
  DIFF_OUT_FIELD(_flags._prl_valid);
  DIFF_OUT_FIELD(_flags._max_x_y);
  DIFF_OUT_FIELD(_flags._end_extension_valid);
  DIFF_OUT_FIELD(_flags._side_extension_valid);
  DIFF_OUT_FIELD(_flags._exact_aligned_spacing_valid);
  DIFF_OUT_FIELD(_flags._horizontal);
  DIFF_OUT_FIELD(_flags._vertical);
  DIFF_OUT_FIELD(_flags._non_opposite_enclosure_spacing_valid);
  DIFF_OUT_FIELD(_flags._opposite_enclosure_resize_spacing_valid);
  DIFF_OUT_FIELD(_default);
  DIFF_OUT_FIELD(_second_layer_name);
  DIFF_OUT_FIELD(_prl);
  DIFF_OUT_FIELD(_extension);

  // User Code Begin out
  // User Code End out
  DIFF_END
}
_dbTechLayerCutSpacingTableDefSubRule::_dbTechLayerCutSpacingTableDefSubRule(
    _dbDatabase* db)
{
  uint* _flags_bit_field = (uint*) &_flags;
  *_flags_bit_field      = 0;
  // User Code Begin constructor
  // User Code End constructor
}
_dbTechLayerCutSpacingTableDefSubRule::_dbTechLayerCutSpacingTableDefSubRule(
    _dbDatabase*                                 db,
    const _dbTechLayerCutSpacingTableDefSubRule& r)
{
  _flags._default_valid               = r._flags._default_valid;
  _flags._same_mask                   = r._flags._same_mask;
  _flags._same_net                    = r._flags._same_net;
  _flags._same_metal                  = r._flags._same_metal;
  _flags._same_via                    = r._flags._same_via;
  _flags._layer_valid                 = r._flags._layer_valid;
  _flags._no_stack                    = r._flags._no_stack;
  _flags._non_zero_enclosure          = r._flags._non_zero_enclosure;
  _flags._prl_for_aligned_cut         = r._flags._prl_for_aligned_cut;
  _flags._center_to_center_valid      = r._flags._center_to_center_valid;
  _flags._center_and_edge_valid       = r._flags._center_and_edge_valid;
  _flags._no_prl                      = r._flags._no_prl;
  _flags._prl_valid                   = r._flags._prl_valid;
  _flags._max_x_y                     = r._flags._max_x_y;
  _flags._end_extension_valid         = r._flags._end_extension_valid;
  _flags._side_extension_valid        = r._flags._side_extension_valid;
  _flags._exact_aligned_spacing_valid = r._flags._exact_aligned_spacing_valid;
  _flags._horizontal                  = r._flags._horizontal;
  _flags._vertical                    = r._flags._vertical;
  _flags._non_opposite_enclosure_spacing_valid
      = r._flags._non_opposite_enclosure_spacing_valid;
  _flags._opposite_enclosure_resize_spacing_valid
      = r._flags._opposite_enclosure_resize_spacing_valid;
  _flags._spare_bits = r._flags._spare_bits;
  _default           = r._default;
  _second_layer_name = r._second_layer_name;
  _prl               = r._prl;
  _extension         = r._extension;
  // User Code Begin CopyConstructor
  // User Code End CopyConstructor
}

dbIStream& operator>>(dbIStream&                             stream,
                      _dbTechLayerCutSpacingTableDefSubRule& obj)
{
  uint* _flags_bit_field = (uint*) &obj._flags;
  stream >> *_flags_bit_field;
  stream >> obj._default;
  stream >> obj._second_layer_name;
  stream >> obj._prl_for_aligned_cut_tbl;
  stream >> obj._center_to_center_tbl;
  stream >> obj._center_and_edge_tbl;
  stream >> obj._prl;
  stream >> obj._prl_tbl;
  stream >> obj._extension;
  stream >> obj._end_extension_tbl;
  stream >> obj._side_extension_tbl;
  stream >> obj._exact_aligned_spacing_tbl;
  stream >> obj._non_opp_enc_spacing_tbl;
  stream >> obj._opp_enc_spacing_tbl;
  stream >> obj._spacing_tbl;
  stream >> obj._row_map;
  stream >> obj._col_map;
  // User Code Begin >>
  // User Code End >>
  return stream;
}
dbOStream& operator<<(dbOStream&                                   stream,
                      const _dbTechLayerCutSpacingTableDefSubRule& obj)
{
  uint* _flags_bit_field = (uint*) &obj._flags;
  stream << *_flags_bit_field;
  stream << obj._default;
  stream << obj._second_layer_name;
  stream << obj._prl_for_aligned_cut_tbl;
  stream << obj._center_to_center_tbl;
  stream << obj._center_and_edge_tbl;
  stream << obj._prl;
  stream << obj._prl_tbl;
  stream << obj._extension;
  stream << obj._end_extension_tbl;
  stream << obj._side_extension_tbl;
  stream << obj._exact_aligned_spacing_tbl;
  stream << obj._non_opp_enc_spacing_tbl;
  stream << obj._opp_enc_spacing_tbl;
  stream << obj._spacing_tbl;
  stream << obj._row_map;
  stream << obj._col_map;
  // User Code Begin <<
  // User Code End <<
  return stream;
}

_dbTechLayerCutSpacingTableDefSubRule::~_dbTechLayerCutSpacingTableDefSubRule()
{
  // User Code Begin Destructor
  // User Code End Destructor
}
////////////////////////////////////////////////////////////////////
//
// dbTechLayerCutSpacingTableDefSubRule - Methods
//
////////////////////////////////////////////////////////////////////

void dbTechLayerCutSpacingTableDefSubRule::setDefault(int _default)
{
  _dbTechLayerCutSpacingTableDefSubRule* obj
      = (_dbTechLayerCutSpacingTableDefSubRule*) this;

  obj->_default = _default;
}

int dbTechLayerCutSpacingTableDefSubRule::getDefault() const
{
  _dbTechLayerCutSpacingTableDefSubRule* obj
      = (_dbTechLayerCutSpacingTableDefSubRule*) this;
  return obj->_default;
}

char* dbTechLayerCutSpacingTableDefSubRule::getSecondLayerName() const
{
  _dbTechLayerCutSpacingTableDefSubRule* obj
      = (_dbTechLayerCutSpacingTableDefSubRule*) this;
  return obj->_second_layer_name;
}

void dbTechLayerCutSpacingTableDefSubRule::setPrlForAlignedCutTable(
    std::vector<std::pair<char*, char*>> _prl_for_aligned_cut_tbl)
{
  _dbTechLayerCutSpacingTableDefSubRule* obj
      = (_dbTechLayerCutSpacingTableDefSubRule*) this;

  obj->_prl_for_aligned_cut_tbl = _prl_for_aligned_cut_tbl;
}

void dbTechLayerCutSpacingTableDefSubRule::getPrlForAlignedCutTable(
    std::vector<std::pair<char*, char*>>& tbl) const
{
  _dbTechLayerCutSpacingTableDefSubRule* obj
      = (_dbTechLayerCutSpacingTableDefSubRule*) this;
  tbl = obj->_prl_for_aligned_cut_tbl;
}

void dbTechLayerCutSpacingTableDefSubRule::setCenterToCenterTable(
    std::vector<std::pair<char*, char*>> _center_to_center_tbl)
{
  _dbTechLayerCutSpacingTableDefSubRule* obj
      = (_dbTechLayerCutSpacingTableDefSubRule*) this;

  obj->_center_to_center_tbl = _center_to_center_tbl;
}

void dbTechLayerCutSpacingTableDefSubRule::getCenterToCenterTable(
    std::vector<std::pair<char*, char*>>& tbl) const
{
  _dbTechLayerCutSpacingTableDefSubRule* obj
      = (_dbTechLayerCutSpacingTableDefSubRule*) this;
  tbl = obj->_center_to_center_tbl;
}

void dbTechLayerCutSpacingTableDefSubRule::setCenterAndEdgeTable(
    std::vector<std::pair<char*, char*>> _center_and_edge_tbl)
{
  _dbTechLayerCutSpacingTableDefSubRule* obj
      = (_dbTechLayerCutSpacingTableDefSubRule*) this;

  obj->_center_and_edge_tbl = _center_and_edge_tbl;
}

void dbTechLayerCutSpacingTableDefSubRule::getCenterAndEdgeTable(
    std::vector<std::pair<char*, char*>>& tbl) const
{
  _dbTechLayerCutSpacingTableDefSubRule* obj
      = (_dbTechLayerCutSpacingTableDefSubRule*) this;
  tbl = obj->_center_and_edge_tbl;
}

void dbTechLayerCutSpacingTableDefSubRule::setPrl(int _prl)
{
  _dbTechLayerCutSpacingTableDefSubRule* obj
      = (_dbTechLayerCutSpacingTableDefSubRule*) this;

  obj->_prl = _prl;
}

int dbTechLayerCutSpacingTableDefSubRule::getPrl() const
{
  _dbTechLayerCutSpacingTableDefSubRule* obj
      = (_dbTechLayerCutSpacingTableDefSubRule*) this;
  return obj->_prl;
}

void dbTechLayerCutSpacingTableDefSubRule::setPrlTable(
    std::vector<std::tuple<char*, char*, int>> _prl_tbl)
{
  _dbTechLayerCutSpacingTableDefSubRule* obj
      = (_dbTechLayerCutSpacingTableDefSubRule*) this;

  obj->_prl_tbl = _prl_tbl;
}

void dbTechLayerCutSpacingTableDefSubRule::getPrlTable(
    std::vector<std::tuple<char*, char*, int>>& tbl) const
{
  _dbTechLayerCutSpacingTableDefSubRule* obj
      = (_dbTechLayerCutSpacingTableDefSubRule*) this;
  tbl = obj->_prl_tbl;
}

void dbTechLayerCutSpacingTableDefSubRule::setExtension(int _extension)
{
  _dbTechLayerCutSpacingTableDefSubRule* obj
      = (_dbTechLayerCutSpacingTableDefSubRule*) this;

  obj->_extension = _extension;
}

int dbTechLayerCutSpacingTableDefSubRule::getExtension() const
{
  _dbTechLayerCutSpacingTableDefSubRule* obj
      = (_dbTechLayerCutSpacingTableDefSubRule*) this;
  return obj->_extension;
}

void dbTechLayerCutSpacingTableDefSubRule::setEndExtensionTable(
    std::vector<std::pair<char*, int>> _end_extension_tbl)
{
  _dbTechLayerCutSpacingTableDefSubRule* obj
      = (_dbTechLayerCutSpacingTableDefSubRule*) this;

  obj->_end_extension_tbl = _end_extension_tbl;
}

void dbTechLayerCutSpacingTableDefSubRule::getEndExtensionTable(
    std::vector<std::pair<char*, int>>& tbl) const
{
  _dbTechLayerCutSpacingTableDefSubRule* obj
      = (_dbTechLayerCutSpacingTableDefSubRule*) this;
  tbl = obj->_end_extension_tbl;
}

void dbTechLayerCutSpacingTableDefSubRule::setSideExtensionTable(
    std::vector<std::pair<char*, int>> _side_extension_tbl)
{
  _dbTechLayerCutSpacingTableDefSubRule* obj
      = (_dbTechLayerCutSpacingTableDefSubRule*) this;

  obj->_side_extension_tbl = _side_extension_tbl;
}

void dbTechLayerCutSpacingTableDefSubRule::getSideExtensionTable(
    std::vector<std::pair<char*, int>>& tbl) const
{
  _dbTechLayerCutSpacingTableDefSubRule* obj
      = (_dbTechLayerCutSpacingTableDefSubRule*) this;
  tbl = obj->_side_extension_tbl;
}

void dbTechLayerCutSpacingTableDefSubRule::setExactAlignedSpacingTable(
    std::vector<std::pair<char*, int>> _exact_aligned_spacing_tbl)
{
  _dbTechLayerCutSpacingTableDefSubRule* obj
      = (_dbTechLayerCutSpacingTableDefSubRule*) this;

  obj->_exact_aligned_spacing_tbl = _exact_aligned_spacing_tbl;
}

void dbTechLayerCutSpacingTableDefSubRule::getExactAlignedSpacingTable(
    std::vector<std::pair<char*, int>>& tbl) const
{
  _dbTechLayerCutSpacingTableDefSubRule* obj
      = (_dbTechLayerCutSpacingTableDefSubRule*) this;
  tbl = obj->_exact_aligned_spacing_tbl;
}

void dbTechLayerCutSpacingTableDefSubRule::setNonOppEncSpacingTable(
    std::vector<std::pair<char*, int>> _non_opp_enc_spacing_tbl)
{
  _dbTechLayerCutSpacingTableDefSubRule* obj
      = (_dbTechLayerCutSpacingTableDefSubRule*) this;

  obj->_non_opp_enc_spacing_tbl = _non_opp_enc_spacing_tbl;
}

void dbTechLayerCutSpacingTableDefSubRule::getNonOppEncSpacingTable(
    std::vector<std::pair<char*, int>>& tbl) const
{
  _dbTechLayerCutSpacingTableDefSubRule* obj
      = (_dbTechLayerCutSpacingTableDefSubRule*) this;
  tbl = obj->_non_opp_enc_spacing_tbl;
}

void dbTechLayerCutSpacingTableDefSubRule::setOppEncSpacingTable(
    std::vector<std::tuple<char*, int, int, int>> _opp_enc_spacing_tbl)
{
  _dbTechLayerCutSpacingTableDefSubRule* obj
      = (_dbTechLayerCutSpacingTableDefSubRule*) this;

  obj->_opp_enc_spacing_tbl = _opp_enc_spacing_tbl;
}

void dbTechLayerCutSpacingTableDefSubRule::getOppEncSpacingTable(
    std::vector<std::tuple<char*, int, int, int>>& tbl) const
{
  _dbTechLayerCutSpacingTableDefSubRule* obj
      = (_dbTechLayerCutSpacingTableDefSubRule*) this;
  tbl = obj->_opp_enc_spacing_tbl;
}

void dbTechLayerCutSpacingTableDefSubRule::setDefaultValid(bool _default_valid)
{
  _dbTechLayerCutSpacingTableDefSubRule* obj
      = (_dbTechLayerCutSpacingTableDefSubRule*) this;

  obj->_flags._default_valid = _default_valid;
}

bool dbTechLayerCutSpacingTableDefSubRule::isDefaultValid() const
{
  _dbTechLayerCutSpacingTableDefSubRule* obj
      = (_dbTechLayerCutSpacingTableDefSubRule*) this;

  return obj->_flags._default_valid;
}

void dbTechLayerCutSpacingTableDefSubRule::setSameMask(bool _same_mask)
{
  _dbTechLayerCutSpacingTableDefSubRule* obj
      = (_dbTechLayerCutSpacingTableDefSubRule*) this;

  obj->_flags._same_mask = _same_mask;
}

bool dbTechLayerCutSpacingTableDefSubRule::isSameMask() const
{
  _dbTechLayerCutSpacingTableDefSubRule* obj
      = (_dbTechLayerCutSpacingTableDefSubRule*) this;

  return obj->_flags._same_mask;
}

void dbTechLayerCutSpacingTableDefSubRule::setSameNet(bool _same_net)
{
  _dbTechLayerCutSpacingTableDefSubRule* obj
      = (_dbTechLayerCutSpacingTableDefSubRule*) this;

  obj->_flags._same_net = _same_net;
}

bool dbTechLayerCutSpacingTableDefSubRule::isSameNet() const
{
  _dbTechLayerCutSpacingTableDefSubRule* obj
      = (_dbTechLayerCutSpacingTableDefSubRule*) this;

  return obj->_flags._same_net;
}

void dbTechLayerCutSpacingTableDefSubRule::setSameMetal(bool _same_metal)
{
  _dbTechLayerCutSpacingTableDefSubRule* obj
      = (_dbTechLayerCutSpacingTableDefSubRule*) this;

  obj->_flags._same_metal = _same_metal;
}

bool dbTechLayerCutSpacingTableDefSubRule::isSameMetal() const
{
  _dbTechLayerCutSpacingTableDefSubRule* obj
      = (_dbTechLayerCutSpacingTableDefSubRule*) this;

  return obj->_flags._same_metal;
}

void dbTechLayerCutSpacingTableDefSubRule::setSameVia(bool _same_via)
{
  _dbTechLayerCutSpacingTableDefSubRule* obj
      = (_dbTechLayerCutSpacingTableDefSubRule*) this;

  obj->_flags._same_via = _same_via;
}

bool dbTechLayerCutSpacingTableDefSubRule::isSameVia() const
{
  _dbTechLayerCutSpacingTableDefSubRule* obj
      = (_dbTechLayerCutSpacingTableDefSubRule*) this;

  return obj->_flags._same_via;
}

void dbTechLayerCutSpacingTableDefSubRule::setLayerValid(bool _layer_valid)
{
  _dbTechLayerCutSpacingTableDefSubRule* obj
      = (_dbTechLayerCutSpacingTableDefSubRule*) this;

  obj->_flags._layer_valid = _layer_valid;
}

bool dbTechLayerCutSpacingTableDefSubRule::isLayerValid() const
{
  _dbTechLayerCutSpacingTableDefSubRule* obj
      = (_dbTechLayerCutSpacingTableDefSubRule*) this;

  return obj->_flags._layer_valid;
}

void dbTechLayerCutSpacingTableDefSubRule::setNoStack(bool _no_stack)
{
  _dbTechLayerCutSpacingTableDefSubRule* obj
      = (_dbTechLayerCutSpacingTableDefSubRule*) this;

  obj->_flags._no_stack = _no_stack;
}

bool dbTechLayerCutSpacingTableDefSubRule::isNoStack() const
{
  _dbTechLayerCutSpacingTableDefSubRule* obj
      = (_dbTechLayerCutSpacingTableDefSubRule*) this;

  return obj->_flags._no_stack;
}

void dbTechLayerCutSpacingTableDefSubRule::setNonZeroEnclosure(
    bool _non_zero_enclosure)
{
  _dbTechLayerCutSpacingTableDefSubRule* obj
      = (_dbTechLayerCutSpacingTableDefSubRule*) this;

  obj->_flags._non_zero_enclosure = _non_zero_enclosure;
}

bool dbTechLayerCutSpacingTableDefSubRule::isNonZeroEnclosure() const
{
  _dbTechLayerCutSpacingTableDefSubRule* obj
      = (_dbTechLayerCutSpacingTableDefSubRule*) this;

  return obj->_flags._non_zero_enclosure;
}

void dbTechLayerCutSpacingTableDefSubRule::setPrlForAlignedCut(
    bool _prl_for_aligned_cut)
{
  _dbTechLayerCutSpacingTableDefSubRule* obj
      = (_dbTechLayerCutSpacingTableDefSubRule*) this;

  obj->_flags._prl_for_aligned_cut = _prl_for_aligned_cut;
}

bool dbTechLayerCutSpacingTableDefSubRule::isPrlForAlignedCut() const
{
  _dbTechLayerCutSpacingTableDefSubRule* obj
      = (_dbTechLayerCutSpacingTableDefSubRule*) this;

  return obj->_flags._prl_for_aligned_cut;
}

void dbTechLayerCutSpacingTableDefSubRule::setCenterToCenterValid(
    bool _center_to_center_valid)
{
  _dbTechLayerCutSpacingTableDefSubRule* obj
      = (_dbTechLayerCutSpacingTableDefSubRule*) this;

  obj->_flags._center_to_center_valid = _center_to_center_valid;
}

bool dbTechLayerCutSpacingTableDefSubRule::isCenterToCenterValid() const
{
  _dbTechLayerCutSpacingTableDefSubRule* obj
      = (_dbTechLayerCutSpacingTableDefSubRule*) this;

  return obj->_flags._center_to_center_valid;
}

void dbTechLayerCutSpacingTableDefSubRule::setCenterAndEdgeValid(
    bool _center_and_edge_valid)
{
  _dbTechLayerCutSpacingTableDefSubRule* obj
      = (_dbTechLayerCutSpacingTableDefSubRule*) this;

  obj->_flags._center_and_edge_valid = _center_and_edge_valid;
}

bool dbTechLayerCutSpacingTableDefSubRule::isCenterAndEdgeValid() const
{
  _dbTechLayerCutSpacingTableDefSubRule* obj
      = (_dbTechLayerCutSpacingTableDefSubRule*) this;

  return obj->_flags._center_and_edge_valid;
}

void dbTechLayerCutSpacingTableDefSubRule::setNoPrl(bool _no_prl)
{
  _dbTechLayerCutSpacingTableDefSubRule* obj
      = (_dbTechLayerCutSpacingTableDefSubRule*) this;

  obj->_flags._no_prl = _no_prl;
}

bool dbTechLayerCutSpacingTableDefSubRule::isNoPrl() const
{
  _dbTechLayerCutSpacingTableDefSubRule* obj
      = (_dbTechLayerCutSpacingTableDefSubRule*) this;

  return obj->_flags._no_prl;
}

void dbTechLayerCutSpacingTableDefSubRule::setPrlValid(bool _prl_valid)
{
  _dbTechLayerCutSpacingTableDefSubRule* obj
      = (_dbTechLayerCutSpacingTableDefSubRule*) this;

  obj->_flags._prl_valid = _prl_valid;
}

bool dbTechLayerCutSpacingTableDefSubRule::isPrlValid() const
{
  _dbTechLayerCutSpacingTableDefSubRule* obj
      = (_dbTechLayerCutSpacingTableDefSubRule*) this;

  return obj->_flags._prl_valid;
}

void dbTechLayerCutSpacingTableDefSubRule::setMaxXY(bool _max_x_y)
{
  _dbTechLayerCutSpacingTableDefSubRule* obj
      = (_dbTechLayerCutSpacingTableDefSubRule*) this;

  obj->_flags._max_x_y = _max_x_y;
}

bool dbTechLayerCutSpacingTableDefSubRule::isMaxXY() const
{
  _dbTechLayerCutSpacingTableDefSubRule* obj
      = (_dbTechLayerCutSpacingTableDefSubRule*) this;

  return obj->_flags._max_x_y;
}

void dbTechLayerCutSpacingTableDefSubRule::setEndExtensionValid(
    bool _end_extension_valid)
{
  _dbTechLayerCutSpacingTableDefSubRule* obj
      = (_dbTechLayerCutSpacingTableDefSubRule*) this;

  obj->_flags._end_extension_valid = _end_extension_valid;
}

bool dbTechLayerCutSpacingTableDefSubRule::isEndExtensionValid() const
{
  _dbTechLayerCutSpacingTableDefSubRule* obj
      = (_dbTechLayerCutSpacingTableDefSubRule*) this;

  return obj->_flags._end_extension_valid;
}

void dbTechLayerCutSpacingTableDefSubRule::setSideExtensionValid(
    bool _side_extension_valid)
{
  _dbTechLayerCutSpacingTableDefSubRule* obj
      = (_dbTechLayerCutSpacingTableDefSubRule*) this;

  obj->_flags._side_extension_valid = _side_extension_valid;
}

bool dbTechLayerCutSpacingTableDefSubRule::isSideExtensionValid() const
{
  _dbTechLayerCutSpacingTableDefSubRule* obj
      = (_dbTechLayerCutSpacingTableDefSubRule*) this;

  return obj->_flags._side_extension_valid;
}

void dbTechLayerCutSpacingTableDefSubRule::setExactAlignedSpacingValid(
    bool _exact_aligned_spacing_valid)
{
  _dbTechLayerCutSpacingTableDefSubRule* obj
      = (_dbTechLayerCutSpacingTableDefSubRule*) this;

  obj->_flags._exact_aligned_spacing_valid = _exact_aligned_spacing_valid;
}

bool dbTechLayerCutSpacingTableDefSubRule::isExactAlignedSpacingValid() const
{
  _dbTechLayerCutSpacingTableDefSubRule* obj
      = (_dbTechLayerCutSpacingTableDefSubRule*) this;

  return obj->_flags._exact_aligned_spacing_valid;
}

void dbTechLayerCutSpacingTableDefSubRule::setHorizontal(bool _horizontal)
{
  _dbTechLayerCutSpacingTableDefSubRule* obj
      = (_dbTechLayerCutSpacingTableDefSubRule*) this;

  obj->_flags._horizontal = _horizontal;
}

bool dbTechLayerCutSpacingTableDefSubRule::isHorizontal() const
{
  _dbTechLayerCutSpacingTableDefSubRule* obj
      = (_dbTechLayerCutSpacingTableDefSubRule*) this;

  return obj->_flags._horizontal;
}

void dbTechLayerCutSpacingTableDefSubRule::setVertical(bool _vertical)
{
  _dbTechLayerCutSpacingTableDefSubRule* obj
      = (_dbTechLayerCutSpacingTableDefSubRule*) this;

  obj->_flags._vertical = _vertical;
}

bool dbTechLayerCutSpacingTableDefSubRule::isVertical() const
{
  _dbTechLayerCutSpacingTableDefSubRule* obj
      = (_dbTechLayerCutSpacingTableDefSubRule*) this;

  return obj->_flags._vertical;
}

void dbTechLayerCutSpacingTableDefSubRule::setNonOppositeEnclosureSpacingValid(
    bool _non_opposite_enclosure_spacing_valid)
{
  _dbTechLayerCutSpacingTableDefSubRule* obj
      = (_dbTechLayerCutSpacingTableDefSubRule*) this;

  obj->_flags._non_opposite_enclosure_spacing_valid
      = _non_opposite_enclosure_spacing_valid;
}

bool dbTechLayerCutSpacingTableDefSubRule::isNonOppositeEnclosureSpacingValid()
    const
{
  _dbTechLayerCutSpacingTableDefSubRule* obj
      = (_dbTechLayerCutSpacingTableDefSubRule*) this;

  return obj->_flags._non_opposite_enclosure_spacing_valid;
}

void dbTechLayerCutSpacingTableDefSubRule::
    setOppositeEnclosureResizeSpacingValid(
        bool _opposite_enclosure_resize_spacing_valid)
{
  _dbTechLayerCutSpacingTableDefSubRule* obj
      = (_dbTechLayerCutSpacingTableDefSubRule*) this;

  obj->_flags._opposite_enclosure_resize_spacing_valid
      = _opposite_enclosure_resize_spacing_valid;
}

bool dbTechLayerCutSpacingTableDefSubRule::
    isOppositeEnclosureResizeSpacingValid() const
{
  _dbTechLayerCutSpacingTableDefSubRule* obj
      = (_dbTechLayerCutSpacingTableDefSubRule*) this;

  return obj->_flags._opposite_enclosure_resize_spacing_valid;
}

// User Code Begin dbTechLayerCutSpacingTableDefSubRulePublicMethods
void dbTechLayerCutSpacingTableDefSubRule::setSecondLayerName(const char* name)
{
  _dbTechLayerCutSpacingTableDefSubRule* obj
      = (_dbTechLayerCutSpacingTableDefSubRule*) this;
  obj->_second_layer_name = strdup(name);
}
void dbTechLayerCutSpacingTableDefSubRule::addCenterToCenter(const char* from,
                                                             const char* to)
{
  _dbTechLayerCutSpacingTableDefSubRule* obj
      = (_dbTechLayerCutSpacingTableDefSubRule*) this;
  obj->_center_to_center_tbl.push_back({strdup(from), strdup(to)});
}

void dbTechLayerCutSpacingTableDefSubRule::addCenterAndEdge(const char* from,
                                                            const char* to)
{
  _dbTechLayerCutSpacingTableDefSubRule* obj
      = (_dbTechLayerCutSpacingTableDefSubRule*) this;
  obj->_center_and_edge_tbl.push_back({strdup(from), strdup(to)});
}

void dbTechLayerCutSpacingTableDefSubRule::addPrl(const char* from,
                                                  const char* to,
                                                  int         ccPrl)
{
  _dbTechLayerCutSpacingTableDefSubRule* obj
      = (_dbTechLayerCutSpacingTableDefSubRule*) this;
  obj->_prl_tbl.push_back({strdup(from), strdup(to), ccPrl});
}

void dbTechLayerCutSpacingTableDefSubRule::addEndExtension(
    const char* class_name,
    int         extension)
{
  _dbTechLayerCutSpacingTableDefSubRule* obj
      = (_dbTechLayerCutSpacingTableDefSubRule*) this;
  obj->_end_extension_tbl.push_back({strdup(class_name), extension});
}

void dbTechLayerCutSpacingTableDefSubRule::addSideExtension(
    const char* class_name,
    int         extension)
{
  _dbTechLayerCutSpacingTableDefSubRule* obj
      = (_dbTechLayerCutSpacingTableDefSubRule*) this;
  obj->_side_extension_tbl.push_back({strdup(class_name), extension});
}

void dbTechLayerCutSpacingTableDefSubRule::addExactAlignedSpacing(
    const char* class_name,
    int         spacing)
{
  _dbTechLayerCutSpacingTableDefSubRule* obj
      = (_dbTechLayerCutSpacingTableDefSubRule*) this;
  obj->_exact_aligned_spacing_tbl.push_back({strdup(class_name), spacing});
}

void dbTechLayerCutSpacingTableDefSubRule::addNonOppositeEnclosureSpacing(
    const char* class_name,
    int         spacing)
{
  _dbTechLayerCutSpacingTableDefSubRule* obj
      = (_dbTechLayerCutSpacingTableDefSubRule*) this;
  obj->_non_opp_enc_spacing_tbl.push_back({strdup(class_name), spacing});
}

void dbTechLayerCutSpacingTableDefSubRule::addOppositeEnclosureResizeSpacing(
    const char* class_name,
    int         resize1,
    int         resize2,
    int         spacing)
{
  _dbTechLayerCutSpacingTableDefSubRule* obj
      = (_dbTechLayerCutSpacingTableDefSubRule*) this;
  obj->_opp_enc_spacing_tbl.push_back(
      {strdup(class_name), resize1, resize2, spacing});
}

void dbTechLayerCutSpacingTableDefSubRule::setSpacingTable(
    std::vector<std::vector<std::pair<int, int>>> table,
    std::map<std::string, uint>                   row_map,
    std::map<std::string, uint>                   col_map)
{
  _dbTechLayerCutSpacingTableDefSubRule* obj
      = (_dbTechLayerCutSpacingTableDefSubRule*) this;
  obj->_row_map = row_map;
  obj->_col_map = col_map;
  for (auto spacing : table) {
    dbVector<std::pair<int, int>> tmp;
    tmp = spacing;
    obj->_spacing_tbl.push_back(tmp);
  }
}

void dbTechLayerCutSpacingTableDefSubRule::getSpacingTable(
    std::vector<std::vector<std::pair<int, int>>>& table,
    std::map<std::string, uint>&                   row_map,
    std::map<std::string, uint>&                   col_map)
{
  _dbTechLayerCutSpacingTableDefSubRule* obj
      = (_dbTechLayerCutSpacingTableDefSubRule*) this;
  row_map = obj->_row_map;
  col_map = obj->_col_map;
  table.clear();
  for (auto spacing : obj->_spacing_tbl) {
    table.push_back(spacing);
  }
}

std::pair<int, int> dbTechLayerCutSpacingTableDefSubRule::getSpacing(
    const char* class1,
    bool        SIDE1,
    const char* class2,
    bool        SIDE2)
{
  _dbTechLayerCutSpacingTableDefSubRule* obj
      = (_dbTechLayerCutSpacingTableDefSubRule*) this;
  std::string c1 = class1;
  std::string c2 = class2;
  if (SIDE1)
    c1 += "/SIDE";
  else
    c1 += "/END";

  if (SIDE2)
    c2 += "/SIDE";
  else
    c2 += "/END";

  if (obj->_row_map.find(c1) != obj->_row_map.end()
      && obj->_col_map.find(c2) != obj->_col_map.end())
    return obj->_spacing_tbl[obj->_row_map[c1]][obj->_col_map[c2]];
  else if (obj->_row_map.find(c2) != obj->_row_map.end()
           && obj->_col_map.find(c1) != obj->_col_map.end())
    return obj->_spacing_tbl[obj->_row_map[c2]][obj->_col_map[c1]];
  else
    return {obj->_default, obj->_default};
}

dbTechLayerCutSpacingTableDefSubRule*
dbTechLayerCutSpacingTableDefSubRule::create(
    dbTechLayerCutSpacingTableRule* parent)
{
  _dbTechLayerCutSpacingTableRule* _parent
      = (_dbTechLayerCutSpacingTableRule*) parent;
  _dbTechLayerCutSpacingTableDefSubRule* newrule
      = _parent->_techlayercutspacingtabledefsubrule_tbl->create();
  return ((dbTechLayerCutSpacingTableDefSubRule*) newrule);
}

dbTechLayerCutSpacingTableDefSubRule*
dbTechLayerCutSpacingTableDefSubRule::getTechLayerCutSpacingTableDefSubRule(
    dbTechLayerCutSpacingTableRule* parent,
    uint                            dbid)
{
  _dbTechLayerCutSpacingTableRule* _parent
      = (_dbTechLayerCutSpacingTableRule*) parent;
  return (dbTechLayerCutSpacingTableDefSubRule*)
      _parent->_techlayercutspacingtabledefsubrule_tbl->getPtr(dbid);
}
void dbTechLayerCutSpacingTableDefSubRule::destroy(
    dbTechLayerCutSpacingTableDefSubRule* rule)
{
  _dbTechLayerCutSpacingTableRule* _parent
      = (_dbTechLayerCutSpacingTableRule*) rule->getImpl()->getOwner();
  dbProperty::destroyProperties(rule);
  _parent->_techlayercutspacingtabledefsubrule_tbl->destroy(
      (_dbTechLayerCutSpacingTableDefSubRule*) rule);
}
// User Code End dbTechLayerCutSpacingTableDefSubRulePublicMethods
}  // namespace odb
   // Generator Code End 1