#pragma once

#pragma once

#include <Windows.h>
#include <conio.h>   // _kbhit, _getch
#include <process.h> // _beginthreadex

#include <iostream>

namespace
{
	struct WinThreadControl
	{
		// manual-reset은 이벤트를 수동으로 재설정할 때까지 신호 상태를 유지함을 의미합니다.
		// auto-reset은 이벤트가 신호 상태일 때 단일 대기 스레드를 깨우고 자동으로 비신호 상태로 재설정됨을 의미합니다.
		HANDLE exitEvent = nullptr; // manual-reset, signaled => exit
		HANDLE runEvent = nullptr;  // manual-reset, signaled => running, reset => paused
	};

	unsigned __stdcall TickTockThreadProc(void* param)
	{
		const auto* ctrl = static_cast<const WinThreadControl*>(param);
		bool tick = true;

		while (true)
		{
			HANDLE waits[2] = { ctrl->exitEvent, ctrl->runEvent };
			const DWORD w = ::WaitForMultipleObjects(2, waits, FALSE, INFINITE);
			if (w == WAIT_OBJECT_0)
				break;

			std::cout << (tick ? "Tick" : "Tock") << "\n";
			tick = !tick;
			::Sleep(1000);
		}

		return 0;
	}
}


inline int WMain()
{
	std::cout << "02_ControlThread - Tick/Tock worker (Press T to Pause/Continue, Q to Quit)\n";
	std::cout << "main tid=" << ::GetCurrentThreadId() << "\n\n";

	WinThreadControl ctrl;
	/*
		HANDLE CreateEvent(LPSECURITY_ATTRIBUTES lpEventAttributes,
		BOOL  bManualReset,BOOL  bInitialState, LPCWSTR lpName);
	*/
	ctrl.exitEvent = ::CreateEvent(nullptr, TRUE, FALSE, nullptr);
	ctrl.runEvent = ::CreateEvent(nullptr, TRUE, TRUE, nullptr);

	if (ctrl.exitEvent == nullptr || ctrl.runEvent == nullptr)
	{
		std::cout << "CreateEvent failed. GetLastError=" << ::GetLastError() << "\n";
		if (ctrl.exitEvent) ::CloseHandle(ctrl.exitEvent);
		if (ctrl.runEvent) ::CloseHandle(ctrl.runEvent);
		return 1;
	}

	unsigned workerTid = 0;
	const uintptr_t workerHandleRaw = _beginthreadex(
		nullptr,
		0,
		&TickTockThreadProc,
		&ctrl,
		0,
		&workerTid);

	if (workerHandleRaw == 0)
	{
		std::cout << "_beginthreadex failed. errno=" << errno << "\n";
		::CloseHandle(ctrl.exitEvent);
		::CloseHandle(ctrl.runEvent);
		return 1;
	}

	HANDLE workerHandle = reinterpret_cast<HANDLE>(workerHandleRaw);
	std::cout << "worker tid=" << workerTid << "\n";

	bool running = true;
	while (true)
	{
		if (_kbhit())
		{
			const int ch = _getch();
			if (ch == 't' || ch == 'T')
			{
				running = !running;
				if (running)
				{
					::SetEvent(ctrl.runEvent);  // signaled
					std::cout << "[Main] Continue\n";
				}
				else
				{
					::ResetEvent(ctrl.runEvent); // non-signaled
					std::cout << "[Main] Pause\n";
				}
			}
			else if (ch == 'q' || ch == 'Q')
			{
				std::cout << "[Main] Quit\n";
				::SetEvent(ctrl.exitEvent);
				break;
			}
		}

		// 쓰레드의 종료 여부의 검사도 가능하다.
		//스레드가 실행 중이면 → non-signaled , 스레드가 종료(Terminated) 되면 → signaled
		const DWORD done = ::WaitForSingleObject(workerHandle, 0);
		if (done == WAIT_OBJECT_0)
			break;

		// 퀀텀까지 소비하지 않고 다른 쓰레드에 양보함
		::Sleep(30);
	}

	::WaitForSingleObject(workerHandle, INFINITE);
	::CloseHandle(workerHandle);
	::CloseHandle(ctrl.exitEvent);
	::CloseHandle(ctrl.runEvent);
	return 0;
}

