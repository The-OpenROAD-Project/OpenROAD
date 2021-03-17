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

#include <stdarg.h>

#ifdef WIN32
#include <io.h>
#include <stdlib.h>
#else
#include <fcntl.h>
#include <unistd.h>
#endif

#define ADS_TM_BIND_HPP

#include <errno.h>

#include <string>

#include "ZNamespace.h"
#include "ZSession.h"
#include "db.h"
#include "dbLogger.h"

//#define TCL_EVAL

namespace odb {

Tcl_SavedResult savedResult[128];
int curSavedResult = 0;

ZInterface::ZInterface()
{
}

ZInterface::~ZInterface()
{
}

int ZInterface::event(const char* name,
                      const char* attr1,
                      const char* val1,
                      ...)
{
  const char* attr = attr1;
  const char* val = val1;
  std::string event_str;
  event_str = "zevent";
  event_str.append(" ");
  event_str.append(name);

  va_list args;
  va_start(args, val1);

  for (;;) {
    event_str.append(" ");
    event_str.append(attr);
    event_str.append(" {\"");
    event_str.append(val);
    event_str.append("\"}");

    attr = va_arg(args, const char*);

    if (attr == NULL)
      break;

    val = va_arg(args, const char*);

    if (val == NULL)
      error(0, "Missing value argument to ZInterface::event()");
  }

  va_end(args);
  // fprintf(stderr,"Event1: %s\n",event_str.c_str() );

  Tcl_SaveResult(_context._interp, &savedResult[curSavedResult++]);

#ifdef TCL_EVAL
  fprintf(stderr, "TCL EVAL 1: %s\n", (char*) event_str.c_str());
#endif
  int rc = Tcl_Eval(_context._interp, (char*) event_str.c_str());
#ifdef TCL_EVAL
  fprintf(stderr, "TCL EVAL 1 (%s) result: %d\n", event_str.c_str(), rc);
#endif
  if (rc != TCL_OK) {
#ifdef TCL_EVAL
    fprintf(stderr, "TCL EVAL 1b error branch!\n");
#endif
    if (strcasecmp(name, "error") == 0) {
#ifdef TCL_EVAL
      fprintf(stderr, "TCL EVAL 1b error branch 1!\n");
#endif
      error(0, "error");
    } else {
#ifdef TCL_EVAL
      fprintf(stderr, "TCL EVAL 1b error branch 2!\n");
#endif
      error(
          0, "TCL evaluation 1b failed on (%s - %d).\n", event_str.c_str(), rc);
    }
  }

  const char* result = Tcl_GetStringResult(_context._interp);
  int x = (int) strtol(result, NULL, 10);

  Tcl_RestoreResult(_context._interp, &savedResult[--curSavedResult]);

  return x;
}

int ZInterface::event(const char* name, const char* attr1, int type, ...)
{
  const char* attr = attr1;
  std::string event_str;
  event_str = "zevent";
  event_str.append(" ");
  event_str.append(name);

  va_list args;
  va_start(args, type);

  for (;;) {
    event_str.append(" ");
    event_str.append(attr);
    event_str.append(" {");

    switch (type) {
      case Z_CHAR:
      case Z_UCHAR:
      case Z_SHORT:
      case Z_USHORT:
      case Z_INT: {
        int val = va_arg(args, int);
        char buffer[16];
        snprintf(buffer, 16, "%d", val);
        event_str.append(buffer);
        break;
      }

      case Z_UINT: {
        unsigned int val = va_arg(args, unsigned int);
        char buffer[16];
        snprintf(buffer, 16, "%u", val);
        event_str.append(buffer);
        break;
      }

      case Z_FLOAT:
      case Z_DOUBLE: {
        double val = va_arg(args, double);
        char buffer[128];
        snprintf(buffer, 128, "%g", val);
        event_str.append(buffer);
        break;
      }

      case Z_STRING: {
        char* val = va_arg(args, char*);
        event_str.append(val);
        break;
      }

      case Z_BOOL: {
        int val = va_arg(args, int);

        if (val)
          event_str.append("1");
        else
          event_str.append("0");
        break;
      }

      case Z_ZOBJECT: {
        ZObject* val = va_arg(args, ZObject*);
        const char* name = _context._session->_ns->addZObject(val);
        event_str.append(name);
        break;
      }

      case Z_DBOBJECT: {
        dbObject* val = va_arg(args, dbObject*);
        char buffer[256];
        val->getDbName(buffer);
        event_str.append(buffer);
        break;
      }

      default:
        ZASSERT(0);
    }

    event_str.append("}");

    attr = va_arg(args, const char*);

    if (attr == NULL)
      break;

    type = (ZValueType) va_arg(args, int);
  }

  va_end(args);
  // fprintf(stderr,"Event2: %s\n",event_str.c_str() );

  Tcl_SaveResult(_context._interp, &savedResult[curSavedResult++]);

#ifdef TCL_EVAL
  fprintf(stderr, "TCL EVAL 2: %s\n", (char*) event_str.c_str());
#endif
  int rc = Tcl_Eval(_context._interp, (char*) event_str.c_str());
#ifdef TCL_EVAL
  fprintf(stderr, "TCL EVAL 2 result: %d\n", rc);
#endif
  if (rc != TCL_OK)
    error(0, "TCL evaluation 2 failed on (%s - %d).\n", event_str.c_str(), rc);

  const char* result = Tcl_GetStringResult(_context._interp);
  int x = (int) strtol(result, NULL, 10);

  Tcl_RestoreResult(_context._interp, &savedResult[--curSavedResult]);

  return x;
}

int ZInterface::idle(int level = 0)
{
  char idle_id[128];
  snprintf(idle_id, 128, "%s:%d", getModuleName(), level);
  return event("idle", idle_id, "", 0);
}

/* ********************************************************************
   ********************************************************************
                                  IMPORTANT!!!
             If you change the methods below, update logger.cpp to
              reflect the same behavior, or there will be trouble.
   ********************************************************************
   ******************************************************************** */

int ZInterface::milos(int code, const char* msg, ...)
{
  char buffer[8192];
  va_list args;
  va_start(args, msg);
  vsnprintf(buffer, 8192, msg, args);
  va_end(args);
  char milos_id[128];
  snprintf(milos_id, 128, "%s:%d", getModuleName(), code);
  return event("milos", milos_id, buffer, 0);
}

int ZInterface::ade(int code, const char* msg, ...)
{
  char buffer[8192];
  va_list args;
  va_start(args, msg);
  vsnprintf(buffer, 8192, msg, args);
  va_end(args);
  char ade_id[128];
  snprintf(ade_id, 128, "%s:%d", getModuleName(), code);
  return event("ade", ade_id, buffer, 0);
}

int ZInterface::warning(int code, const char* msg, ...)
{
  if (checkWarning(msg) == 1)
    return TCL_OK;

  char buffer[8192];
  va_list args;
  va_start(args, msg);
  vsnprintf(buffer, 8192, msg, args);
  va_end(args);
  char warning_id[128];
  snprintf(warning_id, 128, "%s:%d", getModuleName(), code);
  return event("warning", warning_id, buffer, 0);
}

int ZInterface::info(int code, const char* msg, ...)
{
  if (checkWarning(msg) == 1)
    return TCL_OK;

  char buffer[8192];
  va_list args;
  va_start(args, msg);
  vsnprintf(buffer, 8192, msg, args);
  va_end(args);
  char info_id[128];
  snprintf(info_id, 128, "%s:%d", getModuleName(), code);
  return event("info", info_id, buffer, 0);
}

int ZInterface::notice(int code, const char* msg, ...)
{
  char buffer[8192];
  va_list args;
  va_start(args, msg);
  vsnprintf(buffer, 8192, msg, args);
  va_end(args);
  char notice_id[128];
  snprintf(notice_id, 128, "%s:%d", getModuleName(), code);
  return event("notice", notice_id, buffer, 0);
}

int ZInterface::verbose(int code, const char* msg, ...)
{
  char buffer[8192];
  va_list args;
  va_start(args, msg);
  vsnprintf(buffer, 8192, msg, args);
  va_end(args);
  char verbose_id[128];
  snprintf(verbose_id, 128, "%s:%d", getModuleName(), code);
  return event("verbose", verbose_id, buffer, 0);
}

void ZInterface::error(int /* unused: code */, const char* msg, ...)
{
  char buffer[8192];
  va_list args;
  va_start(args, msg);
  vsnprintf(buffer, 8192, msg, args);
  va_end(args);
  // char error_id[128];
  // snprintf(error_id, 128, "%s:%d", getModuleName(), code );
  // event( "error", error_id, buffer, 0 );
  // debug("ERROR","A","error contents: %s\n",buffer);
  throw(ZException("%s", buffer));
}

}  // namespace odb
