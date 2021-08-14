/*****************************************************
 * Partial Class: Renderer
 * Members: Init Materials Helpers (Private)
 *****************************************************/

#include "Renderer.hpp"

using namespace sdfRay4d;

void Renderer::initActorMaterial(const PhysicalDeviceLimits *_limits)
{
  m_actorMaterial = std::make_shared<Material>(
    m_device,
    m_deviceFuncs
  );

  auto uniAlign = _limits->minUniformBufferOffsetAlignment;

  // Note the std140 packing rules. A vec3 still has an alignment of 16,
  // while a mat3 is like 3 * vec3.
  m_actorMaterial->vertUniSize = aligned(2 * 64 + 48, uniAlign); // see color_phong.vert
  m_actorMaterial->fragUniSize = aligned(6 * 16 + 12 + 2 * 4, uniAlign); // see color_phong.frag

  m_actorMaterial->layoutBindings[0] = {
    0, // binding
    VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, // descriptorType
    1, // descriptorCount
    VK_SHADER_STAGE_VERTEX_BIT, // stageFlags
    nullptr // pImmutableSamplers
  };
  m_actorMaterial->layoutBindings[1] = {
    1, // binding
    VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, // descriptorType
    1, // descriptorCount
    VK_SHADER_STAGE_FRAGMENT_BIT, // stageFlags
    nullptr // pImmutableSamplers
  };
  m_actorMaterial->descSetLayoutCount = 1;
}

void Renderer::initSDFRMaterial(const PhysicalDeviceLimits *_limits)
{
  m_sdfrMaterial = std::make_shared<Material>(
    m_device,
    m_deviceFuncs
  );

  m_sdfrMaterial->createPushConstantRange(
    0,
    64
  );

  // TODO : might not need this for depth texture
  auto &maxSamplerAnisotropy = _limits->maxSamplerAnisotropy;

  m_sdfrMaterial->depthTexture.createSampler(maxSamplerAnisotropy);

  m_sdfrMaterial->layoutBindings[0] = {
    0, // binding
    VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, // descriptorType
    1, // descriptorCount
    VK_SHADER_STAGE_VERTEX_BIT, // stageFlags
    nullptr // pImmutableSamplers
  };
  m_sdfrMaterial->layoutBindings[1] = {
    1, // binding
    VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, // descriptorType
    1, // descriptorCount
    VK_SHADER_STAGE_FRAGMENT_BIT, // stageFlags
    nullptr // pImmutableSamplers
  };
  m_sdfrMaterial->descSetLayoutCount = 1;
}
