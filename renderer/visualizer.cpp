#include "visualizer.hpp"

namespace render {

Visualizer::Visualizer(const std::string &app_name, const VertexData &data)
    : window_(1280, 720, app_name, reinterpret_cast<void *>(&camera_)),
      camera_(glm::vec3{}, 1.f, window_.getAspectRatio()),
      renderer_(window_, app_name, data, camera_.getData()) {}

void Visualizer::run() {
  while (!window_.shouldClose()) {
    window_.processEvents();
    renderer_.draw(window_, camera_.getData());
  }
  renderer_.waitIdle();
}

} // namespace render
