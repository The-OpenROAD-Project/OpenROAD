include("openroad")

add_library(opendb_swig_common
    swig_common.cpp
)

target_include_directories(opendb_swig_common
  PUBLIC
    ${PROJECT_SOURCE_DIR}/include
    ${OPENROAD_HOME}/include
    .
)

target_link_libraries(opendb_swig_common
  PUBLIC
    defin
    lefin
    lefout
    utl
)
