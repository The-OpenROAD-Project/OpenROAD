add_library(lef_checker
    LefChecker.cc
)

target_include_directories(lef_checker
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(lef_checker
    PUBLIC
    odb
)