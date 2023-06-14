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
//  $Revision: #6 $
//  $Date: 2015/01/20 $
//  $State:  $
// *****************************************************************************
// *****************************************************************************

/*
 * FILE: crypt.cpp
 *
 */

#include <stdio.h>
#include <stdarg.h>

#include "lefiKRDefs.hpp"

BEGIN_LEFDEF_PARSER_NAMESPACE

#ifdef WIN32
#   include <io.h>
#else // not WIN32 
#   include <unistd.h>

#endif // WIN32 


FILE *
encOpenFileForRead(char *filename)
{
    return fopen(filename, "r");
}

FILE *
encOpenFileForWrite(char    *filename,
                    int     encrypt_f)
{
    return fopen(filename, "w");
}

int
encCloseFile(FILE *fp)
{
    return fclose(fp);
}

void
encClearBuf(FILE *fp)
{
}

void
encReadingEncrypted()
{
}

void
encWritingEncrypted()
{
}

int
encIsEncrypted(unsigned char *buf)
{
    return false;
}

int
encFgetc(FILE *fp)
{
    return fgetc(fp);
}

int
encFputc(char   c,
         FILE   *fp)
{
    return fputc(c, fp);
}

void
encPrint(FILE   *fp,
         char   *format,
         ...)
{
    va_list ap;

    va_start(ap, format);
    vfprintf(fp, format, ap);
    va_end(ap);
}

END_LEFDEF_PARSER_NAMESPACE

