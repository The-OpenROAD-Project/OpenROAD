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
#include "dbTechLayerCutSpacingRule.h"

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
template class dbTable<_dbTechLayerCutSpacingRule>;

bool _dbTechLayerCutSpacingRule::operator==(
    const _dbTechLayerCutSpacingRule& rhs) const
{
  if (_flags._center_to_center != rhs._flags._center_to_center)
    return false;

  if (_flags._same_net != rhs._flags._same_net)
    return false;

  if (_flags._same_metal != rhs._flags._same_metal)
    return false;

  if (_flags._same_via != rhs._flags._same_via)
    return false;

  if (_flags._cut_spacing_type != rhs._flags._cut_spacing_type)
    return false;

  if (_flags._stack != rhs._flags._stack)
    return false;

  if (_flags._orthogonal_spacing_valid != rhs._flags._orthogonal_spacing_valid)
    return false;

  if (_flags._above_width_enclosure_valid
      != rhs._flags._above_width_enclosure_valid)
    return false;

  if (_flags._short_edge_only != rhs._flags._short_edge_only)
    return false;

  if (_flags._concave_corner_width != rhs._flags._concave_corner_width)
    return false;

  if (_flags._concave_corner_parallel != rhs._flags._concave_corner_parallel)
    return false;

  if (_flags._concave_corner_edge_length
      != rhs._flags._concave_corner_edge_length)
    return false;

  if (_flags._concave_corner != rhs._flags._concave_corner)
    return false;

  if (_flags._extension_valid != rhs._flags._extension_valid)
    return false;

  if (_flags._non_eol_convex_corner != rhs._flags._non_eol_convex_corner)
    return false;

  if (_flags._eol_width_valid != rhs._flags._eol_width_valid)
    return false;

  if (_flags._min_length_valid != rhs._flags._min_length_valid)
    return false;

  if (_flags._above_width_valid != rhs._flags._above_width_valid)
    return false;

  if (_flags._mask_overlap != rhs._flags._mask_overlap)
    return false;

  if (_flags._wrong_direction != rhs._flags._wrong_direction)
    return false;

  if (_flags._adjacent_cuts != rhs._flags._adjacent_cuts)
    return false;

  if (_flags._exact_aligned != rhs._flags._exact_aligned)
    return false;

  if (_flags._cut_class_to_all != rhs._flags._cut_class_to_all)
    return false;

  if (_flags._no_prl != rhs._flags._no_prl)
    return false;

  if (_flags._same_mask != rhs._flags._same_mask)
    return false;

  if (_flags._except_same_pgnet != rhs._flags._except_same_pgnet)
    return false;

  if (_flags._side_parallel_overlap != rhs._flags._side_parallel_overlap)
    return false;

  if (_flags._except_same_net != rhs._flags._except_same_net)
    return false;

  if (_flags._except_same_metal != rhs._flags._except_same_metal)
    return false;

  if (_flags._except_same_metal_overlap
      != rhs._flags._except_same_metal_overlap)
    return false;

  if (_flags._except_same_via != rhs._flags._except_same_via)
    return false;

  if (_flags._above != rhs._flags._above)
    return false;

  if (_flags._except_two_edges != rhs._flags._except_two_edges)
    return false;

  if (_flags._two_cuts_valid != rhs._flags._two_cuts_valid)
    return false;

  if (_flags._same_cut != rhs._flags._same_cut)
    return false;

  if (_flags._long_edge_only != rhs._flags._long_edge_only)
    return false;

  if (_flags._below != rhs._flags._below)
    return false;

  if (_flags._par_within_enclosure_valid
      != rhs._flags._par_within_enclosure_valid)
    return false;

  if (_cut_spacing != rhs._cut_spacing)
    return false;

  if (_second_layer != rhs._second_layer)
    return false;

  if (_orthogonal_spacing != rhs._orthogonal_spacing)
    return false;

  if (_width != rhs._width)
    return false;

  if (_enclosure != rhs._enclosure)
    return false;

  if (_edge_length != rhs._edge_length)
    return false;

  if (_par_within != rhs._par_within)
    return false;

  if (_par_enclosure != rhs._par_enclosure)
    return false;

  if (_edge_enclosure != rhs._edge_enclosure)
    return false;

  if (_adj_enclosure != rhs._adj_enclosure)
    return false;

  if (_above_enclosure != rhs._above_enclosure)
    return false;

  if (_above_width != rhs._above_width)
    return false;

  if (_min_length != rhs._min_length)
    return false;

  if (_extension != rhs._extension)
    return false;

  if (_eol_width != rhs._eol_width)
    return false;

  if (_num_cuts != rhs._num_cuts)
    return false;

  if (_within != rhs._within)
    return false;

  if (_second_within != rhs._second_within)
    return false;

  if (_cut_class != rhs._cut_class)
    return false;

  if (_two_cuts != rhs._two_cuts)
    return false;

  if (_par_length != rhs._par_length)
    return false;

  if (_cut_area != rhs._cut_area)
    return false;

  // User Code Begin ==
  // User Code End ==
  return true;
}
bool _dbTechLayerCutSpacingRule::operator<(
    const _dbTechLayerCutSpacingRule& rhs) const
{
  // User Code Begin <
  // User Code End <
  return true;
}
void _dbTechLayerCutSpacingRule::differences(
    dbDiff&                           diff,
    const char*                       field,
    const _dbTechLayerCutSpacingRule& rhs) const
{
  DIFF_BEGIN

  DIFF_FIELD(_flags._center_to_center);
  DIFF_FIELD(_flags._same_net);
  DIFF_FIELD(_flags._same_metal);
  DIFF_FIELD(_flags._same_via);
  DIFF_FIELD(_flags._cut_spacing_type);
  DIFF_FIELD(_flags._stack);
  DIFF_FIELD(_flags._orthogonal_spacing_valid);
  DIFF_FIELD(_flags._above_width_enclosure_valid);
  DIFF_FIELD(_flags._short_edge_only);
  DIFF_FIELD(_flags._concave_corner_width);
  DIFF_FIELD(_flags._concave_corner_parallel);
  DIFF_FIELD(_flags._concave_corner_edge_length);
  DIFF_FIELD(_flags._concave_corner);
  DIFF_FIELD(_flags._extension_valid);
  DIFF_FIELD(_flags._non_eol_convex_corner);
  DIFF_FIELD(_flags._eol_width_valid);
  DIFF_FIELD(_flags._min_length_valid);
  DIFF_FIELD(_flags._above_width_valid);
  DIFF_FIELD(_flags._mask_overlap);
  DIFF_FIELD(_flags._wrong_direction);
  DIFF_FIELD(_flags._adjacent_cuts);
  DIFF_FIELD(_flags._exact_aligned);
  DIFF_FIELD(_flags._cut_class_to_all);
  DIFF_FIELD(_flags._no_prl);
  DIFF_FIELD(_flags._same_mask);
  DIFF_FIELD(_flags._except_same_pgnet);
  DIFF_FIELD(_flags._side_parallel_overlap);
  DIFF_FIELD(_flags._except_same_net);
  DIFF_FIELD(_flags._except_same_metal);
  DIFF_FIELD(_flags._except_same_metal_overlap);
  DIFF_FIELD(_flags._except_same_via);
  DIFF_FIELD(_flags._above);
  DIFF_FIELD(_flags._except_two_edges);
  DIFF_FIELD(_flags._two_cuts_valid);
  DIFF_FIELD(_flags._same_cut);
  DIFF_FIELD(_flags._long_edge_only);
  DIFF_FIELD(_flags._below);
  DIFF_FIELD(_flags._par_within_enclosure_valid);
  DIFF_FIELD(_cut_spacing);
  DIFF_FIELD(_second_layer);
  DIFF_FIELD(_orthogonal_spacing);
  DIFF_FIELD(_width);
  DIFF_FIELD(_enclosure);
  DIFF_FIELD(_edge_length);
  DIFF_FIELD(_par_within);
  DIFF_FIELD(_par_enclosure);
  DIFF_FIELD(_edge_enclosure);
  DIFF_FIELD(_adj_enclosure);
  DIFF_FIELD(_above_enclosure);
  DIFF_FIELD(_above_width);
  DIFF_FIELD(_min_length);
  DIFF_FIELD(_extension);
  DIFF_FIELD(_eol_width);
  DIFF_FIELD(_num_cuts);
  DIFF_FIELD(_within);
  DIFF_FIELD(_second_within);
  DIFF_FIELD(_cut_class);
  DIFF_FIELD(_two_cuts);
  DIFF_FIELD(_par_length);
  DIFF_FIELD(_cut_area);
  // User Code Begin differences
  // User Code End differences
  DIFF_END
}
void _dbTechLayerCutSpacingRule::out(dbDiff&     diff,
                                     char        side,
                                     const char* field) const
{
  DIFF_OUT_BEGIN
  DIFF_OUT_FIELD(_flags._center_to_center);
  DIFF_OUT_FIELD(_flags._same_net);
  DIFF_OUT_FIELD(_flags._same_metal);
  DIFF_OUT_FIELD(_flags._same_via);
  DIFF_OUT_FIELD(_flags._cut_spacing_type);
  DIFF_OUT_FIELD(_flags._stack);
  DIFF_OUT_FIELD(_flags._orthogonal_spacing_valid);
  DIFF_OUT_FIELD(_flags._above_width_enclosure_valid);
  DIFF_OUT_FIELD(_flags._short_edge_only);
  DIFF_OUT_FIELD(_flags._concave_corner_width);
  DIFF_OUT_FIELD(_flags._concave_corner_parallel);
  DIFF_OUT_FIELD(_flags._concave_corner_edge_length);
  DIFF_OUT_FIELD(_flags._concave_corner);
  DIFF_OUT_FIELD(_flags._extension_valid);
  DIFF_OUT_FIELD(_flags._non_eol_convex_corner);
  DIFF_OUT_FIELD(_flags._eol_width_valid);
  DIFF_OUT_FIELD(_flags._min_length_valid);
  DIFF_OUT_FIELD(_flags._above_width_valid);
  DIFF_OUT_FIELD(_flags._mask_overlap);
  DIFF_OUT_FIELD(_flags._wrong_direction);
  DIFF_OUT_FIELD(_flags._adjacent_cuts);
  DIFF_OUT_FIELD(_flags._exact_aligned);
  DIFF_OUT_FIELD(_flags._cut_class_to_all);
  DIFF_OUT_FIELD(_flags._no_prl);
  DIFF_OUT_FIELD(_flags._same_mask);
  DIFF_OUT_FIELD(_flags._except_same_pgnet);
  DIFF_OUT_FIELD(_flags._side_parallel_overlap);
  DIFF_OUT_FIELD(_flags._except_same_net);
  DIFF_OUT_FIELD(_flags._except_same_metal);
  DIFF_OUT_FIELD(_flags._except_same_metal_overlap);
  DIFF_OUT_FIELD(_flags._except_same_via);
  DIFF_OUT_FIELD(_flags._above);
  DIFF_OUT_FIELD(_flags._except_two_edges);
  DIFF_OUT_FIELD(_flags._two_cuts_valid);
  DIFF_OUT_FIELD(_flags._same_cut);
  DIFF_OUT_FIELD(_flags._long_edge_only);
  DIFF_OUT_FIELD(_flags._below);
  DIFF_OUT_FIELD(_flags._par_within_enclosure_valid);
  DIFF_OUT_FIELD(_cut_spacing);
  DIFF_OUT_FIELD(_second_layer);
  DIFF_OUT_FIELD(_orthogonal_spacing);
  DIFF_OUT_FIELD(_width);
  DIFF_OUT_FIELD(_enclosure);
  DIFF_OUT_FIELD(_edge_length);
  DIFF_OUT_FIELD(_par_within);
  DIFF_OUT_FIELD(_par_enclosure);
  DIFF_OUT_FIELD(_edge_enclosure);
  DIFF_OUT_FIELD(_adj_enclosure);
  DIFF_OUT_FIELD(_above_enclosure);
  DIFF_OUT_FIELD(_above_width);
  DIFF_OUT_FIELD(_min_length);
  DIFF_OUT_FIELD(_extension);
  DIFF_OUT_FIELD(_eol_width);
  DIFF_OUT_FIELD(_num_cuts);
  DIFF_OUT_FIELD(_within);
  DIFF_OUT_FIELD(_second_within);
  DIFF_OUT_FIELD(_cut_class);
  DIFF_OUT_FIELD(_two_cuts);
  DIFF_OUT_FIELD(_par_length);
  DIFF_OUT_FIELD(_cut_area);

  // User Code Begin out
  // User Code End out
  DIFF_END
}
_dbTechLayerCutSpacingRule::_dbTechLayerCutSpacingRule(_dbDatabase* db)
{
  long long* _flags_bit_field = (long long*) &_flags;
  *_flags_bit_field           = 0;
  // User Code Begin constructor
  // User Code End constructor
}
_dbTechLayerCutSpacingRule::_dbTechLayerCutSpacingRule(
    _dbDatabase*                      db,
    const _dbTechLayerCutSpacingRule& r)
{
  _flags._center_to_center            = r._flags._center_to_center;
  _flags._same_net                    = r._flags._same_net;
  _flags._same_metal                  = r._flags._same_metal;
  _flags._same_via                    = r._flags._same_via;
  _flags._cut_spacing_type            = r._flags._cut_spacing_type;
  _flags._stack                       = r._flags._stack;
  _flags._orthogonal_spacing_valid    = r._flags._orthogonal_spacing_valid;
  _flags._above_width_enclosure_valid = r._flags._above_width_enclosure_valid;
  _flags._short_edge_only             = r._flags._short_edge_only;
  _flags._concave_corner_width        = r._flags._concave_corner_width;
  _flags._concave_corner_parallel     = r._flags._concave_corner_parallel;
  _flags._concave_corner_edge_length  = r._flags._concave_corner_edge_length;
  _flags._concave_corner              = r._flags._concave_corner;
  _flags._extension_valid             = r._flags._extension_valid;
  _flags._non_eol_convex_corner       = r._flags._non_eol_convex_corner;
  _flags._eol_width_valid             = r._flags._eol_width_valid;
  _flags._min_length_valid            = r._flags._min_length_valid;
  _flags._above_width_valid           = r._flags._above_width_valid;
  _flags._mask_overlap                = r._flags._mask_overlap;
  _flags._wrong_direction             = r._flags._wrong_direction;
  _flags._adjacent_cuts               = r._flags._adjacent_cuts;
  _flags._exact_aligned               = r._flags._exact_aligned;
  _flags._cut_class_to_all            = r._flags._cut_class_to_all;
  _flags._no_prl                      = r._flags._no_prl;
  _flags._same_mask                   = r._flags._same_mask;
  _flags._except_same_pgnet           = r._flags._except_same_pgnet;
  _flags._side_parallel_overlap       = r._flags._side_parallel_overlap;
  _flags._except_same_net             = r._flags._except_same_net;
  _flags._except_same_metal           = r._flags._except_same_metal;
  _flags._except_same_metal_overlap   = r._flags._except_same_metal_overlap;
  _flags._except_same_via             = r._flags._except_same_via;
  _flags._above                       = r._flags._above;
  _flags._except_two_edges            = r._flags._except_two_edges;
  _flags._two_cuts_valid              = r._flags._two_cuts_valid;
  _flags._same_cut                    = r._flags._same_cut;
  _flags._long_edge_only              = r._flags._long_edge_only;
  _flags._below                       = r._flags._below;
  _flags._par_within_enclosure_valid  = r._flags._par_within_enclosure_valid;
  _flags._spare_bits                  = r._flags._spare_bits;
  _cut_spacing                        = r._cut_spacing;
  _second_layer                       = r._second_layer;
  _orthogonal_spacing                 = r._orthogonal_spacing;
  _width                              = r._width;
  _enclosure                          = r._enclosure;
  _edge_length                        = r._edge_length;
  _par_within                         = r._par_within;
  _par_enclosure                      = r._par_enclosure;
  _edge_enclosure                     = r._edge_enclosure;
  _adj_enclosure                      = r._adj_enclosure;
  _above_enclosure                    = r._above_enclosure;
  _above_width                        = r._above_width;
  _min_length                         = r._min_length;
  _extension                          = r._extension;
  _eol_width                          = r._eol_width;
  _num_cuts                           = r._num_cuts;
  _within                             = r._within;
  _second_within                      = r._second_within;
  _cut_class                          = r._cut_class;
  _two_cuts                           = r._two_cuts;
  _par_length                         = r._par_length;
  _cut_area                           = r._cut_area;
  // User Code Begin CopyConstructor
  // User Code End CopyConstructor
}

dbIStream& operator>>(dbIStream& stream, _dbTechLayerCutSpacingRule& obj)
{
  long long* _flags_bit_field = (long long*) &obj._flags;
  stream >> *_flags_bit_field;
  stream >> obj._cut_spacing;
  stream >> obj._second_layer;
  stream >> obj._orthogonal_spacing;
  stream >> obj._width;
  stream >> obj._enclosure;
  stream >> obj._edge_length;
  stream >> obj._par_within;
  stream >> obj._par_enclosure;
  stream >> obj._edge_enclosure;
  stream >> obj._adj_enclosure;
  stream >> obj._above_enclosure;
  stream >> obj._above_width;
  stream >> obj._min_length;
  stream >> obj._extension;
  stream >> obj._eol_width;
  stream >> obj._num_cuts;
  stream >> obj._within;
  stream >> obj._second_within;
  stream >> obj._cut_class;
  stream >> obj._two_cuts;
  stream >> obj._par_length;
  stream >> obj._cut_area;
  // User Code Begin >>
  // User Code End >>
  return stream;
}
dbOStream& operator<<(dbOStream& stream, const _dbTechLayerCutSpacingRule& obj)
{
  long long* _flags_bit_field = (long long*) &obj._flags;
  stream << *_flags_bit_field;
  stream << obj._cut_spacing;
  stream << obj._second_layer;
  stream << obj._orthogonal_spacing;
  stream << obj._width;
  stream << obj._enclosure;
  stream << obj._edge_length;
  stream << obj._par_within;
  stream << obj._par_enclosure;
  stream << obj._edge_enclosure;
  stream << obj._adj_enclosure;
  stream << obj._above_enclosure;
  stream << obj._above_width;
  stream << obj._min_length;
  stream << obj._extension;
  stream << obj._eol_width;
  stream << obj._num_cuts;
  stream << obj._within;
  stream << obj._second_within;
  stream << obj._cut_class;
  stream << obj._two_cuts;
  stream << obj._par_length;
  stream << obj._cut_area;
  // User Code Begin <<
  // User Code End <<
  return stream;
}

_dbTechLayerCutSpacingRule::~_dbTechLayerCutSpacingRule()
{
  // User Code Begin Destructor
  // User Code End Destructor
}
////////////////////////////////////////////////////////////////////
//
// dbTechLayerCutSpacingRule - Methods
//
////////////////////////////////////////////////////////////////////

void dbTechLayerCutSpacingRule::setCutSpacing(int _cut_spacing)
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;

  obj->_cut_spacing = _cut_spacing;
}

int dbTechLayerCutSpacingRule::getCutSpacing() const
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;
  return obj->_cut_spacing;
}

void dbTechLayerCutSpacingRule::setSecondLayer(dbTechLayer* _second_layer)
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;

  obj->_second_layer = _second_layer->getImpl()->getOID();
}

void dbTechLayerCutSpacingRule::setOrthogonalSpacing(int _orthogonal_spacing)
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;

  obj->_orthogonal_spacing = _orthogonal_spacing;
}

int dbTechLayerCutSpacingRule::getOrthogonalSpacing() const
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;
  return obj->_orthogonal_spacing;
}

void dbTechLayerCutSpacingRule::setWidth(int _width)
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;

  obj->_width = _width;
}

int dbTechLayerCutSpacingRule::getWidth() const
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;
  return obj->_width;
}

void dbTechLayerCutSpacingRule::setEnclosure(int _enclosure)
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;

  obj->_enclosure = _enclosure;
}

int dbTechLayerCutSpacingRule::getEnclosure() const
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;
  return obj->_enclosure;
}

void dbTechLayerCutSpacingRule::setEdgeLength(int _edge_length)
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;

  obj->_edge_length = _edge_length;
}

int dbTechLayerCutSpacingRule::getEdgeLength() const
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;
  return obj->_edge_length;
}

void dbTechLayerCutSpacingRule::setParWithin(int _par_within)
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;

  obj->_par_within = _par_within;
}

int dbTechLayerCutSpacingRule::getParWithin() const
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;
  return obj->_par_within;
}

void dbTechLayerCutSpacingRule::setParEnclosure(int _par_enclosure)
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;

  obj->_par_enclosure = _par_enclosure;
}

int dbTechLayerCutSpacingRule::getParEnclosure() const
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;
  return obj->_par_enclosure;
}

void dbTechLayerCutSpacingRule::setEdgeEnclosure(int _edge_enclosure)
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;

  obj->_edge_enclosure = _edge_enclosure;
}

int dbTechLayerCutSpacingRule::getEdgeEnclosure() const
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;
  return obj->_edge_enclosure;
}

void dbTechLayerCutSpacingRule::setAdjEnclosure(int _adj_enclosure)
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;

  obj->_adj_enclosure = _adj_enclosure;
}

int dbTechLayerCutSpacingRule::getAdjEnclosure() const
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;
  return obj->_adj_enclosure;
}

void dbTechLayerCutSpacingRule::setAboveEnclosure(int _above_enclosure)
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;

  obj->_above_enclosure = _above_enclosure;
}

int dbTechLayerCutSpacingRule::getAboveEnclosure() const
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;
  return obj->_above_enclosure;
}

void dbTechLayerCutSpacingRule::setAboveWidth(int _above_width)
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;

  obj->_above_width = _above_width;
}

int dbTechLayerCutSpacingRule::getAboveWidth() const
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;
  return obj->_above_width;
}

void dbTechLayerCutSpacingRule::setMinLength(int _min_length)
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;

  obj->_min_length = _min_length;
}

int dbTechLayerCutSpacingRule::getMinLength() const
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;
  return obj->_min_length;
}

void dbTechLayerCutSpacingRule::setExtension(int _extension)
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;

  obj->_extension = _extension;
}

int dbTechLayerCutSpacingRule::getExtension() const
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;
  return obj->_extension;
}

void dbTechLayerCutSpacingRule::setEolWidth(int _eol_width)
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;

  obj->_eol_width = _eol_width;
}

int dbTechLayerCutSpacingRule::getEolWidth() const
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;
  return obj->_eol_width;
}

void dbTechLayerCutSpacingRule::setNumCuts(uint _num_cuts)
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;

  obj->_num_cuts = _num_cuts;
}

uint dbTechLayerCutSpacingRule::getNumCuts() const
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;
  return obj->_num_cuts;
}

void dbTechLayerCutSpacingRule::setWithin(int _within)
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;

  obj->_within = _within;
}

int dbTechLayerCutSpacingRule::getWithin() const
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;
  return obj->_within;
}

void dbTechLayerCutSpacingRule::setSecondWithin(int _second_within)
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;

  obj->_second_within = _second_within;
}

int dbTechLayerCutSpacingRule::getSecondWithin() const
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;
  return obj->_second_within;
}

void dbTechLayerCutSpacingRule::setCutClass(dbTechLayerCutClassRule* _cut_class)
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;

  obj->_cut_class = _cut_class->getImpl()->getOID();
}

void dbTechLayerCutSpacingRule::setTwoCuts(uint _two_cuts)
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;

  obj->_two_cuts = _two_cuts;
}

uint dbTechLayerCutSpacingRule::getTwoCuts() const
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;
  return obj->_two_cuts;
}

void dbTechLayerCutSpacingRule::setParLength(uint _par_length)
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;

  obj->_par_length = _par_length;
}

uint dbTechLayerCutSpacingRule::getParLength() const
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;
  return obj->_par_length;
}

void dbTechLayerCutSpacingRule::setCutArea(int _cut_area)
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;

  obj->_cut_area = _cut_area;
}

int dbTechLayerCutSpacingRule::getCutArea() const
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;
  return obj->_cut_area;
}

void dbTechLayerCutSpacingRule::setCenterToCenter(bool _center_to_center)
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;

  obj->_flags._center_to_center = _center_to_center;
}

bool dbTechLayerCutSpacingRule::isCenterToCenter() const
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;

  return obj->_flags._center_to_center;
}

void dbTechLayerCutSpacingRule::setSameNet(bool _same_net)
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;

  obj->_flags._same_net = _same_net;
}

bool dbTechLayerCutSpacingRule::isSameNet() const
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;

  return obj->_flags._same_net;
}

void dbTechLayerCutSpacingRule::setSameMetal(bool _same_metal)
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;

  obj->_flags._same_metal = _same_metal;
}

bool dbTechLayerCutSpacingRule::isSameMetal() const
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;

  return obj->_flags._same_metal;
}

void dbTechLayerCutSpacingRule::setSameVia(bool _same_via)
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;

  obj->_flags._same_via = _same_via;
}

bool dbTechLayerCutSpacingRule::isSameVia() const
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;

  return obj->_flags._same_via;
}

void dbTechLayerCutSpacingRule::setStack(bool _stack)
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;

  obj->_flags._stack = _stack;
}

bool dbTechLayerCutSpacingRule::isStack() const
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;

  return obj->_flags._stack;
}

void dbTechLayerCutSpacingRule::setOrthogonalSpacingValid(
    bool _orthogonal_spacing_valid)
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;

  obj->_flags._orthogonal_spacing_valid = _orthogonal_spacing_valid;
}

bool dbTechLayerCutSpacingRule::isOrthogonalSpacingValid() const
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;

  return obj->_flags._orthogonal_spacing_valid;
}

void dbTechLayerCutSpacingRule::setAboveWidthEnclosureValid(
    bool _above_width_enclosure_valid)
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;

  obj->_flags._above_width_enclosure_valid = _above_width_enclosure_valid;
}

bool dbTechLayerCutSpacingRule::isAboveWidthEnclosureValid() const
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;

  return obj->_flags._above_width_enclosure_valid;
}

void dbTechLayerCutSpacingRule::setShortEdgeOnly(bool _short_edge_only)
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;

  obj->_flags._short_edge_only = _short_edge_only;
}

bool dbTechLayerCutSpacingRule::isShortEdgeOnly() const
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;

  return obj->_flags._short_edge_only;
}

void dbTechLayerCutSpacingRule::setConcaveCornerWidth(
    bool _concave_corner_width)
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;

  obj->_flags._concave_corner_width = _concave_corner_width;
}

bool dbTechLayerCutSpacingRule::isConcaveCornerWidth() const
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;

  return obj->_flags._concave_corner_width;
}

void dbTechLayerCutSpacingRule::setConcaveCornerParallel(
    bool _concave_corner_parallel)
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;

  obj->_flags._concave_corner_parallel = _concave_corner_parallel;
}

bool dbTechLayerCutSpacingRule::isConcaveCornerParallel() const
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;

  return obj->_flags._concave_corner_parallel;
}

void dbTechLayerCutSpacingRule::setConcaveCornerEdgeLength(
    bool _concave_corner_edge_length)
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;

  obj->_flags._concave_corner_edge_length = _concave_corner_edge_length;
}

bool dbTechLayerCutSpacingRule::isConcaveCornerEdgeLength() const
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;

  return obj->_flags._concave_corner_edge_length;
}

void dbTechLayerCutSpacingRule::setConcaveCorner(bool _concave_corner)
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;

  obj->_flags._concave_corner = _concave_corner;
}

bool dbTechLayerCutSpacingRule::isConcaveCorner() const
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;

  return obj->_flags._concave_corner;
}

void dbTechLayerCutSpacingRule::setExtensionValid(bool _extension_valid)
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;

  obj->_flags._extension_valid = _extension_valid;
}

bool dbTechLayerCutSpacingRule::isExtensionValid() const
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;

  return obj->_flags._extension_valid;
}

void dbTechLayerCutSpacingRule::setNonEolConvexCorner(
    bool _non_eol_convex_corner)
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;

  obj->_flags._non_eol_convex_corner = _non_eol_convex_corner;
}

bool dbTechLayerCutSpacingRule::isNonEolConvexCorner() const
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;

  return obj->_flags._non_eol_convex_corner;
}

void dbTechLayerCutSpacingRule::setEolWidthValid(bool _eol_width_valid)
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;

  obj->_flags._eol_width_valid = _eol_width_valid;
}

bool dbTechLayerCutSpacingRule::isEolWidthValid() const
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;

  return obj->_flags._eol_width_valid;
}

void dbTechLayerCutSpacingRule::setMinLengthValid(bool _min_length_valid)
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;

  obj->_flags._min_length_valid = _min_length_valid;
}

bool dbTechLayerCutSpacingRule::isMinLengthValid() const
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;

  return obj->_flags._min_length_valid;
}

void dbTechLayerCutSpacingRule::setAboveWidthValid(bool _above_width_valid)
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;

  obj->_flags._above_width_valid = _above_width_valid;
}

bool dbTechLayerCutSpacingRule::isAboveWidthValid() const
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;

  return obj->_flags._above_width_valid;
}

void dbTechLayerCutSpacingRule::setMaskOverlap(bool _mask_overlap)
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;

  obj->_flags._mask_overlap = _mask_overlap;
}

bool dbTechLayerCutSpacingRule::isMaskOverlap() const
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;

  return obj->_flags._mask_overlap;
}

void dbTechLayerCutSpacingRule::setWrongDirection(bool _wrong_direction)
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;

  obj->_flags._wrong_direction = _wrong_direction;
}

bool dbTechLayerCutSpacingRule::isWrongDirection() const
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;

  return obj->_flags._wrong_direction;
}

void dbTechLayerCutSpacingRule::setAdjacentCuts(uint _adjacent_cuts)
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;

  obj->_flags._adjacent_cuts = _adjacent_cuts;
}

uint dbTechLayerCutSpacingRule::getAdjacentCuts() const
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;

  return obj->_flags._adjacent_cuts;
}

void dbTechLayerCutSpacingRule::setExactAligned(bool _exact_aligned)
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;

  obj->_flags._exact_aligned = _exact_aligned;
}

bool dbTechLayerCutSpacingRule::isExactAligned() const
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;

  return obj->_flags._exact_aligned;
}

void dbTechLayerCutSpacingRule::setCutClassToAll(bool _cut_class_to_all)
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;

  obj->_flags._cut_class_to_all = _cut_class_to_all;
}

bool dbTechLayerCutSpacingRule::isCutClassToAll() const
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;

  return obj->_flags._cut_class_to_all;
}

void dbTechLayerCutSpacingRule::setNoPrl(bool _no_prl)
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;

  obj->_flags._no_prl = _no_prl;
}

bool dbTechLayerCutSpacingRule::isNoPrl() const
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;

  return obj->_flags._no_prl;
}

void dbTechLayerCutSpacingRule::setSameMask(bool _same_mask)
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;

  obj->_flags._same_mask = _same_mask;
}

bool dbTechLayerCutSpacingRule::isSameMask() const
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;

  return obj->_flags._same_mask;
}

void dbTechLayerCutSpacingRule::setExceptSamePgnet(bool _except_same_pgnet)
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;

  obj->_flags._except_same_pgnet = _except_same_pgnet;
}

bool dbTechLayerCutSpacingRule::isExceptSamePgnet() const
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;

  return obj->_flags._except_same_pgnet;
}

void dbTechLayerCutSpacingRule::setSideParallelOverlap(
    bool _side_parallel_overlap)
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;

  obj->_flags._side_parallel_overlap = _side_parallel_overlap;
}

bool dbTechLayerCutSpacingRule::isSideParallelOverlap() const
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;

  return obj->_flags._side_parallel_overlap;
}

void dbTechLayerCutSpacingRule::setExceptSameNet(bool _except_same_net)
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;

  obj->_flags._except_same_net = _except_same_net;
}

bool dbTechLayerCutSpacingRule::isExceptSameNet() const
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;

  return obj->_flags._except_same_net;
}

void dbTechLayerCutSpacingRule::setExceptSameMetal(bool _except_same_metal)
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;

  obj->_flags._except_same_metal = _except_same_metal;
}

bool dbTechLayerCutSpacingRule::isExceptSameMetal() const
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;

  return obj->_flags._except_same_metal;
}

void dbTechLayerCutSpacingRule::setExceptSameMetalOverlap(
    bool _except_same_metal_overlap)
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;

  obj->_flags._except_same_metal_overlap = _except_same_metal_overlap;
}

bool dbTechLayerCutSpacingRule::isExceptSameMetalOverlap() const
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;

  return obj->_flags._except_same_metal_overlap;
}

void dbTechLayerCutSpacingRule::setExceptSameVia(bool _except_same_via)
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;

  obj->_flags._except_same_via = _except_same_via;
}

bool dbTechLayerCutSpacingRule::isExceptSameVia() const
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;

  return obj->_flags._except_same_via;
}

void dbTechLayerCutSpacingRule::setAbove(bool _above)
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;

  obj->_flags._above = _above;
}

bool dbTechLayerCutSpacingRule::isAbove() const
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;

  return obj->_flags._above;
}

void dbTechLayerCutSpacingRule::setExceptTwoEdges(bool _except_two_edges)
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;

  obj->_flags._except_two_edges = _except_two_edges;
}

bool dbTechLayerCutSpacingRule::isExceptTwoEdges() const
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;

  return obj->_flags._except_two_edges;
}

void dbTechLayerCutSpacingRule::setTwoCutsValid(bool _two_cuts_valid)
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;

  obj->_flags._two_cuts_valid = _two_cuts_valid;
}

bool dbTechLayerCutSpacingRule::isTwoCutsValid() const
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;

  return obj->_flags._two_cuts_valid;
}

void dbTechLayerCutSpacingRule::setSameCut(bool _same_cut)
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;

  obj->_flags._same_cut = _same_cut;
}

bool dbTechLayerCutSpacingRule::isSameCut() const
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;

  return obj->_flags._same_cut;
}

void dbTechLayerCutSpacingRule::setLongEdgeOnly(bool _long_edge_only)
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;

  obj->_flags._long_edge_only = _long_edge_only;
}

bool dbTechLayerCutSpacingRule::isLongEdgeOnly() const
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;

  return obj->_flags._long_edge_only;
}

void dbTechLayerCutSpacingRule::setBelow(bool _below)
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;

  obj->_flags._below = _below;
}

bool dbTechLayerCutSpacingRule::isBelow() const
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;

  return obj->_flags._below;
}

void dbTechLayerCutSpacingRule::setParWithinEnclosureValid(
    bool _par_within_enclosure_valid)
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;

  obj->_flags._par_within_enclosure_valid = _par_within_enclosure_valid;
}

bool dbTechLayerCutSpacingRule::isParWithinEnclosureValid() const
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;

  return obj->_flags._par_within_enclosure_valid;
}

// User Code Begin dbTechLayerCutSpacingRulePublicMethods
dbTechLayerCutClassRule* dbTechLayerCutSpacingRule::getCutClass() const
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;
  if (obj->_cut_class == 0)
    return nullptr;
  _dbTechLayer* layer = (_dbTechLayer*) obj->getOwner();
  return (dbTechLayerCutClassRule*) layer->_cut_class_rules_tbl->getPtr(
      obj->_cut_class);
}

dbTechLayer* dbTechLayerCutSpacingRule::getSecondLayer() const
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;
  if (obj->_second_layer == 0)
    return nullptr;
  _dbTechLayer* layer = (_dbTechLayer*) obj->getOwner();
  _dbTech*      _tech = (_dbTech*) layer->getOwner();
  return (dbTechLayer*) _tech->_layer_tbl->getPtr(obj->_second_layer);
}

void dbTechLayerCutSpacingRule::setType(CutSpacingType _type)
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;

  obj->_flags._cut_spacing_type = (uint) _type;
}

dbTechLayerCutSpacingRule::CutSpacingType dbTechLayerCutSpacingRule::getType()
    const
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;

  return (dbTechLayerCutSpacingRule::CutSpacingType)
      obj->_flags._cut_spacing_type;
}

dbTechLayerCutSpacingRule* dbTechLayerCutSpacingRule::create(
    dbTechLayer* _layer)
{
  _dbTechLayer*               layer   = (_dbTechLayer*) _layer;
  _dbTechLayerCutSpacingRule* newrule = layer->_cut_spacing_rules_tbl->create();
  return ((dbTechLayerCutSpacingRule*) newrule);
}

dbTechLayerCutSpacingRule*
dbTechLayerCutSpacingRule::getTechLayerCutSpacingRule(dbTechLayer* inly,
                                                      uint         dbid)
{
  _dbTechLayer* layer = (_dbTechLayer*) inly;
  return (dbTechLayerCutSpacingRule*) layer->_cut_spacing_rules_tbl->getPtr(
      dbid);
}
void dbTechLayerCutSpacingRule::destroy(dbTechLayerCutSpacingRule* rule)
{
  _dbTechLayer* layer = (_dbTechLayer*) rule->getImpl()->getOwner();
  dbProperty::destroyProperties(rule);
  layer->_cut_spacing_rules_tbl->destroy((_dbTechLayerCutSpacingRule*) rule);
}
// User Code End dbTechLayerCutSpacingRulePublicMethods
}  // namespace odb
   // Generator Code End 1