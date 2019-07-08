#ifndef SULFUR_SHADER_H
#define SULFUR_SHADER_H
#include <vulkan/vulkan.h>

#include <sulfur/device.h>

/**
 * Contains a shader module.
 */
typedef VkPipelineShaderStageCreateInfo SulfurShader;

/**
 * Compile a shader module from SPIR-V source.
 */
VkResult sulfur_shader_create(SulfurDevice *dev, const char *shader_code,
                              uint32_t shader_code_size,
                              VkShaderStageFlags shader_stage,
                              SulfurShader *shader);

/**
 * Destroy a shader module.
 */
void sulfur_shader_destroy(SulfurDevice *device, SulfurShader *shader);

#endif // SULFUR_SHADER_H
