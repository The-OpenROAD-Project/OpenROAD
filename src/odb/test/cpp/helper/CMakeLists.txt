include(openroad)


add_library(odb_test_helper helper.cpp)


target_link_libraries(odb_test_helper
  PUBLIC
    db
    tst_base
)


target_include_directories(odb_test_helper
  PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}
)
