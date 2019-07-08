#include <sulfur/instance.h>

VkResult sulfur_instance_create(const VkApplicationInfo *app_info,
                                uint32_t extension_count,
                                const char **extensions, VkInstance *instance) {
  VkInstanceCreateInfo instance_info = {};
  instance_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
  instance_info.pApplicationInfo = app_info;
  instance_info.enabledExtensionCount = extension_count;
  instance_info.ppEnabledExtensionNames = extensions;

  return vkCreateInstance(&instance_info, NULL, instance);
}
