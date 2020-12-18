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

#pragma once

#include "odb.h"

#ifdef __GNUC__
#define ADS_FORMAT_PRINTF(F, A) __attribute__((format(printf, F, A)))
#else
#define ADS_FORMAT_PRINTF(F, A)
#endif

namespace odb {

/////////////////////////////////
// Base exception class
/////////////////////////////////
class ZException
{
 public:
  const char* _msg;
  bool        _free_msg;

  ZException();
  ZException(const char* fmt, ...) ADS_FORMAT_PRINTF(2, 3);
  ZException(const ZException& ex);
  ~ZException();
};

class ZOutOfMemory : public ZException
{
 public:
  ZOutOfMemory()
  {
    _free_msg = false;
    _msg      = "Out of memory";
  }
};

class ZIOError : public ZException
{
 public:
  ZIOError(int err);
  ZIOError(int err, const char* msg);
};

class ZAssert : public ZException
{
 public:
  ZAssert(const char* expr, const char* file, int line);
};

// See http://cnicholson.net/2009/02/stupid-c-tricks-adventures-in-assert
// Avoids unused variable warnings.
#ifdef NDEBUG
    #define ZASSERT(x) do { (void)sizeof(x); } while(0)  
#else  
    #define ZASSERT(x) assert(x)
#endif  
  
#define ZALLOCATED(expr)    \
  do {                      \
    if ((expr) == NULL)     \
      throw ZOutOfMemory(); \
  } while (0);

}  // namespace odb


