// *****************************************************************************
// *****************************************************************************
// Copyright 2012 - 2014, Cadence Design Systems
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

#ifndef lefrData_h
#define lefrData_h

#include <cstring>
#include <string>
#include <vector>
#include <map>

#include "lefiDefs.hpp"
#include "lefiUser.hpp"
#include "lefiLayer.hpp"
#include "lefiArray.hpp"
#include "lefiUtil.hpp"
#include "lefiMacro.hpp"
#include "lefiPropType.hpp"
#include "lefiCrossTalk.hpp"
#include "lefiProp.hpp"
#include "lefiNonDefault.hpp"
#include "lefiVia.hpp"
#include "lefiViaRule.hpp"
#include "lefiUnits.hpp"
#include "lefrReader.hpp"

#define CURRENT_VERSION 5.8
#define RING_SIZE 10
#define IN_BUF_SIZE 16384
#define TOKEN_SIZE 4096

BEGIN_LEFDEF_PARSER_NAMESPACE

struct lefCompareStrings 
{
    bool operator()(const std::string &lhs, const std::string &rhs) const {
        return std::strcmp(lhs.c_str(), rhs.c_str()) < 0;
    }
};

typedef std::map<std::string, std::string, lefCompareStrings> lefAliasMap;

typedef std::map<std::string, std::string, lefCompareStrings> lefStringMap;

typedef std::map<std::string, int, lefCompareStrings> lefIntMap;

typedef std::map<std::string, double, lefCompareStrings> lefDoubleMap;


class lefrData {
public:
    lefrData();
    ~lefrData();

    static void reset();
	void		initRead();
    void		doubleBuffer();
    
    FILE*  lefrFile; 
    FILE*  lefrLog; 

    char   lefPropDefType;  
    
    char*  arrayName; 
    char*  last; 
    char*  layerName; 
    char*  lefch; 
    char*  lefrFileName; 
    char*  macroName; 
    char*  ndName; 
    char*  next; 
    char*  nonDefaultRuleName; 
    char*  outMsg; 
    char*  pinName; 
    char*  shiftBuf; 
    char*  siteName; 
    char*  viaName; 
    char*  viaRuleName; 
    
    double  layerCutSpacing; 
    double  lef_save_x; 
    double  lef_save_y; // for interpreting (*) notation of LEF/DEF
    double  leflVal; 
    double  lefrVal; 
    double  versionNum; 
    
    int  antennaInoutWarnings; 
    int  antennaInputWarnings; 
    int  antennaOutputWarnings; 
    int  arrayCutsVal; 
    int  arrayCutsWar; 
    int  arrayWarnings; 
    int  caseSensitiveWarnings; 
    int  correctionTableWarnings; 
    int  dielectricWarnings; 
    int  doneLib; // keep track if the library is done parsing
    int  edgeRateScaleFactorWarnings; 
    int  edgeRateThreshold1Warnings; 
    int  edgeRateThreshold2Warnings; 
    int  encrypted; 
    int  first; 
    int  first_buffer; 
    int  ge56almostDone; // have reached the EXTENSION SECTION
    int  ge56done; // a 5.6 and it has END LIBRARY statement
    int  hasBusBit; 
    int  hasDirection; 
    int  hasDivChar; 
    int  hasFixedMask; 
    int  hasGeoLayer; 
    int  hasInfluence; 
    int  hasLayerMincut; 
    int  hasManufactur; 
    int  hasMask; 
    int  hasMinfeature; 
    int  hasNameCase; 
    int  hasOpenedLogFile; 
    int  hasPRP; 
    int  hasParallel; 
    int  hasPitch; 
    int  hasSamenet; 
    int  hasSite; 
    int  hasSiteClass; 
    int  hasSiteSize; 
    int  hasSpCenter; 
    int  hasSpLayer; 
    int  hasSpParallel; 
    int  hasSpSamenet; 
    int  hasTwoWidths; 
    int  hasType; 
    int  hasVer; 
    int  hasViaRule_layer; 
    int  hasWidth; 
    int  hasFatalError; // don't report errors after the file end.
    int  iRDropWarnings; 
    int  ignoreVersion; // ignore checking version number
    int  inDefine; 
    int  inoutAntennaWarnings; 
    int  inputAntennaWarnings; 
    int  input_level; 
    int  isGenerate; 
    int  layerCut; 
    int  layerDir; 
    int  layerMastOver; 
    int  layerRout; 
    int  layerWarnings; 
    int  lefDefIf; 
    int  lefDumbMode; 
    int  lefErrMsgPrinted; 
    int  lefFixedMask; //All the LEF MACRO PIN MASK assignments can be 
    int  lefInfoMsgPrinted; 
    int  lefInvalidChar; 
    int  lefNdRule; 
    int  lefNewIsKeyword; 
    int  lefNlToken; 
    int  lefNoNum; 
    int  lefRetVal; 
    int  lefWRetVal; 
    int  lefWarnMsgPrinted; 
    int  lef_errors; 
    int  lef_nlines; 
    int  lef_ntokens; 
    int  lef_warnings; 
    int  lefrDoGcell; 
    int  lefrDoGeometries; 
    int  lefrDoSite; 
    int  lefrDoTrack;
    int  lefrHasLayer; // 5.5 this & lefrHasMaxVS is to keep track that
    int  lefrHasMaxVS; // MAXVIASTACK has to be after all layers
    int  lefrHasSpacing; // keep track of spacing in a layer
    int  lefrHasSpacingTbl; // keep track of spacing table in a layer

    int  macroWarnings; 
    int  maxStackViaWarnings; 
    int  minFeatureWarnings; 
    int  msgCnt; 
    int  namesCaseSensitive; // always true in 5.6
    int  ndLayer; 
    int  ndLayerSpace; 
    int  ndLayerWidth; 
    int  ndRule; 
    int  needGeometry; 
    int  noWireExtensionWarnings; 
    int  noiseMarginWarnings; 
    int  noiseTableWarnings; 
    int  nonDefaultWarnings; 
    int  numVia; 
    int  obsDef; 
    int  origDef; 
    int  outputAntennaWarnings; 
    int  pinDef; 
    int  pinWarnings; 
    int  prtNewLine; // sometimes need to print a new line
    int  prtSemiColon; // sometimes ; is not printed yet
    int  ringPlace; 
    int  shiftBufLength; 
    int  siteDef; 
    int  siteWarnings; 
    int  sizeDef; 
    int  spParallelLength; 
    int  spaceMissing; 
    int  spacingWarnings; 
    int  symDef; 
    int  timingWarnings; 
    int  unitsWarnings; 
    int  use5_3; 
    int  use5_4; 
    int  useLenThr; 
    int  useMinSpacingWarnings; 
    int  viaLayer; 
    int  viaRuleHasDir; 
    int  viaRuleHasEnc; 
    int  viaRuleLayer; 
    int  viaRuleWarnings; 
    int  viaWarnings; 
    
    lefiAntennaEnum  antennaType; 
    lefiAntennaPWL*  lefrAntennaPWLPtr; 
    lefiArray  lefrArray; 
    lefiCorrectionTable  lefrCorrectionTable; 
    lefiDensity  lefrDensity; 
    lefiGcellPattern*  lefrGcellPatternPtr; 
    lefiGeometries*  lefrGeometriesPtr; 
    lefiIRDrop  lefrIRDrop; 
    lefiLayer  lefrLayer; 
    lefiMacro  lefrMacro; 
    lefiMaxStackVia  lefrMaxStackVia; // 5.5
    lefiMinFeature  lefrMinFeature; 
    lefiNoiseMargin  lefrNoiseMargin; 
    lefiNoiseTable  lefrNoiseTable; 
    lefiNonDefault  lefrNonDefault; 
    lefiNonDefault*  nd; // PCR 909010 - For VIA in the nondefaultrule
    lefiNum  macroNum; 
    lefiObstruction  lefrObstruction; 
    lefiPin  lefrPin; 
    lefiProp  lefrProp; 
    lefiSite  lefrSite; 
    lefiSitePattern*  lefrSitePatternPtr; 
    lefiSpacing  lefrSpacing; 
    lefiTiming  lefrTiming; 
    lefiTrackPattern*  lefrTrackPatternPtr; 
    lefiUnits  lefrUnits; 
    lefiUseMinSpacing  lefrUseMinSpacing; 
    lefiVia  lefrVia; 
    lefiViaRule  lefrViaRule; 
    
    lefStringMap        alias_set; 
    lefDoubleMap        define_set; 
    lefIntMap           defineb_set; 
    lefStringMap        defines_set; 
    int                 tokenSize;

    //ARRAYS
    // Ring buffer storage 
    char       *ring[RING_SIZE];
    int         ringSizes[RING_SIZE];
    char        lefDebug[100];

    char       *current_token; 
    char       *pv_token; 
    char       *uc_token; 

    char       current_buffer[IN_BUF_SIZE];
    const char *current_stack[20];  // the stack itself 

    char       lefrErrMsg[1024];
    char       temp_name[258];

    std::vector<char>  Hist_text; 

    // to hold the msg limit, 0 - num of limit 
    // 1 - num of message printed, 4701 = 4700 + 1, message starts on 1 
    // 2 - warning printed 
    int msgLimit[2][MAX_LEF_MSGS];
};

extern lefrData *lefData;

END_LEFDEF_PARSER_NAMESPACE

#endif

