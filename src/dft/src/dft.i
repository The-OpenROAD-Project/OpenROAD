///////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// Copyright (c) 2023, Google LLC
// All rights reserved.
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

%module dft

%{

#include "dft/Dft.hh"
#include "DftConfig.hh"
#include "ord/OpenRoad.hh"
#include "ScanArchitect.hh"
#include "ClockDomain.hh"
#include "ResetDomain.hh"

dft::Dft * getDft()
{
  return ord::OpenRoad::openRoad()->getDft();
}

utl::Logger* getLogger()
{
  return ord::OpenRoad::openRoad()->getLogger();
}

%}

%include "../../Exception.i"

%typemap(typecheck) dft::ResetActiveEdge {
  char *str = Tcl_GetStringFromObj($input, 0);
    if (strcasecmp(str, "LOW") == 0) {
    $1 = 1;
  } else if (strcasecmp(str, "HIGH") == 0) {
    $1 = 1;
  } else {
    $1 = 0;
  }
}

%typemap(in) dft::ResetActiveEdge {
  char *str = Tcl_GetStringFromObj($input, 0);
  if (strcasecmp(str, "LOW") == 0) {
    $1 = dft::ResetActiveEdge::Low;
  } else /* other values eliminated in typecheck */ {
    $1 = dft::ResetActiveEdge::High;
  };
}

%typemap(typecheck) dft::ClockEdge {
  char *str = Tcl_GetStringFromObj($input, 0);
    if (strcasecmp(str, "RISING") == 0) {
    $1 = 1;
  } else if (strcasecmp(str, "FALLING") == 0) {
    $1 = 1;
  } else {
    $1 = 0;
  }
}

%typemap(in) dft::ClockEdge {
  char *str = Tcl_GetStringFromObj($input, 0);
  if (strcasecmp(str, "FALLING") == 0) {
    $1 = dft::ClockEdge::Falling;
  } else /* other values eliminated in typecheck */ {
    $1 = dft::ClockEdge::Rising;
  };
}

%inline
%{

void preview_dft(bool verbose)
{
  getDft()->previewDft(verbose);
}

void scan_replace()
{
  getDft()->scanReplace();
}

void insert_dft(bool per_chain_enable,
                const char* scan_enable_name_pattern,
                const char* scan_in_name_pattern,
                const char* scan_out_name_pattern)
{
  getDft()->insertDft(per_chain_enable, scan_enable_name_pattern, scan_in_name_pattern, scan_out_name_pattern);
}

void write_scan_chains(const char* filename_p)
{
  std::string filename(filename_p);
  getDft()->writeScanChains(filename);
}

void set_dft_config_max_length(int max_length)
{
  getDft()->getMutableDftConfig()->getMutableScanArchitectConfig()->setMaxLength(max_length);
}

void set_dft_config_max_chains(int max_chains)
{
  getDft()->getMutableDftConfig()->getMutableScanArchitectConfig()->setMaxChains(max_chains);
}

void set_dft_config_clock_mixing(const char* clock_mixing_ptr)
{
  std::string_view clock_mixing(clock_mixing_ptr);
  if (clock_mixing == "no_mix") {
    getDft()->getMutableDftConfig()->getMutableScanArchitectConfig()->setClockMixing(dft::ScanArchitectConfig::ClockMixing::NoMix);
  } else if (clock_mixing == "clock_mix") {
    getDft()->getMutableDftConfig()->getMutableScanArchitectConfig()->setClockMixing(dft::ScanArchitectConfig::ClockMixing::ClockMix);
  } else {
    getLogger()->error(utl::DFT, 6, "Requested clock mixing config not valid");
  }
}

void report_dft_config() {
  getDft()->reportDftConfig();
}

%}  // inline
