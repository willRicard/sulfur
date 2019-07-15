#include <sulfur/pipeline.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vulkan/vulkan.h>

void sulfur_pipeline_make_default_create_info(
    const SulfurSwapchain *swapchain, VkGraphicsPipelineCreateInfo *info) {
  static VkPipelineVertexInputStateCreateInfo input_info = {};
  input_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

  static VkPipelineInputAssemblyStateCreateInfo assembly_info = {};
  assembly_info.sType =
      VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
  assembly_info.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
  assembly_info.primitiveRestartEnable = VK_FALSE;

  VkExtent2D swapchain_extent = swapchain->info.imageExtent;

  static VkViewport viewport = {};
  viewport.x = 0.0f;
  viewport.y = 0.0f;
  viewport.width = (float)swapchain_extent.width;
  viewport.height = (float)swapchain_extent.height;
  viewport.minDepth = 0.0f;
  viewport.maxDepth = 1.0f;

  static VkRect2D scissor = {};
  scissor.extent = swapchain_extent;
  scissor.offset.x = 0;
  scissor.offset.y = 0;

  static VkPipelineViewportStateCreateInfo viewport_info = {};
  viewport_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
  viewport_info.viewportCount = 1;
  viewport_info.pViewports = &viewport;
  viewport_info.scissorCount = 1;
  viewport_info.pScissors = &scissor;

  static const VkPipelineRasterizationStateCreateInfo rasterization = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
      .polygonMode = VK_POLYGON_MODE_FILL,
      .cullMode = VK_CULL_MODE_NONE,
      .frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
      .lineWidth = 1.0f};

  static const VkPipelineMultisampleStateCreateInfo multisample = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
      .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT};

  static const VkPipelineDepthStencilStateCreateInfo depth_info = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
      .maxDepthBounds = 1.f};

  static const VkPipelineColorBlendAttachmentState blend_attachment = {
      .blendEnable = VK_FALSE,
      .srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
      .dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
      .colorBlendOp = VK_BLEND_OP_ADD,
      .srcAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
      .dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
      .alphaBlendOp = VK_BLEND_OP_ADD,
      .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                        VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT};

  static const VkPipelineColorBlendStateCreateInfo blend_info = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
      .logicOp = VK_LOGIC_OP_COPY,
      .attachmentCount = 1,
      .pAttachments = &blend_attachment};

  info->sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
  info->pVertexInputState = &input_info;
  info->pInputAssemblyState = &assembly_info;
  info->pTessellationState = NULL;
  info->pViewportState = &viewport_info;
  info->pRasterizationState = &rasterization;
  info->pMultisampleState = &multisample;
  info->pDepthStencilState = &depth_info;
  info->pColorBlendState = &blend_info;
  info->pDynamicState = NULL;
  info->renderPass = swapchain->render_pass;
  info->subpass = 0;
  info->basePipelineHandle = VK_NULL_HANDLE;
  info->basePipelineIndex = -1;
}
