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

#ifndef MAKE_ICeWall_H
#define MAKE_ICeWall_H

namespace ICeWall {
class ICeWall;
}

namespace ord {

class OpenRoad;

ICeWall::ICeWall *
makeICeWall();

void
deleteICeWall(ICeWall::ICeWall *ICeWall);

void
initICeWall(OpenRoad *openroad);

} // namespace
#endif
