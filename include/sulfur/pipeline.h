#ifndef SULFUR_PIPELINE_H
#define SULFUR_PIPELINE_H
#include <vulkan/vulkan.h>

#include <sulfur/device.h>
#include <sulfur/swapchain.h>

/**
 * Default pipeline create info with convenient defaults.
 */
void sulfur_pipeline_make_default_create_info(
    const SulfurSwapchain *swapchain, VkGraphicsPipelineCreateInfo *info);

#endif // SULFUR_PIPELINE_H
