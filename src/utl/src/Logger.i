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

%}

%typemap(in) utl::ToolId {
  int length;
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

} // namespace

%} // inline
