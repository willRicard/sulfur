#include <sulfur/buffer.h>

#include <stdlib.h>
#include <string.h>

VkResult sulfur_buffer_create(const SulfurDevice *dev, const VkDeviceSize size,
                              const VkBufferUsageFlags usage,
                              const VkMemoryPropertyFlags memory_properties,
                              SulfurBuffer *buffer) {
  VkPhysicalDeviceProperties props = {};
  vkGetPhysicalDeviceProperties(dev->physical_device, &props);
  VkDeviceSize alignment = props.limits.nonCoherentAtomSize;
  VkDeviceSize actual_size = (size / alignment + 1) * alignment;
  buffer->size = actual_size;

  VkBufferCreateInfo buffer_info = {};
  buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  buffer_info.size = actual_size;
  buffer_info.usage = usage;
  buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

  VkResult result =
      vkCreateBuffer(dev->device, &buffer_info, NULL, &buffer->buffer);
  if (result != VK_SUCCESS) {
    return result;
  }

  VkMemoryRequirements memory_requirements = {};
  vkGetBufferMemoryRequirements(dev->device, buffer->buffer,
                                &memory_requirements);

  uint32_t best_memory = sulfur_device_find_memory_type(
      dev, &memory_requirements, memory_properties);

  VkMemoryAllocateInfo allocate_info = {};
  allocate_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  allocate_info.allocationSize = memory_requirements.size;
  allocate_info.memoryTypeIndex = best_memory;

  vkAllocateMemory(dev->device, &allocate_info, NULL, &buffer->memory);
  vkBindBufferMemory(dev->device, buffer->buffer, buffer->memory, 0);

  if (memory_properties & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) {
    // Map to host memory when possible.
    result = vkMapMemory(dev->device, buffer->memory, 0, actual_size, 0,
                         (void **)&buffer->data);
    if (result != VK_SUCCESS) {
      return result;
    }
  }
  return VK_SUCCESS;
}

void sulfur_buffer_destroy(const SulfurDevice *dev, SulfurBuffer *buffer) {
  if (buffer->data != NULL) {
    vkUnmapMemory(dev->device, buffer->memory);
  }
  vkDestroyBuffer(dev->device, buffer->buffer, NULL);
  vkFreeMemory(dev->device, buffer->memory, NULL);
}

void sulfur_buffer_write(const void *data, SulfurBuffer *buffer) {
  memcpy(buffer->data, data, (size_t)buffer->size);
}

void sulfur_buffer_copy(const SulfurDevice *dev, const SulfurBuffer *src_buf,
                        const SulfurBuffer *dst_buf) {
  VkCommandBuffer cmd_buf = VK_NULL_HANDLE;
  sulfur_device_begin_command_buffer(dev, &cmd_buf);

  VkBufferCopy copy_region = {};
  copy_region.srcOffset = 0;
  copy_region.dstOffset = 0;
  copy_region.size = src_buf->size;
  vkCmdCopyBuffer(cmd_buf, src_buf->buffer, dst_buf->buffer, 1, &copy_region);

  sulfur_device_end_command_buffer(dev, &cmd_buf);
}
