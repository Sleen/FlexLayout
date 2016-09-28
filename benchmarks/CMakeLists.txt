project(benchmarks)

add_executable(benchmarks CSSBenchmark.cpp)

include_directories(${FlexLayout_SOURCE_DIR}/src)

add_dependencies(benchmarks FlexLayout)
target_link_libraries(benchmarks FlexLayout)

set(EXECUTABLE_OUTPUT_PATH ${FlexLayout_SOURCE_DIR}/bin)
