include("openroad")

tcl_lib(NAME          opendbtcl
        NAMESPACE     odb
        I_FILE        ../common/opendb.i
        SWIG_INCLUDES ${PROJECT_SOURCE_DIR}/include/opendb
                      ${PROJECT_SOURCE_DIR}/src/swig/tcl
)

target_include_directories(opendbtcl
    PUBLIC
        ${PROJECT_SOURCE_DIR}/include/opendb
)

target_link_libraries(opendbtcl
    PUBLIC
        opendb
        utility
        tcl
        cdl
        opendb_swig_common
        Boost::boost
)

# Executable
add_executable(opendbtcl-bin
    main.cpp
)

target_include_directories(opendbtcl-bin
    PUBLIC
        ${PROJECT_SOURCE_DIR}/include/opendb
        ${TCL_INCLUDE_PATH}
)

target_link_libraries(opendbtcl-bin
    PUBLIC
        opendbtcl
)

set_target_properties(opendbtcl-bin
    PROPERTIES 
        OUTPUT_NAME opendbtcl
)
