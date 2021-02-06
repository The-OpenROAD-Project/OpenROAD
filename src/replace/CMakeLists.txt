option(USE_CIMG_LIB "Use CImg drawing library for image drawing" OFF)

if( USE_CIMG_LIB )
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Dcimg_use_jpeg=1")
  set(CIMG_HOME ${REPLACE_HOME}/module/CImg)
  add_definitions(-DENABLE_CIMG_LIB=1)
else()
endif()

# For Multithread
# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Dcimg_use_jpeg=1 -DUSE_FFT2D_PTHREADS=1 -DFFT_2D_MAX_THREADS=16 -O3 ${OpenMP_CXX_FLAGS}")

set(THREADS_PREFER_PTHREAD_FLAG ON)

set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)

find_package(Eigen3 REQUIRED)
find_package(ZLIB REQUIRED)

# for cimg
if( USE_CIMG_LIB )
  find_package(X11 REQUIRED)
  find_package(JPEG REQUIRED)
  set (CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} ${X11_LIBRARIES} ${JPEG_LIBRARIES}")
else()
endif()


set(REPLACE_WRAP ${CMAKE_CURRENT_BINARY_DIR}/replace_wrap.cpp)
set(REPLACE_TCL_INIT ${CMAKE_CURRENT_BINARY_DIR}/ReplaceTclInitVar.cc)

set (REPLACE_SRC 
  src/MakeReplace.cpp
  ${REPLACE_WRAP}
  ${REPLACE_TCL_INIT}
  src/replace.cpp
  src/initialPlace.cpp
  src/nesterovPlace.cpp
  src/placerBase.cpp
  src/nesterovBase.cpp
  src/fft.cpp
  src/fftsg.cpp
  src/fftsg2d.cpp
  src/point.cpp
  src/plot.cpp
  src/routeBase.cpp
  src/graphics.cpp
  )

set (REPLACE_HEADER
  include/replace/Replace.h
  include/replace/MakeReplace.h
)

add_custom_command(OUTPUT ${REPLACE_WRAP}
  COMMAND ${SWIG_EXECUTABLE} -tcl8 -c++ -namespace -prefix gpl -o ${REPLACE_WRAP} ${REPLACE_HOME}/src/replace.i
  WORKING_DIRECTORY ${REPLACE_HOME}
  DEPENDS ${REPLACE_HOME}/src/replace.i ${REPLACE_HOME}/include/replace/Replace.h 
)

add_custom_command(OUTPUT ${REPLACE_TCL_INIT}
  COMMAND ${OPENSTA_HOME}/etc/TclEncode.tcl ${REPLACE_TCL_INIT} replace_tcl_inits ${REPLACE_HOME}/src/replace.tcl
  WORKING_DIRECTORY ${REPLACE_HOME}/src
  DEPENDS ${REPLACE_HOME}/src/replace.tcl ${OPENSTA_HOME}/etc/TclEncode.tcl
  )

############################################################
# Create a library 
############################################################
add_library( replace
  ${REPLACE_SRC}
)
  
target_include_directories( replace
  PUBLIC
  ${REPLACE_HOME}/include
  PRIVATE
  ${REPLACE_HOME}/src

  ${FLUTE_HOME}

  ${OPENSTA_HOME}/include

  ${OPENDB_HOME}/include
  ${OPENROAD_HOME}/include
  ${FASTROUTE_HOME}/include
  ${FASTROUTE_HOME}/src/fastroute/include
  ${TCL_INCLUDE_PATH}
  flute3
  gui
)

target_link_libraries( replace
  PRIVATE
  utility
  Eigen3::Eigen
  gui
)

# CIMG_USE case
if( USE_CIMG_LIB )
  target_link_libraries( replace
    PRIVATE
    ${X11_LIBRARIES} 
    ${JPEG_LIBRARIES} 
  )
  target_include_directories( replace
    PUBLIC
    # For CImg
    ${REPLACE_HOME}/module/CImg
    ${X11_INCLUDE_DIR}
    ${JPEG_INCLUDE_DIR}
  )
endif()
