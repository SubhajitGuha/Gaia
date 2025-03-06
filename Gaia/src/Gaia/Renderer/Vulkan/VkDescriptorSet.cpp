#include <pch.h>
#include "VkDescriptorSet.h"

namespace Gaia
{
	void DescriptorLayoutBuilder::add_binding(uint32_t binding, VkDescriptorType type)
	{
		VkDescriptorSetLayoutBinding newbind{};
		newbind.binding = binding;
		newbind.descriptorCount = 1;
		newbind.descriptorType = type;

		bindings.push_back(newbind);
	}
	void DescriptorLayoutBuilder::clear()
	{
		bindings.clear();
	}
	VkDescriptorSetLayout DescriptorLayoutBuilder::build(VkDevice device, VkShaderStageFlags shaderStages, void* pNext, VkDescriptorSetLayoutCreateFlags flags)
	{
		for (auto& b : bindings)
		{
			b.stageFlags |= shaderStages;
		}

		VkDescriptorSetLayoutCreateInfo descSetCreateInfo{};
		descSetCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		descSetCreateInfo.pNext = pNext;
		descSetCreateInfo.pBindings = bindings.data();
		descSetCreateInfo.flags = flags;
		descSetCreateInfo.bindingCount = (uint32_t)bindings.size();

		VkDescriptorSetLayout set_layout;
		auto res = vkCreateDescriptorSetLayout(device, &descSetCreateInfo, nullptr, &set_layout);
		GAIA_ASSERT(res == VK_SUCCESS, "failed to create descriptor set layout error code {}", res);

		return set_layout;
	}
	void DescriptorWriter::write_image(int binding, VkImageView image, VkSampler sampler, VkImageLayout layout, VkDescriptorType type)
	{
		VkDescriptorImageInfo imageInfo{};
		imageInfo.imageView = image;
		imageInfo.imageLayout = layout;
		imageInfo.sampler = sampler;

		VkDescriptorImageInfo& info = imageInfos.emplace_back(imageInfo);

		VkWriteDescriptorSet write{};
		write.dstBinding = binding;
		write.write = VK_NULL_HANDLE;
		write.descriptorCount = 1;
		write.descriptorType = type;
		write.pImageInfo = &info;

		writes.push_back(write);
	}
	void DescriptorWriter::write_buffer(int binding, VkBuffer buffer, size_t size, size_t offset, VkDescriptorType type)
	{
		VkDescriptorBufferInfo bufInfo{};
		bufInfo.buffer = buffer;
		bufInfo.offset = offset;
		bufInfo.range = size;

		VkDescriptorBufferInfo& info = bufferInfos.emplace_back(bufInfo);

		VkWriteDescriptorSet write{};

		write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		write.dstBinding = binding;
		write.write = VK_NULL_HANDLE;
		write.descriptorCount = 1;
		write.descriptorType = type;
		write.pBufferInfo = &info;

		writes.push_back(write);
	}
	void DescriptorWriter::clear()
	{
		imageInfos.clear();
		writes.clear();
		bufferInfos.clear();
	}
	void DescriptorWriter::update_set(VkDevice device, VkDescriptorSet set)
	{
		for (VkWriteDescriptorSet& write : writes) {
			write.write = set;
		}

		vkUpdateDescriptorSets(device, (uint32_t)writes.size(), writes.data(), 0, nullptr);
	}
	void DescriptorAllocator::init_pool(VkDevice device, uint32_t maxSets,const std::vector<PoolSizeRatio>& poolRatios)
	{
		std::vector<VkDescriptorPoolSize> poolSizes;
		for (auto& ratio : poolRatios)
		{
			VkDescriptorPoolSize poolSize{};
			poolSize.descriptorCount = uint32_t(ratio.ratio * maxSets);
			poolSize.type = ratio.type;
			poolSizes.push_back(poolSize);
		}


		VkDescriptorPoolCreateInfo pool_info{};
		pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		pool_info.flags = 0;
		pool_info.maxSets = maxSets;
		pool_info.poolSizeCount = (uint32_t)poolSizes.size();
		pool_info.pPoolSizes = poolSizes.data();

		auto res = vkCreateDescriptorPool(device, &pool_info, nullptr, &pool);
		GAIA_ASSERT(res == VK_SUCCESS, "failed to create descriptor pool error code {}", res);

	}
	void DescriptorAllocator::clear_descriptors(VkDevice device)
	{
		vkResetDescriptorPool(device, pool, 0);
	}
	void DescriptorAllocator::destroy_pool(VkDevice device)
	{
		vkDestroyDescriptorPool(device, pool, nullptr);
	}
	VkDescriptorSet DescriptorAllocator::allocate(VkDevice device, VkDescriptorSetLayout layout)
	{
		VkDescriptorSetAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocInfo.descriptorPool = pool;
		allocInfo.descriptorSetCount = 1;
		allocInfo.pSetLayouts = &layout;

		VkDescriptorSet desc_set;
		auto res = vkAllocateDescriptorSets(device, &allocInfo, &desc_set);
		GAIA_ASSERT(res == VK_SUCCESS, "failed to allocate descriptor set, error code {}", res);

		return desc_set;
	}
	void DescriptorAllocatorGrowable::init(VkDevice device, uint32_t initialSets, std::vector<PoolSizeRatio> poolRatios)
	{
	}
	void DescriptorAllocatorGrowable::clear_pools(VkDevice device)
	{
	}
	void DescriptorAllocatorGrowable::destroy_pools(VkDevice device)
	{
	}
	VkDescriptorSet DescriptorAllocatorGrowable::allocate(VkDevice device, VkDescriptorSetLayout layout, void* pNext)
	{
		return VkDescriptorSet();
	}
	VkDescriptorPool DescriptorAllocatorGrowable::get_pool(VkDevice device)
	{
		return VkDescriptorPool();
	}
	VkDescriptorPool DescriptorAllocatorGrowable::create_pool(VkDevice device, uint32_t setCount, std::vector<PoolSizeRatio> poolRatios)
	{
		return VkDescriptorPool();
	}
}