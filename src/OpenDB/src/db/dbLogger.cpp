///////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// Copyright (c) 2019, Nefelus Inc
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// * Redistributions of source code must retain the above copyright notice, this
//   list of conditions and the following disclaimer.
//
// * Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
//
// * Neither the name of the copyright holder nor the names of its
//   contributors may be used to endorse or promote products derived from
//   this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.

#ifndef WIN32
#include <string.h>
#include <sys/time.h>
#include <unistd.h>
#endif
#include <signal.h>
#include <time.h>

#include "dbLogger.h"

namespace odb {
static char _ath_logbuffer[1024 * 8];

typedef struct deb_rec
{
  char* mod;
  char* except;
  char* only;
} DEBREC;

#define NUMDEBMOD 64
DEBREC debstatus[NUMDEBMOD];
int debcnt = 0;

#define MAX_WARN_STR 4096
#define WARN_CNT 50
typedef struct warn_rec
{
  char* msg;
  int cnt;
  int maxcnt;
} WARNREC;
WARNREC warnstr[MAX_WARN_STR];

int warncnt = 0;

char last_failed_mod[1024] = "";

int idle(int level)
{
  return TCL_OK;
}

int isDebug(const char* mod, const char* tag)
{
  // see if message should be sent
  static int last_idx = -1;

  if (mod != NULL) {
    if ((last_failed_mod[0] != '\0')
        && (strcmp(mod, (char*) last_failed_mod) == 0)) {
      return 0;
    }

    if (tag == NULL) {
      return 0;
    }

    bool flag = false;
    int idx = -1;

    if (last_idx != -1) {
      if (strcmp(debstatus[last_idx].mod, mod) == 0) {
        idx = last_idx;
      }
    }

    if (idx == -1) {
      // find the module
      for (int i = 0; i < debcnt; i++) {
        if (strcmp(debstatus[i].mod, mod) == 0) {
          flag = true;
          idx = i;
          last_idx = idx;
          last_failed_mod[0] = '\0';
          break;
        }
      }
      if (flag == false) {
        // module non-null, not found for debugging
        strcpy(last_failed_mod, mod);
        return 0;
      }
    }

    // check for actual debugging..
    flag = false;
    int tlen = strlen(tag);
    int olen = strlen(debstatus[idx].only);

    // check if debugging was set
    if (olen > 0) {
      // see if there's a tag match
      for (int i = 0; i < tlen; i++) {
        for (int j = 0; j < olen; j++) {
          if (tag[i] == debstatus[idx].only[j]) {
            flag = true;
            break;
          }
        }
        if (flag == true)
          break;
      }
      if (flag == false)
        return 0;
    } else {
      // no "only" setup, see if in except list
      int elen = strlen(debstatus[idx].except);

      // check for total reset
      if (elen == 1 && debstatus[idx].except[0] == '*')
        return 0;

      for (int i = 0; i < tlen; i++) {
        for (int j = 0; j < elen; j++) {
          if (tag[i] == debstatus[idx].except[j]) {
            return 0;
          }
        }
      }
    }

    return 1;
  } else {
    return 1;
  }
}

int debug(const char* mod, const char* tag, const char* msg, ...)
{
  // fprintf(stderr,"Debug message: %s-%s-%s\n",mod,tag,msg);
  if (isDebug(mod, tag) == 1) {
    // if we get to here, we're good to send

    va_list args;
    va_start(args, msg);

    vsnprintf(_ath_logbuffer, sizeof(_ath_logbuffer), msg, args);
    va_end(args);

    return fprintf(stderr, "Debug %s:%s %s", mod, tag, _ath_logbuffer);
  } else {
    return TCL_OK;
  }
}

int checkWarning(const char* msg)
{
  // dumpWarn();
  static int last_idx = -1;

  int idx = -1;

  if (last_idx != -1) {
    if (strcmp(warnstr[last_idx].msg, msg) == 0) {
      idx = last_idx;
    }
  }

  if (idx == -1) {
    for (int i = 0; i < warncnt; i++) {
      if (strcmp(msg, warnstr[i].msg) == 0) {
        idx = i;
        last_idx = idx;
        break;
      }
    }
    if (idx == -1) {
      if (warncnt == MAX_WARN_STR) {
        notice(0,
               "Not enough space to store warning signature -"
               " increase MAX_WARN_STR from %d\n",
               MAX_WARN_STR);
        return 0;
      }
      // not found - first time for this message
      warnstr[warncnt].msg = strdup(msg);
      warnstr[warncnt].cnt = 1;
      warnstr[warncnt].maxcnt = WARN_CNT;

      idx = warncnt;
      warncnt++;
    } else {
      warnstr[idx].cnt++;
    }
  } else {
    warnstr[idx].cnt++;
  }
  // dumpWarn();

  if (warnstr[idx].cnt > warnstr[idx].maxcnt) {
    // exceeded our maximum - don't print
    return 1;
  }
  return 0;
}

/* ********************************************************************
   ********************************************************************
                                  IMPORTANT!!!
               If you change the methods below, update tm.cpp to
              reflect the same behavior, or there will be trouble.
   ********************************************************************
   ******************************************************************** */

int milos(int code, const char* msg, ...)
{
  va_list args;
  va_start(args, msg);

  vsnprintf(_ath_logbuffer, sizeof(_ath_logbuffer), msg, args);
  va_end(args);

  return fprintf(stderr, "Milos %d: %s", code, _ath_logbuffer);
}

int ade(int code, const char* msg, ...)
{
  va_list args;
  va_start(args, msg);

  vsnprintf(_ath_logbuffer, sizeof(_ath_logbuffer), msg, args);
  va_end(args);

  if (fprintf(stderr, "ADE %d: %s", code, _ath_logbuffer) >= 0)
    return TCL_OK;
  else
    return TCL_ERROR;
}

int notice(int code, const char* msg, ...)
{
  va_list args;
  va_start(args, msg);

  vsnprintf(_ath_logbuffer, sizeof(_ath_logbuffer), msg, args);
  va_end(args);

  if (fprintf(stderr, "Notice %d: %s", code, _ath_logbuffer) >= 0)
    return TCL_OK;
  else
    return TCL_ERROR;
}

int verbose(int code, const char* msg, ...)
{
  va_list args;
  va_start(args, msg);

  vsnprintf(_ath_logbuffer, sizeof(_ath_logbuffer), msg, args);
  va_end(args);

  if (fprintf(stderr, "Verbose %d: %s", code, _ath_logbuffer) >= 0)
    return TCL_OK;
  else
    return TCL_ERROR;
}

int info(int code, const char* msg, ...)
{
  if (checkWarning(msg) == 1)
    return TCL_OK;

  va_list args;
  va_start(args, msg);

  vsnprintf(_ath_logbuffer, sizeof(_ath_logbuffer), msg, args);
  va_end(args);

  if (fprintf(stderr, "Info %d: %s", code, _ath_logbuffer) >= 0)
    return TCL_OK;
  else
    return TCL_ERROR;
}

int warning(int code, const char* msg, ...)
{
  if (checkWarning(msg) == 1)
    return TCL_OK;

  va_list args;
  va_start(args, msg);

  vsnprintf(_ath_logbuffer, sizeof(_ath_logbuffer), msg, args);
  va_end(args);

  if (fprintf(stderr, "Warning %d: %s", code, _ath_logbuffer) >= 0)
    return TCL_OK;
  else
    return TCL_ERROR;
}

void error(int code, const char* msg, ...)
{
  va_list args;
  va_start(args, msg);

  vsnprintf(_ath_logbuffer, sizeof(_ath_logbuffer), msg, args);
  va_end(args);

  fprintf(stderr, "Error %d: %s", code, _ath_logbuffer);
}

}  // namespace odb
