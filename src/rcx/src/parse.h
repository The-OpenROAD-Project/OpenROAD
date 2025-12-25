// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "rcx/array1.h"
#include "utl/Logger.h"

namespace rcx {

class Parser
{
 public:
  Parser(utl::Logger* logger);
  ~Parser();
  void openFile(const char* name = nullptr);
  void setInputFP(FILE* fp);
  int mkWords(const char* word, const char* sep = nullptr);
  int readLineAndBreak(int prevWordCnt = -1);
  int parseNextLine();
  char* get(int ii);
  int getInt(int ii);
  int getInt(int n, int start);
  double getDouble(int ii);
  void getDoubleArray(Array1D<double>* A, int start, double mult = 1.0);
  Array1D<double>* readDoubleArray(const char* keyword, int start);
  void printWords(FILE* fp);

  int getWordCnt();
  char getFirstChar();

  void syntaxError(const char* msg);
  bool mkDir(const char* word);
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

  int _lineNum;
  int _currentWordCnt;
  FILE* _inFP;
  char* _inputFile;

  utl::Logger* _logger;

  static constexpr int _progressLineChunk = 1000000;
  static constexpr char _commentChar = '#';
  static constexpr int _maxWordCnt = 100;
  static constexpr int _lineSize = 10000;
};

}  // namespace rcx
