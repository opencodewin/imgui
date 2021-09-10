#pragma once
#include "imgui.h"      // IMGUI_IMPL_API
#include "gpu.h"
#include "pipeline.h"
#include "command.h"
#include "filters/ColorConvert_vulkan.h"
#include "filters/Resize_vulkan.h"
#include <vulkan/vulkan.h>

namespace ImGui
{
#if IMGUI_RENDERING_VULKAN
    IMGUI_API ImTextureID ImVulkanImageToImTexture(const VkImageMat & image_vk);
#endif
    IMGUI_API void ImMatToImVulkanMat(const ImMat &src, VkMat &dst);
    IMGUI_API void ImVulkanVkMatToImMat(const VkMat &src, ImMat &dst);
    IMGUI_API void ImVulkanVkMatToVkImageMat(const VkMat &src, VkImageMat &dst);
    IMGUI_API void ImVulkanShaderClear();
} //namespace ImGui