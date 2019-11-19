// Copyright (c) 2019, Parallax Software, Inc.
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

%module tool

%{

#include "tool/Tool.hh"
#include "openroad/OpenRoad.hh"

tool::Tool *
getTool()
{
  return ord::OpenRoad::openRoad()->getTool();
}

%}

%inline %{

void
tool_set_param1(double param1)
{
  getTool()->setParam1(param1);
}

void
tool_set_flag1(bool flag1)
{
  getTool()->setFlag1(flag1);
}

void
tool_run(const char *pos_arg1)
{
  getTool()->run(pos_arg1);
}

%} // inline
