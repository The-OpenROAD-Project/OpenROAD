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

#include "dbStream.h"

namespace odb {

class dbIdValidation
{
 public:
  static uint invalidId() { return 0; }
  static bool isId(const char* inid);
};

//
// April 2006 twg -
//
//     I removed the inheritance of empty class dbIdValidation from the
//     inheritance list of dbId. The "C++" standard does not enforce an empty
//     class to have zero bytes. It is up to the compiler to optimize the empty
//     class.
//
//     If the compiler does NOT optimize the empty class, then database index
//     will increase from 32-bits to 64-bits! (the compiler will align the "_id"
//     field to a 32-bit boundary.)
//
template <class T>
class dbId
{
  unsigned int _id;

 public:
  typedef T _type;

  dbId() { _id = 0; }
  dbId(const dbId<T>& id) : _id(id._id) {}
  dbId(unsigned int id) { _id = id; }

                operator unsigned int() const { return _id; }
  unsigned int& id() { return _id; }

  bool isValid() { return _id > 0; }

  friend dbOStream& operator<<(dbOStream& stream, const dbId<T>& id)
  {
    stream << id._id;
    return stream;
  }

  friend dbIStream& operator>>(dbIStream& stream, dbId<T>& id)
  {
    stream >> id._id;
    return stream;
  }
};

}  // namespace odb


