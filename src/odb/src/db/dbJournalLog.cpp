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

dbJournalLog::dbJournalLog(utl::Logger* logger)
    : idx_(0), debug_(false), logger_(logger)
{
  debug_ = logger_->debugCheck(utl::ODB, "journal_check", 1);
}

void dbJournalLog::set_type(LogDataType type)
{
  if (debug_) {
    data_.push_back(static_cast<char>(type));
  }
}

void dbJournalLog::check_type(LogDataType expected_type)
{
  if (debug_) {
    const auto type = static_cast<LogDataType>(next());
    if (type != expected_type) {
      logger_->critical(utl::ODB,
                        426,
                        "In journal: expected type {} got {}.",
                        to_string(expected_type),
                        to_string(type));
    }
  }
}

void dbJournalLog::clear()
{
  data_.clear();
  idx_ = 0;
}

void dbJournalLog::push(bool value)
{
  set_type(LOG_BOOL);
  data_.push_back((value == true) ? 1 : 0);
}

void dbJournalLog::push(char value)
{
  set_type(LOG_CHAR);
  data_.push_back(value);
}

void dbJournalLog::push(unsigned char value)
{
  set_type(LOG_UCHAR);
  data_.push_back(value);
}

void dbJournalLog::push(int value)
{
  set_type(LOG_INT);
  unsigned char* v = (unsigned char*) &value;
  data_.push_back(v[0]);
  data_.push_back(v[1]);
  data_.push_back(v[2]);
  data_.push_back(v[3]);
}

void dbJournalLog::push(unsigned int value)
{
  set_type(LOG_UINT);
  unsigned char* v = (unsigned char*) &value;
  data_.push_back(v[0]);
  data_.push_back(v[1]);
  data_.push_back(v[2]);
  data_.push_back(v[3]);
}

void dbJournalLog::push(float value)
{
  set_type(LOG_FLOAT);
  unsigned char* v = (unsigned char*) &value;
  data_.push_back(v[0]);
  data_.push_back(v[1]);
  data_.push_back(v[2]);
  data_.push_back(v[3]);
}

void dbJournalLog::push(double value)
{
  set_type(LOG_DOUBLE);
  unsigned char* v = (unsigned char*) &value;
  data_.push_back(v[0]);
  data_.push_back(v[1]);
  data_.push_back(v[2]);
  data_.push_back(v[3]);
  data_.push_back(v[4]);
  data_.push_back(v[5]);
  data_.push_back(v[6]);
  data_.push_back(v[7]);
}

void dbJournalLog::push(const char* value)
{
  set_type(LOG_STRING);
  if (value == nullptr) {
    push(-1);
  } else {
    int len = strlen(value);
    push(len);

    for (; *value != '\0'; ++value) {
      data_.push_back(*value);
    }
  }
}

void dbJournalLog::moveBackOneInt()
{
  idx_ -= sizeof(uint);
  if (debug_) {
    --idx_;
  }
}

void dbJournalLog::moveToEnd()
{
  idx_ = size();
}

void dbJournalLog::pop(bool& value)
{
  check_type(LOG_BOOL);
  value = (next() == 1) ? true : false;
}

void dbJournalLog::pop(char& value)
{
  check_type(LOG_CHAR);
  value = next();
}

void dbJournalLog::pop(unsigned char& value)
{
  check_type(LOG_UCHAR);
  value = next();
}

void dbJournalLog::pop(int& value)
{
  check_type(LOG_INT);
  unsigned char* v = (unsigned char*) &value;
  v[0] = next();
  v[1] = next();
  v[2] = next();
  v[3] = next();
}

void dbJournalLog::pop(unsigned int& value)
{
  check_type(LOG_UINT);
  unsigned char* v = (unsigned char*) &value;
  v[0] = next();
  v[1] = next();
  v[2] = next();
  v[3] = next();
}

void dbJournalLog::pop(float& value)
{
  check_type(LOG_FLOAT);
  unsigned char* v = (unsigned char*) &value;
  v[0] = next();
  v[1] = next();
  v[2] = next();
  v[3] = next();
}

void dbJournalLog::pop(double& value)
{
  check_type(LOG_DOUBLE);
  unsigned char* v = (unsigned char*) &value;
  v[0] = next();
  v[1] = next();
  v[2] = next();
  v[3] = next();
  v[4] = next();
  v[5] = next();
  v[6] = next();
  v[7] = next();
}

void dbJournalLog::pop(char*& value)
{
  check_type(LOG_STRING);
  int len;
  pop(len);

  if (len == -1) {
    value = nullptr;
    return;
  }

  value = (char*) malloc(len + 1);

  int i;
  for (i = 0; i < len; ++i) {
    value[i] = next();
  }

  value[i] = '\0';
}

void dbJournalLog::pop(std::string& value)
{
  check_type(LOG_STRING);
  int len;
  pop(len);

  if (len == -1) {
    value = "";
    return;
  }

  value.reserve(len + 1);
  value = "";

  int i;
  for (i = 0; i < len; ++i) {
    value.push_back(next());
  }
}

/* static*/
std::string dbJournalLog::to_string(LogDataType type)
{
  switch (type) {
    case LOG_BOOL:
      return "BOOL";
    case LOG_CHAR:
      return "CHAR";
    case LOG_UCHAR:
      return "UCHAR";
    case LOG_INT:
      return "INT";
    case LOG_UINT:
      return "UINT";
    case LOG_FLOAT:
      return "FLOAT";
    case LOG_DOUBLE:
      return "DOUBLE";
    case LOG_STRING:
      return "STRING";
  }
  return fmt::format("UNKNOWN ({})", type);
}

dbIStream& operator>>(dbIStream& stream, dbJournalLog& log)
{
  bool debug;
  stream >> debug;
  if (debug != log.debug_) {
    log.logger_->error(utl::ODB, 427, "Journal debug mode mismatch.");
  }
  stream >> log.data_;
  return stream;
}

dbOStream& operator<<(dbOStream& stream, const dbJournalLog& log)
{
  stream << log.debug_;
  stream << log.data_;
  return stream;
}

}  // namespace odb
