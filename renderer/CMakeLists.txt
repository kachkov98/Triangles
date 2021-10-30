set(LIBRARY_NAME renderer)
add_library(${LIBRARY_NAME} STATIC "window.cpp" "renderer.cpp")
target_compile_features(${LIBRARY_NAME} PUBLIC cxx_std_17)
target_link_libraries(${LIBRARY_NAME} PRIVATE glfw Vulkan::Vulkan INTERFACE glm)

