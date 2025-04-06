// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include <string>

#include "db.h"

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
  uint _cornerCnt;
  uint _ccPreseveGeom;
  uint _ccUp;
  uint _couplingFlag;
  double _coupleThreshold;
  double _mergeResBound;
  bool _mergeViaRes;
  bool _mergeParallelCC;
  bool _exttreePreMerg;
  double _exttreeMaxcap;
  bool _useDbSdb;
  uint _CCnoPowerSource;
  uint _CCnoPowerTarget;
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
