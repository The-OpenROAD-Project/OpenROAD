# Include python
find_package(Python3 COMPONENTS Development REQUIRED)

if (NOT (PythonLibs_FOUND OR Python3_INCLUDE_DIRS))
    message(STATUS "Could not find python, disabling Python SWIG Module")
    return()
endif()
message(STATUS "Enabling OpenDB Python SWIG Module")

include_directories(${Python3_INCLUDE_DIRS})

set(OPENDB_PYTHON_SWIG_FILES
    ${PROJECT_SOURCE_DIR}/src/swig/common/opendb.i
)

set_source_files_properties(${OPENDB_PYTHON_SWIG_FILES} 
    PROPERTIES SWIG_MODULE_NAME    opendbpy
               CPLUSPLUS           ON
               INCLUDE_DIRECTORIES ${PROJECT_SOURCE_DIR}/src/swig/python
)

include_directories(${PROJECT_SOURCE_DIR}/include/opendb)

set(CMAKE_SWIG_FLAGS 
    "-w509,503,501,472,467,402,401,317,325,378,383,389,365,362,314,258,240,203,201")
# Add swig module
swig_add_library(opendbpy 
    TYPE     STATIC
    LANGUAGE python 
    SOURCES  ${OPENDB_PYTHON_SWIG_FILES}
)
swig_link_libraries(opendbpy
    PUBLIC
        opendb
        opendb_swig_common
        utility
        Python3::Python
)
add_executable(opendbpy-bin 
                main.cpp 
)
target_link_libraries(opendbpy-bin
    PUBLIC
    opendbpy
    ${PYTHON_LIBRARIES}

)
target_include_directories(opendbpy-bin
    PUBLIC
        ${PROJECT_SOURCE_DIR}/include/opendb
        ${PYTHON_INCLUDE_DIRS}
)

set_target_properties(opendbpy-bin
    PROPERTIES 
        OUTPUT_NAME opendbpy
)
