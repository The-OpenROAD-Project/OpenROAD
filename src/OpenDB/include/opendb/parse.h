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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "array1.h"

class Ath__parser
{
 private:
  char*  _line;
  char*  _tmpLine;
  char*  _wordSeparators;
  char** _wordArray;
  char   _commentChar;
  int    _maxWordCnt;
  int    _wordSize;

  int   _lineNum;
  int   _currentWordCnt;
  int   _lineSize;
  FILE* _inFP;
  FILE* _dbgFP;
  char* _inputFile;

  int _dbg;
  int _progressLineChunk;

 public:
  Ath__parser();
  Ath__parser(int lSize, int wCnt, int wSize);
  void init();
  void keepLine();
  void getTmpLine();
  ~Ath__parser();
  void   openFile(char* name = NULL);
  void   setInputFP(FILE* fp);
  void   setDbg(int v);
  FILE*  getDbgFP();
  int    createWords();
  int    mkWords(int ii);
  int    mkWords(const char* word, const char* sep = NULL);
  int    makeWords(const char* headSubWord);
  bool   startWord(const char* headSubWord);
  int    get2Double(const char* word, const char* sep, double& v1, double& v2);
  int    get2Int(const char* word, const char* sep, int& v1, int& v2);
  int    skipToEnd(char* endWord);
  int    skipToEnd(char* endWord, char* name);
  int    readLineAndBreak(int prevWordCnt = -1);
  int    readMultipleLineAndBreak(char continuationChar);
  int    readLine(const char* headSubWord);
  int    parseNextLineUntil(char* endWord, int dbg = 0);
  int    parseNextLineUntil(int n, char* endWord1, char* endWord2, int* pos12);
  int    parseNextLineIfended(int jj);
  int    parseNextLine(char continuationChar = '\0');
  int    parseOneMoreLine(int jj);
  char*  get(int ii);
  char*  get(int start, const char* prefix);
  int    getInt(int ii);
  int    getInt(int n, int start);
  double getDouble(int ii);
  int    getIntFromDouble(int ii);
  void   getDoubleArray(Ath__array1D<double>* A, int start, double mult = 1.0);
  Ath__array1D<double>* readDoubleArray(const char* keyword, int start);
  int                   getIntFromDouble(int ii, int lefUnits);
  int                   reportProgress(FILE* fp);
  int                   reportLines(FILE* fp);
  void                  printWords(FILE* fp);
  void                  printWord(int ii, FILE* fp, char* sep);

  int   getWordCnt();
  char* getLastWord();
  char* getFirstWord();
  char  getFirstChar();

  int getPoint(int ii, char* leftParenth, int* x, int* y, char* rightParenth);
  int getNamePair(int   ii,
                  char* leftParenth,
                  int*  i1,
                  int*  i2,
                  char* rightParenth);
  int getWordCountTo(int start, char rightParenth);

  int   getDefCoord(int ii, int prev);
  void  syntaxError(const char* msg);
  bool  mkDir(char* word);
  int   mkDirTree(const char* word, const char* sep);
  bool  isPlusKeyword(int ii, char* key1);
  char* getRequiredPlusKeyword(int ii, char* key1);
  char* getPlusKeyword(int ii);
  int   parseNextUntil(char* endWord);
  bool  isSeparator(char a);
  char* getValue(int start, char* key);

  void resetSeparator(const char* s);
  void addSeparator(const char* s);
  bool isKeyword(int ii, const char* key1);
  void resetLineNum(int v);
  int  getLineNum();
  bool isDigit(int ii, int jj);

  bool setIntVal(const char* key, int n, int& val);
  bool setDoubleVal(const char* key, int n, double& val);
  void printInt(FILE*       fp,
                const char* sep,
                const char* key,
                int         v,
                bool        pos = false);
  void printDouble(FILE*       fp,
                   const char* sep,
                   const char* key,
                   double      v,
                   bool        pos = false);
  void printString(FILE*       fp,
                   const char* sep,
                   const char* key,
                   char*       v,
                   bool        pos = false);
};


