#include "Editor/Logger.h"

#include <cstdint>
#include <cstdarg>

//Log logg;
static Logger_t logger;

void Logger::AddLog(Channel::Channels channel, const char* fmt, ...)
{
	std::va_list listArgs;
	logger.buff.appendf("[Logger::%s] ", Logger::Channel::channelText[log2((std::uint8_t)channel)]);

	va_start(listArgs, fmt);
	logger.buff.appendfv(fmt, listArgs);
	va_end(listArgs);
}

void Logger::RenderLogs()
{
	ImGui::BeginChild("scrolling");
	ImGui::TextUnformatted(logger.buff.c_str());
	ImGui::EndChild();
}
