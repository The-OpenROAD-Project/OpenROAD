// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "odb/dbExtControl.h"

#include "dbDatabase.h"
#include "odb/db.h"

namespace odb {

dbExtControl::dbExtControl()
{
  _wireStamped = false;
  _foreign = false;
  _rsegCoord = false;
  _overCell = false;
  _extracted = false;
  _lefRC = false;
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
  stream << extControl._wireStamped;
  stream << extControl._foreign;
  stream << extControl._rsegCoord;
  stream << extControl._lefRC;
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

  return stream;
}

dbIStream& operator>>(dbIStream& stream, dbExtControl& extControl)
{
  stream >> extControl._overCell;
  stream >> extControl._extracted;
  if (!extControl._extracted) {
    return stream;
  }

  _dbDatabase* db = stream.getDatabase();

  if (!db->isSchema(kSchemaRemovePerCornerBlock)) {
    bool obsolete_independent_ext_corners;
    stream >> obsolete_independent_ext_corners;
  }
  stream >> extControl._wireStamped;
  stream >> extControl._foreign;
  stream >> extControl._rsegCoord;
  stream >> extControl._lefRC;
  if (!db->isSchema(kSchemaRemoveExtControlCornerData)) {
    uint32_t obsolete_corner_cnt;
    stream >> obsolete_corner_cnt;
  }
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
  if (!db->isSchema(kSchemaRemoveExtControlCornerData)) {
    std::string obsolete_corner_list;
    stream >> obsolete_corner_list;  // _extractedCornerList
    stream >> obsolete_corner_list;  // _derivedCornerList
    stream >> obsolete_corner_list;  // _cornerIndexList
    stream >> obsolete_corner_list;  // _resFactorList
    stream >> obsolete_corner_list;  // _gndcFactorList
    stream >> obsolete_corner_list;  // _ccFactorList
  }

  return stream;
}

}  // namespace odb
