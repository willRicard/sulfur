#include <sulfur/device.h>

#include <stdlib.h>

VkResult sulfur_device_create(const VkInstance instance,
                              const VkSurfaceKHR surface, SulfurDevice *dev) {
  uint32_t device_count = 0;
  VkResult result = VK_SUCCESS;
  result = vkEnumeratePhysicalDevices(instance, &device_count, NULL);
  if (result != VK_SUCCESS) {
    return result;
  }

  VkPhysicalDevice *devices = NULL;
  devices = (VkPhysicalDevice *)malloc(device_count * sizeof(VkPhysicalDevice));
  if (devices == NULL) {
    return VK_ERROR_OUT_OF_HOST_MEMORY;
  }

  result = vkEnumeratePhysicalDevices(instance, &device_count, devices);
  if (result != VK_SUCCESS) {
    return result;
  }

  VkPhysicalDeviceProperties physical_device_properties;
  for (uint32_t i = 0; i < device_count; i++) {
    vkGetPhysicalDeviceProperties(devices[i], &physical_device_properties);
    if (physical_device_properties.deviceType ==
        VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
      dev->physical_device = devices[i];
      break;
    } else if (physical_device_properties.deviceType ==
               VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU) {
      dev->physical_device = devices[i];
    }
  }

  free(devices);

  if (dev->physical_device == VK_NULL_HANDLE) {
    // No physical device found.
    return -1;
  }

  // Query memory properties for later allocations
  vkGetPhysicalDeviceMemoryProperties(dev->physical_device,
                                      &dev->physical_device_memory_properties);

  // Pick the best graphics & present queues
  uint32_t queue_family_count = 0;
  vkGetPhysicalDeviceQueueFamilyProperties(dev->physical_device,
                                           &queue_family_count, NULL);

  VkQueueFamilyProperties *queue_family_properties = NULL;
  queue_family_properties = (VkQueueFamilyProperties *)malloc(
      queue_family_count * sizeof(VkQueueFamilyProperties));
  if (queue_family_properties == NULL) {
    return VK_ERROR_OUT_OF_HOST_MEMORY;
  }

  vkGetPhysicalDeviceQueueFamilyProperties(
      dev->physical_device, &queue_family_count, queue_family_properties);

  uint32_t graphics_queue_id = 0;
  uint32_t present_queue_id = 0;

  for (uint32_t i = 0; i < queue_family_count; i++) {
    VkQueueFamilyProperties queue_family = queue_family_properties[i];
    if (queue_family.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
      graphics_queue_id = i;
      break;
    } else if (i == queue_family_count - 1) {
      // No graphics queue found.
      free(queue_family_properties);
      return -1;
    }
  }
  for (uint32_t i = 0; i < queue_family_count; i++) {
    VkBool32 present_supported;
    vkGetPhysicalDeviceSurfaceSupportKHR(dev->physical_device, i, surface,
                                         &present_supported);
    if (present_supported) {
      present_queue_id = i;
      break;
    } else if (i == queue_family_count - 1) {
      // No present queue found.
      free(queue_family_properties);
      return -1;
    }
  }

  free(queue_family_properties);

  float priority = 1.f;

  VkDeviceQueueCreateInfo queue_infos[2] = {};

  queue_infos[0].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
  queue_infos[0].queueFamilyIndex = graphics_queue_id;
  queue_infos[0].queueCount = 1;
  queue_infos[0].pQueuePriorities = &priority;

  queue_infos[1].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
  queue_infos[1].queueFamilyIndex = present_queue_id;
  queue_infos[1].queueCount = 1;
  queue_infos[1].pQueuePriorities = &priority;

  VkPhysicalDeviceFeatures features = {0};
  vkGetPhysicalDeviceFeatures(dev->physical_device, &features);

  static const char *device_extension = VK_KHR_SWAPCHAIN_EXTENSION_NAME;

  VkDeviceCreateInfo device_info = {0};
  device_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
  device_info.pNext = NULL;
  device_info.queueCreateInfoCount = 1;
  device_info.pQueueCreateInfos = queue_infos;
  device_info.enabledExtensionCount = 1;
  device_info.ppEnabledExtensionNames = &device_extension;
  device_info.pEnabledFeatures = &features;

  result =
      vkCreateDevice(dev->physical_device, &device_info, NULL, &dev->device);
  if (result != VK_SUCCESS) {
    return result;
  }

  vkGetDeviceQueue(dev->device, graphics_queue_id, 0, &dev->graphics_queue);
  vkGetDeviceQueue(dev->device, present_queue_id, 0, &dev->present_queue);

  // Create the command pool
  VkCommandPoolCreateInfo pool_info = {0};
  pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
  pool_info.queueFamilyIndex = graphics_queue_id;

  return vkCreateCommandPool(dev->device, &pool_info, NULL, &dev->command_pool);
}

void sulfur_device_destroy(SulfurDevice *dev) {
  vkDestroyCommandPool(dev->device, dev->command_pool, NULL);
  vkDestroyDevice(dev->device, NULL);
}

VkResult sulfur_device_begin_command_buffer(const SulfurDevice *device,
                                            VkCommandBuffer *cmd_buf) {
  VkCommandBufferAllocateInfo command_buffer_info = {0};
  command_buffer_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  command_buffer_info.commandPool = device->command_pool;
  command_buffer_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  command_buffer_info.commandBufferCount = 1;

  VkResult result =
      vkAllocateCommandBuffers(device->device, &command_buffer_info, cmd_buf);
  if (result != VK_SUCCESS) {
    return result;
  }

  static const VkCommandBufferBeginInfo begin_info = {
      .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
      .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT};

  vkBeginCommandBuffer(*cmd_buf, &begin_info);

  return VK_SUCCESS;
}

VkResult sulfur_device_end_command_buffer(const SulfurDevice *device,
                                          VkCommandBuffer *cmd_buf) {
  vkEndCommandBuffer(*cmd_buf);

  VkSubmitInfo submit_info = {};
  submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submit_info.commandBufferCount = 1;
  submit_info.pCommandBuffers = cmd_buf;

  VkResult result =
      vkQueueSubmit(device->graphics_queue, 1, &submit_info, VK_NULL_HANDLE);
  if (result != VK_SUCCESS) {
    return result;
  }
  vkQueueWaitIdle(device->graphics_queue);

  vkFreeCommandBuffers(device->device, device->command_pool, 1, cmd_buf);

  return VK_SUCCESS;
}

uint32_t
sulfur_device_find_memory_type(const SulfurDevice *device,
                               const VkMemoryRequirements *mem_requirements,
                               const VkMemoryPropertyFlags mem_properties) {
  uint32_t best_memory = 0;
  for (uint32_t i = 0;
       i < device->physical_device_memory_properties.memoryTypeCount; i++) {
    VkMemoryType memory_type =
        device->physical_device_memory_properties.memoryTypes[i];
    if ((mem_requirements->memoryTypeBits & (1 << i)) &&
        (memory_type.propertyFlags & mem_properties)) {
      best_memory = i;
      break;
    }
  }
  return best_memory;
}
