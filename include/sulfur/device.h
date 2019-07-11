#ifndef SULFUR_DEVICE_H
#define SULFUR_DEVICE_H
#include <vulkan/vulkan.h>

typedef struct SulfurDevice {
  VkDevice device;

  VkPhysicalDevice physical_device;
  VkPhysicalDeviceProperties physical_device_properties;
  VkPhysicalDeviceMemoryProperties physical_device_memory_properties;

  uint32_t graphics_queue_id;
  uint32_t present_queue_id;
  VkQueue graphics_queue;
  VkQueue present_queue;

  VkCommandPool command_pool;
} SulfurDevice;

/**
 * Create a device for presenting to a window surface.
 * @param VkSurfaceKHR surface The surface to present to.
 */
VkResult sulfur_device_create(VkInstance instance, VkSurfaceKHR surface,
                              SulfurDevice *dev);

/**
 * Destroy a device.
 */
void sulfur_device_destroy(SulfurDevice *device);

/**
 * Find the device memory type most fit for a memory allocation.
 * @param VkMemoryRequirements mem_requirements Size of the allocation.
 * @param VkMemoryPropertyFlags mem_properties Specific memory properties
 * required for the allocated memory.
 */
uint32_t sulfur_device_find_memory_type(SulfurDevice *device,
                                        VkMemoryRequirements mem_requirements,
                                        VkMemoryPropertyFlags mem_properties);

/**
 * Begin a one time command buffer.
 */
VkResult sulfur_device_begin_command_buffer(SulfurDevice *device,
                                            VkCommandBuffer *buffer);

/**
 * Submit and destroy a one time command buffer.
 */
VkResult sulfur_device_end_command_buffer(SulfurDevice *device,
                                          VkCommandBuffer *buffer);

#endif // SULFUR_DEVICE_H
