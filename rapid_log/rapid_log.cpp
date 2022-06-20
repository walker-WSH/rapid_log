#include "rapid_log.h"
#include "memory_map_writer.h"
#include <Windows.h>
#include <Shlwapi.h>
#include <string>
#include <mutex>

#pragma comment(lib, "Shlwapi.lib")

std::recursive_mutex log_lock;
CMemoryMapWriter log_writer;
std::string str_zone = "";

extern "C" bool init_log(const wchar_t *log_file)
{
	TIME_ZONE_INFORMATION tmp;
	::GetTimeZoneInformation(&tmp);

	std::lock_guard<std::recursive_mutex> auto_lock(log_lock);

	if (!str_zone.empty()) {
		assert(false);
		return false;
	}

	str_zone = std::to_string(tmp.Bias / (-60));
	return log_writer.Open(log_file, true);
}

extern "C" void uninit_log()
{
	std::lock_guard<std::recursive_mutex> auto_lock(log_lock);
	str_zone = "";
	log_writer.Close();
}

std::string get_log_level(rapid_log_level level) 
{
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
	case RAPID_LOG_UISTEP:
		str_level = "UISTEP";
		break;
	default:
		str_level = "UNKNOWN";
		assert(false);
		break;
	}
	return str_level;
}

extern "C" void rapid_log(rapid_log_level level, const char *file, const char *func, int line, const char *msg)
{
#ifndef DEBUG
	if (level == RAPID_LOG_DEBUG)
		return;
#endif

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

	boost::format format_msg("%1% [%2%:%3%] %4%(%5%) %6% %7%\n");
	format_msg % format_time.str() % PathFindFileNameA(file) % line % func % GetCurrentThreadId() % get_log_level(level) % msg;

	std::string temp(format_msg.str());
	OutputDebugStringA(temp.c_str());

	if (level != RAPID_LOG_DEBUG) {
		std::lock_guard<std::recursive_mutex> auto_lock(log_lock);
		log_writer.WriteData(temp.c_str(), (unsigned)temp.length());
	}
}
