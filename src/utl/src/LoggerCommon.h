// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022-2025, The OpenROAD Authors

#include <cstdint>
#include <string>

#include "utl/Logger.h"

namespace utl {

void report(const char* msg);
void info(utl::ToolId tool, int id, const char* msg);
void warn(utl::ToolId tool, int id, const char* msg);
void error(utl::ToolId tool, int id, const char* msg);
void critical(utl::ToolId tool, int id, const char* msg);
void open_metrics(const char* metrics_filename);
void close_metrics(const char* metrics_filename);
void metric(const char* metric, const char* value);
void metric_integer(const char* metric, int64_t value);
void metric_float(const char* metric, double value);
void metric_float(const char* metric, const char* value);
void set_metrics_stage(const char* fmt);
void clear_metrics_stage();
void push_metrics_stage(const char* fmt);
std::string pop_metrics_stage();
void suppress_message(utl::ToolId tool, int id);
void unsuppress_message(utl::ToolId tool, int id);

}  // namespace utl
