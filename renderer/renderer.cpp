#include "renderer.hpp"
#include "window.hpp"
#include <GLFW/glfw3.h>
#include <algorithm>
#include <iostream>
#include <limits>
#include <set>
#include <stdexcept>
#include <vector>

VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE;

namespace render {

#ifndef NDEBUG
static VKAPI_ATTR VkBool32 VKAPI_CALL
debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
              VkDebugUtilsMessageTypeFlagsEXT messageType,
              const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
              void *pUserData) {
  if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
    std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;
  return VK_FALSE;
}
#endif

Renderer::Renderer(const Window &window, const std::string &app_name) {
  createInstance(window, app_name);
#ifndef NDEBUG
  createDebugCallback();
#endif
  surface_ = window.createSurface(*instance_);
  selectPhysicalDevice();
  createDevice();
  createQueues();
  createCommandPool();
  createSemaphores();
  createSwapchain(window);
  createRenderPass();
  createPipeline();
  createFramebuffers();
  createCommandBuffers();
  recordCommandBuffers();
};

void Renderer::resize(const Window &window) {
  device_->waitIdle();
  framebuffers_.clear();
  command_buffers_.clear();
  pipeline_.reset();
  pipeline_layout_.reset();
  render_pass_.reset();
  swapchain_image_views_.clear();
  swapchain_images_.clear();
  swapchain_.reset();

  createSwapchain(window);
  createRenderPass();
  createPipeline();
  createFramebuffers();
  createCommandBuffers();
  recordCommandBuffers();
}

void Renderer::draw(const Window &window) {
  auto image_index = device_->acquireNextImageKHR(
      *swapchain_, std::numeric_limits<uint64_t>::max(),
      *image_available_semaphore_, {});
  if (image_index.result == vk::Result::eErrorOutOfDateKHR) {
    resize(window);
    return;
  }

  vk::PipelineStageFlags wait_stage_mask =
      vk::PipelineStageFlagBits::eColorAttachmentOutput;

  auto submit_info = vk::SubmitInfo{1,
                                    &*image_available_semaphore_,
                                    &wait_stage_mask,
                                    1,
                                    &*command_buffers_[image_index.value],
                                    1,
                                    &*render_finished_semaphore_};

  graphics_queue_.submit(submit_info, {});

  auto present_info = vk::PresentInfoKHR{1, &*render_finished_semaphore_, 1,
                                         &*swapchain_, &image_index.value};
  auto result = present_queue_.presentKHR(&present_info);
  if (result == vk::Result::eErrorOutOfDateKHR) {
    resize(window);
    return;
  }

  device_->waitIdle();
}

void Renderer::createInstance(const Window &window,
                              const std::string &app_name) {
  vk::ApplicationInfo app_info(app_name.c_str(), VK_MAKE_VERSION(1, 0, 0),
                               "No engine", VK_MAKE_VERSION(1, 0, 0),
                               VK_API_VERSION_1_1);
  auto layers = getValidationLayers();
  auto extensions = getInstanceExtensions(window);
  vk::InstanceCreateInfo create_info({}, &app_info, layers, extensions);
  VULKAN_HPP_DEFAULT_DISPATCHER.init(glfwGetInstanceProcAddress);
  instance_ = vk::createInstanceUnique(create_info);
  VULKAN_HPP_DEFAULT_DISPATCHER.init(*instance_);
}

#ifndef NDEBUG
void Renderer::createDebugCallback() {
  messenger_ = instance_->createDebugUtilsMessengerEXTUnique(
      vk::DebugUtilsMessengerCreateInfoEXT{
          {},
          vk::DebugUtilsMessageSeverityFlagBitsEXT::eError |
              vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
              vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose |
              vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo,
          vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral |
              vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation |
              vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance,
          debugCallback},
      nullptr);
}
#endif

void Renderer::selectPhysicalDevice() {
  auto physical_devices = instance_->enumeratePhysicalDevices();
  std::cerr << "Supported devices:\n";
  for (const auto &device : physical_devices)
    std::cerr << device.getProperties().deviceName << "\n";
  if (physical_devices.empty())
    throw std::runtime_error("no supported Vulkan devices");

  physical_device_ = physical_devices[0];
}

void Renderer::createDevice() {
  auto queue_family_properties = physical_device_.getQueueFamilyProperties();

  graphics_queue_family_index_ = static_cast<uint32_t>(std::distance(
      queue_family_properties.begin(),
      std::find_if(queue_family_properties.begin(),
                   queue_family_properties.end(),
                   [](vk::QueueFamilyProperties const &qfp) {
                     return qfp.queueFlags & vk::QueueFlagBits::eGraphics;
                   })));

  present_queue_family_index_ = 0;
  for (size_t i = 0; i < queue_family_properties.size(); i++)
    if (physical_device_.getSurfaceSupportKHR(static_cast<uint32_t>(i),
                                              surface_.get()))
      present_queue_family_index_ = static_cast<uint32_t>(i);

  std::set<uint32_t> unique_queue_family_indices = {
      graphics_queue_family_index_, present_queue_family_index_};

  std::vector<uint32_t> family_indices = {unique_queue_family_indices.begin(),
                                          unique_queue_family_indices.end()};

  std::vector<vk::DeviceQueueCreateInfo> queue_create_info;

  float queue_priority = 1.0f;
  for (auto &queue_family_index : unique_queue_family_indices)
    queue_create_info.emplace_back(vk::DeviceQueueCreateFlags(),
                                   queue_family_index, 1, &queue_priority);
  auto extensions = getDeviceExtensions();

  device_ = physical_device_.createDeviceUnique(
      vk::DeviceCreateInfo({}, queue_create_info, {}, extensions));
}

void Renderer::createQueues() {
  graphics_queue_ = device_->getQueue(graphics_queue_family_index_, 0);
  present_queue_ = device_->getQueue(present_queue_family_index_, 0);
}

void Renderer::createSwapchain(const Window &window) {
  auto capabilities = physical_device_.getSurfaceCapabilitiesKHR(*surface_);
  auto formats = physical_device_.getSurfaceFormatsKHR(*surface_);

  constexpr vk::SurfaceFormatKHR default_format{
      vk::Format::eB8G8R8A8Unorm, vk::ColorSpaceKHR::eSrgbNonlinear};
  if (std::none_of(formats.begin(), formats.end(),
                   [default_format](const auto &surface_format) {
                     return surface_format == default_format;
                   }))
    throw std::runtime_error("Can not find B9G8R8A8Unorm surface format");
  format_ = default_format.format;
  color_space_ = default_format.colorSpace;

  extent_ = window.getExtent();
  image_count_ = capabilities.minImageCount + 1;

  struct SM {
    vk::SharingMode sharing_mode;
    std::vector<uint32_t> family_indices;
  } sharingModeUtil{
      (graphics_queue_family_index_ != present_queue_family_index_)
          ? SM{vk::SharingMode::eConcurrent,
               {graphics_queue_family_index_, present_queue_family_index_}}
          : SM{vk::SharingMode::eExclusive, {}}};

  vk::SwapchainCreateInfoKHR swapchain_create_info(
      {}, surface_.get(), image_count_, format_, color_space_, extent_, 1,
      vk::ImageUsageFlagBits::eColorAttachment, sharingModeUtil.sharing_mode,
      sharingModeUtil.family_indices,
      vk::SurfaceTransformFlagBitsKHR::eIdentity,
      vk::CompositeAlphaFlagBitsKHR::eOpaque, vk::PresentModeKHR::eFifo, true,
      nullptr);

  swapchain_ = device_->createSwapchainKHRUnique(swapchain_create_info);

  swapchain_images_ = device_->getSwapchainImagesKHR(swapchain_.get());

  swapchain_image_views_.reserve(swapchain_images_.size());
  for (auto image : swapchain_images_) {
    vk::ImageViewCreateInfo image_view_create_info(
        vk::ImageViewCreateFlags(), image, vk::ImageViewType::e2D, format_,
        vk::ComponentMapping{vk::ComponentSwizzle::eR, vk::ComponentSwizzle::eG,
                             vk::ComponentSwizzle::eB,
                             vk::ComponentSwizzle::eA},
        vk::ImageSubresourceRange{vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1});
    swapchain_image_views_.push_back(
        device_->createImageViewUnique(image_view_create_info));
  }
}

void Renderer::createRenderPass() {
  auto color_attachment =
      vk::AttachmentDescription{{},
                                format_,
                                vk::SampleCountFlagBits::e1,
                                vk::AttachmentLoadOp::eClear,
                                vk::AttachmentStoreOp::eStore,
                                {},
                                {},
                                {},
                                vk::ImageLayout::ePresentSrcKHR};

  auto color_attachment_ref =
      vk::AttachmentReference{0, vk::ImageLayout::eColorAttachmentOptimal};

  auto subpass = vk::SubpassDescription{{}, vk::PipelineBindPoint::eGraphics,
                                        0,  nullptr,
                                        1,  &color_attachment_ref};

  auto subpass_dependency =
      vk::SubpassDependency{VK_SUBPASS_EXTERNAL,
                            0,
                            vk::PipelineStageFlagBits::eColorAttachmentOutput,
                            vk::PipelineStageFlagBits::eColorAttachmentOutput,
                            {},
                            vk::AccessFlagBits::eColorAttachmentRead |
                                vk::AccessFlagBits::eColorAttachmentWrite};

  render_pass_ = device_->createRenderPassUnique(vk::RenderPassCreateInfo{
      {}, 1, &color_attachment, 1, &subpass, 1, &subpass_dependency});
}

void Renderer::createPipeline() {
  std::vector<uint32_t> vert_code =
#include "shader.vert.inc"
      ;
  std::vector<uint32_t> frag_code =
#include "shader.frag.inc"
      ;
  auto vert_module = device_->createShaderModuleUnique(
      {{}, vert_code.size() * sizeof(uint32_t), vert_code.data()});
  auto frag_module = device_->createShaderModuleUnique(
      {{}, frag_code.size() * sizeof(uint32_t), frag_code.data()});
  auto vert_shader_stage_info = vk::PipelineShaderStageCreateInfo{
      {}, vk::ShaderStageFlagBits::eVertex, *vert_module, "main"};

  auto frag_shader_stage_info = vk::PipelineShaderStageCreateInfo{
      {}, vk::ShaderStageFlagBits::eFragment, *frag_module, "main"};

  auto pipeline_shader_stages = std::vector<vk::PipelineShaderStageCreateInfo>{
      vert_shader_stage_info, frag_shader_stage_info};

  auto vertex_input_info =
      vk::PipelineVertexInputStateCreateInfo{{}, 0u, nullptr, 0u, nullptr};

  auto input_assembly = vk::PipelineInputAssemblyStateCreateInfo{
      {}, vk::PrimitiveTopology::eTriangleList, false};

  auto viewport = vk::Viewport{0.0f,
                               0.0f,
                               static_cast<float>(extent_.width),
                               static_cast<float>(extent_.height),
                               0.0f,
                               1.0f};

  auto scissor = vk::Rect2D{{0, 0}, extent_};

  auto viewport_state =
      vk::PipelineViewportStateCreateInfo{{}, 1, &viewport, 1, &scissor};

  auto rasterizer = vk::PipelineRasterizationStateCreateInfo{
      {},    false,
      false, vk::PolygonMode::eFill,
      {},    vk::FrontFace::eCounterClockwise,
      {},    {},
      {},    {},
      1.0f};

  auto multisampling = vk::PipelineMultisampleStateCreateInfo{
      {}, vk::SampleCountFlagBits::e1, false, 1.0};

  auto color_blend_attachment = vk::PipelineColorBlendAttachmentState{
      {},
      vk::BlendFactor::eOne,
      vk::BlendFactor::eZero,
      vk::BlendOp::eAdd,
      vk::BlendFactor::eOne,
      vk::BlendFactor::eZero,
      vk::BlendOp::eAdd,
      vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
          vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA};

  auto color_blending = vk::PipelineColorBlendStateCreateInfo{
      {}, false, vk::LogicOp::eCopy, 1, &color_blend_attachment};

  pipeline_layout_ = device_->createPipelineLayoutUnique({}, nullptr);

  auto pipeline_create_info =
      vk::GraphicsPipelineCreateInfo{{},
                                     pipeline_shader_stages,
                                     &vertex_input_info,
                                     &input_assembly,
                                     nullptr,
                                     &viewport_state,
                                     &rasterizer,
                                     &multisampling,
                                     nullptr,
                                     &color_blending,
                                     nullptr,
                                     *pipeline_layout_,
                                     *render_pass_,
                                     0};
  pipeline_ =
      device_->createGraphicsPipelineUnique({}, pipeline_create_info).value;
}

void Renderer::createFramebuffers() {
  framebuffers_.reserve(swapchain_image_views_.size());
  for (const auto &image_view : swapchain_image_views_)
    framebuffers_.push_back(device_->createFramebufferUnique(
        vk::FramebufferCreateInfo{{},
                                  *render_pass_,
                                  1,
                                  &*image_view,
                                  extent_.width,
                                  extent_.height,
                                  1}));
}

void Renderer::createCommandPool() {
  command_pool_ = device_->createCommandPoolUnique(
      {{}, static_cast<uint32_t>(graphics_queue_family_index_)});
}

void Renderer::createCommandBuffers() {
  command_buffers_ =
      device_->allocateCommandBuffersUnique(vk::CommandBufferAllocateInfo(
          command_pool_.get(), vk::CommandBufferLevel::ePrimary,
          static_cast<uint32_t>(framebuffers_.size())));
}

void Renderer::recordCommandBuffers() {
  for (size_t i = 0; i < command_buffers_.size(); i++) {

    auto begin_info = vk::CommandBufferBeginInfo{};
    command_buffers_[i]->begin(begin_info);
    vk::ClearValue clear_values{};
    auto render_pass_begin_info =
        vk::RenderPassBeginInfo{*render_pass_, *framebuffers_[i],
                                vk::Rect2D{{0, 0}, extent_}, 1, &clear_values};

    command_buffers_[i]->beginRenderPass(render_pass_begin_info,
                                         vk::SubpassContents::eInline);
    command_buffers_[i]->bindPipeline(vk::PipelineBindPoint::eGraphics,
                                      *pipeline_);
    command_buffers_[i]->draw(3, 1, 0, 0);
    command_buffers_[i]->endRenderPass();
    command_buffers_[i]->end();
  }
}

void Renderer::createSemaphores() {
  image_available_semaphore_ =
      device_->createSemaphoreUnique(vk::SemaphoreCreateInfo{});
  render_finished_semaphore_ =
      device_->createSemaphoreUnique(vk::SemaphoreCreateInfo{});
}

std::vector<const char *>
Renderer::getInstanceExtensions(const Window &window) const {
  auto extensions = window.getRequiredExtensions();
#ifndef NDEBUG
  extensions.emplace_back("VK_EXT_debug_utils");
#endif
  return extensions;
}

std::vector<const char *> Renderer::getValidationLayers() const {
  return std::vector<const char *>{
#ifndef NDEBUG
      "VK_LAYER_KHRONOS_validation"
#endif
  };
}

std::vector<const char *> Renderer::getDeviceExtensions() const {
  return std::vector<const char *>{VK_KHR_SWAPCHAIN_EXTENSION_NAME};
}

} // namespace render
