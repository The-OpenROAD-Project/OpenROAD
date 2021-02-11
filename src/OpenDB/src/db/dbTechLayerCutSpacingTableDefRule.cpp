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
#include "dbTechLayerCutSpacingTableDefRule.h"

#include "db.h"
#include "dbDatabase.h"
#include "dbDiff.hpp"
#include "dbTable.h"
#include "dbTable.hpp"
#include "dbTechLayer.h"
// User Code Begin includes
#include "dbTech.h"
#include "dbTechLayerCutClassRule.h"
// User Code End includes
namespace odb {

// User Code Begin definitions
// User Code End definitions
template class dbTable<_dbTechLayerCutSpacingTableDefRule>;

bool _dbTechLayerCutSpacingTableDefRule::operator==(
    const _dbTechLayerCutSpacingTableDefRule& rhs) const
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

  if (_second_layer != rhs._second_layer)
    return false;

  if (_prl != rhs._prl)
    return false;

  if (_extension != rhs._extension)
    return false;

  // User Code Begin ==
  // User Code End ==
  return true;
}
bool _dbTechLayerCutSpacingTableDefRule::operator<(
    const _dbTechLayerCutSpacingTableDefRule& rhs) const
{
  // User Code Begin <
  // User Code End <
  return true;
}
void _dbTechLayerCutSpacingTableDefRule::differences(
    dbDiff&                                   diff,
    const char*                               field,
    const _dbTechLayerCutSpacingTableDefRule& rhs) const
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
  DIFF_FIELD(_second_layer);
  DIFF_FIELD(_prl);
  DIFF_FIELD(_extension);
  // User Code Begin differences
  // User Code End differences
  DIFF_END
}
void _dbTechLayerCutSpacingTableDefRule::out(dbDiff&     diff,
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
  DIFF_OUT_FIELD(_second_layer);
  DIFF_OUT_FIELD(_prl);
  DIFF_OUT_FIELD(_extension);

  // User Code Begin out
  // User Code End out
  DIFF_END
}
_dbTechLayerCutSpacingTableDefRule::_dbTechLayerCutSpacingTableDefRule(
    _dbDatabase* db)
{
  uint32_t* _flags_bit_field = (uint32_t*) &_flags;
  *_flags_bit_field          = 0;
  // User Code Begin constructor
  // User Code End constructor
}
_dbTechLayerCutSpacingTableDefRule::_dbTechLayerCutSpacingTableDefRule(
    _dbDatabase*                              db,
    const _dbTechLayerCutSpacingTableDefRule& r)
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
  _second_layer      = r._second_layer;
  _prl               = r._prl;
  _extension         = r._extension;
  // User Code Begin CopyConstructor
  // User Code End CopyConstructor
}

dbIStream& operator>>(dbIStream&                          stream,
                      _dbTechLayerCutSpacingTableDefRule& obj)
{
  uint32_t* _flags_bit_field = (uint32_t*) &obj._flags;
  stream >> *_flags_bit_field;
  stream >> obj._default;
  stream >> obj._second_layer;
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
dbOStream& operator<<(dbOStream&                                stream,
                      const _dbTechLayerCutSpacingTableDefRule& obj)
{
  uint32_t* _flags_bit_field = (uint32_t*) &obj._flags;
  stream << *_flags_bit_field;
  stream << obj._default;
  stream << obj._second_layer;
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

_dbTechLayerCutSpacingTableDefRule::~_dbTechLayerCutSpacingTableDefRule()
{
  // User Code Begin Destructor
  // User Code End Destructor
}
////////////////////////////////////////////////////////////////////
//
// dbTechLayerCutSpacingTableDefRule - Methods
//
////////////////////////////////////////////////////////////////////

void dbTechLayerCutSpacingTableDefRule::setDefault(int _default)
{
  _dbTechLayerCutSpacingTableDefRule* obj
      = (_dbTechLayerCutSpacingTableDefRule*) this;

  obj->_default = _default;
}

int dbTechLayerCutSpacingTableDefRule::getDefault() const
{
  _dbTechLayerCutSpacingTableDefRule* obj
      = (_dbTechLayerCutSpacingTableDefRule*) this;
  return obj->_default;
}

void dbTechLayerCutSpacingTableDefRule::setSecondLayer(
    dbTechLayer* _second_layer)
{
  _dbTechLayerCutSpacingTableDefRule* obj
      = (_dbTechLayerCutSpacingTableDefRule*) this;

  obj->_second_layer = _second_layer->getImpl()->getOID();
}

void dbTechLayerCutSpacingTableDefRule::setPrl(int _prl)
{
  _dbTechLayerCutSpacingTableDefRule* obj
      = (_dbTechLayerCutSpacingTableDefRule*) this;

  obj->_prl = _prl;
}

int dbTechLayerCutSpacingTableDefRule::getPrl() const
{
  _dbTechLayerCutSpacingTableDefRule* obj
      = (_dbTechLayerCutSpacingTableDefRule*) this;
  return obj->_prl;
}

void dbTechLayerCutSpacingTableDefRule::setExtension(int _extension)
{
  _dbTechLayerCutSpacingTableDefRule* obj
      = (_dbTechLayerCutSpacingTableDefRule*) this;

  obj->_extension = _extension;
}

int dbTechLayerCutSpacingTableDefRule::getExtension() const
{
  _dbTechLayerCutSpacingTableDefRule* obj
      = (_dbTechLayerCutSpacingTableDefRule*) this;
  return obj->_extension;
}

void dbTechLayerCutSpacingTableDefRule::setDefaultValid(bool _default_valid)
{
  _dbTechLayerCutSpacingTableDefRule* obj
      = (_dbTechLayerCutSpacingTableDefRule*) this;

  obj->_flags._default_valid = _default_valid;
}

bool dbTechLayerCutSpacingTableDefRule::isDefaultValid() const
{
  _dbTechLayerCutSpacingTableDefRule* obj
      = (_dbTechLayerCutSpacingTableDefRule*) this;

  return obj->_flags._default_valid;
}

void dbTechLayerCutSpacingTableDefRule::setSameMask(bool _same_mask)
{
  _dbTechLayerCutSpacingTableDefRule* obj
      = (_dbTechLayerCutSpacingTableDefRule*) this;

  obj->_flags._same_mask = _same_mask;
}

bool dbTechLayerCutSpacingTableDefRule::isSameMask() const
{
  _dbTechLayerCutSpacingTableDefRule* obj
      = (_dbTechLayerCutSpacingTableDefRule*) this;

  return obj->_flags._same_mask;
}

void dbTechLayerCutSpacingTableDefRule::setSameNet(bool _same_net)
{
  _dbTechLayerCutSpacingTableDefRule* obj
      = (_dbTechLayerCutSpacingTableDefRule*) this;

  obj->_flags._same_net = _same_net;
}

bool dbTechLayerCutSpacingTableDefRule::isSameNet() const
{
  _dbTechLayerCutSpacingTableDefRule* obj
      = (_dbTechLayerCutSpacingTableDefRule*) this;

  return obj->_flags._same_net;
}

void dbTechLayerCutSpacingTableDefRule::setSameMetal(bool _same_metal)
{
  _dbTechLayerCutSpacingTableDefRule* obj
      = (_dbTechLayerCutSpacingTableDefRule*) this;

  obj->_flags._same_metal = _same_metal;
}

bool dbTechLayerCutSpacingTableDefRule::isSameMetal() const
{
  _dbTechLayerCutSpacingTableDefRule* obj
      = (_dbTechLayerCutSpacingTableDefRule*) this;

  return obj->_flags._same_metal;
}

void dbTechLayerCutSpacingTableDefRule::setSameVia(bool _same_via)
{
  _dbTechLayerCutSpacingTableDefRule* obj
      = (_dbTechLayerCutSpacingTableDefRule*) this;

  obj->_flags._same_via = _same_via;
}

bool dbTechLayerCutSpacingTableDefRule::isSameVia() const
{
  _dbTechLayerCutSpacingTableDefRule* obj
      = (_dbTechLayerCutSpacingTableDefRule*) this;

  return obj->_flags._same_via;
}

void dbTechLayerCutSpacingTableDefRule::setLayerValid(bool _layer_valid)
{
  _dbTechLayerCutSpacingTableDefRule* obj
      = (_dbTechLayerCutSpacingTableDefRule*) this;

  obj->_flags._layer_valid = _layer_valid;
}

bool dbTechLayerCutSpacingTableDefRule::isLayerValid() const
{
  _dbTechLayerCutSpacingTableDefRule* obj
      = (_dbTechLayerCutSpacingTableDefRule*) this;

  return obj->_flags._layer_valid;
}

void dbTechLayerCutSpacingTableDefRule::setNoStack(bool _no_stack)
{
  _dbTechLayerCutSpacingTableDefRule* obj
      = (_dbTechLayerCutSpacingTableDefRule*) this;

  obj->_flags._no_stack = _no_stack;
}

bool dbTechLayerCutSpacingTableDefRule::isNoStack() const
{
  _dbTechLayerCutSpacingTableDefRule* obj
      = (_dbTechLayerCutSpacingTableDefRule*) this;

  return obj->_flags._no_stack;
}

void dbTechLayerCutSpacingTableDefRule::setNonZeroEnclosure(
    bool _non_zero_enclosure)
{
  _dbTechLayerCutSpacingTableDefRule* obj
      = (_dbTechLayerCutSpacingTableDefRule*) this;

  obj->_flags._non_zero_enclosure = _non_zero_enclosure;
}

bool dbTechLayerCutSpacingTableDefRule::isNonZeroEnclosure() const
{
  _dbTechLayerCutSpacingTableDefRule* obj
      = (_dbTechLayerCutSpacingTableDefRule*) this;

  return obj->_flags._non_zero_enclosure;
}

void dbTechLayerCutSpacingTableDefRule::setPrlForAlignedCut(
    bool _prl_for_aligned_cut)
{
  _dbTechLayerCutSpacingTableDefRule* obj
      = (_dbTechLayerCutSpacingTableDefRule*) this;

  obj->_flags._prl_for_aligned_cut = _prl_for_aligned_cut;
}

bool dbTechLayerCutSpacingTableDefRule::isPrlForAlignedCut() const
{
  _dbTechLayerCutSpacingTableDefRule* obj
      = (_dbTechLayerCutSpacingTableDefRule*) this;

  return obj->_flags._prl_for_aligned_cut;
}

void dbTechLayerCutSpacingTableDefRule::setCenterToCenterValid(
    bool _center_to_center_valid)
{
  _dbTechLayerCutSpacingTableDefRule* obj
      = (_dbTechLayerCutSpacingTableDefRule*) this;

  obj->_flags._center_to_center_valid = _center_to_center_valid;
}

bool dbTechLayerCutSpacingTableDefRule::isCenterToCenterValid() const
{
  _dbTechLayerCutSpacingTableDefRule* obj
      = (_dbTechLayerCutSpacingTableDefRule*) this;

  return obj->_flags._center_to_center_valid;
}

void dbTechLayerCutSpacingTableDefRule::setCenterAndEdgeValid(
    bool _center_and_edge_valid)
{
  _dbTechLayerCutSpacingTableDefRule* obj
      = (_dbTechLayerCutSpacingTableDefRule*) this;

  obj->_flags._center_and_edge_valid = _center_and_edge_valid;
}

bool dbTechLayerCutSpacingTableDefRule::isCenterAndEdgeValid() const
{
  _dbTechLayerCutSpacingTableDefRule* obj
      = (_dbTechLayerCutSpacingTableDefRule*) this;

  return obj->_flags._center_and_edge_valid;
}

void dbTechLayerCutSpacingTableDefRule::setNoPrl(bool _no_prl)
{
  _dbTechLayerCutSpacingTableDefRule* obj
      = (_dbTechLayerCutSpacingTableDefRule*) this;

  obj->_flags._no_prl = _no_prl;
}

bool dbTechLayerCutSpacingTableDefRule::isNoPrl() const
{
  _dbTechLayerCutSpacingTableDefRule* obj
      = (_dbTechLayerCutSpacingTableDefRule*) this;

  return obj->_flags._no_prl;
}

void dbTechLayerCutSpacingTableDefRule::setPrlValid(bool _prl_valid)
{
  _dbTechLayerCutSpacingTableDefRule* obj
      = (_dbTechLayerCutSpacingTableDefRule*) this;

  obj->_flags._prl_valid = _prl_valid;
}

bool dbTechLayerCutSpacingTableDefRule::isPrlValid() const
{
  _dbTechLayerCutSpacingTableDefRule* obj
      = (_dbTechLayerCutSpacingTableDefRule*) this;

  return obj->_flags._prl_valid;
}

void dbTechLayerCutSpacingTableDefRule::setMaxXY(bool _max_x_y)
{
  _dbTechLayerCutSpacingTableDefRule* obj
      = (_dbTechLayerCutSpacingTableDefRule*) this;

  obj->_flags._max_x_y = _max_x_y;
}

bool dbTechLayerCutSpacingTableDefRule::isMaxXY() const
{
  _dbTechLayerCutSpacingTableDefRule* obj
      = (_dbTechLayerCutSpacingTableDefRule*) this;

  return obj->_flags._max_x_y;
}

void dbTechLayerCutSpacingTableDefRule::setEndExtensionValid(
    bool _end_extension_valid)
{
  _dbTechLayerCutSpacingTableDefRule* obj
      = (_dbTechLayerCutSpacingTableDefRule*) this;

  obj->_flags._end_extension_valid = _end_extension_valid;
}

bool dbTechLayerCutSpacingTableDefRule::isEndExtensionValid() const
{
  _dbTechLayerCutSpacingTableDefRule* obj
      = (_dbTechLayerCutSpacingTableDefRule*) this;

  return obj->_flags._end_extension_valid;
}

void dbTechLayerCutSpacingTableDefRule::setSideExtensionValid(
    bool _side_extension_valid)
{
  _dbTechLayerCutSpacingTableDefRule* obj
      = (_dbTechLayerCutSpacingTableDefRule*) this;

  obj->_flags._side_extension_valid = _side_extension_valid;
}

bool dbTechLayerCutSpacingTableDefRule::isSideExtensionValid() const
{
  _dbTechLayerCutSpacingTableDefRule* obj
      = (_dbTechLayerCutSpacingTableDefRule*) this;

  return obj->_flags._side_extension_valid;
}

void dbTechLayerCutSpacingTableDefRule::setExactAlignedSpacingValid(
    bool _exact_aligned_spacing_valid)
{
  _dbTechLayerCutSpacingTableDefRule* obj
      = (_dbTechLayerCutSpacingTableDefRule*) this;

  obj->_flags._exact_aligned_spacing_valid = _exact_aligned_spacing_valid;
}

bool dbTechLayerCutSpacingTableDefRule::isExactAlignedSpacingValid() const
{
  _dbTechLayerCutSpacingTableDefRule* obj
      = (_dbTechLayerCutSpacingTableDefRule*) this;

  return obj->_flags._exact_aligned_spacing_valid;
}

void dbTechLayerCutSpacingTableDefRule::setHorizontal(bool _horizontal)
{
  _dbTechLayerCutSpacingTableDefRule* obj
      = (_dbTechLayerCutSpacingTableDefRule*) this;

  obj->_flags._horizontal = _horizontal;
}

bool dbTechLayerCutSpacingTableDefRule::isHorizontal() const
{
  _dbTechLayerCutSpacingTableDefRule* obj
      = (_dbTechLayerCutSpacingTableDefRule*) this;

  return obj->_flags._horizontal;
}

void dbTechLayerCutSpacingTableDefRule::setVertical(bool _vertical)
{
  _dbTechLayerCutSpacingTableDefRule* obj
      = (_dbTechLayerCutSpacingTableDefRule*) this;

  obj->_flags._vertical = _vertical;
}

bool dbTechLayerCutSpacingTableDefRule::isVertical() const
{
  _dbTechLayerCutSpacingTableDefRule* obj
      = (_dbTechLayerCutSpacingTableDefRule*) this;

  return obj->_flags._vertical;
}

void dbTechLayerCutSpacingTableDefRule::setNonOppositeEnclosureSpacingValid(
    bool _non_opposite_enclosure_spacing_valid)
{
  _dbTechLayerCutSpacingTableDefRule* obj
      = (_dbTechLayerCutSpacingTableDefRule*) this;

  obj->_flags._non_opposite_enclosure_spacing_valid
      = _non_opposite_enclosure_spacing_valid;
}

bool dbTechLayerCutSpacingTableDefRule::isNonOppositeEnclosureSpacingValid()
    const
{
  _dbTechLayerCutSpacingTableDefRule* obj
      = (_dbTechLayerCutSpacingTableDefRule*) this;

  return obj->_flags._non_opposite_enclosure_spacing_valid;
}

void dbTechLayerCutSpacingTableDefRule::setOppositeEnclosureResizeSpacingValid(
    bool _opposite_enclosure_resize_spacing_valid)
{
  _dbTechLayerCutSpacingTableDefRule* obj
      = (_dbTechLayerCutSpacingTableDefRule*) this;

  obj->_flags._opposite_enclosure_resize_spacing_valid
      = _opposite_enclosure_resize_spacing_valid;
}

bool dbTechLayerCutSpacingTableDefRule::isOppositeEnclosureResizeSpacingValid()
    const
{
  _dbTechLayerCutSpacingTableDefRule* obj
      = (_dbTechLayerCutSpacingTableDefRule*) this;

  return obj->_flags._opposite_enclosure_resize_spacing_valid;
}

// User Code Begin dbTechLayerCutSpacingTableDefRulePublicMethods
void dbTechLayerCutSpacingTableDefRule::addPrlForAlignedCutEntry(
    dbTechLayerCutClassRule* from,
    dbTechLayerCutClassRule* to)
{
  if (from == nullptr || to == nullptr)
    return;
  _dbTechLayerCutSpacingTableDefRule* obj
      = (_dbTechLayerCutSpacingTableDefRule*) this;
  obj->_prl_for_aligned_cut_tbl.push_back(
      {from->getImpl()->getOID(), to->getImpl()->getOID()});
}

std::vector<std::pair<dbTechLayerCutClassRule*, dbTechLayerCutClassRule*>>
dbTechLayerCutSpacingTableDefRule::getPrlForAlignedCutTable() const
{
  std::vector<std::pair<dbTechLayerCutClassRule*, dbTechLayerCutClassRule*>>
                                      res;
  _dbTechLayerCutSpacingTableDefRule* obj
      = (_dbTechLayerCutSpacingTableDefRule*) this;
  _dbTechLayer* layer = (_dbTechLayer*) obj->getOwner();
  for (auto& [from, to] : obj->_prl_for_aligned_cut_tbl) {
    res.push_back(
        {(dbTechLayerCutClassRule*) layer->_cut_class_rules_tbl->getPtr(from),
         (dbTechLayerCutClassRule*) layer->_cut_class_rules_tbl->getPtr(to)});
  }
  return res;
}

void dbTechLayerCutSpacingTableDefRule::addCenterToCenterEntry(
    dbTechLayerCutClassRule* from,
    dbTechLayerCutClassRule* to)
{
  if (from == nullptr || to == nullptr)
    return;
  _dbTechLayerCutSpacingTableDefRule* obj
      = (_dbTechLayerCutSpacingTableDefRule*) this;
  obj->_center_to_center_tbl.push_back(
      {from->getImpl()->getOID(), to->getImpl()->getOID()});
}

std::vector<std::pair<dbTechLayerCutClassRule*, dbTechLayerCutClassRule*>>
dbTechLayerCutSpacingTableDefRule::getCenterToCenterTable() const
{
  std::vector<std::pair<dbTechLayerCutClassRule*, dbTechLayerCutClassRule*>>
                                      res;
  _dbTechLayerCutSpacingTableDefRule* obj
      = (_dbTechLayerCutSpacingTableDefRule*) this;
  _dbTechLayer* layer = (_dbTechLayer*) obj->getOwner();
  for (auto& [from, to] : obj->_center_to_center_tbl) {
    res.push_back(
        {(dbTechLayerCutClassRule*) layer->_cut_class_rules_tbl->getPtr(from),
         (dbTechLayerCutClassRule*) layer->_cut_class_rules_tbl->getPtr(to)});
  }
  return res;
}

void dbTechLayerCutSpacingTableDefRule::addCenterAndEdgeEntry(
    dbTechLayerCutClassRule* from,
    dbTechLayerCutClassRule* to)
{
  if (from == nullptr || to == nullptr)
    return;
  _dbTechLayerCutSpacingTableDefRule* obj
      = (_dbTechLayerCutSpacingTableDefRule*) this;
  obj->_center_and_edge_tbl.push_back(
      {from->getImpl()->getOID(), to->getImpl()->getOID()});
}

std::vector<std::pair<dbTechLayerCutClassRule*, dbTechLayerCutClassRule*>>
dbTechLayerCutSpacingTableDefRule::getCenterAndEdgeTable() const
{
  std::vector<std::pair<dbTechLayerCutClassRule*, dbTechLayerCutClassRule*>>
                                      res;
  _dbTechLayerCutSpacingTableDefRule* obj
      = (_dbTechLayerCutSpacingTableDefRule*) this;
  _dbTechLayer* layer = (_dbTechLayer*) obj->getOwner();
  for (auto& [from, to] : obj->_center_and_edge_tbl) {
    res.push_back(
        {(dbTechLayerCutClassRule*) layer->_cut_class_rules_tbl->getPtr(from),
         (dbTechLayerCutClassRule*) layer->_cut_class_rules_tbl->getPtr(to)});
  }
  return res;
}

void dbTechLayerCutSpacingTableDefRule::addPrlEntry(
    dbTechLayerCutClassRule* from,
    dbTechLayerCutClassRule* to,
    int                      ccPrl)
{
  if (from == nullptr || to == nullptr)
    return;
  _dbTechLayerCutSpacingTableDefRule* obj
      = (_dbTechLayerCutSpacingTableDefRule*) this;
  obj->_prl_tbl.push_back(
      {from->getImpl()->getOID(), to->getImpl()->getOID(), ccPrl});
}

std::vector<std::tuple<dbTechLayerCutClassRule*, dbTechLayerCutClassRule*, int>>
dbTechLayerCutSpacingTableDefRule::getPrlTable() const
{
  std::vector<
      std::tuple<dbTechLayerCutClassRule*, dbTechLayerCutClassRule*, int>>
                                      res;
  _dbTechLayerCutSpacingTableDefRule* obj
      = (_dbTechLayerCutSpacingTableDefRule*) this;
  _dbTechLayer* layer = (_dbTechLayer*) obj->getOwner();
  for (auto&& [from, to, ccPrl] : obj->_prl_tbl) {
    res.push_back(
        {(dbTechLayerCutClassRule*) layer->_cut_class_rules_tbl->getPtr(from),
         (dbTechLayerCutClassRule*) layer->_cut_class_rules_tbl->getPtr(to),
         ccPrl});
  }
  return res;
}

void dbTechLayerCutSpacingTableDefRule::addEndExtensionEntry(
    dbTechLayerCutClassRule* cls,
    int                      ext)
{
  if (cls == nullptr)
    return;
  _dbTechLayerCutSpacingTableDefRule* obj
      = (_dbTechLayerCutSpacingTableDefRule*) this;
  obj->_end_extension_tbl.push_back({cls->getImpl()->getOID(), ext});
}

std::vector<std::pair<dbTechLayerCutClassRule*, int>>
dbTechLayerCutSpacingTableDefRule::getEndExtensionTable() const
{
  std::vector<std::pair<dbTechLayerCutClassRule*, int>> res;
  _dbTechLayerCutSpacingTableDefRule*                   obj
      = (_dbTechLayerCutSpacingTableDefRule*) this;
  _dbTechLayer* layer = (_dbTechLayer*) obj->getOwner();
  for (auto& [cls, ext] : obj->_end_extension_tbl) {
    res.push_back(
        {(dbTechLayerCutClassRule*) layer->_cut_class_rules_tbl->getPtr(cls),
         ext});
  }
  return res;
}

void dbTechLayerCutSpacingTableDefRule::addSideExtensionEntry(
    dbTechLayerCutClassRule* cls,
    int                      ext)
{
  if (cls == nullptr)
    return;
  _dbTechLayerCutSpacingTableDefRule* obj
      = (_dbTechLayerCutSpacingTableDefRule*) this;
  obj->_side_extension_tbl.push_back({cls->getImpl()->getOID(), ext});
}

std::vector<std::pair<dbTechLayerCutClassRule*, int>>
dbTechLayerCutSpacingTableDefRule::getSideExtensionTable() const
{
  std::vector<std::pair<dbTechLayerCutClassRule*, int>> res;
  _dbTechLayerCutSpacingTableDefRule*                   obj
      = (_dbTechLayerCutSpacingTableDefRule*) this;
  _dbTechLayer* layer = (_dbTechLayer*) obj->getOwner();
  for (auto& [cls, ext] : obj->_side_extension_tbl) {
    res.push_back(
        {(dbTechLayerCutClassRule*) layer->_cut_class_rules_tbl->getPtr(cls),
         ext});
  }
  return res;
}

void dbTechLayerCutSpacingTableDefRule::addExactElignedEntry(
    dbTechLayerCutClassRule* cls,
    int                      spacing)
{
  if (cls == nullptr)
    return;
  _dbTechLayerCutSpacingTableDefRule* obj
      = (_dbTechLayerCutSpacingTableDefRule*) this;
  obj->_exact_aligned_spacing_tbl.push_back(
      {cls->getImpl()->getOID(), spacing});
}

std::vector<std::pair<dbTechLayerCutClassRule*, int>>
dbTechLayerCutSpacingTableDefRule::getExactAlignedTable() const
{
  std::vector<std::pair<dbTechLayerCutClassRule*, int>> res;
  _dbTechLayerCutSpacingTableDefRule*                   obj
      = (_dbTechLayerCutSpacingTableDefRule*) this;
  _dbTechLayer* layer = (_dbTechLayer*) obj->getOwner();
  for (auto& [cls, spacing] : obj->_exact_aligned_spacing_tbl) {
    res.push_back(
        {(dbTechLayerCutClassRule*) layer->_cut_class_rules_tbl->getPtr(cls),
         spacing});
  }
  return res;
}

void dbTechLayerCutSpacingTableDefRule::addNonOppEncSpacingEntry(
    dbTechLayerCutClassRule* cls,
    int                      spacing)
{
  if (cls == nullptr)
    return;
  _dbTechLayerCutSpacingTableDefRule* obj
      = (_dbTechLayerCutSpacingTableDefRule*) this;
  obj->_non_opp_enc_spacing_tbl.push_back({cls->getImpl()->getOID(), spacing});
}

std::vector<std::pair<dbTechLayerCutClassRule*, int>>
dbTechLayerCutSpacingTableDefRule::getNonOppEncSpacingTable() const
{
  std::vector<std::pair<dbTechLayerCutClassRule*, int>> res;
  _dbTechLayerCutSpacingTableDefRule*                   obj
      = (_dbTechLayerCutSpacingTableDefRule*) this;
  _dbTechLayer* layer = (_dbTechLayer*) obj->getOwner();
  for (auto& [cls, spacing] : obj->_non_opp_enc_spacing_tbl) {
    res.push_back(
        {(dbTechLayerCutClassRule*) layer->_cut_class_rules_tbl->getPtr(cls),
         spacing});
  }
  return res;
}

void dbTechLayerCutSpacingTableDefRule::addOppEncSpacingEntry(
    dbTechLayerCutClassRule* cls,
    int                      rsz1,
    int                      rsz2,
    int                      spacing)
{
  if (cls == nullptr)
    return;
  _dbTechLayerCutSpacingTableDefRule* obj
      = (_dbTechLayerCutSpacingTableDefRule*) this;
  obj->_opp_enc_spacing_tbl.push_back(
      {cls->getImpl()->getOID(), rsz1, rsz2, spacing});
}

std::vector<std::tuple<dbTechLayerCutClassRule*, int, int, int>>
dbTechLayerCutSpacingTableDefRule::getOppEncSpacingTable() const
{
  std::vector<std::tuple<dbTechLayerCutClassRule*, int, int, int>> res;
  _dbTechLayerCutSpacingTableDefRule*                              obj
      = (_dbTechLayerCutSpacingTableDefRule*) this;
  _dbTechLayer* layer = (_dbTechLayer*) obj->getOwner();
  for (auto& [cls, rsz1, rsz2, spacing] : obj->_opp_enc_spacing_tbl) {
    res.push_back(
        {(dbTechLayerCutClassRule*) layer->_cut_class_rules_tbl->getPtr(cls),
         rsz1,
         rsz2,
         spacing});
  }
  return res;
}

dbTechLayer* dbTechLayerCutSpacingTableDefRule::getSecondLayer() const
{
  _dbTechLayerCutSpacingTableDefRule* obj
      = (_dbTechLayerCutSpacingTableDefRule*) this;
  if (obj->_second_layer == 0)
    return nullptr;
  _dbTechLayer* layer = (_dbTechLayer*) obj->getOwner();
  _dbTech*      _tech = (_dbTech*) layer->getOwner();
  return (dbTechLayer*) _tech->_layer_tbl->getPtr(obj->_second_layer);
}

void dbTechLayerCutSpacingTableDefRule::setSpacingTable(
    std::vector<std::vector<std::pair<int, int>>> table,
    std::map<std::string, uint>                   row_map,
    std::map<std::string, uint>                   col_map)
{
  _dbTechLayerCutSpacingTableDefRule* obj
      = (_dbTechLayerCutSpacingTableDefRule*) this;
  obj->_row_map = row_map;
  obj->_col_map = col_map;
  for (auto spacing : table) {
    dbVector<std::pair<int, int>> tmp;
    tmp = spacing;
    obj->_spacing_tbl.push_back(tmp);
  }
}

void dbTechLayerCutSpacingTableDefRule::getSpacingTable(
    std::vector<std::vector<std::pair<int, int>>>& table,
    std::map<std::string, uint>&                   row_map,
    std::map<std::string, uint>&                   col_map)
{
  _dbTechLayerCutSpacingTableDefRule* obj
      = (_dbTechLayerCutSpacingTableDefRule*) this;
  row_map = obj->_row_map;
  col_map = obj->_col_map;
  table.clear();
  for (auto spacing : obj->_spacing_tbl) {
    table.push_back(spacing);
  }
}

std::pair<int, int> dbTechLayerCutSpacingTableDefRule::getSpacing(
    const char* class1,
    bool        SIDE1,
    const char* class2,
    bool        SIDE2)
{
  _dbTechLayerCutSpacingTableDefRule* obj
      = (_dbTechLayerCutSpacingTableDefRule*) this;
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

dbTechLayerCutSpacingTableDefRule* dbTechLayerCutSpacingTableDefRule::create(
    dbTechLayer* parent)
{
  _dbTechLayer*                       _parent = (_dbTechLayer*) parent;
  _dbTechLayerCutSpacingTableDefRule* newrule
      = _parent->_cut_spacing_table_def_tbl->create();
  return ((dbTechLayerCutSpacingTableDefRule*) newrule);
}

dbTechLayerCutSpacingTableDefRule*
dbTechLayerCutSpacingTableDefRule::getTechLayerCutSpacingTableDefSubRule(
    dbTechLayer* parent,
    uint         dbid)
{
  _dbTechLayer* _parent = (_dbTechLayer*) parent;
  return (dbTechLayerCutSpacingTableDefRule*)
      _parent->_cut_spacing_table_def_tbl->getPtr(dbid);
}
void dbTechLayerCutSpacingTableDefRule::destroy(
    dbTechLayerCutSpacingTableDefRule* rule)
{
  _dbTechLayer* _parent = (_dbTechLayer*) rule->getImpl()->getOwner();
  dbProperty::destroyProperties(rule);
  _parent->_cut_spacing_table_def_tbl->destroy(
      (_dbTechLayerCutSpacingTableDefRule*) rule);
}

// User Code End dbTechLayerCutSpacingTableDefRulePublicMethods
}  // namespace odb
   // Generator Code End 1