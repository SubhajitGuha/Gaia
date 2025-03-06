#pragma once
#include "vulkan/vulkan.h"
#include "vma/vk_mem_alloc.h"

namespace Gaia {

	class VkBufferCreator
	{
	public:
		void CreateBuffer(VkDevice& device, VmaAllocator& allocator, uint32_t size_in_bytes, VkBufferUsageFlags bufferUsageFlag, VmaMemoryUsage memoryUsage);
		void CopyBufferCPUToGPU(VmaAllocator& allocator, void* buffer, uint32_t size);
		void DestroyBuffer(VkDevice& device, VmaAllocator& allocator);
		VkBuffer m_buffer;
	private:
		VmaAllocation m_allocation;

	};
}


