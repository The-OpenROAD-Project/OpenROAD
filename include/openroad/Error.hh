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

#pragma once

#include <string>
#include <iostream>

namespace ord {

using std::string;

class Exception : public std::exception
{
public:
  Exception(const char *fmt, ...);
  virtual ~Exception();
  virtual const char *what() const noexcept { return what_; }

private:
  char *what_;
};

void
error(const char *fmt, ...);
void
warn(const char *fmt, ...);

} // namespace
