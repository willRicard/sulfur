#ifndef SULFUR_INSTANCE_H
#define SULFUR_INSTANCE_H
#include <vulkan/vulkan.h>

/**
 * Create a Vulkan instance.
 */
VkResult sulfur_instance_create(const VkApplicationInfo *app_info,
                                uint32_t extension_count,
                                const char **extensions, VkInstance *instance);

#endif // SULFUR_INSTANCE_H
