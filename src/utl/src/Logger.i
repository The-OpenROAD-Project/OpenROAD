// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2020-2025, The OpenROAD Authors

%{

#include "utl/Logger.h"
#include "utl/OptionsRegistry.h"
#include "utl/OptionsTcl.h"
#include "utl/Validate.h"
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

// A %inline function parameter named `interp` is filled in by SWIG and is
// not an argument of the Tcl command.
%typemap(in, numinputs=0) Tcl_Interp* interp {
  $1 = interp;
}
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

// The options struct registered for a command.
static const utl::SchemaBase& findOptions(const std::string& command)
{
  const utl::SchemaBase* schema = utl::OptionsRegistry::get().find(command);
  if (schema == nullptr) {
    std::string names;
    for (const std::string& name : utl::OptionsRegistry::get().commands()) {
      names += " " + name;
    }
    ord::getLogger()->error(utl::UTL,
                            110,
                            "No options are registered for {}; commands with "
                            "options:{}",
                            command,
                            names);
  }
  return *schema;
}

std::string describe_options_cmd(const std::string& command, bool markdown)
{
  const utl::SchemaBase& schema = findOptions(command);
  return markdown ? utl::describeMarkdown(schema) : utl::describeText(schema);
}

std::string default_options_cmd(const std::string& command, Tcl_Interp* interp)
{
  return utl::jsonToTclString(interp, findOptions(command).defaults());
}

std::string report_options_cmd(const std::string& command,
                               Tcl_Obj* options,
                               Tcl_Interp* interp)
{
  const utl::SchemaBase& schema = findOptions(command);
  utl::Logger* logger = ord::getLogger();
  const boost::json::object given
      = utl::tclToJson(interp, options, schema, {}, logger);
  return utl::reportValues(schema, schema.resolve(given, logger));
}

} // namespace

%} // inline
