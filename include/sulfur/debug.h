#ifndef SULFUR_DEBUG_H
#define SULFUR_DEBUG_H
#include <vulkan/vulkan.h>

void sulfur_debug_get_validation_layers(uint32_t *layer_count,
                                        const char **layers);

void sulfur_debug_get_extensions(uint32_t *extension_count,
                                 const char **extensions,
                                 VkBool32 *debug_utils_available);

void sulfur_debug_messenger_create(
    VkInstance instance, PFN_vkDebugUtilsMessengerCallbackEXT callback,
    VkDebugUtilsMessengerEXT *debug_messenger);

void sulfur_debug_messenger_destroy(VkInstance instance,
                                    VkDebugUtilsMessengerEXT messenger);

void sulfur_debug_report_callback_create(
    VkInstance instance, PFN_vkDebugReportCallbackEXT callback,
    VkDebugReportCallbackEXT *debug_report_callback);

void sulfur_debug_report_callback_destroy(
    VkInstance instance, VkDebugReportCallbackEXT debug_report_callback);

#endif // SULFUR_DEBUG_H
