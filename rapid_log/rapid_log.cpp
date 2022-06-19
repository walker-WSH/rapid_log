#include "rapid_log.h"
#include "memory_map_writer.h"
#include <Windows.h>
#include <string>
#include <mutex>

#define MAX_LOG_LEN 4096

std::recursive_mutex log_lock;
CMemoryMapWriter log_writer;
std::string str_zone = "";

extern "C" bool init_log(const wchar_t *log_file)
{
	TIME_ZONE_INFORMATION tmp;
	::GetTimeZoneInformation(&tmp);

	std::lock_guard<std::recursive_mutex> auto_lock(log_lock);
	str_zone = std::to_string(tmp.Bias / (-60));
	return log_writer.Open(log_file, true);
}

extern "C" void uninit_log()
{
	std::lock_guard<std::recursive_mutex> auto_lock(log_lock);
	str_zone = "";
	log_writer.Close();
}

extern "C" void rapid_log(rapid_log_level level, const char *file, const char *func, int line, const char *msg)
{
	if (str_zone.empty()) {
		assert(false);
		return;
	}

	SYSTEMTIME systime;
	GetLocalTime(&systime);

	boost::format format_time("%1%/%2%/%3%-%4%:%5%:%6%+%7%(+%8%H)");
	format_time % systime.wYear % systime.wMonth % systime.wDay;
	format_time % systime.wHour % systime.wMinute % systime.wSecond;
	format_time % systime.wMilliseconds % str_zone;

	std::string str_level;
	switch (level) {
	case RAPID_LOG_DEBUG:
		str_level = "DEBUG";
		break;
	case RAPID_LOG_INFO:
		str_level = "INFO";
		break;
	case RAPID_LOG_WARN:
		str_level = "WARN";
		break;
	default:
		str_level = "UNKNOWN";
		assert(false);
		break;
	}

	boost::format format_msg("%1% %2% %3%(%4%) %5%\n");
	format_msg % str_level % format_time.str() % func % GetCurrentThreadId() % msg;

	std::string temp(format_msg.str());
	OutputDebugStringA(temp.c_str());

	if (level != RAPID_LOG_DEBUG) {
		std::lock_guard<std::recursive_mutex> auto_lock(log_lock);
		log_writer.WriteData(temp.c_str(), (unsigned)temp.length());
	}
}
