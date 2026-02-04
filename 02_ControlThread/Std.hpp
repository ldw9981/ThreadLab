#pragma once

#pragma once

#include <Windows.h>

#include <conio.h> // _kbhit, _getch

#include <chrono>
#include <condition_variable>
#include <iostream>
#include <mutex>
#include <thread>

namespace
{
	struct StdThreadControl
	{
		bool running = true;         // 실행/일시정지 상태값
		bool exitRequested = false;  // 종료 요청 상태값
		std::mutex m;                // 상태값 접근 보호용
		std::condition_variable cv;  // 조건 대기 도구
	};

	void TickTockWorker(StdThreadControl* ctrl)
	{
		bool tick = true;
		while (true)
		{
			{
				std::unique_lock<std::mutex> lock(ctrl->m);
				ctrl->cv.wait(lock, [&] { return ctrl->exitRequested || ctrl->running; });
				if (ctrl->exitRequested)
					break;
			}

			std::cout << (tick ? "Tick" : "Tock") << "\n";
			tick = !tick;
			std::this_thread::sleep_for(std::chrono::seconds(1));
		}
	}
}


inline int SMain()
{
	std::cout << "02_ControlThread (std::thread) - Tick/Tock worker (Press T to Pause/Continue, Q to Quit)\n";
	std::cout << "main tid=" << ::GetCurrentThreadId() << "\n\n";

	StdThreadControl ctrl;
	std::thread worker(&TickTockWorker, &ctrl);

	// 메인 스레드: 키 입력으로 워커를 제어
	// - T: Pause <-> Continue
	// - Q: 종료
	bool running = true;
	while (true)
	{
		if (_kbhit())
		{
			const int ch = _getch();
			if (ch == 't' || ch == 'T')
			{
				running = !running;
				// Scope-based lock 을 사용
				{
					std::lock_guard<std::mutex> lock(ctrl.m);
					ctrl.running = running;
				}
				// wait(), wait_for(), wait_until()로 대기 중인 모든 스레드를 깨웁니다.
				ctrl.cv.notify_all();
				std::cout << (running ? "[Main] Continue\n" : "[Main] Pause\n");
			}
			else if (ch == 'q' || ch == 'Q')
			{
				std::cout << "[Main] Quit\n";
				// Scope-based lock 을 사용
				{
					std::lock_guard<std::mutex> lock(ctrl.m);
					ctrl.exitRequested = true;
					ctrl.running = true;
				}
				// wait(), wait_for(), wait_until()로 대기 중인 모든 스레드를 깨웁니다.
				ctrl.cv.notify_all();
				break;
			}
		}

		// 퀀텀까지 소비하지 않고 다른 쓰레드에 양보함
		std::this_thread::sleep_for(std::chrono::milliseconds(30));
	}

	// 생성한 쓰레드의 종료까지 대기
	worker.join();
	return 0;
}

