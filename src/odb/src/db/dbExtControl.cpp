// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "odb/dbExtControl.h"

#include "dbDatabase.h"
#include "odb/db.h"

namespace odb {

dbExtControl::dbExtControl()
{
  _independentExtCorners = false;
  _wireStamped = false;
  _foreign = false;
  _rsegCoord = false;
  _overCell = false;
  _extracted = false;
  _lefRC = false;
  _cornerCnt = 0;
  _ccPreseveGeom = 0;
  _ccUp = 0;
  _couplingFlag = 3;
  _coupleThreshold = 0.1;  // fF
  _mergeResBound = 50.0;   // OHMS
  _mergeViaRes = true;
  _mergeParallelCC = true;
  _exttreePreMerg = true;
  _exttreeMaxcap = 0.0;
  _useDbSdb = true;
  _ccNoPowerSource = 0;
  _ccNoPowerTarget = 0;
  _usingMetalPlanes = false;
}

dbOStream& operator<<(dbOStream& stream, const dbExtControl& extControl)
{
  stream << extControl._overCell;
  stream << extControl._extracted;
  if (!extControl._extracted) {
    return stream;
  }
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
  stream << extControl._ccNoPowerSource;
  stream << extControl._ccNoPowerTarget;
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
  if (!extControl._extracted) {
    return stream;
  }
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
  stream >> extControl._ccNoPowerSource;
  stream >> extControl._ccNoPowerTarget;
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
