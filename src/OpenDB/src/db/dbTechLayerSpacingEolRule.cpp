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
#include "dbTechLayerSpacingEolRule.h"

#include "db.h"
#include "dbDatabase.h"
#include "dbDiff.hpp"
#include "dbTable.h"
#include "dbTable.hpp"

// User Code Begin includes
#include "dbTech.h"
#include "dbTechLayer.h"
// User Code End includes
namespace odb {

// User Code Begin definitions
// User Code End definitions
template class dbTable<_dbTechLayerSpacingEolRule>;

bool _dbTechLayerSpacingEolRule::operator==(
    const _dbTechLayerSpacingEolRule& rhs) const
{
  if (_flags._exact_width_valid != rhs._flags._exact_width_valid)
    return false;

  if (_flags._wrong_dir_spacing_valid != rhs._flags._wrong_dir_spacing_valid)
    return false;

  if (_flags._opposite_width_valid != rhs._flags._opposite_width_valid)
    return false;

  if (_flags._within_valid != rhs._flags._within_valid)
    return false;

  if (_flags._wrong_dir_within_valid != rhs._flags._wrong_dir_within_valid)
    return false;

  if (_flags._same_mask_valid != rhs._flags._same_mask_valid)
    return false;

  if (_flags._except_exact_width_valid != rhs._flags._except_exact_width_valid)
    return false;

  if (_flags._fill_concave_corner_valid
      != rhs._flags._fill_concave_corner_valid)
    return false;

  if (_flags._withcut_valid != rhs._flags._withcut_valid)
    return false;

  if (_flags._cut_class_valid != rhs._flags._cut_class_valid)
    return false;

  if (_flags._with_cut_above_valid != rhs._flags._with_cut_above_valid)
    return false;

  if (_flags._enclosure_end_valid != rhs._flags._enclosure_end_valid)
    return false;

  if (_flags._enclosure_end_within_valid
      != rhs._flags._enclosure_end_within_valid)
    return false;

  if (_flags._end_prl_spacing_valid != rhs._flags._end_prl_spacing_valid)
    return false;

  if (_flags._prl_valid != rhs._flags._prl_valid)
    return false;

  if (_flags._end_to_end_valid != rhs._flags._end_to_end_valid)
    return false;

  if (_flags._cut_spaces_valid != rhs._flags._cut_spaces_valid)
    return false;

  if (_flags._extension_valid != rhs._flags._extension_valid)
    return false;

  if (_flags._wrong_dir_extension_valid
      != rhs._flags._wrong_dir_extension_valid)
    return false;

  if (_flags._other_end_width_valid != rhs._flags._other_end_width_valid)
    return false;

  if (_flags._max_length_valid != rhs._flags._max_length_valid)
    return false;

  if (_flags._min_length_valid != rhs._flags._min_length_valid)
    return false;

  if (_flags._two_sides_valid != rhs._flags._two_sides_valid)
    return false;

  if (_flags._equal_rect_width_valid != rhs._flags._equal_rect_width_valid)
    return false;

  if (_flags._parallel_edge_valid != rhs._flags._parallel_edge_valid)
    return false;

  if (_flags._subtract_eol_width_valid != rhs._flags._subtract_eol_width_valid)
    return false;

  if (_flags._par_prl_valid != rhs._flags._par_prl_valid)
    return false;

  if (_flags._par_min_length_valid != rhs._flags._par_min_length_valid)
    return false;

  if (_flags._two_edges_valid != rhs._flags._two_edges_valid)
    return false;

  if (_flags._same_metal_valid != rhs._flags._same_metal_valid)
    return false;

  if (_flags._non_eol_corner_only_valid
      != rhs._flags._non_eol_corner_only_valid)
    return false;

  if (_flags._parallel_same_mask_valid != rhs._flags._parallel_same_mask_valid)
    return false;

  if (_flags._enclose_cut_valid != rhs._flags._enclose_cut_valid)
    return false;

  if (_flags._below_valid != rhs._flags._below_valid)
    return false;

  if (_flags._above_valid != rhs._flags._above_valid)
    return false;

  if (_flags._cut_spacing_valid != rhs._flags._cut_spacing_valid)
    return false;

  if (_flags._all_cuts_valid != rhs._flags._all_cuts_valid)
    return false;

  if (_flags._to_concave_corner_valid != rhs._flags._to_concave_corner_valid)
    return false;

  if (_flags._min_adjacent_length_valid
      != rhs._flags._min_adjacent_length_valid)
    return false;

  if (_flags._two_min_adj_length_valid != rhs._flags._two_min_adj_length_valid)
    return false;

  if (_flags._to_notch_length_valid != rhs._flags._to_notch_length_valid)
    return false;

  if (_eol_space != rhs._eol_space)
    return false;

  if (_eol_width != rhs._eol_width)
    return false;

  if (_wrong_dir_space != rhs._wrong_dir_space)
    return false;

  if (_opposite_width != rhs._opposite_width)
    return false;

  if (_eol_within != rhs._eol_within)
    return false;

  if (_wrong_dir_within != rhs._wrong_dir_within)
    return false;

  if (_exact_width != rhs._exact_width)
    return false;

  if (_other_width != rhs._other_width)
    return false;

  if (_fill_triangle != rhs._fill_triangle)
    return false;

  if (_cut_class != rhs._cut_class)
    return false;

  if (_with_cut_space != rhs._with_cut_space)
    return false;

  if (_enclosure_end_width != rhs._enclosure_end_width)
    return false;

  if (_enclosure_end_within != rhs._enclosure_end_within)
    return false;

  if (_end_prl_space != rhs._end_prl_space)
    return false;

  if (_end_prl != rhs._end_prl)
    return false;

  if (_end_to_end_space != rhs._end_to_end_space)
    return false;

  if (_one_cut_space != rhs._one_cut_space)
    return false;

  if (_two_cut_space != rhs._two_cut_space)
    return false;

  if (_extension != rhs._extension)
    return false;

  if (_wrong_dir_extension != rhs._wrong_dir_extension)
    return false;

  if (_other_end_width != rhs._other_end_width)
    return false;

  if (_max_length != rhs._max_length)
    return false;

  if (_min_length != rhs._min_length)
    return false;

  if (_par_space != rhs._par_space)
    return false;

  if (_par_within != rhs._par_within)
    return false;

  if (_par_prl != rhs._par_prl)
    return false;

  if (_par_min_length != rhs._par_min_length)
    return false;

  if (_enclose_dist != rhs._enclose_dist)
    return false;

  if (_cut_to_metal_space != rhs._cut_to_metal_space)
    return false;

  if (_min_adj_length != rhs._min_adj_length)
    return false;

  if (_min_adj_length1 != rhs._min_adj_length1)
    return false;

  if (_min_adj_length2 != rhs._min_adj_length2)
    return false;

  if (_notch_length != rhs._notch_length)
    return false;

  // User Code Begin ==
  // User Code End ==
  return true;
}
bool _dbTechLayerSpacingEolRule::operator<(
    const _dbTechLayerSpacingEolRule& rhs) const
{
  // User Code Begin <
  // User Code End <
  return true;
}
void _dbTechLayerSpacingEolRule::differences(
    dbDiff&                           diff,
    const char*                       field,
    const _dbTechLayerSpacingEolRule& rhs) const
{
  DIFF_BEGIN

  DIFF_FIELD(_flags._exact_width_valid);
  DIFF_FIELD(_flags._wrong_dir_spacing_valid);
  DIFF_FIELD(_flags._opposite_width_valid);
  DIFF_FIELD(_flags._within_valid);
  DIFF_FIELD(_flags._wrong_dir_within_valid);
  DIFF_FIELD(_flags._same_mask_valid);
  DIFF_FIELD(_flags._except_exact_width_valid);
  DIFF_FIELD(_flags._fill_concave_corner_valid);
  DIFF_FIELD(_flags._withcut_valid);
  DIFF_FIELD(_flags._cut_class_valid);
  DIFF_FIELD(_flags._with_cut_above_valid);
  DIFF_FIELD(_flags._enclosure_end_valid);
  DIFF_FIELD(_flags._enclosure_end_within_valid);
  DIFF_FIELD(_flags._end_prl_spacing_valid);
  DIFF_FIELD(_flags._prl_valid);
  DIFF_FIELD(_flags._end_to_end_valid);
  DIFF_FIELD(_flags._cut_spaces_valid);
  DIFF_FIELD(_flags._extension_valid);
  DIFF_FIELD(_flags._wrong_dir_extension_valid);
  DIFF_FIELD(_flags._other_end_width_valid);
  DIFF_FIELD(_flags._max_length_valid);
  DIFF_FIELD(_flags._min_length_valid);
  DIFF_FIELD(_flags._two_sides_valid);
  DIFF_FIELD(_flags._equal_rect_width_valid);
  DIFF_FIELD(_flags._parallel_edge_valid);
  DIFF_FIELD(_flags._subtract_eol_width_valid);
  DIFF_FIELD(_flags._par_prl_valid);
  DIFF_FIELD(_flags._par_min_length_valid);
  DIFF_FIELD(_flags._two_edges_valid);
  DIFF_FIELD(_flags._same_metal_valid);
  DIFF_FIELD(_flags._non_eol_corner_only_valid);
  DIFF_FIELD(_flags._parallel_same_mask_valid);
  DIFF_FIELD(_flags._enclose_cut_valid);
  DIFF_FIELD(_flags._below_valid);
  DIFF_FIELD(_flags._above_valid);
  DIFF_FIELD(_flags._cut_spacing_valid);
  DIFF_FIELD(_flags._all_cuts_valid);
  DIFF_FIELD(_flags._to_concave_corner_valid);
  DIFF_FIELD(_flags._min_adjacent_length_valid);
  DIFF_FIELD(_flags._two_min_adj_length_valid);
  DIFF_FIELD(_flags._to_notch_length_valid);
  DIFF_FIELD(_eol_space);
  DIFF_FIELD(_eol_width);
  DIFF_FIELD(_wrong_dir_space);
  DIFF_FIELD(_opposite_width);
  DIFF_FIELD(_eol_within);
  DIFF_FIELD(_wrong_dir_within);
  DIFF_FIELD(_exact_width);
  DIFF_FIELD(_other_width);
  DIFF_FIELD(_fill_triangle);
  DIFF_FIELD(_cut_class);
  DIFF_FIELD(_with_cut_space);
  DIFF_FIELD(_enclosure_end_width);
  DIFF_FIELD(_enclosure_end_within);
  DIFF_FIELD(_end_prl_space);
  DIFF_FIELD(_end_prl);
  DIFF_FIELD(_end_to_end_space);
  DIFF_FIELD(_one_cut_space);
  DIFF_FIELD(_two_cut_space);
  DIFF_FIELD(_extension);
  DIFF_FIELD(_wrong_dir_extension);
  DIFF_FIELD(_other_end_width);
  DIFF_FIELD(_max_length);
  DIFF_FIELD(_min_length);
  DIFF_FIELD(_par_space);
  DIFF_FIELD(_par_within);
  DIFF_FIELD(_par_prl);
  DIFF_FIELD(_par_min_length);
  DIFF_FIELD(_enclose_dist);
  DIFF_FIELD(_cut_to_metal_space);
  DIFF_FIELD(_min_adj_length);
  DIFF_FIELD(_min_adj_length1);
  DIFF_FIELD(_min_adj_length2);
  DIFF_FIELD(_notch_length);
  // User Code Begin differences
  // User Code End differences
  DIFF_END
}
void _dbTechLayerSpacingEolRule::out(dbDiff&     diff,
                                     char        side,
                                     const char* field) const
{
  DIFF_OUT_BEGIN
  DIFF_OUT_FIELD(_flags._exact_width_valid);
  DIFF_OUT_FIELD(_flags._wrong_dir_spacing_valid);
  DIFF_OUT_FIELD(_flags._opposite_width_valid);
  DIFF_OUT_FIELD(_flags._within_valid);
  DIFF_OUT_FIELD(_flags._wrong_dir_within_valid);
  DIFF_OUT_FIELD(_flags._same_mask_valid);
  DIFF_OUT_FIELD(_flags._except_exact_width_valid);
  DIFF_OUT_FIELD(_flags._fill_concave_corner_valid);
  DIFF_OUT_FIELD(_flags._withcut_valid);
  DIFF_OUT_FIELD(_flags._cut_class_valid);
  DIFF_OUT_FIELD(_flags._with_cut_above_valid);
  DIFF_OUT_FIELD(_flags._enclosure_end_valid);
  DIFF_OUT_FIELD(_flags._enclosure_end_within_valid);
  DIFF_OUT_FIELD(_flags._end_prl_spacing_valid);
  DIFF_OUT_FIELD(_flags._prl_valid);
  DIFF_OUT_FIELD(_flags._end_to_end_valid);
  DIFF_OUT_FIELD(_flags._cut_spaces_valid);
  DIFF_OUT_FIELD(_flags._extension_valid);
  DIFF_OUT_FIELD(_flags._wrong_dir_extension_valid);
  DIFF_OUT_FIELD(_flags._other_end_width_valid);
  DIFF_OUT_FIELD(_flags._max_length_valid);
  DIFF_OUT_FIELD(_flags._min_length_valid);
  DIFF_OUT_FIELD(_flags._two_sides_valid);
  DIFF_OUT_FIELD(_flags._equal_rect_width_valid);
  DIFF_OUT_FIELD(_flags._parallel_edge_valid);
  DIFF_OUT_FIELD(_flags._subtract_eol_width_valid);
  DIFF_OUT_FIELD(_flags._par_prl_valid);
  DIFF_OUT_FIELD(_flags._par_min_length_valid);
  DIFF_OUT_FIELD(_flags._two_edges_valid);
  DIFF_OUT_FIELD(_flags._same_metal_valid);
  DIFF_OUT_FIELD(_flags._non_eol_corner_only_valid);
  DIFF_OUT_FIELD(_flags._parallel_same_mask_valid);
  DIFF_OUT_FIELD(_flags._enclose_cut_valid);
  DIFF_OUT_FIELD(_flags._below_valid);
  DIFF_OUT_FIELD(_flags._above_valid);
  DIFF_OUT_FIELD(_flags._cut_spacing_valid);
  DIFF_OUT_FIELD(_flags._all_cuts_valid);
  DIFF_OUT_FIELD(_flags._to_concave_corner_valid);
  DIFF_OUT_FIELD(_flags._min_adjacent_length_valid);
  DIFF_OUT_FIELD(_flags._two_min_adj_length_valid);
  DIFF_OUT_FIELD(_flags._to_notch_length_valid);
  DIFF_OUT_FIELD(_eol_space);
  DIFF_OUT_FIELD(_eol_width);
  DIFF_OUT_FIELD(_wrong_dir_space);
  DIFF_OUT_FIELD(_opposite_width);
  DIFF_OUT_FIELD(_eol_within);
  DIFF_OUT_FIELD(_wrong_dir_within);
  DIFF_OUT_FIELD(_exact_width);
  DIFF_OUT_FIELD(_other_width);
  DIFF_OUT_FIELD(_fill_triangle);
  DIFF_OUT_FIELD(_cut_class);
  DIFF_OUT_FIELD(_with_cut_space);
  DIFF_OUT_FIELD(_enclosure_end_width);
  DIFF_OUT_FIELD(_enclosure_end_within);
  DIFF_OUT_FIELD(_end_prl_space);
  DIFF_OUT_FIELD(_end_prl);
  DIFF_OUT_FIELD(_end_to_end_space);
  DIFF_OUT_FIELD(_one_cut_space);
  DIFF_OUT_FIELD(_two_cut_space);
  DIFF_OUT_FIELD(_extension);
  DIFF_OUT_FIELD(_wrong_dir_extension);
  DIFF_OUT_FIELD(_other_end_width);
  DIFF_OUT_FIELD(_max_length);
  DIFF_OUT_FIELD(_min_length);
  DIFF_OUT_FIELD(_par_space);
  DIFF_OUT_FIELD(_par_within);
  DIFF_OUT_FIELD(_par_prl);
  DIFF_OUT_FIELD(_par_min_length);
  DIFF_OUT_FIELD(_enclose_dist);
  DIFF_OUT_FIELD(_cut_to_metal_space);
  DIFF_OUT_FIELD(_min_adj_length);
  DIFF_OUT_FIELD(_min_adj_length1);
  DIFF_OUT_FIELD(_min_adj_length2);
  DIFF_OUT_FIELD(_notch_length);

  // User Code Begin out
  // User Code End out
  DIFF_END
}
_dbTechLayerSpacingEolRule::_dbTechLayerSpacingEolRule(_dbDatabase* db)
{
  uint64_t* _flags_bit_field = (uint64_t*) &_flags;
  *_flags_bit_field          = 0;
  // User Code Begin constructor
  _eol_space            = 0;
  _eol_width            = 0;
  _wrong_dir_space      = 0;
  _opposite_width       = 0;
  _eol_within           = 0;
  _wrong_dir_within     = 0;
  _exact_width          = 0;
  _other_width          = 0;
  _fill_triangle        = 0;
  _cut_class            = 0;
  _with_cut_space       = 0;
  _enclosure_end_width  = 0;
  _enclosure_end_within = 0;
  _end_prl_space        = 0;
  _end_prl              = 0;
  _end_to_end_space     = 0;
  _one_cut_space        = 0;
  _two_cut_space        = 0;
  _extension            = 0;
  _wrong_dir_extension  = 0;
  _other_end_width      = 0;
  _max_length           = 0;
  _min_length           = 0;
  _par_space            = 0;
  _par_within           = 0;
  _par_prl              = 0;
  _par_min_length       = 0;
  _enclose_dist         = 0;
  _cut_to_metal_space   = 0;
  _min_adj_length       = 0;
  _min_adj_length1      = 0;
  _min_adj_length2      = 0;
  _notch_length         = 0;
  // User Code End constructor
}
_dbTechLayerSpacingEolRule::_dbTechLayerSpacingEolRule(
    _dbDatabase*                      db,
    const _dbTechLayerSpacingEolRule& r)
{
  _flags._exact_width_valid          = r._flags._exact_width_valid;
  _flags._wrong_dir_spacing_valid    = r._flags._wrong_dir_spacing_valid;
  _flags._opposite_width_valid       = r._flags._opposite_width_valid;
  _flags._within_valid               = r._flags._within_valid;
  _flags._wrong_dir_within_valid     = r._flags._wrong_dir_within_valid;
  _flags._same_mask_valid            = r._flags._same_mask_valid;
  _flags._except_exact_width_valid   = r._flags._except_exact_width_valid;
  _flags._fill_concave_corner_valid  = r._flags._fill_concave_corner_valid;
  _flags._withcut_valid              = r._flags._withcut_valid;
  _flags._cut_class_valid            = r._flags._cut_class_valid;
  _flags._with_cut_above_valid       = r._flags._with_cut_above_valid;
  _flags._enclosure_end_valid        = r._flags._enclosure_end_valid;
  _flags._enclosure_end_within_valid = r._flags._enclosure_end_within_valid;
  _flags._end_prl_spacing_valid      = r._flags._end_prl_spacing_valid;
  _flags._prl_valid                  = r._flags._prl_valid;
  _flags._end_to_end_valid           = r._flags._end_to_end_valid;
  _flags._cut_spaces_valid           = r._flags._cut_spaces_valid;
  _flags._extension_valid            = r._flags._extension_valid;
  _flags._wrong_dir_extension_valid  = r._flags._wrong_dir_extension_valid;
  _flags._other_end_width_valid      = r._flags._other_end_width_valid;
  _flags._max_length_valid           = r._flags._max_length_valid;
  _flags._min_length_valid           = r._flags._min_length_valid;
  _flags._two_sides_valid            = r._flags._two_sides_valid;
  _flags._equal_rect_width_valid     = r._flags._equal_rect_width_valid;
  _flags._parallel_edge_valid        = r._flags._parallel_edge_valid;
  _flags._subtract_eol_width_valid   = r._flags._subtract_eol_width_valid;
  _flags._par_prl_valid              = r._flags._par_prl_valid;
  _flags._par_min_length_valid       = r._flags._par_min_length_valid;
  _flags._two_edges_valid            = r._flags._two_edges_valid;
  _flags._same_metal_valid           = r._flags._same_metal_valid;
  _flags._non_eol_corner_only_valid  = r._flags._non_eol_corner_only_valid;
  _flags._parallel_same_mask_valid   = r._flags._parallel_same_mask_valid;
  _flags._enclose_cut_valid          = r._flags._enclose_cut_valid;
  _flags._below_valid                = r._flags._below_valid;
  _flags._above_valid                = r._flags._above_valid;
  _flags._cut_spacing_valid          = r._flags._cut_spacing_valid;
  _flags._all_cuts_valid             = r._flags._all_cuts_valid;
  _flags._to_concave_corner_valid    = r._flags._to_concave_corner_valid;
  _flags._min_adjacent_length_valid  = r._flags._min_adjacent_length_valid;
  _flags._two_min_adj_length_valid   = r._flags._two_min_adj_length_valid;
  _flags._to_notch_length_valid      = r._flags._to_notch_length_valid;
  _flags._spare_bits                 = r._flags._spare_bits;
  _eol_space                         = r._eol_space;
  _eol_width                         = r._eol_width;
  _wrong_dir_space                   = r._wrong_dir_space;
  _opposite_width                    = r._opposite_width;
  _eol_within                        = r._eol_within;
  _wrong_dir_within                  = r._wrong_dir_within;
  _exact_width                       = r._exact_width;
  _other_width                       = r._other_width;
  _fill_triangle                     = r._fill_triangle;
  _cut_class                         = r._cut_class;
  _with_cut_space                    = r._with_cut_space;
  _enclosure_end_width               = r._enclosure_end_width;
  _enclosure_end_within              = r._enclosure_end_within;
  _end_prl_space                     = r._end_prl_space;
  _end_prl                           = r._end_prl;
  _end_to_end_space                  = r._end_to_end_space;
  _one_cut_space                     = r._one_cut_space;
  _two_cut_space                     = r._two_cut_space;
  _extension                         = r._extension;
  _wrong_dir_extension               = r._wrong_dir_extension;
  _other_end_width                   = r._other_end_width;
  _max_length                        = r._max_length;
  _min_length                        = r._min_length;
  _par_space                         = r._par_space;
  _par_within                        = r._par_within;
  _par_prl                           = r._par_prl;
  _par_min_length                    = r._par_min_length;
  _enclose_dist                      = r._enclose_dist;
  _cut_to_metal_space                = r._cut_to_metal_space;
  _min_adj_length                    = r._min_adj_length;
  _min_adj_length1                   = r._min_adj_length1;
  _min_adj_length2                   = r._min_adj_length2;
  _notch_length                      = r._notch_length;
  // User Code Begin CopyConstructor
  // User Code End CopyConstructor
}

dbIStream& operator>>(dbIStream& stream, _dbTechLayerSpacingEolRule& obj)
{
  uint64_t* _flags_bit_field = (uint64_t*) &obj._flags;
  stream >> *_flags_bit_field;
  stream >> obj._eol_space;
  stream >> obj._eol_width;
  stream >> obj._wrong_dir_space;
  stream >> obj._opposite_width;
  stream >> obj._eol_within;
  stream >> obj._wrong_dir_within;
  stream >> obj._exact_width;
  stream >> obj._other_width;
  stream >> obj._fill_triangle;
  stream >> obj._cut_class;
  stream >> obj._with_cut_space;
  stream >> obj._enclosure_end_width;
  stream >> obj._enclosure_end_within;
  stream >> obj._end_prl_space;
  stream >> obj._end_prl;
  stream >> obj._end_to_end_space;
  stream >> obj._one_cut_space;
  stream >> obj._two_cut_space;
  stream >> obj._extension;
  stream >> obj._wrong_dir_extension;
  stream >> obj._other_end_width;
  stream >> obj._max_length;
  stream >> obj._min_length;
  stream >> obj._par_space;
  stream >> obj._par_within;
  stream >> obj._par_prl;
  stream >> obj._par_min_length;
  stream >> obj._enclose_dist;
  stream >> obj._cut_to_metal_space;
  stream >> obj._min_adj_length;
  stream >> obj._min_adj_length1;
  stream >> obj._min_adj_length2;
  stream >> obj._notch_length;
  // User Code Begin >>
  // User Code End >>
  return stream;
}
dbOStream& operator<<(dbOStream& stream, const _dbTechLayerSpacingEolRule& obj)
{
  uint64_t* _flags_bit_field = (uint64_t*) &obj._flags;
  stream << *_flags_bit_field;
  stream << obj._eol_space;
  stream << obj._eol_width;
  stream << obj._wrong_dir_space;
  stream << obj._opposite_width;
  stream << obj._eol_within;
  stream << obj._wrong_dir_within;
  stream << obj._exact_width;
  stream << obj._other_width;
  stream << obj._fill_triangle;
  stream << obj._cut_class;
  stream << obj._with_cut_space;
  stream << obj._enclosure_end_width;
  stream << obj._enclosure_end_within;
  stream << obj._end_prl_space;
  stream << obj._end_prl;
  stream << obj._end_to_end_space;
  stream << obj._one_cut_space;
  stream << obj._two_cut_space;
  stream << obj._extension;
  stream << obj._wrong_dir_extension;
  stream << obj._other_end_width;
  stream << obj._max_length;
  stream << obj._min_length;
  stream << obj._par_space;
  stream << obj._par_within;
  stream << obj._par_prl;
  stream << obj._par_min_length;
  stream << obj._enclose_dist;
  stream << obj._cut_to_metal_space;
  stream << obj._min_adj_length;
  stream << obj._min_adj_length1;
  stream << obj._min_adj_length2;
  stream << obj._notch_length;
  // User Code Begin <<
  // User Code End <<
  return stream;
}

_dbTechLayerSpacingEolRule::~_dbTechLayerSpacingEolRule()
{
  // User Code Begin Destructor
  // User Code End Destructor
}
////////////////////////////////////////////////////////////////////
//
// dbTechLayerSpacingEolRule - Methods
//
////////////////////////////////////////////////////////////////////

void dbTechLayerSpacingEolRule::setEolSpace(int _eol_space)
{
  _dbTechLayerSpacingEolRule* obj = (_dbTechLayerSpacingEolRule*) this;

  obj->_eol_space = _eol_space;
}

int dbTechLayerSpacingEolRule::getEolSpace() const
{
  _dbTechLayerSpacingEolRule* obj = (_dbTechLayerSpacingEolRule*) this;
  return obj->_eol_space;
}

void dbTechLayerSpacingEolRule::setEolWidth(int _eol_width)
{
  _dbTechLayerSpacingEolRule* obj = (_dbTechLayerSpacingEolRule*) this;

  obj->_eol_width = _eol_width;
}

int dbTechLayerSpacingEolRule::getEolWidth() const
{
  _dbTechLayerSpacingEolRule* obj = (_dbTechLayerSpacingEolRule*) this;
  return obj->_eol_width;
}

void dbTechLayerSpacingEolRule::setWrongDirSpace(int _wrong_dir_space)
{
  _dbTechLayerSpacingEolRule* obj = (_dbTechLayerSpacingEolRule*) this;

  obj->_wrong_dir_space = _wrong_dir_space;
}

int dbTechLayerSpacingEolRule::getWrongDirSpace() const
{
  _dbTechLayerSpacingEolRule* obj = (_dbTechLayerSpacingEolRule*) this;
  return obj->_wrong_dir_space;
}

void dbTechLayerSpacingEolRule::setOppositeWidth(int _opposite_width)
{
  _dbTechLayerSpacingEolRule* obj = (_dbTechLayerSpacingEolRule*) this;

  obj->_opposite_width = _opposite_width;
}

int dbTechLayerSpacingEolRule::getOppositeWidth() const
{
  _dbTechLayerSpacingEolRule* obj = (_dbTechLayerSpacingEolRule*) this;
  return obj->_opposite_width;
}

void dbTechLayerSpacingEolRule::setEolWithin(int _eol_within)
{
  _dbTechLayerSpacingEolRule* obj = (_dbTechLayerSpacingEolRule*) this;

  obj->_eol_within = _eol_within;
}

int dbTechLayerSpacingEolRule::getEolWithin() const
{
  _dbTechLayerSpacingEolRule* obj = (_dbTechLayerSpacingEolRule*) this;
  return obj->_eol_within;
}

void dbTechLayerSpacingEolRule::setWrongDirWithin(int _wrong_dir_within)
{
  _dbTechLayerSpacingEolRule* obj = (_dbTechLayerSpacingEolRule*) this;

  obj->_wrong_dir_within = _wrong_dir_within;
}

int dbTechLayerSpacingEolRule::getWrongDirWithin() const
{
  _dbTechLayerSpacingEolRule* obj = (_dbTechLayerSpacingEolRule*) this;
  return obj->_wrong_dir_within;
}

void dbTechLayerSpacingEolRule::setExactWidth(int _exact_width)
{
  _dbTechLayerSpacingEolRule* obj = (_dbTechLayerSpacingEolRule*) this;

  obj->_exact_width = _exact_width;
}

int dbTechLayerSpacingEolRule::getExactWidth() const
{
  _dbTechLayerSpacingEolRule* obj = (_dbTechLayerSpacingEolRule*) this;
  return obj->_exact_width;
}

void dbTechLayerSpacingEolRule::setOtherWidth(int _other_width)
{
  _dbTechLayerSpacingEolRule* obj = (_dbTechLayerSpacingEolRule*) this;

  obj->_other_width = _other_width;
}

int dbTechLayerSpacingEolRule::getOtherWidth() const
{
  _dbTechLayerSpacingEolRule* obj = (_dbTechLayerSpacingEolRule*) this;
  return obj->_other_width;
}

void dbTechLayerSpacingEolRule::setFillTriangle(int _fill_triangle)
{
  _dbTechLayerSpacingEolRule* obj = (_dbTechLayerSpacingEolRule*) this;

  obj->_fill_triangle = _fill_triangle;
}

int dbTechLayerSpacingEolRule::getFillTriangle() const
{
  _dbTechLayerSpacingEolRule* obj = (_dbTechLayerSpacingEolRule*) this;
  return obj->_fill_triangle;
}

void dbTechLayerSpacingEolRule::setCutClass(int _cut_class)
{
  _dbTechLayerSpacingEolRule* obj = (_dbTechLayerSpacingEolRule*) this;

  obj->_cut_class = _cut_class;
}

int dbTechLayerSpacingEolRule::getCutClass() const
{
  _dbTechLayerSpacingEolRule* obj = (_dbTechLayerSpacingEolRule*) this;
  return obj->_cut_class;
}

void dbTechLayerSpacingEolRule::setWithCutSpace(int _with_cut_space)
{
  _dbTechLayerSpacingEolRule* obj = (_dbTechLayerSpacingEolRule*) this;

  obj->_with_cut_space = _with_cut_space;
}

int dbTechLayerSpacingEolRule::getWithCutSpace() const
{
  _dbTechLayerSpacingEolRule* obj = (_dbTechLayerSpacingEolRule*) this;
  return obj->_with_cut_space;
}

void dbTechLayerSpacingEolRule::setEnclosureEndWidth(int _enclosure_end_width)
{
  _dbTechLayerSpacingEolRule* obj = (_dbTechLayerSpacingEolRule*) this;

  obj->_enclosure_end_width = _enclosure_end_width;
}

int dbTechLayerSpacingEolRule::getEnclosureEndWidth() const
{
  _dbTechLayerSpacingEolRule* obj = (_dbTechLayerSpacingEolRule*) this;
  return obj->_enclosure_end_width;
}

void dbTechLayerSpacingEolRule::setEnclosureEndWithin(int _enclosure_end_within)
{
  _dbTechLayerSpacingEolRule* obj = (_dbTechLayerSpacingEolRule*) this;

  obj->_enclosure_end_within = _enclosure_end_within;
}

int dbTechLayerSpacingEolRule::getEnclosureEndWithin() const
{
  _dbTechLayerSpacingEolRule* obj = (_dbTechLayerSpacingEolRule*) this;
  return obj->_enclosure_end_within;
}

void dbTechLayerSpacingEolRule::setEndPrlSpace(int _end_prl_space)
{
  _dbTechLayerSpacingEolRule* obj = (_dbTechLayerSpacingEolRule*) this;

  obj->_end_prl_space = _end_prl_space;
}

int dbTechLayerSpacingEolRule::getEndPrlSpace() const
{
  _dbTechLayerSpacingEolRule* obj = (_dbTechLayerSpacingEolRule*) this;
  return obj->_end_prl_space;
}

void dbTechLayerSpacingEolRule::setEndPrl(int _end_prl)
{
  _dbTechLayerSpacingEolRule* obj = (_dbTechLayerSpacingEolRule*) this;

  obj->_end_prl = _end_prl;
}

int dbTechLayerSpacingEolRule::getEndPrl() const
{
  _dbTechLayerSpacingEolRule* obj = (_dbTechLayerSpacingEolRule*) this;
  return obj->_end_prl;
}

void dbTechLayerSpacingEolRule::setEndToEndSpace(int _end_to_end_space)
{
  _dbTechLayerSpacingEolRule* obj = (_dbTechLayerSpacingEolRule*) this;

  obj->_end_to_end_space = _end_to_end_space;
}

int dbTechLayerSpacingEolRule::getEndToEndSpace() const
{
  _dbTechLayerSpacingEolRule* obj = (_dbTechLayerSpacingEolRule*) this;
  return obj->_end_to_end_space;
}

void dbTechLayerSpacingEolRule::setOneCutSpace(int _one_cut_space)
{
  _dbTechLayerSpacingEolRule* obj = (_dbTechLayerSpacingEolRule*) this;

  obj->_one_cut_space = _one_cut_space;
}

int dbTechLayerSpacingEolRule::getOneCutSpace() const
{
  _dbTechLayerSpacingEolRule* obj = (_dbTechLayerSpacingEolRule*) this;
  return obj->_one_cut_space;
}

void dbTechLayerSpacingEolRule::setTwoCutSpace(int _two_cut_space)
{
  _dbTechLayerSpacingEolRule* obj = (_dbTechLayerSpacingEolRule*) this;

  obj->_two_cut_space = _two_cut_space;
}

int dbTechLayerSpacingEolRule::getTwoCutSpace() const
{
  _dbTechLayerSpacingEolRule* obj = (_dbTechLayerSpacingEolRule*) this;
  return obj->_two_cut_space;
}

void dbTechLayerSpacingEolRule::setExtension(int _extension)
{
  _dbTechLayerSpacingEolRule* obj = (_dbTechLayerSpacingEolRule*) this;

  obj->_extension = _extension;
}

int dbTechLayerSpacingEolRule::getExtension() const
{
  _dbTechLayerSpacingEolRule* obj = (_dbTechLayerSpacingEolRule*) this;
  return obj->_extension;
}

void dbTechLayerSpacingEolRule::setWrongDirExtension(int _wrong_dir_extension)
{
  _dbTechLayerSpacingEolRule* obj = (_dbTechLayerSpacingEolRule*) this;

  obj->_wrong_dir_extension = _wrong_dir_extension;
}

int dbTechLayerSpacingEolRule::getWrongDirExtension() const
{
  _dbTechLayerSpacingEolRule* obj = (_dbTechLayerSpacingEolRule*) this;
  return obj->_wrong_dir_extension;
}

void dbTechLayerSpacingEolRule::setOtherEndWidth(int _other_end_width)
{
  _dbTechLayerSpacingEolRule* obj = (_dbTechLayerSpacingEolRule*) this;

  obj->_other_end_width = _other_end_width;
}

int dbTechLayerSpacingEolRule::getOtherEndWidth() const
{
  _dbTechLayerSpacingEolRule* obj = (_dbTechLayerSpacingEolRule*) this;
  return obj->_other_end_width;
}

void dbTechLayerSpacingEolRule::setMaxLength(int _max_length)
{
  _dbTechLayerSpacingEolRule* obj = (_dbTechLayerSpacingEolRule*) this;

  obj->_max_length = _max_length;
}

int dbTechLayerSpacingEolRule::getMaxLength() const
{
  _dbTechLayerSpacingEolRule* obj = (_dbTechLayerSpacingEolRule*) this;
  return obj->_max_length;
}

void dbTechLayerSpacingEolRule::setMinLength(int _min_length)
{
  _dbTechLayerSpacingEolRule* obj = (_dbTechLayerSpacingEolRule*) this;

  obj->_min_length = _min_length;
}

int dbTechLayerSpacingEolRule::getMinLength() const
{
  _dbTechLayerSpacingEolRule* obj = (_dbTechLayerSpacingEolRule*) this;
  return obj->_min_length;
}

void dbTechLayerSpacingEolRule::setParSpace(int _par_space)
{
  _dbTechLayerSpacingEolRule* obj = (_dbTechLayerSpacingEolRule*) this;

  obj->_par_space = _par_space;
}

int dbTechLayerSpacingEolRule::getParSpace() const
{
  _dbTechLayerSpacingEolRule* obj = (_dbTechLayerSpacingEolRule*) this;
  return obj->_par_space;
}

void dbTechLayerSpacingEolRule::setParWithin(int _par_within)
{
  _dbTechLayerSpacingEolRule* obj = (_dbTechLayerSpacingEolRule*) this;

  obj->_par_within = _par_within;
}

int dbTechLayerSpacingEolRule::getParWithin() const
{
  _dbTechLayerSpacingEolRule* obj = (_dbTechLayerSpacingEolRule*) this;
  return obj->_par_within;
}

void dbTechLayerSpacingEolRule::setParPrl(int _par_prl)
{
  _dbTechLayerSpacingEolRule* obj = (_dbTechLayerSpacingEolRule*) this;

  obj->_par_prl = _par_prl;
}

int dbTechLayerSpacingEolRule::getParPrl() const
{
  _dbTechLayerSpacingEolRule* obj = (_dbTechLayerSpacingEolRule*) this;
  return obj->_par_prl;
}

void dbTechLayerSpacingEolRule::setParMinLength(int _par_min_length)
{
  _dbTechLayerSpacingEolRule* obj = (_dbTechLayerSpacingEolRule*) this;

  obj->_par_min_length = _par_min_length;
}

int dbTechLayerSpacingEolRule::getParMinLength() const
{
  _dbTechLayerSpacingEolRule* obj = (_dbTechLayerSpacingEolRule*) this;
  return obj->_par_min_length;
}

void dbTechLayerSpacingEolRule::setEncloseDist(int _enclose_dist)
{
  _dbTechLayerSpacingEolRule* obj = (_dbTechLayerSpacingEolRule*) this;

  obj->_enclose_dist = _enclose_dist;
}

int dbTechLayerSpacingEolRule::getEncloseDist() const
{
  _dbTechLayerSpacingEolRule* obj = (_dbTechLayerSpacingEolRule*) this;
  return obj->_enclose_dist;
}

void dbTechLayerSpacingEolRule::setCutToMetalSpace(int _cut_to_metal_space)
{
  _dbTechLayerSpacingEolRule* obj = (_dbTechLayerSpacingEolRule*) this;

  obj->_cut_to_metal_space = _cut_to_metal_space;
}

int dbTechLayerSpacingEolRule::getCutToMetalSpace() const
{
  _dbTechLayerSpacingEolRule* obj = (_dbTechLayerSpacingEolRule*) this;
  return obj->_cut_to_metal_space;
}

void dbTechLayerSpacingEolRule::setMinAdjLength(int _min_adj_length)
{
  _dbTechLayerSpacingEolRule* obj = (_dbTechLayerSpacingEolRule*) this;

  obj->_min_adj_length = _min_adj_length;
}

int dbTechLayerSpacingEolRule::getMinAdjLength() const
{
  _dbTechLayerSpacingEolRule* obj = (_dbTechLayerSpacingEolRule*) this;
  return obj->_min_adj_length;
}

void dbTechLayerSpacingEolRule::setMinAdjLength1(int _min_adj_length1)
{
  _dbTechLayerSpacingEolRule* obj = (_dbTechLayerSpacingEolRule*) this;

  obj->_min_adj_length1 = _min_adj_length1;
}

int dbTechLayerSpacingEolRule::getMinAdjLength1() const
{
  _dbTechLayerSpacingEolRule* obj = (_dbTechLayerSpacingEolRule*) this;
  return obj->_min_adj_length1;
}

void dbTechLayerSpacingEolRule::setMinAdjLength2(int _min_adj_length2)
{
  _dbTechLayerSpacingEolRule* obj = (_dbTechLayerSpacingEolRule*) this;

  obj->_min_adj_length2 = _min_adj_length2;
}

int dbTechLayerSpacingEolRule::getMinAdjLength2() const
{
  _dbTechLayerSpacingEolRule* obj = (_dbTechLayerSpacingEolRule*) this;
  return obj->_min_adj_length2;
}

void dbTechLayerSpacingEolRule::setNotchLength(int _notch_length)
{
  _dbTechLayerSpacingEolRule* obj = (_dbTechLayerSpacingEolRule*) this;

  obj->_notch_length = _notch_length;
}

int dbTechLayerSpacingEolRule::getNotchLength() const
{
  _dbTechLayerSpacingEolRule* obj = (_dbTechLayerSpacingEolRule*) this;
  return obj->_notch_length;
}

void dbTechLayerSpacingEolRule::setExactWidthValid(bool _exact_width_valid)
{
  _dbTechLayerSpacingEolRule* obj = (_dbTechLayerSpacingEolRule*) this;

  obj->_flags._exact_width_valid = _exact_width_valid;
}

bool dbTechLayerSpacingEolRule::isExactWidthValid() const
{
  _dbTechLayerSpacingEolRule* obj = (_dbTechLayerSpacingEolRule*) this;

  return obj->_flags._exact_width_valid;
}

void dbTechLayerSpacingEolRule::setWrongDirSpacingValid(
    bool _wrong_dir_spacing_valid)
{
  _dbTechLayerSpacingEolRule* obj = (_dbTechLayerSpacingEolRule*) this;

  obj->_flags._wrong_dir_spacing_valid = _wrong_dir_spacing_valid;
}

bool dbTechLayerSpacingEolRule::isWrongDirSpacingValid() const
{
  _dbTechLayerSpacingEolRule* obj = (_dbTechLayerSpacingEolRule*) this;

  return obj->_flags._wrong_dir_spacing_valid;
}

void dbTechLayerSpacingEolRule::setOppositeWidthValid(
    bool _opposite_width_valid)
{
  _dbTechLayerSpacingEolRule* obj = (_dbTechLayerSpacingEolRule*) this;

  obj->_flags._opposite_width_valid = _opposite_width_valid;
}

bool dbTechLayerSpacingEolRule::isOppositeWidthValid() const
{
  _dbTechLayerSpacingEolRule* obj = (_dbTechLayerSpacingEolRule*) this;

  return obj->_flags._opposite_width_valid;
}

void dbTechLayerSpacingEolRule::setWithinValid(bool _within_valid)
{
  _dbTechLayerSpacingEolRule* obj = (_dbTechLayerSpacingEolRule*) this;

  obj->_flags._within_valid = _within_valid;
}

bool dbTechLayerSpacingEolRule::isWithinValid() const
{
  _dbTechLayerSpacingEolRule* obj = (_dbTechLayerSpacingEolRule*) this;

  return obj->_flags._within_valid;
}

void dbTechLayerSpacingEolRule::setWrongDirWithinValid(
    bool _wrong_dir_within_valid)
{
  _dbTechLayerSpacingEolRule* obj = (_dbTechLayerSpacingEolRule*) this;

  obj->_flags._wrong_dir_within_valid = _wrong_dir_within_valid;
}

bool dbTechLayerSpacingEolRule::isWrongDirWithinValid() const
{
  _dbTechLayerSpacingEolRule* obj = (_dbTechLayerSpacingEolRule*) this;

  return obj->_flags._wrong_dir_within_valid;
}

void dbTechLayerSpacingEolRule::setSameMaskValid(bool _same_mask_valid)
{
  _dbTechLayerSpacingEolRule* obj = (_dbTechLayerSpacingEolRule*) this;

  obj->_flags._same_mask_valid = _same_mask_valid;
}

bool dbTechLayerSpacingEolRule::isSameMaskValid() const
{
  _dbTechLayerSpacingEolRule* obj = (_dbTechLayerSpacingEolRule*) this;

  return obj->_flags._same_mask_valid;
}

void dbTechLayerSpacingEolRule::setExceptExactWidthValid(
    bool _except_exact_width_valid)
{
  _dbTechLayerSpacingEolRule* obj = (_dbTechLayerSpacingEolRule*) this;

  obj->_flags._except_exact_width_valid = _except_exact_width_valid;
}

bool dbTechLayerSpacingEolRule::isExceptExactWidthValid() const
{
  _dbTechLayerSpacingEolRule* obj = (_dbTechLayerSpacingEolRule*) this;

  return obj->_flags._except_exact_width_valid;
}

void dbTechLayerSpacingEolRule::setFillConcaveCornerValid(
    bool _fill_concave_corner_valid)
{
  _dbTechLayerSpacingEolRule* obj = (_dbTechLayerSpacingEolRule*) this;

  obj->_flags._fill_concave_corner_valid = _fill_concave_corner_valid;
}

bool dbTechLayerSpacingEolRule::isFillConcaveCornerValid() const
{
  _dbTechLayerSpacingEolRule* obj = (_dbTechLayerSpacingEolRule*) this;

  return obj->_flags._fill_concave_corner_valid;
}

void dbTechLayerSpacingEolRule::setWithcutValid(bool _withcut_valid)
{
  _dbTechLayerSpacingEolRule* obj = (_dbTechLayerSpacingEolRule*) this;

  obj->_flags._withcut_valid = _withcut_valid;
}

bool dbTechLayerSpacingEolRule::isWithcutValid() const
{
  _dbTechLayerSpacingEolRule* obj = (_dbTechLayerSpacingEolRule*) this;

  return obj->_flags._withcut_valid;
}

void dbTechLayerSpacingEolRule::setCutClassValid(bool _cut_class_valid)
{
  _dbTechLayerSpacingEolRule* obj = (_dbTechLayerSpacingEolRule*) this;

  obj->_flags._cut_class_valid = _cut_class_valid;
}

bool dbTechLayerSpacingEolRule::isCutClassValid() const
{
  _dbTechLayerSpacingEolRule* obj = (_dbTechLayerSpacingEolRule*) this;

  return obj->_flags._cut_class_valid;
}

void dbTechLayerSpacingEolRule::setWithCutAboveValid(bool _with_cut_above_valid)
{
  _dbTechLayerSpacingEolRule* obj = (_dbTechLayerSpacingEolRule*) this;

  obj->_flags._with_cut_above_valid = _with_cut_above_valid;
}

bool dbTechLayerSpacingEolRule::isWithCutAboveValid() const
{
  _dbTechLayerSpacingEolRule* obj = (_dbTechLayerSpacingEolRule*) this;

  return obj->_flags._with_cut_above_valid;
}

void dbTechLayerSpacingEolRule::setEnclosureEndValid(bool _enclosure_end_valid)
{
  _dbTechLayerSpacingEolRule* obj = (_dbTechLayerSpacingEolRule*) this;

  obj->_flags._enclosure_end_valid = _enclosure_end_valid;
}

bool dbTechLayerSpacingEolRule::isEnclosureEndValid() const
{
  _dbTechLayerSpacingEolRule* obj = (_dbTechLayerSpacingEolRule*) this;

  return obj->_flags._enclosure_end_valid;
}

void dbTechLayerSpacingEolRule::setEnclosureEndWithinValid(
    bool _enclosure_end_within_valid)
{
  _dbTechLayerSpacingEolRule* obj = (_dbTechLayerSpacingEolRule*) this;

  obj->_flags._enclosure_end_within_valid = _enclosure_end_within_valid;
}

bool dbTechLayerSpacingEolRule::isEnclosureEndWithinValid() const
{
  _dbTechLayerSpacingEolRule* obj = (_dbTechLayerSpacingEolRule*) this;

  return obj->_flags._enclosure_end_within_valid;
}

void dbTechLayerSpacingEolRule::setEndPrlSpacingValid(
    bool _end_prl_spacing_valid)
{
  _dbTechLayerSpacingEolRule* obj = (_dbTechLayerSpacingEolRule*) this;

  obj->_flags._end_prl_spacing_valid = _end_prl_spacing_valid;
}

bool dbTechLayerSpacingEolRule::isEndPrlSpacingValid() const
{
  _dbTechLayerSpacingEolRule* obj = (_dbTechLayerSpacingEolRule*) this;

  return obj->_flags._end_prl_spacing_valid;
}

void dbTechLayerSpacingEolRule::setPrlValid(bool _prl_valid)
{
  _dbTechLayerSpacingEolRule* obj = (_dbTechLayerSpacingEolRule*) this;

  obj->_flags._prl_valid = _prl_valid;
}

bool dbTechLayerSpacingEolRule::isPrlValid() const
{
  _dbTechLayerSpacingEolRule* obj = (_dbTechLayerSpacingEolRule*) this;

  return obj->_flags._prl_valid;
}

void dbTechLayerSpacingEolRule::setEndToEndValid(bool _end_to_end_valid)
{
  _dbTechLayerSpacingEolRule* obj = (_dbTechLayerSpacingEolRule*) this;

  obj->_flags._end_to_end_valid = _end_to_end_valid;
}

bool dbTechLayerSpacingEolRule::isEndToEndValid() const
{
  _dbTechLayerSpacingEolRule* obj = (_dbTechLayerSpacingEolRule*) this;

  return obj->_flags._end_to_end_valid;
}

void dbTechLayerSpacingEolRule::setCutSpacesValid(bool _cut_spaces_valid)
{
  _dbTechLayerSpacingEolRule* obj = (_dbTechLayerSpacingEolRule*) this;

  obj->_flags._cut_spaces_valid = _cut_spaces_valid;
}

bool dbTechLayerSpacingEolRule::isCutSpacesValid() const
{
  _dbTechLayerSpacingEolRule* obj = (_dbTechLayerSpacingEolRule*) this;

  return obj->_flags._cut_spaces_valid;
}

void dbTechLayerSpacingEolRule::setExtensionValid(bool _extension_valid)
{
  _dbTechLayerSpacingEolRule* obj = (_dbTechLayerSpacingEolRule*) this;

  obj->_flags._extension_valid = _extension_valid;
}

bool dbTechLayerSpacingEolRule::isExtensionValid() const
{
  _dbTechLayerSpacingEolRule* obj = (_dbTechLayerSpacingEolRule*) this;

  return obj->_flags._extension_valid;
}

void dbTechLayerSpacingEolRule::setWrongDirExtensionValid(
    bool _wrong_dir_extension_valid)
{
  _dbTechLayerSpacingEolRule* obj = (_dbTechLayerSpacingEolRule*) this;

  obj->_flags._wrong_dir_extension_valid = _wrong_dir_extension_valid;
}

bool dbTechLayerSpacingEolRule::isWrongDirExtensionValid() const
{
  _dbTechLayerSpacingEolRule* obj = (_dbTechLayerSpacingEolRule*) this;

  return obj->_flags._wrong_dir_extension_valid;
}

void dbTechLayerSpacingEolRule::setOtherEndWidthValid(
    bool _other_end_width_valid)
{
  _dbTechLayerSpacingEolRule* obj = (_dbTechLayerSpacingEolRule*) this;

  obj->_flags._other_end_width_valid = _other_end_width_valid;
}

bool dbTechLayerSpacingEolRule::isOtherEndWidthValid() const
{
  _dbTechLayerSpacingEolRule* obj = (_dbTechLayerSpacingEolRule*) this;

  return obj->_flags._other_end_width_valid;
}

void dbTechLayerSpacingEolRule::setMaxLengthValid(bool _max_length_valid)
{
  _dbTechLayerSpacingEolRule* obj = (_dbTechLayerSpacingEolRule*) this;

  obj->_flags._max_length_valid = _max_length_valid;
}

bool dbTechLayerSpacingEolRule::isMaxLengthValid() const
{
  _dbTechLayerSpacingEolRule* obj = (_dbTechLayerSpacingEolRule*) this;

  return obj->_flags._max_length_valid;
}

void dbTechLayerSpacingEolRule::setMinLengthValid(bool _min_length_valid)
{
  _dbTechLayerSpacingEolRule* obj = (_dbTechLayerSpacingEolRule*) this;

  obj->_flags._min_length_valid = _min_length_valid;
}

bool dbTechLayerSpacingEolRule::isMinLengthValid() const
{
  _dbTechLayerSpacingEolRule* obj = (_dbTechLayerSpacingEolRule*) this;

  return obj->_flags._min_length_valid;
}

void dbTechLayerSpacingEolRule::setTwoSidesValid(bool _two_sides_valid)
{
  _dbTechLayerSpacingEolRule* obj = (_dbTechLayerSpacingEolRule*) this;

  obj->_flags._two_sides_valid = _two_sides_valid;
}

bool dbTechLayerSpacingEolRule::isTwoSidesValid() const
{
  _dbTechLayerSpacingEolRule* obj = (_dbTechLayerSpacingEolRule*) this;

  return obj->_flags._two_sides_valid;
}

void dbTechLayerSpacingEolRule::setEqualRectWidthValid(
    bool _equal_rect_width_valid)
{
  _dbTechLayerSpacingEolRule* obj = (_dbTechLayerSpacingEolRule*) this;

  obj->_flags._equal_rect_width_valid = _equal_rect_width_valid;
}

bool dbTechLayerSpacingEolRule::isEqualRectWidthValid() const
{
  _dbTechLayerSpacingEolRule* obj = (_dbTechLayerSpacingEolRule*) this;

  return obj->_flags._equal_rect_width_valid;
}

void dbTechLayerSpacingEolRule::setParallelEdgeValid(bool _parallel_edge_valid)
{
  _dbTechLayerSpacingEolRule* obj = (_dbTechLayerSpacingEolRule*) this;

  obj->_flags._parallel_edge_valid = _parallel_edge_valid;
}

bool dbTechLayerSpacingEolRule::isParallelEdgeValid() const
{
  _dbTechLayerSpacingEolRule* obj = (_dbTechLayerSpacingEolRule*) this;

  return obj->_flags._parallel_edge_valid;
}

void dbTechLayerSpacingEolRule::setSubtractEolWidthValid(
    bool _subtract_eol_width_valid)
{
  _dbTechLayerSpacingEolRule* obj = (_dbTechLayerSpacingEolRule*) this;

  obj->_flags._subtract_eol_width_valid = _subtract_eol_width_valid;
}

bool dbTechLayerSpacingEolRule::isSubtractEolWidthValid() const
{
  _dbTechLayerSpacingEolRule* obj = (_dbTechLayerSpacingEolRule*) this;

  return obj->_flags._subtract_eol_width_valid;
}

void dbTechLayerSpacingEolRule::setParPrlValid(bool _par_prl_valid)
{
  _dbTechLayerSpacingEolRule* obj = (_dbTechLayerSpacingEolRule*) this;

  obj->_flags._par_prl_valid = _par_prl_valid;
}

bool dbTechLayerSpacingEolRule::isParPrlValid() const
{
  _dbTechLayerSpacingEolRule* obj = (_dbTechLayerSpacingEolRule*) this;

  return obj->_flags._par_prl_valid;
}

void dbTechLayerSpacingEolRule::setParMinLengthValid(bool _par_min_length_valid)
{
  _dbTechLayerSpacingEolRule* obj = (_dbTechLayerSpacingEolRule*) this;

  obj->_flags._par_min_length_valid = _par_min_length_valid;
}

bool dbTechLayerSpacingEolRule::isParMinLengthValid() const
{
  _dbTechLayerSpacingEolRule* obj = (_dbTechLayerSpacingEolRule*) this;

  return obj->_flags._par_min_length_valid;
}

void dbTechLayerSpacingEolRule::setTwoEdgesValid(bool _two_edges_valid)
{
  _dbTechLayerSpacingEolRule* obj = (_dbTechLayerSpacingEolRule*) this;

  obj->_flags._two_edges_valid = _two_edges_valid;
}

bool dbTechLayerSpacingEolRule::isTwoEdgesValid() const
{
  _dbTechLayerSpacingEolRule* obj = (_dbTechLayerSpacingEolRule*) this;

  return obj->_flags._two_edges_valid;
}

void dbTechLayerSpacingEolRule::setSameMetalValid(bool _same_metal_valid)
{
  _dbTechLayerSpacingEolRule* obj = (_dbTechLayerSpacingEolRule*) this;

  obj->_flags._same_metal_valid = _same_metal_valid;
}

bool dbTechLayerSpacingEolRule::isSameMetalValid() const
{
  _dbTechLayerSpacingEolRule* obj = (_dbTechLayerSpacingEolRule*) this;

  return obj->_flags._same_metal_valid;
}

void dbTechLayerSpacingEolRule::setNonEolCornerOnlyValid(
    bool _non_eol_corner_only_valid)
{
  _dbTechLayerSpacingEolRule* obj = (_dbTechLayerSpacingEolRule*) this;

  obj->_flags._non_eol_corner_only_valid = _non_eol_corner_only_valid;
}

bool dbTechLayerSpacingEolRule::isNonEolCornerOnlyValid() const
{
  _dbTechLayerSpacingEolRule* obj = (_dbTechLayerSpacingEolRule*) this;

  return obj->_flags._non_eol_corner_only_valid;
}

void dbTechLayerSpacingEolRule::setParallelSameMaskValid(
    bool _parallel_same_mask_valid)
{
  _dbTechLayerSpacingEolRule* obj = (_dbTechLayerSpacingEolRule*) this;

  obj->_flags._parallel_same_mask_valid = _parallel_same_mask_valid;
}

bool dbTechLayerSpacingEolRule::isParallelSameMaskValid() const
{
  _dbTechLayerSpacingEolRule* obj = (_dbTechLayerSpacingEolRule*) this;

  return obj->_flags._parallel_same_mask_valid;
}

void dbTechLayerSpacingEolRule::setEncloseCutValid(bool _enclose_cut_valid)
{
  _dbTechLayerSpacingEolRule* obj = (_dbTechLayerSpacingEolRule*) this;

  obj->_flags._enclose_cut_valid = _enclose_cut_valid;
}

bool dbTechLayerSpacingEolRule::isEncloseCutValid() const
{
  _dbTechLayerSpacingEolRule* obj = (_dbTechLayerSpacingEolRule*) this;

  return obj->_flags._enclose_cut_valid;
}

void dbTechLayerSpacingEolRule::setBelowValid(bool _below_valid)
{
  _dbTechLayerSpacingEolRule* obj = (_dbTechLayerSpacingEolRule*) this;

  obj->_flags._below_valid = _below_valid;
}

bool dbTechLayerSpacingEolRule::isBelowValid() const
{
  _dbTechLayerSpacingEolRule* obj = (_dbTechLayerSpacingEolRule*) this;

  return obj->_flags._below_valid;
}

void dbTechLayerSpacingEolRule::setAboveValid(bool _above_valid)
{
  _dbTechLayerSpacingEolRule* obj = (_dbTechLayerSpacingEolRule*) this;

  obj->_flags._above_valid = _above_valid;
}

bool dbTechLayerSpacingEolRule::isAboveValid() const
{
  _dbTechLayerSpacingEolRule* obj = (_dbTechLayerSpacingEolRule*) this;

  return obj->_flags._above_valid;
}

void dbTechLayerSpacingEolRule::setCutSpacingValid(bool _cut_spacing_valid)
{
  _dbTechLayerSpacingEolRule* obj = (_dbTechLayerSpacingEolRule*) this;

  obj->_flags._cut_spacing_valid = _cut_spacing_valid;
}

bool dbTechLayerSpacingEolRule::isCutSpacingValid() const
{
  _dbTechLayerSpacingEolRule* obj = (_dbTechLayerSpacingEolRule*) this;

  return obj->_flags._cut_spacing_valid;
}

void dbTechLayerSpacingEolRule::setAllCutsValid(bool _all_cuts_valid)
{
  _dbTechLayerSpacingEolRule* obj = (_dbTechLayerSpacingEolRule*) this;

  obj->_flags._all_cuts_valid = _all_cuts_valid;
}

bool dbTechLayerSpacingEolRule::isAllCutsValid() const
{
  _dbTechLayerSpacingEolRule* obj = (_dbTechLayerSpacingEolRule*) this;

  return obj->_flags._all_cuts_valid;
}

void dbTechLayerSpacingEolRule::setToConcaveCornerValid(
    bool _to_concave_corner_valid)
{
  _dbTechLayerSpacingEolRule* obj = (_dbTechLayerSpacingEolRule*) this;

  obj->_flags._to_concave_corner_valid = _to_concave_corner_valid;
}

bool dbTechLayerSpacingEolRule::isToConcaveCornerValid() const
{
  _dbTechLayerSpacingEolRule* obj = (_dbTechLayerSpacingEolRule*) this;

  return obj->_flags._to_concave_corner_valid;
}

void dbTechLayerSpacingEolRule::setMinAdjacentLengthValid(
    bool _min_adjacent_length_valid)
{
  _dbTechLayerSpacingEolRule* obj = (_dbTechLayerSpacingEolRule*) this;

  obj->_flags._min_adjacent_length_valid = _min_adjacent_length_valid;
}

bool dbTechLayerSpacingEolRule::isMinAdjacentLengthValid() const
{
  _dbTechLayerSpacingEolRule* obj = (_dbTechLayerSpacingEolRule*) this;

  return obj->_flags._min_adjacent_length_valid;
}

void dbTechLayerSpacingEolRule::setTwoMinAdjLengthValid(
    bool _two_min_adj_length_valid)
{
  _dbTechLayerSpacingEolRule* obj = (_dbTechLayerSpacingEolRule*) this;

  obj->_flags._two_min_adj_length_valid = _two_min_adj_length_valid;
}

bool dbTechLayerSpacingEolRule::isTwoMinAdjLengthValid() const
{
  _dbTechLayerSpacingEolRule* obj = (_dbTechLayerSpacingEolRule*) this;

  return obj->_flags._two_min_adj_length_valid;
}

void dbTechLayerSpacingEolRule::setToNotchLengthValid(
    bool _to_notch_length_valid)
{
  _dbTechLayerSpacingEolRule* obj = (_dbTechLayerSpacingEolRule*) this;

  obj->_flags._to_notch_length_valid = _to_notch_length_valid;
}

bool dbTechLayerSpacingEolRule::isToNotchLengthValid() const
{
  _dbTechLayerSpacingEolRule* obj = (_dbTechLayerSpacingEolRule*) this;

  return obj->_flags._to_notch_length_valid;
}

// User Code Begin dbTechLayerSpacingEolRulePublicMethods
dbTechLayerSpacingEolRule* dbTechLayerSpacingEolRule::create(
    dbTechLayer* _layer)
{
  _dbTechLayer*               layer   = (_dbTechLayer*) _layer;
  _dbTechLayerSpacingEolRule* newrule = layer->_spacing_eol_rules_tbl->create();
  newrule->_layer                     = _layer->getImpl()->getOID();

  return ((dbTechLayerSpacingEolRule*) newrule);
}

dbTechLayerSpacingEolRule*
dbTechLayerSpacingEolRule::getTechLayerSpacingEolRule(dbTechLayer* inly,
                                                      uint         dbid)
{
  _dbTechLayer* layer = (_dbTechLayer*) inly;
  return (dbTechLayerSpacingEolRule*) layer->_spacing_eol_rules_tbl->getPtr(
      dbid);
}

void dbTechLayerSpacingEolRule::destroy(dbTechLayerSpacingEolRule* rule)
{
  _dbTechLayer* layer = (_dbTechLayer*) rule->getImpl()->getOwner();
  dbProperty::destroyProperties(rule);
  layer->_spacing_eol_rules_tbl->destroy((_dbTechLayerSpacingEolRule*) rule);
}

// User Code End dbTechLayerSpacingEolRulePublicMethods
}  // namespace odb
   // Generator Code End 1