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

#include <stdio.h>
#include <unistd.h>

#include <list>
#include <string>

#include "lefiDebug.hpp"
#include "lefiUtil.hpp"
#include "lefin.h"
#include "lefrReader.hpp"
#include "utility/Logger.h"

namespace odb {

static int antennaCB(lefrCallbackType_e c, double value, lefiUserData ud)
{
  lefin* lef = (lefin*) ud;

  switch (c) {
    case lefrAntennaInputCbkType:
      lef->antenna(lefin::ANTENNA_INPUT_GATE_AREA, value);
      break;
    case lefrAntennaInoutCbkType:
      lef->antenna(lefin::ANTENNA_INOUT_DIFF_AREA, value);
      break;
    case lefrAntennaOutputCbkType:
      lef->antenna(lefin::ANTENNA_OUTPUT_DIFF_AREA, value);
      break;
    case lefrInputAntennaCbkType:
      lef->antenna(lefin::ANTENNA_INPUT_SIZE, value);
      break;
    case lefrOutputAntennaCbkType:
      lef->antenna(lefin::ANTENNA_OUTPUT_SIZE, value);
      break;
    case lefrInoutAntennaCbkType:
      lef->antenna(lefin::ANTENNA_INOUT_SIZE, value);
      break;
    default:
      break;
  }

  return 0;
}

static int arrayBeginCB(lefrCallbackType_e /* unused: c */,
                        const char*  name,
                        lefiUserData ud)
{
  lefin* lef = (lefin*) ud;
  lef->arrayBegin(name);
  return 0;
}

static int arrayCB(lefrCallbackType_e /* unused: c */,
                   lefiArray*   a,
                   lefiUserData ud)
{
  lefin* lef = (lefin*) ud;
  lef->array(a);
  return 0;
}

static int arrayEndCB(lefrCallbackType_e /* unused: c */,
                      const char*  name,
                      lefiUserData ud)
{
  lefin* lef = (lefin*) ud;
  lef->arrayEnd(name);
  return 0;
}

static int busBitCharsCB(lefrCallbackType_e /* unused: c */,
                         const char*  busBit,
                         lefiUserData ud)
{
  lefin* lef = (lefin*) ud;
  lef->busBitChars(busBit);
  return 0;
}

static int caseSensCB(lefrCallbackType_e /* unused: c */,
                      int          caseSense,
                      lefiUserData ud)
{
  lefin* lef = (lefin*) ud;
  lef->caseSense(caseSense);
  return 0;
}

static int clearanceCB(lefrCallbackType_e /* unused: c */,
                       const char*  name,
                       lefiUserData ud)
{
  lefin* lef = (lefin*) ud;
  lef->clearance(name);
  return 0;
}

static int dividerCB(lefrCallbackType_e /* unused: c */,
                     const char*  name,
                     lefiUserData ud)
{
  lefin* lef = (lefin*) ud;
  lef->divider(name);
  return 0;
}

static int noWireExtCB(lefrCallbackType_e /* unused: c */,
                       const char*  name,
                       lefiUserData ud)
{
  lefin* lef = (lefin*) ud;
  lef->noWireExt(name);
  return 0;
}

static int noiseMarCB(lefrCallbackType_e /* unused: c */,
                      lefiNoiseMargin* noise,
                      lefiUserData     ud)
{
  lefin* lef = (lefin*) ud;
  lef->noiseMargin(noise);
  return 0;
}

static int edge1CB(lefrCallbackType_e /* unused: c */,
                   double       value,
                   lefiUserData ud)
{
  lefin* lef = (lefin*) ud;
  lef->edge1(value);
  return 0;
}

static int edge2CB(lefrCallbackType_e /* unused: c */,
                   double       value,
                   lefiUserData ud)
{
  lefin* lef = (lefin*) ud;
  lef->edge2(value);
  return 0;
}

static int edgeScaleCB(lefrCallbackType_e /* unused: c */,
                       double       value,
                       lefiUserData ud)
{
  lefin* lef = (lefin*) ud;
  lef->edgeScale(value);
  return 0;
}

static int noiseTableCB(lefrCallbackType_e /* unused: c */,
                        lefiNoiseTable* noise,
                        lefiUserData    ud)
{
  lefin* lef = (lefin*) ud;
  lef->noiseTable(noise);
  return 0;
}

static int correctionCB(lefrCallbackType_e /* unused: c */,
                        lefiCorrectionTable* corr,
                        lefiUserData         ud)
{
  lefin* lef = (lefin*) ud;
  lef->correction(corr);
  return 0;
}

static int dielectricCB(lefrCallbackType_e /* unused: c */,
                        double       dielectric,
                        lefiUserData ud)
{
  lefin* lef = (lefin*) ud;
  lef->dielectric(dielectric);
  return 0;
}

static int irdropBeginCB(lefrCallbackType_e /* unused: c */,
                         void*        ptr,
                         lefiUserData ud)
{
  lefin* lef = (lefin*) ud;
  lef->irdropBegin(ptr);
  return 0;
}

static int irdropCB(lefrCallbackType_e /* unused: c */,
                    lefiIRDrop*  irdrop,
                    lefiUserData ud)
{
  lefin* lef = (lefin*) ud;
  lef->irdrop(irdrop);
  return 0;
}

static int irdropEndCB(lefrCallbackType_e /* unused: c */,
                       void*        ptr,
                       lefiUserData ud)
{
  lefin* lef = (lefin*) ud;
  lef->irdropEnd(ptr);
  return 0;
}

static int layerCB(lefrCallbackType_e /* unused: c */,
                   lefiLayer*   layer,
                   lefiUserData ud)
{
  lefin* lef = (lefin*) ud;
  lef->layer(layer);
  return 0;
}

static int macroBeginCB(lefrCallbackType_e /* unused: c */,
                        const char*  name,
                        lefiUserData ud)
{
  lefin* lef = (lefin*) ud;
  lef->macroBegin(name);
  return 0;
}

static int macroCB(lefrCallbackType_e /* unused: c */,
                   lefiMacro*   macro,
                   lefiUserData ud)
{
  lefin* lef = (lefin*) ud;
  lef->macro(macro);
  return 0;
}

static int macroEndCB(lefrCallbackType_e /* unused: c */,
                      const char*  name,
                      lefiUserData ud)
{
  lefin* lef = (lefin*) ud;
  lef->macroEnd(name);
  return 0;
}

static int manufacturingCB(lefrCallbackType_e /* unused: c */,
                           double       num,
                           lefiUserData ud)
{
  lefin* lef = (lefin*) ud;
  lef->manufacturing(num);
  return 0;
}

static int maxStackViaCB(lefrCallbackType_e /* unused: c */,
                         lefiMaxStackVia* max,
                         lefiUserData     ud)
{
  lefin* lef = (lefin*) ud;
  lef->maxStackVia(max);
  return 0;
}

static int minFeatureCB(lefrCallbackType_e /* unused: c */,
                        lefiMinFeature* min,
                        lefiUserData    ud)
{
  lefin* lef = (lefin*) ud;
  lef->minFeature(min);
  return 0;
}

static int nonDefaultCB(lefrCallbackType_e /* unused: c */,
                        lefiNonDefault* def,
                        lefiUserData    ud)
{
  lefin* lef = (lefin*) ud;
  lef->nonDefault(def);
  return 0;
}

static int obstructionCB(lefrCallbackType_e /* unused: c */,
                         lefiObstruction* obs,
                         lefiUserData     ud)
{
  lefin* lef = (lefin*) ud;
  lef->obstruction(obs);
  return 0;
}

static int pinCB(lefrCallbackType_e /* unused: c */,
                 lefiPin*     pin,
                 lefiUserData ud)
{
  lefin* lef = (lefin*) ud;
  lef->pin(pin);
  return 0;
}

static int propDefBeginCB(lefrCallbackType_e /* unused: c */,
                          void*        ptr,
                          lefiUserData ud)
{
  lefin* lef = (lefin*) ud;
  lef->propDefBegin(ptr);
  return 0;
}

static int propDefCB(lefrCallbackType_e /* unused: c */,
                     lefiProp*    prop,
                     lefiUserData ud)
{
  lefin* lef = (lefin*) ud;
  lef->propDef(prop);
  return 0;
}

static int propDefEndCB(lefrCallbackType_e /* unused: c */,
                        void*        ptr,
                        lefiUserData ud)
{
  lefin* lef = (lefin*) ud;
  lef->propDefEnd(ptr);
  return 0;
}

static int siteCB(lefrCallbackType_e /* unused: c */,
                  lefiSite*    site,
                  lefiUserData ud)
{
  lefin* lef = (lefin*) ud;
  lef->site(site);
  return 0;
}

static int spacingBeginCB(lefrCallbackType_e /* unused: c */,
                          void*        ptr,
                          lefiUserData ud)
{
  lefin* lef = (lefin*) ud;
  lef->spacingBegin(ptr);
  return 0;
}

static int spacingCB(lefrCallbackType_e /* unused: c */,
                     lefiSpacing* spacing,
                     lefiUserData ud)
{
  lefin* lef = (lefin*) ud;
  lef->spacing(spacing);
  return 0;
}

static int spacingEndCB(lefrCallbackType_e /* unused: c */,
                        void*        ptr,
                        lefiUserData ud)
{
  lefin* lef = (lefin*) ud;
  lef->spacingEnd(ptr);
  return 0;
}

static int timingCB(lefrCallbackType_e /* unused: c */,
                    lefiTiming*  timing,
                    lefiUserData ud)
{
  lefin* lef = (lefin*) ud;
  lef->timing(timing);
  return 0;
}

static int unitsCB(lefrCallbackType_e /* unused: c */,
                   lefiUnits*   unit,
                   lefiUserData ud)
{
  lefin* lef = (lefin*) ud;
  lef->units(unit);
  return 0;
}

static int useMinSpacingCB(lefrCallbackType_e /* unused: c */,
                           lefiUseMinSpacing* spacing,
                           lefiUserData       ud)
{
  lefin* lef = (lefin*) ud;
  lef->useMinSpacing(spacing);
  return 0;
}

static int versionCB(lefrCallbackType_e /* unused: c */,
                     double       num,
                     lefiUserData ud)
{
  lefin* lef = (lefin*) ud;
  lef->version(num);
  return 0;
}

static int viaCB(lefrCallbackType_e /* unused: c */,
                 lefiVia*     via,
                 lefiUserData ud)
{
  lefin* lef = (lefin*) ud;
  lef->via(via);
  return 0;
}

static int viaRuleCB(lefrCallbackType_e /* unused: c */,
                     lefiViaRule* rule,
                     lefiUserData ud)
{
  lefin* lef = (lefin*) ud;
  lef->viaRule(rule);
  return 0;
}

static int doneCB(lefrCallbackType_e /* unused: c */,
                  void*        ptr,
                  lefiUserData ud)
{
  lefin* lef = (lefin*) ud;
  lef->done(ptr);
  return 0;
}

static void errorCB(const char* msg)
{
  lefin* lef = (lefin*) lefrGetUserData();
  lef->errorTolerant(219, "Error: {}", msg);
}

static void warningCB(const char* msg)
{
  lefin* lef = (lefin*) lefrGetUserData();
  lef->warning(220, msg);
}

static void lineNumberCB(int line)
{
  lefin* lef = (lefin*) lefrGetUserData();
  lef->lineNumber(line);
}

bool lefin_parse(lefin* lef, utl::Logger* logger, const char* file_name)
{
  // sets the parser to be case sensitive...
  // default was supposed to be the case but false...
  // lefrSetCaseSensitivity(true);
  lefrSetAntennaInputCbk(antennaCB);
  lefrSetAntennaInoutCbk(antennaCB);
  lefrSetAntennaOutputCbk(antennaCB);
  lefrSetArrayBeginCbk(arrayBeginCB);
  lefrSetArrayCbk(arrayCB);
  lefrSetArrayEndCbk(arrayEndCB);
  lefrSetBusBitCharsCbk(busBitCharsCB);
  lefrSetCaseSensitiveCbk(caseSensCB);
  lefrSetClearanceMeasureCbk(clearanceCB);
  lefrSetDividerCharCbk(dividerCB);
  lefrSetNoWireExtensionCbk(noWireExtCB);
  lefrSetNoiseMarginCbk(noiseMarCB);
  lefrSetEdgeRateThreshold1Cbk(edge1CB);
  lefrSetEdgeRateThreshold2Cbk(edge2CB);
  lefrSetEdgeRateScaleFactorCbk(edgeScaleCB);
  lefrSetNoiseTableCbk(noiseTableCB);
  lefrSetCorrectionTableCbk(correctionCB);
  lefrSetDielectricCbk(dielectricCB);
  lefrSetIRDropBeginCbk(irdropBeginCB);
  lefrSetIRDropCbk(irdropCB);
  lefrSetIRDropEndCbk(irdropEndCB);
  lefrSetLayerCbk(layerCB);
  lefrSetLibraryEndCbk(doneCB);
  lefrSetMacroBeginCbk(macroBeginCB);
  lefrSetMacroCbk(macroCB);
  lefrSetMacroEndCbk(macroEndCB);
  lefrSetManufacturingCbk(manufacturingCB);
  lefrSetMaxStackViaCbk(maxStackViaCB);
  lefrSetMinFeatureCbk(minFeatureCB);
  lefrSetNonDefaultCbk(nonDefaultCB);
  lefrSetObstructionCbk(obstructionCB);
  lefrSetPinCbk(pinCB);
  lefrSetPropBeginCbk(propDefBeginCB);
  lefrSetPropCbk(propDefCB);
  lefrSetPropEndCbk(propDefEndCB);
  lefrSetSiteCbk(siteCB);
  lefrSetSpacingBeginCbk(spacingBeginCB);
  lefrSetSpacingCbk(spacingCB);
  lefrSetSpacingEndCbk(spacingEndCB);
  lefrSetTimingCbk(timingCB);
  lefrSetUnitsCbk(unitsCB);
  lefrSetUseMinSpacingCbk(useMinSpacingCB);
  lefrSetVersionCbk(versionCB);
  lefrSetViaCbk(viaCB);
  lefrSetViaRuleCbk(viaRuleCB);
  lefrSetInputAntennaCbk(antennaCB);
  lefrSetOutputAntennaCbk(antennaCB);
  lefrSetInoutAntennaCbk(antennaCB);
  lefrSetLogFunction(errorCB);
  lefrSetWarningLogFunction(warningCB);
  lefrSetLineNumberFunction(lineNumberCB);

  // Available callbacks not registered - FIXME??
  // lefrSetDensityCbk
  // lefrSetExtensionCbk
  // lefrSetFixedMaskCbk
  // lefrSetMacroClassTypeCbk
  // lefrSetMacroFixedMaskCbk
  // lefrSetMacroForeignCbk
  // lefrSetMacroOriginCbk
  // lefrSetMacroSiteCbk
  // lefrSetMacroSizeCbk

  lefrSetDeltaNumberLines(1000000);
  lefrInit();

  FILE* file = fopen(file_name, "r");

  if (file == NULL) {
    logger->warn(utl::ODB, 240, "error: Cannot open LEF file {}", file_name);
    return false;
  }

  int res = lefrRead(file, file_name, (void*) lef);

  fclose(file);

  lefrClear();

  if (res)
    return false;

  return true;
}

int lefin_get_current_line()
{
  return lefrLineNumber();
}

}  // namespace odb
