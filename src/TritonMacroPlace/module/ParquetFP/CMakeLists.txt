# ParquetFP CMAKE Compile Settings
project(ParquetFP
  LANGUAGES CXX 
)

############################################################
# Create a library for PARQUET
############################################################
set( PARQUET_SRC
  src/AnalytSolve.cxx
  src/baseannealer.cxx
  src/basepacking.cxx
  src/btreeanneal.cxx
  src/btreecompact.cxx
  src/btree.cxx
  src/btreeslackeval.cxx
  src/ClusterDB.cxx
  src/CommandLine.cxx
  src/DB.cxx
  src/FPcommon.cxx
  src/mixedpacking.cxx
  src/mixedpackingfromdb.cxx
  src/Net.cxx
  src/netlist.cxx
  src/Nets.cxx
  src/Node.cxx
  src/Nodes.cxx
  src/plcompact.cxx
  src/plsptobtree.cxx
  src/pltobtree.cxx
  src/PlToSP.cxx
  src/SeqPair.cxx
  src/skyline.cxx
  src/SolveMulti.cxx
  src/SPeval.cxx
)

# Unused
#  src/Annealer.h
#  src/Annealer.cxx
#  src/btreecompactsstree.cxx
#  src/btreefromsstree.cxx

set ( PARQUET_HEADER
  src/allparquet.h
  src/AnalytSolve.h
  src/Annealer.h
  src/baseannealer.h
  src/basepacking.h
  src/btreeanneal.h
  src/btreecompact.h
  src/btreecompactsstree.h
  src/btreefromsstree.h
  src/btree.h
  src/btreeslackeval.h
  src/ClusterDB.h
  src/CommandLine.h
  src/DB.h
  src/debugflags.h
  src/debug.h
  src/FPcommon.h
  src/mixedpackingfromdb.h
  src/mixedpacking.h
  src/Net.h
  src/netlist.h
  src/Nets.h
  src/Node.h
  src/Nodes.h
  src/parsers.h
  src/plcompact.h
  src/plsptobtree.h
  src/pltobtree.h
  src/PlToSP.h
  src/SeqPair.h
  src/skyline.h
  src/SolveMulti.h
  src/SPeval.h
)

add_library( ParquetFP ${PARQUET_SRC} )

# include directory settings
target_include_directories( ParquetFP
  PUBLIC
  ${PARQUET_HOME}
  )

######################################################
# install

install(FILES ${PARQUET_HEADERS} DESTINATION include)
install(TARGETS ParquetFP DESTINATION lib)
