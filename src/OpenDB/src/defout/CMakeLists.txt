add_library(defout
    defout.cpp
    defout_impl.cpp
)

target_include_directories(defout
    PUBLIC
        $<INSTALL_INTERFACE:include/opendb>
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include/opendb>
        ${TCL_INCLUDE_PATH}
    PRIVATE
        ${PROJECT_SOURCE_DIR}/src/defout
)

target_compile_features(defout PRIVATE cxx_auto_type)
target_compile_options(defout PRIVATE $<$<CXX_COMPILER_ID:GNU>:-Wall>)
set_property(TARGET defout PROPERTY POSITION_INDEPENDENT_CODE ON)
