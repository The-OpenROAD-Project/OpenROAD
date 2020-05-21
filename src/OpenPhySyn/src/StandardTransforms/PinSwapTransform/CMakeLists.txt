project(OpenPhySynPinSwapTransform VERSION 1.0.0 LANGUAGES CXX)

set(TRANSFORM_NAME pin_swap)


add_library(${TRANSFORM_NAME}  src/PinSwapTransform.cpp)
target_compile_definitions(${TRANSFORM_NAME} INTERFACE OPENPHYSYN_TRANSFORM_BUILD)


target_link_libraries(${TRANSFORM_NAME}
PUBLIC
${OPENPHYSYB_TRANSFORMS_LINKING_LIBRARY}
OpenSTA
)

if (DEFINED "TRANSFORMS_BUILD_DIR")
set_target_properties(${TRANSFORM_NAME}
    PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY "${TRANSFORMS_BUILD_DIR}"
)
endif()
target_compile_features(${TRANSFORM_NAME} PRIVATE cxx_range_for cxx_auto_type)
target_compile_options(${TRANSFORM_NAME} PRIVATE $<$<CXX_COMPILER_ID:GNU>:-Wall>)
set_property(TARGET ${TRANSFORM_NAME} PROPERTY POSITION_INDEPENDENT_CODE ON)