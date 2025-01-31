#include <memory>
#include "Log.h"
#pragma once
#ifdef GAIA_PLATFORM_WINDOWS
#ifdef GAIA_DYNAMIC_LINK
#ifdef GAIA_BUILD_DLL
#define GAIA_API __declspec(dllexport)
#else
#define GAIA_API __declspec(dllimport)
#endif // GAIA_BUILD_DLL
#else
#define GAIA_API
#endif 
#else
#error Hazel only supports windows
#endif // GAIA_PLATFORM_WINDOWS

#ifdef GAIA_DEBUG
#if GAIA_PLATFORM_WINDOWS
#define GAIA_DEBUGBREAK() __debugbreak()
#else
#error "This platform is not supported"
#endif
#define GAIA_ENABLE_ASSERTS
#else
#define GAIA_DEBUGBREAK()
#endif // GAIA_DEBUG

#ifdef GAIA_DEBUG
#define GAIA_ASSERT(x,...) { if(!(x)) { GAIA_ERROR("Assertion Failed: {0}", __VA_ARGS__); GAIA_DEBUGBREAK(); } }
#else
#define GAIA_ASSERT(x, ...)
#endif
#define BIT(x) (1<<x)
#define GAMMA 2.2

//engine texture slots
#define ALBEDO_SLOT 1
#define ROUGHNESS_SLOT 2
#define NORMAL_SLOT 3
#define ENV_SLOT 4
#define IRR_ENV_SLOT 5
#define SHDOW_MAP1 6
#define SHDOW_MAP2 7
#define SHDOW_MAP3 8
#define SHDOW_MAP4 9
#define SSAO_SLOT 10
#define SSAO_BLUR_SLOT 11
#define NOISE_SLOT 12
#define SCENE_TEXTURE_SLOT 13
#define SCENE_DEPTH_SLOT 14
#define ORIGINAL_SCENE_TEXTURE_SLOT 15
#define HEIGHT_MAP_TEXTURE_SLOT 16
#define FOLIAGE_DENSITY_TEXTURE_SLOT 17
#define PERLIN_NOISE_TEXTURE_SLOT 18
#define BLUE_NOISE_TEXTURE_SLOT 29
#define G_COLOR_TEXTURE_SLOT 20
#define G_NORMAL_TEXTURE_SLOT 21
#define G_ROUGHNESS_METALLIC_TEXTURE_SLOT 22
#define G_VELOCITY_BUFFER_SLOT 23
#define LUT_SLOT 24
#define PT_IMAGE_SLOT 25 //Image slot for path tracing to copy the accmulated image
#define HISTORY_TEXTURE_SLOT 26
#define TERRAIN_MASK_TEXTURE_SLOT 27


namespace Gaia {

	template<typename T>
	using ref = std::shared_ptr<T>;

	template<typename X>
	using scope = std::unique_ptr<X>;

}