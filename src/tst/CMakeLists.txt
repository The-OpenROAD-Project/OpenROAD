# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2025-2025, The OpenROAD Authors

include("openroad")

# DbFixture: odb-only base fixture. Has no dependency on dbSta/OpenSTA, so
# tests that only manipulate an odb database can link this and avoid the STA
# libraries entirely.
add_library(tst_base
    src/db_fixture.cpp
)

target_include_directories(tst_base
  PUBLIC
    include
)

target_link_libraries(tst_base
  PUBLIC
    db
    utl_lib
    GTest::gtest
  PRIVATE
    lefin
)

# Fixture: adds a dbSta/OpenSTA instance on top of DbFixture. Tests that need
# timing depend on this; it still avoids the full optimization tool chain
# (see tst_integrated_fixture).
add_library(tst
    src/fixture.cpp
)

target_include_directories(tst
  PUBLIC
    include
)

target_link_libraries(tst
  PUBLIC
    tst_base
    dbSta_lib
  PRIVATE
    utl_lib
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
