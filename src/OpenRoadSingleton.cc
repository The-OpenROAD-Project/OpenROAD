// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

// This translation unit holds the small, low-dependency core of the
// ord::OpenRoad singleton: the static application pointer and its
// accessors, plus the ord::getOpenRoad() free function used by the
// SWIG-generated TCL/Python wrappers.
//
// It is deliberately split out of OpenRoad.cc so these symbols live in
// a static archive (the `ord_app` library) rather than only in the
// `openroad` executable.  SWIG wrappers in other libraries (e.g.
// dbSta's dbStaTCL_wrap.cxx) reference ord::OpenRoad::openRoad(),
// ord::OpenRoad::getDbNetwork() and ord::getOpenRoad().  Under strict
// symbol resolution (Nix, static linking, `-Wl,-z,defs`) those
// references must be satisfiable from an archive; otherwise targets
// such as CutGTests fail to link (issue #9563).
//
// IMPORTANT: keep this file free of dependencies on the individual flow
// tools.  Its only library dependency is dbSta (for getDbNetwork()),
// which is a leaf relative to ord_app, so no library dependency cycle
// is introduced.

#include <cstdlib>
#include <iostream>

#include "db_sta/dbNetwork.hh"
#include "db_sta/dbSta.hh"
#include "ord/OpenRoad.hh"

namespace ord {

OpenRoad* OpenRoad::app_ = nullptr;

/* static */
OpenRoad* OpenRoad::openRoad()
{
  return app_;
}

/* static */
void OpenRoad::setOpenRoad(OpenRoad* app, bool reinit_ok)
{
  if (!reinit_ok && app_) {
    std::cerr << "Attempt to reinitialize the application.\n";
    exit(1);
  }
  app_ = app;
}

sta::dbNetwork* OpenRoad::getDbNetwork()
{
  return sta_->getDbNetwork();
}

// Free function used by the SWIG wrappers across tools.  Declared in the
// various *.i files; defined here so it lives in the ord_app archive.
OpenRoad* getOpenRoad()
{
  return OpenRoad::openRoad();
}

}  // namespace ord
