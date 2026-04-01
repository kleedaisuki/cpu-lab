cmake_minimum_required(VERSION 3.20)

project(cpu-lab VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(CPU_LAB_BUILD_TESTS "Build cpu-lab tests" ON)

add_library(cpu_lab_core STATIC
    src/infrastructure/timing/high_resolution_timer.cpp
    src/infrastructure/system/platform_info.cpp
    src/infrastructure/csv/row.cpp
    src/infrastructure/csv/csv_reader.cpp
    src/infrastructure/csv/csv_writer.cpp
    src/domain/shared/benchmark_result.cpp
    src/domain/shared/algorithm_orchestrator.cpp
    src/domain/matrix_dot/matrix.cpp
    src/domain/matrix_dot/vector.cpp
    src/domain/matrix_dot/matrix_generator.cpp
    src/domain/matrix_dot/matrix_dot_naive.cpp
    src/domain/matrix_dot/matrix_dot_cache.cpp
    src/domain/sum_reduce/sum_generator.cpp
    src/domain/sum_reduce/sum_naive.cpp
    src/domain/sum_reduce/sum_superscalar.cpp
    src/application/matrix_benchmark_pipeline.cpp
    src/application/sum_reduce_benchmark_pipeline.cpp
    src/interfaces/cli/cli_parser.cpp
    src/interfaces/cli/validation_service.cpp
    src/interfaces/cli/command_dispatcher.cpp
)

target_include_directories(cpu_lab_core
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

if(MSVC)
    target_compile_options(cpu_lab_core PRIVATE /W4 /permissive-)
else()
    target_compile_options(cpu_lab_core PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_executable(cpu-lab
    src/interfaces/cli/main.cpp
)

target_link_libraries(cpu-lab PRIVATE cpu_lab_core)

if(CPU_LAB_BUILD_TESTS)
    enable_testing()

    file(GLOB CPU_LAB_TEST_SOURCES CONFIGURE_DEPENDS
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/*_test.cpp
    )

    foreach(test_source IN LISTS CPU_LAB_TEST_SOURCES)
        get_filename_component(test_name ${test_source} NAME_WE)

        add_executable(${test_name}
            ${test_source}
        )

        target_link_libraries(${test_name} PRIVATE cpu_lab_core)

        if(MSVC)
            target_compile_options(${test_name} PRIVATE /W4 /permissive-)
        else()
            target_compile_options(${test_name} PRIVATE -Wall -Wextra -Wpedantic)
        endif()

        add_test(NAME ${test_name} COMMAND ${test_name})
    endforeach()
endif()
