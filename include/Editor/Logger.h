#include <imgui.h>
#include <string>
#include <cstdint>
#include <array>

struct Logger_t {
	ImGuiTextBuffer buff;
	ImGuiTextFilter filter;
};

namespace Logger {

	namespace Channel {
		enum Channels : std::uint8_t {
			INFO = 1,
			WARNING = 2,
			ERR = 4,
			TRACE = 8
		};
		const std::array<const char*,4> channelText = {"INFO", "WARNING", "ERROR", "TRACE"};
	}

	void AddLog(Channel::Channels channel, const char* fmt, ...);
	void RenderLogs();
}