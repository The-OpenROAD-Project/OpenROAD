# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2025-2025, The OpenROAD Authors

include("openroad")

# Lightweight fixture: only needs dbSta/odb/sta. Tests that just read a
# design should depend on this and avoid pulling in the full optimization
# tool chain.
add_library(tst
    src/fixture.cpp
)

target_include_directories(tst
  PUBLIC
    include
)

target_link_libraries(tst
  PUBLIC
    dbSta_lib
  PRIVATE
    utl_lib
    db
    lefin
    OpenSTA
)

# IntegratedFixture pulls in the full optimization tool chain (ant, dpl, est,
# grt, rsz, stt). Kept separate so lightweight tst consumers don't link it.
add_library(tst_integrated_fixture
    src/IntegratedFixture.cpp
)

target_include_directories(tst_integrated_fixture
  PUBLIC
    include
)

target_link_libraries(tst_integrated_fixture
  PUBLIC
    tst
    dbSta_lib
    ant_lib
    dpl_lib
    est_lib
    grt_lib
    rsz_lib
    stt_lib
  PRIVATE
    utl_lib
    db
    OpenSTA
)
