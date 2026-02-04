#pragma once

#include <Windows.h>
#include <process.h> // _beginthreadex
#include <iostream>


constexpr DWORD kSleepMs = 2000;

DWORD WINAPI WinThreadProc(LPVOID)
{
	const char* tag = "::CreateThread()";
	const DWORD tid = ::GetCurrentThreadId();
	std::cout << "[" << tag << "] thread start. tid=" << tid << "\n";
	::Sleep(kSleepMs);
	std::cout << "[" << tag << "] thread end.   tid=" << tid << "\n";
    return 0;
}

// CreateThread함수는 내부에서 CRT 초기화를 수행하지 않으므로 CRT사용을위한 쓰레드 당 데이터가 준비되지 않는다.
// 따라서 CRT 를 사용하는 경우 CreateThread 사용을 권장하지 않는다.
void RunWinApiThread()
{
    std::cout << "\n=== WinAPI CreateThread version ===\n";
    DWORD threadId = 0;
    HANDLE hThread = ::CreateThread(
        nullptr,
        0,
        &WinThreadProc,
        nullptr,
        0,
        &threadId);

    if (hThread == nullptr)
    {
        std::cout << "[WinAPI] CreateThread failed. GetLastError=" << ::GetLastError() << "\n";
        return;
    }

    std::cout << "[WinAPI] created. threadId=" << threadId << " (main tid=" << ::GetCurrentThreadId() << ")\n";
    ::WaitForSingleObject(hThread, INFINITE);
    ::CloseHandle(hThread);
    std::cout << "[WinAPI] joined (WaitForSingleObject done).\n";
}

unsigned __stdcall CrtThreadProc(void*)
{
	const char* tag = "::_beginthreadex()";
	const DWORD tid = ::GetCurrentThreadId();
	std::cout << "[" << tag << "] thread start. tid=" << tid << "\n";
	::Sleep(kSleepMs);
	std::cout << "[" << tag << "] thread end.   tid=" << tid << "\n";
    return 0;
}

void RunCrtThread()
{
    std::cout << "\n=== C (CRT) _beginthreadex version ===\n";
    unsigned threadId = 0;

    // _beginthreadex는 CRT(C 런타임)에서 제공하는 쓰레드 생성 함수.
    // 반환 핸들은 WaitForSingleObject/CloseHandle로 정리한다.
    const uintptr_t hThreadRaw = _beginthreadex(
        nullptr,
        0,
        &CrtThreadProc,
        nullptr,
        0,
        &threadId);

    if (hThreadRaw == 0)
    {
        std::cout << "[CRT] _beginthreadex failed. errno=" << errno << "\n";
        return;
    }

    HANDLE hThread = reinterpret_cast<HANDLE>(hThreadRaw);
    std::cout << "[CRT] created. threadId=" << threadId << " (main tid=" << ::GetCurrentThreadId() << ")\n";
    ::WaitForSingleObject(hThread, INFINITE);
    ::CloseHandle(hThread);
    std::cout << "[CRT] joined (WaitForSingleObject done).\n";
}


int WMain()
{
    std::cout << "01_CreateThread - Win/CRT \n";
    RunWinApiThread();
    RunCrtThread();
    return 0;
}

