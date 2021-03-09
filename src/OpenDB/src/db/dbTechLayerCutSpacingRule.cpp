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

// Generator Code Begin cpp
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
  if (flags_.center_to_center_ != rhs.flags_.center_to_center_)
    return false;

  if (flags_.same_net_ != rhs.flags_.same_net_)
    return false;

  if (flags_.same_metal_ != rhs.flags_.same_metal_)
    return false;

  if (flags_.same_via_ != rhs.flags_.same_via_)
    return false;

  if (flags_.cut_spacing_type_ != rhs.flags_.cut_spacing_type_)
    return false;

  if (flags_.stack_ != rhs.flags_.stack_)
    return false;

  if (flags_.orthogonal_spacing_valid_ != rhs.flags_.orthogonal_spacing_valid_)
    return false;

  if (flags_.above_width_enclosure_valid_
      != rhs.flags_.above_width_enclosure_valid_)
    return false;

  if (flags_.short_edge_only_ != rhs.flags_.short_edge_only_)
    return false;

  if (flags_.concave_corner_width_ != rhs.flags_.concave_corner_width_)
    return false;

  if (flags_.concave_corner_parallel_ != rhs.flags_.concave_corner_parallel_)
    return false;

  if (flags_.concave_corner_edge_length_
      != rhs.flags_.concave_corner_edge_length_)
    return false;

  if (flags_.concave_corner_ != rhs.flags_.concave_corner_)
    return false;

  if (flags_.extension_valid_ != rhs.flags_.extension_valid_)
    return false;

  if (flags_.non_eol_convex_corner_ != rhs.flags_.non_eol_convex_corner_)
    return false;

  if (flags_.eol_width_valid_ != rhs.flags_.eol_width_valid_)
    return false;

  if (flags_.min_length_valid_ != rhs.flags_.min_length_valid_)
    return false;

  if (flags_.above_width_valid_ != rhs.flags_.above_width_valid_)
    return false;

  if (flags_.mask_overlap_ != rhs.flags_.mask_overlap_)
    return false;

  if (flags_.wrong_direction_ != rhs.flags_.wrong_direction_)
    return false;

  if (flags_.adjacent_cuts_ != rhs.flags_.adjacent_cuts_)
    return false;

  if (flags_.exact_aligned_ != rhs.flags_.exact_aligned_)
    return false;

  if (flags_.cut_class_to_all_ != rhs.flags_.cut_class_to_all_)
    return false;

  if (flags_.no_prl_ != rhs.flags_.no_prl_)
    return false;

  if (flags_.same_mask_ != rhs.flags_.same_mask_)
    return false;

  if (flags_.except_same_pgnet_ != rhs.flags_.except_same_pgnet_)
    return false;

  if (flags_.side_parallel_overlap_ != rhs.flags_.side_parallel_overlap_)
    return false;

  if (flags_.except_same_net_ != rhs.flags_.except_same_net_)
    return false;

  if (flags_.except_same_metal_ != rhs.flags_.except_same_metal_)
    return false;

  if (flags_.except_same_metal_overlap_
      != rhs.flags_.except_same_metal_overlap_)
    return false;

  if (flags_.except_same_via_ != rhs.flags_.except_same_via_)
    return false;

  if (flags_.above_ != rhs.flags_.above_)
    return false;

  if (flags_.except_two_edges_ != rhs.flags_.except_two_edges_)
    return false;

  if (flags_.two_cuts_valid_ != rhs.flags_.two_cuts_valid_)
    return false;

  if (flags_.same_cut_ != rhs.flags_.same_cut_)
    return false;

  if (flags_.long_edge_only_ != rhs.flags_.long_edge_only_)
    return false;

  if (flags_.prl_valid_ != rhs.flags_.prl_valid_)
    return false;

  if (flags_.below_ != rhs.flags_.below_)
    return false;

  if (flags_.par_within_enclosure_valid_
      != rhs.flags_.par_within_enclosure_valid_)
    return false;

  if (cut_spacing_ != rhs.cut_spacing_)
    return false;

  if (second_layer_ != rhs.second_layer_)
    return false;

  if (orthogonal_spacing_ != rhs.orthogonal_spacing_)
    return false;

  if (width_ != rhs.width_)
    return false;

  if (enclosure_ != rhs.enclosure_)
    return false;

  if (edge_length_ != rhs.edge_length_)
    return false;

  if (par_within_ != rhs.par_within_)
    return false;

  if (par_enclosure_ != rhs.par_enclosure_)
    return false;

  if (edge_enclosure_ != rhs.edge_enclosure_)
    return false;

  if (adj_enclosure_ != rhs.adj_enclosure_)
    return false;

  if (above_enclosure_ != rhs.above_enclosure_)
    return false;

  if (above_width_ != rhs.above_width_)
    return false;

  if (min_length_ != rhs.min_length_)
    return false;

  if (extension_ != rhs.extension_)
    return false;

  if (eol_width_ != rhs.eol_width_)
    return false;

  if (num_cuts_ != rhs.num_cuts_)
    return false;

  if (within_ != rhs.within_)
    return false;

  if (second_within_ != rhs.second_within_)
    return false;

  if (cut_class_ != rhs.cut_class_)
    return false;

  if (two_cuts_ != rhs.two_cuts_)
    return false;

  if (prl_ != rhs.prl_)
    return false;

  if (par_length_ != rhs.par_length_)
    return false;

  if (cut_area_ != rhs.cut_area_)
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

  DIFF_FIELD(flags_.center_to_center_);
  DIFF_FIELD(flags_.same_net_);
  DIFF_FIELD(flags_.same_metal_);
  DIFF_FIELD(flags_.same_via_);
  DIFF_FIELD(flags_.cut_spacing_type_);
  DIFF_FIELD(flags_.stack_);
  DIFF_FIELD(flags_.orthogonal_spacing_valid_);
  DIFF_FIELD(flags_.above_width_enclosure_valid_);
  DIFF_FIELD(flags_.short_edge_only_);
  DIFF_FIELD(flags_.concave_corner_width_);
  DIFF_FIELD(flags_.concave_corner_parallel_);
  DIFF_FIELD(flags_.concave_corner_edge_length_);
  DIFF_FIELD(flags_.concave_corner_);
  DIFF_FIELD(flags_.extension_valid_);
  DIFF_FIELD(flags_.non_eol_convex_corner_);
  DIFF_FIELD(flags_.eol_width_valid_);
  DIFF_FIELD(flags_.min_length_valid_);
  DIFF_FIELD(flags_.above_width_valid_);
  DIFF_FIELD(flags_.mask_overlap_);
  DIFF_FIELD(flags_.wrong_direction_);
  DIFF_FIELD(flags_.adjacent_cuts_);
  DIFF_FIELD(flags_.exact_aligned_);
  DIFF_FIELD(flags_.cut_class_to_all_);
  DIFF_FIELD(flags_.no_prl_);
  DIFF_FIELD(flags_.same_mask_);
  DIFF_FIELD(flags_.except_same_pgnet_);
  DIFF_FIELD(flags_.side_parallel_overlap_);
  DIFF_FIELD(flags_.except_same_net_);
  DIFF_FIELD(flags_.except_same_metal_);
  DIFF_FIELD(flags_.except_same_metal_overlap_);
  DIFF_FIELD(flags_.except_same_via_);
  DIFF_FIELD(flags_.above_);
  DIFF_FIELD(flags_.except_two_edges_);
  DIFF_FIELD(flags_.two_cuts_valid_);
  DIFF_FIELD(flags_.same_cut_);
  DIFF_FIELD(flags_.long_edge_only_);
  DIFF_FIELD(flags_.prl_valid_);
  DIFF_FIELD(flags_.below_);
  DIFF_FIELD(flags_.par_within_enclosure_valid_);
  DIFF_FIELD(cut_spacing_);
  DIFF_FIELD(second_layer_);
  DIFF_FIELD(orthogonal_spacing_);
  DIFF_FIELD(width_);
  DIFF_FIELD(enclosure_);
  DIFF_FIELD(edge_length_);
  DIFF_FIELD(par_within_);
  DIFF_FIELD(par_enclosure_);
  DIFF_FIELD(edge_enclosure_);
  DIFF_FIELD(adj_enclosure_);
  DIFF_FIELD(above_enclosure_);
  DIFF_FIELD(above_width_);
  DIFF_FIELD(min_length_);
  DIFF_FIELD(extension_);
  DIFF_FIELD(eol_width_);
  DIFF_FIELD(num_cuts_);
  DIFF_FIELD(within_);
  DIFF_FIELD(second_within_);
  DIFF_FIELD(cut_class_);
  DIFF_FIELD(two_cuts_);
  DIFF_FIELD(prl_);
  DIFF_FIELD(par_length_);
  DIFF_FIELD(cut_area_);
  // User Code Begin differences
  // User Code End differences
  DIFF_END
}
void _dbTechLayerCutSpacingRule::out(dbDiff&     diff,
                                     char        side,
                                     const char* field) const
{
  DIFF_OUT_BEGIN
  DIFF_OUT_FIELD(flags_.center_to_center_);
  DIFF_OUT_FIELD(flags_.same_net_);
  DIFF_OUT_FIELD(flags_.same_metal_);
  DIFF_OUT_FIELD(flags_.same_via_);
  DIFF_OUT_FIELD(flags_.cut_spacing_type_);
  DIFF_OUT_FIELD(flags_.stack_);
  DIFF_OUT_FIELD(flags_.orthogonal_spacing_valid_);
  DIFF_OUT_FIELD(flags_.above_width_enclosure_valid_);
  DIFF_OUT_FIELD(flags_.short_edge_only_);
  DIFF_OUT_FIELD(flags_.concave_corner_width_);
  DIFF_OUT_FIELD(flags_.concave_corner_parallel_);
  DIFF_OUT_FIELD(flags_.concave_corner_edge_length_);
  DIFF_OUT_FIELD(flags_.concave_corner_);
  DIFF_OUT_FIELD(flags_.extension_valid_);
  DIFF_OUT_FIELD(flags_.non_eol_convex_corner_);
  DIFF_OUT_FIELD(flags_.eol_width_valid_);
  DIFF_OUT_FIELD(flags_.min_length_valid_);
  DIFF_OUT_FIELD(flags_.above_width_valid_);
  DIFF_OUT_FIELD(flags_.mask_overlap_);
  DIFF_OUT_FIELD(flags_.wrong_direction_);
  DIFF_OUT_FIELD(flags_.adjacent_cuts_);
  DIFF_OUT_FIELD(flags_.exact_aligned_);
  DIFF_OUT_FIELD(flags_.cut_class_to_all_);
  DIFF_OUT_FIELD(flags_.no_prl_);
  DIFF_OUT_FIELD(flags_.same_mask_);
  DIFF_OUT_FIELD(flags_.except_same_pgnet_);
  DIFF_OUT_FIELD(flags_.side_parallel_overlap_);
  DIFF_OUT_FIELD(flags_.except_same_net_);
  DIFF_OUT_FIELD(flags_.except_same_metal_);
  DIFF_OUT_FIELD(flags_.except_same_metal_overlap_);
  DIFF_OUT_FIELD(flags_.except_same_via_);
  DIFF_OUT_FIELD(flags_.above_);
  DIFF_OUT_FIELD(flags_.except_two_edges_);
  DIFF_OUT_FIELD(flags_.two_cuts_valid_);
  DIFF_OUT_FIELD(flags_.same_cut_);
  DIFF_OUT_FIELD(flags_.long_edge_only_);
  DIFF_OUT_FIELD(flags_.prl_valid_);
  DIFF_OUT_FIELD(flags_.below_);
  DIFF_OUT_FIELD(flags_.par_within_enclosure_valid_);
  DIFF_OUT_FIELD(cut_spacing_);
  DIFF_OUT_FIELD(second_layer_);
  DIFF_OUT_FIELD(orthogonal_spacing_);
  DIFF_OUT_FIELD(width_);
  DIFF_OUT_FIELD(enclosure_);
  DIFF_OUT_FIELD(edge_length_);
  DIFF_OUT_FIELD(par_within_);
  DIFF_OUT_FIELD(par_enclosure_);
  DIFF_OUT_FIELD(edge_enclosure_);
  DIFF_OUT_FIELD(adj_enclosure_);
  DIFF_OUT_FIELD(above_enclosure_);
  DIFF_OUT_FIELD(above_width_);
  DIFF_OUT_FIELD(min_length_);
  DIFF_OUT_FIELD(extension_);
  DIFF_OUT_FIELD(eol_width_);
  DIFF_OUT_FIELD(num_cuts_);
  DIFF_OUT_FIELD(within_);
  DIFF_OUT_FIELD(second_within_);
  DIFF_OUT_FIELD(cut_class_);
  DIFF_OUT_FIELD(two_cuts_);
  DIFF_OUT_FIELD(prl_);
  DIFF_OUT_FIELD(par_length_);
  DIFF_OUT_FIELD(cut_area_);

  // User Code Begin out
  // User Code End out
  DIFF_END
}
_dbTechLayerCutSpacingRule::_dbTechLayerCutSpacingRule(_dbDatabase* db)
{
  uint64_t* flags__bit_field = (uint64_t*) &flags_;
  *flags__bit_field          = 0;
  // User Code Begin constructor
  // User Code End constructor
}
_dbTechLayerCutSpacingRule::_dbTechLayerCutSpacingRule(
    _dbDatabase*                      db,
    const _dbTechLayerCutSpacingRule& r)
{
  flags_.center_to_center_            = r.flags_.center_to_center_;
  flags_.same_net_                    = r.flags_.same_net_;
  flags_.same_metal_                  = r.flags_.same_metal_;
  flags_.same_via_                    = r.flags_.same_via_;
  flags_.cut_spacing_type_            = r.flags_.cut_spacing_type_;
  flags_.stack_                       = r.flags_.stack_;
  flags_.orthogonal_spacing_valid_    = r.flags_.orthogonal_spacing_valid_;
  flags_.above_width_enclosure_valid_ = r.flags_.above_width_enclosure_valid_;
  flags_.short_edge_only_             = r.flags_.short_edge_only_;
  flags_.concave_corner_width_        = r.flags_.concave_corner_width_;
  flags_.concave_corner_parallel_     = r.flags_.concave_corner_parallel_;
  flags_.concave_corner_edge_length_  = r.flags_.concave_corner_edge_length_;
  flags_.concave_corner_              = r.flags_.concave_corner_;
  flags_.extension_valid_             = r.flags_.extension_valid_;
  flags_.non_eol_convex_corner_       = r.flags_.non_eol_convex_corner_;
  flags_.eol_width_valid_             = r.flags_.eol_width_valid_;
  flags_.min_length_valid_            = r.flags_.min_length_valid_;
  flags_.above_width_valid_           = r.flags_.above_width_valid_;
  flags_.mask_overlap_                = r.flags_.mask_overlap_;
  flags_.wrong_direction_             = r.flags_.wrong_direction_;
  flags_.adjacent_cuts_               = r.flags_.adjacent_cuts_;
  flags_.exact_aligned_               = r.flags_.exact_aligned_;
  flags_.cut_class_to_all_            = r.flags_.cut_class_to_all_;
  flags_.no_prl_                      = r.flags_.no_prl_;
  flags_.same_mask_                   = r.flags_.same_mask_;
  flags_.except_same_pgnet_           = r.flags_.except_same_pgnet_;
  flags_.side_parallel_overlap_       = r.flags_.side_parallel_overlap_;
  flags_.except_same_net_             = r.flags_.except_same_net_;
  flags_.except_same_metal_           = r.flags_.except_same_metal_;
  flags_.except_same_metal_overlap_   = r.flags_.except_same_metal_overlap_;
  flags_.except_same_via_             = r.flags_.except_same_via_;
  flags_.above_                       = r.flags_.above_;
  flags_.except_two_edges_            = r.flags_.except_two_edges_;
  flags_.two_cuts_valid_              = r.flags_.two_cuts_valid_;
  flags_.same_cut_                    = r.flags_.same_cut_;
  flags_.long_edge_only_              = r.flags_.long_edge_only_;
  flags_.prl_valid_                   = r.flags_.prl_valid_;
  flags_.below_                       = r.flags_.below_;
  flags_.par_within_enclosure_valid_  = r.flags_.par_within_enclosure_valid_;
  flags_.spare_bits_                  = r.flags_.spare_bits_;
  cut_spacing_                        = r.cut_spacing_;
  second_layer_                       = r.second_layer_;
  orthogonal_spacing_                 = r.orthogonal_spacing_;
  width_                              = r.width_;
  enclosure_                          = r.enclosure_;
  edge_length_                        = r.edge_length_;
  par_within_                         = r.par_within_;
  par_enclosure_                      = r.par_enclosure_;
  edge_enclosure_                     = r.edge_enclosure_;
  adj_enclosure_                      = r.adj_enclosure_;
  above_enclosure_                    = r.above_enclosure_;
  above_width_                        = r.above_width_;
  min_length_                         = r.min_length_;
  extension_                          = r.extension_;
  eol_width_                          = r.eol_width_;
  num_cuts_                           = r.num_cuts_;
  within_                             = r.within_;
  second_within_                      = r.second_within_;
  cut_class_                          = r.cut_class_;
  two_cuts_                           = r.two_cuts_;
  prl_                                = r.prl_;
  par_length_                         = r.par_length_;
  cut_area_                           = r.cut_area_;
  // User Code Begin CopyConstructor
  // User Code End CopyConstructor
}

dbIStream& operator>>(dbIStream& stream, _dbTechLayerCutSpacingRule& obj)
{
  uint64_t* flags__bit_field = (uint64_t*) &obj.flags_;
  stream >> *flags__bit_field;
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
  // User Code Begin >>
  // User Code End >>
  return stream;
}
dbOStream& operator<<(dbOStream& stream, const _dbTechLayerCutSpacingRule& obj)
{
  uint64_t* flags__bit_field = (uint64_t*) &obj.flags_;
  stream << *flags__bit_field;
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
  // User Code Begin <<
  // User Code End <<
  return stream;
}

_dbTechLayerCutSpacingRule::~_dbTechLayerCutSpacingRule()
{
  // User Code Begin Destructor
  // User Code End Destructor
}

// User Code Begin PrivateMethods
// User Code End PrivateMethods

////////////////////////////////////////////////////////////////////
//
// dbTechLayerCutSpacingRule - Methods
//
////////////////////////////////////////////////////////////////////

void dbTechLayerCutSpacingRule::setCutSpacing(int cut_spacing_)
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;

  obj->cut_spacing_ = cut_spacing_;
}

int dbTechLayerCutSpacingRule::getCutSpacing() const
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;
  return obj->cut_spacing_;
}

void dbTechLayerCutSpacingRule::setSecondLayer(dbTechLayer* second_layer_)
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;

  obj->second_layer_ = second_layer_->getImpl()->getOID();
}

void dbTechLayerCutSpacingRule::setOrthogonalSpacing(int orthogonal_spacing_)
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;

  obj->orthogonal_spacing_ = orthogonal_spacing_;
}

int dbTechLayerCutSpacingRule::getOrthogonalSpacing() const
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;
  return obj->orthogonal_spacing_;
}

void dbTechLayerCutSpacingRule::setWidth(int width_)
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;

  obj->width_ = width_;
}

int dbTechLayerCutSpacingRule::getWidth() const
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;
  return obj->width_;
}

void dbTechLayerCutSpacingRule::setEnclosure(int enclosure_)
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;

  obj->enclosure_ = enclosure_;
}

int dbTechLayerCutSpacingRule::getEnclosure() const
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;
  return obj->enclosure_;
}

void dbTechLayerCutSpacingRule::setEdgeLength(int edge_length_)
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;

  obj->edge_length_ = edge_length_;
}

int dbTechLayerCutSpacingRule::getEdgeLength() const
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;
  return obj->edge_length_;
}

void dbTechLayerCutSpacingRule::setParWithin(int par_within_)
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;

  obj->par_within_ = par_within_;
}

int dbTechLayerCutSpacingRule::getParWithin() const
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;
  return obj->par_within_;
}

void dbTechLayerCutSpacingRule::setParEnclosure(int par_enclosure_)
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;

  obj->par_enclosure_ = par_enclosure_;
}

int dbTechLayerCutSpacingRule::getParEnclosure() const
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;
  return obj->par_enclosure_;
}

void dbTechLayerCutSpacingRule::setEdgeEnclosure(int edge_enclosure_)
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;

  obj->edge_enclosure_ = edge_enclosure_;
}

int dbTechLayerCutSpacingRule::getEdgeEnclosure() const
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;
  return obj->edge_enclosure_;
}

void dbTechLayerCutSpacingRule::setAdjEnclosure(int adj_enclosure_)
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;

  obj->adj_enclosure_ = adj_enclosure_;
}

int dbTechLayerCutSpacingRule::getAdjEnclosure() const
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;
  return obj->adj_enclosure_;
}

void dbTechLayerCutSpacingRule::setAboveEnclosure(int above_enclosure_)
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;

  obj->above_enclosure_ = above_enclosure_;
}

int dbTechLayerCutSpacingRule::getAboveEnclosure() const
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;
  return obj->above_enclosure_;
}

void dbTechLayerCutSpacingRule::setAboveWidth(int above_width_)
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;

  obj->above_width_ = above_width_;
}

int dbTechLayerCutSpacingRule::getAboveWidth() const
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;
  return obj->above_width_;
}

void dbTechLayerCutSpacingRule::setMinLength(int min_length_)
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;

  obj->min_length_ = min_length_;
}

int dbTechLayerCutSpacingRule::getMinLength() const
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;
  return obj->min_length_;
}

void dbTechLayerCutSpacingRule::setExtension(int extension_)
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;

  obj->extension_ = extension_;
}

int dbTechLayerCutSpacingRule::getExtension() const
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;
  return obj->extension_;
}

void dbTechLayerCutSpacingRule::setEolWidth(int eol_width_)
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;

  obj->eol_width_ = eol_width_;
}

int dbTechLayerCutSpacingRule::getEolWidth() const
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;
  return obj->eol_width_;
}

void dbTechLayerCutSpacingRule::setNumCuts(uint num_cuts_)
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;

  obj->num_cuts_ = num_cuts_;
}

uint dbTechLayerCutSpacingRule::getNumCuts() const
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;
  return obj->num_cuts_;
}

void dbTechLayerCutSpacingRule::setWithin(int within_)
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;

  obj->within_ = within_;
}

int dbTechLayerCutSpacingRule::getWithin() const
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;
  return obj->within_;
}

void dbTechLayerCutSpacingRule::setSecondWithin(int second_within_)
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;

  obj->second_within_ = second_within_;
}

int dbTechLayerCutSpacingRule::getSecondWithin() const
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;
  return obj->second_within_;
}

void dbTechLayerCutSpacingRule::setCutClass(dbTechLayerCutClassRule* cut_class_)
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;

  obj->cut_class_ = cut_class_->getImpl()->getOID();
}

void dbTechLayerCutSpacingRule::setTwoCuts(uint two_cuts_)
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;

  obj->two_cuts_ = two_cuts_;
}

uint dbTechLayerCutSpacingRule::getTwoCuts() const
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;
  return obj->two_cuts_;
}

void dbTechLayerCutSpacingRule::setPrl(uint prl_)
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;

  obj->prl_ = prl_;
}

uint dbTechLayerCutSpacingRule::getPrl() const
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;
  return obj->prl_;
}

void dbTechLayerCutSpacingRule::setParLength(uint par_length_)
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;

  obj->par_length_ = par_length_;
}

uint dbTechLayerCutSpacingRule::getParLength() const
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;
  return obj->par_length_;
}

void dbTechLayerCutSpacingRule::setCutArea(int cut_area_)
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;

  obj->cut_area_ = cut_area_;
}

int dbTechLayerCutSpacingRule::getCutArea() const
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;
  return obj->cut_area_;
}

void dbTechLayerCutSpacingRule::setCenterToCenter(bool center_to_center_)
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;

  obj->flags_.center_to_center_ = center_to_center_;
}

bool dbTechLayerCutSpacingRule::isCenterToCenter() const
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;

  return obj->flags_.center_to_center_;
}

void dbTechLayerCutSpacingRule::setSameNet(bool same_net_)
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;

  obj->flags_.same_net_ = same_net_;
}

bool dbTechLayerCutSpacingRule::isSameNet() const
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;

  return obj->flags_.same_net_;
}

void dbTechLayerCutSpacingRule::setSameMetal(bool same_metal_)
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;

  obj->flags_.same_metal_ = same_metal_;
}

bool dbTechLayerCutSpacingRule::isSameMetal() const
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;

  return obj->flags_.same_metal_;
}

void dbTechLayerCutSpacingRule::setSameVia(bool same_via_)
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;

  obj->flags_.same_via_ = same_via_;
}

bool dbTechLayerCutSpacingRule::isSameVia() const
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;

  return obj->flags_.same_via_;
}

void dbTechLayerCutSpacingRule::setStack(bool stack_)
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;

  obj->flags_.stack_ = stack_;
}

bool dbTechLayerCutSpacingRule::isStack() const
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;

  return obj->flags_.stack_;
}

void dbTechLayerCutSpacingRule::setOrthogonalSpacingValid(
    bool orthogonal_spacing_valid_)
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;

  obj->flags_.orthogonal_spacing_valid_ = orthogonal_spacing_valid_;
}

bool dbTechLayerCutSpacingRule::isOrthogonalSpacingValid() const
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;

  return obj->flags_.orthogonal_spacing_valid_;
}

void dbTechLayerCutSpacingRule::setAboveWidthEnclosureValid(
    bool above_width_enclosure_valid_)
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;

  obj->flags_.above_width_enclosure_valid_ = above_width_enclosure_valid_;
}

bool dbTechLayerCutSpacingRule::isAboveWidthEnclosureValid() const
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;

  return obj->flags_.above_width_enclosure_valid_;
}

void dbTechLayerCutSpacingRule::setShortEdgeOnly(bool short_edge_only_)
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;

  obj->flags_.short_edge_only_ = short_edge_only_;
}

bool dbTechLayerCutSpacingRule::isShortEdgeOnly() const
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;

  return obj->flags_.short_edge_only_;
}

void dbTechLayerCutSpacingRule::setConcaveCornerWidth(
    bool concave_corner_width_)
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;

  obj->flags_.concave_corner_width_ = concave_corner_width_;
}

bool dbTechLayerCutSpacingRule::isConcaveCornerWidth() const
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;

  return obj->flags_.concave_corner_width_;
}

void dbTechLayerCutSpacingRule::setConcaveCornerParallel(
    bool concave_corner_parallel_)
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;

  obj->flags_.concave_corner_parallel_ = concave_corner_parallel_;
}

bool dbTechLayerCutSpacingRule::isConcaveCornerParallel() const
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;

  return obj->flags_.concave_corner_parallel_;
}

void dbTechLayerCutSpacingRule::setConcaveCornerEdgeLength(
    bool concave_corner_edge_length_)
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;

  obj->flags_.concave_corner_edge_length_ = concave_corner_edge_length_;
}

bool dbTechLayerCutSpacingRule::isConcaveCornerEdgeLength() const
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;

  return obj->flags_.concave_corner_edge_length_;
}

void dbTechLayerCutSpacingRule::setConcaveCorner(bool concave_corner_)
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;

  obj->flags_.concave_corner_ = concave_corner_;
}

bool dbTechLayerCutSpacingRule::isConcaveCorner() const
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;

  return obj->flags_.concave_corner_;
}

void dbTechLayerCutSpacingRule::setExtensionValid(bool extension_valid_)
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;

  obj->flags_.extension_valid_ = extension_valid_;
}

bool dbTechLayerCutSpacingRule::isExtensionValid() const
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;

  return obj->flags_.extension_valid_;
}

void dbTechLayerCutSpacingRule::setNonEolConvexCorner(
    bool non_eol_convex_corner_)
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;

  obj->flags_.non_eol_convex_corner_ = non_eol_convex_corner_;
}

bool dbTechLayerCutSpacingRule::isNonEolConvexCorner() const
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;

  return obj->flags_.non_eol_convex_corner_;
}

void dbTechLayerCutSpacingRule::setEolWidthValid(bool eol_width_valid_)
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;

  obj->flags_.eol_width_valid_ = eol_width_valid_;
}

bool dbTechLayerCutSpacingRule::isEolWidthValid() const
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;

  return obj->flags_.eol_width_valid_;
}

void dbTechLayerCutSpacingRule::setMinLengthValid(bool min_length_valid_)
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;

  obj->flags_.min_length_valid_ = min_length_valid_;
}

bool dbTechLayerCutSpacingRule::isMinLengthValid() const
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;

  return obj->flags_.min_length_valid_;
}

void dbTechLayerCutSpacingRule::setAboveWidthValid(bool above_width_valid_)
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;

  obj->flags_.above_width_valid_ = above_width_valid_;
}

bool dbTechLayerCutSpacingRule::isAboveWidthValid() const
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;

  return obj->flags_.above_width_valid_;
}

void dbTechLayerCutSpacingRule::setMaskOverlap(bool mask_overlap_)
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;

  obj->flags_.mask_overlap_ = mask_overlap_;
}

bool dbTechLayerCutSpacingRule::isMaskOverlap() const
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;

  return obj->flags_.mask_overlap_;
}

void dbTechLayerCutSpacingRule::setWrongDirection(bool wrong_direction_)
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;

  obj->flags_.wrong_direction_ = wrong_direction_;
}

bool dbTechLayerCutSpacingRule::isWrongDirection() const
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;

  return obj->flags_.wrong_direction_;
}

void dbTechLayerCutSpacingRule::setAdjacentCuts(uint adjacent_cuts_)
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;

  obj->flags_.adjacent_cuts_ = adjacent_cuts_;
}

uint dbTechLayerCutSpacingRule::getAdjacentCuts() const
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;

  return obj->flags_.adjacent_cuts_;
}

void dbTechLayerCutSpacingRule::setExactAligned(bool exact_aligned_)
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;

  obj->flags_.exact_aligned_ = exact_aligned_;
}

bool dbTechLayerCutSpacingRule::isExactAligned() const
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;

  return obj->flags_.exact_aligned_;
}

void dbTechLayerCutSpacingRule::setCutClassToAll(bool cut_class_to_all_)
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;

  obj->flags_.cut_class_to_all_ = cut_class_to_all_;
}

bool dbTechLayerCutSpacingRule::isCutClassToAll() const
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;

  return obj->flags_.cut_class_to_all_;
}

void dbTechLayerCutSpacingRule::setNoPrl(bool no_prl_)
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;

  obj->flags_.no_prl_ = no_prl_;
}

bool dbTechLayerCutSpacingRule::isNoPrl() const
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;

  return obj->flags_.no_prl_;
}

void dbTechLayerCutSpacingRule::setSameMask(bool same_mask_)
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;

  obj->flags_.same_mask_ = same_mask_;
}

bool dbTechLayerCutSpacingRule::isSameMask() const
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;

  return obj->flags_.same_mask_;
}

void dbTechLayerCutSpacingRule::setExceptSamePgnet(bool except_same_pgnet_)
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;

  obj->flags_.except_same_pgnet_ = except_same_pgnet_;
}

bool dbTechLayerCutSpacingRule::isExceptSamePgnet() const
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;

  return obj->flags_.except_same_pgnet_;
}

void dbTechLayerCutSpacingRule::setSideParallelOverlap(
    bool side_parallel_overlap_)
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;

  obj->flags_.side_parallel_overlap_ = side_parallel_overlap_;
}

bool dbTechLayerCutSpacingRule::isSideParallelOverlap() const
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;

  return obj->flags_.side_parallel_overlap_;
}

void dbTechLayerCutSpacingRule::setExceptSameNet(bool except_same_net_)
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;

  obj->flags_.except_same_net_ = except_same_net_;
}

bool dbTechLayerCutSpacingRule::isExceptSameNet() const
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;

  return obj->flags_.except_same_net_;
}

void dbTechLayerCutSpacingRule::setExceptSameMetal(bool except_same_metal_)
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;

  obj->flags_.except_same_metal_ = except_same_metal_;
}

bool dbTechLayerCutSpacingRule::isExceptSameMetal() const
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;

  return obj->flags_.except_same_metal_;
}

void dbTechLayerCutSpacingRule::setExceptSameMetalOverlap(
    bool except_same_metal_overlap_)
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;

  obj->flags_.except_same_metal_overlap_ = except_same_metal_overlap_;
}

bool dbTechLayerCutSpacingRule::isExceptSameMetalOverlap() const
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;

  return obj->flags_.except_same_metal_overlap_;
}

void dbTechLayerCutSpacingRule::setExceptSameVia(bool except_same_via_)
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;

  obj->flags_.except_same_via_ = except_same_via_;
}

bool dbTechLayerCutSpacingRule::isExceptSameVia() const
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;

  return obj->flags_.except_same_via_;
}

void dbTechLayerCutSpacingRule::setAbove(bool above_)
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;

  obj->flags_.above_ = above_;
}

bool dbTechLayerCutSpacingRule::isAbove() const
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;

  return obj->flags_.above_;
}

void dbTechLayerCutSpacingRule::setExceptTwoEdges(bool except_two_edges_)
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;

  obj->flags_.except_two_edges_ = except_two_edges_;
}

bool dbTechLayerCutSpacingRule::isExceptTwoEdges() const
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;

  return obj->flags_.except_two_edges_;
}

void dbTechLayerCutSpacingRule::setTwoCutsValid(bool two_cuts_valid_)
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;

  obj->flags_.two_cuts_valid_ = two_cuts_valid_;
}

bool dbTechLayerCutSpacingRule::isTwoCutsValid() const
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;

  return obj->flags_.two_cuts_valid_;
}

void dbTechLayerCutSpacingRule::setSameCut(bool same_cut_)
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;

  obj->flags_.same_cut_ = same_cut_;
}

bool dbTechLayerCutSpacingRule::isSameCut() const
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;

  return obj->flags_.same_cut_;
}

void dbTechLayerCutSpacingRule::setLongEdgeOnly(bool long_edge_only_)
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;

  obj->flags_.long_edge_only_ = long_edge_only_;
}

bool dbTechLayerCutSpacingRule::isLongEdgeOnly() const
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;

  return obj->flags_.long_edge_only_;
}

void dbTechLayerCutSpacingRule::setPrlValid(bool prl_valid_)
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;

  obj->flags_.prl_valid_ = prl_valid_;
}

bool dbTechLayerCutSpacingRule::isPrlValid() const
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;

  return obj->flags_.prl_valid_;
}

void dbTechLayerCutSpacingRule::setBelow(bool below_)
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;

  obj->flags_.below_ = below_;
}

bool dbTechLayerCutSpacingRule::isBelow() const
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;

  return obj->flags_.below_;
}

void dbTechLayerCutSpacingRule::setParWithinEnclosureValid(
    bool par_within_enclosure_valid_)
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;

  obj->flags_.par_within_enclosure_valid_ = par_within_enclosure_valid_;
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
  if (obj->cut_class_ == 0)
    return nullptr;
  _dbTechLayer* layer = (_dbTechLayer*) obj->getOwner();
  return (dbTechLayerCutClassRule*) layer->cut_class_rules_tbl_->getPtr(
      obj->cut_class_);
}

dbTechLayer* dbTechLayerCutSpacingRule::getSecondLayer() const
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;
  if (obj->second_layer_ == 0)
    return nullptr;
  _dbTechLayer* layer = (_dbTechLayer*) obj->getOwner();
  _dbTech*      _tech = (_dbTech*) layer->getOwner();
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
  _dbTechLayer*               layer   = (_dbTechLayer*) _layer;
  _dbTechLayerCutSpacingRule* newrule = layer->cut_spacing_rules_tbl_->create();
  return ((dbTechLayerCutSpacingRule*) newrule);
}

dbTechLayerCutSpacingRule*
dbTechLayerCutSpacingRule::getTechLayerCutSpacingRule(dbTechLayer* inly,
                                                      uint         dbid)
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
   // Generator Code End cpp