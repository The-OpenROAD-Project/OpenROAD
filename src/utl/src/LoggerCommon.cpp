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

#include "LoggerCommon.h"

#include "utl/Logger.h"

namespace ord {
// Defined in OpenRoad.i
utl::Logger* getLogger();
}  // namespace ord

namespace utl {

using ord::getLogger;

void report(const char* msg)
{
  Logger* logger = getLogger();
  logger->report(msg);
}

void info(utl::ToolId tool, int id, const char* msg)
{
  Logger* logger = getLogger();
  logger->info(tool, id, "{}", msg);
}

void warn(utl::ToolId tool, int id, const char* msg)
{
  Logger* logger = getLogger();
  logger->warn(tool, id, "{}", msg);
}

void error(utl::ToolId tool, int id, const char* msg)
{
  Logger* logger = getLogger();
  logger->error(tool, id, "{}", msg);
}

void critical(utl::ToolId tool, int id, const char* msg)
{
  Logger* logger = getLogger();
  logger->critical(tool, id, "{}", msg);
}

void open_metrics(const char* metrics_filename)
{
  Logger* logger = getLogger();
  logger->addMetricsSink(metrics_filename);
}

void close_metrics(const char* metrics_filename)
{
  Logger* logger = getLogger();
  logger->removeMetricsSink(metrics_filename);
}

void metric(const char* metric, const char* value)
{
  Logger* logger = getLogger();
  logger->metric(metric, value);
}

void metric_integer(const char* metric, const int value)
{
  Logger* logger = getLogger();
  logger->metric(metric, value);
}

void metric_float(const char* metric, const double value)
{
  Logger* logger = getLogger();
  logger->metric(metric, value);
}

void set_metrics_stage(const char* fmt)
{
  Logger* logger = getLogger();
  logger->setMetricsStage(fmt);
}

void clear_metrics_stage()
{
  Logger* logger = getLogger();
  logger->clearMetricsStage();
}

void push_metrics_stage(const char* fmt)
{
  Logger* logger = getLogger();
  logger->pushMetricsStage(fmt);
}

std::string pop_metrics_stage()
{
  Logger* logger = getLogger();
  return logger->popMetricsStage();
}

void suppress_message(utl::ToolId tool, int id)
{
  Logger* logger = getLogger();
  logger->suppressMessage(tool, id);
}

void unsuppress_message(utl::ToolId tool, int id)
{
  Logger* logger = getLogger();
  logger->unsuppressMessage(tool, id);
}

}  // namespace utl
