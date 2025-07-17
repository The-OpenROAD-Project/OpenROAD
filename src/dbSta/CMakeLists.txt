# OpenROAD/src/dbSta/CMakeLists.txt

# Original dbSta build configuration
set(DBSTA_SOURCES
    dbSta.cc
    dbSta.i
    dbSta.tcl
    # ... (other existing source files)
    
    # Add your new pin checker
    pin_checker.cc
)

# Define the library
add_library(dbSta ${DBSTA_SOURCES})

# Dependencies
target_link_libraries(dbSta
    odb
    sta
    utl
)

# Install rules
install(TARGETS dbSta
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)

# Testing configuration (if needed)
if(BUILD_TESTING)
    add_subdirectory(test)
endif()
