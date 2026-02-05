04_ThreadResult
======================

### 목표

워커 스레드가 만든 결과를 메인 스레드에서 **안전하게 수신**하는 방법을 비교합니다.

이 프로젝트에서 워커 스레드가 하는 계산은 아래와 같습니다.

- 입력: 정수 `n`
- 계산: $1$부터 $n$까지의 합
  - 예: `n = 5`이면 `1+2+3+4+5 = 15`
- 결과 타입: `long long`

즉, 워커는 `SumUpTo(n)`을 계산하고 그 값을 메인 스레드로 전달합니다.

- Std 버전: `std::promise` / `std::future`
  - 완료 대기 + 결과 수신을 한 통로로 묶음
  - 값뿐 아니라 예외도 전파 가능 (`future.get()`)

- WinAPI 버전: Event + shared state
  - 완료 신호(Event)를 기다린 뒤 결과 구조체를 읽는 방식
  - 예외 전파는 직접 설계(예: error code)해야 함

---

### Std 버전 (std::promise / std::future)

핵심 아이디어는 “워커가 `promise`에 값을 채워 넣고, 메인 스레드는 `future.get()`으로 결과를 받는다” 입니다.

- 워커 스레드가 하는 일
  - 정상 케이스: `SumUpTo(n)` 계산 후 `promise.set_value(result)`
  - 실패 케이스: 예외를 던진 뒤 `promise.set_exception(...)`

- 메인 스레드가 하는 일
  - `future.get()`을 호출하면 결과가 준비될 때까지 자동으로 대기합니다.
  - 정상 케이스: `get()`이 `long long` 결과를 반환합니다.
  - 실패 케이스: 워커에서 설정한 예외가 `get()` 시점에 다시 throw 됩니다.

이 프로젝트의 Std 데모는
- 성공 케이스 1회(값 전달)
- 실패 케이스 1회(예외 전달)
를 연속으로 보여줍니다.

---

### WinAPI 버전 (Event + shared state)

핵심 아이디어는 “워커가 공유 상태(shared state)에 결과를 써두고, Event로 완료를 알린다. 메인은 Event를 기다렸다가(shared state를) 읽는다” 입니다.

- 워커 스레드가 하는 일
  - 정상 케이스: `value`에 `SumUpTo(n)` 결과를 저장
  - 실패 케이스: `error`에 에러 코드를 저장 (예: `ERROR_GEN_FAILURE`)
  - 마지막에 `SetEvent(doneEvent)`로 완료 신호를 보냄

- 메인 스레드가 하는 일
  - `WaitForSingleObject(doneEvent, INFINITE)`로 완료 신호를 기다림
  - 완료 후
    - `error == 0`이면 `value`를 결과로 사용
    - `error != 0`이면 실패로 처리

여기서 “안전”은 라이브러리가 자동으로 보장해주는 의미가 아니라,
"완료 신호를 기다린 뒤에만 결과를 읽는다"라는 프로토콜(규칙)을 지켜서 데이터 레이스를 피하는 의미입니다.

### 실행

- 기본 `main()`은 `SMain()`(Std)을 호출합니다.
- WinAPI 버전을 실행하려면 `04_ThreadResult.cpp`에서 `WMain()` 호출로 바꾸면 됩니다.
