#include <sulfur/swapchain.h>

#include <stdlib.h>

static void choose_resolution(const VkSurfaceCapabilitiesKHR *capabilities,
                              VkExtent2D *res) {
  if (capabilities->currentExtent.width != UINT32_MAX) {
    *res = capabilities->currentExtent;
  } else {
    // Clamp the window size within the supported range
    res->width = 800;
    res->height = 450;
    if (res->width < capabilities->minImageExtent.width) {
      res->width = capabilities->minImageExtent.width;
    } else if (res->width > capabilities->maxImageExtent.width) {
      res->width = capabilities->maxImageExtent.width;
    }

    if (res->height < capabilities->minImageExtent.height) {
      res->height = capabilities->minImageExtent.height;
    } else if (res->height > capabilities->maxImageExtent.width) {
      res->height = capabilities->maxImageExtent.width;
    }
  }
}

VkResult sulfur_swapchain_create(const SulfurDevice *dev,
                                 const VkSurfaceKHR surface,
                                 SulfurSwapchain *swapchain) {
  VkSwapchainCreateInfoKHR *info = &swapchain->info;
  info->sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
  info->pNext = NULL;
  info->flags = 0;
  info->surface = surface;

  VkSurfaceCapabilitiesKHR capabilities = {0};
  vkGetPhysicalDeviceSurfaceCapabilitiesKHR(dev->physical_device, surface,
                                            &capabilities);

  // Choose the best image count
  if (3 < capabilities.minImageCount) {
    info->minImageCount = capabilities.minImageCount;
  } else if (3 > capabilities.maxImageCount) {
    info->minImageCount = capabilities.maxImageCount;
  } else {
    info->minImageCount = 3;
  }

  // Pick the best image format
  uint32_t format_count = 0;
  vkGetPhysicalDeviceSurfaceFormatsKHR(dev->physical_device, surface,
                                       &format_count, NULL);

  VkSurfaceFormatKHR *formats = NULL;
  formats =
      (VkSurfaceFormatKHR *)malloc(format_count * sizeof(VkSurfaceFormatKHR));
  if (formats == NULL) {
    return VK_ERROR_OUT_OF_HOST_MEMORY;
  }

  vkGetPhysicalDeviceSurfaceFormatsKHR(dev->physical_device, surface,
                                       &format_count, formats);

  if (format_count != 1 || formats[0].format != VK_FORMAT_UNDEFINED) {
    info->imageFormat = formats[0].format;
    info->imageColorSpace = formats[0].colorSpace;
  } else {
    return -1;
  }

  free(formats);

  choose_resolution(&capabilities, &info->imageExtent);

  swapchain->info.imageArrayLayers = 1;
  swapchain->info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

  if (dev->graphics_queue_id != dev->present_queue_id) {
    uint32_t queue_family_indices[2] = {dev->graphics_queue_id,
                                        dev->present_queue_id};

    info->imageSharingMode = VK_SHARING_MODE_CONCURRENT;
    info->queueFamilyIndexCount = 2;
    info->pQueueFamilyIndices = queue_family_indices;
  } else {
    info->imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    info->queueFamilyIndexCount = 0;
    info->pQueueFamilyIndices = NULL;
  }

  info->preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;

  // Default to opaque windows.
  if (capabilities.supportedCompositeAlpha &
      VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR) {
    info->compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
  } else {
    info->compositeAlpha = VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR;
  }

  // Choose the best present mode.
  //
  // @see
  // https://www.khronos.org/registry/vulkan/specs/1.1-extensions/html/vkspec.html#VkPresentModeKHR
  //
  // Preference in descending
  // order.
  // 1. Mailbox
  // 2. Immediate
  // 3. FIFO
  uint32_t present_mode_count = 0;

  vkGetPhysicalDeviceSurfacePresentModesKHR(dev->physical_device, surface,
                                            &present_mode_count, NULL);

  VkPresentModeKHR *present_modes = NULL;
  present_modes =
      (VkPresentModeKHR *)malloc(present_mode_count * sizeof(VkPresentModeKHR));
  if (present_modes == NULL) {
    return VK_ERROR_OUT_OF_HOST_MEMORY;
  }

  vkGetPhysicalDeviceSurfacePresentModesKHR(dev->physical_device, surface,
                                            &present_mode_count, present_modes);

  for (uint32_t i = 0; i < present_mode_count; i++) {
    if (present_modes[i] == VK_PRESENT_MODE_MAILBOX_KHR) {
      info->presentMode = VK_PRESENT_MODE_MAILBOX_KHR;
      break;
    } else if (present_modes[i] == VK_PRESENT_MODE_IMMEDIATE_KHR) {
      info->presentMode = VK_PRESENT_MODE_IMMEDIATE_KHR;
    }
  }

  free(present_modes);

  info->clipped = VK_FALSE;
  info->oldSwapchain = VK_NULL_HANDLE;

  return sulfur_swapchain_resize(dev, surface, swapchain);
}

void swapchain_cleanup(const SulfurDevice *dev, SulfurSwapchain *swapchain) {
  vkDeviceWaitIdle(dev->device);

  for (uint32_t i = 0; i < swapchain->image_count; i++) {
    vkDestroyFramebuffer(dev->device, swapchain->framebuffers[i], NULL);
    vkDestroyImageView(dev->device, swapchain->image_views[i], NULL);
    vkDestroyFence(dev->device, swapchain->fences[i], NULL);
    vkDestroySemaphore(dev->device, swapchain->image_available_semaphores[i],
                       NULL);
    vkDestroySemaphore(dev->device, swapchain->render_finished_semaphores[i],
                       NULL);
  }

  vkDestroyRenderPass(dev->device, swapchain->render_pass, NULL);

  vkFreeCommandBuffers(dev->device, dev->command_pool, swapchain->image_count,
                       swapchain->command_buffers);
}

void sulfur_swapchain_destroy(const SulfurDevice *dev,
                              SulfurSwapchain *swapchain) {
  swapchain_cleanup(dev, swapchain);

  vkDestroySwapchainKHR(dev->device, swapchain->swapchain, NULL);
}

VkResult sulfur_swapchain_resize(const SulfurDevice *dev,
                                 const VkSurfaceKHR surface,
                                 SulfurSwapchain *swapchain) {
  VkSurfaceCapabilitiesKHR capabilities;
  vkGetPhysicalDeviceSurfaceCapabilitiesKHR(dev->physical_device, surface,
                                            &capabilities);

  choose_resolution(&capabilities, &swapchain->info.imageExtent);

  VkSwapchainKHR old_swapchain = swapchain->swapchain;

  // Using `oldSwapchain` currently fails
  // with MoltenVK.
#ifndef __APPLE__
  swapchain->info.oldSwapchain = old_swapchain;
#endif

  VkResult result = vkCreateSwapchainKHR(dev->device, &swapchain->info, NULL,
                                         &swapchain->swapchain);
  if (result != VK_SUCCESS) {
    return result;
  }

  vkDestroySwapchainKHR(dev->device, old_swapchain, NULL);

  // Retrieve the swapchain images.
  VkImage swapchain_images[4];
  vkGetSwapchainImagesKHR(dev->device, swapchain->swapchain,
                          &swapchain->image_count, NULL);
  vkGetSwapchainImagesKHR(dev->device, swapchain->swapchain,
                          &swapchain->image_count, swapchain_images);

  // Create the render pass
  VkAttachmentDescription color_attachment = {0};
  color_attachment.format = swapchain->info.imageFormat;
  color_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
  color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
  color_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  color_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  color_attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

  static const VkAttachmentReference color_attachment_reference = {
      0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};

  static const VkSubpassDescription subpass = {
      .flags = VK_PIPELINE_BIND_POINT_GRAPHICS,
      .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
      .colorAttachmentCount = 1,
      .pColorAttachments = &color_attachment_reference};

  // Render pass overwrites previous image contents.
  static const VkSubpassDependency subpass_dependency = {
      .srcSubpass = VK_SUBPASS_EXTERNAL,
      .dstSubpass = 0,
      .srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
      .dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
      .srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
      .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT};

  VkRenderPassCreateInfo render_pass_info = {0};
  render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
  render_pass_info.attachmentCount = 1;
  render_pass_info.pAttachments = &color_attachment;
  render_pass_info.subpassCount = 1;
  render_pass_info.pSubpasses = &subpass;
  render_pass_info.dependencyCount = 1;
  render_pass_info.pDependencies = &subpass_dependency;

  result = vkCreateRenderPass(dev->device, &render_pass_info, NULL,
                              &swapchain->render_pass);
  if (result != VK_SUCCESS) {
    return result;
  }

  VkImageViewCreateInfo image_view_create_info = {0};
  image_view_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
  image_view_create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
  image_view_create_info.format = swapchain->info.imageFormat;
  image_view_create_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
  image_view_create_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
  image_view_create_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
  image_view_create_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
  image_view_create_info.subresourceRange.aspectMask =
      VK_IMAGE_ASPECT_COLOR_BIT;
  image_view_create_info.subresourceRange.baseMipLevel = 0;
  image_view_create_info.subresourceRange.levelCount = 1;
  image_view_create_info.subresourceRange.baseArrayLayer = 0;
  image_view_create_info.subresourceRange.layerCount = 1;

  VkFramebufferCreateInfo framebuffer_info = {0};
  framebuffer_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
  framebuffer_info.renderPass = swapchain->render_pass;
  framebuffer_info.attachmentCount = 1;
  framebuffer_info.width = swapchain->info.imageExtent.width;
  framebuffer_info.height = swapchain->info.imageExtent.height;
  framebuffer_info.layers = 1;

  for (uint32_t i = 0; i < swapchain->image_count; ++i) {
    image_view_create_info.image = swapchain_images[i];

    result = vkCreateImageView(dev->device, &image_view_create_info, NULL,
                               &swapchain->image_views[i]);
    if (result != VK_SUCCESS) {
      return result;
    }

    framebuffer_info.pAttachments = &swapchain->image_views[i];

    result = vkCreateFramebuffer(dev->device, &framebuffer_info, NULL,
                                 &swapchain->framebuffers[i]);
    if (result != VK_SUCCESS) {
      return result;
    }
  }

  // Allocate command buffers.
  VkCommandBufferAllocateInfo command_buffer_info = {0};
  command_buffer_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  command_buffer_info.commandPool = dev->command_pool;
  command_buffer_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  command_buffer_info.commandBufferCount = swapchain->image_count;

  vkAllocateCommandBuffers(dev->device, &command_buffer_info,
                           swapchain->command_buffers);

  // Create semaphores and fences for rendering synchronization.
  static const VkSemaphoreCreateInfo semaphore_info = {
      .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};

  static const VkFenceCreateInfo fence_info = {
      .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
      .flags = VK_FENCE_CREATE_SIGNALED_BIT};

  result = VK_SUCCESS;
  for (uint32_t i = 0; i < swapchain->image_count; i++) {
    result = vkCreateSemaphore(dev->device, &semaphore_info, NULL,
                               &swapchain->image_available_semaphores[i]);
    if (result != VK_SUCCESS) {
      return result;
    }

    result = vkCreateSemaphore(dev->device, &semaphore_info, NULL,
                               &swapchain->render_finished_semaphores[i]);
    if (result != VK_SUCCESS) {
      return result;
    }

    result =
        vkCreateFence(dev->device, &fence_info, NULL, &swapchain->fences[i]);
    if (result != VK_SUCCESS) {
      return result;
    }
  }
  return VK_SUCCESS;
}

int sulfur_swapchain_present(const SulfurDevice *dev,
                             const VkSurfaceKHR surface,
                             SulfurSwapchain *swapchain) {
  swapchain->frame_id = (swapchain->frame_id + 1) % swapchain->image_count;

  vkWaitForFences(dev->device, 1, &swapchain->fences[swapchain->frame_id],
                  VK_TRUE, UINT64_MAX);
  vkResetFences(dev->device, 1, &swapchain->fences[swapchain->frame_id]);

  uint32_t image_index;
  VkResult result = vkAcquireNextImageKHR(
      dev->device, swapchain->swapchain, UINT64_MAX,
      swapchain->image_available_semaphores[swapchain->frame_id],
      VK_NULL_HANDLE, &image_index);

  if (result == VK_ERROR_OUT_OF_DATE_KHR) {
    sulfur_swapchain_destroy(dev, swapchain);
    sulfur_swapchain_resize(dev, surface, swapchain);

    return 0;
  }

  static const VkPipelineStageFlags waitStage =
      VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

  VkSubmitInfo submit_info = {0};
  submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submit_info.waitSemaphoreCount = 1;
  submit_info.pWaitSemaphores =
      &swapchain->image_available_semaphores[swapchain->frame_id];
  submit_info.pWaitDstStageMask = &waitStage;
  submit_info.commandBufferCount = 1;
  submit_info.pCommandBuffers = &swapchain->command_buffers[image_index];
  submit_info.signalSemaphoreCount = 1;
  submit_info.pSignalSemaphores =
      &swapchain->render_finished_semaphores[swapchain->frame_id];

  result = vkQueueSubmit(dev->graphics_queue, 1, &submit_info,
                         swapchain->fences[swapchain->frame_id]);
  if (result != VK_SUCCESS) {
    return result;
  }

  VkPresentInfoKHR present_info = {0};
  present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
  present_info.waitSemaphoreCount = 1;
  present_info.pWaitSemaphores =
      &swapchain->render_finished_semaphores[swapchain->frame_id];
  present_info.swapchainCount = 1;
  present_info.pSwapchains = &swapchain->swapchain;
  present_info.pImageIndices = &image_index;

  result = vkQueuePresentKHR(dev->present_queue, &present_info);

  if (result == VK_ERROR_OUT_OF_DATE_KHR) {
    swapchain_cleanup(dev, swapchain);
    sulfur_swapchain_resize(dev, surface, swapchain);
    return 0;
  }

  return 1;
}
