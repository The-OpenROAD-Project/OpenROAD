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

#include <tcl.h>
#include "opendb/db.h"

namespace tool {

class Tool
{
public:
  Tool();
  ~Tool();
  void init(Tcl_Interp *tcl_interp,
	    odb::dbDatabase *db);
  void run(const char *pos_arg1);
  void setParam1(double param1);
  void setFlag1(bool flag1);

private:
  odb::dbDatabase *db_;
  double param1_;
  bool flag1_;
};

}
