/* Authors: Lutong Wang and Bangqi Xu */
/*
 * Copyright (c) 2019, The Regents of the University of California
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the University nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE REGENTS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#pragma once

#include <boost/icl/interval_set.hpp>

#include "frDesign.h"

namespace drt {
class FlexRP
{
 public:
  FlexRP(frDesign* design, Logger* logger, RouterConfiguration* router_cfg);

  void main();

 private:
  // init
  void init();

  // prep
  void prep();

  // functions
  void prep_viaForbiddenThrough();
  void prep_minStepViasCheck();
  bool hasMinStepViol(const Rect& r1, const Rect& r2, frLayerNum lNum);
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
      const Rect& enclosureBox1,
      const Rect& enclosureBox2,
      const Rect& cutBox,
      frCoord reqSpcVal,
      std::pair<frCoord, frCoord>& range);

  frDesign* design_;
  frTechObject* tech_;
  Logger* logger_;
  RouterConfiguration* router_cfg_;
};
}  // namespace drt
