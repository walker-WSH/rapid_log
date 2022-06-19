#include <rapid_log.h>

int main()
{
	init_log(L"G:\\test.txt");

	LOG_INFO("This is for %s, num: %d", "wsh", 789);
	LOG_INFO("This is for %1%, num: %2%", "test", 123);
	LOG_INFO("This is for %s, num: %d", "demo", "test");

	LOG_INFO("This is for %1%, num: %d", "test", 123);
	LOG_INFO("This is for %1%, num: %2%", "test");

	uninit_log();

	return 0;
}
