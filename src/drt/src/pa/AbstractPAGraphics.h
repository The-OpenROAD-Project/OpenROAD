// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025-2025, The OpenROAD Authors

#pragma once

#include <memory>
#include <utility>
#include <vector>

#include "db/obj/frAccess.h"
#include "db/obj/frBPin.h"
#include "db/obj/frBlockObject.h"
#include "db/obj/frFig.h"
#include "db/obj/frInst.h"
#include "db/obj/frInstTerm.h"
#include "db/obj/frMPin.h"
#include "db/obj/frMarker.h"
#include "db/obj/frShape.h"
#include "db/obj/frVia.h"
#include "frBaseTypes.h"
#include "pa/FlexPA.h"
#include "pa/FlexPA_unique.h"

namespace drt {

class AbstractPAGraphics
{
 public:
  virtual ~AbstractPAGraphics() = default;

  virtual void startPin(frBPin* pin,
                        frInstTerm* inst_term,
                        UniqueClass* inst_class)
      = 0;

  virtual void startPin(frMPin* pin,
                        frInstTerm* inst_term,
                        UniqueClass* inst_class)
      = 0;

  virtual void setAPs(const std::vector<std::unique_ptr<frAccessPoint>>& aps,
                      frAccessPointEnum lower_type,
                      frAccessPointEnum upper_type)
      = 0;

  virtual void setViaAP(const frAccessPoint* ap,
                        const frVia* via,
                        const std::vector<std::unique_ptr<frMarker>>& markers)
      = 0;

  virtual void setPlanarAP(
      const frAccessPoint* ap,
      const frPathSeg* seg,
      const std::vector<std::unique_ptr<frMarker>>& markers)
      = 0;

  virtual void setObjsAndMakers(
      const std::vector<std::pair<frConnFig*, frBlockObject*>>& objs,
      const std::vector<std::unique_ptr<frMarker>>& markers,
      FlexPA::PatternType type)
      = 0;
};

}  // namespace drt
