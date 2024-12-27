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

BEGIN_LEFDEF_PARSER_NAMESPACE

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

  LEFI_LINE_NUMBER_FUNCTION LineNumberFunction;
  LEFI_READ_FUNCTION ReadFunction;
  int AntennaInoutWarnings;
  int AntennaInputWarnings;
  int AntennaOutputWarnings;
  int ArrayWarnings;
  int CaseSensitive;
  int CaseSensitiveSet;
  int CaseSensitiveWarnings;
  char CommentChar;
  int CorrectionTableWarnings;
  int DeltaNumberLines;
  int DielectricWarnings;
  int DisPropStrProcess;
  int EdgeRateScaleFactorWarnings;
  int EdgeRateThreshold1Warnings;
  int EdgeRateThreshold2Warnings;
  int IRDropWarnings;
  int InoutAntennaWarnings;
  int InputAntennaWarnings;
  int LayerWarnings;
  int LogFileAppend;
  int MacroWarnings;
  int MaxStackViaWarnings;
  int MinFeatureWarnings;
  int NoWireExtensionWarnings;
  int NoiseMarginWarnings;
  int NoiseTableWarnings;
  int NonDefaultWarnings;
  int OutputAntennaWarnings;
  int PinWarnings;
  int ReadEncrypted;
  int RegisterUnused;
  int RelaxMode;
  int ShiftCase;
  int SiteWarnings;
  int SpacingWarnings;
  int TimingWarnings;
  int TotalMsgLimit;
  int UnitsWarnings;
  int UseMinSpacingWarnings;
  int ViaRuleWarnings;
  int ViaWarnings;
  lefiUserData UserData;
  int dAllMsgs;
  double VersionNum;

  LEFI_MALLOC_FUNCTION MallocFunction;
  LEFI_REALLOC_FUNCTION ReallocFunction;
  LEFI_FREE_FUNCTION FreeFunction;
  LEFI_LOG_FUNCTION ErrorLogFunction;
  LEFI_LOG_FUNCTION SetLogFunction;
  LEFI_WARNING_LOG_FUNCTION WarningLogFunction;
  StringSet Lef58TypePairs;

  int MsgLimit[MAX_LEF_MSGS];

  MsgsDisableMap msgsDisableMap;

  lefrProps lefProps;
  static const char* lefOxides[lefMaxOxides];

  lefKeywordMap Keyword_set;
};

extern lefrSettings* lefSettings;

END_LEFDEF_PARSER_NAMESPACE

#endif
