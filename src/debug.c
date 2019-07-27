#include <sulfur/debug.h>

#include <stdio.h>
#include <string.h>

void sulfur_debug_get_validation_layers(uint32_t *layer_count,
                                        const char **layers) {
  uint32_t available_layer_count = 0;
  VkLayerProperties available_layers[32] = {{0}};

  vkEnumerateInstanceLayerProperties(&available_layer_count, NULL);
  vkEnumerateInstanceLayerProperties(&available_layer_count, available_layers);

  // Try using the newer `VK_LAYER_KHRONOS_validation`
  // instead of the deprecated `VK_LAYER_LUNARG_standard_validation`.
  VkBool32 validation_available = VK_FALSE;
  for (uint32_t i = 0; i < available_layer_count; ++i) {
    if (!strcmp(available_layers[i].layerName, "VK_LAYER_KHRONOS_validation")) {
      *layer_count = 1;
      layers[0] = "VK_LAYER_LUNARG_standard_validation";
      validation_available = VK_TRUE;
    } else if (!strcmp(available_layers[i].layerName,
                       "VK_LAYER_LUNARG_standard_validation")) {
      *layer_count = 1;
      layers[0] = "VK_LAYER_LUNARG_standard_validation";
      validation_available = VK_TRUE;
    }
  }
  // If neither are available, load any available
  // elementary validation layers.
  if (!validation_available) {
    for (uint32_t i = 0; i < available_layer_count; ++i) {
      if (!strcmp(available_layers[i].layerName,
                  "VK_LAYER_LUNARG_core_validation")) {
        layers[(*layer_count)++] = "VK_LAYER_LUNARG_core_validation";
      } else if (!strcmp(available_layers[i].layerName,
                         "VK_LAYER_LUNARG_parameter_validation")) {
        layers[(*layer_count)++] = "VK_LAYER_LUNARG_parameter_validation";
      }
    }
  }
}

void sulfur_debug_get_extensions(uint32_t *extension_count,
                                 const char **extensions,
                                 VkBool32 *debug_utils_available) {
  uint32_t available_extension_count = 0;
  VkExtensionProperties available_extensions[32] = {{0}};
  vkEnumerateInstanceExtensionProperties(NULL, &available_extension_count,
                                         NULL);
  vkEnumerateInstanceExtensionProperties(NULL, &available_extension_count,
                                         available_extensions);

  // Prefer the more modern `VK_EXT_debug_utils`
  // to `VK_EXT_debug_report`.
  for (uint32_t i = 0; i < available_extension_count; ++i) {
    printf("%s\n", available_extensions[i].extensionName);
    if (!strcmp(available_extensions[i].extensionName,
                VK_EXT_DEBUG_UTILS_EXTENSION_NAME)) {
      *extension_count = 1;
      extensions[0] = VK_EXT_DEBUG_UTILS_EXTENSION_NAME;
      *debug_utils_available = VK_TRUE;
      return;
    } else if (!strcmp(available_extensions[i].extensionName,
                       VK_EXT_DEBUG_REPORT_EXTENSION_NAME)) {
      *extension_count = 1;
      extensions[0] = VK_EXT_DEBUG_REPORT_EXTENSION_NAME;
    }
  }
  *debug_utils_available = VK_FALSE;
}

/**
 * Call `callback` for any messages from Vulkan.
 *
 * Requires the `VK_EXT_debug_utils` extension which
 * is more modern than `VK_EXT_debug_report` but not
 * always available.
 */
void sulfur_debug_messenger_create(
    VkInstance instance, PFN_vkDebugUtilsMessengerCallbackEXT callback,
    VkDebugUtilsMessengerEXT *debug_messenger) {

  PFN_vkCreateDebugUtilsMessengerEXT vkCreateDebugUtilsMessengerEXT =
      (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
          instance, "vkCreateDebugUtilsMessengerEXT");

  VkDebugUtilsMessengerCreateInfoEXT debug_info = {0};
  debug_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
  debug_info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT |
                               VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                               VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
                               VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT;
  debug_info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                           VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                           VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
  debug_info.pfnUserCallback = callback;

  vkCreateDebugUtilsMessengerEXT(instance, &debug_info, NULL, debug_messenger);
}

void sulfur_debug_messenger_destroy(VkInstance instance,
                                    VkDebugUtilsMessengerEXT messenger) {
  PFN_vkDestroyDebugUtilsMessengerEXT vkDestroyDebugUtilsMessengerEXT =
      (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
          instance, "vkDestroyDebugUtilsMessengerEXT");

  vkDestroyDebugUtilsMessengerEXT(instance, messenger, NULL);
}

/**
 * Call `callback` upon Vulkan messages.
 *
 * Requires the `VK_EXT_debug_report` extension.
 */
void sulfur_debug_report_callback_create(
    VkInstance instance, PFN_vkDebugReportCallbackEXT callback,
    VkDebugReportCallbackEXT *debug_report_callback) {
  PFN_vkCreateDebugReportCallbackEXT vkCreateDebugReportCallbackEXT = NULL;
  vkCreateDebugReportCallbackEXT =
      (PFN_vkCreateDebugReportCallbackEXT)vkGetInstanceProcAddr(
          instance, "vkCreateDebugReportCallbackEXT");

  VkDebugReportCallbackCreateInfoEXT debug_report_info;
  debug_report_info.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CREATE_INFO_EXT;
  debug_report_info.pNext = NULL;
  debug_report_info.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT |
                            VK_DEBUG_REPORT_WARNING_BIT_EXT |
                            VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT;
  debug_report_info.pfnCallback = callback;
  debug_report_info.pUserData = NULL;

  vkCreateDebugReportCallbackEXT(instance, &debug_report_info, NULL,
                                 debug_report_callback);
}

void sulfur_debug_report_callback_destroy(
    VkInstance instance, VkDebugReportCallbackEXT debug_report_callback) {
  PFN_vkDestroyDebugReportCallbackEXT vkDestroyDebugReportCallbackEXT = NULL;
  vkDestroyDebugReportCallbackEXT =
      (PFN_vkDestroyDebugReportCallbackEXT)vkGetInstanceProcAddr(
          instance, "vkDestroyDebugReportCallbackEXT");

  vkDestroyDebugReportCallbackEXT(instance, debug_report_callback, NULL);
}
