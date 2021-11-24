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

#ifndef CRYPT_H
#define CRYPT_H 1

#include "lefiKRDefs.hpp"

BEGIN_LEFDEF_PARSER_NAMESPACE

extern FILE* encOpenFileForRead(char* filename);
extern FILE* encOpenFileForWrite(char* filename, int encrypt_f);
extern int encCloseFile(FILE* fp);
extern void encClearBuf(FILE* fp);
extern void encReadingEncrypted();
extern void encWritingEncrypted();
extern int encIsEncrypted(unsigned char* buf);
extern int encFgetc(FILE* fp);
extern int encFputc(char c, FILE* fp);
extern void encPrint(FILE*fp, char* format,...);

END_LEFDEF_PARSER_NAMESPACE

USE_LEFDEF_PARSER_NAMESPACE

#endif

