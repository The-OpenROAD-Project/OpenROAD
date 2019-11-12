# Copyright (c) 2019, Parallax Software, Inc.
# 
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
# 
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>.

set(RESIZER_SWIG ${CMAKE_CURRENT_BINARY_DIR}/Resizer_wrap.cc)

add_custom_command(OUTPUT ${RESIZER_SWIG}
  COMMAND ${SWIG_EXECUTABLE} -tcl8 -c++ -namespace -prefix sta -I${OPENROAD_HOME}/include -o ${RESIZER_SWIG} ${RESIZER_HOME}/src/Resizer.i
  COMMAND ${OPENSTA_HOME}/etc/SwigCleanup.tcl ${CMAKE_CURRENT_BINARY_DIR}/Resizer_wrap.cc
  WORKING_DIRECTORY ${RESIZER_HOME}/src
  DEPENDS ${RESIZER_HOME}/src/Resizer.i
  )

add_library(resizer
  Resizer.cc
  SteinerTree.cc
  ${RESIZER_SWIG}
  )

target_include_directories(resizer
  PUBLIC ${RESIZER_HOME}/include
  PRIVATE ${OPENROAD_HOME}/include ${STA_INCLUDE_DIRS} ${OPENDB_HOME}/include ${OPENROAD_HOME}/src/flute3
  )
