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

#### `promise`와 `future`가 공유하는 Shared State란?

`std::promise<T>`와 `std::future<T>`는 각각 "생산자/소비자" 역할의 **핸들(handle)** 이고,
실제 데이터는 둘이 함께 가리키는 내부의 **shared state(공유 상태)** 에 저장됩니다.

shared state에는 보통 아래 정보가 들어 있습니다.

- 결과값 `T` 또는 예외(exception)
- 준비됨(ready) 상태
- ready까지 기다릴 수 있는 동기화(대기) 메커니즘

그래서 `future.get()`은

- 아직 ready가 아니면 자동으로 기다리고
- ready가 되면 결과값을 반환하거나(정상)
- 저장된 예외를 다시 throw(실패)

를 한 번에 수행합니다.

이게 WinAPI의 `value/error + doneEvent`와 같은 "공유 상태 + 완료 신호" 패턴을
표준 라이브러리가 실수하기 어렵게(통로로 강제되게) 묶어 준 형태라고 보면 됩니다.

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

#### 왜 WinAPI 쪽이 오히려 더 위험해 보일 수 있나?

WinAPI 방식은 "결과 저장"과 "완료 알림"이 분리되어 있고, 결과 필드(`value/error`)는 외부에서 언제든 접근 가능합니다.
그래서 아래 같은 실수가 구조적으로 더 쉽게 발생합니다.

- **Wait 전에 결과를 읽음**: 결과가 아직 준비되지 않았거나(논리 오류), 워커가 동시에 쓰는 중일 수 있어(데이터 레이스) 위험합니다.
- **순서 실수**: 워커가 `SetEvent(doneEvent)`를 결과 기록보다 먼저 해버리면, 메인이 깨어나서 미완성 결과를 읽을 수 있습니다.

반대로 Std의 `future.get()`은 "대기"와 "결과 수신"이 한 통로로 묶여 있어서,
사용자가 결과를 읽는 시점에 이미 완료 대기가 강제되는 형태입니다.

### 실행

- 기본 `main()`은 `SMain()`(Std)을 호출합니다.
- WinAPI 버전을 실행하려면 `04_ThreadResult.cpp`에서 `WMain()` 호출로 바꾸면 됩니다.
