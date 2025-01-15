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

#include <unistd.h>

#include <boost/algorithm/string/predicate.hpp>
#include <cstdio>
#include <list>
#include <string>

#include "lefiDebug.hpp"
#include "lefiUtil.hpp"
#include "lefrReader.hpp"
#include "lefzlib.hpp"
#include "odb/lefin.h"
#include "utl/Logger.h"

namespace odb {

static int antennaCB(LefParser::lefrCallbackType_e c,
                     double value,
                     LefParser::lefiUserData ud)
{
  lefinReader* lef = (lefinReader*) ud;

  switch (c) {
    case LefParser::lefrAntennaInputCbkType:
      lef->antenna(lefinReader::ANTENNA_INPUT_GATE_AREA, value);
      break;
    case LefParser::lefrAntennaInoutCbkType:
      lef->antenna(lefinReader::ANTENNA_INOUT_DIFF_AREA, value);
      break;
    case LefParser::lefrAntennaOutputCbkType:
      lef->antenna(lefinReader::ANTENNA_OUTPUT_DIFF_AREA, value);
      break;
    case LefParser::lefrInputAntennaCbkType:
      lef->antenna(lefinReader::ANTENNA_INPUT_SIZE, value);
      break;
    case LefParser::lefrOutputAntennaCbkType:
      lef->antenna(lefinReader::ANTENNA_OUTPUT_SIZE, value);
      break;
    case LefParser::lefrInoutAntennaCbkType:
      lef->antenna(lefinReader::ANTENNA_INOUT_SIZE, value);
      break;
    default:
      break;
  }

  return 0;
}

static int arrayBeginCB(LefParser::lefrCallbackType_e /* unused: c */,
                        const char* name,
                        LefParser::lefiUserData ud)
{
  lefinReader* lef = (lefinReader*) ud;
  lef->arrayBegin(name);
  return 0;
}

static int arrayCB(LefParser::lefrCallbackType_e /* unused: c */,
                   LefParser::lefiArray* a,
                   LefParser::lefiUserData ud)
{
  lefinReader* lef = (lefinReader*) ud;
  lef->array(a);
  return 0;
}

static int arrayEndCB(LefParser::lefrCallbackType_e /* unused: c */,
                      const char* name,
                      LefParser::lefiUserData ud)
{
  lefinReader* lef = (lefinReader*) ud;
  lef->arrayEnd(name);
  return 0;
}

static int busBitCharsCB(LefParser::lefrCallbackType_e /* unused: c */,
                         const char* busBit,
                         LefParser::lefiUserData ud)
{
  lefinReader* lef = (lefinReader*) ud;
  lef->busBitChars(busBit);
  return 0;
}

static int caseSensCB(LefParser::lefrCallbackType_e /* unused: c */,
                      int caseSense,
                      LefParser::lefiUserData ud)
{
  lefinReader* lef = (lefinReader*) ud;
  lef->caseSense(caseSense);
  return 0;
}

static int clearanceCB(LefParser::lefrCallbackType_e /* unused: c */,
                       const char* name,
                       LefParser::lefiUserData ud)
{
  lefinReader* lef = (lefinReader*) ud;
  lef->clearance(name);
  return 0;
}

static int dividerCB(LefParser::lefrCallbackType_e /* unused: c */,
                     const char* name,
                     LefParser::lefiUserData ud)
{
  lefinReader* lef = (lefinReader*) ud;
  lef->divider(name);
  return 0;
}

static int noWireExtCB(LefParser::lefrCallbackType_e /* unused: c */,
                       const char* name,
                       LefParser::lefiUserData ud)
{
  lefinReader* lef = (lefinReader*) ud;
  lef->noWireExt(name);
  return 0;
}

static int noiseMarCB(LefParser::lefrCallbackType_e /* unused: c */,
                      LefParser::lefiNoiseMargin* noise,
                      LefParser::lefiUserData ud)
{
  lefinReader* lef = (lefinReader*) ud;
  lef->noiseMargin(noise);
  return 0;
}

static int edge1CB(LefParser::lefrCallbackType_e /* unused: c */,
                   double value,
                   LefParser::lefiUserData ud)
{
  lefinReader* lef = (lefinReader*) ud;
  lef->edge1(value);
  return 0;
}

static int edge2CB(LefParser::lefrCallbackType_e /* unused: c */,
                   double value,
                   LefParser::lefiUserData ud)
{
  lefinReader* lef = (lefinReader*) ud;
  lef->edge2(value);
  return 0;
}

static int edgeScaleCB(LefParser::lefrCallbackType_e /* unused: c */,
                       double value,
                       LefParser::lefiUserData ud)
{
  lefinReader* lef = (lefinReader*) ud;
  lef->edgeScale(value);
  return 0;
}

static int noiseTableCB(LefParser::lefrCallbackType_e /* unused: c */,
                        LefParser::lefiNoiseTable* noise,
                        LefParser::lefiUserData ud)
{
  lefinReader* lef = (lefinReader*) ud;
  lef->noiseTable(noise);
  return 0;
}

static int correctionCB(LefParser::lefrCallbackType_e /* unused: c */,
                        LefParser::lefiCorrectionTable* corr,
                        LefParser::lefiUserData ud)
{
  lefinReader* lef = (lefinReader*) ud;
  lef->correction(corr);
  return 0;
}

static int dielectricCB(LefParser::lefrCallbackType_e /* unused: c */,
                        double dielectric,
                        LefParser::lefiUserData ud)
{
  lefinReader* lef = (lefinReader*) ud;
  lef->dielectric(dielectric);
  return 0;
}

static int irdropBeginCB(LefParser::lefrCallbackType_e /* unused: c */,
                         void* ptr,
                         LefParser::lefiUserData ud)
{
  lefinReader* lef = (lefinReader*) ud;
  lef->irdropBegin(ptr);
  return 0;
}

static int irdropCB(LefParser::lefrCallbackType_e /* unused: c */,
                    LefParser::lefiIRDrop* irdrop,
                    LefParser::lefiUserData ud)
{
  lefinReader* lef = (lefinReader*) ud;
  lef->irdrop(irdrop);
  return 0;
}

static int irdropEndCB(LefParser::lefrCallbackType_e /* unused: c */,
                       void* ptr,
                       LefParser::lefiUserData ud)
{
  lefinReader* lef = (lefinReader*) ud;
  lef->irdropEnd(ptr);
  return 0;
}

static int layerCB(LefParser::lefrCallbackType_e /* unused: c */,
                   LefParser::lefiLayer* layer,
                   LefParser::lefiUserData ud)
{
  lefinReader* lef = (lefinReader*) ud;
  lef->layer(layer);
  return 0;
}

static int macroBeginCB(LefParser::lefrCallbackType_e /* unused: c */,
                        const char* name,
                        LefParser::lefiUserData ud)
{
  lefinReader* lef = (lefinReader*) ud;
  lef->macroBegin(name);
  return 0;
}

static int macroCB(LefParser::lefrCallbackType_e /* unused: c */,
                   LefParser::lefiMacro* macro,
                   LefParser::lefiUserData ud)
{
  lefinReader* lef = (lefinReader*) ud;
  lef->macro(macro);
  return 0;
}

static int macroEndCB(LefParser::lefrCallbackType_e /* unused: c */,
                      const char* name,
                      LefParser::lefiUserData ud)
{
  lefinReader* lef = (lefinReader*) ud;
  lef->macroEnd(name);
  return 0;
}

static int manufacturingCB(LefParser::lefrCallbackType_e /* unused: c */,
                           double num,
                           LefParser::lefiUserData ud)
{
  lefinReader* lef = (lefinReader*) ud;
  lef->manufacturing(num);
  return 0;
}

static int maxStackViaCB(LefParser::lefrCallbackType_e /* unused: c */,
                         LefParser::lefiMaxStackVia* max,
                         LefParser::lefiUserData ud)
{
  lefinReader* lef = (lefinReader*) ud;
  lef->maxStackVia(max);
  return 0;
}

static int minFeatureCB(LefParser::lefrCallbackType_e /* unused: c */,
                        LefParser::lefiMinFeature* min,
                        LefParser::lefiUserData ud)
{
  lefinReader* lef = (lefinReader*) ud;
  lef->minFeature(min);
  return 0;
}

static int nonDefaultCB(LefParser::lefrCallbackType_e /* unused: c */,
                        LefParser::lefiNonDefault* def,
                        LefParser::lefiUserData ud)
{
  lefinReader* lef = (lefinReader*) ud;
  lef->nonDefault(def);
  return 0;
}

static int obstructionCB(LefParser::lefrCallbackType_e /* unused: c */,
                         LefParser::lefiObstruction* obs,
                         LefParser::lefiUserData ud)
{
  lefinReader* lef = (lefinReader*) ud;
  lef->obstruction(obs);
  return 0;
}

static int pinCB(LefParser::lefrCallbackType_e /* unused: c */,
                 LefParser::lefiPin* pin,
                 LefParser::lefiUserData ud)
{
  lefinReader* lef = (lefinReader*) ud;
  lef->pin(pin);
  return 0;
}

static int propDefBeginCB(LefParser::lefrCallbackType_e /* unused: c */,
                          void* ptr,
                          LefParser::lefiUserData ud)
{
  lefinReader* lef = (lefinReader*) ud;
  lef->propDefBegin(ptr);
  return 0;
}

static int propDefCB(LefParser::lefrCallbackType_e /* unused: c */,
                     LefParser::lefiProp* prop,
                     LefParser::lefiUserData ud)
{
  lefinReader* lef = (lefinReader*) ud;
  lef->propDef(prop);
  return 0;
}

static int propDefEndCB(LefParser::lefrCallbackType_e /* unused: c */,
                        void* ptr,
                        LefParser::lefiUserData ud)
{
  lefinReader* lef = (lefinReader*) ud;
  lef->propDefEnd(ptr);
  return 0;
}

static int siteCB(LefParser::lefrCallbackType_e /* unused: c */,
                  LefParser::lefiSite* site,
                  LefParser::lefiUserData ud)
{
  lefinReader* lef = (lefinReader*) ud;
  lef->site(site);
  return 0;
}

static int spacingBeginCB(LefParser::lefrCallbackType_e /* unused: c */,
                          void* ptr,
                          LefParser::lefiUserData ud)
{
  lefinReader* lef = (lefinReader*) ud;
  lef->spacingBegin(ptr);
  return 0;
}

static int spacingCB(LefParser::lefrCallbackType_e /* unused: c */,
                     LefParser::lefiSpacing* spacing,
                     LefParser::lefiUserData ud)
{
  lefinReader* lef = (lefinReader*) ud;
  lef->spacing(spacing);
  return 0;
}

static int spacingEndCB(LefParser::lefrCallbackType_e /* unused: c */,
                        void* ptr,
                        LefParser::lefiUserData ud)
{
  lefinReader* lef = (lefinReader*) ud;
  lef->spacingEnd(ptr);
  return 0;
}

static int timingCB(LefParser::lefrCallbackType_e /* unused: c */,
                    LefParser::lefiTiming* timing,
                    LefParser::lefiUserData ud)
{
  lefinReader* lef = (lefinReader*) ud;
  lef->timing(timing);
  return 0;
}

static int unitsCB(LefParser::lefrCallbackType_e /* unused: c */,
                   LefParser::lefiUnits* unit,
                   LefParser::lefiUserData ud)
{
  lefinReader* lef = (lefinReader*) ud;
  lef->units(unit);
  return 0;
}

static int useMinSpacingCB(LefParser::lefrCallbackType_e /* unused: c */,
                           LefParser::lefiUseMinSpacing* spacing,
                           LefParser::lefiUserData ud)
{
  lefinReader* lef = (lefinReader*) ud;
  lef->useMinSpacing(spacing);
  return 0;
}

static int versionCB(LefParser::lefrCallbackType_e /* unused: c */,
                     double num,
                     LefParser::lefiUserData ud)
{
  lefinReader* lef = (lefinReader*) ud;
  lef->version(num);
  return 0;
}

static int viaCB(LefParser::lefrCallbackType_e /* unused: c */,
                 LefParser::lefiVia* via,
                 LefParser::lefiUserData ud)
{
  lefinReader* lef = (lefinReader*) ud;
  lef->via(via);
  return 0;
}

static int viaRuleCB(LefParser::lefrCallbackType_e /* unused: c */,
                     LefParser::lefiViaRule* rule,
                     LefParser::lefiUserData ud)
{
  lefinReader* lef = (lefinReader*) ud;
  lef->viaRule(rule);
  return 0;
}

static int doneCB(LefParser::lefrCallbackType_e /* unused: c */,
                  void* ptr,
                  LefParser::lefiUserData ud)
{
  lefinReader* lef = (lefinReader*) ud;
  lef->done(ptr);
  return 0;
}

static void errorCB(const char* msg)
{
  lefinReader* lef = (lefinReader*) LefParser::lefrGetUserData();
  lef->errorTolerant(219, "Error: {}", msg);
}

static void warningCB(const char* msg)
{
  lefinReader* lef = (lefinReader*) LefParser::lefrGetUserData();
  lef->warning(220, msg);
}

static void lineNumberCB(int line)
{
  lefinReader* lef = (lefinReader*) LefParser::lefrGetUserData();
  lef->lineNumber(line);
}

bool lefin_parse(lefinReader* lef, utl::Logger* logger, const char* file_name)
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
  LefParser::lefrSetLogFunction(errorCB);
  LefParser::lefrSetWarningLogFunction(warningCB);
  LefParser::lefrSetLineNumberFunction(lineNumberCB);
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

  LefParser::lefrSetDeltaNumberLines(1000000);
  LefParser::lefrInit();
  int res;
  if (boost::algorithm::ends_with(file_name, ".gz")) {
    auto zfile = lefGZipOpen(file_name, "r");
    if (zfile == nullptr) {
      logger->warn(
          utl::ODB, 270, "error: Cannot open zipped LEF file {}", file_name);
      return false;
    }
    res = lefrReadGZip(zfile, file_name, (void*) lef);
    lefGZipClose(zfile);
  } else {
    FILE* file = fopen(file_name, "r");
    if (file == nullptr) {
      logger->warn(utl::ODB, 240, "error: Cannot open LEF file {}", file_name);
      return false;
    }
    res = LefParser::lefrRead(file, file_name, (void*) lef);
    fclose(file);
  }
  LefParser::lefrClear();

  if (res) {
    return false;
  }

  return true;
}

int lefin_get_current_line()
{
  return LefParser::lefrLineNumber();
}

}  // namespace odb
