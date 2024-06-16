include(openroad)


add_library(odb_test_helper helper.cpp env.cpp)


target_link_libraries(odb_test_helper
  PRIVATE
    odb
)


target_include_directories(odb_test_helper
  PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}
)
