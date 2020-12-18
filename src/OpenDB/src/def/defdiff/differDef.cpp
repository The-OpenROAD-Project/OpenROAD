// *****************************************************************************
// *****************************************************************************
// Copyright 2012 - 2016, Cadence Design Systems
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
//  $Author$
//  $Revision$
//  $Date$
//  $State:  $
// *****************************************************************************
// *****************************************************************************


// This program will diff two lef files or two def files and list the
// different between the two files.  This problem is not intend to
// diff a real design.  If user runs this program will a big design,
// they may experience long execution time and may even ran out of
// memory.
//
// This program is to give user a feel of whether they are using the
// parser correctly.  After they read the lef/def file in, and
// write them back out in lef/def format.
//
// This program support lef/def 5.6.

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#ifdef ibmrs
#   include <strings.h>
#endif
#ifndef WIN32
#   include <unistd.h>
extern char VersionIdent[];
#else
char* VersionIdent = "N/A";
#endif /* not WIN32 */
#include "defrReader.hpp"
#include "diffDefRW.hpp"

char * exeName;   // use to save the executable name

// This program requires 3 input, the type of the file, lef or def
// fileName1 and fileName2
void diffUsage() {
    printf("Usage: lefdefdiff -lef|-def fileName1 fileName2 [-o outputFileName]\n");
}

int main(int argc, char** argv) {
    char *fileName1, *fileName2;   // For the filenames to compare
    char *defOut1, *defOut2;       // For the tmp output files

#ifdef WIN32
    // Enable two-digit exponent format
    _set_output_format(_TWO_DIGIT_EXPONENT);
#endif

    exeName = argv[0];

    if (argc != 9) {               // If pass in from lefdefdiff, argc is
        diffUsage();               // always 9: defdiff file1 file2 out1 out2
        return(1);                 // ignorePinExtra ignoreRowName ignoreViaName
    }                              // newSegCmp

    fileName1 = argv[1];
    fileName2 = argv[2];

    // Temporary output files, to whole the def file information as
    // they are read in.  Later these files will be sorted for compare
    defOut1 = argv[3];
    defOut2 = argv[4];

    // def files
    printf("Reading file: %s\n", fileName1);
    if (diffDefReadFile(fileName1, defOut1, argv[5], argv[6], argv[7], argv[8]) != 0)
        return(1);
    printf("Reading file: %s\n", fileName2);
    if (diffDefReadFile(fileName2, defOut2, argv[5], argv[6], argv[7], argv[8]) != 0)
        return(1); 

    return (0);
}
