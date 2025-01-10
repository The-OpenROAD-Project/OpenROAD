// *****************************************************************************
// *****************************************************************************
// Copyright 2012 - 2019, Cadence Design Systems
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

#ifndef lefrSettings_h
#define lefrSettings_h

#include <cstring>
#include <map>
#include <set>
#include <string>

#include "lefrReader.hpp"

#define lefMaxOxides 32

BEGIN_LEF_PARSER_NAMESPACE

struct lefCompareCStrings
{
  bool operator()(const char* lhs, const char* rhs) const
  {
    return std::strcmp(lhs, rhs) < 0;
  }
};

using lefKeywordMap = std::map<const char*, int, lefCompareCStrings>;
using MsgsDisableMap = std::map<int, int>;

using StringSet = std::set<std::string>;

class lefrProps
{
 public:
  lefiPropType lefrCompProp;
  lefiPropType lefrLayerProp;
  lefiPropType lefrLibProp;
  lefiPropType lefrMacroProp;
  lefiPropType lefrNondefProp;
  lefiPropType lefrPinProp;
  lefiPropType lefrViaProp;
  lefiPropType lefrViaRuleProp;
};

class lefrSettings
{
 public:
  lefrSettings();

  void init_symbol_table();
  static void reset();
  void addLef58Type(const char* lef58Type, const char** layerType);

  std::string getLayerLef58Types(const char* type) const;

  void disableMsg(int msgId);
  void enableMsg(int msgId);
  void enableAllMsgs();
  int suppresMsg(int msgId);

  static std::string getToken(const std::string& input, int& startIdx);

  LEFI_LINE_NUMBER_FUNCTION LineNumberFunction{nullptr};
  LEFI_READ_FUNCTION ReadFunction{nullptr};
  int AntennaInoutWarnings{999};
  int AntennaInputWarnings{999};
  int AntennaOutputWarnings{999};
  int ArrayWarnings;
  int CaseSensitive{0};
  int CaseSensitiveSet{0};
  int CaseSensitiveWarnings{999};
  char CommentChar{'#'};
  int CorrectionTableWarnings{999};
  int DeltaNumberLines{10000};
  int DielectricWarnings{999};
  int DisPropStrProcess{0};
  int EdgeRateScaleFactorWarnings{999};
  int EdgeRateThreshold1Warnings{999};
  int EdgeRateThreshold2Warnings{999};
  int IRDropWarnings{999};
  int InoutAntennaWarnings{999};
  int InputAntennaWarnings{999};
  int LayerWarnings{999};
  int LogFileAppend{0};
  int MacroWarnings{999};
  int MaxStackViaWarnings{999};
  int MinFeatureWarnings{999};
  int NoWireExtensionWarnings{999};
  int NoiseMarginWarnings{999};
  int NoiseTableWarnings{999};
  int NonDefaultWarnings{999};
  int OutputAntennaWarnings{999};
  int PinWarnings{999};
  int ReadEncrypted{0};
  int RegisterUnused{0};
  int RelaxMode{0};
  int ShiftCase{0};
  int SiteWarnings{999};
  int SpacingWarnings{999};
  int TimingWarnings{999};
  int TotalMsgLimit{0};
  int UnitsWarnings{999};
  int UseMinSpacingWarnings{999};
  int ViaRuleWarnings{999};
  int ViaWarnings{999};
  lefiUserData UserData{nullptr};
  int dAllMsgs{0};
  double VersionNum{0.0};

  LEFI_MALLOC_FUNCTION MallocFunction{nullptr};
  LEFI_REALLOC_FUNCTION ReallocFunction{nullptr};
  LEFI_FREE_FUNCTION FreeFunction{nullptr};
  LEFI_LOG_FUNCTION ErrorLogFunction{nullptr};
  LEFI_LOG_FUNCTION SetLogFunction{nullptr};
  LEFI_WARNING_LOG_FUNCTION WarningLogFunction{nullptr};
  StringSet Lef58TypePairs;

  int MsgLimit[MAX_LEF_MSGS];

  MsgsDisableMap msgsDisableMap;

  lefrProps lefProps;
  static const char* lefOxides[lefMaxOxides];

  lefKeywordMap Keyword_set;
};

extern lefrSettings* lefSettings;

END_LEF_PARSER_NAMESPACE

#endif
