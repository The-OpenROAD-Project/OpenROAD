///////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// Copyright (c) 2022, The Regents of the University of California
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
#include "dbGDSBox.h"

#include "dbDatabase.h"
#include "dbDiff.hpp"
#include "dbTable.h"
#include "dbTable.hpp"
#include "odb/db.h"
#include "odb/dbTypes.h"
// User Code Begin Includes
// User Code End Includes
namespace odb {
template class dbTable<_dbGDSBox>;

bool _dbGDSBox::operator==(const _dbGDSBox& rhs) const
{
  if (_boxType != rhs._boxType) {
    return false;
  }

  // User Code Begin ==
  // User Code End ==
  return true;
}

bool _dbGDSBox::operator<(const _dbGDSBox& rhs) const
{
  // User Code Begin <
  // User Code End <
  return true;
}

void _dbGDSBox::differences(dbDiff& diff,
                            const char* field,
                            const _dbGDSBox& rhs) const
{
  DIFF_BEGIN
  DIFF_FIELD(_boxType);
  // User Code Begin Differences
  // User Code End Differences
  DIFF_END
}

void _dbGDSBox::out(dbDiff& diff, char side, const char* field) const
{
  DIFF_OUT_BEGIN
  DIFF_OUT_FIELD(_boxType);

  // User Code Begin Out
  // User Code End Out
  DIFF_END
}

_dbGDSBox::_dbGDSBox(_dbDatabase* db)
{
  // User Code Begin Constructor
  // User Code End Constructor
}

_dbGDSBox::_dbGDSBox(_dbDatabase* db, const _dbGDSBox& r)
{
  _boxType = r._boxType;
  // User Code Begin CopyConstructor
  // User Code End CopyConstructor
}

dbIStream& operator>>(dbIStream& stream, _dbGDSBox& obj)
{
  stream >> obj._boxType;
  // User Code Begin >>
  // User Code End >>
  return stream;
}

dbOStream& operator<<(dbOStream& stream, const _dbGDSBox& obj)
{
  stream << obj._boxType;
  // User Code Begin <<
  // User Code End <<
  return stream;
}

// User Code Begin PrivateMethods
// User Code End PrivateMethods

////////////////////////////////////////////////////////////////////
//
// dbGDSBox - Methods
//
////////////////////////////////////////////////////////////////////

void dbGDSBox::set_boxType(int16_t boxType)
{
  _dbGDSBox* obj = (_dbGDSBox*) this;

  obj->_boxType = boxType;
}

int16_t dbGDSBox::get_boxType() const
{
  _dbGDSBox* obj = (_dbGDSBox*) this;
  return obj->_boxType;
}

// User Code Begin dbGDSBoxPublicMethods
// User Code End dbGDSBoxPublicMethods
}  // namespace odb
   // Generator Code End Cpp