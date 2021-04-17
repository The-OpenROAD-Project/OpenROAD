
include("openroad")

swig_lib(NAME      pdn
         NAMESPACE pdn
         I_FILE    PdnGen.i
         SCRIPTS   PdnGen.tcl
)

target_sources(pdn
  PRIVATE
    PdnGen.cc
    MakePdnGen.cc
)

target_include_directories(pdn
  PUBLIC
    ../include
)

target_link_libraries(pdn
  PRIVATE
    opendb
    utl
    OpenSTA
)
