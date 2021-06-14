/* Authors: Lutong Wang and Bangqi Xu */
/*
 * Copyright (c) 2019, The Regents of the University of California
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the University nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE REGENTS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "frTime.h"

#include <boost/io/ios_state.hpp>
#include <iomanip>

using namespace std;
using namespace fr;

void frTime::print(Logger* logger)
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
               getCurrentRSS() / (1024.0 * 1024.0),
               getPeakRSS() / (1024.0 * 1024.0));
}

namespace fr {

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
     << getCurrentRSS() * 1.0 / 1024 / 1024 << " (MB)";
  guard.restore();
  return os;
}

}  // end namespace fr
