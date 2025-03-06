#include "pch.h"
#include "VkBuffer.h"
#include "Gaia/Log.h"
#include "Gaia/Core.h"

namespace Gaia {
void VkBufferCreator::CreateBuffer(VkDevice& device, VmaAllocator& allocator, uint32_t size_in_bytes, VkBufferUsageFlags bufferUsageFlag, VmaMemoryUsage memoryUsage = VMA_MEMORY_USAGE_CPU_TO_GPU)
{
    VmaAllocationCreateInfo vma_allocInfo{};
    vma_allocInfo.usage = memoryUsage;


    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.pNext = nullptr;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE; //buffer sharing is only possible in single queue family
    bufferInfo.usage = bufferUsageFlag;
    bufferInfo.size = size_in_bytes;

    auto res = vmaCreateBuffer(allocator, &bufferInfo, &vma_allocInfo, &m_buffer, &m_allocation, nullptr);
    GAIA_ASSERT(res == VK_SUCCESS, "failed to create buffer error code:- {}", res);
    }

void VkBufferCreator::CopyBufferCPUToGPU(VmaAllocator& allocator, void* buffer, uint32_t size)
{
    //get the memory spot in the gpu so that we can copy the data
    void* data;
    vmaMapMemory(allocator, m_allocation, &data);
    memcpy(data, buffer, size);
    vmaUnmapMemory(allocator, m_allocation);
}

void VkBufferCreator::DestroyBuffer(VkDevice& device, VmaAllocator& allocator)
{
    vkDeviceWaitIdle(device);
    vmaDestroyBuffer(allocator, m_buffer, m_allocation);
}

}
