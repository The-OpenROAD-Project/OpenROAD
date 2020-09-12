###############################################################################
##
## BSD 3-Clause License
##
## Copyright (c) 2019, University of California, San Diego.
## All rights reserved.
##
## Redistribution and use in source and binary forms, with or without
## modification, are permitted provided that the following conditions are met:
##
## * Redistributions of source code must retain the above copyright notice, this
##   list of conditions and the following disclaimer.
##
## * Redistributions in binary form must reproduce the above copyright notice,
##   this list of conditions and the following disclaimer in the documentation
##   and#or other materials provided with the distribution.
##
## * Neither the name of the copyright holder nor the names of its
##   contributors may be used to endorse or promote products derived from
##   this software without specific prior written permission.
##
## THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
## AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
## IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
## ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
## LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
## CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
## SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
## INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
## CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
## ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
## POSSIBILITY OF SUCH DAMAGE.
##
###############################################################################

cmake_minimum_required(VERSION 3.1)

project(TritonCTS)

set(BIN_NAME "TritonCTS")

set(CMAKE_CXX_STANDARD 11)

find_package(LEMON REQUIRED)

set(TRITONCTS_WRAP ${CMAKE_CURRENT_BINARY_DIR}/TritonCTSKernelTclWrap.cpp)
add_custom_command(OUTPUT ${TRITONCTS_WRAP}
        COMMAND ${SWIG_EXECUTABLE} -tcl8 -c++ -o ${TRITONCTS_WRAP} TritonCTSKernelTcl.i   
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        DEPENDS
        TritonCTSKernelTcl.i
        TritonCTSKernel.h
        ${OPENROAD_HOME}/src/Exception.i
)

set(TRITONCTS_TCL_INIT ${CMAKE_CURRENT_BINARY_DIR}/TritonCtsTclInitVar.cc)
add_custom_command(OUTPUT ${TRITONCTS_TCL_INIT}
        COMMAND ${OPENSTA_HOME}/etc/TclEncode.tcl ${TRITONCTS_TCL_INIT} tritoncts_tcl_inits ${TRITONCTS_HOME}/src/tritoncts.tcl
        WORKING_DIRECTORY ${TRITONCTS_HOME}/src
        DEPENDS ${TRITONCTS_HOME}/src/tritoncts.tcl ${OPENSTA_HOME}/etc/TclEncode.tcl
)

set(SRC_FILES
        Clock.cpp
        DbWrapper.cpp
        HTreeBuilder.cpp
        MakeTritoncts.cpp
        PostCtsOpt.cpp
        SinkClustering.cpp
        StaEngine.cpp
        TechChar.cpp
        TritonCTSKernel.cpp
        TritonCTSKernelTcl.cpp
        third_party/CKMeans/clustering.cpp
)

add_library(${BIN_NAME} ${SRC_FILES} ${TRITONCTS_WRAP} ${TRITONCTS_TCL_INIT})


include_directories(${TRITONCTS_HOME}/src
                    PRIVATE
                    ${OPENDB_HOME}/include
                    ${OPENDB_HOME}/include/opendb
                    ${OPENROAD_HOME}/include
                    ${DBSTA_HOME}/include
                    ${OPENSTA_HOME}/include
		    ${TCL_INCLUDE_PATH}
                    ${LEMON_INCLUDE_DIRS}
                   ) 

target_link_libraries(${BIN_NAME} INTERFACE
                      ${TCL_LIB}
                      lefin
                      opendb
                     )
