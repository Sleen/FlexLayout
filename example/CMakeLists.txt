project(demo)

add_executable(demo demo.cpp)

include_directories(${FlexLayout_SOURCE_DIR}/src)

add_dependencies(demo FlexLayout)
target_link_libraries(demo FlexLayout)

set(EXECUTABLE_OUTPUT_PATH ${FlexLayout_SOURCE_DIR}/bin)
