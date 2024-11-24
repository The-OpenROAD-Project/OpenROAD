# Copyright (c) 2024, The Regents of the University of 
# California
# All rights reserved.
# 
# This program is free software: you can redistribute it 
# and/or modify it under the terms of the GNU General 
# Public License as published by the Free Software 
# Foundation, either version 3 of the License, or (at your 
# option) any later version.
# 
# This program is distributed in the hope that it will be 
# useful, but WITHOUT ANY WARRANTY; without even the 
# implied warranty of MERCHANTABILITY or FITNESS FOR A 
# PARTICULAR PURPOSE. See the GNU General Public License 
# for more details.
# 
# You should have received a copy of the GNU General 
# Public License along with this program. If not, see 
# <https://www.gnu.org/licenses/>.
include("openroad")

project(ora)

swig_lib(NAME      ora
         NAMESPACE ora
         I_FILE    src/Ora.i
         SCRIPTS   src/Ora.tcl
)

target_sources(ora
  PRIVATE
  src/MakeOra.cc
  src/Ora.cc
)

target_include_directories(ora
  PUBLIC
    include
)

find_package(CURL REQUIRED)
find_package(Boost REQUIRED COMPONENTS json) 

target_include_directories(ora
    PUBLIC ${CURL_INCLUDE_DIRS}
    PUBLIC ${Boost_INCLUDE_DIRS}
)

target_link_libraries(ora
    OpenSTA
    odb
    ${CURL_LIBRARIES}
    Boost::json
)

messages(
  TARGET ora
)

