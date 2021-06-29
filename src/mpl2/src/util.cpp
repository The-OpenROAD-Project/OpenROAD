///////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// Copyright (c) 2021, The Regents of the University of California
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

#include "util.h"

#include <fstream>
#include <iostream>
#include <unordered_map>
#include <vector>

using namespace std;

bool SortBySecond(const pair<int, float>& a, const pair<int, float>& b)
{
  return a.second < b.second;
}

// char_match:  determine if the char is part of deliminators
bool CharMatch(char c, const string& delim)
{
  auto it = delim.begin();
  while (it != delim.end()) {
    if ((*it) == c) {
      return true;
    }

    ++it;
  }
  return false;
}

// find the next position for deliminator char
string::const_iterator FindDelim(string::const_iterator i,
                                 string::const_iterator end,
                                 const string& delim)
{
  while (i != end && !CharMatch(*i, delim))
    ++i;

  return i;
}

// find the next position for non deliminator char
string::const_iterator FindNotDelim(string::const_iterator i,
                                    string::const_iterator end,
                                    const string& delim)
{
  while (i != end && CharMatch(*i, delim))
    ++i;

  return i;
}

// split the strings based on the deliminators
vector<string> Split(const string& str, string delim)
{
  vector<string> ret;
  typedef string::const_iterator iter;
  iter i = str.begin();
  while (i != str.end()) {
    i = FindNotDelim(i, str.end(), delim);
    iter j = FindDelim(i, str.end(), delim);
    if (i != str.end()) {
      ret.push_back(string(i, j));
      i = j;
    }
  }

  return ret;
}

unordered_map<string, string> ParseConfigFile(const char* file_name)
{
  unordered_map<string, string> params;
  fstream f;
  string line;
  f.open(file_name, ios::in);
  while (getline(f, line)) {
    vector<string> words = Split(line);

    if (!words.empty() && words[1] == "=") {
      params[words[0]] = words[2];
    }
  }

  f.close();

  return params;
}
