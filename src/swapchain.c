#include <sulfur/swapchain.h>

#include <stdlib.h>

#include <sulfur/sulfur.h>

static void choose_resolution(VkSurfaceCapabilitiesKHR *capabilities,
                              VkExtent2D *res) {
  if (capabilities->currentExtent.width != UINT32_MAX) {
    *res = capabilities->currentExtent;
  } else {
    // Clamp the window size within the supported range
    res->width = SULFUR_WINDOW_WIDTH;
    res->height = SULFUR_WINDOW_HEIGHT;
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

VkResult sulfur_swapchain_create(SulfurDevice *dev, VkSurfaceKHR surface,
                                 SulfurSwapchain *swapchain) {
  VkSwapchainCreateInfoKHR *info = &swapchain->info;
  info->sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
  info->pNext = NULL;
  info->flags = 0;
  info->surface = surface;

  VkSurfaceCapabilitiesKHR capabilities = {};
  vkGetPhysicalDeviceSurfaceCapabilitiesKHR(dev->physical_device, surface,
                                            &capabilities);

  // Choose the best image count
  info->minImageCount = 3;
  if (3 < capabilities.minImageCount) {
    info->minImageCount = capabilities.minImageCount;
  } else if (3 > capabilities.maxImageCount) {
    info->minImageCount = capabilities.maxImageCount;
  }

  // Pick the best image format
  uint32_t format_count = 0;
  vkGetPhysicalDeviceSurfaceFormatsKHR(dev->physical_device, surface,
                                       &format_count, NULL);

  VkSurfaceFormatKHR *formats =
      (VkSurfaceFormatKHR *)malloc(format_count * sizeof(VkSurfaceFormatKHR));

  vkGetPhysicalDeviceSurfaceFormatsKHR(dev->physical_device, surface,
                                       &format_count, formats);

  if (format_count != 1 || formats[0].format != VK_FORMAT_UNDEFINED) {
    info->imageFormat = formats[0].format;
    info->imageColorSpace = formats[0].colorSpace;
  }

  free(formats);

  choose_resolution(&capabilities, &info->imageExtent);

  swapchain->info.imageArrayLayers = 1;
  swapchain->info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

  if (dev->graphics_queue_id != dev->present_queue_id) {
    uint32_t queue_familyIndices[2] = {dev->graphics_queue_id,
                                       dev->present_queue_id};

    info->imageSharingMode = VK_SHARING_MODE_CONCURRENT;
    info->queueFamilyIndexCount = 2;
    info->pQueueFamilyIndices = queue_familyIndices;
  } else {
    info->imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    info->queueFamilyIndexCount = 0;
    info->pQueueFamilyIndices = NULL;
  }

  info->preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
  info->compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;

  // Choose the best present mode (see the spec, preference in descending
  // order).
  // 1. Mailbox
  // 2. Immediate
  // 3. FIFO
  uint32_t present_mode_count = 0;

  vkGetPhysicalDeviceSurfacePresentModesKHR(dev->physical_device, surface,
                                            &present_mode_count, NULL);

  VkPresentModeKHR *present_modes =
      (VkPresentModeKHR *)malloc(present_mode_count * sizeof(VkPresentModeKHR));

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

  sulfur_swapchain_resize(dev, surface, swapchain);

  // Create semaphores for rendering synchronization.
  VkSemaphoreCreateInfo semaphore_info = {};
  semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

  VkFenceCreateInfo fence_info = {};
  fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
  fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

  VkResult result = VK_SUCCESS;
  for (uint32_t i = 0; i < 3; i++) {
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

void swapchain_cleanup(SulfurDevice *dev, SulfurSwapchain *swapchain) {
  vkDeviceWaitIdle(dev->device);

  for (uint32_t i = 0; i < swapchain->image_count; i++) {
    vkDestroyFramebuffer(dev->device, swapchain->framebuffers[i], NULL);
    vkDestroyImageView(dev->device, swapchain->image_views[i], NULL);
  }

  vkDestroyRenderPass(dev->device, swapchain->render_pass, NULL);

  vkFreeCommandBuffers(dev->device, dev->command_pool, swapchain->image_count,
                       swapchain->command_buffers);
}

void sulfur_swapchain_destroy(SulfurDevice *dev, SulfurSwapchain *swapchain) {
  swapchain_cleanup(dev, swapchain);

  vkDestroySwapchainKHR(dev->device, swapchain->swapchain, NULL);

  for (uint32_t i = 0; i < 3; i++) {
    vkDestroySemaphore(dev->device, swapchain->render_finished_semaphores[i],
                       NULL);
    vkDestroySemaphore(dev->device, swapchain->image_available_semaphores[i],
                       NULL);
    vkDestroyFence(dev->device, swapchain->fences[i], NULL);
  }
}

VkResult sulfur_swapchain_resize(SulfurDevice *dev, VkSurfaceKHR surface,
                                 SulfurSwapchain *swapchain) {
  VkSurfaceCapabilitiesKHR capabilities;
  vkGetPhysicalDeviceSurfaceCapabilitiesKHR(dev->physical_device, surface,
                                            &capabilities);

  choose_resolution(&capabilities, &swapchain->info.imageExtent);

  VkSwapchainKHR old_swapchain = swapchain->swapchain;

  swapchain->info.oldSwapchain = old_swapchain;

  VkResult result = vkCreateSwapchainKHR(dev->device, &swapchain->info, NULL,
                                         &swapchain->swapchain);
  if (result != VK_SUCCESS) {
    return result;
  }

  vkDestroySwapchainKHR(dev->device, old_swapchain, NULL);

  // Retrieve the swapchain images.
  VkImage swapchain_images[3];
  vkGetSwapchainImagesKHR(dev->device, swapchain->swapchain,
                          &swapchain->image_count, NULL);
  vkGetSwapchainImagesKHR(dev->device, swapchain->swapchain,
                          &swapchain->image_count, swapchain_images);

  // Create the render pass
  VkAttachmentDescription color_attachment = {};
  color_attachment.format = swapchain->info.imageFormat;
  color_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
  color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
  color_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  color_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  color_attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

  VkAttachmentReference color_attachment_reference = {};
  color_attachment_reference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

  VkSubpassDescription subpass = {};
  subpass.flags = VK_PIPELINE_BIND_POINT_GRAPHICS;
  subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
  subpass.colorAttachmentCount = 1;
  subpass.pColorAttachments = &color_attachment_reference;

  VkSubpassDependency subpass_dependency = {};
  subpass_dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
  subpass_dependency.dstSubpass = 0;
  subpass_dependency.srcStageMask =
      VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  subpass_dependency.dstStageMask =
      VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  subpass_dependency.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
  subpass_dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;

  VkRenderPassCreateInfo render_pass_info = {};
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

  for (uint32_t i = 0; i < swapchain->image_count; ++i) {
    VkImageViewCreateInfo image_view_create_info = {};
    image_view_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    image_view_create_info.image = swapchain_images[i];
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

    result = vkCreateImageView(dev->device, &image_view_create_info, NULL,
                               &swapchain->image_views[i]);
    if (result != VK_SUCCESS) {
      return result;
    }

    VkFramebufferCreateInfo framebuffer_info = {};
    framebuffer_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebuffer_info.renderPass = swapchain->render_pass;
    framebuffer_info.attachmentCount = 1;
    framebuffer_info.pAttachments = &swapchain->image_views[i];
    framebuffer_info.width = swapchain->info.imageExtent.width;
    framebuffer_info.height = swapchain->info.imageExtent.height;
    framebuffer_info.layers = 1;

    result = vkCreateFramebuffer(dev->device, &framebuffer_info, NULL,
                                 &swapchain->framebuffers[i]);
    if (result != VK_SUCCESS) {
      return result;
    }
  }

  // Allocate the command buffers.
  VkCommandBufferAllocateInfo command_buffer_info = {};
  command_buffer_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  command_buffer_info.commandPool = dev->command_pool;
  command_buffer_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  command_buffer_info.commandBufferCount = swapchain->image_count;

  return vkAllocateCommandBuffers(dev->device, &command_buffer_info,
                                  swapchain->command_buffers);
}

int sulfur_swapchain_present(SulfurDevice *dev, VkSurfaceKHR surface,
                             SulfurSwapchain *swapchain) {
  swapchain->frame_id = (swapchain->frame_id + 1) % 3;

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

  VkPipelineStageFlags waitStage =
      VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

  VkSubmitInfo submit_info = {};
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

  VkPresentInfoKHR present_info = {};
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