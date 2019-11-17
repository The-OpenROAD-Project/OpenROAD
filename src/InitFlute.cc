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
  string prog_dir = prog_path;
  // Look up one directory level from /build/src.
  auto last_slash = prog_dir.find_last_of("/");
  if (last_slash != string::npos) {
    prog_dir.erase(last_slash);
    last_slash = prog_dir.find_last_of("/");
    if (last_slash != string::npos) {
      prog_dir.erase(last_slash);
      last_slash = prog_dir.find_last_of("/");
      if (last_slash != string::npos) {
	prog_dir.erase(last_slash);
	if (readFluteInits(prog_dir))
	  return;
      }
    }
  }
  // try ./etc
  prog_dir = ".";
  if (readFluteInits(prog_dir))
    return;

  // try ../etc
  prog_dir = "..";
  if (readFluteInits(prog_dir))
    return;

  // try ../../etc
  prog_dir = "../..";
  if (readFluteInits(prog_dir))
    return;

  printf("Error: could not find FluteLUT files POWV9.dat and POST9.dat.\n");
  exit(EXIT_FAILURE);
}

static bool
readFluteInits(string dir)
{
  //  printf("flute try %s\n", dir.c_str());
  string etc;
  sta::stringPrint(etc, "%s/etc", dir.c_str());
  string flute_path1;
  string flute_path2;
  sta::stringPrint(flute_path1, "%s/%s", etc.c_str(), FLUTE_POWVFILE);
  sta::stringPrint(flute_path2, "%s/%s", etc.c_str(), FLUTE_POSTFILE);
  if (fileExists(flute_path1) && fileExists(flute_path2)) {
    char *cwd = getcwd(NULL, 0);
    chdir(etc.c_str());
    Flute::readLUT();
    chdir(cwd);
    free(cwd);
    return true;
  }
  else
    return false;
}

// c++17 std::filesystem::exists
static bool
fileExists(const string &filename)
{
  std::ifstream stream(filename.c_str());
  return stream.good();
}

}
