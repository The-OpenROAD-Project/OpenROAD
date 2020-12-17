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

#include "ZException.h"
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

namespace odb {

ZException::ZException()
{
  _msg      = NULL;
  _free_msg = true;
}

ZException::ZException(const char* fmt, ...)
{
  char    buffer[8192];
  va_list args;
  va_start(args, fmt);
  vsnprintf(buffer, 8192, fmt, args);
  va_end(args);
  _msg = strdup(buffer);
  ZALLOCATED(_msg);
  _free_msg = true;
}

ZException::ZException(const ZException& ex)
{
  _msg = strdup(ex._msg);
  ZALLOCATED(_msg);
  _free_msg = true;
}

ZException::~ZException()
{
  if (_free_msg && _msg)
    free((void*) _msg);
}

ZIOError::ZIOError(int err)
{
  char buffer[8192];
  snprintf(buffer, 8192, "system io error (%s).", strerror(err));
  _msg = strdup(buffer);
  ZALLOCATED(_msg);
}

ZIOError::ZIOError(int err, const char* msg)
{
  char buffer[8192];
  snprintf(buffer, 8192, "%s (%s).", msg, strerror(err));
  _msg = strdup(buffer);
  ZALLOCATED(_msg);
}

ZAssert::ZAssert(const char* expr, const char* file, int line)
{
  char buffer[8192];
  snprintf(buffer, 8192, "assert(%s) in %s at %d", expr, file, line);
  _msg = strdup(buffer);
  ZALLOCATED(_msg);
}

}  // namespace odb
