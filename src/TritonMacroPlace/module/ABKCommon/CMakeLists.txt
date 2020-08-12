# ABKCommon CMAKE Compile Settings
project(ABKCommon
  LANGUAGES CXX 
)

cmake_minimum_required(VERSION 2.6)

set(ABKCOMMON_HOME ${CMAKE_CURRENT_SOURCE_DIR})
message(STATUS "ABKCommon_home: ${ABKCOMMON_HOME}")

############################################################
# Create a library for ABKCOMMON
############################################################
set( ABKCOMMON_SRC
  src/abkassert.cxx
  src/abkcpunorm.cxx
  src/abkfunc.cxx
  src/abkmessagebuf.cxx
  src/abkroot.cxx
  src/abktimer.cxx
  src/crc32.cxx
  src/infolines.cxx
  src/mmapStreamBuf.cxx
  src/paramproc.cxx
  src/platfDepend.cxx
  src/r1279.cxx
  src/r250.cxx
  src/seed.cxx
  src/Tausworthe.cxx
  src/verbosity.cxx
  src/MD5/md5.cxx
)

set (ABKCOMMON_HEADERS
  src/abkassert.h
  src/abkcommon.h
  src/abkcpunorm.h
  src/abkCRC32.h
  src/abkfunc.h
  src/abkMD5.h
  src/abkmessagebuf.h
  src/abkrand.h
  src/abkseed.h
  src/abktempl.h
  src/abktimer.h
  src/abkversion.h
  src/config.h
  src/infolines.h
  src/mmapIStream.h
  src/mmapStreamBuf.h
  src/paramproc.h

  src/verbosity.h
)

add_library( ABKCommon STATIC ${ABKCOMMON_SRC} )

# include directory settings
target_include_directories( ABKCommon
  PUBLIC
  ${ABKCOMMON_HOME}
  src
  )

# add_dependencies( cABKCommonzlib cABKCommon )

######################################################
# install

install(FILES ${ABKCOMMON_HEADERS} DESTINATION include)
install(TARGETS ABKCommon DESTINATION lib)
