///////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// Copyright (c) 2020, The Regents of the University of California
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

// Generator Code Begin Cpp
#include "dbTechLayerSpacingEolRule.h"

#include "db.h"
#include "dbDatabase.h"
#include "dbDiff.hpp"
#include "dbTable.h"
#include "dbTable.hpp"

// User Code Begin Includes
#include "dbTech.h"
#include "dbTechLayer.h"
// User Code End Includes
namespace odb {

template class dbTable<_dbTechLayerSpacingEolRule>;

bool _dbTechLayerSpacingEolRule::operator==(
    const _dbTechLayerSpacingEolRule& rhs) const
{
  if (flags_.exact_width_valid_ != rhs.flags_.exact_width_valid_)
    return false;

  if (flags_.wrong_dir_spacing_valid_ != rhs.flags_.wrong_dir_spacing_valid_)
    return false;

  if (flags_.opposite_width_valid_ != rhs.flags_.opposite_width_valid_)
    return false;

  if (flags_.within_valid_ != rhs.flags_.within_valid_)
    return false;

  if (flags_.wrong_dir_within_valid_ != rhs.flags_.wrong_dir_within_valid_)
    return false;

  if (flags_.same_mask_valid_ != rhs.flags_.same_mask_valid_)
    return false;

  if (flags_.except_exact_width_valid_ != rhs.flags_.except_exact_width_valid_)
    return false;

  if (flags_.fill_concave_corner_valid_
      != rhs.flags_.fill_concave_corner_valid_)
    return false;

  if (flags_.withcut_valid_ != rhs.flags_.withcut_valid_)
    return false;

  if (flags_.cut_class_valid_ != rhs.flags_.cut_class_valid_)
    return false;

  if (flags_.with_cut_above_valid_ != rhs.flags_.with_cut_above_valid_)
    return false;

  if (flags_.enclosure_end_valid_ != rhs.flags_.enclosure_end_valid_)
    return false;

  if (flags_.enclosure_end_within_valid_
      != rhs.flags_.enclosure_end_within_valid_)
    return false;

  if (flags_.end_prl_spacing_valid_ != rhs.flags_.end_prl_spacing_valid_)
    return false;

  if (flags_.prl_valid_ != rhs.flags_.prl_valid_)
    return false;

  if (flags_.end_to_end_valid_ != rhs.flags_.end_to_end_valid_)
    return false;

  if (flags_.cut_spaces_valid_ != rhs.flags_.cut_spaces_valid_)
    return false;

  if (flags_.extension_valid_ != rhs.flags_.extension_valid_)
    return false;

  if (flags_.wrong_dir_extension_valid_
      != rhs.flags_.wrong_dir_extension_valid_)
    return false;

  if (flags_.other_end_width_valid_ != rhs.flags_.other_end_width_valid_)
    return false;

  if (flags_.max_length_valid_ != rhs.flags_.max_length_valid_)
    return false;

  if (flags_.min_length_valid_ != rhs.flags_.min_length_valid_)
    return false;

  if (flags_.two_sides_valid_ != rhs.flags_.two_sides_valid_)
    return false;

  if (flags_.equal_rect_width_valid_ != rhs.flags_.equal_rect_width_valid_)
    return false;

  if (flags_.parallel_edge_valid_ != rhs.flags_.parallel_edge_valid_)
    return false;

  if (flags_.subtract_eol_width_valid_ != rhs.flags_.subtract_eol_width_valid_)
    return false;

  if (flags_.par_prl_valid_ != rhs.flags_.par_prl_valid_)
    return false;

  if (flags_.par_min_length_valid_ != rhs.flags_.par_min_length_valid_)
    return false;

  if (flags_.two_edges_valid_ != rhs.flags_.two_edges_valid_)
    return false;

  if (flags_.same_metal_valid_ != rhs.flags_.same_metal_valid_)
    return false;

  if (flags_.non_eol_corner_only_valid_
      != rhs.flags_.non_eol_corner_only_valid_)
    return false;

  if (flags_.parallel_same_mask_valid_ != rhs.flags_.parallel_same_mask_valid_)
    return false;

  if (flags_.enclose_cut_valid_ != rhs.flags_.enclose_cut_valid_)
    return false;

  if (flags_.below_valid_ != rhs.flags_.below_valid_)
    return false;

  if (flags_.above_valid_ != rhs.flags_.above_valid_)
    return false;

  if (flags_.cut_spacing_valid_ != rhs.flags_.cut_spacing_valid_)
    return false;

  if (flags_.all_cuts_valid_ != rhs.flags_.all_cuts_valid_)
    return false;

  if (flags_.to_concave_corner_valid_ != rhs.flags_.to_concave_corner_valid_)
    return false;

  if (flags_.min_adjacent_length_valid_
      != rhs.flags_.min_adjacent_length_valid_)
    return false;

  if (flags_.two_min_adj_length_valid_ != rhs.flags_.two_min_adj_length_valid_)
    return false;

  if (flags_.to_notch_length_valid_ != rhs.flags_.to_notch_length_valid_)
    return false;

  if (eol_space_ != rhs.eol_space_)
    return false;

  if (eol_width_ != rhs.eol_width_)
    return false;

  if (wrong_dir_space_ != rhs.wrong_dir_space_)
    return false;

  if (opposite_width_ != rhs.opposite_width_)
    return false;

  if (eol_within_ != rhs.eol_within_)
    return false;

  if (wrong_dir_within_ != rhs.wrong_dir_within_)
    return false;

  if (exact_width_ != rhs.exact_width_)
    return false;

  if (other_width_ != rhs.other_width_)
    return false;

  if (fill_triangle_ != rhs.fill_triangle_)
    return false;

  if (cut_class_ != rhs.cut_class_)
    return false;

  if (with_cut_space_ != rhs.with_cut_space_)
    return false;

  if (enclosure_end_width_ != rhs.enclosure_end_width_)
    return false;

  if (enclosure_end_within_ != rhs.enclosure_end_within_)
    return false;

  if (end_prl_space_ != rhs.end_prl_space_)
    return false;

  if (end_prl_ != rhs.end_prl_)
    return false;

  if (end_to_end_space_ != rhs.end_to_end_space_)
    return false;

  if (one_cut_space_ != rhs.one_cut_space_)
    return false;

  if (two_cut_space_ != rhs.two_cut_space_)
    return false;

  if (extension_ != rhs.extension_)
    return false;

  if (wrong_dir_extension_ != rhs.wrong_dir_extension_)
    return false;

  if (other_end_width_ != rhs.other_end_width_)
    return false;

  if (max_length_ != rhs.max_length_)
    return false;

  if (min_length_ != rhs.min_length_)
    return false;

  if (par_space_ != rhs.par_space_)
    return false;

  if (par_within_ != rhs.par_within_)
    return false;

  if (par_prl_ != rhs.par_prl_)
    return false;

  if (par_min_length_ != rhs.par_min_length_)
    return false;

  if (enclose_dist_ != rhs.enclose_dist_)
    return false;

  if (cut_to_metal_space_ != rhs.cut_to_metal_space_)
    return false;

  if (min_adj_length_ != rhs.min_adj_length_)
    return false;

  if (min_adj_length1_ != rhs.min_adj_length1_)
    return false;

  if (min_adj_length2_ != rhs.min_adj_length2_)
    return false;

  if (notch_length_ != rhs.notch_length_)
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
    dbDiff& diff,
    const char* field,
    const _dbTechLayerSpacingEolRule& rhs) const
{
  DIFF_BEGIN

  DIFF_FIELD(flags_.exact_width_valid_);
  DIFF_FIELD(flags_.wrong_dir_spacing_valid_);
  DIFF_FIELD(flags_.opposite_width_valid_);
  DIFF_FIELD(flags_.within_valid_);
  DIFF_FIELD(flags_.wrong_dir_within_valid_);
  DIFF_FIELD(flags_.same_mask_valid_);
  DIFF_FIELD(flags_.except_exact_width_valid_);
  DIFF_FIELD(flags_.fill_concave_corner_valid_);
  DIFF_FIELD(flags_.withcut_valid_);
  DIFF_FIELD(flags_.cut_class_valid_);
  DIFF_FIELD(flags_.with_cut_above_valid_);
  DIFF_FIELD(flags_.enclosure_end_valid_);
  DIFF_FIELD(flags_.enclosure_end_within_valid_);
  DIFF_FIELD(flags_.end_prl_spacing_valid_);
  DIFF_FIELD(flags_.prl_valid_);
  DIFF_FIELD(flags_.end_to_end_valid_);
  DIFF_FIELD(flags_.cut_spaces_valid_);
  DIFF_FIELD(flags_.extension_valid_);
  DIFF_FIELD(flags_.wrong_dir_extension_valid_);
  DIFF_FIELD(flags_.other_end_width_valid_);
  DIFF_FIELD(flags_.max_length_valid_);
  DIFF_FIELD(flags_.min_length_valid_);
  DIFF_FIELD(flags_.two_sides_valid_);
  DIFF_FIELD(flags_.equal_rect_width_valid_);
  DIFF_FIELD(flags_.parallel_edge_valid_);
  DIFF_FIELD(flags_.subtract_eol_width_valid_);
  DIFF_FIELD(flags_.par_prl_valid_);
  DIFF_FIELD(flags_.par_min_length_valid_);
  DIFF_FIELD(flags_.two_edges_valid_);
  DIFF_FIELD(flags_.same_metal_valid_);
  DIFF_FIELD(flags_.non_eol_corner_only_valid_);
  DIFF_FIELD(flags_.parallel_same_mask_valid_);
  DIFF_FIELD(flags_.enclose_cut_valid_);
  DIFF_FIELD(flags_.below_valid_);
  DIFF_FIELD(flags_.above_valid_);
  DIFF_FIELD(flags_.cut_spacing_valid_);
  DIFF_FIELD(flags_.all_cuts_valid_);
  DIFF_FIELD(flags_.to_concave_corner_valid_);
  DIFF_FIELD(flags_.min_adjacent_length_valid_);
  DIFF_FIELD(flags_.two_min_adj_length_valid_);
  DIFF_FIELD(flags_.to_notch_length_valid_);
  DIFF_FIELD(eol_space_);
  DIFF_FIELD(eol_width_);
  DIFF_FIELD(wrong_dir_space_);
  DIFF_FIELD(opposite_width_);
  DIFF_FIELD(eol_within_);
  DIFF_FIELD(wrong_dir_within_);
  DIFF_FIELD(exact_width_);
  DIFF_FIELD(other_width_);
  DIFF_FIELD(fill_triangle_);
  DIFF_FIELD(cut_class_);
  DIFF_FIELD(with_cut_space_);
  DIFF_FIELD(enclosure_end_width_);
  DIFF_FIELD(enclosure_end_within_);
  DIFF_FIELD(end_prl_space_);
  DIFF_FIELD(end_prl_);
  DIFF_FIELD(end_to_end_space_);
  DIFF_FIELD(one_cut_space_);
  DIFF_FIELD(two_cut_space_);
  DIFF_FIELD(extension_);
  DIFF_FIELD(wrong_dir_extension_);
  DIFF_FIELD(other_end_width_);
  DIFF_FIELD(max_length_);
  DIFF_FIELD(min_length_);
  DIFF_FIELD(par_space_);
  DIFF_FIELD(par_within_);
  DIFF_FIELD(par_prl_);
  DIFF_FIELD(par_min_length_);
  DIFF_FIELD(enclose_dist_);
  DIFF_FIELD(cut_to_metal_space_);
  DIFF_FIELD(min_adj_length_);
  DIFF_FIELD(min_adj_length1_);
  DIFF_FIELD(min_adj_length2_);
  DIFF_FIELD(notch_length_);
  // User Code Begin Differences
  // User Code End Differences
  DIFF_END
}
void _dbTechLayerSpacingEolRule::out(dbDiff& diff,
                                     char side,
                                     const char* field) const
{
  DIFF_OUT_BEGIN
  DIFF_OUT_FIELD(flags_.exact_width_valid_);
  DIFF_OUT_FIELD(flags_.wrong_dir_spacing_valid_);
  DIFF_OUT_FIELD(flags_.opposite_width_valid_);
  DIFF_OUT_FIELD(flags_.within_valid_);
  DIFF_OUT_FIELD(flags_.wrong_dir_within_valid_);
  DIFF_OUT_FIELD(flags_.same_mask_valid_);
  DIFF_OUT_FIELD(flags_.except_exact_width_valid_);
  DIFF_OUT_FIELD(flags_.fill_concave_corner_valid_);
  DIFF_OUT_FIELD(flags_.withcut_valid_);
  DIFF_OUT_FIELD(flags_.cut_class_valid_);
  DIFF_OUT_FIELD(flags_.with_cut_above_valid_);
  DIFF_OUT_FIELD(flags_.enclosure_end_valid_);
  DIFF_OUT_FIELD(flags_.enclosure_end_within_valid_);
  DIFF_OUT_FIELD(flags_.end_prl_spacing_valid_);
  DIFF_OUT_FIELD(flags_.prl_valid_);
  DIFF_OUT_FIELD(flags_.end_to_end_valid_);
  DIFF_OUT_FIELD(flags_.cut_spaces_valid_);
  DIFF_OUT_FIELD(flags_.extension_valid_);
  DIFF_OUT_FIELD(flags_.wrong_dir_extension_valid_);
  DIFF_OUT_FIELD(flags_.other_end_width_valid_);
  DIFF_OUT_FIELD(flags_.max_length_valid_);
  DIFF_OUT_FIELD(flags_.min_length_valid_);
  DIFF_OUT_FIELD(flags_.two_sides_valid_);
  DIFF_OUT_FIELD(flags_.equal_rect_width_valid_);
  DIFF_OUT_FIELD(flags_.parallel_edge_valid_);
  DIFF_OUT_FIELD(flags_.subtract_eol_width_valid_);
  DIFF_OUT_FIELD(flags_.par_prl_valid_);
  DIFF_OUT_FIELD(flags_.par_min_length_valid_);
  DIFF_OUT_FIELD(flags_.two_edges_valid_);
  DIFF_OUT_FIELD(flags_.same_metal_valid_);
  DIFF_OUT_FIELD(flags_.non_eol_corner_only_valid_);
  DIFF_OUT_FIELD(flags_.parallel_same_mask_valid_);
  DIFF_OUT_FIELD(flags_.enclose_cut_valid_);
  DIFF_OUT_FIELD(flags_.below_valid_);
  DIFF_OUT_FIELD(flags_.above_valid_);
  DIFF_OUT_FIELD(flags_.cut_spacing_valid_);
  DIFF_OUT_FIELD(flags_.all_cuts_valid_);
  DIFF_OUT_FIELD(flags_.to_concave_corner_valid_);
  DIFF_OUT_FIELD(flags_.min_adjacent_length_valid_);
  DIFF_OUT_FIELD(flags_.two_min_adj_length_valid_);
  DIFF_OUT_FIELD(flags_.to_notch_length_valid_);
  DIFF_OUT_FIELD(eol_space_);
  DIFF_OUT_FIELD(eol_width_);
  DIFF_OUT_FIELD(wrong_dir_space_);
  DIFF_OUT_FIELD(opposite_width_);
  DIFF_OUT_FIELD(eol_within_);
  DIFF_OUT_FIELD(wrong_dir_within_);
  DIFF_OUT_FIELD(exact_width_);
  DIFF_OUT_FIELD(other_width_);
  DIFF_OUT_FIELD(fill_triangle_);
  DIFF_OUT_FIELD(cut_class_);
  DIFF_OUT_FIELD(with_cut_space_);
  DIFF_OUT_FIELD(enclosure_end_width_);
  DIFF_OUT_FIELD(enclosure_end_within_);
  DIFF_OUT_FIELD(end_prl_space_);
  DIFF_OUT_FIELD(end_prl_);
  DIFF_OUT_FIELD(end_to_end_space_);
  DIFF_OUT_FIELD(one_cut_space_);
  DIFF_OUT_FIELD(two_cut_space_);
  DIFF_OUT_FIELD(extension_);
  DIFF_OUT_FIELD(wrong_dir_extension_);
  DIFF_OUT_FIELD(other_end_width_);
  DIFF_OUT_FIELD(max_length_);
  DIFF_OUT_FIELD(min_length_);
  DIFF_OUT_FIELD(par_space_);
  DIFF_OUT_FIELD(par_within_);
  DIFF_OUT_FIELD(par_prl_);
  DIFF_OUT_FIELD(par_min_length_);
  DIFF_OUT_FIELD(enclose_dist_);
  DIFF_OUT_FIELD(cut_to_metal_space_);
  DIFF_OUT_FIELD(min_adj_length_);
  DIFF_OUT_FIELD(min_adj_length1_);
  DIFF_OUT_FIELD(min_adj_length2_);
  DIFF_OUT_FIELD(notch_length_);

  // User Code Begin Out
  // User Code End Out
  DIFF_END
}
_dbTechLayerSpacingEolRule::_dbTechLayerSpacingEolRule(_dbDatabase* db)
{
  uint64_t* flags__bit_field = (uint64_t*) &flags_;
  *flags__bit_field = 0;
  eol_space_ = 0;
  eol_width_ = 0;
  wrong_dir_space_ = 0;
  opposite_width_ = 0;
  eol_within_ = 0;
  wrong_dir_within_ = 0;
  exact_width_ = 0;
  other_width_ = 0;
  fill_triangle_ = 0;
  cut_class_ = 0;
  with_cut_space_ = 0;
  enclosure_end_width_ = 0;
  enclosure_end_within_ = 0;
  end_prl_space_ = 0;
  end_prl_ = 0;
  end_to_end_space_ = 0;
  one_cut_space_ = 0;
  two_cut_space_ = 0;
  extension_ = 0;
  wrong_dir_extension_ = 0;
  other_end_width_ = 0;
  max_length_ = 0;
  min_length_ = 0;
  par_space_ = 0;
  par_within_ = 0;
  par_prl_ = 0;
  par_min_length_ = 0;
  enclose_dist_ = 0;
  cut_to_metal_space_ = 0;
  min_adj_length_ = 0;
  min_adj_length1_ = 0;
  min_adj_length2_ = 0;
  notch_length_ = 0;
  // User Code Begin Constructor
  eol_space_ = 0;
  eol_width_ = 0;
  wrong_dir_space_ = 0;
  opposite_width_ = 0;
  eol_within_ = 0;
  wrong_dir_within_ = 0;
  exact_width_ = 0;
  other_width_ = 0;
  fill_triangle_ = 0;
  cut_class_ = 0;
  with_cut_space_ = 0;
  enclosure_end_width_ = 0;
  enclosure_end_within_ = 0;
  end_prl_space_ = 0;
  end_prl_ = 0;
  end_to_end_space_ = 0;
  one_cut_space_ = 0;
  two_cut_space_ = 0;
  extension_ = 0;
  wrong_dir_extension_ = 0;
  other_end_width_ = 0;
  max_length_ = 0;
  min_length_ = 0;
  par_space_ = 0;
  par_within_ = 0;
  par_prl_ = 0;
  par_min_length_ = 0;
  enclose_dist_ = 0;
  cut_to_metal_space_ = 0;
  min_adj_length_ = 0;
  min_adj_length1_ = 0;
  min_adj_length2_ = 0;
  notch_length_ = 0;
  // User Code End Constructor
}
_dbTechLayerSpacingEolRule::_dbTechLayerSpacingEolRule(
    _dbDatabase* db,
    const _dbTechLayerSpacingEolRule& r)
{
  flags_.exact_width_valid_ = r.flags_.exact_width_valid_;
  flags_.wrong_dir_spacing_valid_ = r.flags_.wrong_dir_spacing_valid_;
  flags_.opposite_width_valid_ = r.flags_.opposite_width_valid_;
  flags_.within_valid_ = r.flags_.within_valid_;
  flags_.wrong_dir_within_valid_ = r.flags_.wrong_dir_within_valid_;
  flags_.same_mask_valid_ = r.flags_.same_mask_valid_;
  flags_.except_exact_width_valid_ = r.flags_.except_exact_width_valid_;
  flags_.fill_concave_corner_valid_ = r.flags_.fill_concave_corner_valid_;
  flags_.withcut_valid_ = r.flags_.withcut_valid_;
  flags_.cut_class_valid_ = r.flags_.cut_class_valid_;
  flags_.with_cut_above_valid_ = r.flags_.with_cut_above_valid_;
  flags_.enclosure_end_valid_ = r.flags_.enclosure_end_valid_;
  flags_.enclosure_end_within_valid_ = r.flags_.enclosure_end_within_valid_;
  flags_.end_prl_spacing_valid_ = r.flags_.end_prl_spacing_valid_;
  flags_.prl_valid_ = r.flags_.prl_valid_;
  flags_.end_to_end_valid_ = r.flags_.end_to_end_valid_;
  flags_.cut_spaces_valid_ = r.flags_.cut_spaces_valid_;
  flags_.extension_valid_ = r.flags_.extension_valid_;
  flags_.wrong_dir_extension_valid_ = r.flags_.wrong_dir_extension_valid_;
  flags_.other_end_width_valid_ = r.flags_.other_end_width_valid_;
  flags_.max_length_valid_ = r.flags_.max_length_valid_;
  flags_.min_length_valid_ = r.flags_.min_length_valid_;
  flags_.two_sides_valid_ = r.flags_.two_sides_valid_;
  flags_.equal_rect_width_valid_ = r.flags_.equal_rect_width_valid_;
  flags_.parallel_edge_valid_ = r.flags_.parallel_edge_valid_;
  flags_.subtract_eol_width_valid_ = r.flags_.subtract_eol_width_valid_;
  flags_.par_prl_valid_ = r.flags_.par_prl_valid_;
  flags_.par_min_length_valid_ = r.flags_.par_min_length_valid_;
  flags_.two_edges_valid_ = r.flags_.two_edges_valid_;
  flags_.same_metal_valid_ = r.flags_.same_metal_valid_;
  flags_.non_eol_corner_only_valid_ = r.flags_.non_eol_corner_only_valid_;
  flags_.parallel_same_mask_valid_ = r.flags_.parallel_same_mask_valid_;
  flags_.enclose_cut_valid_ = r.flags_.enclose_cut_valid_;
  flags_.below_valid_ = r.flags_.below_valid_;
  flags_.above_valid_ = r.flags_.above_valid_;
  flags_.cut_spacing_valid_ = r.flags_.cut_spacing_valid_;
  flags_.all_cuts_valid_ = r.flags_.all_cuts_valid_;
  flags_.to_concave_corner_valid_ = r.flags_.to_concave_corner_valid_;
  flags_.min_adjacent_length_valid_ = r.flags_.min_adjacent_length_valid_;
  flags_.two_min_adj_length_valid_ = r.flags_.two_min_adj_length_valid_;
  flags_.to_notch_length_valid_ = r.flags_.to_notch_length_valid_;
  flags_.spare_bits_ = r.flags_.spare_bits_;
  eol_space_ = r.eol_space_;
  eol_width_ = r.eol_width_;
  wrong_dir_space_ = r.wrong_dir_space_;
  opposite_width_ = r.opposite_width_;
  eol_within_ = r.eol_within_;
  wrong_dir_within_ = r.wrong_dir_within_;
  exact_width_ = r.exact_width_;
  other_width_ = r.other_width_;
  fill_triangle_ = r.fill_triangle_;
  cut_class_ = r.cut_class_;
  with_cut_space_ = r.with_cut_space_;
  enclosure_end_width_ = r.enclosure_end_width_;
  enclosure_end_within_ = r.enclosure_end_within_;
  end_prl_space_ = r.end_prl_space_;
  end_prl_ = r.end_prl_;
  end_to_end_space_ = r.end_to_end_space_;
  one_cut_space_ = r.one_cut_space_;
  two_cut_space_ = r.two_cut_space_;
  extension_ = r.extension_;
  wrong_dir_extension_ = r.wrong_dir_extension_;
  other_end_width_ = r.other_end_width_;
  max_length_ = r.max_length_;
  min_length_ = r.min_length_;
  par_space_ = r.par_space_;
  par_within_ = r.par_within_;
  par_prl_ = r.par_prl_;
  par_min_length_ = r.par_min_length_;
  enclose_dist_ = r.enclose_dist_;
  cut_to_metal_space_ = r.cut_to_metal_space_;
  min_adj_length_ = r.min_adj_length_;
  min_adj_length1_ = r.min_adj_length1_;
  min_adj_length2_ = r.min_adj_length2_;
  notch_length_ = r.notch_length_;
  // User Code Begin CopyConstructor
  // User Code End CopyConstructor
}

dbIStream& operator>>(dbIStream& stream, _dbTechLayerSpacingEolRule& obj)
{
  uint64_t* flags__bit_field = (uint64_t*) &obj.flags_;
  stream >> *flags__bit_field;
  stream >> obj.eol_space_;
  stream >> obj.eol_width_;
  stream >> obj.wrong_dir_space_;
  stream >> obj.opposite_width_;
  stream >> obj.eol_within_;
  stream >> obj.wrong_dir_within_;
  stream >> obj.exact_width_;
  stream >> obj.other_width_;
  stream >> obj.fill_triangle_;
  stream >> obj.cut_class_;
  stream >> obj.with_cut_space_;
  stream >> obj.enclosure_end_width_;
  stream >> obj.enclosure_end_within_;
  stream >> obj.end_prl_space_;
  stream >> obj.end_prl_;
  stream >> obj.end_to_end_space_;
  stream >> obj.one_cut_space_;
  stream >> obj.two_cut_space_;
  stream >> obj.extension_;
  stream >> obj.wrong_dir_extension_;
  stream >> obj.other_end_width_;
  stream >> obj.max_length_;
  stream >> obj.min_length_;
  stream >> obj.par_space_;
  stream >> obj.par_within_;
  stream >> obj.par_prl_;
  stream >> obj.par_min_length_;
  stream >> obj.enclose_dist_;
  stream >> obj.cut_to_metal_space_;
  stream >> obj.min_adj_length_;
  stream >> obj.min_adj_length1_;
  stream >> obj.min_adj_length2_;
  stream >> obj.notch_length_;
  // User Code Begin >>
  // User Code End >>
  return stream;
}
dbOStream& operator<<(dbOStream& stream, const _dbTechLayerSpacingEolRule& obj)
{
  uint64_t* flags__bit_field = (uint64_t*) &obj.flags_;
  stream << *flags__bit_field;
  stream << obj.eol_space_;
  stream << obj.eol_width_;
  stream << obj.wrong_dir_space_;
  stream << obj.opposite_width_;
  stream << obj.eol_within_;
  stream << obj.wrong_dir_within_;
  stream << obj.exact_width_;
  stream << obj.other_width_;
  stream << obj.fill_triangle_;
  stream << obj.cut_class_;
  stream << obj.with_cut_space_;
  stream << obj.enclosure_end_width_;
  stream << obj.enclosure_end_within_;
  stream << obj.end_prl_space_;
  stream << obj.end_prl_;
  stream << obj.end_to_end_space_;
  stream << obj.one_cut_space_;
  stream << obj.two_cut_space_;
  stream << obj.extension_;
  stream << obj.wrong_dir_extension_;
  stream << obj.other_end_width_;
  stream << obj.max_length_;
  stream << obj.min_length_;
  stream << obj.par_space_;
  stream << obj.par_within_;
  stream << obj.par_prl_;
  stream << obj.par_min_length_;
  stream << obj.enclose_dist_;
  stream << obj.cut_to_metal_space_;
  stream << obj.min_adj_length_;
  stream << obj.min_adj_length1_;
  stream << obj.min_adj_length2_;
  stream << obj.notch_length_;
  // User Code Begin <<
  // User Code End <<
  return stream;
}

_dbTechLayerSpacingEolRule::~_dbTechLayerSpacingEolRule()
{
  // User Code Begin Destructor
  // User Code End Destructor
}

// User Code Begin PrivateMethods
// User Code End PrivateMethods

////////////////////////////////////////////////////////////////////
//
// dbTechLayerSpacingEolRule - Methods
//
////////////////////////////////////////////////////////////////////

void dbTechLayerSpacingEolRule::setEolSpace(int eol_space)
{
  _dbTechLayerSpacingEolRule* obj = (_dbTechLayerSpacingEolRule*) this;

  obj->eol_space_ = eol_space;
}

int dbTechLayerSpacingEolRule::getEolSpace() const
{
  _dbTechLayerSpacingEolRule* obj = (_dbTechLayerSpacingEolRule*) this;
  return obj->eol_space_;
}

void dbTechLayerSpacingEolRule::setEolWidth(int eol_width)
{
  _dbTechLayerSpacingEolRule* obj = (_dbTechLayerSpacingEolRule*) this;

  obj->eol_width_ = eol_width;
}

int dbTechLayerSpacingEolRule::getEolWidth() const
{
  _dbTechLayerSpacingEolRule* obj = (_dbTechLayerSpacingEolRule*) this;
  return obj->eol_width_;
}

void dbTechLayerSpacingEolRule::setWrongDirSpace(int wrong_dir_space)
{
  _dbTechLayerSpacingEolRule* obj = (_dbTechLayerSpacingEolRule*) this;

  obj->wrong_dir_space_ = wrong_dir_space;
}

int dbTechLayerSpacingEolRule::getWrongDirSpace() const
{
  _dbTechLayerSpacingEolRule* obj = (_dbTechLayerSpacingEolRule*) this;
  return obj->wrong_dir_space_;
}

void dbTechLayerSpacingEolRule::setOppositeWidth(int opposite_width)
{
  _dbTechLayerSpacingEolRule* obj = (_dbTechLayerSpacingEolRule*) this;

  obj->opposite_width_ = opposite_width;
}

int dbTechLayerSpacingEolRule::getOppositeWidth() const
{
  _dbTechLayerSpacingEolRule* obj = (_dbTechLayerSpacingEolRule*) this;
  return obj->opposite_width_;
}

void dbTechLayerSpacingEolRule::setEolWithin(int eol_within)
{
  _dbTechLayerSpacingEolRule* obj = (_dbTechLayerSpacingEolRule*) this;

  obj->eol_within_ = eol_within;
}

int dbTechLayerSpacingEolRule::getEolWithin() const
{
  _dbTechLayerSpacingEolRule* obj = (_dbTechLayerSpacingEolRule*) this;
  return obj->eol_within_;
}

void dbTechLayerSpacingEolRule::setWrongDirWithin(int wrong_dir_within)
{
  _dbTechLayerSpacingEolRule* obj = (_dbTechLayerSpacingEolRule*) this;

  obj->wrong_dir_within_ = wrong_dir_within;
}

int dbTechLayerSpacingEolRule::getWrongDirWithin() const
{
  _dbTechLayerSpacingEolRule* obj = (_dbTechLayerSpacingEolRule*) this;
  return obj->wrong_dir_within_;
}

void dbTechLayerSpacingEolRule::setExactWidth(int exact_width)
{
  _dbTechLayerSpacingEolRule* obj = (_dbTechLayerSpacingEolRule*) this;

  obj->exact_width_ = exact_width;
}

int dbTechLayerSpacingEolRule::getExactWidth() const
{
  _dbTechLayerSpacingEolRule* obj = (_dbTechLayerSpacingEolRule*) this;
  return obj->exact_width_;
}

void dbTechLayerSpacingEolRule::setOtherWidth(int other_width)
{
  _dbTechLayerSpacingEolRule* obj = (_dbTechLayerSpacingEolRule*) this;

  obj->other_width_ = other_width;
}

int dbTechLayerSpacingEolRule::getOtherWidth() const
{
  _dbTechLayerSpacingEolRule* obj = (_dbTechLayerSpacingEolRule*) this;
  return obj->other_width_;
}

void dbTechLayerSpacingEolRule::setFillTriangle(int fill_triangle)
{
  _dbTechLayerSpacingEolRule* obj = (_dbTechLayerSpacingEolRule*) this;

  obj->fill_triangle_ = fill_triangle;
}

int dbTechLayerSpacingEolRule::getFillTriangle() const
{
  _dbTechLayerSpacingEolRule* obj = (_dbTechLayerSpacingEolRule*) this;
  return obj->fill_triangle_;
}

void dbTechLayerSpacingEolRule::setCutClass(int cut_class)
{
  _dbTechLayerSpacingEolRule* obj = (_dbTechLayerSpacingEolRule*) this;

  obj->cut_class_ = cut_class;
}

int dbTechLayerSpacingEolRule::getCutClass() const
{
  _dbTechLayerSpacingEolRule* obj = (_dbTechLayerSpacingEolRule*) this;
  return obj->cut_class_;
}

void dbTechLayerSpacingEolRule::setWithCutSpace(int with_cut_space)
{
  _dbTechLayerSpacingEolRule* obj = (_dbTechLayerSpacingEolRule*) this;

  obj->with_cut_space_ = with_cut_space;
}

int dbTechLayerSpacingEolRule::getWithCutSpace() const
{
  _dbTechLayerSpacingEolRule* obj = (_dbTechLayerSpacingEolRule*) this;
  return obj->with_cut_space_;
}

void dbTechLayerSpacingEolRule::setEnclosureEndWidth(int enclosure_end_width)
{
  _dbTechLayerSpacingEolRule* obj = (_dbTechLayerSpacingEolRule*) this;

  obj->enclosure_end_width_ = enclosure_end_width;
}

int dbTechLayerSpacingEolRule::getEnclosureEndWidth() const
{
  _dbTechLayerSpacingEolRule* obj = (_dbTechLayerSpacingEolRule*) this;
  return obj->enclosure_end_width_;
}

void dbTechLayerSpacingEolRule::setEnclosureEndWithin(int enclosure_end_within)
{
  _dbTechLayerSpacingEolRule* obj = (_dbTechLayerSpacingEolRule*) this;

  obj->enclosure_end_within_ = enclosure_end_within;
}

int dbTechLayerSpacingEolRule::getEnclosureEndWithin() const
{
  _dbTechLayerSpacingEolRule* obj = (_dbTechLayerSpacingEolRule*) this;
  return obj->enclosure_end_within_;
}

void dbTechLayerSpacingEolRule::setEndPrlSpace(int end_prl_space)
{
  _dbTechLayerSpacingEolRule* obj = (_dbTechLayerSpacingEolRule*) this;

  obj->end_prl_space_ = end_prl_space;
}

int dbTechLayerSpacingEolRule::getEndPrlSpace() const
{
  _dbTechLayerSpacingEolRule* obj = (_dbTechLayerSpacingEolRule*) this;
  return obj->end_prl_space_;
}

void dbTechLayerSpacingEolRule::setEndPrl(int end_prl)
{
  _dbTechLayerSpacingEolRule* obj = (_dbTechLayerSpacingEolRule*) this;

  obj->end_prl_ = end_prl;
}

int dbTechLayerSpacingEolRule::getEndPrl() const
{
  _dbTechLayerSpacingEolRule* obj = (_dbTechLayerSpacingEolRule*) this;
  return obj->end_prl_;
}

void dbTechLayerSpacingEolRule::setEndToEndSpace(int end_to_end_space)
{
  _dbTechLayerSpacingEolRule* obj = (_dbTechLayerSpacingEolRule*) this;

  obj->end_to_end_space_ = end_to_end_space;
}

int dbTechLayerSpacingEolRule::getEndToEndSpace() const
{
  _dbTechLayerSpacingEolRule* obj = (_dbTechLayerSpacingEolRule*) this;
  return obj->end_to_end_space_;
}

void dbTechLayerSpacingEolRule::setOneCutSpace(int one_cut_space)
{
  _dbTechLayerSpacingEolRule* obj = (_dbTechLayerSpacingEolRule*) this;

  obj->one_cut_space_ = one_cut_space;
}

int dbTechLayerSpacingEolRule::getOneCutSpace() const
{
  _dbTechLayerSpacingEolRule* obj = (_dbTechLayerSpacingEolRule*) this;
  return obj->one_cut_space_;
}

void dbTechLayerSpacingEolRule::setTwoCutSpace(int two_cut_space)
{
  _dbTechLayerSpacingEolRule* obj = (_dbTechLayerSpacingEolRule*) this;

  obj->two_cut_space_ = two_cut_space;
}

int dbTechLayerSpacingEolRule::getTwoCutSpace() const
{
  _dbTechLayerSpacingEolRule* obj = (_dbTechLayerSpacingEolRule*) this;
  return obj->two_cut_space_;
}

void dbTechLayerSpacingEolRule::setExtension(int extension)
{
  _dbTechLayerSpacingEolRule* obj = (_dbTechLayerSpacingEolRule*) this;

  obj->extension_ = extension;
}

int dbTechLayerSpacingEolRule::getExtension() const
{
  _dbTechLayerSpacingEolRule* obj = (_dbTechLayerSpacingEolRule*) this;
  return obj->extension_;
}

void dbTechLayerSpacingEolRule::setWrongDirExtension(int wrong_dir_extension)
{
  _dbTechLayerSpacingEolRule* obj = (_dbTechLayerSpacingEolRule*) this;

  obj->wrong_dir_extension_ = wrong_dir_extension;
}

int dbTechLayerSpacingEolRule::getWrongDirExtension() const
{
  _dbTechLayerSpacingEolRule* obj = (_dbTechLayerSpacingEolRule*) this;
  return obj->wrong_dir_extension_;
}

void dbTechLayerSpacingEolRule::setOtherEndWidth(int other_end_width)
{
  _dbTechLayerSpacingEolRule* obj = (_dbTechLayerSpacingEolRule*) this;

  obj->other_end_width_ = other_end_width;
}

int dbTechLayerSpacingEolRule::getOtherEndWidth() const
{
  _dbTechLayerSpacingEolRule* obj = (_dbTechLayerSpacingEolRule*) this;
  return obj->other_end_width_;
}

void dbTechLayerSpacingEolRule::setMaxLength(int max_length)
{
  _dbTechLayerSpacingEolRule* obj = (_dbTechLayerSpacingEolRule*) this;

  obj->max_length_ = max_length;
}

int dbTechLayerSpacingEolRule::getMaxLength() const
{
  _dbTechLayerSpacingEolRule* obj = (_dbTechLayerSpacingEolRule*) this;
  return obj->max_length_;
}

void dbTechLayerSpacingEolRule::setMinLength(int min_length)
{
  _dbTechLayerSpacingEolRule* obj = (_dbTechLayerSpacingEolRule*) this;

  obj->min_length_ = min_length;
}

int dbTechLayerSpacingEolRule::getMinLength() const
{
  _dbTechLayerSpacingEolRule* obj = (_dbTechLayerSpacingEolRule*) this;
  return obj->min_length_;
}

void dbTechLayerSpacingEolRule::setParSpace(int par_space)
{
  _dbTechLayerSpacingEolRule* obj = (_dbTechLayerSpacingEolRule*) this;

  obj->par_space_ = par_space;
}

int dbTechLayerSpacingEolRule::getParSpace() const
{
  _dbTechLayerSpacingEolRule* obj = (_dbTechLayerSpacingEolRule*) this;
  return obj->par_space_;
}

void dbTechLayerSpacingEolRule::setParWithin(int par_within)
{
  _dbTechLayerSpacingEolRule* obj = (_dbTechLayerSpacingEolRule*) this;

  obj->par_within_ = par_within;
}

int dbTechLayerSpacingEolRule::getParWithin() const
{
  _dbTechLayerSpacingEolRule* obj = (_dbTechLayerSpacingEolRule*) this;
  return obj->par_within_;
}

void dbTechLayerSpacingEolRule::setParPrl(int par_prl)
{
  _dbTechLayerSpacingEolRule* obj = (_dbTechLayerSpacingEolRule*) this;

  obj->par_prl_ = par_prl;
}

int dbTechLayerSpacingEolRule::getParPrl() const
{
  _dbTechLayerSpacingEolRule* obj = (_dbTechLayerSpacingEolRule*) this;
  return obj->par_prl_;
}

void dbTechLayerSpacingEolRule::setParMinLength(int par_min_length)
{
  _dbTechLayerSpacingEolRule* obj = (_dbTechLayerSpacingEolRule*) this;

  obj->par_min_length_ = par_min_length;
}

int dbTechLayerSpacingEolRule::getParMinLength() const
{
  _dbTechLayerSpacingEolRule* obj = (_dbTechLayerSpacingEolRule*) this;
  return obj->par_min_length_;
}

void dbTechLayerSpacingEolRule::setEncloseDist(int enclose_dist)
{
  _dbTechLayerSpacingEolRule* obj = (_dbTechLayerSpacingEolRule*) this;

  obj->enclose_dist_ = enclose_dist;
}

int dbTechLayerSpacingEolRule::getEncloseDist() const
{
  _dbTechLayerSpacingEolRule* obj = (_dbTechLayerSpacingEolRule*) this;
  return obj->enclose_dist_;
}

void dbTechLayerSpacingEolRule::setCutToMetalSpace(int cut_to_metal_space)
{
  _dbTechLayerSpacingEolRule* obj = (_dbTechLayerSpacingEolRule*) this;

  obj->cut_to_metal_space_ = cut_to_metal_space;
}

int dbTechLayerSpacingEolRule::getCutToMetalSpace() const
{
  _dbTechLayerSpacingEolRule* obj = (_dbTechLayerSpacingEolRule*) this;
  return obj->cut_to_metal_space_;
}

void dbTechLayerSpacingEolRule::setMinAdjLength(int min_adj_length)
{
  _dbTechLayerSpacingEolRule* obj = (_dbTechLayerSpacingEolRule*) this;

  obj->min_adj_length_ = min_adj_length;
}

int dbTechLayerSpacingEolRule::getMinAdjLength() const
{
  _dbTechLayerSpacingEolRule* obj = (_dbTechLayerSpacingEolRule*) this;
  return obj->min_adj_length_;
}

void dbTechLayerSpacingEolRule::setMinAdjLength1(int min_adj_length1)
{
  _dbTechLayerSpacingEolRule* obj = (_dbTechLayerSpacingEolRule*) this;

  obj->min_adj_length1_ = min_adj_length1;
}

int dbTechLayerSpacingEolRule::getMinAdjLength1() const
{
  _dbTechLayerSpacingEolRule* obj = (_dbTechLayerSpacingEolRule*) this;
  return obj->min_adj_length1_;
}

void dbTechLayerSpacingEolRule::setMinAdjLength2(int min_adj_length2)
{
  _dbTechLayerSpacingEolRule* obj = (_dbTechLayerSpacingEolRule*) this;

  obj->min_adj_length2_ = min_adj_length2;
}

int dbTechLayerSpacingEolRule::getMinAdjLength2() const
{
  _dbTechLayerSpacingEolRule* obj = (_dbTechLayerSpacingEolRule*) this;
  return obj->min_adj_length2_;
}

void dbTechLayerSpacingEolRule::setNotchLength(int notch_length)
{
  _dbTechLayerSpacingEolRule* obj = (_dbTechLayerSpacingEolRule*) this;

  obj->notch_length_ = notch_length;
}

int dbTechLayerSpacingEolRule::getNotchLength() const
{
  _dbTechLayerSpacingEolRule* obj = (_dbTechLayerSpacingEolRule*) this;
  return obj->notch_length_;
}

void dbTechLayerSpacingEolRule::setExactWidthValid(bool exact_width_valid)
{
  _dbTechLayerSpacingEolRule* obj = (_dbTechLayerSpacingEolRule*) this;

  obj->flags_.exact_width_valid_ = exact_width_valid;
}

bool dbTechLayerSpacingEolRule::isExactWidthValid() const
{
  _dbTechLayerSpacingEolRule* obj = (_dbTechLayerSpacingEolRule*) this;

  return obj->flags_.exact_width_valid_;
}

void dbTechLayerSpacingEolRule::setWrongDirSpacingValid(
    bool wrong_dir_spacing_valid)
{
  _dbTechLayerSpacingEolRule* obj = (_dbTechLayerSpacingEolRule*) this;

  obj->flags_.wrong_dir_spacing_valid_ = wrong_dir_spacing_valid;
}

bool dbTechLayerSpacingEolRule::isWrongDirSpacingValid() const
{
  _dbTechLayerSpacingEolRule* obj = (_dbTechLayerSpacingEolRule*) this;

  return obj->flags_.wrong_dir_spacing_valid_;
}

void dbTechLayerSpacingEolRule::setOppositeWidthValid(bool opposite_width_valid)
{
  _dbTechLayerSpacingEolRule* obj = (_dbTechLayerSpacingEolRule*) this;

  obj->flags_.opposite_width_valid_ = opposite_width_valid;
}

bool dbTechLayerSpacingEolRule::isOppositeWidthValid() const
{
  _dbTechLayerSpacingEolRule* obj = (_dbTechLayerSpacingEolRule*) this;

  return obj->flags_.opposite_width_valid_;
}

void dbTechLayerSpacingEolRule::setWithinValid(bool within_valid)
{
  _dbTechLayerSpacingEolRule* obj = (_dbTechLayerSpacingEolRule*) this;

  obj->flags_.within_valid_ = within_valid;
}

bool dbTechLayerSpacingEolRule::isWithinValid() const
{
  _dbTechLayerSpacingEolRule* obj = (_dbTechLayerSpacingEolRule*) this;

  return obj->flags_.within_valid_;
}

void dbTechLayerSpacingEolRule::setWrongDirWithinValid(
    bool wrong_dir_within_valid)
{
  _dbTechLayerSpacingEolRule* obj = (_dbTechLayerSpacingEolRule*) this;

  obj->flags_.wrong_dir_within_valid_ = wrong_dir_within_valid;
}

bool dbTechLayerSpacingEolRule::isWrongDirWithinValid() const
{
  _dbTechLayerSpacingEolRule* obj = (_dbTechLayerSpacingEolRule*) this;

  return obj->flags_.wrong_dir_within_valid_;
}

void dbTechLayerSpacingEolRule::setSameMaskValid(bool same_mask_valid)
{
  _dbTechLayerSpacingEolRule* obj = (_dbTechLayerSpacingEolRule*) this;

  obj->flags_.same_mask_valid_ = same_mask_valid;
}

bool dbTechLayerSpacingEolRule::isSameMaskValid() const
{
  _dbTechLayerSpacingEolRule* obj = (_dbTechLayerSpacingEolRule*) this;

  return obj->flags_.same_mask_valid_;
}

void dbTechLayerSpacingEolRule::setExceptExactWidthValid(
    bool except_exact_width_valid)
{
  _dbTechLayerSpacingEolRule* obj = (_dbTechLayerSpacingEolRule*) this;

  obj->flags_.except_exact_width_valid_ = except_exact_width_valid;
}

bool dbTechLayerSpacingEolRule::isExceptExactWidthValid() const
{
  _dbTechLayerSpacingEolRule* obj = (_dbTechLayerSpacingEolRule*) this;

  return obj->flags_.except_exact_width_valid_;
}

void dbTechLayerSpacingEolRule::setFillConcaveCornerValid(
    bool fill_concave_corner_valid)
{
  _dbTechLayerSpacingEolRule* obj = (_dbTechLayerSpacingEolRule*) this;

  obj->flags_.fill_concave_corner_valid_ = fill_concave_corner_valid;
}

bool dbTechLayerSpacingEolRule::isFillConcaveCornerValid() const
{
  _dbTechLayerSpacingEolRule* obj = (_dbTechLayerSpacingEolRule*) this;

  return obj->flags_.fill_concave_corner_valid_;
}

void dbTechLayerSpacingEolRule::setWithcutValid(bool withcut_valid)
{
  _dbTechLayerSpacingEolRule* obj = (_dbTechLayerSpacingEolRule*) this;

  obj->flags_.withcut_valid_ = withcut_valid;
}

bool dbTechLayerSpacingEolRule::isWithcutValid() const
{
  _dbTechLayerSpacingEolRule* obj = (_dbTechLayerSpacingEolRule*) this;

  return obj->flags_.withcut_valid_;
}

void dbTechLayerSpacingEolRule::setCutClassValid(bool cut_class_valid)
{
  _dbTechLayerSpacingEolRule* obj = (_dbTechLayerSpacingEolRule*) this;

  obj->flags_.cut_class_valid_ = cut_class_valid;
}

bool dbTechLayerSpacingEolRule::isCutClassValid() const
{
  _dbTechLayerSpacingEolRule* obj = (_dbTechLayerSpacingEolRule*) this;

  return obj->flags_.cut_class_valid_;
}

void dbTechLayerSpacingEolRule::setWithCutAboveValid(bool with_cut_above_valid)
{
  _dbTechLayerSpacingEolRule* obj = (_dbTechLayerSpacingEolRule*) this;

  obj->flags_.with_cut_above_valid_ = with_cut_above_valid;
}

bool dbTechLayerSpacingEolRule::isWithCutAboveValid() const
{
  _dbTechLayerSpacingEolRule* obj = (_dbTechLayerSpacingEolRule*) this;

  return obj->flags_.with_cut_above_valid_;
}

void dbTechLayerSpacingEolRule::setEnclosureEndValid(bool enclosure_end_valid)
{
  _dbTechLayerSpacingEolRule* obj = (_dbTechLayerSpacingEolRule*) this;

  obj->flags_.enclosure_end_valid_ = enclosure_end_valid;
}

bool dbTechLayerSpacingEolRule::isEnclosureEndValid() const
{
  _dbTechLayerSpacingEolRule* obj = (_dbTechLayerSpacingEolRule*) this;

  return obj->flags_.enclosure_end_valid_;
}

void dbTechLayerSpacingEolRule::setEnclosureEndWithinValid(
    bool enclosure_end_within_valid)
{
  _dbTechLayerSpacingEolRule* obj = (_dbTechLayerSpacingEolRule*) this;

  obj->flags_.enclosure_end_within_valid_ = enclosure_end_within_valid;
}

bool dbTechLayerSpacingEolRule::isEnclosureEndWithinValid() const
{
  _dbTechLayerSpacingEolRule* obj = (_dbTechLayerSpacingEolRule*) this;

  return obj->flags_.enclosure_end_within_valid_;
}

void dbTechLayerSpacingEolRule::setEndPrlSpacingValid(
    bool end_prl_spacing_valid)
{
  _dbTechLayerSpacingEolRule* obj = (_dbTechLayerSpacingEolRule*) this;

  obj->flags_.end_prl_spacing_valid_ = end_prl_spacing_valid;
}

bool dbTechLayerSpacingEolRule::isEndPrlSpacingValid() const
{
  _dbTechLayerSpacingEolRule* obj = (_dbTechLayerSpacingEolRule*) this;

  return obj->flags_.end_prl_spacing_valid_;
}

void dbTechLayerSpacingEolRule::setPrlValid(bool prl_valid)
{
  _dbTechLayerSpacingEolRule* obj = (_dbTechLayerSpacingEolRule*) this;

  obj->flags_.prl_valid_ = prl_valid;
}

bool dbTechLayerSpacingEolRule::isPrlValid() const
{
  _dbTechLayerSpacingEolRule* obj = (_dbTechLayerSpacingEolRule*) this;

  return obj->flags_.prl_valid_;
}

void dbTechLayerSpacingEolRule::setEndToEndValid(bool end_to_end_valid)
{
  _dbTechLayerSpacingEolRule* obj = (_dbTechLayerSpacingEolRule*) this;

  obj->flags_.end_to_end_valid_ = end_to_end_valid;
}

bool dbTechLayerSpacingEolRule::isEndToEndValid() const
{
  _dbTechLayerSpacingEolRule* obj = (_dbTechLayerSpacingEolRule*) this;

  return obj->flags_.end_to_end_valid_;
}

void dbTechLayerSpacingEolRule::setCutSpacesValid(bool cut_spaces_valid)
{
  _dbTechLayerSpacingEolRule* obj = (_dbTechLayerSpacingEolRule*) this;

  obj->flags_.cut_spaces_valid_ = cut_spaces_valid;
}

bool dbTechLayerSpacingEolRule::isCutSpacesValid() const
{
  _dbTechLayerSpacingEolRule* obj = (_dbTechLayerSpacingEolRule*) this;

  return obj->flags_.cut_spaces_valid_;
}

void dbTechLayerSpacingEolRule::setExtensionValid(bool extension_valid)
{
  _dbTechLayerSpacingEolRule* obj = (_dbTechLayerSpacingEolRule*) this;

  obj->flags_.extension_valid_ = extension_valid;
}

bool dbTechLayerSpacingEolRule::isExtensionValid() const
{
  _dbTechLayerSpacingEolRule* obj = (_dbTechLayerSpacingEolRule*) this;

  return obj->flags_.extension_valid_;
}

void dbTechLayerSpacingEolRule::setWrongDirExtensionValid(
    bool wrong_dir_extension_valid)
{
  _dbTechLayerSpacingEolRule* obj = (_dbTechLayerSpacingEolRule*) this;

  obj->flags_.wrong_dir_extension_valid_ = wrong_dir_extension_valid;
}

bool dbTechLayerSpacingEolRule::isWrongDirExtensionValid() const
{
  _dbTechLayerSpacingEolRule* obj = (_dbTechLayerSpacingEolRule*) this;

  return obj->flags_.wrong_dir_extension_valid_;
}

void dbTechLayerSpacingEolRule::setOtherEndWidthValid(
    bool other_end_width_valid)
{
  _dbTechLayerSpacingEolRule* obj = (_dbTechLayerSpacingEolRule*) this;

  obj->flags_.other_end_width_valid_ = other_end_width_valid;
}

bool dbTechLayerSpacingEolRule::isOtherEndWidthValid() const
{
  _dbTechLayerSpacingEolRule* obj = (_dbTechLayerSpacingEolRule*) this;

  return obj->flags_.other_end_width_valid_;
}

void dbTechLayerSpacingEolRule::setMaxLengthValid(bool max_length_valid)
{
  _dbTechLayerSpacingEolRule* obj = (_dbTechLayerSpacingEolRule*) this;

  obj->flags_.max_length_valid_ = max_length_valid;
}

bool dbTechLayerSpacingEolRule::isMaxLengthValid() const
{
  _dbTechLayerSpacingEolRule* obj = (_dbTechLayerSpacingEolRule*) this;

  return obj->flags_.max_length_valid_;
}

void dbTechLayerSpacingEolRule::setMinLengthValid(bool min_length_valid)
{
  _dbTechLayerSpacingEolRule* obj = (_dbTechLayerSpacingEolRule*) this;

  obj->flags_.min_length_valid_ = min_length_valid;
}

bool dbTechLayerSpacingEolRule::isMinLengthValid() const
{
  _dbTechLayerSpacingEolRule* obj = (_dbTechLayerSpacingEolRule*) this;

  return obj->flags_.min_length_valid_;
}

void dbTechLayerSpacingEolRule::setTwoSidesValid(bool two_sides_valid)
{
  _dbTechLayerSpacingEolRule* obj = (_dbTechLayerSpacingEolRule*) this;

  obj->flags_.two_sides_valid_ = two_sides_valid;
}

bool dbTechLayerSpacingEolRule::isTwoSidesValid() const
{
  _dbTechLayerSpacingEolRule* obj = (_dbTechLayerSpacingEolRule*) this;

  return obj->flags_.two_sides_valid_;
}

void dbTechLayerSpacingEolRule::setEqualRectWidthValid(
    bool equal_rect_width_valid)
{
  _dbTechLayerSpacingEolRule* obj = (_dbTechLayerSpacingEolRule*) this;

  obj->flags_.equal_rect_width_valid_ = equal_rect_width_valid;
}

bool dbTechLayerSpacingEolRule::isEqualRectWidthValid() const
{
  _dbTechLayerSpacingEolRule* obj = (_dbTechLayerSpacingEolRule*) this;

  return obj->flags_.equal_rect_width_valid_;
}

void dbTechLayerSpacingEolRule::setParallelEdgeValid(bool parallel_edge_valid)
{
  _dbTechLayerSpacingEolRule* obj = (_dbTechLayerSpacingEolRule*) this;

  obj->flags_.parallel_edge_valid_ = parallel_edge_valid;
}

bool dbTechLayerSpacingEolRule::isParallelEdgeValid() const
{
  _dbTechLayerSpacingEolRule* obj = (_dbTechLayerSpacingEolRule*) this;

  return obj->flags_.parallel_edge_valid_;
}

void dbTechLayerSpacingEolRule::setSubtractEolWidthValid(
    bool subtract_eol_width_valid)
{
  _dbTechLayerSpacingEolRule* obj = (_dbTechLayerSpacingEolRule*) this;

  obj->flags_.subtract_eol_width_valid_ = subtract_eol_width_valid;
}

bool dbTechLayerSpacingEolRule::isSubtractEolWidthValid() const
{
  _dbTechLayerSpacingEolRule* obj = (_dbTechLayerSpacingEolRule*) this;

  return obj->flags_.subtract_eol_width_valid_;
}

void dbTechLayerSpacingEolRule::setParPrlValid(bool par_prl_valid)
{
  _dbTechLayerSpacingEolRule* obj = (_dbTechLayerSpacingEolRule*) this;

  obj->flags_.par_prl_valid_ = par_prl_valid;
}

bool dbTechLayerSpacingEolRule::isParPrlValid() const
{
  _dbTechLayerSpacingEolRule* obj = (_dbTechLayerSpacingEolRule*) this;

  return obj->flags_.par_prl_valid_;
}

void dbTechLayerSpacingEolRule::setParMinLengthValid(bool par_min_length_valid)
{
  _dbTechLayerSpacingEolRule* obj = (_dbTechLayerSpacingEolRule*) this;

  obj->flags_.par_min_length_valid_ = par_min_length_valid;
}

bool dbTechLayerSpacingEolRule::isParMinLengthValid() const
{
  _dbTechLayerSpacingEolRule* obj = (_dbTechLayerSpacingEolRule*) this;

  return obj->flags_.par_min_length_valid_;
}

void dbTechLayerSpacingEolRule::setTwoEdgesValid(bool two_edges_valid)
{
  _dbTechLayerSpacingEolRule* obj = (_dbTechLayerSpacingEolRule*) this;

  obj->flags_.two_edges_valid_ = two_edges_valid;
}

bool dbTechLayerSpacingEolRule::isTwoEdgesValid() const
{
  _dbTechLayerSpacingEolRule* obj = (_dbTechLayerSpacingEolRule*) this;

  return obj->flags_.two_edges_valid_;
}

void dbTechLayerSpacingEolRule::setSameMetalValid(bool same_metal_valid)
{
  _dbTechLayerSpacingEolRule* obj = (_dbTechLayerSpacingEolRule*) this;

  obj->flags_.same_metal_valid_ = same_metal_valid;
}

bool dbTechLayerSpacingEolRule::isSameMetalValid() const
{
  _dbTechLayerSpacingEolRule* obj = (_dbTechLayerSpacingEolRule*) this;

  return obj->flags_.same_metal_valid_;
}

void dbTechLayerSpacingEolRule::setNonEolCornerOnlyValid(
    bool non_eol_corner_only_valid)
{
  _dbTechLayerSpacingEolRule* obj = (_dbTechLayerSpacingEolRule*) this;

  obj->flags_.non_eol_corner_only_valid_ = non_eol_corner_only_valid;
}

bool dbTechLayerSpacingEolRule::isNonEolCornerOnlyValid() const
{
  _dbTechLayerSpacingEolRule* obj = (_dbTechLayerSpacingEolRule*) this;

  return obj->flags_.non_eol_corner_only_valid_;
}

void dbTechLayerSpacingEolRule::setParallelSameMaskValid(
    bool parallel_same_mask_valid)
{
  _dbTechLayerSpacingEolRule* obj = (_dbTechLayerSpacingEolRule*) this;

  obj->flags_.parallel_same_mask_valid_ = parallel_same_mask_valid;
}

bool dbTechLayerSpacingEolRule::isParallelSameMaskValid() const
{
  _dbTechLayerSpacingEolRule* obj = (_dbTechLayerSpacingEolRule*) this;

  return obj->flags_.parallel_same_mask_valid_;
}

void dbTechLayerSpacingEolRule::setEncloseCutValid(bool enclose_cut_valid)
{
  _dbTechLayerSpacingEolRule* obj = (_dbTechLayerSpacingEolRule*) this;

  obj->flags_.enclose_cut_valid_ = enclose_cut_valid;
}

bool dbTechLayerSpacingEolRule::isEncloseCutValid() const
{
  _dbTechLayerSpacingEolRule* obj = (_dbTechLayerSpacingEolRule*) this;

  return obj->flags_.enclose_cut_valid_;
}

void dbTechLayerSpacingEolRule::setBelowValid(bool below_valid)
{
  _dbTechLayerSpacingEolRule* obj = (_dbTechLayerSpacingEolRule*) this;

  obj->flags_.below_valid_ = below_valid;
}

bool dbTechLayerSpacingEolRule::isBelowValid() const
{
  _dbTechLayerSpacingEolRule* obj = (_dbTechLayerSpacingEolRule*) this;

  return obj->flags_.below_valid_;
}

void dbTechLayerSpacingEolRule::setAboveValid(bool above_valid)
{
  _dbTechLayerSpacingEolRule* obj = (_dbTechLayerSpacingEolRule*) this;

  obj->flags_.above_valid_ = above_valid;
}

bool dbTechLayerSpacingEolRule::isAboveValid() const
{
  _dbTechLayerSpacingEolRule* obj = (_dbTechLayerSpacingEolRule*) this;

  return obj->flags_.above_valid_;
}

void dbTechLayerSpacingEolRule::setCutSpacingValid(bool cut_spacing_valid)
{
  _dbTechLayerSpacingEolRule* obj = (_dbTechLayerSpacingEolRule*) this;

  obj->flags_.cut_spacing_valid_ = cut_spacing_valid;
}

bool dbTechLayerSpacingEolRule::isCutSpacingValid() const
{
  _dbTechLayerSpacingEolRule* obj = (_dbTechLayerSpacingEolRule*) this;

  return obj->flags_.cut_spacing_valid_;
}

void dbTechLayerSpacingEolRule::setAllCutsValid(bool all_cuts_valid)
{
  _dbTechLayerSpacingEolRule* obj = (_dbTechLayerSpacingEolRule*) this;

  obj->flags_.all_cuts_valid_ = all_cuts_valid;
}

bool dbTechLayerSpacingEolRule::isAllCutsValid() const
{
  _dbTechLayerSpacingEolRule* obj = (_dbTechLayerSpacingEolRule*) this;

  return obj->flags_.all_cuts_valid_;
}

void dbTechLayerSpacingEolRule::setToConcaveCornerValid(
    bool to_concave_corner_valid)
{
  _dbTechLayerSpacingEolRule* obj = (_dbTechLayerSpacingEolRule*) this;

  obj->flags_.to_concave_corner_valid_ = to_concave_corner_valid;
}

bool dbTechLayerSpacingEolRule::isToConcaveCornerValid() const
{
  _dbTechLayerSpacingEolRule* obj = (_dbTechLayerSpacingEolRule*) this;

  return obj->flags_.to_concave_corner_valid_;
}

void dbTechLayerSpacingEolRule::setMinAdjacentLengthValid(
    bool min_adjacent_length_valid)
{
  _dbTechLayerSpacingEolRule* obj = (_dbTechLayerSpacingEolRule*) this;

  obj->flags_.min_adjacent_length_valid_ = min_adjacent_length_valid;
}

bool dbTechLayerSpacingEolRule::isMinAdjacentLengthValid() const
{
  _dbTechLayerSpacingEolRule* obj = (_dbTechLayerSpacingEolRule*) this;

  return obj->flags_.min_adjacent_length_valid_;
}

void dbTechLayerSpacingEolRule::setTwoMinAdjLengthValid(
    bool two_min_adj_length_valid)
{
  _dbTechLayerSpacingEolRule* obj = (_dbTechLayerSpacingEolRule*) this;

  obj->flags_.two_min_adj_length_valid_ = two_min_adj_length_valid;
}

bool dbTechLayerSpacingEolRule::isTwoMinAdjLengthValid() const
{
  _dbTechLayerSpacingEolRule* obj = (_dbTechLayerSpacingEolRule*) this;

  return obj->flags_.two_min_adj_length_valid_;
}

void dbTechLayerSpacingEolRule::setToNotchLengthValid(
    bool to_notch_length_valid)
{
  _dbTechLayerSpacingEolRule* obj = (_dbTechLayerSpacingEolRule*) this;

  obj->flags_.to_notch_length_valid_ = to_notch_length_valid;
}

bool dbTechLayerSpacingEolRule::isToNotchLengthValid() const
{
  _dbTechLayerSpacingEolRule* obj = (_dbTechLayerSpacingEolRule*) this;

  return obj->flags_.to_notch_length_valid_;
}

// User Code Begin dbTechLayerSpacingEolRulePublicMethods
dbTechLayerSpacingEolRule* dbTechLayerSpacingEolRule::create(
    dbTechLayer* _layer)
{
  _dbTechLayer* layer = (_dbTechLayer*) _layer;
  _dbTechLayerSpacingEolRule* newrule = layer->spacing_eol_rules_tbl_->create();
  newrule->_layer = _layer->getImpl()->getOID();

  return ((dbTechLayerSpacingEolRule*) newrule);
}

dbTechLayerSpacingEolRule*
dbTechLayerSpacingEolRule::getTechLayerSpacingEolRule(dbTechLayer* inly,
                                                      uint dbid)
{
  _dbTechLayer* layer = (_dbTechLayer*) inly;
  return (dbTechLayerSpacingEolRule*) layer->spacing_eol_rules_tbl_->getPtr(
      dbid);
}

void dbTechLayerSpacingEolRule::destroy(dbTechLayerSpacingEolRule* rule)
{
  _dbTechLayer* layer = (_dbTechLayer*) rule->getImpl()->getOwner();
  dbProperty::destroyProperties(rule);
  layer->spacing_eol_rules_tbl_->destroy((_dbTechLayerSpacingEolRule*) rule);
}

// User Code End dbTechLayerSpacingEolRulePublicMethods
}  // namespace odb
   // Generator Code End Cpp
