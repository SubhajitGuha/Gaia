#pragma once
#include "VkPipeline.h"
#include "Gaia/Core.h"
#include "Gaia/Log.h"
#include "Gaia/Events/Event.h"
#include "VkDescriptorSet.h"
#include "VkBuffer.h"
#include "glm/glm.hpp"

//#include "Gaia/LoadMesh.h"

namespace Gaia {
	class LoadMesh;
	class EditorCamera;

	//gpu camera buffer
	struct CameraBuffer
	{
		glm::mat4 model_matrix;
		glm::mat4 projection_matrix;
		glm::mat4 view_matrix;
	};
	class VkRenderer
	{
	public:
		VkRenderer(float width, float height);
		void OnEvent(Event& e);
		void OnUpdate(float ts);
		//void Render()
		void Destroy();
		VkPipeline pipeline;
		ref<LoadMesh> mesh;
		ref<EditorCamera> camera;
		VkDescriptorSet camera_desc_set;
		ref<VkPipelineBuilder> graphics_pipeline;
	private:
		DescriptorAllocator desc_allocator;
		DescriptorLayoutBuilder desc_layout_builder;
		DescriptorWriter desc_writer;

		VkBufferCreator camera_buffer;


		VkShaderModule vertex_shader;
		VkShaderModule fragment_shader;
		VkDescriptorSetLayout camera_desc_layout;
	};
}

