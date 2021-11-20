#include "visualizer.hpp"

namespace render {

Visualizer::Visualizer(const std::string &app_name, size_t num_vertices)
    : window_(1280, 720, app_name, reinterpret_cast<void *>(&camera_)),
      camera_(glm::vec3{}, 1.f, window_.getAspectRatio()),
      renderer_(window_, app_name, num_vertices) {}

bool Visualizer::shouldClose() const { return window_.shouldClose(); }

void Visualizer::drawFrame(const VertexData &vertex_data) {
  window_.processEvents();
  renderer_.draw(window_, vertex_data, camera_.getData());
}

} // namespace render
