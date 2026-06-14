// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2020-2025, The OpenROAD Authors

%{

#include "utl/Logger.h"
#include "LoggerCommon.h"
    
namespace ord {
// Defined in OpenRoad.i
utl::Logger *
getLogger();
}

using utl::ToolId;
using utl::Logger;
using ord::getLogger;

#if TCL_MAJOR_VERSION < 9 && !defined(Tcl_Size)
  typedef int Tcl_Size;
#endif

%}

%typemap(in) utl::ToolId {
  Tcl_Size length;
  const char *arg = Tcl_GetStringFromObj($input, &length);
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

// --- Database logging controls ---

void start_log_db(const std::string& filename)
{
  utl::Logger* logger = ord::getLogger();
  logger->startLogDb(filename.c_str());
}

void stop_log_db()
{
  utl::Logger* logger = ord::getLogger();
  logger->stopLogDb();
}

void set_db_log_global_max_mem(size_t bytes)
{
  utl::Logger* logger = ord::getLogger();
  logger->setDbLogGlobalMaxMem(bytes);
}

size_t get_db_log_global_max_mem()
{
  utl::Logger* logger = ord::getLogger();
  return logger->getDbLogGlobalMaxMem();
}

void set_db_log_per_channel_max_mem(size_t bytes)
{
  utl::Logger* logger = ord::getLogger();
  logger->setDbLogPerChannelMaxMem(bytes);
}

size_t get_db_log_per_channel_max_mem()
{
  utl::Logger* logger = ord::getLogger();
  return logger->getDbLogPerChannelMaxMem();
}

void set_db_log_enabled(const std::string& tool, int id, bool enabled)
{
  utl::Logger* logger = ord::getLogger();
  logger->setDbLogEnabled(utl::Logger::findToolId(tool.c_str()), id, enabled);
}

bool get_db_log_enabled(const std::string& tool, int id)
{
  utl::Logger* logger = ord::getLogger();
  return logger->getDbLogEnabled(utl::Logger::findToolId(tool.c_str()), id);
}

} // namespace

%} // inline
