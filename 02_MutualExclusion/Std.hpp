#pragma once

#include <Windows.h>

#include <iostream>
#include <mutex>
#include <thread>

extern long long g_Total;


struct StdThreadArgs
{
	std::mutex* totalMutex = nullptr;
	int max = 0;
};

void AccumulateStdThreadProc(StdThreadArgs* args)
{
	for (int i = 1; i <= args->max; ++i)
	{
		std::lock_guard<std::mutex> lock(*args->totalMutex);
		g_Total += i;
	}
}


int SMain()
{
	constexpr int kThreadCount = 10000;
	constexpr int kMax = 10000;

	g_Total = 0;
	std::mutex totalMutex;

	StdThreadArgs args;
	args.totalMutex = &totalMutex;
	args.max = kMax;

	std::cout << "02_MutualExclusion (std::thread + std::mutex)\n";
	std::cout << "main tid=" << ::GetCurrentThreadId() << "\n";

	std::thread threads[kThreadCount];
	for (int t = 0; t < kThreadCount; ++t)
		threads[t] = std::thread(&AccumulateStdThreadProc, &args);
	for (auto& th : threads)
		th.join();

	const long long expectedPerThread = (static_cast<long long>(kMax) * (kMax + 1)) / 2;
	const long long expected = expectedPerThread * kThreadCount;

	std::cout << "expected=" << expected << "\n";
	std::cout << "g_Total =" << g_Total << "\n";
	return 0;
}