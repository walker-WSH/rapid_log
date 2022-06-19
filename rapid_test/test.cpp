#include <rapid_log.h>
#include <Windows.h>

int main()
{
	init_log(L"G:\\test.txt");

	DWORD start = GetTickCount();
	for (size_t i = 0; i < 2000; i++) {
		LOG_INFO("This is for %s, num: %d", "wsh", 789);
		LOG_INFO("This is for %1%, num: %2%", "test", 123);
		LOG_INFO("This is for %s, num: %d", "demo", "test");

		LOG_INFO("This is for %1%, num: %d", "test", 123);
		LOG_INFO("This is for %1%, num: %2%", "test");
	}
	DWORD end = GetTickCount();
	printf("take %u ms \n\n", end - start);

	uninit_log();

	getchar();
	return 0;
}
