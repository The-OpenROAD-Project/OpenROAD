// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "odb/array1.h"
#include "utl/Logger.h"

namespace rcx {

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
  void getDoubleArray(odb::Ath__array1D<double>* A,
                      int start,
                      double mult = 1.0);
  odb::Ath__array1D<double>* readDoubleArray(const char* keyword, int start);
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

}  // namespace rcx
