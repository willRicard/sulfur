#ifndef SULFUR_IMAGE_H_
#define SULFUR_IMAGE_H_
#include <vulkan/vulkan.h>

#include <sulfur/device.h>

typedef struct SulfurTexture {
  VkImage image;
  VkDeviceMemory image_memory;
  VkImageView image_view;
  VkSampler sampler;
  int width;
  int height;
} SulfurTexture;

/**
 * Create an empty texture.
 */
VkResult sulfur_texture_create(const SulfurDevice *device, const int width,
                               const int height, const VkFormat format,
                               SulfurTexture *texture);

/**
 * Create a texture from an image file.
 */
VkResult sulfur_texture_create_from_image(const SulfurDevice *device,
                                          const VkFormat format,
                                          const int width, const int height,
                                          const unsigned char *pixels,
                                          SulfurTexture *texture);

/**
 * Destroy a texture.
 */
void sulfur_texture_destroy(const SulfurDevice *device, SulfurTexture *texture);

/**
 * Transition the texture image to a given VkImageLayout.
 */
void sulfur_texture_transition_layout(const SulfurDevice *device,
                                      const SulfurTexture *texture,
                                      const VkImageLayout old_layout,
                                      const VkImageLayout new_layout);

#endif // SULFUR_IMAGE_H_
