cmake_minimum_required(VERSION 3.10)
project(triangles)
include(GoogleTest)

set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
add_subdirectory(3rdparty/glfw)
include_directories(3rdparty/glfw/include)

add_subdirectory(3rdparty/glm)
include_directories(3rdparty/glm)

set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
add_subdirectory(3rdparty/googletest)


find_package(Vulkan REQUIRED)
include_directories(${Vulkan_INCLUDE_DIRS})


add_subdirectory(shaders)
include_directories(${CMAKE_CURRENT_BINARY_DIR}/shaders)
add_subdirectory(collisions)
add_subdirectory(renderer)

set(STATIC_TRIANGLES_PROJECT static-triangles)
add_executable(${STATIC_TRIANGLES_PROJECT} "static_triangles.cpp" "common.cpp")
add_dependencies(${STATIC_TRIANGLES_PROJECT} Shaders)
target_compile_features(${STATIC_TRIANGLES_PROJECT} PRIVATE cxx_std_17)
target_link_libraries(${STATIC_TRIANGLES_PROJECT} PRIVATE collisions renderer)

set(DYNAMIC_TRIANGLES_PROJECT dynamic-triangles)
add_executable(${DYNAMIC_TRIANGLES_PROJECT} "dynamic_triangles.cpp" "common.cpp")
add_dependencies(${DYNAMIC_TRIANGLES_PROJECT} Shaders)
target_compile_features(${DYNAMIC_TRIANGLES_PROJECT} PRIVATE cxx_std_17)
target_link_libraries(${DYNAMIC_TRIANGLES_PROJECT} PRIVATE collisions renderer)
