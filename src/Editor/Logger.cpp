#include "editor/Logger.h"

#include <cstdint>
#include <cstdarg>

namespace zorya
{
	//Log logg;
	static Logger_t logger;

	void Logger::add_log(Channel::Channels channel, const char* fmt, ...)
	{
		std::va_list list_args;
		logger.buff.appendf("[Logger::%s] ", Logger::Channel::channel_text[log2((uint8_t)channel)]);

		va_start(list_args, fmt);
		logger.buff.appendfv(fmt, list_args);
		va_end(list_args);
	}

	void Logger::render_logs()
	{
		ImGui::BeginChild("scrolling");
		ImGui::TextUnformatted(logger.buff.c_str());
		ImGui::EndChild();
	}

}


