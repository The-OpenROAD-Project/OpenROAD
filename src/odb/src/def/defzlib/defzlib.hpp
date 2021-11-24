// *****************************************************************************
// *****************************************************************************
// Copyright 2012 - 2015, Cadence Design Systems
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

#ifndef LEFDEFZIP_H
#define LEFDEFZIP_H

#include "defiDefs.hpp"

BEGIN_LEFDEF_PARSER_NAMESPACE

typedef void* defGZFile;
class defrContext;

//
// Name: defrSetReadGZipFunction
// Description: Sets GZip read function for the parser
// Returns: 0 if no errors
//
extern void defrSetGZipReadFunction();

//
// Name: defOpenGZip
// Description: Open a gzip file
// Returns: A file pointer
//
extern defGZFile defrGZipOpen(const char* gzipFile, const char* mode);

// 
// Name: defCloseGZip
// Description: Close a gzip file
// Returns: 0 if no errors
//
extern int defrGZipClose(defGZFile filePtr);

//
// Name: defrReadGZip
// Description: Parse a def gzip file
// Returns: 0 if no errors
//
extern int defrReadGZip(defGZFile file, const char* gzipFile, void* uData);

//
// FUNCTIONS TO BE OBSOLETED.
// The API is kept only for compatibility reasons.
//

//
// Name: defGZipOpen
// Description: Open a gzip file
// Returns: A file pointer
//
extern defGZFile defGZipOpen(const char* gzipFile, const char* mode);

// 
// Name: defGZipClose
// Description: Close a gzip file
// Returns: 0 if no errors
//
extern int defGZipClose(defGZFile filePtr);

END_LEFDEF_PARSER_NAMESPACE

USE_LEFDEF_PARSER_NAMESPACE

#endif
