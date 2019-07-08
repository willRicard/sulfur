#ifndef SULFUR_BUFFER_H_
#define SULFUR_BUFFER_H_
#include <vulkan/vulkan.h>

#include <sulfur/device.h>

/**
 * GPU or host buffer
 */
typedef struct SulfurBuffer {
  VkBuffer buffer;
  VkDeviceMemory memory;
  VkDeviceSize size;
  uint8_t *data;
} SulfurBuffer;

/**
 * Create a buffer and allocate memory on the device.
 */
VkResult sulfur_buffer_create(SulfurDevice *device, VkDeviceSize size,
                          VkBufferUsageFlags usage,
                          VkMemoryPropertyFlags memory_properties,
                          SulfurBuffer *buf);

/**
 * Destroy a buffer and free the bound memory.
 */
void sulfur_buffer_destroy(SulfurDevice *device, SulfurBuffer *buf);

/**
 * Write data to a buffer.
 */
void sulfur_buffer_write(const void *data, SulfurBuffer *buf);

/**
 * Synchronous copy from one buffer to another.
 */
void sulfur_buffer_copy(SulfurDevice *device, SulfurBuffer *src_buf,
                        SulfurBuffer *dst_buf);

#endif // SULFUR_BUFFER_H_
