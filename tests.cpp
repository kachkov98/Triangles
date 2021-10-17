#include "glm/gtc/random.hpp"
#include "scene.hpp"
#include <gtest/gtest.h>
#include <iostream>

TEST(Scene, RandomScene) {
  constexpr unsigned N = 1000000;
  scene::Scene triangles;
  for (unsigned i = 0; i < N; ++i) {
    glm::vec3 center = glm::linearRand(glm::vec3(-1000.f, -1000.f, -1000.f),
                                       glm::vec3(1000.f, 1000.f, 1000.f));
    triangles.emplace_back(center + glm::ballRand(1.f),
                           center + glm::ballRand(1.f),
                           center + glm::ballRand(1.f));
  }
  auto res = scene::findIntersectingTriangles(triangles);
  for (auto idx : res)
    std::cout << idx << " ";
  std::cout << std::endl;
  EXPECT_TRUE(true);
}
