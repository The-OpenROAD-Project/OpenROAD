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

%{

#include "utl/Logger.h"

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

%inline %{

namespace utl {

void
report(const char *msg)
{
  Logger *logger = getLogger();
  logger->report(msg);
}

void
info(utl::ToolId tool,
     int id,
     const char *msg)
{
  Logger *logger = getLogger();
  logger->info(tool, id, msg);
}

void
warn(utl::ToolId tool,
     int id,
     const char *msg)
{
  Logger *logger = getLogger();
  logger->warn(tool, id, msg);
}

void
error(utl::ToolId tool,
      int id,
      const char *msg)
{
  Logger *logger = getLogger();
  logger->error(tool, id, msg);
}

void
critical(utl::ToolId tool,
         int id,
         const char *msg)
{
  Logger *logger = getLogger();
  logger->critical(tool, id, msg);
}

void
open_metrics(const char *metrics_filename)
{
  Logger *logger = getLogger();
  logger->addMetricsSink(metrics_filename);
}

void
metric(utl::ToolId tool,
       const char *metric,
       const char *value)
{
  Logger *logger = getLogger();
  logger->metric(tool, metric, value);
}

} // namespace

%} // inline
