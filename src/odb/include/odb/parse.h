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

#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "array1.h"
#include "utl/Logger.h"

namespace odb {

class Ath__parser
{
 public:
  Ath__parser(utl::Logger* logger);
  ~Ath__parser();
  void openFile(const char* name = nullptr);
  void setInputFP(FILE* fp);
  int mkWords(const char* word, const char* sep = nullptr);
  int readLineAndBreak(int prevWordCnt = -1);
  int parseNextLine();
  char* get(int ii);
  int getInt(int ii);
  int getInt(int n, int start);
  double getDouble(int ii);
  void getDoubleArray(Ath__array1D<double>* A, int start, double mult = 1.0);
  Ath__array1D<double>* readDoubleArray(const char* keyword, int start);
  void printWords(FILE* fp);

  int getWordCnt();
  char getFirstChar();

  void syntaxError(const char* msg);
  bool mkDir(char* word);
  int mkDirTree(const char* word, const char* sep);

  void resetSeparator(const char* s);
  void addSeparator(const char* s);
  bool isKeyword(int ii, const char* key1);
  void resetLineNum(int v);
  int getLineNum();
  bool isDigit(int ii, int jj);

 private:
  void init();
  void reportProgress();
  int mkWords(int jj);
  bool isSeparator(char a);

  char* _line;
  char* _tmpLine;
  char* _wordSeparators;
  char** _wordArray;
  char _commentChar;
  int _maxWordCnt;

  int _lineNum;
  int _currentWordCnt;
  int _lineSize;
  FILE* _inFP;
  char* _inputFile;

  int _progressLineChunk;
  utl::Logger* _logger;
};

}  // namespace odb
