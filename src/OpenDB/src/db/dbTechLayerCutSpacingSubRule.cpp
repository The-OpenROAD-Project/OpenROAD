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
#include "dbTechLayerCutSpacingSubRule.h"

#include "db.h"
#include "dbDatabase.h"
#include "dbDiff.hpp"
#include "dbTable.h"
#include "dbTable.hpp"
#include "dbTechLayerCutSpacingRule.h"
// User Code Begin includes
// User Code End includes
namespace odb {

// User Code Begin definitions
// User Code End definitions
template class dbTable<_dbTechLayerCutSpacingSubRule>;

bool _dbTechLayerCutSpacingSubRule::operator==(
    const _dbTechLayerCutSpacingSubRule& rhs) const
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

  if (_flags._cut_class_valid != rhs._flags._cut_class_valid)
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

  if (_second_layer_name != rhs._second_layer_name)
    return false;

  if (_num_cuts != rhs._num_cuts)
    return false;

  if (_within != rhs._within)
    return false;

  if (_cut_class_name != rhs._cut_class_name)
    return false;

  if (_cut_area != rhs._cut_area)
    return false;

  // User Code Begin ==
  // User Code End ==
  return true;
}
bool _dbTechLayerCutSpacingSubRule::operator<(
    const _dbTechLayerCutSpacingSubRule& rhs) const
{
  // User Code Begin <
  // User Code End <
  return true;
}
void _dbTechLayerCutSpacingSubRule::differences(
    dbDiff&                              diff,
    const char*                          field,
    const _dbTechLayerCutSpacingSubRule& rhs) const
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
  DIFF_FIELD(_flags._cut_class_valid);
  DIFF_FIELD(_flags._side_parallel_overlap);
  DIFF_FIELD(_flags._except_same_net);
  DIFF_FIELD(_flags._except_same_metal);
  DIFF_FIELD(_flags._except_same_via);
  DIFF_FIELD(_flags._above);
  DIFF_FIELD(_flags._except_two_edges);
  DIFF_FIELD(_cut_spacing);
  DIFF_FIELD(_second_layer_name);
  DIFF_FIELD(_num_cuts);
  DIFF_FIELD(_within);
  DIFF_FIELD(_cut_class_name);
  DIFF_FIELD(_cut_area);
  // User Code Begin differences
  // User Code End differences
  DIFF_END
}
void _dbTechLayerCutSpacingSubRule::out(dbDiff&     diff,
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
  DIFF_OUT_FIELD(_flags._cut_class_valid);
  DIFF_OUT_FIELD(_flags._side_parallel_overlap);
  DIFF_OUT_FIELD(_flags._except_same_net);
  DIFF_OUT_FIELD(_flags._except_same_metal);
  DIFF_OUT_FIELD(_flags._except_same_via);
  DIFF_OUT_FIELD(_flags._above);
  DIFF_OUT_FIELD(_flags._except_two_edges);
  DIFF_OUT_FIELD(_cut_spacing);
  DIFF_OUT_FIELD(_second_layer_name);
  DIFF_OUT_FIELD(_num_cuts);
  DIFF_OUT_FIELD(_within);
  DIFF_OUT_FIELD(_cut_class_name);
  DIFF_OUT_FIELD(_cut_area);

  // User Code Begin out
  // User Code End out
  DIFF_END
}
_dbTechLayerCutSpacingSubRule::_dbTechLayerCutSpacingSubRule(_dbDatabase* db)
{
  uint* _flags_bit_field = (uint*) &_flags;
  *_flags_bit_field      = 0;
  // User Code Begin constructor
  // User Code End constructor
}
_dbTechLayerCutSpacingSubRule::_dbTechLayerCutSpacingSubRule(
    _dbDatabase*                         db,
    const _dbTechLayerCutSpacingSubRule& r)
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
  _flags._cut_class_valid       = r._flags._cut_class_valid;
  _flags._side_parallel_overlap = r._flags._side_parallel_overlap;
  _flags._except_same_net       = r._flags._except_same_net;
  _flags._except_same_metal     = r._flags._except_same_metal;
  _flags._except_same_via       = r._flags._except_same_via;
  _flags._above                 = r._flags._above;
  _flags._except_two_edges      = r._flags._except_two_edges;
  _flags._spare_bits            = r._flags._spare_bits;
  _cut_spacing                  = r._cut_spacing;
  _second_layer_name            = r._second_layer_name;
  _num_cuts                     = r._num_cuts;
  _within                       = r._within;
  _cut_class_name               = r._cut_class_name;
  _cut_area                     = r._cut_area;
  // User Code Begin CopyConstructor
  // User Code End CopyConstructor
}

dbIStream& operator>>(dbIStream& stream, _dbTechLayerCutSpacingSubRule& obj)
{
  uint* _flags_bit_field = (uint*) &obj._flags;
  stream >> *_flags_bit_field;
  stream >> obj._cut_spacing;
  stream >> obj._second_layer_name;
  stream >> obj._num_cuts;
  stream >> obj._within;
  stream >> obj._cut_class_name;
  stream >> obj._cut_area;
  // User Code Begin >>
  // User Code End >>
  return stream;
}
dbOStream& operator<<(dbOStream&                           stream,
                      const _dbTechLayerCutSpacingSubRule& obj)
{
  uint* _flags_bit_field = (uint*) &obj._flags;
  stream << *_flags_bit_field;
  stream << obj._cut_spacing;
  stream << obj._second_layer_name;
  stream << obj._num_cuts;
  stream << obj._within;
  stream << obj._cut_class_name;
  stream << obj._cut_area;
  // User Code Begin <<
  // User Code End <<
  return stream;
}

_dbTechLayerCutSpacingSubRule::~_dbTechLayerCutSpacingSubRule()
{
  // User Code Begin Destructor
  // User Code End Destructor
}
////////////////////////////////////////////////////////////////////
//
// dbTechLayerCutSpacingSubRule - Methods
//
////////////////////////////////////////////////////////////////////

void dbTechLayerCutSpacingSubRule::setCutSpacing(int _cut_spacing)
{
  _dbTechLayerCutSpacingSubRule* obj = (_dbTechLayerCutSpacingSubRule*) this;

  obj->_cut_spacing = _cut_spacing;
}

int dbTechLayerCutSpacingSubRule::getCutSpacing() const
{
  _dbTechLayerCutSpacingSubRule* obj = (_dbTechLayerCutSpacingSubRule*) this;
  return obj->_cut_spacing;
}

char* dbTechLayerCutSpacingSubRule::getSecondLayerName() const
{
  _dbTechLayerCutSpacingSubRule* obj = (_dbTechLayerCutSpacingSubRule*) this;
  return obj->_second_layer_name;
}

void dbTechLayerCutSpacingSubRule::setNumCuts(uint _num_cuts)
{
  _dbTechLayerCutSpacingSubRule* obj = (_dbTechLayerCutSpacingSubRule*) this;

  obj->_num_cuts = _num_cuts;
}

uint dbTechLayerCutSpacingSubRule::getNumCuts() const
{
  _dbTechLayerCutSpacingSubRule* obj = (_dbTechLayerCutSpacingSubRule*) this;
  return obj->_num_cuts;
}

void dbTechLayerCutSpacingSubRule::setWithin(int _within)
{
  _dbTechLayerCutSpacingSubRule* obj = (_dbTechLayerCutSpacingSubRule*) this;

  obj->_within = _within;
}

int dbTechLayerCutSpacingSubRule::getWithin() const
{
  _dbTechLayerCutSpacingSubRule* obj = (_dbTechLayerCutSpacingSubRule*) this;
  return obj->_within;
}

void dbTechLayerCutSpacingSubRule::setCutClassName(char* _cut_class_name)
{
  _dbTechLayerCutSpacingSubRule* obj = (_dbTechLayerCutSpacingSubRule*) this;

  obj->_cut_class_name = _cut_class_name;
}

char* dbTechLayerCutSpacingSubRule::getCutClassName() const
{
  _dbTechLayerCutSpacingSubRule* obj = (_dbTechLayerCutSpacingSubRule*) this;
  return obj->_cut_class_name;
}

void dbTechLayerCutSpacingSubRule::setCutArea(int _cut_area)
{
  _dbTechLayerCutSpacingSubRule* obj = (_dbTechLayerCutSpacingSubRule*) this;

  obj->_cut_area = _cut_area;
}

int dbTechLayerCutSpacingSubRule::getCutArea() const
{
  _dbTechLayerCutSpacingSubRule* obj = (_dbTechLayerCutSpacingSubRule*) this;
  return obj->_cut_area;
}

void dbTechLayerCutSpacingSubRule::setCenterToCenter(bool _center_to_center)
{
  _dbTechLayerCutSpacingSubRule* obj = (_dbTechLayerCutSpacingSubRule*) this;

  obj->_flags._center_to_center = _center_to_center;
}

bool dbTechLayerCutSpacingSubRule::isCenterToCenter() const
{
  _dbTechLayerCutSpacingSubRule* obj = (_dbTechLayerCutSpacingSubRule*) this;

  return obj->_flags._center_to_center;
}

void dbTechLayerCutSpacingSubRule::setSameNet(bool _same_net)
{
  _dbTechLayerCutSpacingSubRule* obj = (_dbTechLayerCutSpacingSubRule*) this;

  obj->_flags._same_net = _same_net;
}

bool dbTechLayerCutSpacingSubRule::isSameNet() const
{
  _dbTechLayerCutSpacingSubRule* obj = (_dbTechLayerCutSpacingSubRule*) this;

  return obj->_flags._same_net;
}

void dbTechLayerCutSpacingSubRule::setSameMetal(bool _same_metal)
{
  _dbTechLayerCutSpacingSubRule* obj = (_dbTechLayerCutSpacingSubRule*) this;

  obj->_flags._same_metal = _same_metal;
}

bool dbTechLayerCutSpacingSubRule::isSameMetal() const
{
  _dbTechLayerCutSpacingSubRule* obj = (_dbTechLayerCutSpacingSubRule*) this;

  return obj->_flags._same_metal;
}

void dbTechLayerCutSpacingSubRule::setSameVias(bool _same_vias)
{
  _dbTechLayerCutSpacingSubRule* obj = (_dbTechLayerCutSpacingSubRule*) this;

  obj->_flags._same_vias = _same_vias;
}

bool dbTechLayerCutSpacingSubRule::isSameVias() const
{
  _dbTechLayerCutSpacingSubRule* obj = (_dbTechLayerCutSpacingSubRule*) this;

  return obj->_flags._same_vias;
}

void dbTechLayerCutSpacingSubRule::setStack(bool _stack)
{
  _dbTechLayerCutSpacingSubRule* obj = (_dbTechLayerCutSpacingSubRule*) this;

  obj->_flags._stack = _stack;
}

bool dbTechLayerCutSpacingSubRule::isStack() const
{
  _dbTechLayerCutSpacingSubRule* obj = (_dbTechLayerCutSpacingSubRule*) this;

  return obj->_flags._stack;
}

void dbTechLayerCutSpacingSubRule::setAdjacentCuts(uint _adjacent_cuts)
{
  _dbTechLayerCutSpacingSubRule* obj = (_dbTechLayerCutSpacingSubRule*) this;

  obj->_flags._adjacent_cuts = _adjacent_cuts;
}

uint dbTechLayerCutSpacingSubRule::getAdjacentCuts() const
{
  _dbTechLayerCutSpacingSubRule* obj = (_dbTechLayerCutSpacingSubRule*) this;

  return obj->_flags._adjacent_cuts;
}

void dbTechLayerCutSpacingSubRule::setExactAligned(bool _exact_aligned)
{
  _dbTechLayerCutSpacingSubRule* obj = (_dbTechLayerCutSpacingSubRule*) this;

  obj->_flags._exact_aligned = _exact_aligned;
}

bool dbTechLayerCutSpacingSubRule::isExactAligned() const
{
  _dbTechLayerCutSpacingSubRule* obj = (_dbTechLayerCutSpacingSubRule*) this;

  return obj->_flags._exact_aligned;
}

void dbTechLayerCutSpacingSubRule::setExceptSamePgnet(bool _except_same_pgnet)
{
  _dbTechLayerCutSpacingSubRule* obj = (_dbTechLayerCutSpacingSubRule*) this;

  obj->_flags._except_same_pgnet = _except_same_pgnet;
}

bool dbTechLayerCutSpacingSubRule::isExceptSamePgnet() const
{
  _dbTechLayerCutSpacingSubRule* obj = (_dbTechLayerCutSpacingSubRule*) this;

  return obj->_flags._except_same_pgnet;
}

void dbTechLayerCutSpacingSubRule::setCutClassValid(bool _cut_class_valid)
{
  _dbTechLayerCutSpacingSubRule* obj = (_dbTechLayerCutSpacingSubRule*) this;

  obj->_flags._cut_class_valid = _cut_class_valid;
}

bool dbTechLayerCutSpacingSubRule::isCutClassValid() const
{
  _dbTechLayerCutSpacingSubRule* obj = (_dbTechLayerCutSpacingSubRule*) this;

  return obj->_flags._cut_class_valid;
}

void dbTechLayerCutSpacingSubRule::setSideParallelOverlap(
    bool _side_parallel_overlap)
{
  _dbTechLayerCutSpacingSubRule* obj = (_dbTechLayerCutSpacingSubRule*) this;

  obj->_flags._side_parallel_overlap = _side_parallel_overlap;
}

bool dbTechLayerCutSpacingSubRule::isSideParallelOverlap() const
{
  _dbTechLayerCutSpacingSubRule* obj = (_dbTechLayerCutSpacingSubRule*) this;

  return obj->_flags._side_parallel_overlap;
}

void dbTechLayerCutSpacingSubRule::setExceptSameNet(bool _except_same_net)
{
  _dbTechLayerCutSpacingSubRule* obj = (_dbTechLayerCutSpacingSubRule*) this;

  obj->_flags._except_same_net = _except_same_net;
}

bool dbTechLayerCutSpacingSubRule::isExceptSameNet() const
{
  _dbTechLayerCutSpacingSubRule* obj = (_dbTechLayerCutSpacingSubRule*) this;

  return obj->_flags._except_same_net;
}

void dbTechLayerCutSpacingSubRule::setExceptSameMetal(bool _except_same_metal)
{
  _dbTechLayerCutSpacingSubRule* obj = (_dbTechLayerCutSpacingSubRule*) this;

  obj->_flags._except_same_metal = _except_same_metal;
}

bool dbTechLayerCutSpacingSubRule::isExceptSameMetal() const
{
  _dbTechLayerCutSpacingSubRule* obj = (_dbTechLayerCutSpacingSubRule*) this;

  return obj->_flags._except_same_metal;
}

void dbTechLayerCutSpacingSubRule::setExceptSameVia(bool _except_same_via)
{
  _dbTechLayerCutSpacingSubRule* obj = (_dbTechLayerCutSpacingSubRule*) this;

  obj->_flags._except_same_via = _except_same_via;
}

bool dbTechLayerCutSpacingSubRule::isExceptSameVia() const
{
  _dbTechLayerCutSpacingSubRule* obj = (_dbTechLayerCutSpacingSubRule*) this;

  return obj->_flags._except_same_via;
}

void dbTechLayerCutSpacingSubRule::setAbove(bool _above)
{
  _dbTechLayerCutSpacingSubRule* obj = (_dbTechLayerCutSpacingSubRule*) this;

  obj->_flags._above = _above;
}

bool dbTechLayerCutSpacingSubRule::isAbove() const
{
  _dbTechLayerCutSpacingSubRule* obj = (_dbTechLayerCutSpacingSubRule*) this;

  return obj->_flags._above;
}

void dbTechLayerCutSpacingSubRule::setExceptTwoEdges(bool _except_two_edges)
{
  _dbTechLayerCutSpacingSubRule* obj = (_dbTechLayerCutSpacingSubRule*) this;

  obj->_flags._except_two_edges = _except_two_edges;
}

bool dbTechLayerCutSpacingSubRule::isExceptTwoEdges() const
{
  _dbTechLayerCutSpacingSubRule* obj = (_dbTechLayerCutSpacingSubRule*) this;

  return obj->_flags._except_two_edges;
}

// User Code Begin dbTechLayerCutSpacingSubRulePublicMethods
void dbTechLayerCutSpacingSubRule::setCutClassName(const char* name)
{
  _dbTechLayerCutSpacingSubRule* obj = (_dbTechLayerCutSpacingSubRule*) this;
  obj->_cut_class_name               = strdup(name);
}

void dbTechLayerCutSpacingSubRule::setSecondLayerName(const char* name)
{
  _dbTechLayerCutSpacingSubRule* obj = (_dbTechLayerCutSpacingSubRule*) this;
  obj->_second_layer_name            = strdup(name);
}

void dbTechLayerCutSpacingSubRule::setType(CutSpacingType _type)
{
  _dbTechLayerCutSpacingSubRule* obj = (_dbTechLayerCutSpacingSubRule*) this;

  obj->_flags._cut_spacing_type = (uint) _type;
}

dbTechLayerCutSpacingSubRule::CutSpacingType
dbTechLayerCutSpacingSubRule::getType() const
{
  _dbTechLayerCutSpacingSubRule* obj = (_dbTechLayerCutSpacingSubRule*) this;

  return (dbTechLayerCutSpacingSubRule::CutSpacingType)
      obj->_flags._cut_spacing_type;
}

dbTechLayerCutSpacingSubRule* dbTechLayerCutSpacingSubRule::create(
    dbTechLayerCutSpacingRule* parent)
{
  _dbTechLayerCutSpacingRule*    _parent = (_dbTechLayerCutSpacingRule*) parent;
  _dbTechLayerCutSpacingSubRule* newrule
      = _parent->_techlayercutspacingsubrule_tbl->create();
  return ((dbTechLayerCutSpacingSubRule*) newrule);
}

dbTechLayerCutSpacingSubRule*
dbTechLayerCutSpacingSubRule::getTechLayerCutSpacingSubRule(
    dbTechLayerCutSpacingRule* parent,
    uint                       dbid)
{
  _dbTechLayerCutSpacingRule* _parent = (_dbTechLayerCutSpacingRule*) parent;
  return (dbTechLayerCutSpacingSubRule*)
      _parent->_techlayercutspacingsubrule_tbl->getPtr(dbid);
}
void dbTechLayerCutSpacingSubRule::destroy(dbTechLayerCutSpacingSubRule* rule)
{
  _dbTechLayerCutSpacingRule* _parent
      = (_dbTechLayerCutSpacingRule*) rule->getImpl()->getOwner();
  dbProperty::destroyProperties(rule);
  _parent->_techlayercutspacingsubrule_tbl->destroy(
      (_dbTechLayerCutSpacingSubRule*) rule);
}

// User Code End dbTechLayerCutSpacingSubRulePublicMethods
}  // namespace odb
   // Generator Code End 1