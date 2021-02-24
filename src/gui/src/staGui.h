/////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2020, OpenROAD
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

#pragma once

#include "opendb/db.h"
namespace ord {
class OpenRoad;
}
namespace sta {
class Instance;
class Net;
class Pin;
}  // namespace sta
namespace gui {
class guiTimingPath;
class staGui
{
 public:
  staGui(ord::OpenRoad* oprd);

  void findInstances(std::string pattern, std::vector<odb::dbInst*>& insts);
  void findNets(std::string pattern, std::vector<odb::dbNet*>& nets);
  void findPins(std::string pattern, std::vector<odb::dbObject*>& pins);

  bool getPaths(std::vector<guiTimingPath*>& paths,
                bool get_max,
                int path_count);

 private:
  ord::OpenRoad* or_;
  std::vector<sta::Instance*> findInstancesNetwork(std::string pattern);
  std::vector<sta::Net*> findNetsNetwork(std::string pattern);
  std::vector<sta::Pin*> findPinsNetwork(std::string pattern);
};

class guiTimingNode
{
 public:
  guiTimingNode(odb::dbObject* pin,
                bool isRising,
                float arrival,
                float required,
                float slack)
      : pin_(pin),
        isRising_(isRising),
        arrival_(arrival),
        required_(required),
        slack_(slack)
  {
  }

  odb::dbObject* pin_;
  bool isRising_;
  float arrival_;
  float required_;
  float slack_;
};

class guiTimingPath
{
 public:
  std::vector<guiTimingNode> nodes;
};
}  // namespace gui
