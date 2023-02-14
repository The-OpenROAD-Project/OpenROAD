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
