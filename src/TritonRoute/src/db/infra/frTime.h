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

#ifndef _FR_TIME_H_
#define _FR_TIME_H_

#include <chrono>
#include <ctime>
#include <iostream>

#include "frBaseTypes.h"

extern size_t getPeakRSS();
extern size_t getCurrentRSS();

namespace fr {
class frTime
{
 public:
  frTime() : t0_(std::chrono::high_resolution_clock::now()), t_(clock()) {}
  std::chrono::high_resolution_clock::time_point getT0() const { return t0_; }
  void print(Logger* logger);
  bool isExceed(double in)
  {
    auto t1 = std::chrono::high_resolution_clock::now();
    auto time_span
        = std::chrono::duration_cast<std::chrono::duration<double>>(t1 - t0_);
    return (time_span.count() > in);
  }

 protected:
  std::chrono::high_resolution_clock::time_point t0_;
  clock_t t_;
};

std::ostream& operator<<(std::ostream& os, const frTime& t);
}  // namespace fr

#endif
