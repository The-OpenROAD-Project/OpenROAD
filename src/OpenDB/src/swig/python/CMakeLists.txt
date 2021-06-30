include("openroad")

swig_lib(NAME          opendbpy
         NAMESPACE     odb
         LANGUAGE      python
         I_FILE        ../common/opendb.i
         SWIG_INCLUDES ${PROJECT_SOURCE_DIR}/include/opendb
                       ${PROJECT_SOURCE_DIR}/src/swig/python
         SCRIPTS       ${CMAKE_CURRENT_BINARY_DIR}/opendbpy.py
)

target_include_directories(opendbpy
    PUBLIC
        ${PROJECT_SOURCE_DIR}/include/opendb
)

target_link_libraries(opendbpy
    PUBLIC
        opendb
        utl
        tcl
        cdl
        opendb_swig_common
        Boost::boost
)
