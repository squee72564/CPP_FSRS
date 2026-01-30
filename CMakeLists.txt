cmake_minimum_required(VERSION 3.16)

project(CPP_FSRS LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)


add_library(fsrs
    src/FSRS.cpp
    src/models.cpp
)

target_include_directories(fsrs
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

option(BUILD_TESTS "Build the test executable" ON)
if(BUILD_TESTS)
    enable_testing()
    find_package(nlohmann_json CONFIG REQUIRED)
    find_package(GTest CONFIG REQUIRED)

    add_executable(space_repitition_test
        tests/test_fsrs.cpp
    )

    target_link_libraries(space_repitition_test PRIVATE fsrs nlohmann_json::nlohmann_json GTest::gtest_main)
    target_include_directories(space_repitition_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)

    set_target_properties(space_repitition_test PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests
    )

    add_test(NAME fsrs_tests COMMAND space_repitition_test)
endif()
