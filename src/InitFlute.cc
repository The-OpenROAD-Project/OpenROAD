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

#include "flute.h"
#include <fstream>
#include <string>
#include <unistd.h> // getcwd
#include "StringUtil.hh"
#include "InitFlute.hh"

namespace ord {

using std::string;

static bool
readFluteInits(string dir);
static bool
fileExists(const string &filename);

// Flute reads look up tables from local files. gag me.
// Poke around and try and find them near the executable.
void
initFlute(const char *prog_path)
{
  Flute::readLUT();
}

}
