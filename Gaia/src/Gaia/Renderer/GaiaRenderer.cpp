#include "pch.h"
#include "GaiaRenderer.h"

void Gaia::destroy(IContext* ctx, RenderPipelineHandle handle)
{
	if (ctx)
	{
		ctx->destroy(handle);
	}
}

void Gaia::destroy(IContext* ctx, ShaderModuleHandle handle)
{
	if (ctx)
	{
		ctx->destroy(handle);
	}
}

void Gaia::destroy(IContext* ctx, BufferHandle handle)
{
	if (ctx)
	{
		ctx->destroy(handle);
	}
}

void Gaia::destroy(IContext* ctx, TextureHandle handle)
{
	if (ctx)
	{
		ctx->destroy(handle);
	}
}

void Gaia::destroy(Gaia::IContext* ctx, Gaia::DescriptorSetLayoutHandle handle)
{
	if (ctx)
	{
		ctx->destroy(handle);
	}
}

void Gaia::destroy(IContext* ctx, SamplerHandle handle)
{
	if (ctx)
	{
		ctx->destroy(handle);
	}
}

void Gaia::destroy(Gaia::IContext* ctx, Gaia::AccelStructHandle handle)
{
	if (ctx)
	{
		ctx->destroy(handle);
	}
}

void Gaia::destroy(Gaia::IContext* ctx, Gaia::RayTracingPipelineHandle handle)
{
	if (ctx)
	{
		ctx->destroy(handle);
	}
}

void Gaia::destroy(IContext* ctx, ComputePipelineHandle handle)
{
	if (ctx)
	{
		ctx->destroy(handle);
	}
}
