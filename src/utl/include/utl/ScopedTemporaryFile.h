///////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// Copyright (c) 2023, Google LLC
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

#include <climits>
#include <cstdio>
#include <fstream>

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

class StreamHandler
{
 public:
  // Set binary to true to open in binary mode
  StreamHandler(const char* filename, bool binary = false);
  ~StreamHandler();
  std::ofstream& getStream();

 private:
  std::string filename_;
  std::string tmp_filename_;
  std::ofstream os_;
};

class FileHandler
{
 public:
  // Set binary to true to open in binary mode
  FileHandler(const char* filename, bool binary = false);
  ~FileHandler();
  FILE* getFile();

 private:
  std::string filename_;
  std::string tmp_filename_;
  FILE* file_;
};

}  // namespace utl
