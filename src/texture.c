#include <sulfur/texture.h>

#include <sulfur/buffer.h>

VkResult sulfur_texture_create(const SulfurDevice *device, const int width,
                               const int height, VkFormat format,
                               SulfurTexture *texture) {
  texture->width = width;
  texture->height = height;

  VkImageCreateInfo image_info = {0};
  image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
  image_info.imageType = VK_IMAGE_TYPE_2D;
  image_info.format = format;
  image_info.extent.width = width;
  image_info.extent.height = height;
  image_info.extent.depth = 1;
  image_info.mipLevels = 1;
  image_info.arrayLayers = 1;
  image_info.samples = VK_SAMPLE_COUNT_1_BIT;
  image_info.tiling = VK_IMAGE_TILING_OPTIMAL;
  image_info.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT |
                     VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
                     VK_IMAGE_USAGE_SAMPLED_BIT;
  image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

  VkResult result =
      vkCreateImage(device->device, &image_info, NULL, &texture->image);
  if (result != VK_SUCCESS) {
    return result;
  }

  VkMemoryRequirements mem_requirements = {0};
  vkGetImageMemoryRequirements(device->device, texture->image,
                               &mem_requirements);

  uint32_t best_memory = sulfur_device_find_memory_type(
      device, &mem_requirements, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

  VkMemoryAllocateInfo alloc_info = {0};
  alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  alloc_info.allocationSize = mem_requirements.size;
  alloc_info.memoryTypeIndex = best_memory;

  result = vkAllocateMemory(device->device, &alloc_info, NULL,
                            &texture->image_memory);
  if (result != VK_SUCCESS) {
    return result;
  }

  vkBindImageMemory(device->device, texture->image, texture->image_memory, 0);

  // Create an image view to access the texture.
  VkImageViewCreateInfo view_info = {0};
  view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
  view_info.image = texture->image;
  view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
  view_info.format = format;
  view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  view_info.subresourceRange.baseMipLevel = 0;
  view_info.subresourceRange.levelCount = 1;
  view_info.subresourceRange.baseArrayLayer = 0;
  view_info.subresourceRange.layerCount = 1;

  result =
      vkCreateImageView(device->device, &view_info, NULL, &texture->image_view);
  if (result != VK_SUCCESS) {
    return result;
  }

  // Create a sampler.
  static const VkSamplerCreateInfo sampler_info = {
      .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
      .magFilter = VK_FILTER_NEAREST,
      .minFilter = VK_FILTER_NEAREST,
      .mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST,
      .addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT,
      .addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT,
      .addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT,
      .mipLodBias = 0,
      .anisotropyEnable = VK_FALSE,
      .maxAnisotropy = 1.0f,
      .compareEnable = VK_FALSE,
      .compareOp = VK_COMPARE_OP_ALWAYS,
      .minLod = 0.0f,
      .maxLod = 0.0f,
      .borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK,
      .unnormalizedCoordinates = VK_FALSE};

  return vkCreateSampler(device->device, &sampler_info, NULL,
                         &texture->sampler);
}

VkResult sulfur_texture_create_from_image(const SulfurDevice *device,
                                          const VkFormat format,
                                          const int width, const int height,
                                          const unsigned char *pixels,
                                          SulfurTexture *texture) {
  VkResult result =
      sulfur_texture_create(device, width, height, format, texture);
  if (result != VK_SUCCESS) {
    return result;
  }

  const VkDeviceSize image_size = width * height * 4;

  SulfurBuffer buffer = {0};
  sulfur_buffer_create(device, image_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                       VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                           VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                       &buffer);

  sulfur_buffer_write(pixels, &buffer);

  sulfur_texture_transition_layout(device, texture, VK_IMAGE_LAYOUT_UNDEFINED,
                                   VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

  // Copy image data
  VkCommandBuffer cmd_buf = VK_NULL_HANDLE;
  sulfur_device_begin_command_buffer(device, &cmd_buf);

  VkBufferImageCopy region = {0};
  region.bufferOffset = 0;
  region.bufferRowLength = 0;
  region.bufferImageHeight = 0;
  region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  region.imageSubresource.mipLevel = 0;
  region.imageSubresource.baseArrayLayer = 0;
  region.imageSubresource.layerCount = 1;
  region.imageOffset.x = 0;
  region.imageOffset.y = 0;
  region.imageOffset.z = 0;
  region.imageExtent.width = width;
  region.imageExtent.height = height;
  region.imageExtent.depth = 1;

  vkCmdCopyBufferToImage(cmd_buf, buffer.buffer, texture->image,
                         VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

  sulfur_device_end_command_buffer(device, &cmd_buf);

  sulfur_texture_transition_layout(device, texture,
                                   VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                   VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

  sulfur_buffer_destroy(device, &buffer);

  return VK_SUCCESS;
}

void sulfur_texture_destroy(const SulfurDevice *device,
                            SulfurTexture *texture) {
  vkDestroySampler(device->device, texture->sampler, NULL);
  vkDestroyImageView(device->device, texture->image_view, NULL);
  vkDestroyImage(device->device, texture->image, NULL);
  vkFreeMemory(device->device, texture->image_memory, NULL);
}

void sulfur_texture_transition_layout(const SulfurDevice *device,
                                      const SulfurTexture *texture,
                                      const VkImageLayout old_layout,
                                      const VkImageLayout new_layout) {
  VkCommandBuffer cmd_buf = VK_NULL_HANDLE;
  sulfur_device_begin_command_buffer(device, &cmd_buf);

  VkImageMemoryBarrier barrier = {0};
  barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  barrier.oldLayout = old_layout;
  barrier.newLayout = new_layout;
  barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  barrier.subresourceRange.baseMipLevel = 0;
  barrier.subresourceRange.levelCount = 1;
  barrier.subresourceRange.baseArrayLayer = 0;
  barrier.subresourceRange.layerCount = 1;
  barrier.image = texture->image;

  VkPipelineStageFlags source_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
  VkPipelineStageFlags destination_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;

  if (old_layout == VK_IMAGE_LAYOUT_UNDEFINED &&
      new_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
    barrier.srcAccessMask = 0;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

    source_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    destination_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
  } else if (old_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL &&
             new_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    source_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    destination_stage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
  }

  vkCmdPipelineBarrier(cmd_buf, source_stage, destination_stage, 0, 0, NULL, 0,
                       NULL, 1, &barrier);

  sulfur_device_end_command_buffer(device, &cmd_buf);
}
