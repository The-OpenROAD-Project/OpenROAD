include("openroad")

tcl_lib(NAME          opendbtcl
        NAMESPACE     odb
        I_FILE        opendbtcl.i
        SWIG_INCLUDES ${PROJECT_SOURCE_DIR}/include/opendb
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
