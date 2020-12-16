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

#include "dbExtControl.h"

#include "db.h"
#include "dbDatabase.h"

namespace odb {

dbExtControl::dbExtControl()
{
  _independentExtCorners = false;
  _wireStamped           = false;
  _foreign               = false;
  _rsegCoord             = false;
  _overCell              = false;
  _extracted             = false;
  _lefRC                 = false;
  _cornerCnt             = 0;
  _ccPreseveGeom         = 0;
  _ccUp                  = 0;
  _couplingFlag          = 3;
  _coupleThreshold       = 0.1;   // fF
  _mergeResBound         = 50.0;  // OHMS
  _mergeViaRes           = true;
  _mergeParallelCC       = true;
  _exttreePreMerg        = true;
  _exttreeMaxcap         = 0.0;
  _useDbSdb              = true;
  _CCnoPowerSource       = 0;
  _CCnoPowerTarget       = 0;
  _usingMetalPlanes      = 0;
  _ruleFileName          = (char*) 0;
  _extractedCornerList   = (char*) 0;
  _derivedCornerList     = (char*) 0;
  _cornerIndexList       = (char*) 0;
  _resFactorList         = (char*) 0;
  _gndcFactorList        = (char*) 0;
  _ccFactorList          = (char*) 0;
}
dbExtControl::~dbExtControl()
{
  if (_ruleFileName)
    free(_ruleFileName);

  if (_extractedCornerList)
    free(_extractedCornerList);
  if (_derivedCornerList)
    free(_derivedCornerList);
  if (_cornerIndexList)
    free(_cornerIndexList);
  if (_resFactorList)
    free(_resFactorList);
  if (_gndcFactorList)
    free(_gndcFactorList);
  if (_ccFactorList)
    free(_ccFactorList);
}

dbOStream& operator<<(dbOStream& stream, const dbExtControl& extControl)
{
  stream << extControl._overCell;
  stream << extControl._extracted;
  if (!extControl._extracted)
    return stream;
  stream << extControl._independentExtCorners;
  stream << extControl._wireStamped;
  stream << extControl._foreign;
  stream << extControl._rsegCoord;
  stream << extControl._lefRC;
  stream << extControl._cornerCnt;
  stream << extControl._ccPreseveGeom;
  stream << extControl._ccUp;
  stream << extControl._couplingFlag;
  stream << extControl._coupleThreshold;
  stream << extControl._mergeResBound;
  stream << extControl._mergeViaRes;
  stream << extControl._mergeParallelCC;
  stream << extControl._exttreePreMerg;
  stream << extControl._exttreeMaxcap;
  stream << extControl._useDbSdb;
  stream << extControl._CCnoPowerSource;
  stream << extControl._CCnoPowerTarget;
  stream << extControl._usingMetalPlanes;
  stream << extControl._ruleFileName;
  // new fields
  stream << extControl._extractedCornerList;
  stream << extControl._derivedCornerList;
  stream << extControl._cornerIndexList;
  stream << extControl._resFactorList;
  stream << extControl._gndcFactorList;
  stream << extControl._ccFactorList;

  return stream;
}

dbIStream& operator>>(dbIStream& stream, dbExtControl& extControl)
{
  stream >> extControl._overCell;
  stream >> extControl._extracted;
  if (!extControl._extracted)
    return stream;
  stream >> extControl._independentExtCorners;
  stream >> extControl._wireStamped;
  stream >> extControl._foreign;
  stream >> extControl._rsegCoord;
  stream >> extControl._lefRC;
  stream >> extControl._cornerCnt;
  stream >> extControl._ccPreseveGeom;
  stream >> extControl._ccUp;
  stream >> extControl._couplingFlag;
  stream >> extControl._coupleThreshold;
  stream >> extControl._mergeResBound;
  stream >> extControl._mergeViaRes;
  stream >> extControl._mergeParallelCC;
  stream >> extControl._exttreePreMerg;
  stream >> extControl._exttreeMaxcap;
  stream >> extControl._useDbSdb;
  stream >> extControl._CCnoPowerSource;
  stream >> extControl._CCnoPowerTarget;
  stream >> extControl._usingMetalPlanes;
  stream >> extControl._ruleFileName;
  stream >> extControl._extractedCornerList;
  stream >> extControl._derivedCornerList;
  stream >> extControl._cornerIndexList;
  stream >> extControl._resFactorList;
  stream >> extControl._gndcFactorList;
  stream >> extControl._ccFactorList;

  return stream;
}

}  // namespace odb
