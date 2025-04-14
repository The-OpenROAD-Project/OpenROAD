// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include <string>

#include "dbPagedVector.h"
#include "odb/odb.h"

namespace odb {

class dbIStream;
class dbOStream;

class dbJournalLog
{
 public:
  dbJournalLog(utl::Logger* logger);

  void clear();
  bool empty() const { return data_.size() == 0; }

  uint idx() const { return idx_; }
  uint size() const { return data_.size(); }
  void push(bool value);
  void push(char value);
  void push(unsigned char value);
  void push(int value);
  void push(unsigned int value);
  void push(float value);
  void push(double value);
  void push(const char* value);

  void begin() { idx_ = 0; }
  bool end() { return idx_ == (int) data_.size(); }
  void set(uint idx) { idx_ = idx; }
  void moveBackOneInt();
  void moveToEnd();

  void pop(bool& value);
  void pop(char& value);
  void pop(unsigned char& value);
  void pop(int& value);
  void pop(unsigned int& value);
  void pop(float& value);
  void pop(double& value);
  void pop(char*& value);
  void pop(std::string& value);

 private:
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

  void set_type(LogDataType type);
  void check_type(LogDataType expected_type);
  unsigned char next() { return data_[idx_++]; }

  friend dbIStream& operator>>(dbIStream& stream, dbJournalLog& log);
  friend dbOStream& operator<<(dbOStream& stream, const dbJournalLog& log);

  static std::string to_string(LogDataType type);

  dbPagedVector<unsigned char> data_;
  int idx_;
  bool debug_;
  utl::Logger* logger_;
};

}  // namespace odb
