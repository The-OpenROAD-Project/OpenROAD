///////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// Copyright (c) 2018-2020, The Regents of the University of California
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
// ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
// SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
// CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
// OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
///////////////////////////////////////////////////////////////////////////////

#include "openroad/Error.hh"
#include "logger.h"
#include <iostream>
#include <cstdlib>

using std::cout;
using std::endl;
using std::string;

#define VERBOSE_CHECK() if( verbose_ < verbose ) { return; };

namespace replace {

Logger::Logger(string name, int verbose)
  : name_(name), verbose_(verbose) {}

// Procedure message
void Logger::proc(string input, int verbose) {
  VERBOSE_CHECK()
  cout << "[PROC] " << input << endl;
}
void Logger::procBegin(string input, int verbose) {
  VERBOSE_CHECK()
  cout << "[PROC] Begin " << input << " ..." << endl;
}
void Logger::procEnd(string input, int verbose) {
  VERBOSE_CHECK()
  cout << "[PROC] End " << input << endl;
}

// Error message
void Logger::error(string input, int code, int verbose) {
  VERBOSE_CHECK()
  cout << "[ERROR] " << input;
  cout << " (" << name_ << "-" << code << ")" << endl;
}

void Logger::errorQuit(string input, int code, int verbose) {
  error(input, code, verbose);
  ord::error("RePlAce terminated with errors.");
}

void Logger::warn(string input, int code, int verbose) {
  VERBOSE_CHECK()
  cout << "[WARN] " << input;
  cout << " (" << name_ << "-" << code << ")" << endl;
}

// Info message
void Logger::infoInt(string input, int val, int verbose) {
  VERBOSE_CHECK()
  cout << "[INFO] " << input << " = " << val << endl;
}
void Logger::infoIntPair(string input, int val1, int val2, int verbose) {
  VERBOSE_CHECK()
  cout << "[INFO] " << input << " = (" << val1 << ", " << val2 << ")" << endl;
}

void Logger::infoInt64(string input, int64_t val, int verbose) {
  VERBOSE_CHECK()
  cout << "[INFO] " << input << " = " << val << endl;
}


void Logger::infoFloat(string input, float val, int verbose) {
  VERBOSE_CHECK()
  printf("[INFO] %s = %.6f\n", input.c_str(), val);
  fflush(stdout);
}
// SI format due to WNS/TNS
void Logger::infoFloatSignificant(string input, float val, int verbose) {
  VERBOSE_CHECK()
  printf("[INFO] %s = %g\n", input.c_str(), val);
  fflush(stdout);
}

void Logger::infoFloatPair(string input, float val1, float val2, int verbose) {
  VERBOSE_CHECK()
  printf("[INFO] %s = (%.6f, %.6f)\n", input.c_str(), val1, val2);
  fflush(stdout);
}
void Logger::infoString(string input, int verbose) {
  VERBOSE_CHECK()
  cout << "[INFO] " << input << endl;
}
void Logger::infoString(string input, string val, int verbose) {
  VERBOSE_CHECK()
  cout << "[INFO] " << input << " = " << val << endl;
}
void Logger::infoRuntime(string input, double runtime, int verbose) {
  VERBOSE_CHECK()
  printf("[INFO] %sRuntime = %.4f\n", input.c_str(), runtime);
  fflush(stdout);
}

}
