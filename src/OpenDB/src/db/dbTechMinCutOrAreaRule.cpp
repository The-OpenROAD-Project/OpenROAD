///////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// Copyright (c) 2019, Nefelus Inc
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

#include "dbTechMinCutOrAreaRule.h"

#include "db.h"
#include "dbDatabase.h"
#include "dbTable.h"
#include "dbTable.hpp"
#include "dbTech.h"
#include "dbTechLayer.h"
#include "lefout.h"

namespace odb {

template class dbTable<_dbTechMinCutRule>;
template class dbTable<_dbTechMinEncRule>;

bool _dbTechMinCutRule::operator==(const _dbTechMinCutRule& rhs) const
{
  if (_flags._rule != rhs._flags._rule)
    return false;

  if (_flags._cuts_length != rhs._flags._cuts_length)
    return false;

  if (_num_cuts != rhs._num_cuts)
    return false;

  if (_width != rhs._width)
    return false;

  if (_cut_distance != rhs._cut_distance)
    return false;

  if (_length != rhs._length)
    return false;

  if (_distance != rhs._distance)
    return false;

  return true;
}

void _dbTechMinCutRule::differences(dbDiff&                  diff,
                                    const char*              field,
                                    const _dbTechMinCutRule& rhs) const
{
  DIFF_BEGIN
  DIFF_FIELD(_flags._rule);
  DIFF_FIELD(_flags._cuts_length);
  DIFF_FIELD(_num_cuts);
  DIFF_FIELD(_width);
  DIFF_FIELD(_cut_distance);
  DIFF_FIELD(_length);
  DIFF_FIELD(_distance);
  DIFF_END
}

void _dbTechMinCutRule::out(dbDiff& diff, char side, const char* field) const
{
  DIFF_OUT_BEGIN
  DIFF_OUT_FIELD(_flags._rule);
  DIFF_OUT_FIELD(_flags._cuts_length);
  DIFF_OUT_FIELD(_num_cuts);
  DIFF_OUT_FIELD(_width);
  DIFF_OUT_FIELD(_cut_distance);
  DIFF_OUT_FIELD(_length);
  DIFF_OUT_FIELD(_distance);
  DIFF_END
}

bool _dbTechMinEncRule::operator==(const _dbTechMinEncRule& rhs) const
{
  if (_flags._has_width != rhs._flags._has_width)
    return false;

  if (_area != rhs._area)
    return false;

  if (_width != rhs._width)
    return false;

  return true;
}

void _dbTechMinEncRule::differences(dbDiff&                  diff,
                                    const char*              field,
                                    const _dbTechMinEncRule& rhs) const
{
  DIFF_BEGIN
  DIFF_FIELD(_flags._has_width);
  DIFF_FIELD(_area);
  DIFF_FIELD(_width);
  DIFF_END
}

void _dbTechMinEncRule::out(dbDiff& diff, char side, const char* field) const
{
  DIFF_BEGIN
  DIFF_OUT_FIELD(_flags._has_width);
  DIFF_OUT_FIELD(_area);
  DIFF_OUT_FIELD(_width);
  DIFF_END
}

////////////////////////////////////////////////////////////////////
//
// _dbTechMinCutRule - Methods
//
////////////////////////////////////////////////////////////////////

dbOStream& operator<<(dbOStream& stream, const _dbTechMinCutRule& rule)
{
  uint* bit_field = (uint*) &rule._flags;
  stream << *bit_field;
  stream << rule._num_cuts;
  stream << rule._width;
  stream << rule._cut_distance;
  stream << rule._length;
  stream << rule._distance;
  return stream;
}

dbIStream& operator>>(dbIStream& stream, _dbTechMinCutRule& rule)
{
  uint* bit_field = (uint*) &rule._flags;
  stream >> *bit_field;
  stream >> rule._num_cuts;
  stream >> rule._width;
  stream >> rule._cut_distance;
  stream >> rule._length;
  stream >> rule._distance;
  return stream;
}

////////////////////////////////////////////////////////////////////
//
// _dbTechMinEncRule - Methods
//
////////////////////////////////////////////////////////////////////

dbOStream& operator<<(dbOStream& stream, const _dbTechMinEncRule& rule)
{
  uint* bit_field = (uint*) &rule._flags;
  stream << *bit_field;
  stream << rule._area;
  stream << rule._width;
  return stream;
}

dbIStream& operator>>(dbIStream& stream, _dbTechMinEncRule& rule)
{
  uint* bit_field = (uint*) &rule._flags;
  stream >> *bit_field;
  stream >> rule._area;
  stream >> rule._width;
  return stream;
}

////////////////////////////////////////////////////////////////////
//
// dbTechMinCutRule - Methods
//
////////////////////////////////////////////////////////////////////

using namespace TechMinCutRule;

bool dbTechMinCutRule::getMinimumCuts(uint& numcuts, uint& width) const
{
  _dbTechMinCutRule* _lsm = (_dbTechMinCutRule*) this;

  if (_lsm->_flags._rule == NONE)
    return false;

  numcuts = _lsm->_num_cuts;
  width   = _lsm->_width;
  return true;
}

void dbTechMinCutRule::setMinimumCuts(uint numcuts,
                                      uint width,
                                      bool above_only,
                                      bool below_only)
{
  _dbTechMinCutRule* _lsm = (_dbTechMinCutRule*) this;

  _lsm->_num_cuts = numcuts;
  _lsm->_width    = width;

  if (above_only && below_only) {  // For default encoding, rule applies from
                                   // both above and below
    above_only = false;
    below_only = false;
  }

  if (above_only)
    _lsm->_flags._rule = MINIMUM_CUT_ABOVE;
  else if (below_only)
    _lsm->_flags._rule = MINIMUM_CUT_BELOW;
  else
    _lsm->_flags._rule = MINIMUM_CUT;
}

bool dbTechMinCutRule::isAboveOnly() const
{
  _dbTechMinCutRule* _lsm = (_dbTechMinCutRule*) this;
  return (_lsm->_flags._rule == MINIMUM_CUT_ABOVE);
}

bool dbTechMinCutRule::isBelowOnly() const
{
  _dbTechMinCutRule* _lsm = (_dbTechMinCutRule*) this;
  return (_lsm->_flags._rule == MINIMUM_CUT_BELOW);
}

bool dbTechMinCutRule::getLengthForCuts(uint& length, uint& distance) const
{
  _dbTechMinCutRule* _lsm = (_dbTechMinCutRule*) this;

  if ((_lsm->_flags._rule == NONE) || !(_lsm->_flags._cuts_length))
    return false;

  length   = _lsm->_length;
  distance = _lsm->_distance;
  return true;
}

bool dbTechMinCutRule::getCutDistance(uint& cut_distance) const
{
  _dbTechMinCutRule* _lsm = (_dbTechMinCutRule*) this;
  if (_lsm->_cut_distance < 0) {
    return false;
  }

  cut_distance = _lsm->_cut_distance;
  return true;
}

void dbTechMinCutRule::setCutDistance(uint cut_distance)
{
  _dbTechMinCutRule* _lsm = (_dbTechMinCutRule*) this;
  _lsm->_cut_distance     = cut_distance;
}

//
//  NOTE: Assumes that the rule type has already been set.
//
void dbTechMinCutRule::setLengthForCuts(uint length, uint distance)
{
  _dbTechMinCutRule* _lsm = (_dbTechMinCutRule*) this;

  assert((_lsm->_flags._rule == MINIMUM_CUT)
         || (_lsm->_flags._rule == MINIMUM_CUT_ABOVE)
         || (_lsm->_flags._rule == MINIMUM_CUT_BELOW));

  _lsm->_flags._cuts_length = 1;
  _lsm->_length             = length;
  _lsm->_distance           = distance;
}

void dbTechMinCutRule::writeLef(lefout& writer) const
{
  uint numcuts   = 0;
  uint cut_width = 0;
  getMinimumCuts(numcuts, cut_width);
  fprintf(writer.out(),
          "    MINIMUMCUT %d  WIDTH %g ",
          numcuts,
          writer.lefdist(cut_width));

  uint cut_distance;
  if (getCutDistance(cut_distance)) {
    fprintf(writer.out(), "WITHIN %g ", writer.lefdist(cut_distance));
  }

  if (isAboveOnly())
    fprintf(writer.out(), "FROMABOVE ");
  else if (isBelowOnly())
    fprintf(writer.out(), "FROMBELOW ");

  uint length, distance;
  if (getLengthForCuts(length, distance))
    fprintf(writer.out(),
            "LENGTH %g  WITHIN %g ",
            writer.lefdist(length),
            writer.lefdist(distance));
  fprintf(writer.out(), ";\n");
}

dbTechMinCutRule* dbTechMinCutRule::create(dbTechLayer* inly)
{
  _dbTechLayer*      layer   = (_dbTechLayer*) inly;
  _dbTechMinCutRule* newrule = layer->_min_cut_rules_tbl->create();
  return ((dbTechMinCutRule*) newrule);
}

dbTechMinCutRule* dbTechMinCutRule::getMinCutRule(dbTechLayer* inly, uint dbid)
{
  _dbTechLayer* layer = (_dbTechLayer*) inly;
  return (dbTechMinCutRule*) layer->_min_cut_rules_tbl->getPtr(dbid);
}

////////////////////////////////////////////////////////////////////
//
// dbTechMinEncRule - Methods
//
////////////////////////////////////////////////////////////////////

bool dbTechMinEncRule::getEnclosure(uint& area) const
{
  _dbTechMinEncRule* _lsm = (_dbTechMinEncRule*) this;

  area = _lsm->_area;
  return true;
}

void dbTechMinEncRule::setEnclosure(uint area)
{
  _dbTechMinEncRule* _lsm = (_dbTechMinEncRule*) this;

  _lsm->_area = area;
}

bool dbTechMinEncRule::getEnclosureWidth(uint& width) const
{
  _dbTechMinEncRule* _lsm = (_dbTechMinEncRule*) this;

  if (!(_lsm->_flags._has_width))
    return false;

  width = _lsm->_width;
  return true;
}

void dbTechMinEncRule::setEnclosureWidth(uint width)
{
  _dbTechMinEncRule* _lsm = (_dbTechMinEncRule*) this;

  _lsm->_flags._has_width = 1;
  _lsm->_width            = width;
}

void dbTechMinEncRule::writeLef(lefout& writer) const
{
  uint enc_area, enc_width;
  getEnclosure(enc_area);
  fprintf(writer.out(), "    MINENCLOSEDAREA %g ", writer.lefarea(enc_area));
  if (getEnclosureWidth(enc_width))
    fprintf(writer.out(), "WIDTH %g ", writer.lefdist(enc_width));
  fprintf(writer.out(), ";\n");
}

dbTechMinEncRule* dbTechMinEncRule::create(dbTechLayer* inly)
{
  _dbTechLayer*      layer   = (_dbTechLayer*) inly;
  _dbTechMinEncRule* newrule = layer->_min_enc_rules_tbl->create();
  return ((dbTechMinEncRule*) newrule);
}

dbTechMinEncRule* dbTechMinEncRule::getMinEncRule(dbTechLayer* inly, uint dbid)
{
  _dbTechLayer* layer = (_dbTechLayer*) inly;
  return (dbTechMinEncRule*) layer->_min_enc_rules_tbl->getPtr(dbid);
}

}  // namespace odb
