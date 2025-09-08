// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "Netlist.h"

#include <string>
#include <vector>

#include "GeoTypes.h"
#include "odb/db.h"

namespace grt {

CUGRPin::CUGRPin(int index,
                 odb::dbITerm* iterm,
                 const std::vector<BoxOnLayer>& pin_shapes,
                 bool is_port)
    : index_(index), iterm(iterm), pin_shapes_(pin_shapes), is_port_(is_port)
{
}

CUGRPin::CUGRPin(int index,
                 odb::dbBTerm* bterm,
                 const std::vector<BoxOnLayer>& pin_shapes,
                 bool is_port)
    : index_(index), bterm(bterm), pin_shapes_(pin_shapes), is_port_(is_port)
{
}

odb::dbITerm* CUGRPin::getITerm() const
{
  return is_port_ ? nullptr : iterm;
}
odb::dbBTerm* CUGRPin::getBTerm() const
{
  return is_port_ ? bterm : nullptr;
}

std::string CUGRPin::getName() const
{
  return is_port_ ? bterm->getName() : iterm->getName();
}

CUGRNet::CUGRNet(const int index,
                 odb::dbNet* db_net,
                 const std::vector<CUGRPin>& pins)
    : index_(index), db_net_(db_net), pins_(pins)
{
}

}  // namespace grt
