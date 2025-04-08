// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023-2025, The OpenROAD Authors

%module dft

%{

#include "dft/Dft.hh"
#include "DftConfig.hh"
#include "ord/OpenRoad.hh"
#include "ScanArchitect.hh"
#include "ClockDomain.hh"

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

// Enum: dft::ClockEdge
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

// Enum: dft::ScanArchitectConfig::ClockMixing
%typemap(typecheck) dft::ScanArchitectConfig::ClockMixing {
  char *str = Tcl_GetStringFromObj($input, 0);
    if (strcasecmp(str, "NO_MIX") == 0) {
    $1 = 1;
  } else if (strcasecmp(str, "CLOCK_MIX") == 0) {
    $1 = 1;
  } else {
    $1 = 0;
  }
}

%typemap(in) dft::ScanArchitectConfig::ClockMixing {
  char *str = Tcl_GetStringFromObj($input, 0);
  if (strcasecmp(str, "NO_MIX") == 0) {
    $1 = dft::ScanArchitectConfig::ClockMixing::NoMix;
  } else /* other values eliminated in typecheck */ {
    $1 = dft::ScanArchitectConfig::ClockMixing::ClockMix;
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

void insert_dft()
{
  getDft()->insertDft();
}

void set_dft_config_max_length(int max_length)
{
  getDft()->getMutableDftConfig()->getMutableScanArchitectConfig()->setMaxLength(max_length);
}

void set_dft_config_max_chains(int max_chains)
{
  getDft()->getMutableDftConfig()->getMutableScanArchitectConfig()->setMaxChains(max_chains);
}

void set_dft_config_clock_mixing(dft::ScanArchitectConfig::ClockMixing clock_mixing)
{
  getDft()->getMutableDftConfig()->getMutableScanArchitectConfig()->setClockMixing(clock_mixing);
}

void set_dft_config_scan_signal_name_pattern(const char* signal_ptr, const char* pattern_ptr) {
  dft::ScanStitchConfig* config = getDft()->getMutableDftConfig()->getMutableScanStitchConfig();
  std::string_view signal(signal_ptr), pattern(pattern_ptr);
  
  if (signal == "scan_in") {
    config->setInNamePattern(pattern);
  } else if (signal == "scan_enable") {
    config->setEnableNamePattern(pattern);
  } else if (signal == "scan_out") {
    config->setOutNamePattern(pattern);
  } else {
    getLogger()->error(utl::DFT, 6, "Internal error: unrecognized signal '{}' to set a pattern for", signal); 
  }
}

void report_dft_config() {
  getDft()->reportDftConfig();
}

%}  // inline
