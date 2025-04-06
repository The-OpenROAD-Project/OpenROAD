// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022-2025, The OpenROAD Authors

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

void metric_float(const char* metric, const char* value)
{
  Logger* logger = getLogger();
  logger->metric(metric, std::stod(value));
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
