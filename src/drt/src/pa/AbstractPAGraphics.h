// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025-2025, The OpenROAD Authors

#pragma once

#include <memory>
#include <utility>
#include <vector>

#include "src/drt/src/db/obj/frAccess.h"
#include "src/drt/src/db/obj/frBPin.h"
#include "src/drt/src/db/obj/frBlockObject.h"
#include "src/drt/src/db/obj/frFig.h"
#include "src/drt/src/db/obj/frInst.h"
#include "src/drt/src/db/obj/frInstTerm.h"
#include "src/drt/src/db/obj/frMPin.h"
#include "src/drt/src/db/obj/frMarker.h"
#include "src/drt/src/db/obj/frShape.h"
#include "src/drt/src/db/obj/frVia.h"
#include "src/drt/src/frBaseTypes.h"
#include "src/drt/src/pa/FlexPA.h"
#include "src/drt/src/pa/FlexPA_unique.h"

namespace drt {

class AbstractPAGraphics
{
 public:
  virtual ~AbstractPAGraphics() = default;

  virtual void startPin(frBPin* pin,
                        frInstTerm* inst_term,
                        UniqueClass* inst_class) = 0;

  virtual void startPin(frMPin* pin,
                        frInstTerm* inst_term,
                        UniqueClass* inst_class) = 0;

  virtual void setAPs(const std::vector<std::unique_ptr<frAccessPoint>>& aps,
                      frAccessPointEnum lower_type,
                      frAccessPointEnum upper_type) = 0;

  virtual void setViaAP(const frAccessPoint* ap,
                        const frVia* via,
                        const std::vector<std::unique_ptr<frMarker>>& markers)
      = 0;

  virtual void setPlanarAP(
      const frAccessPoint* ap,
      const frPathSeg* seg,
      const std::vector<std::unique_ptr<frMarker>>& markers) = 0;

  virtual void setObjsAndMakers(
      const std::vector<std::pair<frConnFig*, frBlockObject*>>& objs,
      const std::vector<std::unique_ptr<frMarker>>& markers,
      FlexPA::PatternType type) = 0;
};

}  // namespace drt
