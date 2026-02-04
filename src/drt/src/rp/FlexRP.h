// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include <utility>

#include "boost/icl/interval_set.hpp"
#include "db/tech/frConstraint.h"
#include "db/tech/frLayer.h"
#include "db/tech/frTechObject.h"
#include "db/tech/frViaDef.h"
#include "frBaseTypes.h"
#include "frDesign.h"
#include "global.h"
#include "utl/Logger.h"

namespace drt {
class FlexRP
{
 public:
  FlexRP(frDesign* design,
         utl::Logger* logger,
         RouterConfiguration* router_cfg);

  void main();

 private:
  // init
  void init();

  // prep
  void prep();

  // functions
  void prep_viaForbiddenThrough();
  void prep_minStepViasCheck();
  bool hasMinStepViol(const odb::Rect& r1,
                      const odb::Rect& r2,
                      frLayerNum lNum);
  void prep_viaForbiddenThrough_helper(const frLayerNum& lNum,
                                       const int& tableLayerIdx,
                                       const int& tableEntryIdx,
                                       const frViaDef* viaDef,
                                       bool isCurrDirX);
  bool prep_viaForbiddenThrough_minStep(const frLayerNum& lNum,
                                        const frViaDef* viaDef,
                                        bool isCurrDirX);
  void prep_lineForbiddenLen();
  void prep_eolForbiddenLen_helper(const frLayer* layer,
                                   frCoord eolWidth,
                                   frCoord& eolSpace,
                                   frCoord& eolWithin);
  void prep_eolForbiddenLen();
  void prep_cutSpcTbl();
  void prep_lineForbiddenLen_helper(const frLayerNum& lNum,
                                    const int& tableLayerIdx,
                                    const int& tableEntryIdx,
                                    bool isZShape,
                                    bool isCurrDirX);
  void prep_lineForbiddenLen_minSpc(const frLayerNum& lNum,
                                    bool isZShape,
                                    bool isCurrDirX,
                                    ForbiddenRanges& forbiddenRanges);
  void prep_viaForbiddenPlanarLen();
  void prep_viaForbiddenPlanarLen_helper(const frLayerNum& lNum,
                                         const int& tableLayerIdx,
                                         const int& tableEntryIdx,
                                         const frViaDef* viaDef,
                                         bool isCurrDirX);
  void prep_viaForbiddenPlanarLen_minStep(const frLayerNum& lNum,
                                          const frViaDef* viaDef,
                                          bool isCurrDirX,
                                          ForbiddenRanges& forbiddenRanges);
  void prep_viaForbiddenTurnLen(frNonDefaultRule* ndr = nullptr);
  void prep_viaForbiddenTurnLen_helper(const frLayerNum& lNum,
                                       const int& tableLayerIdx,
                                       const int& tableEntryIdx,
                                       const frViaDef* viaDef,
                                       bool isCurrDirX,
                                       frNonDefaultRule* ndr = nullptr);
  void prep_viaForbiddenTurnLen_minSpc(const frLayerNum& lNum,
                                       const frViaDef* viaDef,
                                       bool isCurrDirX,
                                       ForbiddenRanges& forbiddenRanges,
                                       frNonDefaultRule* ndr = nullptr);
  void prep_via2viaForbiddenLen(frNonDefaultRule* ndr = nullptr);
  void prep_via2viaForbiddenLen_helper(const frLayerNum& lNum,
                                       const int& tableLayerIdx,
                                       const int& tableEntryIdx,
                                       const frViaDef* viaDef1,
                                       const frViaDef* viaDef2,
                                       bool isHorizontal,
                                       frNonDefaultRule* ndr = nullptr);
  void prep_via2viaForbiddenLen_minStep(const frLayerNum& lNum,
                                        const frViaDef* viaDef1,
                                        const frViaDef* viaDef2,
                                        bool isVertical,
                                        ForbiddenRanges& forbiddenRanges);
  void prep_via2viaForbiddenLen_minimumCut(const frLayerNum& lNum,
                                           const frViaDef* viaDef1,
                                           const frViaDef* viaDef2,
                                           bool isCurrDirX,
                                           ForbiddenRanges& forbiddenRanges);
  void prep_via2viaForbiddenLen_widthViaMap(const frLayerNum& lNum,
                                            const frViaDef* viaDef1,
                                            const frViaDef* viaDef2,
                                            bool isCurrDirX,
                                            ForbiddenRanges& forbiddenRanges);
  void prep_via2viaForbiddenLen_cutSpc(const frLayerNum& lNum,
                                       const frViaDef* viaDef1,
                                       const frViaDef* viaDef2,
                                       bool isCurrDirX,
                                       ForbiddenRanges& forbiddenRanges);
  void prep_via2viaForbiddenLen_minSpc(frLayerNum lNum,
                                       const frViaDef* viaDef1,
                                       const frViaDef* viaDef2,
                                       bool isCurrDirX,
                                       ForbiddenRanges& forbiddenRanges,
                                       frNonDefaultRule* ndr = nullptr);
  void prep_via2viaPRL(frLayerNum lNum,
                       const frViaDef* viaDef1,
                       const frViaDef* viaDef2,
                       bool isCurrDirX,
                       frCoord& prl);
  void prep_via2viaForbiddenLen_lef58CutSpc(const frLayerNum& lNum,
                                            const frViaDef* viaDef1,
                                            const frViaDef* viaDef2,
                                            bool isCurrDirX,
                                            ForbiddenRanges& forbiddenRanges);

  void prep_via2viaForbiddenLen_lef58CutSpcTbl(
      const frLayerNum& lNum,
      const frViaDef* viaDef1,
      const frViaDef* viaDef2,
      bool isCurrDirX,
      ForbiddenRanges& forbiddenRanges);

  void prep_via2viaForbiddenLen_lef58CutSpc_helper(
      const odb::Rect& enclosureBox1,
      const odb::Rect& enclosureBox2,
      const odb::Rect& cutBox,
      frCoord reqSpcVal,
      std::pair<frCoord, frCoord>& range);

  frDesign* design_;
  frTechObject* tech_;
  utl::Logger* logger_;
  RouterConfiguration* router_cfg_;
};
}  // namespace drt
