#pragma once

#include <Windows.h>

#include <chrono>
#include <exception>
#include <future>
#include <iostream>
#include <stdexcept>
#include <thread>

// 04_ThreadResult (Std 버전)
//
// 목적
// - 워커 스레드가 계산한 "결과"를 메인 스레드에서 안전하게 받는 방법을 보여줍니다.
//
// 워커가 하는 계산
// - 입력: n
// - 출력: 1부터 n까지의 합 (long long)
//
// 전달(리턴) 방식
// - 워커 스레드는 std::promise<long long>에 값을 set_value()로 채웁니다.
// - 메인 스레드는 std::future<long long>::get()으로 결과를 받습니다.
//   - 결과가 아직 준비되지 않았으면 get()이 "완료될 때까지" 자동으로 대기합니다.
//   - 워커가 set_exception()을 해두면 get()에서 예외가 다시 throw 됩니다.
//
// Shared State(공유 상태)
// - std::promise와 std::future는 "서로 다른 객체"이지만, 내부적으로 같은 shared state를 가리키는 핸들입니다.
// - shared state에는 보통 아래가 들어있습니다.
//   1) 결과값(T) 또는 예외(exception)
//   2) 준비됨(ready) 플래그
//   3) ready까지 기다릴 수 있는 동기화(대기) 메커니즘
// - promise는 생산자(값/예외를 채우는 쪽), future는 소비자(대기 후 꺼내 쓰는 쪽) 역할입니다.
// - future.get()은 "ready가 될 때까지 대기" + "값/예외를 꺼내기"를 한 번에 수행합니다.
//
// 포인트
// - 값 전달과 완료 대기가 하나의 통로(future)로 묶여서 사용 실수가 줄어듭니다.
// - 예외도 안전하게 전파할 수 있습니다.

long long SumUpToStd(int n)
{
	long long total = 0;
	for (int i = 1; i <= n; ++i)
		total += i;
	return total;
}

void PromiseWorker(std::promise<long long> promise, int n, bool shouldFail)
{
	try
	{
		// (데모) 메인 스레드가 기다리는 상황을 보여주기 위해 잠깐 지연
		std::this_thread::sleep_for(std::chrono::milliseconds(250));

		// (데모) 실패 케이스: 예외가 future.get()으로 전달되는지 보여줍니다.
		if (shouldFail)
			throw std::runtime_error("worker failed intentionally");

		if (n < 0)
			throw std::invalid_argument("n must be >= 0");

		// 정상 케이스: 계산 결과를 promise에 저장(=메인 스레드에게 전달)
		promise.set_value(SumUpToStd(n));
	}
	catch (...)
	{
		// 실패 케이스: 예외를 promise에 저장(=메인 스레드 get()에서 다시 throw)
		promise.set_exception(std::current_exception());
	}
}


int SMain()
{
	std::cout << "04_ThreadResult (std::promise / std::future)\n";
	std::cout << "main tid=" << ::GetCurrentThreadId() << "\n\n";

	constexpr int n = 100000;

	{
		// 성공 케이스
		std::promise<long long> promise;
		// get_future()를 호출하는 순간, promise와 future가 같은 shared state를 공유하게 됩니다.
		std::future<long long> future = promise.get_future();

		std::thread worker(&PromiseWorker, std::move(promise), n, false);

		std::cout << "[Main] waiting for result...\n";
		try
		{
			// 결과가 준비될 때까지 대기 후, 값 수신
			const long long result = future.get();
			std::cout << "[Main] result=" << result << "\n";
		}
		catch (const std::exception& e)
		{
			std::cout << "[Main] exception: " << e.what() << "\n";
		}

		worker.join();
	}

	std::cout << "\n";

	{
		// 실패 케이스
		std::promise<long long> promise;
		// shared state는 값뿐 아니라 "예외"도 담을 수 있습니다.
		std::future<long long> future = promise.get_future();

		std::thread worker(&PromiseWorker, std::move(promise), n, true);

		std::cout << "[Main] waiting for result (failure case)...\n";
		try
		{
			// 워커가 set_exception()하면 여기서 예외가 발생
			const long long result = future.get();
			std::cout << "[Main] result=" << result << "\n";
		}
		catch (const std::exception& e)
		{
			std::cout << "[Main] exception: " << e.what() << "\n";
		}

		worker.join();
	}

	return 0;
}
