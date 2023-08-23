/////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2022, The Regents of the University of California
// All rights reserved.
//
// BSD 3-Clause License
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
//
///////////////////////////////////////////////////////////////////////////////

#include "ord/Tech.h"

#include "db_sta/dbSta.hh"
#include "odb/db.h"
#include "odb/lefin.h"
#include "ord/OpenRoad.hh"

namespace ord {

Tech::Tech()
{
  auto app = OpenRoad::openRoad();
  db_ = app->getDb();
}

odb::dbDatabase* Tech::getDB()
{
  return db_;
}

void Tech::readLef(const std::string& file_name)
{
  auto app = OpenRoad::openRoad();
  const bool make_tech = db_->getTech() == nullptr;
  const bool make_library = true;
  std::string lib_name = file_name;

  // Hacky but easier than dealing with stdc++fs linking
  auto slash_pos = lib_name.find_last_of('/');
  if (slash_pos != std::string::npos) {
    lib_name.erase(0, slash_pos + 1);
  }
  auto dot_pos = lib_name.find_last_of('.');
  if (dot_pos != std::string::npos) {
    lib_name.erase(lib_name.begin() + dot_pos, lib_name.end());
  }

  app->readLef(
      file_name.c_str(), lib_name.c_str(), "", make_tech, make_library);
}

void Tech::readLiberty(const std::string& file_name)
{
  auto sta = OpenRoad::openRoad()->getSta();
  // TODO: take corner & min/max args
  sta->readLiberty(file_name.c_str(),
                   sta->cmdCorner(),
                   sta::MinMaxAll::all(),
                   true /* infer_latches */);
}

sta::dbSta* Tech::getSta()
{
  auto sta = OpenRoad::openRoad()->getSta();
  return sta;
}

}  // namespace ord
