#ifndef LOGGER_H_
#define LOGGER_H_

#include <imgui.h>
#include <string>
#include <cstdint>
#include <array>

namespace zorya
{
	struct Logger_t
	{
		ImGuiTextBuffer buff;
		ImGuiTextFilter filter;
	};

	namespace Logger
	{

		namespace Channel
		{
			enum Channels : std::uint8_t
			{
				INFO = 1,
				WARNING = 2,
				ERR = 4,
				TRACE = 8
			};
			const std::array<const char*, 4> channel_text = { "INFO", "WARNING", "ERROR", "TRACE" };
		}

		void add_log(Channel::Channels channel, const char* fmt, ...);
		void render_logs();
	}

}



#endif