#ifndef SULFUR_SWAPCHAIN_H
#define SULFUR_SWAPCHAIN_H
#include <vulkan/vulkan.h>

#include <sulfur/device.h>

/**
 * Image swapchain.
 */
typedef struct SulfurSwapchain {
  VkSwapchainKHR swapchain;

  VkSwapchainCreateInfoKHR info; /** Swapchain details */

  VkRenderPass render_pass; /** Write to color attachment. */

  uint32_t image_count; /** Number of multiple buffering images */
  VkImageView image_views[4];
  VkFramebuffer framebuffers[4];

  VkCommandBuffer command_buffers[4]; /** Command buffers run every frame */

  // Synchronization primitives
  uint32_t frame_id;
  VkSemaphore image_available_semaphores[3];
  VkSemaphore render_finished_semaphores[3];
  VkFence fences[3];
} SulfurSwapchain;

/**
 * Create an image swapchain presenting to a window surface.
 */
VkResult sulfur_swapchain_create(SulfurDevice *device, VkSurfaceKHR surface,
                                 SulfurSwapchain *swapchain);

void sulfur_swapchain_destroy(SulfurDevice *device, SulfurSwapchain *swapchain);

/**
 * Recreate the swapchain, image views and framebuffers.
 */
VkResult sulfur_swapchain_resize(SulfurDevice *device, VkSurfaceKHR surface,
                                 SulfurSwapchain *swapchain);

/**
 * Destroy the swapchain, image views and framebuffers.
 */
void sulfur_swapchain_cleanup(SulfurDevice *device, SulfurSwapchain *swapchain);

/**
 * Submit the command buffers and present to the window surface.
 */
int sulfur_swapchain_present(SulfurDevice *device, VkSurfaceKHR surface,
                             SulfurSwapchain *swapchain);

#endif // SULFUR_SWAPCHAIN_H
