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

#include "parse.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "dbLogger.h"
#include "misc_global.h"
int Ath__double2int(double v);

void Ath__parser::init()
{
  _line = ATH__allocCharWord(_lineSize);
  _tmpLine = ATH__allocCharWord(_lineSize);

  _wordArray = new char*[_maxWordCnt];
  if (_wordArray == NULL)
    ATH__failMessage("Cannot allocate array of char*");

  for (int ii = 0; ii < _maxWordCnt; ii++)
    _wordArray[ii] = ATH__allocCharWord(_wordSize);

  _wordSeparators = ATH__allocCharWord(24);

  strcpy(_wordSeparators, " \n\t");

  _commentChar = '#';

  _lineNum = 0;
  _currentWordCnt = -1;

  _inFP = NULL;
  _dbgFP = NULL;
  _inputFile = ATH__allocCharWord(512);

  _dbg = -1;
  _progressLineChunk = 1000000;
}
Ath__parser::Ath__parser()
{
  _lineSize = 10000;
  _maxWordCnt = 100;
  _wordSize = 512;
  init();
}
Ath__parser::Ath__parser(int lSize, int wCnt, int wSize)
{
  _lineSize = lSize;
  _maxWordCnt = wCnt;
  _wordSize = wSize;
  init();
}
int Ath__parser::getLineNum()
{
  return _lineNum;
}
void Ath__parser::resetLineNum(int v)
{
  _lineNum = v;
}
bool Ath__parser::isDigit(int ii, int jj)
{
  char C = _wordArray[ii][jj];

  if ((C >= '0') && (C <= '9'))
    return true;
  else
    return false;
}

void Ath__parser::resetSeparator(const char* s)
{
  strcpy(_wordSeparators, s);
}
void Ath__parser::addSeparator(const char* s)
{
  strcat(_wordSeparators, s);
}

void Ath__parser::openFile(char* name)
{
  if (name != NULL && strlen(name) > 4
      && !strcmp(name + strlen(name) - 3, ".gz")) {
    char cmd[256];
    sprintf(cmd, "gzip -cd %s", name);
    _inFP = popen(cmd, "r");
    strcpy(_inputFile, name);
  } else if (name == NULL && strlen(_inputFile) > 4
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
      _inFP = NULL;
    }
    sprintf(cmd, "gzip -cd %s", _inputFile);
    _inFP = popen(cmd, "r");
  } else if (name != NULL) {
    _inFP = ATH__openFile(name, (char*) "r");
    strcpy(_inputFile, name);
  } else {  //
    _inFP = ATH__openFile(_inputFile, (char*) "r");
  }
  //	_dbgFP= ATH__openFile("parse.dbg", (char*) "w");
}
void Ath__parser::setInputFP(FILE* fp)
{
  _inFP = fp;
}
FILE* Ath__parser::getDbgFP()
{
  return _dbgFP;
}
void Ath__parser::setDbg(int v)
{
  _dbg = v;
}
Ath__parser::~Ath__parser()
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
    _inFP = NULL;
  }
  ATH__deallocCharWord(_inputFile);
  ATH__deallocCharWord(_line);
  ATH__deallocCharWord(_tmpLine);
  ATH__deallocCharWord(_wordSeparators);

  for (int ii = 0; ii < _maxWordCnt; ii++)
    ATH__deallocCharWord(_wordArray[ii]);

  delete[] _wordArray;

  if (_inFP)
    ATH__closeFile(_inFP);
}
void Ath__parser::printWord(int ii, FILE* fp, char* sep)
{
  if (ii < 0 || ii >= _currentWordCnt)
    return;
  if (sep != NULL)
    fprintf(fp, "%s%s", _wordArray[ii], sep);
  else
    fprintf(fp, "%s", _wordArray[ii]);
}
void Ath__parser::printWords(FILE* fp)
{
  if (fp == NULL) {  // use motice
    for (int ii = 0; ii < _currentWordCnt; ii++)
      odb::notice(0, "%s ", _wordArray[ii]);
    odb::notice(0, "\n");
    return;
  }
  for (int ii = 0; ii < _currentWordCnt; ii++)
    fprintf(fp, "%s ", _wordArray[ii]);
  fprintf(fp, "\n");
}
int Ath__parser::parseNextLineIfended(int jj)
{
  if (jj >= _currentWordCnt) {
    parseNextLine();
    return 0;
  } else {
    return jj;
  }
}

int Ath__parser::getWordCnt()
{
  return _currentWordCnt;
}
char* Ath__parser::getLastWord()
{
  return get(_currentWordCnt - 1);
}
char* Ath__parser::getFirstWord()
{
  return get(0);
}
char Ath__parser::getFirstChar()
{
  return get(0)[0];
}
char* Ath__parser::get(int ii)
{
  if ((ii < 0) || (ii >= _currentWordCnt))
    return NULL;
  else
    return _wordArray[ii];
}
int Ath__parser::getInt(int ii)
{
  return atoi(get(ii));
}
bool Ath__parser::setIntVal(const char* key, int n, int& val)
{
  if (strcmp(key, get(0)) == 0) {
    val = getInt(n);
    return true;
  }
  return false;
}
bool Ath__parser::setDoubleVal(const char* key, int n, double& val)
{
  if (strcmp(key, get(0)) == 0) {
    val = getDouble(n);
    return true;
  }
  return false;
}

void Ath__parser::printInt(FILE* fp,
                           const char* sep,
                           const char* key,
                           int v,
                           bool pos)
{
  if (pos && !(v > 0))
    return;

  fprintf(fp, "%s%s %d\n", sep, key, v);
}
void Ath__parser::printDouble(FILE* fp,
                              const char* sep,
                              const char* key,
                              double v,
                              bool pos)
{
  if (pos && !(v > 0))
    return;

  fprintf(fp, "%s%s %g\n", sep, key, v);
}
void Ath__parser::printString(FILE* fp,
                              const char* sep,
                              const char* key,
                              char* v,
                              bool pos)
{
  if (pos && !((v == NULL) || (strcmp("", v) == 0)))
    return;

  fprintf(fp, "%s%s %s\n", sep, key, v);
}
int Ath__parser::getInt(int n, int start)
{
  char* word = get(n);
  char buff[128];
  int k = 0;
  for (int ii = start; word[ii] != '\0'; ii++)
    buff[k++] = word[ii];
  buff[k++] = '\0';

  return atoi(buff);
}
double Ath__parser::getDouble(int ii)
{
  return atof(get(ii));
}
int Ath__parser::getIntFromDouble(int ii)
{
  return Ath__double2int(atof(get(ii)));
}
void Ath__parser::getDoubleArray(Ath__array1D<double>* A,
                                 int start,
                                 double mult)
{
  if (mult == 1.0) {
    for (int ii = start; ii < _currentWordCnt; ii++)
      A->add(atof(get(ii)));
  } else {
    for (int ii = start; ii < _currentWordCnt; ii++)
      A->add(atof(get(ii)) * mult);
  }
}
Ath__array1D<double>* Ath__parser::readDoubleArray(const char* keyword,
                                                   int start1)
{
  if ((keyword != NULL) && (strcmp(keyword, get(0)) != 0))
    return NULL;

  if (getWordCnt() < 1)
    return NULL;

  Ath__array1D<double>* A = new Ath__array1D<double>(getWordCnt());
  int start = 0;
  if (keyword != NULL)
    start = start1;
  getDoubleArray(A, start);
  return A;
}
int Ath__parser::getIntFromDouble(int ii, int lefUnits)
{
  char buf[1000];
  sprintf(buf, "%f", atof(get(ii)) * lefUnits);
  return atoi(buf);
}
int Ath__parser::mkWords(const char* word, const char* sep)
{
  if (word == NULL)
    return 0;

  char buf1[100];
  if (sep != NULL) {
    strcpy(buf1, _wordSeparators);
    strcpy(_wordSeparators, sep);
  }

  strcpy(_line, word);
  _currentWordCnt = mkWords(0);

  if (sep != NULL)
    strcpy(_wordSeparators, buf1);

  return _currentWordCnt;
}
int Ath__parser::get2Int(const char* word, const char* sep, int& v1, int& v2)
{
  // return -1 if no tokens
  // return 0 if there are 2 words separated with sep[0]
  // return 1 if v1 is valid
  // return 2 if v2 is valid

  if ((word == NULL) || (sep == NULL))
    return -1;

  int n = mkWords(word, sep);
  if (n == 0)
    return -1;

  if (n == 1) {
    if (word[0] == sep[0]) {
      v2 = getInt(0);
      return 2;
    }
    v1 = getInt(0);
    return 1;
  }
  v1 = getInt(0);
  v2 = getInt(1);

  return 0;
}
int Ath__parser::get2Double(const char* word,
                            const char* sep,
                            double& v1,
                            double& v2)
{
  // return -1 if no tokens
  // return 0 if there are 2 words separated with sep[0]
  // return 1 if v1 is valid
  // return 2 if v2 is valid

  if ((word == NULL) || (sep == NULL))
    return -1;

  int n = mkWords(word, sep);
  if (n == 0)
    return -1;

  if (n == 1) {
    if (word[0] == sep[0]) {
      v2 = getDouble(0);
      return 2;
    }
    v1 = getDouble(0);
    return 1;
  }
  v1 = getDouble(0);
  v2 = getDouble(1);

  return 0;
}
bool Ath__parser::mkDir(char* word)
{
  char command[1024];
  sprintf(command, "mkdir -p %s", word);
  return system(command) == 0;
}

int Ath__parser::mkDirTree(const char* word, const char* sep)
{
  mkWords(word, sep);

  if (_currentWordCnt > 0) {
    mkDir(_wordArray[0]);
    sprintf(_line, "%s", _wordArray[0]);
  }
  int pos = 0;
  for (int ii = 1; ii < _currentWordCnt; ii++) {
    pos += sprintf(&_line[pos], "/%s", _wordArray[ii]);
    mkDir(_line);
  }
  return _currentWordCnt;
}
bool Ath__parser::isSeparator(char a)
{
  int len = strlen(_wordSeparators);
  for (int k = 0; k < len; k++) {
    if (a == _wordSeparators[k])
      return true;
  }
  return false;
}
int Ath__parser::mkWords(int jj)
{
  //	fprintf(stdout, "%s", _line);
  if (_line[0] == _commentChar)
    return jj;

  int ii = 0;
  int len = strlen(_line);
  while (ii < len) {
    int k = ii;
    for (; k < len; k++) {
      if (!isSeparator(_line[k]))
        break;
    }
    if (k == len)
      break;

    int charIndex = 0;
    for (; k < len; k++) {
      if ((_line[k] == _wordSeparators[0]) || isSeparator(_line[k])) {
        _wordArray[jj][charIndex] = '\0';
        jj++;

        break;
      }
      if (_line[k] == _commentChar)
        return jj;

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
int Ath__parser::reportProgress(FILE* fp)
{
  if (_lineNum % _progressLineChunk == 0)
    fprintf(fp, "\t\tHave read %d lines\n", _lineNum);

  return _lineNum;
}
int Ath__parser::reportLines(FILE* fp)
{
  fprintf(fp, "Have read %d lines\n", _lineNum);
  return _lineNum;
}
int Ath__parser::readLineAndBreak(int prevWordCnt)
{
  if (fgets(_line, _lineSize, _inFP) == NULL) {
    _currentWordCnt = prevWordCnt;
    return prevWordCnt;
  }

  _lineNum++;
  reportProgress(stdout);

  if (_dbg > 0)
    fprintf(stdout, "%s", _line);

  if (prevWordCnt < 0)
    _currentWordCnt = mkWords(0);
  else
    _currentWordCnt = mkWords(prevWordCnt);

  return _currentWordCnt;
}
int Ath__parser::readMultipleLineAndBreak(char continuationChar)
{
  strcpy(_tmpLine, "");

  if (fgets(_line, _lineSize, _inFP) == NULL) {
    _currentWordCnt = 0;
    return -1;
  }

  do {
    _lineNum++;
    int len = strlen(_line);

    int kk = len - 2;
    if (kk < 0) {
      continue;
    }

    while ((kk >= 0) && (_line[kk] == ' '))  // consume spaces
      kk--;

    if (_line[kk] != continuationChar) {
      strcat(_tmpLine, _line);
      strcpy(_line, _tmpLine);
      break;
    } else {
      _line[len - 1] = ' ';  // overwrite the newline
      _line[kk] = ' ';
      strcat(_tmpLine, _line);
    }
    reportProgress(stdout);

  } while (fgets(_line, _lineSize, _inFP) != NULL);

  if (_dbg > 0)
    fprintf(stdout, "%s", _line);

  _currentWordCnt = mkWords(0);

  return _currentWordCnt;
}
void Ath__parser::syntaxError(const char* msg)
{
  fprintf(stderr, "\n Syntax Error at line %d (%s)\n", _lineNum, msg);
  exit(1);
}
int Ath__parser::parseOneMoreLine(int jj)
{
  int k = 0;
  for (int ii = jj; ii < getWordCnt(); ii++) {
    strcpy(_wordArray[k++], get(ii));
  }
  if (k > 0)
    _currentWordCnt = k;

  while (readLineAndBreak(_currentWordCnt) == 0)
    ;

  return _currentWordCnt;
}
int Ath__parser::parseNextLine(char continuationChar)
{
  if (continuationChar == '\0') {
    while (readLineAndBreak() == 0)
      ;
  } else {
    while (readMultipleLineAndBreak(continuationChar) == 0)
      ;
  }
  if (_dbg == 1)
    printWords(stdout);

  return _currentWordCnt;
}
int Ath__parser::parseNextLineUntil(int n,
                                    char* endWord1,
                                    char* endWord2,
                                    int* pos12)
{
  *pos12 = -1;
  int prevCnt = n;
  while (readLineAndBreak(prevCnt) > 0) {
    if (prevCnt == _currentWordCnt)
      continue;
    if (strcmp(endWord1, get(prevCnt)) == 0) {
      *pos12 = 0;
      break;
    }
    if (strcmp(endWord2, get(prevCnt)) == 0) {
      *pos12 = 0;
      break;
    }
    prevCnt = _currentWordCnt;
  }
  return _currentWordCnt;
}
int Ath__parser::parseNextLineUntil(char* endWord, int dbg)
{
  if (dbg > 0)
    _dbg = dbg;

  while (readLineAndBreak(_currentWordCnt) > 0) {
    if (strcmp(endWord, getLastWord()) == 0)
      break;
  }

  if (dbg > 0)
    _dbg = 0;

  return _currentWordCnt;
}
int Ath__parser::parseNextUntil(char* endWord)
{
  _currentWordCnt = 0;
  while (readLineAndBreak(_currentWordCnt) > 0) {
    if (strcmp(endWord, getLastWord()) == 0)
      break;
  }
  return _currentWordCnt;
}
int Ath__parser::getPoint(int ii,
                          char* leftParenth,
                          int* x,
                          int* y,
                          char* rightParenth)
{
  if ((strcmp(get(ii), leftParenth) == 0)
      && (strcmp(get(ii + 3), rightParenth) == 0)) {
    *x = getInt(ii + 1);
    *y = getInt(ii + 2);
    return 1;
  } else {
    return -1;
  }
}
char* Ath__parser::getPlusKeyword(int ii)
{
  if ((strcmp(get(ii), "+") == 0) && (get(ii + 1) != NULL))
    return get(ii + 1);
  else
    return NULL;
}
bool Ath__parser::isKeyword(int ii, const char* key1)
{
  if ((get(ii) != NULL) && (strcmp(get(ii), key1) == 0)) {
    return true;
  } else {
    return false;
  }
}
bool Ath__parser::isPlusKeyword(int ii, char* key1)
{
  if ((strcmp(get(ii), "+") == 0) && (get(ii + 1) != NULL)
      && (strcmp(get(ii + 1), key1) == 0)) {
    return true;
  } else {
    return false;
  }
}
char* Ath__parser::getValue(int start, char* key)
{
  int ii = start;
  for (; ii < _currentWordCnt; ii++) {
    if (strcmp(get(ii), key) == 0)
      break;
  }
  return get(ii + 1);
}
char* Ath__parser::getRequiredPlusKeyword(int ii, char* key1)
{
  if ((strcmp(get(ii), "+") == 0) && (get(ii + 1) != NULL)
      && (strcmp(get(ii + 1), key1) == 0) && (get(ii + 2) != NULL)) {
    return get(ii + 2);
  } else {
    return NULL;
  }
}
int Ath__parser::getNamePair(int ii,
                             char* leftParenth,
                             int* i1,
                             int* i2,
                             char* rightParenth)
{
  if ((strcmp(get(ii), leftParenth) == 0) && (get(ii + 3) != NULL)
      && (strcmp(get(ii + 3), rightParenth) == 0)) {
    getInt(ii + 1);
    getInt(ii + 2);
    *i1 = ii + 1;
    *i2 = ii + 2;
    return 1;
  } else {
    return -1;
  }
}
char* Ath__parser::get(int start, const char* prefix)
{
  for (int ii = start; ii < _currentWordCnt; ii++) {
    char* w = get(ii);
    if (strstr(w, prefix) != NULL)
      return w;
  }
  return NULL;
}
int Ath__parser::getWordCountTo(int start, char rightParenth)
{
  for (int ii = start; ii < _currentWordCnt; ii++) {
    if (get(ii)[0] == rightParenth) {
      return ii - start;
    }
  }
  return -1;
}
int Ath__parser::getDefCoord(int ii, int prev)
{
  if (get(ii)[0] == '*')
    return prev;
  else
    return getInt(ii);
}
int Ath__parser::skipToEnd(char* endWord)
{
  while (parseNextLine() > 0) {
    if (strcmp(endWord, get(0)) == 0) {
      return 0;
    }
  }
  return 1;
}
int Ath__parser::skipToEnd(char* endWord, char* name)
{
  while (parseNextLine() > 0) {
    if ((getWordCnt() > 1) && (strcmp(endWord, get(0)) == 0)
        && (strcmp(name, get(1)) == 0)) {
      return 0;
    }
  }
  return 1;
}
int Ath__parser::createWords()
{
  _currentWordCnt = mkWords(0);
  return _currentWordCnt;
}
int Ath__parser::readLine(const char* headSubWord)
{
  _currentWordCnt = 0;
  if (fgets(_line, _lineSize, _inFP) == NULL) {
    return -1;
  }
  if (_dbg > 0)
    fprintf(stdout, "%s", _line);

  if (headSubWord == NULL)
    return 0;
  return makeWords(headSubWord);
}
bool Ath__parser::startWord(const char* headSubWord)
{
  if (headSubWord == NULL)
    return true;

  uint hCnt = strlen(headSubWord);
  uint lineCharCnt = strlen(_line);
  if (lineCharCnt < hCnt)
    return false;

  uint ii = 0;
  for (; _line[ii] == ' '; ii++)
    ;
  if (lineCharCnt - ii < hCnt)
    return false;
  for (uint jj = 0; jj < hCnt; jj++) {
    if (headSubWord[jj] != _line[ii++])
      return false;
  }
  return true;
}
int Ath__parser::makeWords(const char* headSubWord)
{
  if (!startWord(headSubWord))
    return 0;

  _currentWordCnt = mkWords(0);
  return _currentWordCnt;
}
void Ath__parser::keepLine()
{
  strcpy(_tmpLine, _line);
}
void Ath__parser::getTmpLine()
{
  strcpy(_line, _tmpLine);
}
