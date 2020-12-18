// *****************************************************************************
// *****************************************************************************
// Copyright 2012 - 2013, Cadence Design Systems
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
//  $Author: dell $
//  $Revision: #1 $
//  $Date: 2017/06/06 $
//  $State:  $
// *****************************************************************************
// *****************************************************************************

#include "lefiDebug.hpp"
#include "lefrReader.hpp"
#include "lex.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lefrData.hpp"
#include "lefrSettings.hpp"

BEGIN_LEFDEF_PARSER_NAMESPACE

// ******************
//   Debug flags:
//   0 -
//   1 - malloc debug
//   2 - print each history size bump up.
//   3 -
//   4 -
//   5 -
//   6 -
//   7 -
//   8 -
//   9 -
//  10 -
//  11 - lexer debug
//  12 -
//  13 - print each lex token as read in.
//
// ******************************

// Set flag 
void
lefiSetDebug(int    num,
             int    value)
{
    lefData->lefDebug[num] = value;
}
// Read flag 
int
lefiDebug(int num)
{
    return lefData->lefDebug[num];
}

void
lefiError(int           check,
          int           msgNum,
          const char    *str)
{

    // check is 1 if the caller function has checked TotalMsgLimit, etc. 

    if (!check) {
        if ((lefSettings->TotalMsgLimit > 0) && (lefData->lefErrMsgPrinted >= lefSettings->TotalMsgLimit))
            return;
        if (lefSettings->MsgLimit[msgNum] > 0) {
            if (lefData->msgLimit[0][msgNum] >= lefSettings->MsgLimit[msgNum]) //over the limit
                return;
            lefData->msgLimit[0][msgNum] = lefData->msgLimit[0][msgNum] + 1;
        }
        lefData->lefErrMsgPrinted++;
    }

    if (lefSettings->ErrorLogFunction)
        (*lefSettings->ErrorLogFunction)(str);
    else
        fprintf(stderr, "%s", str);
}

static char lefiShift [] = {
    '\0',
    '\0',
    '\0',
    '\0',
    '\0',
    '\0',
    '\0',
    '\0',
    '\0',
    '\0',
    '\0',
    '\0',
    '\0',
    '\0',
    '\0',
    '\0',
    '\0',
    '\0',
    '\0',
    '\0',
    '\0',
    '\0',
    '\0',
    '\0',
    '\0',
    '\0',
    '\0',
    '\0',
    '\0',
    '\0',
    '\0',
    '\0',
    ' ',
    '!',
    '"',
    '#',
    '$',
    '%',
    '&',
    '\'',
    '(',
    ')',
    '*',
    '+',
    ',',
    '-',
    '.',
    '/',
    '0',
    '1',
    '2',
    '3',
    '4',
    '5',
    '6',
    '7',
    '8',
    '9',
    ':',
    ';',
    '<',
    '=',
    '>',
    '?',
    '@',
    'A',
    'B',
    'C',
    'D',
    'E',
    'F',
    'G',
    'H',
    'I',
    'J',
    'K',
    'L',
    'M',
    'N',
    'O',
    'P',
    'Q',
    'R',
    'S',
    'T',
    'U',
    'V',
    'W',
    'X',
    'Y',
    'Z',
    '[',
    '\\',
    ']',
    '^',
    '_',
    '`',
    'A',
    'B',
    'C',
    'D',
    'E',
    'F',
    'G',
    'H',
    'I',
    'J',
    'K',
    'l',
    'M',
    'N',
    'O',
    'P',
    'Q',
    'R',
    'S',
    'T',
    'U',
    'V',
    'W',
    'X',
    'Y',
    'Z',
    '{',
    '|',
    '}',
    '~',
    '\0',
    '\0',
    '\0',
    '\0',
    '\0',
    '\0',
    '\0',
    '\0',
    '\0',
    '\0',
    '\0',
    '\0',
    '\0',
    '\0',
    '\0',
    '\0',
    '\0',
    '\0',
    '\0',
    '\0',
    '\0',
    '\0',
    '\0',
    '\0',
    '\0',
    '\0',
    '\0',
    '\0',
    '\0',
    '\0',
    '\0',
    '\0',
    '\0',
    '\0',
    '\0',
    '\0',
    '\0',
    '\0',
    '\0',
    '\0',
    '\0',
    '\0',
    '\0',
    '\0',
    '\0',
    '\0',
    '\0',
    '\0',
    '\0',
    '\0',
    '\0',
    '\0',
    '\0',
    '\0',
    '\0',
    '\0',
    '\0',
    '\0',
    '\0',
    '\0',
    '\0',
    '\0',
    '\0',
    '\0',
    '\0',
    '\0',
    '\0',
    '\0',
    '\0',
    '\0',
    '\0',
    '\0',
    '\0',
    '\0',
    '\0',
    '\0',
    '\0',
    '\0',
    '\0',
    '\0',
    '\0',
    '\0',
    '\0',
    '\0',
    '\0',
    '\0',
    '\0',
    '\0',
    '\0',
    '\0',
    '\0',
    '\0',
    '\0',
    '\0',
    '\0',
    '\0',
    '\0',
    '\0',
    '\0',
    '\0',
    '\0',
    '\0',
    '\0',
    '\0',
    '\0',
    '\0',
    '\0',
    '\0',
    '\0',
    '\0',
    '\0',
    '\0',
    '\0',
    '\0',
    '\0',
    '\0',
    '\0',
    '\0',
    '\0',
    '\0',
    '\0',
    '\0',
    '\0',
    '\0',
    '\0',
    '\0',
    '\0',
    '\0',
    '\0'
};

const char *
lefUpperCase(const char *str)
{
    char    *place = (char*) str;
    char    *to;
    int     len = strlen(str) + 1;

    if (len > lefData->shiftBufLength) {
        if (lefData->shiftBuf == 0) {
            len = len < 64 ? 64 : len;
            lefData->shiftBuf = (char*) lefMalloc(len);
            lefData->shiftBufLength = len;
        } else {
            lefFree(lefData->shiftBuf);
            lefData->shiftBuf = (char*) malloc(len);
            lefData->shiftBufLength = len;
        }
    }

    to = lefData->shiftBuf;
    while (*place) {
        int i = (int) *place;
        place++;
        *to++ = lefiShift[i];
    }
    *to = '\0';

    return lefData->shiftBuf;
}

// Function is done from #define CASE, compatibility only
const char *
CASE(const char *x)
{
    return !lefData->namesCaseSensitive && lefSettings->ShiftCase ? lefUpperCase(x) : x;
}

END_LEFDEF_PARSER_NAMESPACE

