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

  if (_flags._same_vias != rhs._flags._same_vias)
    return false;

  if (_flags._cut_spacing_type != rhs._flags._cut_spacing_type)
    return false;

  if (_flags._stack != rhs._flags._stack)
    return false;

  if (_flags._adjacent_cuts != rhs._flags._adjacent_cuts)
    return false;

  if (_flags._exact_aligned != rhs._flags._exact_aligned)
    return false;

  if (_flags._except_same_pgnet != rhs._flags._except_same_pgnet)
    return false;

  if (_flags._side_parallel_overlap != rhs._flags._side_parallel_overlap)
    return false;

  if (_flags._except_same_net != rhs._flags._except_same_net)
    return false;

  if (_flags._except_same_metal != rhs._flags._except_same_metal)
    return false;

  if (_flags._except_same_via != rhs._flags._except_same_via)
    return false;

  if (_flags._above != rhs._flags._above)
    return false;

  if (_flags._except_two_edges != rhs._flags._except_two_edges)
    return false;

  if (_cut_spacing != rhs._cut_spacing)
    return false;

  if (_second_layer != rhs._second_layer)
    return false;

  if (_num_cuts != rhs._num_cuts)
    return false;

  if (_within != rhs._within)
    return false;

  if (_cut_class != rhs._cut_class)
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
  DIFF_FIELD(_flags._same_vias);
  DIFF_FIELD(_flags._cut_spacing_type);
  DIFF_FIELD(_flags._stack);
  DIFF_FIELD(_flags._adjacent_cuts);
  DIFF_FIELD(_flags._exact_aligned);
  DIFF_FIELD(_flags._except_same_pgnet);
  DIFF_FIELD(_flags._side_parallel_overlap);
  DIFF_FIELD(_flags._except_same_net);
  DIFF_FIELD(_flags._except_same_metal);
  DIFF_FIELD(_flags._except_same_via);
  DIFF_FIELD(_flags._above);
  DIFF_FIELD(_flags._except_two_edges);
  DIFF_FIELD(_cut_spacing);
  DIFF_FIELD(_second_layer);
  DIFF_FIELD(_num_cuts);
  DIFF_FIELD(_within);
  DIFF_FIELD(_cut_class);
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
  DIFF_OUT_FIELD(_flags._same_vias);
  DIFF_OUT_FIELD(_flags._cut_spacing_type);
  DIFF_OUT_FIELD(_flags._stack);
  DIFF_OUT_FIELD(_flags._adjacent_cuts);
  DIFF_OUT_FIELD(_flags._exact_aligned);
  DIFF_OUT_FIELD(_flags._except_same_pgnet);
  DIFF_OUT_FIELD(_flags._side_parallel_overlap);
  DIFF_OUT_FIELD(_flags._except_same_net);
  DIFF_OUT_FIELD(_flags._except_same_metal);
  DIFF_OUT_FIELD(_flags._except_same_via);
  DIFF_OUT_FIELD(_flags._above);
  DIFF_OUT_FIELD(_flags._except_two_edges);
  DIFF_OUT_FIELD(_cut_spacing);
  DIFF_OUT_FIELD(_second_layer);
  DIFF_OUT_FIELD(_num_cuts);
  DIFF_OUT_FIELD(_within);
  DIFF_OUT_FIELD(_cut_class);
  DIFF_OUT_FIELD(_cut_area);

  // User Code Begin out
  // User Code End out
  DIFF_END
}
_dbTechLayerCutSpacingRule::_dbTechLayerCutSpacingRule(_dbDatabase* db)
{
  uint* _flags_bit_field = (uint*) &_flags;
  *_flags_bit_field      = 0;
  // User Code Begin constructor
  // User Code End constructor
}
_dbTechLayerCutSpacingRule::_dbTechLayerCutSpacingRule(
    _dbDatabase*                      db,
    const _dbTechLayerCutSpacingRule& r)
{
  _flags._center_to_center      = r._flags._center_to_center;
  _flags._same_net              = r._flags._same_net;
  _flags._same_metal            = r._flags._same_metal;
  _flags._same_vias             = r._flags._same_vias;
  _flags._cut_spacing_type      = r._flags._cut_spacing_type;
  _flags._stack                 = r._flags._stack;
  _flags._adjacent_cuts         = r._flags._adjacent_cuts;
  _flags._exact_aligned         = r._flags._exact_aligned;
  _flags._except_same_pgnet     = r._flags._except_same_pgnet;
  _flags._side_parallel_overlap = r._flags._side_parallel_overlap;
  _flags._except_same_net       = r._flags._except_same_net;
  _flags._except_same_metal     = r._flags._except_same_metal;
  _flags._except_same_via       = r._flags._except_same_via;
  _flags._above                 = r._flags._above;
  _flags._except_two_edges      = r._flags._except_two_edges;
  _flags._spare_bits            = r._flags._spare_bits;
  _cut_spacing                  = r._cut_spacing;
  _second_layer                 = r._second_layer;
  _num_cuts                     = r._num_cuts;
  _within                       = r._within;
  _cut_class                    = r._cut_class;
  _cut_area                     = r._cut_area;
  // User Code Begin CopyConstructor
  // User Code End CopyConstructor
}

dbIStream& operator>>(dbIStream& stream, _dbTechLayerCutSpacingRule& obj)
{
  uint* _flags_bit_field = (uint*) &obj._flags;
  stream >> *_flags_bit_field;
  stream >> obj._cut_spacing;
  stream >> obj._second_layer;
  stream >> obj._num_cuts;
  stream >> obj._within;
  stream >> obj._cut_class;
  stream >> obj._cut_area;
  // User Code Begin >>
  // User Code End >>
  return stream;
}
dbOStream& operator<<(dbOStream& stream, const _dbTechLayerCutSpacingRule& obj)
{
  uint* _flags_bit_field = (uint*) &obj._flags;
  stream << *_flags_bit_field;
  stream << obj._cut_spacing;
  stream << obj._second_layer;
  stream << obj._num_cuts;
  stream << obj._within;
  stream << obj._cut_class;
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

void dbTechLayerCutSpacingRule::setCutClass(dbTechLayerCutClassRule* _cut_class)
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;

  obj->_cut_class = _cut_class->getImpl()->getOID();
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

void dbTechLayerCutSpacingRule::setSameVias(bool _same_vias)
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;

  obj->_flags._same_vias = _same_vias;
}

bool dbTechLayerCutSpacingRule::isSameVias() const
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;

  return obj->_flags._same_vias;
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