// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "Netlist.h"

#include <string>
#include <vector>

#include "GeoTypes.h"
#include "odb/db.h"

namespace grt {

CUGRPin::CUGRPin(const int index,
                 odb::dbITerm* iterm,
                 const std::vector<BoxOnLayer>& pin_shapes)
    : iterm(iterm), pin_shapes_(pin_shapes), index_(index), is_port_(false)
{
}

CUGRPin::CUGRPin(const int index,
                 odb::dbBTerm* bterm,
                 const std::vector<BoxOnLayer>& pin_shapes)
    : bterm(bterm), pin_shapes_(pin_shapes), index_(index), is_port_(true)
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
                 const std::vector<CUGRPin>& pins,
                 LayerRange layer_range)
    : index_(index), db_net_(db_net), pins_(pins), layer_range_(layer_range)
{
}

}  // namespace grt
