// *****************************************************************************
// *****************************************************************************
// Copyright 2013-2014, Cadence Design Systems
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

#include "defrReader.hpp"

#include <cstring>
#include <string>
#include <map>

#define DEF_DEBUG_IDS 100

BEGIN_LEFDEF_PARSER_NAMESPACE

struct defCompareCStrings 
{
    bool operator()(const char* lhs, const char* rhs) const {
        return std::strcmp(lhs, rhs) < 0;
    }
};

typedef std::map<const char*, int, defCompareCStrings>  defKeywordMap;

class defrSettings {
public:
    defrSettings();

    void init_symbol_table();

    defKeywordMap Keyword_set; 

    int defiDeltaNumberLines;

    ////////////////////////////////////
    //
    //       Flags to control number of warnings to print out, max will be 999
    //
    ////////////////////////////////////

    int AssertionWarnings;
    int BlockageWarnings;
    int CaseSensitiveWarnings;
    int ComponentWarnings;
    int ConstraintWarnings;
    int DefaultCapWarnings;
    int FillWarnings;
    int GcellGridWarnings;
    int IOTimingWarnings;
    int LogFileAppend; 
    int NetWarnings;
    int NonDefaultWarnings;
    int PinExtWarnings;
    int PinWarnings;
    int RegionWarnings;
    int RowWarnings;
    int TrackWarnings;
    int ScanchainWarnings;
    int SNetWarnings;
    int StylesWarnings;
    int UnitsWarnings;
    int VersionWarnings;
    int ViaWarnings;

    int  nDDMsgs; 
    int* disableDMsgs;
    int  totalDefMsgLimit; // to save the user set total msg limit to output
    int AddPathToNet;
    int AllowComponentNets;
    char CommentChar;
    int DisPropStrProcess; 

    int reader_case_sensitive_set;

    DEFI_READ_FUNCTION ReadFunction;
    DEFI_LOG_FUNCTION ErrorLogFunction;
    DEFI_WARNING_LOG_FUNCTION WarningLogFunction;
    DEFI_CONTEXT_LOG_FUNCTION ContextErrorLogFunction;
    DEFI_CONTEXT_WARNING_LOG_FUNCTION ContextWarningLogFunction;
    DEFI_MAGIC_COMMENT_FOUND_FUNCTION MagicCommentFoundFunction;
    DEFI_MALLOC_FUNCTION MallocFunction;
    DEFI_REALLOC_FUNCTION ReallocFunction;
    DEFI_FREE_FUNCTION FreeFunction;
    DEFI_LINE_NUMBER_FUNCTION LineNumberFunction;
    DEFI_LONG_LINE_NUMBER_FUNCTION LongLineNumberFunction;
    DEFI_CONTEXT_LINE_NUMBER_FUNCTION ContextLineNumberFunction;
    DEFI_CONTEXT_LONG_LINE_NUMBER_FUNCTION ContextLongLineNumberFunction;

    int UnusedCallbacks[CBMAX];
    int MsgLimit[DEF_MSGS];
};


class defrSession {
public:
    defrSession();

    char*           FileName;
    int             reader_case_sensitive;
    defiUserData    UserData;

    defiPropType    CompProp;
    defiPropType    CompPinProp;
    defiPropType    DesignProp;
    defiPropType    GroupProp;
    defiPropType    NDefProp;
    defiPropType    NetProp;
    defiPropType    RegionProp;
    defiPropType    RowProp;
    defiPropType    SNetProp;
};

END_LEFDEF_PARSER_NAMESPACE

#endif
