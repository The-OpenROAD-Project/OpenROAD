# Copyright (c) 2021, OpenRoad Project
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

set(LOGGER_WRAP ${CMAKE_CURRENT_BINARY_DIR}/Logger_wrap.cc)

add_custom_command(OUTPUT ${LOGGER_WRAP}
  COMMAND ${SWIG_EXECUTABLE} -tcl8 -c++ -namespace -prefix utl -I${OPENROAD_HOME}/include -o ${LOGGER_WRAP} ${UTILITY_HOME}/src/Logger.i
  COMMAND ${OPENSTA_HOME}/etc/SwigCleanup.tcl ${LOGGER_WRAP}
  WORKING_DIRECTORY ${UTILITY_HOME}/src
  DEPENDS
  ${UTILITY_HOME}/src/Logger.i
  ${UTILITY_HOME}/include/utility/Logger.h
  )

############################################################
# Create a library 
############################################################
add_library( utility
  src/Logger.cpp
  src/MakeLogger.cpp
  ${LOGGER_WRAP}
)
  
target_include_directories( utility
  PUBLIC
  ${UTILITY_HOME}/include
  ${TCL_INCLUDE_PATH}
)

target_link_libraries( utility
  PUBLIC
  spdlog::spdlog
)
