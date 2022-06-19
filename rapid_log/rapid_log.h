#pragma once
#include <assert.h>
#include <iostream>
#include <boost/format.hpp>

enum rapid_log_level
{
        RAPID_LOG_DEBUG = 0,
        RAPID_LOG_INFO,
        RAPID_LOG_WARN,
};

extern "C" bool init_log(const wchar_t* log_file);
extern "C" void rapid_log(rapid_log_level level, const char* file, const char* func, int line, const char* msg);
extern "C" void uninit_log();

template<typename... Arguments>
void format_log(rapid_log_level level, const char* file, const char* func, int line, const char* fmt, Arguments&&... args)
{
        try
        {
                boost::format f(fmt);

                int unroll[]{ 0, (f % std::forward<Arguments>(args), 0)... };
                static_cast<void>(unroll);

                std::string msg = boost::str(f);
                rapid_log(level, file, func, line, msg.c_str());
        }
        catch (std::exception& e)
        {
                rapid_log(RAPID_LOG_WARN, file, func, line, e.what());
                //assert(false);
        }
        catch (...)
        {
                rapid_log(RAPID_LOG_WARN, file, func, line, "format error");
                //assert(false);
        }
}

#define LOG_DEBUG(fmt, ...) format_log(RAPID_LOG_DEBUG, __FILE__, __FUNCTION__, __LINE__, fmt, __VA_ARGS__)
#define LOG_INFO(fmt, ...) format_log(RAPID_LOG_INFO, __FILE__, __FUNCTION__, __LINE__, fmt, __VA_ARGS__)
#define LOG_WARN(fmt, ...) format_log(RAPID_LOG_WARN, __FILE__, __FUNCTION__, __LINE__, fmt, __VA_ARGS__)