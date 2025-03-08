#pragma once
#include "vulkan/vulkan.hpp"

namespace Gaia
{
	namespace vkutil
	{
		void transition_image(VkCommandBuffer cmd, VkImage image, VkImageLayout currentLayout, VkImageLayout newLayout);
		void copy_image_to_image(VkCommandBuffer cmd, VkImage src, VkImage dst, VkExtent2D srcSize, VkExtent2D dstSize);
		VkSemaphore createSemaphore(VkDevice device);
		VkPipelineShaderStageCreateInfo getPipelineShaderStageCreateInfo(VkShaderStageFlagBits stage, VkShaderModule shaderModule, const char* entry_point, const VkSpecializationInfo* specializationInfo);
	}
}

