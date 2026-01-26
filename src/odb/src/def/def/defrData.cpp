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

#include "defrData.hpp"

#include <string.h>  // NOLINT(modernize-deprecated-headers): for strdup()

#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "defiKRDefs.hpp"
#include "defrCallBacks.hpp"
#include "defrSettings.hpp"

BEGIN_DEF_PARSER_NAMESPACE

defrData::defrData(const defrCallbacks* pCallbacks,
                   const defrSettings* pSettings,
                   defrSession* pSession)
    : specialWire_routeStatus((char*) "ROUTED"),
      specialWire_routeStatusName((char*) ""),
      specialWire_shapeType((char*) ""),
      // defrReader vars
      PathObj(this),
      Prop(this),
      Site(this),
      Component(this),
      ComponentMaskShiftLayer(this),
      Net(this),
      PinCap(),
      CannotOccupy(this),
      Canplace(this),
      Pin(this),
      Row(this),
      Track(this),
      GcellGrid(this),
      Via(this),
      Region(this),
      Group(this),
      Assertion(this),
      Scanchain(this),
      IOTiming(this),
      FPC(this),
      TimingDisable(this),
      Partition(this),
      PinProp(this),
      Blockage(this),
      Slot(this),
      Fill(this),
      NonDefault(this),
      Geometries(this),
      session(pSession),
      settings(pSettings),
      magic((char*) malloc(1)),
      deftoken((char*) malloc(TOKEN_SIZE)),
      uc_token((char*) malloc(TOKEN_SIZE)),
      pv_deftoken((char*) malloc(TOKEN_SIZE)),
      callbacks(pCallbacks)
{
  magic[0] = '\0';
  deftoken[0] = '\0';
  History_text.push_back('\0');

  memset(msgLimit, 0, DEF_MSGS * sizeof(int));
  memset(buffer, 0, IN_BUF_SIZE * sizeof(char));
  memset(ring, 0, RING_SIZE * sizeof(char*));
  memset(ringSizes, 0, RING_SIZE * sizeof(int));
  memset(lineBuffer, 0, MSG_SIZE * sizeof(char));

  // initRingBuffer
  int i;
  ringPlace = 0;
  for (i = 0; i < RING_SIZE; i++) {
    ring[i] = (char*) malloc(TOKEN_SIZE);
    ringSizes[i] = TOKEN_SIZE;
  }

  nlines = 1;
  next = buffer;
  first_buffer = 1;

  lVal = strtod("-2147483648", nullptr);
  rVal = strtod("2147483647", nullptr);
}

defrData::~defrData()
{
  // lex_un_init.
  /* Close the file */
  if (defrLog) {
    fclose(defrLog);
    defrLog = nullptr;
  }

  free(deftoken);
  free(uc_token);
  free(pv_deftoken);
  free(magic);

  // freeRingBuffer.
  int i;

  ringPlace = 0;
  for (i = 0; i < RING_SIZE; i++) {
    free(ring[i]);
  }
}

void defrData::defiError(int check, int msgNum, const char* mess)
{
  /* check is 1 if the caller function has checked totalMsgLimit, etc. */

  if (!check) {
    if ((settings->totalDefMsgLimit > 0)
        && (defMsgPrinted >= settings->totalDefMsgLimit)) {
      return;
    }
    if (settings->MsgLimit[msgNum - 5000] > 0) {
      if (msgLimit[msgNum - 5000] >= settings->MsgLimit[msgNum - 5000]) {
        return; /*over the limit*/
      }
      msgLimit[msgNum - 5000] = msgLimit[msgNum - 5000] + 1;
    }
    defMsgPrinted++;
  }

  if (settings->ContextErrorLogFunction) {
    (*(settings->ContextErrorLogFunction))(session->UserData, mess);
  } else if (settings->ErrorLogFunction) {
    (*(settings->ErrorLogFunction))(mess);
  } else {
    fprintf(stderr, "%s", mess);
  }
}

const char* defrData::upperCase(const char* str)
{
  const static char defiShift[]
      = {'\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',
         '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',
         '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', ' ',  '!',  '"',  '#',
         '$',  '%',  '&',  '\'', '(',  ')',  '*',  '+',  ',',  '-',  '.',  '/',
         '0',  '1',  '2',  '3',  '4',  '5',  '6',  '7',  '8',  '9',  ':',  ';',
         '<',  '=',  '>',  '?',  '@',  'A',  'B',  'C',  'D',  'E',  'F',  'G',
         'H',  'I',  'J',  'K',  'L',  'M',  'N',  'O',  'P',  'Q',  'R',  'S',
         'T',  'U',  'V',  'W',  'X',  'Y',  'Z',  '[',  '\\', ']',  '^',  '_',
         '`',  'A',  'B',  'C',  'D',  'E',  'F',  'G',  'H',  'I',  'J',  'K',
         'l',  'M',  'N',  'O',  'P',  'Q',  'R',  'S',  'T',  'U',  'V',  'W',
         'X',  'Y',  'Z',  '{',  '|',  '}',  '~',  '\0', '\0', '\0', '\0', '\0',
         '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',
         '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',
         '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',
         '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',
         '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',
         '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',
         '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',
         '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',
         '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',
         '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',
         '\0', '\0', '\0', '\0'};

  char* place = (char*) str;
  char* to;
  int len = strlen(str) + 1;

  if (len > shiftBufLength) {
    if (shiftBuf == nullptr) {
      len = len < 64 ? 64 : len;
      shiftBuf = (char*) malloc(len);
      shiftBufLength = len;
    } else {
      free(shiftBuf);
      shiftBuf = (char*) malloc(len);
      shiftBufLength = len;
    }
  }

  to = shiftBuf;
  while (*place) {
    int i = static_cast<unsigned char>(*place);
    place++;
    *to++ = defiShift[i];
  }
  *to = '\0';

  return shiftBuf;
}

int defrData::validateMaskInput(int input,
                                int warningIndex,
                                int getWarningsIndex)
{
  if (VersionNum < 5.8 && input > 0) {
    if (warningIndex++ < getWarningsIndex) {
      defMsg = (char*) malloc(1000);
      sprintf(defMsg,
              "The MASK statement is available in version 5.8 and "
              "later.\nHowever, your DEF file is defined with version %g",
              VersionNum);
      defError(7415, defMsg);
      free(defMsg);
      if (checkErrors()) {
        return 1;
      }
      return 0;
    }
  }

  return 1;
}

int defrData::validateMaskShiftInput(const char* shiftMask,
                                     int warningIndex,
                                     int getWarningsIndex)
{
  int shiftMaskLength = strlen(shiftMask);
  int hasShiftData = 0;
  int hasError = 0;

  // Verification of the mask string
  for (int i = 0; i < shiftMaskLength; i++) {
    int curShift = static_cast<unsigned char>(shiftMask[i]);

    if (curShift < '0' || curShift > '9') {
      hasError = 1;
    }

    if (curShift > '0') {
      hasShiftData = 1;
    }
  }

  if (hasError) {
    char* msg = (char*) malloc(1000);

    sprintf(msg,
            "The MASKSHIFT value '%s' is not valid. The value should be a "
            "string consisting of decimal digits ('0' - '9').",
            shiftMask);
    defError(7416, msg);
    free(msg);

    if (checkErrors()) {
      return 1;
    }

    return 0;
  }

  if (VersionNum < 5.8 && hasShiftData) {
    if (warningIndex++ < getWarningsIndex) {
      char* msg = (char*) malloc(1000);

      sprintf(msg,
              "The MASKSHIFT statement can be used only in DEF version 5.8 and "
              "later. This DEF file version is '%g'.",
              VersionNum);
      defError(7417, msg);
      free(msg);
      if (checkErrors()) {
        return 1;
      }
    }

    return 0;
  }

  return 1;
}

double defrData::convert_defname2num(char* versionName)
{
  char majorNm[80];
  char minorNm[80];
  char* subMinorNm = nullptr;
  char* versionNm = strdup(versionName);

  double major = 0, minor = 0, subMinor = 0;
  double version;

  sscanf(versionNm, "%[^.].%s", majorNm, minorNm);

  char* p1 = strchr(minorNm, '.');
  if (p1) {
    subMinorNm = p1 + 1;
    *p1 = '\0';
  }
  major = atof(majorNm);
  minor = atof(minorNm);
  if (subMinorNm) {
    subMinor = atof(subMinorNm);
  }

  version = major;

  if (minor > 0) {
    version = major + minor / 10;
  }

  if (subMinor > 0) {
    version = version + subMinor / 1000;
  }

  free(versionNm);
  return version;
}

int defrData::numIsInt(char* volt)
{
  return strchr(volt, '.') == nullptr;  // a floating point
}

int defrData::defValidNum(int values)
{
  char* outMsg;
  switch (values) {
    case 100:
    case 200:
    case 1000:
    case 2000:
      return 1;
    case 400:
    case 800:
    case 4000:
    case 8000:
    case 10000:
    case 20000:
      if (VersionNum < 5.6) {
        if (callbacks->UnitsCbk) {
          if (unitsWarnings++ < settings->UnitsWarnings) {
            outMsg = (char*) malloc(1000);
            sprintf(outMsg,
                    "An error has been found while processing the DEF file "
                    "'%s'\nUnit %d is a 5.6 or later syntax. Define the DEF "
                    "file as 5.6 and then try again.",
                    session->FileName,
                    values);
            defError(6501, outMsg);
            free(outMsg);
          }
        }

        return 0;
      } else {
        return 1;
      }
  }
  if (callbacks->UnitsCbk) {
    if (unitsWarnings++ < settings->UnitsWarnings) {
      outMsg = (char*) malloc(10000);
      sprintf(
          outMsg,
          "The value %d defined for DEF UNITS DISTANCE MICRON is invalid\n. "
          "The valid values are 100, 200, 400, 800, 1000, 2000, 4000, 8000, "
          "10000, or 20000. Specify a valid value and then try again.",
          values);
      defError(6502, outMsg);
      free(outMsg);
      if (checkErrors()) {
        return 1;
      }
    }
  }
  return 0;
}

defrContext::defrContext(int ownConf) : ownConfig(ownConf)
{
}

defrContext defContext;

END_DEF_PARSER_NAMESPACE
