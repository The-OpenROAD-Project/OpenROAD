// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022-2025, The OpenROAD Authors

#ifdef BAZEL
%{

#include <cstdlib>

#include "utl/Logger.h"
#include "utl/StackTrace.h"
%}
#else
%{
  
#include <cstdlib>

#include "ord/OpenRoad.hh"
#include "utl/Logger.h"
#include "utl/StackTrace.h"
%}
#endif

%exception {
  try { $action }
  catch (std::bad_alloc &) {
    fprintf(stderr, "Error: out of memory.");
    abort();
  }
  // This catches std::runtime_error (utl::error) and sta::Exception.
  catch (std::exception &excp) {
    #ifdef BAZEL
    utl::Logger* logger = utl::Logger::defaultLogger();
    if (logger->debugCheck(utl::ORD, "trace", 1)) {
      logger->report("Stack trace");
      logger->report(utl::stackTrace());
    }
    #else
    auto* openroad = ord::OpenRoad::openRoad();
    if (openroad != nullptr) {
      auto* logger = openroad->getLogger();
      if (logger->debugCheck(utl::ORD, "trace", 1)) {
        logger->report("Stack trace");
        logger->report(utl::stackTrace());
      }
    }
    #endif

    PyErr_SetString(PyExc_RuntimeError, excp.what());
    SWIG_fail;
  }
  catch (...) {
    #ifdef BAZEL
    utl::Logger* logger = utl::Logger::defaultLogger();
    if (logger->debugCheck(utl::ORD, "trace", 1)) {
      logger->report("Stack trace");
      logger->report(utl::stackTrace());
    }
    #else
    auto* openroad = ord::OpenRoad::openRoad();
    if (openroad != nullptr) {
      auto* logger = openroad->getLogger();
      if (logger->debugCheck(utl::ORD, "trace", 1)) {
        logger->report("Stack trace");
        logger->report(utl::stackTrace());
      }
    }
    #endif

    PyErr_SetString(PyExc_Exception, "Unknown exception");
    SWIG_fail;
  }      
}
