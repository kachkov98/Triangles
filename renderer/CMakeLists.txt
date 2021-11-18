set(LIBRARY_NAME renderer)

add_library(${LIBRARY_NAME} STATIC "window.cpp" "renderer.cpp" "scene.cpp" "visualizer.cpp")
add_dependencies(${LIBRARY_NAME} Shaders)
target_compile_features(${LIBRARY_NAME} PUBLIC cxx_std_17)
target_compile_definitions(${LIBRARY_NAME} PUBLIC VULKAN_HPP_DISPATCH_LOADER_DYNAMIC=1)
target_link_libraries(${LIBRARY_NAME} PRIVATE glfw Vulkan::Vulkan INTERFACE glm)

