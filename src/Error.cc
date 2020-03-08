// Copyright (c) 2020, Parallax Software, Inc.
// 
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.

#include <stdarg.h>
#include "openroad/Error.hh"

namespace ord {

Exception::Exception(const char *fmt, ...) :
  std::exception()
{
  va_list args;
  va_start(args, fmt);
  vasprintf(&what_, fmt, args);
}

Exception::~Exception()
{
  delete [] what_;
}

void
error(const char *fmt, ...)
{
  va_list args;
  va_start(args, fmt);
  char *what;
  vasprintf(&what, fmt, args);

  // Exception should be caught by swig error handler.
  throw Exception(what);
}

void
warn(const char *fmt, ...)
{
  va_list args;
  va_start(args, fmt);
  printf("Warning: ");
  vprintf(fmt, args);
  printf("\n");
}

} // namespace
