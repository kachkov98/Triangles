cmake_minimum_required(VERSION 3.8)
project(triangles)

add_subdirectory(3rdparty/glm)
add_subdirectory(3rdparty/googletest)

set(SOURCES main.cpp)

add_executable(${PROJECT_NAME} ${SOURCES})
target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_17)
target_link_libraries(${PROJECT_NAME} PRIVATE glm)
