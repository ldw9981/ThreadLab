#pragma once

#include <Windows.h>
#include <thread>
#include <iostream>

constexpr DWORD kSleepMs = 2000;

void ThreadProc()
{
	const char* tag = "std::thread";
	const DWORD tid = ::GetCurrentThreadId();
	std::cout << "[" << tag << "] thread start. tid=" << tid << "\n";
	::Sleep(kSleepMs);
	std::cout << "[" << tag << "] thread end.   tid=" << tid << "\n";
}


int SMain()
{
    std::cout << "01_CreateThread - std::thread)\n";
	std::cout << "\n=== C++ std::thread version ===\n";
	std::thread worker(&ThreadProc);
	worker.join(); // ::WaitForSingleObject(hThread, INFINITE) 과 동일
	std::cout << "[std::thread] joined (join done).\n";

    return 0;
}

