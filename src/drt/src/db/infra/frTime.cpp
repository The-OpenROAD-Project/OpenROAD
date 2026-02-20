// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "db/infra/frTime.h"

#include <chrono>
#include <iomanip>
#include <ios>
#include <iostream>
#include <ostream>

#include "boost/io/ios_state.hpp"
#include "frBaseTypes.h"
#include "utl/Logger.h"
#include "utl/mem_stats.h"

namespace drt {

void frTime::print(utl::Logger* logger)
{
  auto t1 = std::chrono::high_resolution_clock::now();
  auto time_span = std::chrono::duration_cast<std::chrono::seconds>(t1 - t0_);
  int hour = time_span.count() / 3600;
  int min = (time_span.count() % 3600) / 60;
  int sec = time_span.count() % 60;
  auto t2 = (clock() - t_) / CLOCKS_PER_SEC;
  int chour = t2 / 3600;
  int cmin = (t2 % 3600) / 60;
  int csec = t2 % 60;
  logger->info(DRT,
               267,
               "cpu time = {:02}:{:02}:{:02}, "
               "elapsed time = {:02}:{:02}:{:02}, "
               "memory = {:.2f} (MB), peak = {:.2f} (MB)",
               chour,
               cmin,
               csec,
               hour,
               min,
               sec,
               utl::getCurrentRSS() / (1024.0 * 1024.0),
               utl::getPeakRSS() / (1024.0 * 1024.0));
}

std::ostream& operator<<(std::ostream& os, const frTime& t)
{
  boost::io::ios_all_saver guard(std::cout);
  auto t1 = std::chrono::high_resolution_clock::now();
  auto time_span
      = std::chrono::duration_cast<std::chrono::seconds>(t1 - t.getT0());
  int hour = time_span.count() / 3600;
  int min = (time_span.count() % 3600) / 60;
  int sec = time_span.count() % 60;
  os << "elapsed time = ";
  os << std::setw(2) << std::setfill('0') << hour;
  os << ":";
  os << std::setw(2) << std::setfill('0') << min;
  os << ":";
  os << std::setw(2) << std::setfill('0') << sec;
  os << ", memory = " << std::fixed << std::setprecision(2)
     << utl::getCurrentRSS() * 1.0 / 1024 / 1024 << " (MB)";
  guard.restore();
  return os;
}

}  // end namespace drt
