// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include <cstdint>
#include <string>

#include "odb/db.h"
#include "odb/dbObject.h"
#include "odb/dbStream.h"

namespace odb {

class dbExtControl : public dbObject
{
 public:
  // PERSISTANT-MEMBERS
  bool _independentExtCorners;
  bool _foreign;
  bool _wireStamped;
  bool _rsegCoord;
  bool _overCell;
  bool _extracted;
  bool _lefRC;
  uint32_t _cornerCnt;
  uint32_t _ccPreseveGeom;
  uint32_t _ccUp;
  uint32_t _couplingFlag;
  double _coupleThreshold;
  double _mergeResBound;
  bool _mergeViaRes;
  bool _mergeParallelCC;
  bool _exttreePreMerg;
  double _exttreeMaxcap;
  bool _useDbSdb;
  uint32_t _ccNoPowerSource;
  uint32_t _ccNoPowerTarget;
  bool _usingMetalPlanes;
  std::string _ruleFileName;
  std::string _extractedCornerList;
  std::string _derivedCornerList;
  std::string _cornerIndexList;
  std::string _resFactorList;
  std::string _gndcFactorList;
  std::string _ccFactorList;

  dbExtControl();
};

dbOStream& operator<<(dbOStream& stream, const dbExtControl& extControl);
dbIStream& operator>>(dbIStream& stream, dbExtControl& extControl);

}  // namespace odb
