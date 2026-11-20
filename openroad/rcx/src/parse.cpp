// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "parse.h"

#include <stdio.h>  // NOLINT(modernize-deprecated-headers): for popen()

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>

#include "rcx/array1.h"
#include "utl/Logger.h"

namespace rcx {

static FILE* ath_openFile(const char* name,
                          const char* mode,
                          utl::Logger* logger)
{
  FILE* a = fopen(name, mode);

  if (a == nullptr) {
    logger->error(utl::RCX, 428, "Cannot open file {} for \"{}\"", name, mode);
  }
  return a;
}

static void ath_closeFile(FILE* fp)
{
  if (fp != nullptr) {
    fclose(fp);
  }
}

static char* ath_allocCharWord(int n, utl::Logger* logger)
{
  if (n <= 0) {
    logger->error(utl::RCX, 424, "Cannot zero/negative number of chars");
  }

  char* a = new char[n];
  a[0] = '\0';
  return a;
}

Parser::Parser(utl::Logger* logger)
{
  _logger = logger;
  init();
}

Parser::~Parser()
{
  if (_inFP && strlen(_inputFile) > 4
      && !strcmp(_inputFile + strlen(_inputFile) - 3, ".gz")) {
    char buff[1024];
    while (!feof(_inFP)) {
      if (fread(buff, 1, 1023, _inFP) != 1) {
        break;
      }
    }
    pclose(_inFP);
    _inFP = nullptr;
  }
  delete[] _inputFile;
  delete[] _line;
  delete[] _wordSeparators;

  for (int ii = 0; ii < _maxWordCnt; ii++) {
    delete[] _wordArray[ii];
  }

  delete[] _wordArray;

  if (_inFP) {
    ath_closeFile(_inFP);
  }
}

void Parser::init()
{
  _line = ath_allocCharWord(_lineSize, _logger);

  _wordArray = new char*[_maxWordCnt];

  for (int ii = 0; ii < _maxWordCnt; ii++) {
    _wordArray[ii] = ath_allocCharWord(512, _logger);
  }

  _wordSeparators = ath_allocCharWord(24, _logger);

  strcpy(_wordSeparators, " \n\t");

  _lineNum = 0;
  _currentWordCnt = -1;

  _inFP = nullptr;
  _inputFile = ath_allocCharWord(512, _logger);
}

int Parser::getLineNum()
{
  return _lineNum;
}

void Parser::resetLineNum(int v)
{
  _lineNum = v;
}

bool Parser::isDigit(int ii, int jj)
{
  const char C = _wordArray[ii][jj];

  return (C >= '0') && (C <= '9');
}

void Parser::resetSeparator(const char* s)
{
  strcpy(_wordSeparators, s);
}

void Parser::addSeparator(const char* s)
{
  strcat(_wordSeparators, s);
}

void Parser::openFile(const char* name)
{
  if (name != nullptr && strlen(name) > 4
      && !strcmp(name + strlen(name) - 3, ".gz")) {
    char cmd[256];
    sprintf(cmd, "gzip -cd %s", name);
    _inFP = popen(cmd, "r");
    strcpy(_inputFile, name);
  } else if (name == nullptr && strlen(_inputFile) > 4
             && !strcmp(_inputFile + strlen(_inputFile) - 3, ".gz")) {
    char cmd[256];
    if (_inFP) {
      char buff[1024];
      while (!feof(_inFP)) {
        if (fread(buff, 1, 1023, _inFP) != 1) {
          break;
        }
      }
      pclose(_inFP);
      _inFP = nullptr;
    }
    sprintf(cmd, "gzip -cd %s", _inputFile);
    _inFP = popen(cmd, "r");
  } else if (name != nullptr) {
    _inFP = ath_openFile(name, "r", _logger);
    strcpy(_inputFile, name);
  } else {  //
    _inFP = ath_openFile(_inputFile, "r", _logger);
  }
}

void Parser::setInputFP(FILE* fp)
{
  _inFP = fp;
}

void Parser::printWords(FILE* fp)
{
  if (fp == nullptr) {
    return;
  }
  for (int ii = 0; ii < _currentWordCnt; ii++) {
    fprintf(fp, "%s ", _wordArray[ii]);
  }
  fprintf(fp, "\n");
}

int Parser::getWordCnt()
{
  return _currentWordCnt;
}

char Parser::getFirstChar()
{
  return get(0)[0];
}

char* Parser::get(int ii)
{
  if ((ii < 0) || (ii >= _currentWordCnt)) {
    return nullptr;
  }
  return _wordArray[ii];
}

int Parser::getInt(int ii)
{
  return atoi(get(ii));
}

int Parser::getInt(int n, int start)
{
  char* word = get(n);
  char buff[128];
  int k = 0;
  for (int ii = start; word[ii] != '\0'; ii++) {
    buff[k++] = word[ii];
  }
  buff[k++] = '\0';

  return atoi(buff);
}

double Parser::getDouble(int ii)
{
  return atof(get(ii));
}

void Parser::getDoubleArray(Array1D<double>* A, int start, double mult)
{
  if (mult == 1.0) {
    for (int ii = start; ii < _currentWordCnt; ii++) {
      A->add(atof(get(ii)));
    }
  } else {
    for (int ii = start; ii < _currentWordCnt; ii++) {
      A->add(atof(get(ii)) * mult);
    }
  }
}

Array1D<double>* Parser::readDoubleArray(const char* keyword, int start1)
{
  if ((keyword != nullptr) && (strcmp(keyword, get(0)) != 0)) {
    return nullptr;
  }

  if (getWordCnt() < 1) {
    return nullptr;
  }

  auto* A = new Array1D<double>(getWordCnt());
  int start = 0;
  if (keyword != nullptr) {
    start = start1;
  }
  getDoubleArray(A, start);
  return A;
}

int Parser::mkWords(const char* word, const char* sep)
{
  if (word == nullptr) {
    return 0;
  }

  char buf1[100];
  if (sep != nullptr) {
    strcpy(buf1, _wordSeparators);
    strcpy(_wordSeparators, sep);
  }

  strcpy(_line, word);
  _currentWordCnt = mkWords(0);

  if (sep != nullptr) {
    strcpy(_wordSeparators, buf1);
  }

  return _currentWordCnt;
}

bool Parser::mkDir(const char* word)
{
  return std::filesystem::create_directories(word);
}

int Parser::mkDirTree(const char* word, const char* sep)
{
  mkDir((char*) word);

  mkWords(word, sep);
  return _currentWordCnt;
}

bool Parser::isSeparator(char a)
{
  int len = strlen(_wordSeparators);
  for (int k = 0; k < len; k++) {
    if (a == _wordSeparators[k]) {
      return true;
    }
  }
  return false;
}

int Parser::mkWords(int jj)
{
  if (_line[0] == _commentChar) {
    return jj;
  }

  int ii = 0;
  int len = strlen(_line);
  while (ii < len) {
    int k = ii;
    for (; k < len; k++) {
      if (!isSeparator(_line[k])) {
        break;
      }
    }
    if (k == len) {
      break;
    }

    int charIndex = 0;
    for (; k < len; k++) {
      if ((_line[k] == _wordSeparators[0]) || isSeparator(_line[k])) {
        _wordArray[jj][charIndex] = '\0';
        jj++;

        break;
      }
      if (_line[k] == _commentChar) {
        return jj;
      }

      _wordArray[jj][charIndex++] = _line[k];
    }
    if (k == len) {
      _wordArray[jj][charIndex] = '\0';
      jj++;
    }
    ii = k;
  }
  return jj;
}

void Parser::reportProgress()
{
  if (_lineNum % _progressLineChunk == 0) {
    _logger->report("\t\tRead {} lines", _lineNum);
  }
}

int Parser::readLineAndBreak(int prevWordCnt)
{
  if (fgets(_line, _lineSize, _inFP) == nullptr) {
    _currentWordCnt = prevWordCnt;
    return prevWordCnt;
  }

  _lineNum++;
  reportProgress();

  if (prevWordCnt < 0) {
    _currentWordCnt = mkWords(0);
  } else {
    _currentWordCnt = mkWords(prevWordCnt);
  }

  return _currentWordCnt;
}

void Parser::syntaxError(const char* msg)
{
  _logger->error(utl::RCX, 429, "Syntax Error at line {} ({})", _lineNum, msg);
}

int Parser::parseNextLine()
{
  while (readLineAndBreak() == 0) {
    ;
  }

  return _currentWordCnt;
}

bool Parser::isKeyword(int ii, const char* key1)
{
  return (get(ii) != nullptr) && (strcmp(get(ii), key1) == 0);
}

}  // namespace rcx
