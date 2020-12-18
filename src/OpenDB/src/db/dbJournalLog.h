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

#include "dbPagedVector.h"
#include "odb.h"

namespace odb {

class dbIStream;
class dbOStream;

class dbJournalLog
{
  dbPagedVector<unsigned char> _data;
  int                          _idx;
  int                          _debug;

  unsigned char next() { return _data[_idx++]; }

 public:
  dbJournalLog();
  ~dbJournalLog();

  void clear()
  {
    _data.clear();
    _idx = 0;
  }

  bool empty() { return _data.size() == 0; }

  uint idx() { return _idx; }
  uint size() { return _data.size(); }
  void push(bool value);
  void push(char value);
  void push(unsigned char value);
  void push(int value);
  void push(unsigned int value);
  void push(float value);
  void push(double value);
  void push(const char* value);

  void begin() { _idx = 0; }
  bool end() { return _idx == (int) _data.size(); }
  void set(uint idx) { _idx = idx; }

  void              pop(bool& value);
  void              pop(char& value);
  void              pop(unsigned char& value);
  void              pop(int& value);
  void              pop(unsigned int& value);
  void              pop(float& value);
  void              pop(double& value);
  void              pop(char*& value);
  void              pop(std::string& value);
  friend dbIStream& operator>>(dbIStream& stream, dbJournalLog& log);
  friend dbOStream& operator<<(dbOStream& stream, const dbJournalLog& log);
};

}  // namespace odb
