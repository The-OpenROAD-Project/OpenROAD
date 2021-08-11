include("openroad")

swig_lib(NAME          odbtcl
         NAMESPACE     odb
         I_FILE        ../common/odb.i
         SWIG_INCLUDES ${PROJECT_SOURCE_DIR}/include
                       ${PROJECT_SOURCE_DIR}/src/swig/tcl
)

target_include_directories(odbtcl
    PUBLIC
        ${PROJECT_SOURCE_DIR}/include/odb
)

target_link_libraries(odbtcl
    PUBLIC
        odb
        utl
        tcl
        cdl
        odb_swig_common
        Boost::boost
        ${TCL_LIBRARY}
)

# Executable
add_executable(odbtcl-bin
    main.cpp
)

target_include_directories(odbtcl-bin
    PUBLIC
        ${PROJECT_SOURCE_DIR}/include/odb
)

target_link_libraries(odbtcl-bin
    PUBLIC
        odbtcl
)

set_target_properties(odbtcl-bin
    PROPERTIES 
        OUTPUT_NAME odbtcl
)
