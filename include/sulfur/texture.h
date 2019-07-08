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
VkResult sulfur_texture_create(SulfurDevice *device, int width, int height,
                           VkFormat format, SulfurTexture *texture);

/**
 * Create a texture from an image file.
 */
VkResult sulfur_texture_create_from_image(SulfurDevice *device, VkFormat format,
                                      int width, int height,
                                      const unsigned char *pixels,
                                      SulfurTexture *texture);

/**
 * Destroy a texture.
 */
void sulfur_texture_destroy(SulfurDevice *device, SulfurTexture *texture);

/**
 * Transition the texture image to a given VkImageLayout.
 */
void sulfur_texture_transition_layout(SulfurDevice *device,
                                      SulfurTexture *texture,
                                      VkImageLayout old_layout,
                                      VkImageLayout new_layout);

#endif // SULFUR_IMAGE_H_
