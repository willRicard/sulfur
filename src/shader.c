#include <sulfur/shader.h>

#include <stdlib.h>

VkResult sulfur_shader_create(const SulfurDevice *dev, const char *shader_code,
                              const uint32_t shader_code_size,
                              const VkShaderStageFlags shader_stage,
                              SulfurShader *shader) {
  VkShaderModuleCreateInfo module_info = {};
  module_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
  module_info.pNext = NULL;
  module_info.flags = 0;
  module_info.pCode = (const uint32_t *)shader_code;
  module_info.codeSize = shader_code_size;

  VkResult result =
      vkCreateShaderModule(dev->device, &module_info, NULL, &shader->module);

  if (result != VK_SUCCESS) {
    return result;
  }

  shader->sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  shader->pNext = NULL;
  shader->flags = 0;
  shader->stage = shader_stage;
  shader->pName = "main";
  shader->pSpecializationInfo = NULL;

  return VK_SUCCESS;
}

void sulfur_shader_destroy(const SulfurDevice *dev, SulfurShader *shader) {
  vkDestroyShaderModule(dev->device, shader->module, NULL);
}
