///////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// Copyright (c) 2023, Nefelus Inc
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

#include "rcx/ext2dBox.h"

#include <utility>

namespace rcx {

ext2dBox::ext2dBox(std::array<int, 2> ll,
                   std::array<int, 2> ur,
                   unsigned int met,
                   unsigned int id,
                   unsigned int map,
                   bool dir)
    : _ll(ll), _ur(ur), _met(met), _id(id), _map(map), _dir(dir)
{
}
bool ext2dBox::matchCoords(int* ll, int* ur) const
{
  if ((ur[0] < _ll[0]) || (ll[0] > _ur[0]) || (ur[1] < _ll[1])
      || (ll[1] > _ur[1]))
    return false;

  return true;
}

void ext2dBox::rotate()
{
  std::swap(_ur[0], _ur[1]);
  std::swap(_ll[0], _ll[1]);
  _dir = !_dir;
}
unsigned int ext2dBox::length() const
{
  return _ur[_dir] - _ll[_dir];
}
unsigned int ext2dBox::width() const
{
  return _ur[!_dir] - _ll[!_dir];
}
int ext2dBox::loX() const
{
  return _ll[0];
}
int ext2dBox::loY() const
{
  return _ll[1];
}
unsigned int ext2dBox::id() const
{
  return _id;
}
void ext2dBox::printGeoms3D(FILE* fp,
                            double h,
                            double t,
                            const std::array<int, 2>& orig) const
{
  fprintf(fp,
          "%3d %8d -- M%d D%d  %g %g  %g %g  L= %g W= %g  H= %g  TH= %g ORIG "
          "%g %g\n",
          _id,
          _map,
          _met,
          _dir,
          0.001 * _ll[0],
          0.001 * _ll[1],
          0.001 * _ur[0],
          0.001 * _ur[1],
          0.001 * length(),
          0.001 * width(),
          h,
          t,
          0.001 * (_ll[0] - orig[0]),
          0.001 * (_ll[1] - orig[1]));
}

}  // namespace rcx
