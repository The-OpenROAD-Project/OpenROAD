// *****************************************************************************
// *****************************************************************************
// Copyright 2013-2019, Cadence Design Systems
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
//  $Revision: #6 $
//  $Date: 2013/08/09 $
//  $State:  $
// *****************************************************************************
// *****************************************************************************

#ifndef defrSettings_h
#define defrSettings_h

#include <cstring>
#include <map>
#include <string>

#include "defrReader.hpp"

#define DEF_DEBUG_IDS 100
#define defMaxOxides 32

BEGIN_DEF_PARSER_NAMESPACE

struct defCompareCStrings
{
  bool operator()(const char* lhs, const char* rhs) const
  {
    return std::strcmp(lhs, rhs) < 0;
  }
};

using defKeywordMap = std::map<const char*, int, defCompareCStrings>;

class defrSettings
{
 public:
  defrSettings();

  void init_symbol_table();

  defKeywordMap Keyword_set;

  int defiDeltaNumberLines{10000};

  ////////////////////////////////////
  //
  //       Flags to control number of warnings to print out, max will be 999
  //
  ////////////////////////////////////

  int AssertionWarnings{999};
  int BlockageWarnings{999};
  int CaseSensitiveWarnings{999};
  int ComponentWarnings{999};
  int ConstraintWarnings{999};
  int DefaultCapWarnings{999};
  int FillWarnings{999};
  int GcellGridWarnings{999};
  int IOTimingWarnings{999};
  int NetWarnings{999};
  int NonDefaultWarnings{999};
  int PinExtWarnings{999};
  int PinWarnings{999};
  int RegionWarnings{999};
  int RowWarnings{999};
  int TrackWarnings{999};
  int ScanchainWarnings{999};
  int SNetWarnings{999};
  int StylesWarnings{999};
  int UnitsWarnings{999};
  int VersionWarnings{999};
  int ViaWarnings{999};

  int nDDMsgs{0};
  int* disableDMsgs{nullptr};
  int totalDefMsgLimit{0};  // to save the user set total msg limit to output
  int AddPathToNet{0};
  int AllowComponentNets{0};
  char CommentChar{'#'};

  DEFI_READ_FUNCTION ReadFunction{nullptr};
  DEFI_LOG_FUNCTION ErrorLogFunction{nullptr};
  DEFI_WARNING_LOG_FUNCTION WarningLogFunction{nullptr};
  DEFI_CONTEXT_LOG_FUNCTION ContextErrorLogFunction{nullptr};
  DEFI_CONTEXT_WARNING_LOG_FUNCTION ContextWarningLogFunction{nullptr};
  DEFI_MAGIC_COMMENT_FOUND_FUNCTION MagicCommentFoundFunction{nullptr};
  DEFI_MALLOC_FUNCTION MallocFunction{nullptr};
  DEFI_REALLOC_FUNCTION ReallocFunction{nullptr};
  DEFI_FREE_FUNCTION FreeFunction{nullptr};
  DEFI_LINE_NUMBER_FUNCTION LineNumberFunction{nullptr};
  DEFI_LONG_LINE_NUMBER_FUNCTION LongLineNumberFunction{nullptr};
  DEFI_CONTEXT_LINE_NUMBER_FUNCTION ContextLineNumberFunction{nullptr};
  DEFI_CONTEXT_LONG_LINE_NUMBER_FUNCTION ContextLongLineNumberFunction{nullptr};

  int UnusedCallbacks[CBMAX];
  int MsgLimit[DEF_MSGS];
  int reader_case_sensitive_set{0};
  int DisPropStrProcess{0};
  int LogFileAppend{0};

  static const char* defOxides[defMaxOxides];
};

class defrSession
{
 public:
  char* FileName{nullptr};
  int reader_case_sensitive{0};
  defiUserData UserData{nullptr};

  defiPropType CompProp;
  defiPropType CompPinProp;
  defiPropType DesignProp;
  defiPropType GroupProp;
  defiPropType NDefProp;
  defiPropType NetProp;
  defiPropType RegionProp;
  defiPropType RowProp;
  defiPropType SNetProp;
};

END_DEF_PARSER_NAMESPACE

#endif
