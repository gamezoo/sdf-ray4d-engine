/*****************************************************
 * Class: FramebufferHelper (General)
 * Members: General Functions (Public/Private)
 * Partials: None
 *****************************************************/

#include "Helpers/Framebuffer.hpp"

using namespace sdfRay4d::helpers;

/**
 * @param[in] _device
 * @param[in] _deviceFuncs
 * @param[in] _attachments
 */
FramebufferHelper::FramebufferHelper(
  const Device &_device,
  QVulkanDeviceFunctions *_deviceFuncs,
  texture::ImageView *_attachments
) :
  m_device(_device)
, m_deviceFuncs(_deviceFuncs)
, m_attachments(_attachments)
{}

/**
 * @brief sets default Qt Vulkan framebuffer
 * @param[in] _framebuffer
 */
void FramebufferHelper::setDefaultFramebuffer(const Framebuffer &_framebuffer)
{
  m_defaultFrameBuffer = _framebuffer;
}

/**
 * @brief sets framebuffer width and height
 * @param[in] _extentWidth
 * @param[in] _extentHeight
 */
void FramebufferHelper::setSize(uint32_t _extentWidth, uint32_t _extentHeight)
{
  m_extentWidth = _extentWidth;
  m_extentHeight = _extentHeight;
}

/**
 * @brief retrieves current in-use framebuffer instance
 * @param[in] _useDefault
 * @return Framebuffer instance
 */
Framebuffer &FramebufferHelper::getFramebuffer(
  const RenderPass &_renderPass,
  bool _useDefault
)
{
  /**
   * @note
   * Since this method is called per frame,
   * to make framebuffer instance reusable/non-copyable
   * and so to be able to draw to it per frame,
   * like a singleton instance
   */
  if(!m_frameBuffer && !_useDefault)
  {
    createFramebuffer(_renderPass);
  }

  /**
   * @note
   * if useDefault  => already populated with Qt default framebuffer with command helper's init function in render loop
   * else           => custom framebuffer is created and populated by above
   */
  return _useDefault ? m_defaultFrameBuffer : m_frameBuffer;
}

/**
 * @brief creates a custom (non Qt-Vulkan) framebuffer
 */
void FramebufferHelper::createFramebuffer(const RenderPass &_renderPass)
{
  if(m_extentWidth == 0 || m_extentHeight == 0)
  {
    qWarning("Framebuffer width or height is not set!");

    return;
  }

  FramebufferInfo framebufferInfo = {}; // memset

  framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
  framebufferInfo.renderPass = _renderPass; // custom renderPass
  framebufferInfo.attachmentCount = 1; // depth pass attachment image view is only 1
  framebufferInfo.pAttachments = m_attachments;
  framebufferInfo.width = m_extentWidth;
  framebufferInfo.height = m_extentHeight;
  framebufferInfo.layers = 1;

  auto result = m_deviceFuncs->vkCreateFramebuffer(
    m_device,
    &framebufferInfo,
    nullptr,
    &m_frameBuffer
  );

  if (result != VK_SUCCESS)
  {
    qWarning("Failed to create Framebuffer: %d", result);
  }
}