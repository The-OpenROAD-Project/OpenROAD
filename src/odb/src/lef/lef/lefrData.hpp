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
#include <map>
#include <string>
#include <vector>

#include "lefiArray.hpp"
#include "lefiCrossTalk.hpp"
#include "lefiDefs.hpp"
#include "lefiLayer.hpp"
#include "lefiMacro.hpp"
#include "lefiNonDefault.hpp"
#include "lefiProp.hpp"
#include "lefiPropType.hpp"
#include "lefiUnits.hpp"
#include "lefiUser.hpp"
#include "lefiUtil.hpp"
#include "lefiVia.hpp"
#include "lefiViaRule.hpp"
#include "lefrReader.hpp"

#define CURRENT_VERSION 5.8
#define RING_SIZE 10
#define IN_BUF_SIZE 16384
#define TOKEN_SIZE 4096

BEGIN_LEF_PARSER_NAMESPACE

struct lefCompareStrings
{
  bool operator()(const std::string& lhs, const std::string& rhs) const
  {
    return std::strcmp(lhs.c_str(), rhs.c_str()) < 0;
  }
};

using lefAliasMap = std::map<std::string, std::string, lefCompareStrings>;

using lefStringMap = std::map<std::string, std::string, lefCompareStrings>;

using lefIntMap = std::map<std::string, int, lefCompareStrings>;

using lefDoubleMap = std::map<std::string, double, lefCompareStrings>;

class lefrData
{
 public:
  lefrData();
  ~lefrData();

  static void reset();
  void initRead();
  void doubleBuffer();

  FILE* lefrFile{nullptr};
  FILE* lefrLog{nullptr};

  char lefPropDefType{'\0'};

  char* arrayName{nullptr};
  char* last{nullptr};
  char* layerName{nullptr};
  char* lefch{nullptr};
  char* lefrFileName{nullptr};
  char* macroName{nullptr};
  char* ndName{nullptr};
  char* next{nullptr};
  char* nonDefaultRuleName{nullptr};
  char* outMsg{nullptr};
  char* pinName{nullptr};
  char* shiftBuf{nullptr};
  char* siteName{nullptr};
  char* viaName{nullptr};
  char* viaRuleName{nullptr};

  double layerCutSpacing{0.0};
  double lef_save_x{0.0};
  double lef_save_y{0.0};  // for interpreting (*) notation of LEF/DEF
  double leflVal{0.0};
  double lefrVal{0.0};
  double versionNum{CURRENT_VERSION};

  int antennaInoutWarnings{0};
  int antennaInputWarnings{0};
  int antennaOutputWarnings{0};
  int arrayCutsVal{0};
  int arrayCutsWar{0};
  int arrayWarnings{0};
  int caseSensitiveWarnings{0};
  int correctionTableWarnings{0};
  int dielectricWarnings{0};
  int doneLib{1};  // keep track if the library is done parsing
  int edgeRateScaleFactorWarnings{0};
  int edgeRateThreshold1Warnings{0};
  int edgeRateThreshold2Warnings{0};
  int encrypted{0};
  int first{1};
  int first_buffer{0};
  int ge56almostDone{0};  // have reached the EXTENSION SECTION
  int ge56done{0};        // a 5.6 and it has END LIBRARY statement
  int hasBusBit{0};
  int hasDirection{0};
  int hasDivChar{0};
  int hasFixedMask{0};
  int hasGeoLayer{0};
  int hasInfluence{0};
  int hasLayerMincut{0};
  int hasManufactur{0};
  int hasMask{0};
  int hasMinfeature{0};
  int hasNameCase{0};
  int hasOpenedLogFile{0};
  int hasPRP{0};
  int hasParallel{0};
  int hasPitch{0};
  int hasSamenet{0};
  int hasSite{0};
  int hasSiteClass{0};
  int hasSiteSize{0};
  int hasSpCenter{0};
  int hasSpLayer{0};
  int hasSpParallel{0};
  int hasSpSamenet{0};
  int hasTwoWidths{0};
  int hasType{0};
  int hasVer{0};
  int hasViaRule_layer{0};
  int hasWidth{0};
  int hasFatalError{0};  // don't report errors after the file end.
  int iRDropWarnings{0};
  int ignoreVersion{0};  // ignore checking version number
  int inDefine{0};
  int inoutAntennaWarnings{0};
  int inputAntennaWarnings{0};
  int input_level{-1};
  int isGenerate{0};
  int layerCut{0};
  int layerDir{0};
  int layerMastOver{0};
  int layerRout{0};
  int layerWarnings{0};
  int lefDefIf{0};
  int lefDumbMode{0};
  int lefErrMsgPrinted{0};
  int lefFixedMask{0};  // All the LEF MACRO PIN MASK assignments can be
  int lefInfoMsgPrinted{0};
  int lefInvalidChar{0};
  int lefNdRule{0};
  int lefNewIsKeyword{0};
  int lefNlToken{0};
  int lefNoNum{0};
  int lefRetVal{0};
  int lefWRetVal{0};
  int lefWarnMsgPrinted{0};
  int lef_errors{0};
  int lef_nlines{1};
  int lef_ntokens{0};
  int lef_warnings{0};
  int lefrDoGcell{0};
  int lefrDoGeometries{0};
  int lefrDoSite{0};
  int lefrDoTrack{0};
  int lefrHasLayer{0};       // 5.5 this & lefrHasMaxVS is to keep track that
  int lefrHasMaxVS{0};       // MAXVIASTACK has to be after all layers
  int lefrHasSpacing{0};     // keep track of spacing in a layer
  int lefrHasSpacingTbl{0};  // keep track of spacing table in a layer

  int macroWarnings{0};
  int maxStackViaWarnings{0};
  int minFeatureWarnings{0};
  int msgCnt{1};
  int namesCaseSensitive{1};  // always true in 5.6
  int ndLayer{0};
  int ndLayerSpace{0};
  int ndLayerWidth{0};
  int ndRule{0};
  int needGeometry{0};
  int noWireExtensionWarnings{0};
  int noiseMarginWarnings{0};
  int noiseTableWarnings{0};
  int nonDefaultWarnings{0};
  int numVia{0};
  int obsDef{0};
  int origDef{0};
  int outputAntennaWarnings{0};
  int pinDef{0};
  int pinWarnings{0};
  int prtNewLine{0};    // sometimes need to print a new line
  int prtSemiColon{0};  // sometimes {0}; is not printed yet
  int ringPlace{0};
  int shiftBufLength{0};
  int siteDef{0};
  int siteWarnings{0};
  int sizeDef{0};
  int spParallelLength{0};
  int spaceMissing{0};
  int spacingWarnings{0};
  int symDef{0};
  int timingWarnings{0};
  int unitsWarnings{0};
  int use5_3{0};
  int use5_4{0};
  int useLenThr{0};
  int useMinSpacingWarnings{0};
  int viaLayer{0};
  int viaRuleHasDir{0};
  int viaRuleHasEnc{0};
  int viaRuleLayer{0};
  int viaRuleWarnings{0};
  int viaWarnings{0};

  lefiAntennaEnum antennaType{lefiAntennaAR};
  lefiAntennaPWL* lefrAntennaPWLPtr{nullptr};
  lefiArray lefrArray;
  lefiCorrectionTable lefrCorrectionTable;
  lefiDensity lefrDensity;
  lefiGcellPattern* lefrGcellPatternPtr{nullptr};
  lefiGeometries* lefrGeometriesPtr{nullptr};
  lefiIRDrop lefrIRDrop;
  lefiLayer lefrLayer;
  lefiMacro lefrMacro;
  lefiMaxStackVia lefrMaxStackVia;  // 5.5
  lefiMinFeature lefrMinFeature;
  lefiNoiseMargin lefrNoiseMargin;
  lefiNoiseTable lefrNoiseTable;
  lefiNonDefault lefrNonDefault;
  lefiNonDefault* nd{nullptr};  // For VIA in the nondefaultrule
  lefiNum macroNum;
  lefiObstruction lefrObstruction;
  lefiPin lefrPin;
  lefiProp lefrProp;
  lefiSite lefrSite;
  lefiSitePattern* lefrSitePatternPtr{nullptr};
  lefiSpacing lefrSpacing;
  lefiTiming lefrTiming;
  lefiTrackPattern* lefrTrackPatternPtr{nullptr};
  lefiUnits lefrUnits;
  lefiUseMinSpacing lefrUseMinSpacing;
  lefiVia lefrVia;
  lefiViaRule lefrViaRule;

  lefStringMap alias_set;
  lefDoubleMap define_set;
  lefIntMap defineb_set;
  lefStringMap defines_set;
  int tokenSize{TOKEN_SIZE};

  // ARRAYS
  //  Ring buffer storage
  char* ring[RING_SIZE];
  int ringSizes[RING_SIZE];
  char lefDebug[100];

  char* current_token;
  char* pv_token;
  char* uc_token;

  char current_buffer[IN_BUF_SIZE];
  const char* current_stack[20];  // the stack itself

  char lefrErrMsg[1024];
  char temp_name[258];

  std::vector<char> Hist_text;

  // to hold the msg limit, 0 - num of limit
  // 1 - num of message printed, 4701 = 4700 + 1, message starts on 1
  // 2 - warning printed
  int msgLimit[2][MAX_LEF_MSGS];
};

extern lefrData* lefData;

END_LEF_PARSER_NAMESPACE

#endif
