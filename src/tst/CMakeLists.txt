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

# LoadedDesign: one netlist loaded through one link mode, owning its own
# db/sta. Separate from tst_integrated_fixture so consumers that only need to
# read/link/write a netlist don't link the optimization tool chain.
#
# The LEC wrapper (src/lec.cpp) is deliberately NOT built here: its only
# consumers are the hierarchy conformance suites, which are bazel-only. See the
# note on them in src/dbSta/test/BUILD.
add_library(tst_loaded_design
    src/loaded_design.cpp
)

target_include_directories(tst_loaded_design
  PUBLIC
    include
)

target_link_libraries(tst_loaded_design
  PUBLIC
    tst_base
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
    tst_loaded_design
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

# The fixtures log through utl, so they contribute man3 message pages like any
# other module. SOURCE_DIR confines the scan to the library sources, matching
# the Bazel messages_txt glob.
messages(
  TARGET tst_integrated_fixture
  SOURCE_DIR src
)
