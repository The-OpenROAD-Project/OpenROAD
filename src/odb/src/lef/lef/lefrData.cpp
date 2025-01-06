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
//  $Author: arakhman $
//  $Revision: #11 $
//  $Date: 2013/04/23 $
//  $State:  $
// *****************************************************************************
// *****************************************************************************
#include "lefrData.hpp"

#include <sys/stat.h>

#include <cctype>
#include <cstdio>
#include <cstdlib>

#include "lefrSettings.hpp"

BEGIN_LEF_PARSER_NAMESPACE

extern void* lefMalloc(size_t lef_size);

lefrData* lefData = nullptr;

lefrData::lefrData()
    : current_token((char*) malloc(TOKEN_SIZE)),
      pv_token((char*) malloc(TOKEN_SIZE)),
      uc_token((char*) malloc(TOKEN_SIZE))
{
  Hist_text.push_back('\0');

  // Initialization of arrays.
  memset(ring, 0, RING_SIZE * sizeof(char*));
  memset(ringSizes, 0, RING_SIZE * sizeof(int));
  memset(lefDebug, 0, 100 * sizeof(char));
  memset(current_buffer, 0, IN_BUF_SIZE * sizeof(char));
  memset(current_stack, 0, 20 * sizeof(char*));
  memset(lefrErrMsg, 0, 1024 * sizeof(char));
  memset(msgLimit, 0, 2 * MAX_LEF_MSGS * sizeof(int));
  memset(temp_name, 0, 258 * sizeof(char));

  current_token[0] = '\0';

  // lef_lex_init()
  struct stat statbuf;

  // initRingBuffer();
  int i;
  ringPlace = 0;
  for (i = 0; i < RING_SIZE; i++) {
    ring[i] = (char*) lefMalloc(TOKEN_SIZE);
    ringSizes[i] = TOKEN_SIZE;
  }

  if (first) {
    first = 0;
  }

  lef_nlines = 1;
  last = &current_buffer[0] - 1;
  next = current_buffer;
  encrypted = 0;
  first_buffer = 1;
  // 12/08/1999 -- Wanda da Rosa
  // open the lefrLog to write
  /* 3/23/2000 -- Wanda da Rosa.  Due to lots of complain, don't open
     the file until there is really warning messages only.
  if ((lefrLog = fopen("lefRWarning.log", "w")) == 0) {
     printf(
     "WARNING: Unable to open the file lefRWarning.log for writing from the
  directory %s.\n", getcwd(NULL, 64)); printf("Warning messages will not be
  printed.\n");
  }
  */

  // 4/11/2003 - Remove file lefrRWarning.log from directory if it exist
  // pcr 569729
  if (stat("lefRWarning.log", &statbuf) != -1) {
    // file exist, remove it
    if (!lefSettings->LogFileAppend)
      remove("lefRWarning.log");
  }

  // initialize the value
  leflVal = strtod("-2147483648", &lefch);
  lefrVal = strtod("2147483647", &lefch);
}

lefrData::~lefrData()
{
  // lef_lex_un_init()
  /* Close the file */
  if (lefrLog) {
    fclose(lefrLog);
    lefrLog = nullptr;
  }

  // destroyRingBuffer();
  for (int i = 0; i < RING_SIZE; i++) {
    free(ring[i]);
  }

  free(current_token);
  free(uc_token);
  free(pv_token);

  if (lefrAntennaPWLPtr) {
    lefrAntennaPWLPtr->Destroy();
    free(lefrAntennaPWLPtr);
  }
}

void lefrData::reset()
{
  if (lefData) {
    delete lefData;
  }

  lefData = new lefrData();
}

void lefrData::initRead()
{
  hasVer = 1;
  hasBusBit = 0;
  hasDirection = 0;
  hasDivChar = 0;
  hasFixedMask = 0;
  hasGeoLayer = 0;
  hasInfluence = 0;
  hasLayerMincut = 0;
  hasManufactur = 0;
  hasMask = 0;
  hasMinfeature = 0;
  hasNameCase = 0;
  hasOpenedLogFile = 0;
  hasPRP = 0;
  hasParallel = 0;
  hasPitch = 0;
  hasSamenet = 0;
  hasSite = 0;
  hasSiteClass = 0;
  hasSiteSize = 0;
  hasSpCenter = 0;
  hasSpLayer = 0;
  hasSpParallel = 0;
  hasSpSamenet = 0;
  hasTwoWidths = 0;
  hasType = 0;
  hasViaRule_layer = 0;
  hasWidth = 0;
}

END_LEF_PARSER_NAMESPACE
