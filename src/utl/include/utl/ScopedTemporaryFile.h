// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023-2025, The OpenROAD Authors

#pragma once

#include <climits>
#include <cstdio>
#include <fstream>
#include <istream>
#include <memory>
#include <ostream>
#include <string>

#include "boost/iostreams/filtering_streambuf.hpp"
#include "utl/Logger.h"

namespace utl {

// Creates a temporary file that is unlinked/closed at the end of the object's
// lifetime scope.
//
// This is particularly useful in testing.
//
// NOTE: C-style `FILE*` constructs are not recommended for use in new code,
// prefer C++ iostreams for any new code.
class ScopedTemporaryFile
{
 public:
  explicit ScopedTemporaryFile(Logger* logger);

  ~ScopedTemporaryFile();

  // See class-level note: c-style `FILE*` usage is deprecated for any new code
  // introduced in OpenROAD. Prefer other temporary-file-creating constructs
  // that are based on iostreams for novel code.
  FILE* file() const { return file_; }

 private:
  Logger* logger_;
  char path_[PATH_MAX] = {0};
  int fd_ = -1;
  FILE* file_;
};

class OutStreamHandler
{
 public:
  // Set binary to true to open in binary mode
  OutStreamHandler(const char* filename, bool binary = false);
  ~OutStreamHandler();
  std::ostream& getStream();
  void close();

 private:
  std::string filename_;
  std::string tmp_filename_;
  std::ofstream os_;

  std::unique_ptr<boost::iostreams::filtering_ostreambuf> buf_;
  std::unique_ptr<std::ostream> stream_;
};

class InStreamHandler
{
 public:
  // Set binary to true to open in binary mode
  InStreamHandler(const char* filename, bool binary = false);
  ~InStreamHandler();
  std::istream& getStream();
  void close();

 private:
  std::string filename_;
  std::ifstream is_;

  std::unique_ptr<boost::iostreams::filtering_istreambuf> buf_;
  std::unique_ptr<std::istream> stream_;
};

class FileHandler
{
 public:
  // Set binary to true to open in binary mode
  FileHandler(const char* filename, bool binary = false);
  ~FileHandler();
  FILE* getFile();
  void close();

 private:
  std::string filename_;
  std::string tmp_filename_;
  FILE* file_;
};

}  // namespace utl
