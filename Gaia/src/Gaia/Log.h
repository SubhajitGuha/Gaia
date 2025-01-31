#pragma once
#include "pch.h"

#include "Core.h"
#include "spdlog/spdlog.h"
#include "spdlog/fmt/ostr.h"

namespace Gaia {

	class Log {
	public:
		static void init();

		inline static std::shared_ptr<spdlog::logger>& GetCoreLogger() { return s_corelogger; }
		inline static std::shared_ptr<spdlog::logger>& GetAppLogger() { return s_applogger; }

	private:
		static std::shared_ptr<spdlog::logger> s_corelogger;
		static std::shared_ptr<spdlog::logger> s_applogger;
	};

}

//core
#define GAIA_CORE_TRACE(...) Gaia::Log::GetCoreLogger()->trace(__VA_ARGS__)
#define GAIA_CORE_ERROR(...) Gaia::Log::GetCoreLogger()->error(__VA_ARGS__)
#define GAIA_CORE_WARN(...) Gaia::Log::GetCoreLogger()->warn(__VA_ARGS__)
#define GAIA_CORE_INFO(...) Gaia::Log::GetCoreLogger()->info(__VA_ARGS__)
//#define GAIA_CORE_FETAL(...) Gaia::Log::GetCoreLogger()->fetal(__VA_ARGS__)

//app
#define GAIA_TRACE(...) Gaia::Log::GetAppLogger()->trace(__VA_ARGS__)
#define GAIA_ERROR(...) Gaia::Log::GetAppLogger()->error(__VA_ARGS__)
#define GAIA_WARN(...) Gaia::Log::GetAppLogger()->warn(__VA_ARGS__)
#define GAIA_INFO(...) Gaia::Log::GetAppLogger()->info(__VA_ARGS__)