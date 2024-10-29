// Copyright (c) 2021, The Regents of the University of California
// All rights reserved.
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
#include "odb/db.h"
#include <curl/curl.h>
#include <boost/json.hpp>

namespace utl {
class Logger;
}

namespace ora {

using utl::Logger;

class Ora
{

public:
  Ora();
  ~Ora();
  void init(Tcl_Interp *tcl_interp, 
            odb::dbDatabase *db,
            utl::Logger* logger);
  void askbot(const char *pos_arg1);
  void set_bothost(const char *host);
  void setSourceFlag(bool sourceFlag);

private:
  odb::dbDatabase *db_;
  bool sourceFlag_;
  std::string hostUrl;
  
 Logger* logger_ = nullptr;

};

}
