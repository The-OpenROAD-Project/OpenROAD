// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2020-2025, The OpenROAD Authors

%{

#include "ant/AntennaChecker.hh"
#include "ord/OpenRoad.hh"

ant::AntennaChecker *
getAntennaChecker()
{
  return ord::OpenRoad::openRoad()->getAntennaChecker();
}

namespace ord {
// Defined in OpenRoad.i
odb::dbDatabase *getDb();
}

%}

%include "../../Exception.i"

%inline %{

namespace ant {

int
check_antennas(const char *net_name, bool verbose)
{
  auto app = ord::OpenRoad::openRoad();
  auto block = app->getDb()->getChip()->getBlock();
  odb::dbNet* net = nullptr;
  if (strlen(net_name) > 0) {
    net = block->findNet(net_name);
    if (!net) {
      auto logger = app->getLogger();
      logger->error(utl::ANT, 12, "Net {} not found.", net_name);
    }
  }
  const int num_threads = ord::OpenRoad::openRoad()->getThreadCount();
  return getAntennaChecker()->checkAntennas(net, num_threads, verbose);
}

int
antenna_violation_count()
{
  return getAntennaChecker()->antennaViolationCount();
}

// check a net for antenna violations
bool
check_net_violation(char* net_name)
{ 
  odb::dbNet* net = ord::getDb()->getChip()->getBlock()->findNet(net_name);
  if (net) {
    auto vios = getAntennaChecker()->getAntennaViolations(net, nullptr, 0);
    return !vios.empty();
  }
  else
    return false;
}

void
set_report_file_name(char* file_name)
{
  getAntennaChecker()->setReportFileName(file_name);
}

} // namespace

%} // inline
