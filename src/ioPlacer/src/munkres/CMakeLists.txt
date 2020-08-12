cmake_minimum_required(VERSION 3.1)

project(Munkres)
set(LIB_NAME "Munkres")
set (CMAKE_CXX_STANDARD 11)

add_definitions(-DNOOA)
add_definitions(-pg)


file(GLOB_RECURSE SRC_FILES
	${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
        )


add_library(${LIB_NAME} STATIC ${SRC_FILES})

target_include_directories(${LIB_NAME} PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src/
	)

target_link_libraries(${LIB_NAME})
