///////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// Copyright (c) 2022, The Regents of the University of California
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

// Generator Code Begin Cpp
#include "dbGDSText.h"

#include "dbDatabase.h"
#include "dbDiff.hpp"
#include "dbTable.h"
#include "dbTable.hpp"
#include "odb/db.h"
#include "odb/dbTypes.h"
namespace odb {
template class dbTable<_dbGDSText>;

bool _dbGDSText::operator==(const _dbGDSText& rhs) const
{
  if (_textType != rhs._textType) {
    return false;
  }
  if (_pathType != rhs._pathType) {
    return false;
  }
  if (_width != rhs._width) {
    return false;
  }
  if (_text != rhs._text) {
    return false;
  }

  return true;
}

bool _dbGDSText::operator<(const _dbGDSText& rhs) const
{
  return true;
}

void _dbGDSText::differences(dbDiff& diff,
                             const char* field,
                             const _dbGDSText& rhs) const
{
  DIFF_BEGIN
  DIFF_FIELD(_textType);
  DIFF_FIELD(_pathType);
  DIFF_FIELD(_width);
  DIFF_FIELD(_text);
  DIFF_END
}

void _dbGDSText::out(dbDiff& diff, char side, const char* field) const
{
  DIFF_OUT_BEGIN
  DIFF_OUT_FIELD(_textType);
  DIFF_OUT_FIELD(_pathType);
  DIFF_OUT_FIELD(_width);
  DIFF_OUT_FIELD(_text);

  DIFF_END
}

_dbGDSText::_dbGDSText(_dbDatabase* db)
{
}

_dbGDSText::_dbGDSText(_dbDatabase* db, const _dbGDSText& r)
{
  _textType = r._textType;
  _pathType = r._pathType;
  _width = r._width;
  _text = r._text;
}

dbIStream& operator>>(dbIStream& stream, _dbGDSText& obj)
{
  stream >> obj._textType;
  stream >> obj._presentation;
  stream >> obj._pathType;
  stream >> obj._width;
  stream >> obj._sTrans;
  stream >> obj._text;
  return stream;
}

dbOStream& operator<<(dbOStream& stream, const _dbGDSText& obj)
{
  stream << obj._textType;
  stream << obj._presentation;
  stream << obj._pathType;
  stream << obj._width;
  stream << obj._sTrans;
  stream << obj._text;
  return stream;
}

////////////////////////////////////////////////////////////////////
//
// dbGDSText - Methods
//
////////////////////////////////////////////////////////////////////

void dbGDSText::set_textType(int16_t textType)
{
  _dbGDSText* obj = (_dbGDSText*) this;

  obj->_textType = textType;
}

int16_t dbGDSText::get_textType() const
{
  _dbGDSText* obj = (_dbGDSText*) this;
  return obj->_textType;
}

void dbGDSText::setPresentation(dbGDSTextPres presentation)
{
  _dbGDSText* obj = (_dbGDSText*) this;

  obj->_presentation = presentation;
}

dbGDSTextPres dbGDSText::getPresentation() const
{
  _dbGDSText* obj = (_dbGDSText*) this;
  return obj->_presentation;
}

void dbGDSText::set_pathType(int16_t pathType)
{
  _dbGDSText* obj = (_dbGDSText*) this;

  obj->_pathType = pathType;
}

int16_t dbGDSText::get_pathType() const
{
  _dbGDSText* obj = (_dbGDSText*) this;
  return obj->_pathType;
}

void dbGDSText::setWidth(int width)
{
  _dbGDSText* obj = (_dbGDSText*) this;

  obj->_width = width;
}

int dbGDSText::getWidth() const
{
  _dbGDSText* obj = (_dbGDSText*) this;
  return obj->_width;
}

void dbGDSText::set_sTrans(dbGDSSTrans sTrans)
{
  _dbGDSText* obj = (_dbGDSText*) this;

  obj->_sTrans = sTrans;
}

dbGDSSTrans dbGDSText::get_sTrans() const
{
  _dbGDSText* obj = (_dbGDSText*) this;
  return obj->_sTrans;
}

void dbGDSText::setText(const std::string& text)
{
  _dbGDSText* obj = (_dbGDSText*) this;

  obj->_text = text;
}

std::string dbGDSText::getText() const
{
  _dbGDSText* obj = (_dbGDSText*) this;
  return obj->_text;
}

}  // namespace odb
   // Generator Code End Cpp