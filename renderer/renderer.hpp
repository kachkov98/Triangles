#ifndef RENDERER_HPP
#define RENDERER_HPP

#include <string>
#include <vector>
#include <vulkan/vulkan.hpp>

namespace render {

class Window;

class Renderer {
public:
  Renderer(const Window &window, const std::string &app_name);
  void resize(const Window &window);
  void draw(const Window &window);
  void waitIdle() const { device_->waitIdle(); }

private:
  vk::UniqueInstance instance_;
#ifndef NDEBUG
  vk::UniqueDebugUtilsMessengerEXT messenger_;
#endif
  vk::UniqueSurfaceKHR surface_;
  vk::PhysicalDevice physical_device_;
  uint32_t graphics_queue_family_index_, present_queue_family_index_;
  vk::UniqueDevice device_;
  vk::Queue graphics_queue_, present_queue_;
  vk::UniqueCommandPool command_pool_;
  vk::UniqueSemaphore image_available_semaphore_, render_finished_semaphore_;

  vk::Format format_;
  vk::ColorSpaceKHR color_space_;
  vk::Extent2D extent_;
  unsigned image_count_;
  vk::UniqueSwapchainKHR swapchain_;
  std::vector<vk::Image> swapchain_images_;
  std::vector<vk::UniqueImageView> swapchain_image_views_;

  vk::UniqueRenderPass render_pass_;
  vk::UniquePipelineLayout pipeline_layout_;
  vk::UniquePipeline pipeline_;
  std::vector<vk::UniqueFramebuffer> framebuffers_;
  std::vector<vk::UniqueCommandBuffer> command_buffers_;

  void createInstance(const Window &window, const std::string &app_name);
  void createDebugCallback();
  void selectPhysicalDevice();
  void createDevice();
  void createQueues();
  void createCommandPool();
  void createSemaphores();
  void createSwapchain(const Window &window);
  void createRenderPass();
  void createPipeline();
  void createFramebuffers();
  void createCommandBuffers();
  void recordCommandBuffers();

  std::vector<const char *> getInstanceExtensions(const Window &window) const;
  std::vector<const char *> getValidationLayers() const;
  std::vector<const char *> getDeviceExtensions() const;
};

} // namespace render

#endif
