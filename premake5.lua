workspace "Gaia"

architecture "x86_64"

startproject "Gaia"

configurations
{
	"Debug",
	"Release",
	"Dist"
}

outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

IncludeDir = {}
IncludeDir["GLFW"] = "Gaia/vendor/GLFW/include"
IncludeDir["imgui"] = "Gaia/vendor/imgui"
IncludeDir["glm"] = "Gaia/vendor/glm"
IncludeDir["stb_image"] = "Gaia/vendor/stb_image"
IncludeDir["entt"] = "Gaia/vendor/entt"
IncludeDir["curl"] = "Gaia/vendor/Curl/include"
IncludeDir["json"]="Gaia/vendor/jsoncpp"
IncludeDir["assimp"]="Gaia/vendor/assimp/include"
IncludeDir["Physx"]="Gaia/vendor/physx_x64-windows/include"
-- IncludeDir["yaml_cpp"] = "Gaia/vendor/yaml_cpp/include"
IncludeDir["oidn"] = "Gaia/vendor/oidn/include"

include "Gaia/vendor/GLFW"
include "Gaia/vendor/imgui"
-- include "Gaia/vendor/yaml_cpp"

project "Gaia"

	location "Gaia"
	kind "StaticLib"
	language "c++"
	staticruntime "off"
	cppdialect "c++20"

	targetdir ("bin/"..outputdir.."/%{prj.name}")
	objdir ("bin-int/"..outputdir.."/%{prj.name}")


	pchheader "pch.h"
	pchsource "%{prj.name}/src/pch.cpp"


	files
	{
		"%{prj.name}/src/**.h",
		"%{prj.name}/src/**.cpp",
		"%{IncludeDir.stb_image}/**.h",
		"%{IncludeDir.stb_image}/**.cpp",
		"%{IncludeDir.curl}/**.h",
		"%{IncludeDir.curl}/**.c",
		"%{IncludeDir.curl}/**.cpp",
		"%{IncludeDir.json}/**.h",
		"%{IncludeDir.json}/**.cpp",
		"%{IncludeDir.json}/**.c",
		"%{IncludeDir.entt}/**.hpp",
		"%{IncludeDir.assimp}/**.h",
		"%{IncludeDir.assimp}/**.cpp",
		"%{IncludeDir.assimp}/**.hpp",
		"%{IncludeDir.Physx}/**.h",
		"%{IncludeDir.Physx}/**.hpp",
		"%{IncludeDir.Physx}/**.cpp",
		"%{IncludeDir.oidn}/**.h",
		"%{IncludeDir.oidn}/**.hpp",
	}

	includedirs
	{
		"$(VULKAN_SDK)/include",
		"%{prj.name}/src",
		"%{prj.name}/vendor/spdlog/include",
		"%{IncludeDir.GLFW}",
		"%{IncludeDir.imgui}",
		"%{IncludeDir.glm}",
		"%{IncludeDir.stb_image}",
		"%{IncludeDir.curl}",
		"%{IncludeDir.json}",
		"%{IncludeDir.entt}",
		"%{IncludeDir.assimp}",
		"%{IncludeDir.Physx}",
		-- "%{IncludeDir.yaml_cpp}",
		"%{IncludeDir.oidn}",
	}

	libdirs{
		"$(VULKAN_SDK)/lib",
	}

	links{
		-- "$(VULKAN_SDK)/lib/vulkan-1.lib",
		"$(VULKAN_SDK)/lib/SPIRV-Tools.lib",
		"$(VULKAN_SDK)/lib/SPIRV-Tools-diff.lib",
		"$(VULKAN_SDK)/lib/SPIRV-Tools-link.lib",
		"$(VULKAN_SDK)/lib/SPIRV-Tools-lint.lib",
		"$(VULKAN_SDK)/lib/SPIRV-Tools-opt.lib",
		"$(VULKAN_SDK)/lib/SPIRV-Tools-reduce.lib",
		"$(VULKAN_SDK)/lib/SPIRV-Tools-shared.lib",
		"$(VULKAN_SDK)/lib/SPIRV.lib",
		"$(VULKAN_SDK)/lib/glslang.lib",
		"$(VULKAN_SDK)/lib/slang.lib",
		"$(VULKAN_SDK)/lib/slang-rt.lib",
		"$(VULKAN_SDK)/lib/glslang-default-resource-limits.lib",
		"$(VULKAN_SDK)/lib/MachineIndependent.lib",
		"$(VULKAN_SDK)/lib/GenericCodeGen.lib",
		"$(VULKAN_SDK)/lib/spirv-cross-glsl.lib",
		"$(VULKAN_SDK)/lib/spirv-cross-hlsl.lib",
		"$(VULKAN_SDK)/lib/spirv-cross-util.lib",
		"$(VULKAN_SDK)/lib/spirv-cross-reflect.lib",

		"GLFW",
		"imgui",
		-- "yaml_cpp",
		"opengl32.lib",
		"Normaliz.lib",
		"Ws2_32.lib",
		"Wldap32.lib",
		"Crypt32.lib",
		"advapi32.lib",
		"Gaia/vendor/Curl/lib/libcurl_a_debug.lib",
		"Gaia/vendor/assimp/lib/x64/assimp-vc143-mt.lib",
		"Gaia/vendor/physx_x64-windows/lib/LowLevel_static_64.lib",
		"Gaia/vendor/physx_x64-windows/lib/LowLevelAABB_static_64.lib",
		"Gaia/vendor/physx_x64-windows/lib/LowLevelDynamics_static_64.lib",
		"Gaia/vendor/physx_x64-windows/lib/PhysX_64.lib",
		"Gaia/vendor/physx_x64-windows/lib/PhysXCharacterKinematic_static_64.lib",
		"Gaia/vendor/physx_x64-windows/lib/PhysXCommon_64.lib",
		"Gaia/vendor/physx_x64-windows/lib/PhysXCooking_64.lib",
		"Gaia/vendor/physx_x64-windows/lib/PhysXExtensions_static_64.lib",
		"Gaia/vendor/physx_x64-windows/lib/PhysXFoundation_64.lib",
		"Gaia/vendor/physx_x64-windows/lib/PhysXPvdSDK_static_64.lib",
		"Gaia/vendor/physx_x64-windows/lib/PhysXTask_static_64.lib",
		"Gaia/vendor/physx_x64-windows/lib/PhysXVehicle_static_64.lib",
		"Gaia/vendor/physx_x64-windows/lib/SceneQuery_static_64.lib",
		"Gaia/vendor/physx_x64-windows/lib/SimulationController_static_64.lib",
		"Gaia/vendor/oidn/lib/OpenImageDenoise.lib",
		"Gaia/vendor/oidn/lib/OpenImageDenoise_core.lib",

	}

	filter "system:windows"
		
		systemversion "latest"

		defines
		{
			"GAIA_PLATFORM_WINDOWS",
			"GAIA_BUILD_DLL",
			"GLFW_INCLUDE_NONE",
			"NDEBUG",
			"PX_PHYSX_STATIC_LIB"
		}


	filter "configurations:Debug"
		defines "GAIA_DEBUG"
		staticruntime "off"
		runtime "Debug"
		symbols "On"

	filter "configurations:Release"
		defines "GAIA_RELEASE"
		staticruntime "off"
		runtime "Release"
		optimize "On"

	filter "configurations:Dist"
		defines "GAUA_DIST"
		staticruntime "off"
		runtime "Release"
		optimize "On"
	

project "Gaia_Editor"

	location "Gaia_Editor"
	kind "ConsoleApp"
	language "c++"
	staticruntime "off"
	cppdialect "c++20"

	targetdir ("bin/"..outputdir.."/%{prj.name}")
	objdir ("bin-int/"..outputdir.."/%{prj.name}")

	files
	{
		"%{prj.name}/src/**.h",
		"%{prj.name}/src/**.cpp",
		"%{prj.name}/Assets/**.png"
	}
	includedirs
	{
		"Gaia/vendor/spdlog/include",
		"%{IncludeDir.imgui}",
		"%{IncludeDir.glm}",
		"%{IncludeDir.entt}",
		"%{IncludeDir.curl}",
		"%{IncludeDir.json}",
		"Gaia/src",
		"%{IncludeDir.Physx}",
		-- "%{IncludeDir.yaml_cpp}"
	}
	links "Gaia"

	filter "system:windows"
		
		systemversion "latest"

		defines
		{
			"GAIA_PLATFORM_WINDOWS"
		}
		
	filter "configurations:Debug"
		defines "GAIA_DEBUG"
		runtime "Debug"
		symbols "On"

	filter "configurations:Release"
		defines "GAIA_RELEASE"
		runtime "Release"
		optimize "On"

	filter "configurations:Dist"
		defines "GAIA_DIST"
		runtime "Release"
		optimize "On"
