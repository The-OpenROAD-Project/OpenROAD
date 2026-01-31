// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2020-2025, The OpenROAD Authors

%{
#include "LoggerCommon.h"
#include "db_sta/dbSta.hh"
#include "odb/db.h"
#include "utl/IpChecker.h"
#include "utl/Logger.h"

namespace ord {
// Defined in OpenRoad.i
utl::Logger* getLogger();

odb::dbDatabase* getDb();

sta::dbSta* getSta();
}  // namespace ord

using ord::getLogger;
using utl::Logger;
using utl::ToolId;
%}

%typemap(in) utl::ToolId
{
  int length;
  const char* arg = Tcl_GetStringFromObj($input, &length);
  $1 = utl::Logger::findToolId(arg);
}

// Catch exceptions in inline functions.
%include "../../Exception.i"
%include "stdint.i"
%import <std_string.i>

%include "LoggerCommon.h"

%inline %{
namespace utl {

void teeFileBegin(const std::string& filename)
{
  utl::Logger* logger = ord::getLogger();
  logger->teeFileBegin(filename);
}

void teeFileAppendBegin(const std::string& filename)
{
  utl::Logger* logger = ord::getLogger();
  logger->teeFileAppendBegin(filename);
}

void teeFileEnd()
{
  utl::Logger* logger = ord::getLogger();
  logger->teeFileEnd();
}

void redirectFileBegin(const std::string& filename)
{
  utl::Logger* logger = ord::getLogger();
  logger->redirectFileBegin(filename);
}

void redirectFileAppendBegin(const std::string& filename)
{
  utl::Logger* logger = ord::getLogger();
  logger->redirectFileAppendBegin(filename);
}

void redirectFileEnd()
{
  utl::Logger* logger = ord::getLogger();
  logger->redirectFileEnd();
}

void teeStringBegin()
{
  utl::Logger* logger = ord::getLogger();
  logger->teeStringBegin();
}

std::string teeStringEnd()
{
  utl::Logger* logger = ord::getLogger();
  return logger->teeStringEnd();
}

void redirectStringBegin()
{
  utl::Logger* logger = ord::getLogger();
  logger->redirectStringBegin();
}

std::string redirectStringEnd()
{
  utl::Logger* logger = ord::getLogger();
  return logger->redirectStringEnd();
}

void startPrometheusEndpoint(uint16_t port)
{
  utl::Logger* logger = ord::getLogger();
  logger->startPrometheusEndpoint(port);
}

}  // namespace utl

bool check_ip_cmd(const char* master_name,
                  bool check_all,
                  int max_polygons,
                  const char* report_file,
                  bool verbose)
{
  odb::dbDatabase* db = ord::getDb();
  sta::dbSta* sta = ord::getSta();
  utl::Logger* logger = ord::getLogger();

  utl::IpChecker checker(db, sta, logger);

  utl::IpCheckerConfig config;
  config.max_polygons = max_polygons;
  config.verbose = verbose;
  checker.setConfig(config);

  bool result;
  if (check_all) {
    result = checker.checkAll();
  } else {
    result = checker.checkMaster(master_name);
  }

  if (strlen(report_file) > 0) {
    checker.writeReport(report_file);
  }

  return result;
}
%}
