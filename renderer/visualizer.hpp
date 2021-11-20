#ifndef VISUALIZER_HPP
#define VISUALIZER_HPP

#include "renderer.hpp"
#include "window.hpp"

namespace render {

class Visualizer {
public:
  Visualizer(const std::string &app_name, size_t num_vertices);
  bool shouldClose() const;
  void drawFrame(const VertexData &vertex_data);

private:
  Window window_;
  Camera camera_;
  Renderer renderer_;
};

} // namespace render

#endif
