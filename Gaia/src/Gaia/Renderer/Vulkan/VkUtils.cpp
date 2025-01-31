#include "pch.h"
#include "VkUtils.h"
#include "VkInitializers.h"

namespace Gaia
{
	namespace vkutil
	{
		void transition_image(VkCommandBuffer cmd, VkImage image, VkImageLayout currentLayout, VkImageLayout newLayout)
		{

			VkImageMemoryBarrier2 imageBarrier{};
			imageBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
			imageBarrier.pNext = nullptr;

			imageBarrier.srcStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
			imageBarrier.dstStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
			imageBarrier.srcAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT;
			imageBarrier.dstAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT | VK_ACCESS_MEMORY_READ_BIT;
			
			imageBarrier.oldLayout = currentLayout;
			imageBarrier.newLayout = newLayout;

			VkImageAspectFlags aspectMask = (newLayout == VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL) ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;
			imageBarrier.subresourceRange = vkinit::image_subresource_range(aspectMask);
			imageBarrier.image = image;

			VkDependencyInfo depInfo{};
			depInfo.dependencyFlags = 0;
			depInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
			depInfo.pNext = nullptr;
			depInfo.imageMemoryBarrierCount = 1;
			depInfo.pImageMemoryBarriers = &imageBarrier;

			vkCmdPipelineBarrier2(cmd, &depInfo);

		}
		void copy_image_to_image(VkCommandBuffer cmd, VkImage src, VkImage dst, VkExtent2D srcSize, VkExtent2D dstSize)
		{
			VkImageBlit2 imageBlit{};
			//define the src max bounds (min bounds = (0,0,0))
			imageBlit.srcOffsets[1].x = srcSize.width;
			imageBlit.srcOffsets[1].y = srcSize.height;
			imageBlit.srcOffsets[1].z = 1;

			imageBlit.dstOffsets[1].x = dstSize.width;
			imageBlit.dstOffsets[1].y = dstSize.height;
			imageBlit.dstOffsets[1].z = 1;

			imageBlit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			imageBlit.srcSubresource.baseArrayLayer = 0;
			imageBlit.srcSubresource.layerCount = 1;
			imageBlit.srcSubresource.mipLevel = 0;

			imageBlit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			imageBlit.dstSubresource.baseArrayLayer = 0;
			imageBlit.dstSubresource.layerCount = 1;
			imageBlit.dstSubresource.mipLevel = 0;

			imageBlit.sType = VK_STRUCTURE_TYPE_IMAGE_BLIT_2;
			imageBlit.pNext = nullptr;

			VkBlitImageInfo2 imageInfo{};
			imageInfo.sType = VK_STRUCTURE_TYPE_BLIT_IMAGE_INFO_2;
			imageInfo.pNext = nullptr;
			imageInfo.srcImage = src;
			imageInfo.dstImage = dst;
			imageInfo.dstImageLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			imageInfo.srcImageLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
			imageInfo.filter = VK_FILTER_LINEAR;
			imageInfo.regionCount = 1;
			imageInfo.pRegions = &imageBlit;

			vkCmdBlitImage2(cmd, &imageInfo);
		}
	}
}