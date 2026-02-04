
#pragma once

#include <Windows.h>

#include <process.h> // _beginthreadex

#include <cerrno>
#include <iostream>

extern long long g_Total;


struct WinThreadArgs
{
	CRITICAL_SECTION* cs = nullptr;
	int max = 0;
};

unsigned __stdcall AccumulateWinThreadProc(void* param)
{
	const auto* args = static_cast<const WinThreadArgs*>(param);
	for (int i = 1; i <= args->max; ++i)
	{
		::EnterCriticalSection(args->cs);
		g_Total += i;
		::LeaveCriticalSection(args->cs);
	}
	return 0;
}


int WMain()
{
	constexpr int kThreadCount = 10000;
	constexpr int kMax = 10000;

	g_Total = 0;
	CRITICAL_SECTION cs;
	::InitializeCriticalSection(&cs);

	WinThreadArgs args;
	args.cs = &cs;
	args.max = kMax;

	std::cout << "03_MutualExclusion (WinAPI _beginthreadex + CRITICAL_SECTION)\n";
	std::cout << "main tid=" << ::GetCurrentThreadId() << "\n";

	HANDLE threads[kThreadCount] = {};
	unsigned tids[kThreadCount] = {};

	for (int t = 0; t < kThreadCount; ++t)
	{
		const uintptr_t h = _beginthreadex(nullptr, 0, &AccumulateWinThreadProc, &args, 0, &tids[t]);
		threads[t] = reinterpret_cast<HANDLE>(h);
		if (threads[t] == nullptr)
		{
			std::cout << "_beginthreadex failed. errno=" << errno << "\n";
			for (int i = 0; i < t; ++i)
				::CloseHandle(threads[i]);
			::DeleteCriticalSection(&cs);
			return 1;
		}
	}

	::WaitForMultipleObjects(kThreadCount, threads, TRUE, INFINITE);
	for (HANDLE h : threads)
		::CloseHandle(h);
	::DeleteCriticalSection(&cs);

	const long long expectedPerThread = (static_cast<long long>(kMax) * (kMax + 1)) / 2;
	const long long expected = expectedPerThread * kThreadCount;

	std::cout << "expected=" << expected << "\n";
	std::cout << "g_Total =" << g_Total << "\n";
	return 0;
}

