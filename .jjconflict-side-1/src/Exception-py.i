// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022-2025, The OpenROAD Authors

#ifdef BAZEL
%{

#include <boost/stacktrace.hpp>
#include <cstdlib>
#include <sstream>

#include "utl/Logger.h"
%}
#else
%{

#include <boost/stacktrace.hpp>
#include <cstdlib>
#include <sstream>

#include "ord/OpenRoad.hh"
#include "utl/Logger.h"
%}
#endif

%exception {
  try { $function }
  catch (std::bad_alloc &) {
    fprintf(stderr, "Error: out of memory.");
    abort();
  }
  // This catches std::runtime_error (utl::error) and sta::Exception.
  catch (std::exception &excp) {
    #ifdef BAZEL
    Logger* logger = Logger::defaultLogger();
    if (logger->debugCheck(utl::ORD, "trace", 1)) {
      std::stringstream trace;
      trace << boost::stacktrace::stacktrace();
      logger->report("Stack trace");
      logger->report(trace.str());
    }
    #else
    auto* openroad = ord::OpenRoad::openRoad();
    if (openroad != nullptr) {
      auto* logger = openroad->getLogger();
      if (logger->debugCheck(utl::ORD, "trace", 1)) {
        std::stringstream trace;
        trace << boost::stacktrace::stacktrace();
        logger->report("Stack trace");
        logger->report(trace.str());
      }
    }
    #endif

    PyErr_SetString(PyExc_RuntimeError, excp.what());
    SWIG_fail;
  }
  catch (...) {
    #ifdef BAZEL
    Logger* logger = Logger::defaultLogger();
    if (logger->debugCheck(utl::ORD, "trace", 1)) {
      std::stringstream trace;
      trace << boost::stacktrace::stacktrace();
      logger->report("Stack trace");
      logger->report(trace.str());
    }
    #else
    auto* openroad = ord::OpenRoad::openRoad();
    if (openroad != nullptr) {
      auto* logger = openroad->getLogger();
      if (logger->debugCheck(utl::ORD, "trace", 1)) {
        std::stringstream trace;
        trace << boost::stacktrace::stacktrace();
        logger->report("Stack trace");
        logger->report(trace.str());
      }
    }
    #endif

    PyErr_SetString(PyExc_Exception, "Unknown exception");
    SWIG_fail;
  }      
}
