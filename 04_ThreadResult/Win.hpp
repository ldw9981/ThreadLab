#pragma once

#include <Windows.h>
#include <process.h> // _beginthreadex

#include <cerrno>
#include <iostream>

// 04_ThreadResult (WinAPI 버전)
//
// 목적
// - WinAPI만으로 "스레드 결과"를 메인 스레드에서 받는 전형적인 형태를 보여줍니다.
//
// 워커가 하는 계산
// - 입력: n
// - 출력: 1부터 n까지의 합 (long long)
//
// 전달(리턴) 방식
// - std::future처럼 라이브러리 통로가 있는 게 아니라, 직접 "공유 상태(shared state)"를 둡니다.
//   - value : 정상 결과
//   - error : 실패 원인(에러 코드)
// - doneEvent를 SetEvent()로 신호 상태로 만들면 "결과 준비 완료"를 의미합니다.
// - 메인 스레드는 doneEvent를 기다린 뒤 value/error를 읽습니다.
//
// 포인트
// - 이 방식의 핵심은 "완료 신호를 기다린 뒤에만 공유 상태를 읽는다"는 규칙(프로토콜)입니다.
// - 이 규칙을 어기면(예: Wait 전에 value/error를 읽기) 데이터 레이스/미완성 상태 노출로 이어질 수 있습니다.
//   즉, WinAPI는 '통로가 강제'되지 않기 때문에 오히려 실수에 더 취약해질 수 있습니다.
// - 예외 전파는 자동으로 되지 않으므로, error code/HRESULT/예외 포인터 등으로 직접 설계해야 합니다.


struct WinResultState
{
	HANDLE doneEvent = nullptr; // signaled when result is ready
	// 주의: 아래 value/error는 doneEvent가 signaled 된 뒤에만 읽어야 합니다.
	// (그 전에 읽는 것은 논리적으로도 틀리고, 동기화 관점에서도 위험합니다.)
	long long value = 0;
	DWORD error = 0; // 0 == OK
	int n = 0;
	bool shouldFail = false;
};

long long SumUpTo(int n)
{
	long long total = 0;
	for (int i = 1; i <= n; ++i)
		total += i;
	return total;
}

unsigned __stdcall WinWorkerProc(void* param)
{
	auto* state = static_cast<WinResultState*>(param);
	state->error = 0;
	state->value = 0;

	// (데모) 메인 스레드가 기다리는 상황을 보여주기 위해 잠깐 지연
	::Sleep(250);

	// 실패/성공 결과를 shared state에 기록
	if (state->shouldFail)
	{
		state->error = ERROR_GEN_FAILURE;
	}
	else if (state->n < 0)
	{
		state->error = ERROR_INVALID_DATA;
	}
	else
	{
		state->value = SumUpTo(state->n);
	}

	// 완료 신호 publish
	// 중요: 완료 신호는 반드시 "결과 기록이 끝난 뒤" 마지막에 보내야 합니다.
	// (만약 SetEvent를 먼저 호출해버리면, 메인 스레드는 깨어나서 미완성 value/error를 읽을 수 있습니다.)
	::SetEvent(state->doneEvent);
	return 0;
}


int WMain()
{
	std::cout << "04_ThreadResult (WinAPI: Event + shared state)\n";
	std::cout << "main tid=" << ::GetCurrentThreadId() << "\n\n";

	WinResultState state;
	state.n = 100000;
	state.shouldFail = false;

	state.doneEvent = ::CreateEvent(nullptr, TRUE, FALSE, nullptr);
	if (state.doneEvent == nullptr)
	{
		std::cout << "CreateEvent failed. GetLastError=" << ::GetLastError() << "\n";
		return 1;
	}

	unsigned workerTid = 0;
	const uintptr_t workerHandleRaw = _beginthreadex(
		nullptr,
		0,
		&WinWorkerProc,
		&state,
		0,
		&workerTid);

	if (workerHandleRaw == 0)
	{
		std::cout << "_beginthreadex failed. errno=" << errno << "\n";
		::CloseHandle(state.doneEvent);
		return 1;
	}

	HANDLE workerHandle = reinterpret_cast<HANDLE>(workerHandleRaw);
	std::cout << "worker tid=" << workerTid << "\n";

	// (의도적으로 주석 처리) 아래처럼 Wait 전에 읽으면, 결과가 아직 준비되지 않았을 수 있습니다.
	// 또한 워커가 동시에 쓰는 중일 수 있어 데이터 레이스가 됩니다.
	// std::cout << "[Main] (WRONG) early read value=" << state.value << "\n";

	std::cout << "[Main] waiting for doneEvent...\n";
	// 완료 신호를 기다린 뒤에만 value/error를 읽습니다.
	::WaitForSingleObject(state.doneEvent, INFINITE);

	if (state.error == 0)
		std::cout << "[Main] result=" << state.value << "\n";
	else
		std::cout << "[Main] error=" << state.error << "\n";

	::WaitForSingleObject(workerHandle, INFINITE);
	::CloseHandle(workerHandle);
	::CloseHandle(state.doneEvent);
	return 0;
}
