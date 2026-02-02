쓰레드 생성하기

1. C Runtime _beginthreadex로 생성하기
2. STL std::thread로 생성하기
3. WinAPI로 생성하기
  *주의* 내부에서 CRT사용에 필요한 쓰레드 데이터를 초기화를 하지않으므로
  CRT나 STL을 사용한다면 반드시  1,2번 방법으로 생성해야 한다.


실행/디버깅 스위칭(편하게)

01_CreateThread 프로젝트는 `THREADLAB_CREATE_THREAD_ENTRY` 매크로 값으로 엔트리(main)를 바꿔서
Win/CRT 버전과 std::thread 버전을 빠르게 스위칭할 수 있다.

* `THREADLAB_CREATE_THREAD_ENTRY=1` (기본값) : WinAPI + CRT + Std 순서로 실행
* `THREADLAB_CREATE_THREAD_ENTRY=2` : std::thread만 실행

설정 방법(Visual Studio)
프로젝트 속성 -> C/C++ -> 전처리기 -> 전처리기 정의에
`THREADLAB_CREATE_THREAD_ENTRY=2` 처럼 추가하면 된다.



  프로세스는 운영체제 관점에서 특정 프로그램의 실행 단위이다.
각 프로세스는 가상의 논리적인 주소 공간을 가지며, 
운영체제에 의해 물리 메모리에 매핑되어 실행된다.
하나의 프로세스는 여러 개의 쓰레드를 가질 수 있고, 
쓰레드는 프로세스 내부에서 실제 CPU 명령어를 실행하는 흐름의 단위이다.
쓰레드는 프로세스의 자원을 공유하면서  각각 독립적인 스택,레지스터, TLS를 가지고 실행된다.


  프로세스
 ├─ 코드(.text)          ← 공유
 ├─ 전역/정적(.data)     ← 공유
 ├─ 힙(heap)             ← 공유
 └─ 스레드들
     ├─ 스택(Stack)      ← 스레드 전용
     ├─ CPU 레지스터        ← 스레드 전용
     └─ TLS(Thread Local Storage) ← 스레드 전용 