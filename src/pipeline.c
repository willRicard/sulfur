#include <sulfur/pipeline.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vulkan/vulkan.h>

void sulfur_pipeline_make_default_create_info(
    SulfurSwapchain *swapchain, VkGraphicsPipelineCreateInfo *info) {
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

  static VkPipelineRasterizationStateCreateInfo rasterization = {};
  rasterization.sType =
      VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
  rasterization.polygonMode = VK_POLYGON_MODE_FILL;
  rasterization.cullMode = VK_CULL_MODE_NONE;
  rasterization.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
  rasterization.lineWidth = 1.0f;

  static VkPipelineMultisampleStateCreateInfo multisample = {};
  multisample.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
  multisample.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

  static VkPipelineDepthStencilStateCreateInfo depth_info = {};
  depth_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
  depth_info.maxDepthBounds = 1.f;

  static VkPipelineColorBlendAttachmentState blend_attachment = {};
  blend_attachment.blendEnable = VK_FALSE;
  blend_attachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
  blend_attachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
  blend_attachment.colorBlendOp = VK_BLEND_OP_ADD;
  blend_attachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
  blend_attachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
  blend_attachment.alphaBlendOp = VK_BLEND_OP_ADD;
  blend_attachment.colorWriteMask =
      VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
      VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

  static VkPipelineColorBlendStateCreateInfo blend_info = {};
  blend_info.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
  blend_info.logicOp = VK_LOGIC_OP_COPY;
  blend_info.attachmentCount = 1;
  blend_info.pAttachments = &blend_attachment;

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
