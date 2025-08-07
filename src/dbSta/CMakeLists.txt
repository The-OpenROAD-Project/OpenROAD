# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2019-2025, The OpenROAD Authors

# REMOVE THESE (UNNECESSARY)
# add_subdirectory(src)
# add_subdirectory(test)

# ADD THIS INSTEAD - Integrate with existing dbSta build
add_library(dbSta
    # ... existing dbSta files ...
    pin_checker.cc  # Add pin_checker implementation
)

target_include_directories(dbSta
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${OPENROAD_INCLUDE_DIRS}
)

target_link_libraries(dbSta
    PUBLIC
    odb
    # ... other existing dependencies ...
)

# TCL INTEGRATION (Add to existing interface)
set(DBSTA_TCL_SRCS
    ${CMAKE_CURRENT_SOURCE_DIR}/dbSta.tcl  # MODIFY THIS FILE
)

# TESTING INTEGRATION
if(BUILD_TESTING)
    # Add to existing test executable
    add_executable(test_dbSta
        # ... existing test files ...
        test/pin_checker_test.cc  # New tests
    )
    
    target_link_libraries(test_dbSta
        dbSta
        gtest
        # ... other test dependencies ...
    )
    
    add_test(
        NAME test_dbSta
        COMMAND test_dbSta
    )
endif()
