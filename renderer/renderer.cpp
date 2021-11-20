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

Renderer::Renderer(const Window &window, const std::string &app_name,
                   size_t num_vertices) {
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
  createSwapchain(window.getExtent());
  scene_ = std::move(VertexBuffer(*device_, physical_device_, num_vertices));
  camera_buffers_.reserve(image_count_);
  for (unsigned i = 0; i < image_count_; ++i)
    camera_buffers_.emplace_back(*device_, physical_device_);
  createDescriptors();
  createDepthResources();
  createRenderPass();
  createPipeline();
  createFramebuffers();
  createCommandBuffers();
  recordCommandBuffers();
};

void Renderer::resize(const Window &window) {
  device_->waitIdle();
  depth_image_view_.reset();
  depth_image_.reset();
  depth_image_mem_.reset();
  framebuffers_.clear();
  command_buffers_.clear();
  pipeline_.reset();
  pipeline_layout_.reset();
  render_pass_.reset();
  swapchain_image_views_.clear();
  swapchain_images_.clear();
  swapchain_.reset();

  createSwapchain(window.getExtent());
  createDepthResources();
  createRenderPass();
  createPipeline();
  createFramebuffers();
  createCommandBuffers();
  recordCommandBuffers();
}

void Renderer::draw(const Window &window, const VertexData &vertex_data,
                    const CameraData &camera_data) {
  auto image_index = device_->acquireNextImageKHR(
      *swapchain_, std::numeric_limits<uint64_t>::max(),
      *image_available_semaphore_, {});
  if (image_index.result == vk::Result::eErrorOutOfDateKHR) {
    resize(window);
    return;
  }

  scene_.upload(*device_, vertex_data);
  camera_buffers_[image_index.value].upload(*device_, camera_data);

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

void Renderer::createSwapchain(vk::Extent2D extent) {
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

  extent_ = extent;
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

void Renderer::createDescriptors() {
  auto pool_size =
      vk::DescriptorPoolSize{vk::DescriptorType::eUniformBuffer, image_count_};
  descriptor_pool_ = device_->createDescriptorPoolUnique(
      {{}, image_count_, 1, &pool_size}, nullptr);
  vk::DescriptorSetLayoutBinding ubo_layout_binding{
      0, vk::DescriptorType::eUniformBuffer, 1,
      vk::ShaderStageFlagBits::eVertex};
  descriptor_set_layout_ =
      device_->createDescriptorSetLayoutUnique({{}, 1, &ubo_layout_binding});
  std::vector<vk::DescriptorSetLayout> layouts(image_count_,
                                               *descriptor_set_layout_);
  descriptor_sets_ =
      device_->allocateDescriptorSets({*descriptor_pool_, layouts});
  for (unsigned i = 0; i < image_count_; ++i) {
    auto buffer_info = vk::DescriptorBufferInfo{camera_buffers_[i].get(), 0,
                                                sizeof(CameraData)};
    auto descriptor_write =
        vk::WriteDescriptorSet{descriptor_sets_[i],
                               0,
                               0,
                               1,
                               vk::DescriptorType::eUniformBuffer,
                               nullptr,
                               &buffer_info,
                               nullptr};
    device_->updateDescriptorSets(1, &descriptor_write, 0, nullptr);
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

  auto depth_attachment = vk::AttachmentDescription{
      {},
      depth_format_,
      vk::SampleCountFlagBits::e1,
      vk::AttachmentLoadOp::eClear,
      vk::AttachmentStoreOp::eDontCare,
      {},
      {},
      {},
      vk::ImageLayout::eDepthStencilAttachmentOptimal};

  auto depth_attachment_ref = vk::AttachmentReference{
      1, vk::ImageLayout::eDepthStencilAttachmentOptimal};

  auto subpass = {vk::SubpassDescription{{},
                                         vk::PipelineBindPoint::eGraphics,
                                         0,
                                         nullptr,
                                         1,
                                         &color_attachment_ref,
                                         nullptr,
                                         &depth_attachment_ref}};

  auto subpass_dependency = {
      vk::SubpassDependency{VK_SUBPASS_EXTERNAL,
                            0,
                            vk::PipelineStageFlagBits::eColorAttachmentOutput,
                            vk::PipelineStageFlagBits::eColorAttachmentOutput,
                            {},
                            vk::AccessFlagBits::eColorAttachmentRead |
                                vk::AccessFlagBits::eColorAttachmentWrite}};
  std::array<vk::AttachmentDescription, 2> attachments = {color_attachment,
                                                          depth_attachment};
  render_pass_ = device_->createRenderPassUnique(
      vk::RenderPassCreateInfo{{}, attachments, subpass, subpass_dependency});
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

  auto binding_description = Vertex::getBindingDescription();
  auto attribute_descriptions = Vertex::getAttributeDescriptions();
  auto vertex_input_info =
      vk::PipelineVertexInputStateCreateInfo{{},
                                             1,
                                             &binding_description,
                                             attribute_descriptions.size(),
                                             attribute_descriptions.data()};

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

  auto depth_stencil = vk::PipelineDepthStencilStateCreateInfo{
      {}, VK_TRUE, VK_TRUE, vk::CompareOp::eLess, VK_FALSE, VK_FALSE};

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

  pipeline_layout_ = device_->createPipelineLayoutUnique(
      {{}, 1, &*descriptor_set_layout_}, nullptr);

  auto pipeline_create_info =
      vk::GraphicsPipelineCreateInfo{{},
                                     pipeline_shader_stages,
                                     &vertex_input_info,
                                     &input_assembly,
                                     nullptr,
                                     &viewport_state,
                                     &rasterizer,
                                     &multisampling,
                                     &depth_stencil,
                                     &color_blending,
                                     nullptr,
                                     *pipeline_layout_,
                                     *render_pass_,
                                     0};
  pipeline_ =
      device_->createGraphicsPipelineUnique({}, pipeline_create_info).value;
}

void Renderer::createDepthResources() {
  depth_format_ = vk::Format::eD32Sfloat;
  auto image_info =
      vk::ImageCreateInfo{{},
                          vk::ImageType::e2D,
                          depth_format_,
                          vk::Extent3D{extent_.width, extent_.height, 1},
                          1,
                          1,
                          vk::SampleCountFlagBits::e1,
                          vk::ImageTiling::eOptimal,
                          vk::ImageUsageFlagBits::eDepthStencilAttachment,
                          vk::SharingMode::eExclusive,
                          0,
                          nullptr,
                          vk::ImageLayout::eUndefined};
  depth_image_ = device_->createImageUnique(image_info, nullptr);
  auto memory_properties = physical_device_.getMemoryProperties();
  auto memory_requirements = device_->getImageMemoryRequirements(*depth_image_);
  auto memory_index =
      findMemoryTypeIndex(memory_properties, memory_requirements,
                          vk::MemoryPropertyFlagBits::eDeviceLocal);
  depth_image_mem_ = device_->allocateMemoryUnique(
      vk::MemoryAllocateInfo{memory_requirements.size, memory_index});
  device_->bindImageMemory(*depth_image_, *depth_image_mem_, 0);
  auto image_view_info =
      vk::ImageViewCreateInfo{{},
                              *depth_image_,
                              vk::ImageViewType::e2D,
                              depth_format_,
                              {},
                              {vk::ImageAspectFlagBits::eDepth, 0, 1, 0, 1}};
  depth_image_view_ = device_->createImageViewUnique(image_view_info, nullptr);
}

void Renderer::createFramebuffers() {
  framebuffers_.reserve(swapchain_image_views_.size());
  for (const auto &image_view : swapchain_image_views_) {
    std::array<vk::ImageView, 2> attachments = {*image_view,
                                                *depth_image_view_};
    framebuffers_.push_back(
        device_->createFramebufferUnique(vk::FramebufferCreateInfo{
            {}, *render_pass_, attachments, extent_.width, extent_.height, 1}));
  }
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
    auto color_clear_value =
        vk::ClearColorValue{std::array<float, 4>{0.0f, 0.0f, 0.f, 1.f}};
    auto depth_clear_value = vk::ClearDepthStencilValue{1.f, 0};
    std::array<vk::ClearValue, 2> clear_values{color_clear_value,
                                               depth_clear_value};
    auto render_pass_begin_info =
        vk::RenderPassBeginInfo{*render_pass_, *framebuffers_[i],
                                vk::Rect2D{{0, 0}, extent_}, clear_values};

    command_buffers_[i]->beginRenderPass(render_pass_begin_info,
                                         vk::SubpassContents::eInline);
    command_buffers_[i]->bindPipeline(vk::PipelineBindPoint::eGraphics,
                                      *pipeline_);
    command_buffers_[i]->bindDescriptorSets(vk::PipelineBindPoint::eGraphics,
                                            *pipeline_layout_, 0, 1,
                                            &descriptor_sets_[i], 0, nullptr);
    vk::Buffer vertex_buffers[] = {scene_.get()};
    vk::DeviceSize offsets[] = {0};
    command_buffers_[i]->bindVertexBuffers(0, 1, vertex_buffers, offsets);
    command_buffers_[i]->draw(scene_.getNumVertices(), 1, 0, 0);
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
