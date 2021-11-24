/*******************************************************************************
 *******************************************************************************
 * Copyright 2012 - 2014, Cadence Design Systems
 * 
 * This  file  is  part  of  the  Cadence  LEF/DEF  Open   Source
 * Distribution,  Product Version 5.8. 
 * 
 * Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 * 
 *        http://www.apache.org/licenses/LICENSE-2.0
 * 
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or
 *    implied. See the License for the specific language governing
 *    permissions and limitations under the License.
 * 
 * For updates, support, or to become part of the LEF/DEF Community,
 * check www.openeda.org for details.
 *******************************************************************************
 * 
 *  $Author: dell $
 *  $Revision: #1 $
 *  $Date: 2017/06/06 $
 *  $State:  $
 ******************************************************************************/

#ifndef leh_h
#define leh_h

#include "lefiKRDefs.hpp"

BEGIN_LEFDEF_PARSER_NAMESPACE

void lefAddStringDefine(const char    *token,
                        const char    *string);

void lefAddBooleanDefine(const char   *token,
                         int          val);

void lefAddNumDefine(const char  *token,
                     double      val);

void yyerror(const char *s);
void lefError(int           msgNum,
              const char    *s);
void lefWarning(int         msgNum,
                const char  *s);
void lefInfo(int        msgNum,
             const char *s);
void *lefMalloc(size_t lef_size);
void *lefRealloc(void   *name,
                 size_t    lef_size);
void lefFree(void *name);
void lefSetNonDefault(const char *name);
void lefUnsetNonDefault();

extern int yylex();
extern void lex_init();
extern int lefyyparse();
extern void lex_un_init();

int fake_ftell();

END_LEFDEF_PARSER_NAMESPACE

#endif
