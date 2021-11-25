#ifndef COMMON_HPP
#define COMMON_HPP

#include "collisions/scene.hpp"
#include "renderer/visualizer.hpp"

render::VertexData getVertexData(const scene::Scene &scene,
                                 const scene::Collisions &collisions);

#endif
