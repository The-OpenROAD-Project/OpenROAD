include("openroad")

swig_lib(NAME          odbpy
         NAMESPACE     odb
         LANGUAGE      python
         I_FILE        ../common/odb.i
         SWIG_INCLUDES ${PROJECT_SOURCE_DIR}/include
                       ${PROJECT_SOURCE_DIR}/src/swig/python
         SCRIPTS       ${CMAKE_CURRENT_BINARY_DIR}/odbpy.py
)

target_include_directories(odbpy
    PUBLIC
        ${PROJECT_SOURCE_DIR}/include/odb
)

target_link_libraries(odbpy
    PUBLIC
        odb
        utl
        tcl
        cdl
        odb_swig_common
        Boost::boost
)

# Executable
add_executable(odbpy-bin 
                main.cpp 
)

target_include_directories(odbpy-bin
  PUBLIC
    ${PROJECT_SOURCE_DIR}/include/odb
)

target_link_libraries(odbpy-bin
  PUBLIC
    odbpy
)

set_target_properties(odbpy-bin
    PROPERTIES 
        OUTPUT_NAME odbpy
)
