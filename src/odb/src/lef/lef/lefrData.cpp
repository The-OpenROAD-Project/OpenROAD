// *****************************************************************************
// *****************************************************************************
// Copyright 2012 - 2016, Cadence Design Systems
// 
// This  file  is  part  of  the  Cadence  LEF/DEF  Open   Source
// Distribution,  Product Version 5.8. 
// 
// Licensed under the Apache License, Version 2.0 (the "License");
//    you may not use this file except in compliance with the License.
//    You may obtain a copy of the License at
// 
//        http://www.apache.org/licenses/LICENSE-2.0
// 
//    Unless required by applicable law or agreed to in writing, software
//    distributed under the License is distributed on an "AS IS" BASIS,
//    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or
//    implied. See the License for the specific language governing
//    permissions and limitations under the License.
// 
// For updates, support, or to become part of the LEF/DEF Community,
// check www.openeda.org for details.
// 
//  $Author: arakhman $
//  $Revision: #11 $
//  $Date: 2013/04/23 $
//  $State:  $
// *****************************************************************************
// *****************************************************************************
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#include "lefrData.hpp"
#include "lefrSettings.hpp"

using namespace std;

BEGIN_LEFDEF_PARSER_NAMESPACE

extern void *lefMalloc(size_t lef_size);

lefrData  *lefData = NULL;

lefrData::lefrData()
: antennaInoutWarnings(0),
  antennaInputWarnings(0),
  antennaOutputWarnings(0),
  antennaType(lefiAntennaAR),
  arrayCutsVal(0),
  arrayCutsWar(0),
  arrayName(NULL),
  arrayWarnings(0),
  caseSensitiveWarnings(0),
  correctionTableWarnings(0),
  dielectricWarnings(0),
  doneLib(1),
  edgeRateScaleFactorWarnings(0),
  edgeRateThreshold1Warnings(0),
  edgeRateThreshold2Warnings(0),
  encrypted(0),
  first(1),
  first_buffer(0),
  ge56almostDone(0),
  ge56done(0),
  hasBusBit(0),
  hasDirection(0),
  hasDivChar(0),
  hasFixedMask(0),
  hasGeoLayer(0),
  hasInfluence(0),
  hasLayerMincut(0),
  hasManufactur(0),
  hasMask(0),
  hasMinfeature(0),
  hasNameCase(0),
  hasOpenedLogFile(0),
  hasPRP(0),
  hasParallel(0),
  hasPitch(0),
  hasSamenet(0),
  hasSite(0),
  hasSiteClass(0),
  hasSiteSize(0),
  hasSpCenter(0),
  hasSpLayer(0),
  hasSpParallel(0),
  hasSpSamenet(0),
  hasTwoWidths(0),
  hasType(0),
  hasVer(0),
  hasViaRule_layer(0),
  hasWidth(0),
  hasFatalError(0),
  iRDropWarnings(0),
  ignoreVersion(0),
  inDefine(0),
  inoutAntennaWarnings(0),
  inputAntennaWarnings(0),
  input_level(-1),
  isGenerate(0),
  last(NULL),
  layerCut(0),
  layerCutSpacing(0),
  layerDir(0),
  layerMastOver(0),
  layerName(NULL),
  layerRout(0),
  layerWarnings(0),
  lefDefIf(FALSE),
  lefDumbMode(0),
  lefErrMsgPrinted(0),
  lefFixedMask(0),
  lefInfoMsgPrinted(0),
  lefInvalidChar(0),
  namesCaseSensitive(TRUE),
  lefNdRule(0),
  lefNewIsKeyword(0),
  lefNlToken(FALSE),
  lefNoNum(0),
  lefPropDefType('\0'),
  lefRetVal(0),
  lefWRetVal(0),
  lefWarnMsgPrinted(0),
  lef_errors(0),
  lef_nlines(1),
  lef_ntokens(0),
  lef_save_x(0.0),
  lef_save_y(0.0),
  lef_warnings(0),
  lefch(NULL),
  leflVal(0.0),
  lefrAntennaPWLPtr(0),
  lefrArray(),
  lefrCorrectionTable(),
  lefrDensity(),
  lefrDoGcell(0),
  lefrDoGeometries(0),
  lefrDoSite(0),
  lefrDoTrack(0),
  lefrFile(0),
  lefrFileName(0),
  lefrGcellPatternPtr(0),
  lefrGeometriesPtr(0),
  lefrHasLayer(0),
  lefrHasMaxVS(0),
  lefrHasSpacing(0),
  lefrHasSpacingTbl(0),
  lefrIRDrop(),
  lefrLayer(),
  lefrLog(0),
  lefrMacro(),
  lefrMaxStackVia(),
  lefrMinFeature(),
  lefrNoiseMargin(),
  lefrNoiseTable(),
  lefrNonDefault(),
  lefrObstruction(),
  lefrPin(),
  lefrProp(),
  lefrSite(),
  lefrSitePatternPtr(0),
  lefrSpacing(),
  lefrTiming(),
  lefrTrackPatternPtr(0),
  lefrUnits(),
  lefrUseMinSpacing(),
  lefrVal(0.0),
  lefrVia(),
  lefrViaRule(),
  macroName(NULL),
  macroNum(),
  macroWarnings(0),
  maxStackViaWarnings(0),

  minFeatureWarnings(0),
  msgCnt(1),
  nd(0),
  ndLayer(0),
  ndLayerSpace(0),
  ndLayerWidth(0),
  ndName(0),
  ndRule(0),
  needGeometry(0),
  next(NULL),
  noWireExtensionWarnings(0),
  noiseMarginWarnings(0),
  noiseTableWarnings(0),
  nonDefaultRuleName(NULL),
  nonDefaultWarnings(0),
  numVia(0),
  obsDef(0),
  origDef(0),
  outMsg(NULL),
  outputAntennaWarnings(0),
  pinDef(0),
  pinName(NULL),
  pinWarnings(0),
  prtNewLine(0),
  prtSemiColon(0),
  ringPlace(0),
  shiftBuf(0),
  shiftBufLength(0),
  siteDef(0),
  siteName(NULL),
  siteWarnings(0),
  sizeDef(0),
  spParallelLength(0),
  spaceMissing(0),
  spacingWarnings(0),
  symDef(0),
  timingWarnings(0),
  unitsWarnings(0),
  use5_3(0),
  use5_4(0),
  useLenThr(0),
  useMinSpacingWarnings(0),
  versionNum(CURRENT_VERSION),
  viaLayer(0),
  viaName(NULL),
  viaRuleHasDir(0),
  viaRuleHasEnc(0),
  viaRuleLayer(0),
  viaRuleName(NULL),
  viaRuleWarnings(0),
  viaWarnings(0),
  current_token((char*) malloc(TOKEN_SIZE)),
  pv_token((char*) malloc(TOKEN_SIZE)),
  uc_token((char*) malloc(TOKEN_SIZE)), 
  tokenSize(TOKEN_SIZE)
{
    Hist_text.push_back('\0');

    // Initialization of arrays.
    memset(ring, 0, RING_SIZE * sizeof(char*));
    memset(ringSizes, 0, RING_SIZE * sizeof(int));
    memset(lefDebug, 0, 100 * sizeof(char));
    memset(current_buffer, 0, IN_BUF_SIZE * sizeof(char));
    memset(current_stack, 0, 20 * sizeof(char*));
    memset(lefrErrMsg, 0, 1024 * sizeof(char));
    memset(msgLimit, 0, 2 * MAX_LEF_MSGS * sizeof(int));
    memset(temp_name, 0, 258 * sizeof(char));

    
    current_token[0] = '\0';

    //lef_lex_init()
    struct stat statbuf;

    //initRingBuffer();
    int i;
    ringPlace = 0;
    for (i = 0; i < RING_SIZE; i++) {
        ring[i] = (char*) lefMalloc(TOKEN_SIZE);
        ringSizes[i] = TOKEN_SIZE;
    }

    if (first) {
        first = 0;
    }

    lef_nlines = 1;
    last = current_buffer - 1;
    next = current_buffer;
    encrypted = 0;
    first_buffer = 1;
    // 12/08/1999 -- Wanda da Rosa 
    // open the lefrLog to write 
    /* 3/23/2000 -- Wanda da Rosa.  Due to lots of complain, don't open
       the file until there is really warning messages only.
    if ((lefrLog = fopen("lefRWarning.log", "w")) == 0) {
       printf(
       "WARNING: Unable to open the file lefRWarning.log for writing from the directory %s.\n",
          getcwd(NULL, 64));
       printf("Warning messages will not be printed.\n");
    }
    */

    // 4/11/2003 - Remove file lefrRWarning.log from directory if it exist 
    // pcr 569729 
    if (stat("lefRWarning.log", &statbuf) != -1) {
        // file exist, remove it 
        if (!lefSettings->LogFileAppend)
            remove("lefRWarning.log");
    }

    // initialize the value 
    leflVal = strtod("-2147483648", &lefch);
    lefrVal = strtod("2147483647", &lefch);  
}


lefrData::~lefrData()
{
    //lef_lex_un_init()
    /* Close the file */
    if (lefrLog) {
        fclose(lefrLog);
        lefrLog = 0;
    }

    //destroyRingBuffer();
    for (int i = 0; i < RING_SIZE; i++) {
        free(ring[i]);
    }

    free(current_token);
    free(uc_token);
    free(pv_token);

    if (lefrAntennaPWLPtr) {
        lefrAntennaPWLPtr->Destroy();
        free(lefrAntennaPWLPtr);
    }
}

void
lefrData::reset()
{
    if (lefData) {
        delete lefData;
    }

    lefData = new lefrData();
}

void 
lefrData::initRead()
{
	hasVer = 1; 
	hasBusBit = 0; 
	hasDirection = 0; 
	hasDivChar = 0; 
	hasFixedMask = 0; 
	hasGeoLayer = 0; 
	hasInfluence = 0; 
	hasLayerMincut = 0; 
	hasManufactur = 0; 
	hasMask = 0; 
	hasMinfeature = 0; 
	hasNameCase = 0; 
	hasOpenedLogFile = 0; 
	hasPRP = 0; 
	hasParallel = 0; 
	hasPitch = 0; 
	hasSamenet = 0; 
	hasSite = 0; 
	hasSiteClass = 0; 
	hasSiteSize = 0; 
	hasSpCenter = 0; 
	hasSpLayer = 0; 
	hasSpParallel = 0; 
	hasSpSamenet = 0; 
	hasTwoWidths = 0; 
	hasType = 0; 
	hasViaRule_layer = 0; 
	hasWidth = 0; 
}

END_LEFDEF_PARSER_NAMESPACE

