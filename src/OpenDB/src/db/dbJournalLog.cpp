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

#include "dbJournalLog.h"

#include <string>

namespace odb {

#define DEBUG_JOURNAL_LOG

#ifdef DEBUG_JOURNAL_LOG
#define SET_TYPE(TYPE) _data.push_back((char) TYPE)
#define CHECK_TYPE(TYPE)                   \
  LogDataType type = (LogDataType) next(); \
  ZASSERT(type == TYPE)
#else
#define SET_TYPE(TYPE)
#define CHECK_TYPE(TYPE)
#endif

enum LogDataType
{
  LOG_BOOL,
  LOG_CHAR,
  LOG_UCHAR,
  LOG_INT,
  LOG_UINT,
  LOG_FLOAT,
  LOG_DOUBLE,
  LOG_STRING
};

dbJournalLog::dbJournalLog() : _idx(0), _debug(0)
{
#ifdef DEBUG_JOURNAL_LOG
  _debug = 1;
#endif
}

dbJournalLog::~dbJournalLog()
{
  clear();
}

void dbJournalLog::push(bool value)
{
  SET_TYPE(LOG_BOOL);
  _data.push_back((value == true) ? 1 : 0);
}

void dbJournalLog::push(char value)
{
  SET_TYPE(LOG_CHAR);
  _data.push_back(value);
}

void dbJournalLog::push(unsigned char value)
{
  SET_TYPE(LOG_UCHAR);
  _data.push_back(value);
}

void dbJournalLog::push(int value)
{
  SET_TYPE(LOG_INT);
  unsigned char* v = (unsigned char*) &value;
  _data.push_back(v[0]);
  _data.push_back(v[1]);
  _data.push_back(v[2]);
  _data.push_back(v[3]);
}

void dbJournalLog::push(unsigned int value)
{
  SET_TYPE(LOG_UINT);
  unsigned char* v = (unsigned char*) &value;
  _data.push_back(v[0]);
  _data.push_back(v[1]);
  _data.push_back(v[2]);
  _data.push_back(v[3]);
}

void dbJournalLog::push(float value)
{
  SET_TYPE(LOG_FLOAT);
  unsigned char* v = (unsigned char*) &value;
  _data.push_back(v[0]);
  _data.push_back(v[1]);
  _data.push_back(v[2]);
  _data.push_back(v[3]);
}

void dbJournalLog::push(double value)
{
  SET_TYPE(LOG_DOUBLE);
  unsigned char* v = (unsigned char*) &value;
  _data.push_back(v[0]);
  _data.push_back(v[1]);
  _data.push_back(v[2]);
  _data.push_back(v[3]);
  _data.push_back(v[4]);
  _data.push_back(v[5]);
  _data.push_back(v[6]);
  _data.push_back(v[7]);
}

void dbJournalLog::push(const char* value)
{
  SET_TYPE(LOG_STRING);
  if (value == NULL)
    push(-1);
  else {
    int len = strlen(value);
    push(len);

    for (; *value != '\0'; ++value)
      _data.push_back(*value);
  }
}

void dbJournalLog::pop(bool& value)
{
  CHECK_TYPE(LOG_BOOL);
  value = (next() == 1) ? true : false;
}

void dbJournalLog::pop(char& value)
{
  CHECK_TYPE(LOG_CHAR);
  value = next();
}

void dbJournalLog::pop(unsigned char& value)
{
  CHECK_TYPE(LOG_UCHAR);
  value = next();
}

void dbJournalLog::pop(int& value)
{
  CHECK_TYPE(LOG_INT);
  unsigned char* v = (unsigned char*) &value;
  v[0]             = next();
  v[1]             = next();
  v[2]             = next();
  v[3]             = next();
}

void dbJournalLog::pop(unsigned int& value)
{
  CHECK_TYPE(LOG_UINT);
  unsigned char* v = (unsigned char*) &value;
  v[0]             = next();
  v[1]             = next();
  v[2]             = next();
  v[3]             = next();
}

void dbJournalLog::pop(float& value)
{
  CHECK_TYPE(LOG_FLOAT);
  unsigned char* v = (unsigned char*) &value;
  v[0]             = next();
  v[1]             = next();
  v[2]             = next();
  v[3]             = next();
}

void dbJournalLog::pop(double& value)
{
  CHECK_TYPE(LOG_DOUBLE);
  unsigned char* v = (unsigned char*) &value;
  v[0]             = next();
  v[1]             = next();
  v[2]             = next();
  v[3]             = next();
  v[4]             = next();
  v[5]             = next();
  v[6]             = next();
  v[7]             = next();
}

void dbJournalLog::pop(char*& value)
{
  CHECK_TYPE(LOG_STRING);
  int len;
  pop(len);

  if (len == -1) {
    value = NULL;
    return;
  }

  value = (char*) malloc(len + 1);

  int i;
  for (i = 0; i < len; ++i)
    value[i] = next();

  value[i] = '\0';
}

void dbJournalLog::pop(std::string& value)
{
  CHECK_TYPE(LOG_STRING);
  int len;
  pop(len);

  if (len == -1) {
    value = "";
    return;
  }

  value.reserve(len + 1);
  value = "";

  int i;
  for (i = 0; i < len; ++i)
#ifndef _WIN32
    value.push_back(next());
#else
    value += next();
#endif
}

dbIStream& operator>>(dbIStream& stream, dbJournalLog& log)
{
  uint debug;
  stream >> debug;
  assert((int) debug == log._debug);  // debug mismatch
  stream >> log._data;
  return stream;
}

dbOStream& operator<<(dbOStream& stream, const dbJournalLog& log)
{
  stream << log._debug;
  stream << log._data;
  return stream;
}

}  // namespace odb
