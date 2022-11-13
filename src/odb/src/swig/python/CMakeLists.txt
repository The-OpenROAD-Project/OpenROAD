include("openroad")

swig_lib(NAME          odb_py
         NAMESPACE     odb
         LANGUAGE      python
         I_FILE        ../common/odb.i
         SWIG_INCLUDES ${PROJECT_SOURCE_DIR}/include
                       ${PROJECT_SOURCE_DIR}/src/swig/python
         SCRIPTS       ${CMAKE_CURRENT_BINARY_DIR}/odb_py.py
)

target_include_directories(odb_py
    PUBLIC
        ${PROJECT_SOURCE_DIR}/include/odb
)

target_link_libraries(odb_py
    PUBLIC
        odb
        utl
        ${TCL_LIBRARY}
        cdl
        odb_swig_common
        Boost::boost
)

# Executable
add_executable(odb_py-bin 
                main.cpp 
)

target_include_directories(odb_py-bin
  PUBLIC
    ${PROJECT_SOURCE_DIR}/include/odb
)

target_link_libraries(odb_py-bin
  PUBLIC
    odb_py
)

set_target_properties(odb_py-bin
    PROPERTIES 
        OUTPUT_NAME odb_py
)
