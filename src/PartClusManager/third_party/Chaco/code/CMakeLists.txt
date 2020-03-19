
cmake_minimum_required (VERSION 3.1)

project (Chaco)

set(LIB_NAME "Chaco")

set(CMAKE_CXX_STANDARD 11)

file(GLOB_RECURSE SRC_FILES ${CMAKE_CURRENT_SOURCE_DIR}/*.c)

add_definitions(-DLIBRARY_MODE)
add_LIBRARY(${LIB_NAME} STATIC ${SRC_FILES})

target_include_directories(${LIB_NAME} PUBLIC
                           ${CMAKE_CURRENT_SOURCE_DIR}/main
                           ${CMAKE_CURRENT_SOURCE_DIR}/assign
                           ${CMAKE_CURRENT_SOURCE_DIR}/bpmatch
                           ${CMAKE_CURRENT_SOURCE_DIR}/coarsen
                           ${CMAKE_CURRENT_SOURCE_DIR}/connect
                           ${CMAKE_CURRENT_SOURCE_DIR}/eigen
                           ${CMAKE_CURRENT_SOURCE_DIR}/graph
                           ${CMAKE_CURRENT_SOURCE_DIR}/inertial
                           ${CMAKE_CURRENT_SOURCE_DIR}/input
                           ${CMAKE_CURRENT_SOURCE_DIR}/internal
                           ${CMAKE_CURRENT_SOURCE_DIR}/klspiff
                           ${CMAKE_CURRENT_SOURCE_DIR}/klvspiff
                           ${CMAKE_CURRENT_SOURCE_DIR}/mainmatlab
                           ${CMAKE_CURRENT_SOURCE_DIR}/misc
                           ${CMAKE_CURRENT_SOURCE_DIR}/optimize
                           ${CMAKE_CURRENT_SOURCE_DIR}/refine_map
                           ${CMAKE_CURRENT_SOURCE_DIR}/refine_part
                           ${CMAKE_CURRENT_SOURCE_DIR}/submain
                           ${CMAKE_CURRENT_SOURCE_DIR}/symmlq
                           ${CMAKE_CURRENT_SOURCE_DIR}/tinvit
                           ${CMAKE_CURRENT_SOURCE_DIR}/util)

target_link_libraries(${LIB_NAME})
