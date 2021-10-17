cmake_minimum_required(VERSION 3.10)
project(triangles)
include(GoogleTest)

add_subdirectory(3rdparty/glm)
add_subdirectory(3rdparty/googletest)

set(LIBRARY_NAME collisions)
add_library(${LIBRARY_NAME} STATIC "geometry.cpp" "scene.cpp")
target_compile_features(collisions PUBLIC cxx_std_17)
target_link_libraries(collisions PUBLIC glm)

add_executable(${PROJECT_NAME} "main.cpp")
target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_17)
target_link_libraries(${PROJECT_NAME} PRIVATE collisions)

set(TESTS_NAME tests)
add_executable(${TESTS_NAME} "tests.cpp")
target_compile_features(${TESTS_NAME} PRIVATE cxx_std_17)
target_link_libraries(${TESTS_NAME} PRIVATE ${LIBRARY_NAME} gtest_main)
gtest_discover_tests(${TESTS_NAME})
