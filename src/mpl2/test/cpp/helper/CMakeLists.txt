include(openroad)

add_library(mpl2_odb_helper helper.cpp)

target_link_libraries(mpl2_odb_helper
  PRIVATE
    mpl2_lib
)

target_include_directories(mpl2_odb_helper
  PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}
)