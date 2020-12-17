// *****************************************************************************
// *****************************************************************************
// Copyright 2013 - 2015, Cadence Design Systems
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lex.h"
#include "defiDebug.hpp"


#include "defrData.hpp"
#include "defrSettings.hpp"

BEGIN_LEFDEF_PARSER_NAMESPACE

extern defrContext defContext;
    
    /*******************
 *  Debug flags:
 *  0 -
 *  1 - malloc debug
 *  2 - print each history size bump up.
 *  3 - print each call to CatchAll
 *  4 -
 *  5 -
 *  6 -
 *  7 -
 *  8 -
 *  9 -
 * 10 -
 * 11 - lexer debug
 *
 ******************************/

/* Set flag */
void defiSetDebug(int, int) {
}

/* Read flag */
int defiDebug(int) {
    return 0;
}

void defiError(int check, int msgNum, const char* mess, defrData *defData) {
  /* check is 1 if the caller function has checked totalMsgLimit, etc. */
  if (!defData) {
    defData = defContext.data;
  }

  return defData->defiError(check, msgNum, mess);
}

const char* upperCase(const char* str, defrData *defData) {
  if (!defData) {
    defData = defContext.data;
  }

   return defData->upperCase(str);
}

const char* DEFCASE(const char* ch, defrData *defData) {
  if (!defData) {
    defData = defContext.data;
  }

   return defData->DEFCASE(ch);
}

END_LEFDEF_PARSER_NAMESPACE

